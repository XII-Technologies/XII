/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginAssets/Util/MeshTextureImport.h>
#include <ModelImporter/MeshImport.h>

namespace
{
  xiiStatus ValidateTextures(const xiiModelMeshMaterial& material)
  {
    xiiHashSet<xiiUInt32> usages;
    for (const auto& texture : material.m_Textures)
    {
      if (usages.Contains(static_cast<xiiUInt32>(texture.m_Usage)))
        return xiiStatus(xiiFmt("Material '{}' uses layered textures. The native material renderer has one texture per slot.", material.m_sName));
      usages.Insert(static_cast<xiiUInt32>(texture.m_Usage));
      if (texture.m_uiUVChannel != 0 || texture.m_vUVOffset != xiiVec2::MakeZero() || texture.m_vUVScale != xiiVec2(1) || texture.m_fUVRotation != 0 || texture.m_fBlend != 1 || texture.m_Operation != xiiModelMaterialTexture::Operation::Multiply)
        return xiiStatus(xiiFmt("Material '{}' requires per-texture UV mapping or layered blending that the native renderer does not expose. Assign a custom material or import geometry without materials.", material.m_sName));
    }
    return XII_SUCCESS;
  }

  xiiStatus ConfigureMaterial(xiiMaterialAssetDocument& document, const xiiModelMeshMaterial& material, const xiiModelMeshImport& imported, xiiStringView sourceDirectory, xiiStringView targetDirectory)
  {
    auto* accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Import Native Material");
    bool committed = false;
    XII_SCOPE_EXIT(if (!committed) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(document.GetPropertyObject(), "ShaderMode", static_cast<xiiInt32>(xiiMaterialShaderMode::Native)));
    const auto* properties = accessor->GetChildObjectByName(document.GetPropertyObject(), "NativeMaterial", xiiVariant());
    if (properties == nullptr)
      return xiiStatus("Native material descriptor is missing from the document.");
    xiiColor baseColor = material.m_BaseColor;
    baseColor.a *= material.m_fOpacity;
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "BaseColor", baseColor));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "EmissiveColor", material.m_EmissiveColor));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "Metallic", material.m_fMetallic));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "Roughness", material.m_fRoughness));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "AlphaCutoff", material.m_fAlphaCutoff));
    const auto alpha = material.m_AlphaMode == xiiModelMeshMaterial::AlphaMode::Mask ? xiiMaterialAlphaMode::Mask :
                                                                                       (material.m_AlphaMode == xiiModelMeshMaterial::AlphaMode::Blend ? xiiMaterialAlphaMode::Blend : xiiMaterialAlphaMode::Opaque);
    const auto blend = material.m_AlphaMode == xiiModelMeshMaterial::AlphaMode::Mask ? xiiMaterialBlendMode::Masked :
                                                                                       (material.m_AlphaMode == xiiModelMeshMaterial::AlphaMode::Blend ? xiiMaterialBlendMode::Translucent : xiiMaterialBlendMode::Opaque);
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "AlphaMode", static_cast<xiiInt32>(alpha)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "BlendMode", static_cast<xiiInt32>(blend)));
    auto flags = xiiBitflags<xiiMaterialFeatureFlags>(xiiMaterialFeatureFlags::Default);
    flags.AddOrRemove(xiiMaterialFeatureFlags::TwoSided, material.m_bTwoSided);
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "Features", static_cast<xiiInt64>(flags.GetValue())));
    const xiiModelMaterialTexture* roughness = nullptr;
    const xiiModelMaterialTexture* metallic  = nullptr;
    for (const auto& texture : material.m_Textures)
    {
      if (texture.m_Usage == xiiModelTextureUsage::Roughness)
      {
        roughness = &texture;
        continue;
      }
      if (texture.m_Usage == xiiModelTextureUsage::Metallic)
      {
        metallic = &texture;
        continue;
      }
      const char* property = nullptr;
      auto        usage    = xiiTextureConverterUsage::Linear;
      switch (texture.m_Usage)
      {
        case xiiModelTextureUsage::BaseColor:
          property = "BaseColorTexture";
          usage    = xiiTextureConverterUsage::Color;
          break;
        case xiiModelTextureUsage::Normal:
          property = "NormalTexture";
          usage    = xiiTextureConverterUsage::NormalMap;
          XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "NormalScale", texture.m_fStrength));
          break;
        case xiiModelTextureUsage::MetallicRoughness:
          property = "MetallicRoughnessTexture";
          break;
        case xiiModelTextureUsage::Occlusion:
          property = "OcclusionTexture";
          XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "OcclusionStrength", texture.m_fStrength));
          break;
        case xiiModelTextureUsage::Emissive:
          property = "EmissiveTexture";
          usage    = xiiTextureConverterUsage::Color;
          break;
        case xiiModelTextureUsage::Height:
          property = "HeightTexture";
          break;
        default:
          return xiiStatus("Unsupported native material texture usage.");
      }
      xiiString asset;
      XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::ImportTexture(texture, imported, sourceDirectory, targetDirectory, usage, texture.m_Usage == xiiModelTextureUsage::BaseColor, asset));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, property, asset));
    }
    if (roughness != nullptr || metallic != nullptr)
    {
      xiiString asset;
      XII_SUCCEED_OR_RETURN(xiiMeshImportUtils::ImportMetallicRoughnessTexture(roughness, metallic, imported, sourceDirectory, targetDirectory, asset));
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(properties, "MetallicRoughnessTexture", asset));
    }
    accessor->FinishTransaction();
    committed = true;
    return document.SaveDocument();
  }
} // namespace

