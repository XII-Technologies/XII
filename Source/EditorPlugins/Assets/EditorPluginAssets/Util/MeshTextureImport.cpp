/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshTextureImport.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <ModelImporter/MeshImport.h>
#include <Texture/Image/Image.h>

namespace
{
  xiiStatus ResolveSource(const xiiModelMaterialTexture& texture, const xiiModelMeshImport& imported, xiiStringView sourceDirectory, xiiStringView targetDirectory, xiiStringBuilder& out_path, bool bWrite = true)
  {
    for (const auto& embedded : imported.m_EmbeddedTextures)
    {
      if (embedded.m_sKey != texture.m_sPath)
        continue;
      if (embedded.m_Data.IsEmpty())
        return xiiStatus("Embedded model texture has no image data.");
      xiiStringBuilder name;
      // Source keys, rather than image bytes, keep generated Input1 paths and
      // material identities stable when a GLB's embedded image is edited.
      const auto hash = xiiHashingUtils::StringHash(embedded.m_sKey);
      name.SetFormat("Embedded_{}", xiiArgU(hash, 16, true, 16));
      out_path.Set(targetDirectory, "/", name);
      if (embedded.m_uiHeight != 0)
      {
        if (embedded.m_uiWidth == 0 || xiiUInt64(embedded.m_uiWidth) * embedded.m_uiHeight * 4 != embedded.m_Data.GetCount())
          return xiiStatus("Invalid embedded RGBA image dimensions.");
        out_path.Append(".dds");
        if (xiiOSFile::ExistsFile(out_path))
        {
          xiiImage existing;
          if (existing.LoadFrom(out_path).Succeeded() && existing.GetWidth() == embedded.m_uiWidth && existing.GetHeight() == embedded.m_uiHeight &&
              existing.GetByteBlobPtr() == xiiConstByteBlobPtr(embedded.m_Data.GetData(), embedded.m_Data.GetCount()))
            return XII_SUCCESS;
        }
        if (!bWrite)
          return xiiStatus("Embedded texture needs extraction.");
        XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(targetDirectory));
        xiiGALTextureCreationDescription description;
        description.m_Type   = xiiGALResourceDimension::Texture2D;
        description.m_Size   = {embedded.m_uiWidth, embedded.m_uiHeight};
        description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
        xiiImageView image;
        image.ResetAndViewExternalStorage(description, xiiConstByteBlobPtr(embedded.m_Data.GetData(), embedded.m_Data.GetCount()));
        if (image.SaveTo(out_path).Failed())
          return xiiStatus(xiiFmt("Could not extract embedded texture '{}'.", out_path));
        xiiAssetCurator::GetSingleton()->NotifyOfFileChange(out_path);
        return XII_SUCCESS;
      }
      xiiHybridArray<xiiString, 16> extensions;
      xiiStringView(xiiFileBrowserAttribute::ImagesLdrAndHdr).Split(false, extensions, ";", "*", ".");
      if (embedded.m_sFileExtension.IsEmpty() || !extensions.Contains(embedded.m_sFileExtension))
        return xiiStatus(xiiFmt("Unsupported embedded image format '{}'.", embedded.m_sFileExtension));
      out_path.Append(".", embedded.m_sFileExtension);
      {
        xiiOSFile file;
        if (xiiOSFile::ExistsFile(out_path) && file.Open(out_path, xiiFileOpenMode::Read).Succeeded() && file.GetFileSize() == embedded.m_Data.GetCount())
        {
          xiiDynamicArray<xiiUInt8> bytes;
          if (file.ReadAll(bytes) == embedded.m_Data.GetCount() && bytes == embedded.m_Data)
            return XII_SUCCESS;
        }
      }
      if (!bWrite)
        return xiiStatus("Embedded texture needs extraction.");
      XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(targetDirectory));
      xiiDeferredFileWriter file;
      file.SetOutput(out_path, true);
      XII_SUCCEED_OR_RETURN(file.WriteBytes(embedded.m_Data.GetData(), embedded.m_Data.GetCount()));
      XII_SUCCEED_OR_RETURN(file.Close());
      xiiAssetCurator::GetSingleton()->NotifyOfFileChange(out_path);
      return XII_SUCCESS;
    }
    out_path = texture.m_sPath;
    out_path.ReplaceAll("\\", "/");
    if (!out_path.IsAbsolutePath())
      out_path.Prepend(sourceDirectory, "/");
    out_path.MakeCleanPath();
    if (!xiiOSFile::ExistsFile(out_path))
    {
      // Exporters often retain an absolute DCC path or move maps next to the model.
      // Search only conventional folders adjacent to this model, never a global tree.
      xiiStringBuilder sourcePath(texture.m_sPath);
      sourcePath.ReplaceAll("\\", "/");
      const xiiString  filename(sourcePath.GetFileNameAndExtension());
      xiiStringBuilder resolved;
      for (const char* folder : {"", "textures", "Textures", "maps", "Maps"})
      {
        xiiStringBuilder candidate(sourceDirectory);
        candidate.AppendPath(folder, filename);
        candidate.MakeCleanPath();
        if (xiiOSFile::ExistsFile(candidate))
        {
          if (!resolved.IsEmpty() &&
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
              !resolved.IsEqual_NoCase(candidate)
#else
              resolved != candidate
#endif
          )
          {
            xiiLog::Warning("Texture '{}' matches multiple adjacent files ('{}', '{}'). Remap Input1 in the generated texture asset.", texture.m_sPath, resolved, candidate);
            return XII_SUCCESS;
          }
          resolved = candidate;
        }
      }
      if (!resolved.IsEmpty())
      {
        out_path = resolved;
        return XII_SUCCESS;
      }
      // Keep a real texture document and its source dependency. The material remains
      // inspectable and the user can remap Input1, or restore the missing file later.
      xiiLog::Warning("Model texture '{}' is missing (resolved to '{}'). Restore the file or remap Input1 in the generated texture asset.", texture.m_sPath, out_path);
    }
    return XII_SUCCESS;
  }
} // namespace

