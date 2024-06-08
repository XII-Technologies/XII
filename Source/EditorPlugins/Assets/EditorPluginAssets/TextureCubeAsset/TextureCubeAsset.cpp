#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeAssetDocument, 3, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTextureCubeChannelMode, 1)
  XII_ENUM_CONSTANTS(xiiTextureCubeChannelMode::RGB, xiiTextureCubeChannelMode::Red, xiiTextureCubeChannelMode::Green, xiiTextureCubeChannelMode::Blue, xiiTextureCubeChannelMode::Alpha)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

const char* ToFilterMode(xiiTextureFilterSetting::Enum mode);
const char* ToUsageMode(xiiTexConvUsage::Enum mode);
const char* ToCompressionMode(xiiTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(xiiTexConvMipmapMode::Enum mode);

xiiTextureCubeAssetDocument::xiiTextureCubeAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiTextureCubeAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::Simple)
{
  m_iTextureLod = -1;
}

xiiStatus xiiTextureCubeAssetDocument::RunTexConv(const char* szTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const xiiTextureCubeAssetProperties* pProp = GetProperties();

  QStringList      arguments;
  xiiStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << xiiConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const xiiUInt64 uiHash64     = AssetHeader.GetFileHash();
    const xiiUInt32 uiHashLow32  = uiHash64 & 0xFFFFFFFF;
    const xiiUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", xiiArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", xiiArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  const xiiStringBuilder sThumbnail = GetThumbnailFilePath();
  if (bUpdateThumbnail)
  {
    // Thumbnail
    const xiiStringBuilder sDir = sThumbnail.GetFileDirectory();
    xiiOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  if (pProp->m_TextureUsage == xiiTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.SetFormat("{0}", xiiArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  // TODO: downscale steps and min/max resolution

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  arguments << "-type";
  arguments << "Cubemap";

  switch (pProp->m_ChannelMapping)
  {
    case xiiTextureCubeChannelMappingEnum::RGB1:
      arguments << "-rgb"
                << "in0";
      break;

    case xiiTextureCubeChannelMappingEnum::RGB1TO6:
      arguments << "-rgb0"
                << "in0";
      arguments << "-rgb1"
                << "in1";
      arguments << "-rgb2"
                << "in2";
      arguments << "-rgb3"
                << "in3";
      arguments << "-rgb4"
                << "in4";
      arguments << "-rgb5"
                << "in5";
      break;


    case xiiTextureCubeChannelMappingEnum::RGBA1:
      arguments << "-rgba"
                << "in0";
      break;

    case xiiTextureCubeChannelMappingEnum::RGBA1TO6:
      arguments << "-rgba0"
                << "in0";
      arguments << "-rgba1"
                << "in1";
      arguments << "-rgba2"
                << "in2";
      arguments << "-rgba3"
                << "in3";
      arguments << "-rgba4"
                << "in4";
      arguments << "-rgba5"
                << "in5";
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  const xiiInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (xiiInt32 i = 0; i < iNumInputFiles; ++i)
  {
    if (xiiStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    temp.SetFormat("-in{0}", i);
    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  XII_SUCCEED_OR_RETURN(xiiQtEditorApp::GetSingleton()->ExecuteTool("xiiTexConv", arguments, 180, xiiLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    xiiUInt64 uiThumbnailHash = xiiAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    XII_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return xiiStatus(XII_SUCCESS);
}

void xiiTextureCubeAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  switch (GetProperties()->m_ChannelMapping)
  {
    case xiiTextureCubeChannelMappingEnum::RGB1:
    case xiiTextureCubeChannelMappingEnum::RGBA1:
    {
      // remove file dependencies, that aren't used
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile1());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile2());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile3());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile4());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile5());
      break;
    }

    case xiiTextureCubeChannelMappingEnum::RGB1TO6:
    case xiiTextureCubeChannelMappingEnum::RGBA1TO6:
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

xiiTransformStatus xiiTextureCubeAssetDocument::InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  // XII_ASSERT_DEV(xiiStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);
  const bool bUpdateThumbnail = pAssetProfile == xiiAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  xiiTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

  xiiFileStats stat;
  if (xiiOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    xiiOSFile::DeleteFile(szTargetFile).IgnoreResult();
    if (result.Succeeded())
      result = xiiTransformStatus("TexConv did not write an output file");
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiTextureCubeAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTextureCubeAssetDocumentGenerator::xiiTextureCubeAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");

  // these formats would need to use 6 files for the faces
  // more elaborate detection and mapping would need to be implemented
  // AddSupportedFileType("tga");
  // AddSupportedFileType("jpg");
  // AddSupportedFileType("jpeg");
  // AddSupportedFileType("png");
}

xiiTextureCubeAssetDocumentGenerator::~xiiTextureCubeAssetDocumentGenerator() = default;

void xiiTextureCubeAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const xiiStringBuilder baseFilename = sAbsInputFile.GetFileName();
  const bool             isHDR        = sAbsInputFile.HasExtension("hdr") || sAbsInputFile.HasExtension("exr");

  const bool isCubemap = ((baseFilename.FindSubString_NoCase("cubemap") != nullptr) || (baseFilename.FindSubString_NoCase("skybox") != nullptr));

  // TODO: if (sAbsInputFile.IsEmpty()) -> CubemapImport.SkyboxAuto

  if (isHDR)
  {
    {
      xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority                             = isCubemap ? xiiAssetDocGeneratorPriority::HighPriority : xiiAssetDocGeneratorPriority::Undecided;
      info.m_sName                                = "CubemapImport.SkyboxHDR";
      info.m_sIcon                                = ":/AssetIcons/Texture_Cube.svg";
    }
  }
  else
  {
    {
      xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority                             = isCubemap ? xiiAssetDocGeneratorPriority::HighPriority : xiiAssetDocGeneratorPriority::Undecided;
      info.m_sName                                = "CubemapImport.Skybox";
      info.m_sIcon                                = ":/AssetIcons/Texture_Cube.svg";
    }
  }
}

xiiStatus xiiTextureCubeAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDocument*& out_pGeneratedDocument)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  out_pGeneratedDocument = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiTextureCubeAssetDocument* pAssetDoc = xiiDynamicCast<xiiTextureCubeAssetDocument*>(out_pGeneratedDocument);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)xiiTextureCubeChannelMappingEnum::RGB1);

  if (sMode == "CubemapImport.SkyboxHDR")
  {
    accessor.SetValue("Usage", (int)xiiTexConvUsage::Hdr);
  }
  else if (sMode == "CubemapImport.Skybox")
  {
    accessor.SetValue("Usage", (int)xiiTexConvUsage::Color);
  }

  return xiiStatus(XII_SUCCESS);
}