void xiiMeshImportUtils::SetMeshAssetMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, const xiiModelMeshImport& imported)
{
  xiiHybridArray<xiiStringView, 16> labels;
  labels.Reserve(imported.m_Materials.GetCount());
  for (const auto& material : imported.m_Materials)
    labels.PushBack(material.m_sName);
  xiiMeshMaterialSlots::Reconcile(labels, inout_slots);
}

xiiResult xiiMeshImportUtils::CopyMeshAssetMaterialSlotToResource(xiiMeshResourceDescriptor& inout_descriptor, xiiArrayPtr<const xiiMaterialResourceSlot> materialSlots)
{
  if (materialSlots.GetCount() != inout_descriptor.GetMaterials().GetCount())
    return XII_FAILURE;
  for (xiiUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    inout_descriptor.SetMaterial(i, materialSlots[i].m_sResource);
  return XII_SUCCESS;
}

xiiStatus xiiMeshImportUtils::ImportMeshAssetMaterials(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, xiiStringView documentPath, xiiStringView sourceFile, const xiiModelMeshImport& imported)
{
  if (inout_slots.GetCount() != imported.m_Materials.GetCount())
    return xiiStatus("Native mesh material slot count does not match imported material definitions.");
  // Validate the whole pending import before creating any documents.
  for (xiiUInt32 i = 0; i < inout_slots.GetCount(); ++i)
    if (inout_slots[i].m_sResource.IsEmpty())
      XII_SUCCEED_OR_RETURN(ValidateTextures(imported.m_Materials[i]));
  xiiStringBuilder targetDirectory(documentPath);
  targetDirectory.RemoveFileExtension();
  targetDirectory.Append("_data");
  XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(targetDirectory));
  const auto                         sourceDirectory = sourceFile.GetFileDirectory();
  auto                               materialSlots   = inout_slots;
  xiiHashTable<xiiString, xiiUInt32> nameCounts;
  xiiHybridArray<xiiString, 16>      names;
  xiiHashSet<xiiString>              usedNames;
  for (const auto& material : imported.m_Materials)
  {
    xiiStringBuilder name;
    xiiPathUtils::MakeValidFilename(material.m_sName, '_', name);
    if (name.IsEmpty())
      name = "Material";
    names.PushBack(name);
    name.ToLower();
    ++nameCounts[name];
    usedNames.Insert(name);
  }
  for (xiiUInt32 i = 0; i < materialSlots.GetCount(); ++i)
  {
    if (!materialSlots[i].m_sResource.IsEmpty())
      continue;
    xiiStringBuilder name(names[i]), path;
    xiiStringBuilder key(name);
    key.ToLower();
    if (nameCounts[key] > 1)
    {
      do
      {
        name.AppendFormat("_{}", i);
        key = name;
        key.ToLower();
      } while (usedNames.Contains(key));
      usedNames.Insert(key);
    }
    path.Set(targetDirectory, "/", name, ".xiiMaterialAsset");
    xiiStringBuilder id;
    if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(path))
    {
      materialSlots[i].m_sResource = xiiConversionUtils::ToString(asset->m_Data.m_Guid, id);
      continue;
    }
    if (xiiOSFile::ExistsFile(path))
      return xiiStatus(xiiFmt("Material asset exists but is not indexed yet: '{}'. Retry after asset discovery.", path));
    auto* document = xiiDynamicCast<xiiMaterialAssetDocument*>(xiiQtEditorApp::GetSingleton()->CreateDocument(path, xiiDocumentFlags::None));
    if (document == nullptr)
      return xiiStatus(xiiFmt("Could not create material asset '{}'.", path));
    bool saved = false;
    XII_SCOPE_EXIT(
      document->GetDocumentManager()->CloseDocument(document);
      if (!saved) { if (xiiOSFile::DeleteFile(path).Failed()) xiiLog::Error("Could not remove failed imported asset: '{}'.", path); xiiAssetCurator::GetSingleton()->NotifyOfFileChange(path); });
    XII_SUCCEED_OR_RETURN(ConfigureMaterial(*document, imported.m_Materials[i], imported, sourceDirectory, targetDirectory));
    saved                        = true;
    materialSlots[i].m_sResource = xiiConversionUtils::ToString(document->GetGuid(), id);
  }
  inout_slots = std::move(materialSlots);
  return XII_SUCCESS;
}

xiiTransformStatus xiiMeshImportUtils::PrepareMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_slots, bool importMaterials, bool allowImport, xiiStringView documentPath, xiiStringView sourceFile, const xiiModelMeshImport& imported)
{
  auto materialSlots = inout_slots;
  SetMeshAssetMaterialSlots(materialSlots, imported);
  bool pendingMaterials = false;
  if (importMaterials)
    for (const auto& slot : materialSlots)
      pendingMaterials |= slot.m_sResource.IsEmpty();
  if (!allowImport && (pendingMaterials || !xiiMeshMaterialSlots::Equal(materialSlots, inout_slots)))
    return xiiTransformStatus(xiiTransformResult::NeedsImport);
  if (pendingMaterials)
    XII_SUCCEED_OR_RETURN(ImportMeshAssetMaterials(materialSlots, documentPath, sourceFile, imported));
  inout_slots = std::move(materialSlots);
  return XII_SUCCESS;
}
