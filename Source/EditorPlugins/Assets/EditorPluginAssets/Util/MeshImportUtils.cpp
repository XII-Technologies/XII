/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>
#include <ModelImporter/Importer/Importer.h>

namespace xiiMeshImportUtils
{
  void FillFileFilter(xiiDynamicArray<xiiString>& out_list, xiiStringView sSeparated)
  {
    sSeparated.Split(false, out_list, ";", "*", ".");
  }

  xiiString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, xiiStringView sTexturePath, xiiModelImporter::TextureSemantic hint, bool bTextureClamp, const xiiModelImporter::Importer* pImporter)
  {
    if (!xiiUnicodeUtils::IsValidUtf8(sTexturePath.GetStartPointer(), sTexturePath.GetEndPointer()))
    {
      xiiLog::Error("Texture to resolve is not a valid UTF-8 string.");
      return xiiString();
    }

    xiiHybridArray<xiiString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, xiiFileBrowserAttribute::ImagesLdrAndHdr);

    xiiStringBuilder sFinalTextureName;
    xiiPathUtils::MakeValidFilename(sTexturePath.GetFileName(), '_', sFinalTextureName);

    xiiStringBuilder relTexturePath = szImportSourceFolder;
    relTexturePath.AppendPath(sFinalTextureName);

    if (auto itTex = pImporter->m_OutputTextures.Find(sTexturePath); itTex.IsValid())
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        xiiLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        return xiiString();
      }

      itTex.Value().GenerateFileName(sFinalTextureName);

      xiiStringBuilder sEmbededFile;
      sEmbededFile = szImportTargetFolder;
      sEmbededFile.AppendPath(sFinalTextureName);

      relTexturePath = sEmbededFile;
    }

    xiiStringBuilder newAssetPathAbs = szImportTargetFolder;
    newAssetPathAbs.AppendPath(sFinalTextureName);
    newAssetPathAbs.ChangeFileExtension("xiiTextureAsset");

    if (auto textureAssetInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(newAssetPathAbs))
    {
      // Try to resolve.

      xiiStringBuilder guidString;
      return xiiConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, guidString);
    }
    else
    {
      // Import otherwise

      xiiTextureAssetDocument* textureDocument = xiiDynamicCast<xiiTextureAssetDocument*>(xiiQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, xiiDocumentFlags::None));
      if (!textureDocument)
      {
        xiiLog::Error("Failed to create new texture asset '{0}'", sTexturePath);
        return sFinalTextureName;
      }

      xiiObjectAccessorBase* pAccessor = textureDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Import Texture");
      xiiDocumentObject* pTextureAsset = textureDocument->GetPropertyObject();

      if (xiiAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions).Failed())
      {
        relTexturePath = sFinalTextureName;
      }

      pAccessor->SetValueByName(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

      xiiEnum<xiiTexture2DChannelMappingEnum> channelMapping;

      // Try to map usage.
      xiiEnum<xiiTexConvUsage> usage;
      switch (hint)
      {
        case xiiModelImporter::TextureSemantic::DiffuseMap:
          usage = xiiTexConvUsage::Color;
          break;

        case xiiModelImporter::TextureSemantic::DiffuseAlphaMap:
          usage          = xiiTexConvUsage::Color;
          channelMapping = xiiTexture2DChannelMappingEnum::RGBA1;
          break;
        case xiiModelImporter::TextureSemantic::OcclusionMap: // Making wild guesses here.
        case xiiModelImporter::TextureSemantic::EmissiveMap:
          usage = xiiTexConvUsage::Color;
          break;

        case xiiModelImporter::TextureSemantic::RoughnessMap:
        case xiiModelImporter::TextureSemantic::MetallicMap:
          channelMapping = xiiTexture2DChannelMappingEnum::R1;
          usage          = xiiTexConvUsage::Linear;
          break;

        case xiiModelImporter::TextureSemantic::OrmMap:
          channelMapping = xiiTexture2DChannelMappingEnum::RGB1;
          usage          = xiiTexConvUsage::Linear;
          break;

        case xiiModelImporter::TextureSemantic::NormalMap:
          usage = xiiTexConvUsage::NormalMap;
          break;

        case xiiModelImporter::TextureSemantic::DisplacementMap:
          usage          = xiiTexConvUsage::Linear;
          channelMapping = xiiTexture2DChannelMappingEnum::R1;
          break;

        default:
          usage = xiiTexConvUsage::Auto;
      }

      pAccessor->SetValueByName(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValueByName(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValueByName(pTextureAsset, "AddressModeU", (int)xiiImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeV", (int)xiiImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeW", (int)xiiImageAddressMode::Clamp).LogFailure();
      }

      // TODO: Set... something else?

      pAccessor->FinishTransaction();
      textureDocument->SaveDocument().LogFailure();

      xiiStringBuilder guid;
      xiiConversionUtils::ToString(textureDocument->GetGuid(), guid);
      textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

      return guid;
    }
  };

  void SetMeshAssetMaterialSlots(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_materialSlots, const xiiModelImporter::Importer* pImporter)
  {
    const auto& opt = pImporter->GetImportOptions();

    const xiiUInt32 uiNumSubmeshes = opt.m_pMeshOutput->GetSubMeshes().GetCount();

    inout_materialSlots.SetCount(uiNumSubmeshes);

    for (const auto& material : pImporter->m_OutputMaterials)
    {
      if (material.m_iReferencedByMesh < 0)
        continue;

      inout_materialSlots[material.m_iReferencedByMesh].m_sLabel = material.m_sName;
    }
  }

  void CopyMeshAssetMaterialSlotToResource(xiiMeshResourceDescriptor& ref_desc, const xiiHybridArray<xiiMaterialResourceSlot, 8>& materialSlots)
  {
    for (xiiUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    {
      ref_desc.SetMaterial(i, materialSlots[i].m_sResource);
    }
  }

  static void ImportMeshAssetMaterialProperties(xiiMaterialAssetDocument* pMaterialDoc, const xiiModelImporter::OutputMaterial& material, const char* szImportSourceFolder, const char* szImportTargetFolder, const xiiModelImporter::Importer* pImporter)
  {
    xiiStringBuilder materialName = xiiPathUtils::GetFileName(pMaterialDoc->GetDocumentPath());

    XII_LOG_BLOCK("Apply Material Settings", materialName.GetData());

    xiiObjectAccessorBase* pAccessor = pMaterialDoc->GetObjectAccessor();
    pAccessor->StartTransaction("Apply Material Settings");
    xiiDocumentObject* pMaterialAsset = pMaterialDoc->GetPropertyObject();

    xiiStringBuilder tmp;

    // Set base material.
    xiiStatus res = pAccessor->SetValueByName(pMaterialAsset, "BaseMaterial", xiiConversionUtils::ToString(xiiMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
    res.LogFailure();
    if (res.Failed())
      return;

    // From now on we're setting shader properties.
    xiiDocumentObject* pMaterialProperties = pMaterialDoc->GetShaderPropertyObject();

    xiiVariant propertyValue;

    xiiString textureAo, textureRoughness, textureMetallic;
    material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::OcclusionMap, textureAo);
    material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::RoughnessMap, textureRoughness);
    material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::MetallicMap, textureMetallic);

    const bool bHasOrmTexture = !textureRoughness.IsEmpty() && ((textureAo == textureRoughness) || (textureMetallic == textureRoughness));


    // Set base texture.
    {
      xiiString textureDiffuse;

      if (material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::DiffuseMap, textureDiffuse))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "BaseTexture", xiiVariant(xiiMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureDiffuse, xiiModelImporter::TextureSemantic::DiffuseMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", false).LogFailure();
      }
    }

    // Set Normal Texture / Roughness Texture
    {
      xiiString textureNormal;

      if (!material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::NormalMap, textureNormal))
      {
        // Due to the lack of options in stuff like obj files, people stuff normals into the bump slot.
        material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::DisplacementMap, textureNormal);
      }

      if (!textureNormal.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseNormalTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureNormal, xiiModelImporter::TextureSemantic::NormalMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", xiiConversionUtils::ToString(xiiMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData()).LogFailure();
      }
    }

    if (!bHasOrmTexture)
    {
      if (!textureRoughness.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, xiiModelImporter::TextureSemantic::RoughnessMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
      }
    }

    // Set metallic texture
    if (!bHasOrmTexture)
    {
      if (!textureMetallic.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureMetallic, xiiModelImporter::TextureSemantic::MetallicMap, false, pImporter))).LogFailure();
      }
    }

    // Set emissive texture
    {
      xiiString textureEmissive;

      if (material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::EmissiveMap, textureEmissive))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseEmissiveTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "EmissiveTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureEmissive, xiiModelImporter::TextureSemantic::EmissiveMap, false, pImporter))).LogFailure();
      }
    }

    // Set AO texture
    if (!bHasOrmTexture)
    {
      xiiString textureAo;

      if (material.m_TextureReferences.TryGetValue(xiiModelImporter::TextureSemantic::OcclusionMap, textureAo))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureAo, xiiModelImporter::TextureSemantic::OcclusionMap, false, pImporter))).LogFailure();
      }
    }

    // TODO: ambient occlusion texture

    // Set base color property
    if (material.m_Properties.TryGetValue(xiiModelImporter::PropertySemantic::DiffuseColor, propertyValue) && propertyValue.IsA<xiiColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "BaseColor", propertyValue).LogFailure();
    }

    // Set emissive color property
    if (material.m_Properties.TryGetValue(xiiModelImporter::PropertySemantic::EmissiveColor, propertyValue) && propertyValue.IsA<xiiColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "EmissiveColor", propertyValue).LogFailure();
    }

    // Set two-sided property
    if (material.m_Properties.TryGetValue(xiiModelImporter::PropertySemantic::TwosidedValue, propertyValue) && propertyValue.IsNumber())
    {
      pAccessor->SetValueByName(pMaterialProperties, "TWO_SIDED", propertyValue.ConvertTo<bool>()).LogFailure();
    }

    // Set metallic property
    if (material.m_Properties.TryGetValue(xiiModelImporter::PropertySemantic::MetallicValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value >= 1.0f)
        value = 1.0f;
      else
        value = 0.0f;

      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", value).LogFailure();
    }

    // Set roughness property
    if (material.m_Properties.TryGetValue(xiiModelImporter::PropertySemantic::RoughnessValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value > 1.0f)
        value /= 255.0f;

      value = xiiMath::Clamp(value, 0.0f, 1.0f);
      value = xiiMath::Lerp(0.4f, 1.0f, value);

      // the extracted roughness value is really just a guess to get started

      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", value).LogFailure();
    }

    // Set ORM Texture
    if (bHasOrmTexture)
    {
      pAccessor->SetValueByName(pMaterialProperties, "UseOrmTexture", true).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", false).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", 1.0f).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", 0.0f).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "OrmTexture", xiiVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, xiiModelImporter::TextureSemantic::OrmMap, false, pImporter))).LogFailure();
    }

    // Todo:
    // * Shading Mode
    // * Mask Threshold

    pAccessor->FinishTransaction();
  }

  void ImportMeshAssetMaterials(xiiHybridArray<xiiMaterialResourceSlot, 8>& inout_materialSlots, xiiStringView sDocumentDirectory, const xiiModelImporter::Importer* pImporter)
  {
    XII_PROFILE_SCOPE("ImportMeshAssetMaterials");

    xiiStringBuilder targetDirectory = sDocumentDirectory;
    targetDirectory.RemoveFileExtension();
    targetDirectory.Append("_data/");
    const xiiStringBuilder sourceDirectory = xiiPathUtils::GetFileDirectory(pImporter->GetImportOptions().m_sSourceFile);

    xiiStringBuilder tmp;
    xiiStringBuilder newResourcePathAbs;

    const xiiUInt32 uiNumSubmeshes = inout_materialSlots.GetCount();

    if (uiNumSubmeshes == 0)
      return;

    xiiProgressRange range("Importing Materials", uiNumSubmeshes, false);

    xiiHashTable<const xiiModelImporter::OutputMaterial*, xiiString> importMatToGuid;

    xiiHybridArray<xiiDocument*, 32> pendingSaveTasks;

    auto WaitForPendingTasks = [&pendingSaveTasks]() {
      XII_PROFILE_SCOPE("WaitForPendingTasks");
      for (xiiDocument* pDoc : pendingSaveTasks)
      {
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
      }
      pendingSaveTasks.Clear();
    };

    xiiHybridArray<xiiString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, xiiFileBrowserAttribute::ImagesLdrAndHdr);

    for (const auto& itTex : pImporter->m_OutputTextures)
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        xiiLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        continue;
      }

      xiiStringBuilder sFinalTextureName;
      itTex.Value().GenerateFileName(sFinalTextureName);

      xiiStringBuilder sEmbededFile;
      sEmbededFile = targetDirectory;
      sEmbededFile.AppendPath(sFinalTextureName);

      xiiDeferredFileWriter out;
      out.SetOutput(sEmbededFile, true);
      out.WriteBytes(itTex.Value().m_RawData.GetPtr(), itTex.Value().m_RawData.GetCount()).AssertSuccess();
      out.Close().IgnoreResult();
    }

    for (const auto& impMaterial : pImporter->m_OutputMaterials)
    {
      if (impMaterial.m_iReferencedByMesh < 0)
        continue;

      const xiiUInt32 subMeshIdx = impMaterial.m_iReferencedByMesh;

      range.BeginNextStep("Importing Material");

      // Didn't find currently set resource, create new imported material.
      if (!xiiAssetCurator::GetSingleton()->FindSubAsset(inout_materialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToGuid.TryGetValue(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource))
          continue;

        // Put the new asset in the data folder.
        newResourcePathAbs = targetDirectory;
        newResourcePathAbs.AppendPath(impMaterial.m_sName);
        newResourcePathAbs.Append(".xiiMaterialAsset");

        // Does the generated path already exist? Use it.
        if (const auto assetInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs))
        {
          inout_materialSlots[subMeshIdx].m_sResource = xiiConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
          continue;
        }

        xiiMaterialAssetDocument* pMaterialDoc = xiiDynamicCast<xiiMaterialAssetDocument*>(xiiQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, xiiDocumentFlags::AsyncSave));
        if (!pMaterialDoc)
        {
          xiiLog::Error("Failed to create new material '{0}'", impMaterial.m_sName);
          continue;
        }

        ImportMeshAssetMaterialProperties(pMaterialDoc, impMaterial, sourceDirectory, targetDirectory, pImporter);
        inout_materialSlots[subMeshIdx].m_sResource = xiiConversionUtils::ToString(pMaterialDoc->GetGuid(), tmp);

        pMaterialDoc->SaveDocumentAsync({});
        pendingSaveTasks.PushBack(pMaterialDoc);

        // we have to flush because materials create worlds in the engine process and there is a world limit of 64
        if (pendingSaveTasks.GetCount() >= 16)
          WaitForPendingTasks();
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material as previous slots.
      if (!inout_materialSlots[subMeshIdx].m_sResource.IsEmpty())
      {
        importMatToGuid.Insert(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource);
      }
    }

    WaitForPendingTasks();
  }

} // namespace xiiMeshImportUtils
