#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetDocument, 6, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTextureChannelMode, 1)
  XII_ENUM_CONSTANT(xiiTextureChannelMode::RGBA)->AddAttributes(new xiiGroupAttribute("Multi", 0.0f)),
  XII_ENUM_CONSTANT(xiiTextureChannelMode::RGB)->AddAttributes(new xiiGroupAttribute("Multi", 1.0f)),
  XII_ENUM_CONSTANT(xiiTextureChannelMode::Red)->AddAttributes(new xiiGroupAttribute("Single", 0.0f)),
  XII_ENUM_CONSTANT(xiiTextureChannelMode::Green)->AddAttributes(new xiiGroupAttribute("Single", 1.0f)),
  XII_ENUM_CONSTANT(xiiTextureChannelMode::Blue)->AddAttributes(new xiiGroupAttribute("Single", 2.0f)),
  XII_ENUM_CONSTANT(xiiTextureChannelMode::Alpha)->AddAttributes(new xiiGroupAttribute("Single", 3.0f))
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiTextureAssetDocument::xiiTextureAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiTextureAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::Simple)
{
  m_iTextureLod = -1;
}

static const char* ToWrapMode(xiiImageAddressMode::Enum mode)
{
  switch (mode)
  {
    case xiiImageAddressMode::Repeat:
      return "Repeat";
    case xiiImageAddressMode::Clamp:
      return "Clamp";
    case xiiImageAddressMode::ClampBorder:
      return "ClampBorder";
    case xiiImageAddressMode::Mirror:
      return "Mirror";
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

const char* ToFilterMode(xiiTextureFilterSetting::Enum mode)
{
  switch (mode)
  {
    case xiiTextureFilterSetting::FixedNearest:
      return "Nearest";
    case xiiTextureFilterSetting::FixedBilinear:
      return "Bilinear";
    case xiiTextureFilterSetting::FixedTrilinear:
      return "Trilinear";
    case xiiTextureFilterSetting::FixedAnisotropic2x:
      return "Aniso2x";
    case xiiTextureFilterSetting::FixedAnisotropic4x:
      return "Aniso4x";
    case xiiTextureFilterSetting::FixedAnisotropic8x:
      return "Aniso8x";
    case xiiTextureFilterSetting::FixedAnisotropic16x:
      return "Aniso16x";
    case xiiTextureFilterSetting::LowestQuality:
      return "Lowest";
    case xiiTextureFilterSetting::LowQuality:
      return "Low";
    case xiiTextureFilterSetting::DefaultQuality:
      return "Default";
    case xiiTextureFilterSetting::HighQuality:
      return "High";
    case xiiTextureFilterSetting::HighestQuality:
      return "Highest";
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToUsageMode(xiiTexConvUsage::Enum mode)
{
  switch (mode)
  {
    case xiiTexConvUsage::Auto:
      return "Auto";
    case xiiTexConvUsage::Color:
      return "Color";
    case xiiTexConvUsage::Linear:
      return "Linear";
    case xiiTexConvUsage::Hdr:
      return "Hdr";
    case xiiTexConvUsage::NormalMap:
      return "NormalMap";
    case xiiTexConvUsage::NormalMap_Inverted:
      return "NormalMap_Inverted";
    case xiiTexConvUsage::BumpMap:
      return "BumpMap";
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToMipmapMode(xiiTexConvMipmapMode::Enum mode)
{
  switch (mode)
  {
    case xiiTexConvMipmapMode::None:
      return "None";
    case xiiTexConvMipmapMode::Linear:
      return "Linear";
    case xiiTexConvMipmapMode::Kaiser:
      return "Kaiser";
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToCompressionMode(xiiTexConvCompressionMode::Enum mode)
{
  switch (mode)
  {
    case xiiTexConvCompressionMode::None:
      return "None";
    case xiiTexConvCompressionMode::Medium:
      return "Medium";
    case xiiTexConvCompressionMode::High:
      return "High";
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return "";
}

xiiStatus xiiTextureAssetDocument::RunTexConv(const char* szTargetFile, const xiiAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const xiiTextureAssetProfileConfig* pAssetConfig)
{
  const xiiTextureAssetProperties* pProp = GetProperties();

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

    temp.Format("{0}", xiiArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("{0}", xiiArgU(uiHashHigh32, 8, true, 16, true));
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

  // low resolution data
  {
    xiiStringBuilder lowResPath = szTargetFile;
    xiiStringBuilder name       = lowResPath.GetFileName();
    name.Append("-lowres");
    lowResPath.ChangeFileName(name);

    arguments << "-lowMips";
    arguments << "6";
    arguments << "-lowOut";

    arguments << QString::fromUtf8(lowResPath.GetData());
  }

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->m_bDilateColor)
  {
    arguments << "-dilate";
    // arguments << "8"; // default value
  }

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  if (pProp->m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    temp.Format("{0}", xiiArgF(pProp->m_fAlphaThreshold, 2));
    arguments << temp.GetData();
  }

  if (pProp->m_TextureUsage == xiiTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.Format("{0}", xiiArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-addressU" << ToWrapMode(pProp->m_AddressModeU);
  arguments << "-addressV" << ToWrapMode(pProp->m_AddressModeV);
  arguments << "-addressW" << ToWrapMode(pProp->m_AddressModeW);
  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  const xiiInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (xiiInt32 i = 0; i < iNumInputFiles; ++i)
  {
    temp.Format("-in{0}", i);

    if (xiiStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  switch (pProp->GetChannelMapping())
  {
    case xiiTexture2DChannelMappingEnum::R1:
    {
      arguments << "-r";
      arguments << "in0.r"; // always linear
    }
    break;

    case xiiTexture2DChannelMappingEnum::RG1:
    {
      arguments << "-rg";
      arguments << "in0.rg"; // always linear
    }
    break;

    case xiiTexture2DChannelMappingEnum::R1_G2:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.g"; // always linear
    }
    break;

    case xiiTexture2DChannelMappingEnum::RGB1:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
    }
    break;

    case xiiTexture2DChannelMappingEnum::RGB1_ABLACK:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "black";
    }
    break;

    case xiiTexture2DChannelMappingEnum::R1_G2_B3:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
    }
    break;

    case xiiTexture2DChannelMappingEnum::RGBA1:
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
    break;

    case xiiTexture2DChannelMappingEnum::RGB1_A2:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "in1.r";
    }
    break;

    case xiiTexture2DChannelMappingEnum::R1_G2_B3_A4:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
      arguments << "-a";
      arguments << "in3.r";
    }
    break;
  }

  XII_SUCCEED_OR_RETURN(xiiQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, xiiLog::GetThreadLocalLogSystem()));

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


void xiiTextureAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (!m_bIsRenderTarget)
  {
    // Every 2D texture also generates a "-lowres" output, which is used to be embedded into materials for quick streaming
    pInfo->m_Outputs.Insert("LOWRES");
  }

  for (xiiUInt32 i = GetProperties()->GetNumInputFiles(); i < 4; ++i)
  {
    // remove unused dependencies
    pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile(i));
  }
}

void xiiTextureAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bIsRenderTarget)
  {
    if (GetProperties()->m_bIsRenderTarget == false)
    {
      GetCommandHistory()->StartTransaction("MakeRenderTarget");
      GetObjectAccessor()->SetValue(GetPropertyObject(), "IsRenderTarget", true);
      GetCommandHistory()->FinishTransaction();
      GetCommandHistory()->ClearUndoHistory();
    }
  }
}

xiiTransformStatus xiiTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  if (xiiStringUtils::IsEqual(szOutputTag, "LOWRES"))
  {
    // No need to generate this file, it will be generated together with the main output
    return xiiTransformStatus();
  }

  // XII_ASSERT_DEV(xiiStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);

  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<xiiTextureAssetProfileConfig>();

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    xiiDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    XII_SUCCEED_OR_RETURN(AssetHeader.Write(file));

    // TODO: move this into a shared location, reuse in xiiTexConv::WriteTexHeader
    const xiiUInt8 uiTexFileFormatVersion = 5;
    file << uiTexFileFormatVersion;

    xiiGALResourceFormat::Enum format  = xiiGALResourceFormat::Invalid;
    bool                       bIsSRGB = false;

    switch (props->m_RtFormat)
    {
      case xiiRenderTargetFormat::RGBA8:
        format = xiiGALResourceFormat::RGBAUByteNormalized;
        break;

      case xiiRenderTargetFormat::RGBA8sRgb:
        format  = xiiGALResourceFormat::RGBAUByteNormalizedsRGB;
        bIsSRGB = true;
        break;

      case xiiRenderTargetFormat::RGB10:
        format = xiiGALResourceFormat::RG11B10Float;
        break;

      case xiiRenderTargetFormat::RGBA16:
        format = xiiGALResourceFormat::RGBAHalf;
        break;
    }

    file << bIsSRGB;
    file << (xiiUInt8)props->m_AddressModeU;
    file << (xiiUInt8)props->m_AddressModeV;
    file << (xiiUInt8)props->m_AddressModeW;
    file << (xiiUInt8)props->m_TextureFilter;

    xiiInt16 resX = 0, resY = 0;

    switch (props->m_Resolution)
    {
      case xiiTexture2DResolution::Fixed64x64:
        resX = 64;
        resY = 64;
        break;
      case xiiTexture2DResolution::Fixed128x128:
        resX = 128;
        resY = 128;
        break;
      case xiiTexture2DResolution::Fixed256x256:
        resX = 256;
        resY = 256;
        break;
      case xiiTexture2DResolution::Fixed512x512:
        resX = 512;
        resY = 512;
        break;
      case xiiTexture2DResolution::Fixed1024x1024:
        resX = 1024;
        resY = 1024;
        break;
      case xiiTexture2DResolution::Fixed2048x2048:
        resX = 2048;
        resY = 2048;
        break;
      case xiiTexture2DResolution::CVarRtResolution1:
        resX = -1;
        resY = 1;
        break;
      case xiiTexture2DResolution::CVarRtResolution2:
        resX = -1;
        resY = 2;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }

    file << resX;
    file << resY;
    file << props->m_fCVarResolutionScale;
    file << (xiiInt32)format;


    if (file.Close().Failed())
      return xiiTransformStatus(xiiFmt("Writing to target file failed: '{0}'", szTargetFile));

    return xiiTransformStatus();
  }
  else
  {
    const bool bUpdateThumbnail = pAssetProfile == xiiAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

    xiiTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail, pAssetConfig);

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
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiTextureAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

enum class TextureType
{
  Diffuse,
  Normal,
  Occlusion,
  Roughness,
  Metalness,
  ORM,
  Height,
  HDR,
  Linear,
};

xiiTextureAssetDocumentGenerator::xiiTextureAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");
}

xiiTextureAssetDocumentGenerator::~xiiTextureAssetDocumentGenerator() = default;

void xiiTextureAssetDocumentGenerator::GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = sParentDirRelativePath;

  xiiStringBuilder baseFilename = baseOutputFile.GetFileName();

  while (baseFilename.TrimWordEnd("_") ||
         baseFilename.TrimWordEnd("K") ||
         baseFilename.TrimWordEnd("-") ||
         baseFilename.TrimWordEnd("1") ||
         baseFilename.TrimWordEnd("2") ||
         baseFilename.TrimWordEnd("3") ||
         baseFilename.TrimWordEnd("4") ||
         baseFilename.TrimWordEnd("5") ||
         baseFilename.TrimWordEnd("6") ||
         baseFilename.TrimWordEnd("7") ||
         baseFilename.TrimWordEnd("8") ||
         baseFilename.TrimWordEnd("9") ||
         baseFilename.TrimWordEnd("0") ||
         baseFilename.TrimWordEnd("gl"))
  {
  }

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  TextureType tt = TextureType::Diffuse;

  if (xiiPathUtils::HasExtension(sParentDirRelativePath, "hdr"))
  {
    tt = TextureType::HDR;
  }
  else if (xiiPathUtils::HasExtension(sParentDirRelativePath, "exr"))
  {
    tt = TextureType::HDR;
  }
  else if (baseFilename.EndsWith_NoCase("_d") || baseFilename.EndsWith_NoCase("diffuse") || baseFilename.EndsWith_NoCase("diff") || baseFilename.EndsWith_NoCase("col") || baseFilename.EndsWith_NoCase("color"))
  {
    tt = TextureType::Diffuse;
  }
  else if (baseFilename.EndsWith_NoCase("_n") || baseFilename.EndsWith_NoCase("normal") || baseFilename.EndsWith_NoCase("normals") || baseFilename.EndsWith_NoCase("nrm") || baseFilename.EndsWith_NoCase("norm") || baseFilename.EndsWith_NoCase("_nor"))
  {
    tt = TextureType::Normal;
  }
  else if (baseFilename.EndsWith_NoCase("_arm") || baseFilename.EndsWith_NoCase("_orm"))
  {
    tt = TextureType::ORM;
  }
  else if (baseFilename.EndsWith_NoCase("_rough") || baseFilename.EndsWith_NoCase("roughness") || baseFilename.EndsWith_NoCase("_rgh"))
  {
    tt = TextureType::Roughness;
  }
  else if (baseFilename.EndsWith_NoCase("_ao"))
  {
    tt = TextureType::Occlusion;
  }
  else if (baseFilename.EndsWith_NoCase("_height") || baseFilename.EndsWith_NoCase("_disp"))
  {
    tt = TextureType::Height;
  }
  else if (baseFilename.EndsWith_NoCase("_metal") || baseFilename.EndsWith_NoCase("_met") || baseFilename.EndsWith_NoCase("metallic") || baseFilename.EndsWith_NoCase("metalness"))
  {
    tt = TextureType::Metalness;
  }
  else if (baseFilename.EndsWith_NoCase("_alpha"))
  {
    tt = TextureType::Linear;
  }

  xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
  info.m_Priority                       = xiiAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative      = baseOutputFile;

  // first add the default option
  switch (tt)
  {
    case TextureType::Diffuse:
    {
      info.m_sName = "TextureImport.Diffuse";
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
      break;
    }

    case TextureType::Normal:
    {
      info.m_sName = "TextureImport.Normal";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.png";
      break;
    }

    case TextureType::Roughness:
    {
      info.m_sName = "TextureImport.Roughness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::Occlusion:
    {
      info.m_sName = "TextureImport.Occlusion";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::Metalness:
    {
      info.m_sName = "TextureImport.Metalness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::ORM:
    {
      info.m_sName = "TextureImport.ORM";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::Height:
    {
      info.m_sName = "TextureImport.Height";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }

    case TextureType::HDR:
    {
      info.m_sName = "TextureImport.HDR";
      info.m_sIcon = ":/AssetIcons/Texture_2D.png";
      break;
    }

    case TextureType::Linear:
    {
      info.m_sName = "TextureImport.Linear";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.png";
      break;
    }
  }

  // now add all the other options

  if (tt != TextureType::Diffuse)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Diffuse";
    info2.m_sIcon                          = ":/AssetIcons/Texture_2D.png";
  }

  if (tt != TextureType::Linear)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Linear";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Normal)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Normal";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Normals.png";
  }

  if (tt != TextureType::Metalness)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Metalness";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Roughness)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Roughness";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Occlusion)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Occlusion";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::ORM)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.ORM";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }

  if (tt != TextureType::Height)
  {
    xiiAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority                       = xiiAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative      = baseOutputFile;
    info2.m_sName                          = "TextureImport.Height";
    info2.m_sIcon                          = ":/AssetIcons/Texture_Linear.png";
  }
}