xiiTransformStatus xiiMeshImportUtils::PrepareEmbeddedTextures(const xiiModelMeshImport& imported, xiiStringView sDocumentPath, bool bAllowImport)
{
  xiiStringBuilder directory(sDocumentPath);
  directory.RemoveFileExtension();
  directory.Append("_data");
  for (const auto& embedded : imported.m_EmbeddedTextures)
  {
    if (embedded.m_uiHeight == 0)
    {
      xiiHybridArray<xiiString, 16> extensions;
      xiiStringView(xiiFileBrowserAttribute::ImagesLdrAndHdr).Split(false, extensions, ";", "*", ".");
      if (!extensions.Contains(embedded.m_sFileExtension))
        continue; // Optional unsupported images are diagnosed by material conversion.
    }
    xiiModelMaterialTexture texture;
    texture.m_sPath = embedded.m_sKey;
    xiiStringBuilder path;
    const auto       result = ResolveSource(texture, imported, {}, directory, path, bAllowImport);
    if (result.Failed())
      return bAllowImport ? xiiTransformStatus(result) : xiiTransformStatus(xiiTransformResult::NeedsImport);
  }
  return XII_SUCCESS;
}

namespace
{
  xiiStatus CreateTextureAsset(xiiStringView source, xiiStringView secondSource, xiiStringView targetDirectory, const xiiGALSamplerCreationDescription& sampler, xiiTextureConverterUsage::Enum usage, xiiTexture2DChannelMappingEnum::Enum mapping, xiiString& out_asset)
  {
    // Separate assets for different color spaces, channel mappings and samplers, including equal source basenames.
    xiiUInt64 hash             = xiiHashingUtils::StringHash(source);
    hash                       = xiiHashingUtils::StringHash(secondSource, hash);
    const xiiUInt32 settings[] = {static_cast<xiiUInt32>(usage), static_cast<xiiUInt32>(mapping), sampler.CalculateHash()};
    hash                       = xiiHashingUtils::xxHash64(settings, sizeof(settings), hash);
    xiiStringBuilder name, path;
    xiiPathUtils::MakeValidFilename(source.GetFileName(), '_', name);
    path.SetFormat("{}/{}_{}.xiiTextureAsset", targetDirectory, name, xiiArgU(hash, 16, true, 16));
    if (xiiOSFile::ExistsFile(path))
      return xiiMeshImportUtils::ResolveAssetReference(path, out_asset);
    auto* document = xiiDynamicCast<xiiTextureAssetDocument*>(xiiQtEditorApp::GetSingleton()->CreateDocument(path, xiiDocumentFlags::None));
    if (document == nullptr)
      return xiiStatus(xiiFmt("Could not create texture asset '{}'.", path));
    bool saved = false;
    XII_SCOPE_EXIT(
      document->GetDocumentManager()->CloseDocument(document);
      if (!saved) { if (xiiOSFile::DeleteFile(path).Failed()) xiiLog::Error("Could not remove failed imported asset: '{}'.", path); xiiAssetCurator::GetSingleton()->NotifyOfFileChange(path); });
    auto* accessor = document->GetObjectAccessor();
    accessor->StartTransaction("Import Model Texture");
    bool committed = false;
    XII_SCOPE_EXIT(if (!committed) accessor->CancelTransaction());
    auto* object = document->GetPropertyObject();
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "Input1", source));
    if (!secondSource.IsEmpty())
      XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "Input2", secondSource));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "Usage", static_cast<xiiInt32>(usage)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "ChannelMapping", static_cast<xiiInt32>(mapping)));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "UseCustomFiltering", true));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MinFilter", static_cast<xiiInt32>(sampler.m_MinFilter.GetValue())));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MagFilter", static_cast<xiiInt32>(sampler.m_MagFilter.GetValue())));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MipFilter", static_cast<xiiInt32>(sampler.m_MipFilter.GetValue())));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MaxAnisotropy", sampler.m_uiMaxAnisotropy));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MipLODBias", sampler.m_fMipLODBias));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MinLOD", sampler.m_fMinLOD));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "MaxLOD", sampler.m_fMaxLOD));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "AddressModeU", static_cast<xiiInt32>(sampler.m_AddressU.GetValue())));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "AddressModeV", static_cast<xiiInt32>(sampler.m_AddressV.GetValue())));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(object, "AddressModeW", static_cast<xiiInt32>(sampler.m_AddressW.GetValue())));
    accessor->FinishTransaction();
    committed = true;
    XII_SUCCEED_OR_RETURN(document->SaveDocument());
    saved = true;
    xiiAssetCurator::GetSingleton()->NotifyOfFileChange(path);
    xiiStringBuilder id;
    out_asset = xiiConversionUtils::ToString(document->GetGuid(), id);
    return XII_SUCCESS;
  }

} // namespace