xiiStatus xiiTextureAssetDocumentGenerator::Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiTextureAssetDocument* pAssetDoc = xiiDynamicCast<xiiTextureAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sDataDirRelativePath);
  accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::RGB1);
  accessor.SetValue("Usage", (int)xiiTexConvUsage::Linear);

  if (info.m_sName == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)xiiTexConvUsage::Color);
  }
  else if (info.m_sName == "TextureImport.Normal")
  {
    accessor.SetValue("Usage", (int)xiiTexConvUsage::NormalMap);
  }
  else if (info.m_sName == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)xiiTexConvUsage::Hdr);
  }
  else if (info.m_sName == "TextureImport.Linear")
  {
  }
  else if (info.m_sName == "TextureImport.Occlusion")
  {
    accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)xiiTextureFilterSetting::LowestQuality);
  }
  else if (info.m_sName == "TextureImport.Height")
  {
    accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)xiiTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Roughness")
  {
    accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)xiiTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Metalness")
  {
    accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)xiiTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.ORM")
  {
    accessor.SetValue("ChannelMapping", (int)xiiTexture2DChannelMappingEnum::RGB1);
    accessor.SetValue("TextureFilter", (int)xiiTextureFilterSetting::LowQuality);
  }

  return xiiStatus(XII_SUCCESS);
}