xiiStatus xiiMeshImportUtils::ResolveAssetReference(xiiStringView sAbsolutePath, xiiString& out_sAsset)
{
  if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sAbsolutePath))
  {
    xiiStringBuilder id;
    out_sAsset = xiiConversionUtils::ToString(asset->m_Data.m_Guid, id);
    return XII_SUCCESS;
  }
  const xiiDocumentTypeDescriptor* type = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(sAbsolutePath, false, type).Failed() || type == nullptr)
    return xiiStatus(xiiFmt("Cannot identify generated asset '{}'.", sAbsolutePath));
  auto*      document    = type->m_pManager->GetDocumentByPath(sAbsolutePath);
  const bool alreadyOpen = document != nullptr;
  if (!alreadyOpen)
    document = xiiQtEditorApp::GetSingleton()->OpenDocument(sAbsolutePath, xiiDocumentFlags::None);
  if (document == nullptr)
    return xiiStatus(xiiFmt("Cannot read existing asset '{}'.", sAbsolutePath));
  xiiStringBuilder id;
  out_sAsset = xiiConversionUtils::ToString(document->GetGuid(), id);
  if (!alreadyOpen)
    document->GetDocumentManager()->CloseDocument(document);
  xiiAssetCurator::GetSingleton()->NotifyOfFileChange(sAbsolutePath);
  return XII_SUCCESS;
}

xiiStatus xiiMeshImportUtils::ImportTexture(const xiiModelMaterialTexture& texture, const xiiModelMeshImport& imported, xiiStringView sourceDirectory, xiiStringView targetDirectory, xiiTextureConverterUsage::Enum usage, bool alpha, xiiString& out_asset)
{
  if (xiiTextureConverterSamplerPreset::ValidateFiltering(texture.m_Sampler).Failed())
    return xiiStatus("Model texture uses invalid native sampler settings.");
  xiiStringBuilder source;
  XII_SUCCEED_OR_RETURN(ResolveSource(texture, imported, sourceDirectory, targetDirectory, source));
  return CreateTextureAsset(source, xiiStringView(), targetDirectory, texture.m_Sampler, usage, alpha ? xiiTexture2DChannelMappingEnum::RGBA1 : xiiTexture2DChannelMappingEnum::RGB1, out_asset);
}

xiiStatus xiiMeshImportUtils::ImportMetallicRoughnessTexture(const xiiModelMaterialTexture* roughness, const xiiModelMaterialTexture* metallic, const xiiModelMeshImport& imported, xiiStringView sourceDirectory, xiiStringView targetDirectory, xiiString& out_asset)
{
  if (roughness == nullptr && metallic == nullptr)
    return xiiStatus("No roughness or metallic source texture was provided.");
  const auto& sampler = roughness != nullptr ? roughness->m_Sampler : metallic->m_Sampler;
  if (xiiTextureConverterSamplerPreset::ValidateFiltering(sampler).Failed())
    return xiiStatus("Invalid metallic/roughness sampler.");
  if (roughness != nullptr && metallic != nullptr && roughness->m_Sampler.CalculateHash() != metallic->m_Sampler.CalculateHash())
    return xiiStatus("Separate metallic/roughness textures require matching samplers to share one native texture. Assign a custom material for independent sampling.");
  xiiStringBuilder firstSource, secondSource;
  XII_SUCCEED_OR_RETURN(ResolveSource(roughness != nullptr ? *roughness : *metallic, imported, sourceDirectory, targetDirectory, firstSource));
  auto mapping = roughness != nullptr ? xiiTexture2DChannelMappingEnum::WHITE_R1_WHITE : xiiTexture2DChannelMappingEnum::WHITE_WHITE_R1;
  if (roughness != nullptr && metallic != nullptr)
  {
    XII_SUCCEED_OR_RETURN(ResolveSource(*metallic, imported, sourceDirectory, targetDirectory, secondSource));
    mapping = xiiTexture2DChannelMappingEnum::WHITE_R1_R2;
  }
  return CreateTextureAsset(firstSource, secondSource, targetDirectory, sampler, xiiTextureConverterUsage::Linear, mapping, out_asset);
}
