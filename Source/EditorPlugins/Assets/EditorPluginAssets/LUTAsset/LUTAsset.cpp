#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>



#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>



// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLUTAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLUTAssetDocument::xiiLUTAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiLUTAssetProperties>(szDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const auto props = GetProperties();

  // Read CUBE file, convert to 3D texture and write to file
  xiiFileStats Stats;
  bool         bStat = xiiOSFile::GetFileStats(props->GetAbsoluteInputFilePath(), Stats).Succeeded();

  xiiFileReader cubeFile;
  if (!bStat || cubeFile.Open(props->GetAbsoluteInputFilePath()).Failed())
  {
    return xiiStatus(xiiFmt("Couldn't open CUBE file '{0}'.", props->GetAbsoluteInputFilePath()));
  }

  xiiAdobeCUBEReader cubeReader;
  auto               parseRes = cubeReader.ParseFile(cubeFile);
  if (parseRes.Failed())
    return parseRes;

  const xiiUInt32 lutSize = cubeReader.GetLUTSize();

  // Build a xiiImage from the data
  xiiImageHeader imgHeader;
  imgHeader.SetImageFormat(xiiImageFormat::R8G8B8A8_UNORM_SRGB);
  imgHeader.SetWidth(lutSize);
  imgHeader.SetHeight(lutSize);
  imgHeader.SetDepth(lutSize);

  xiiImage img;
  img.ResetAndAlloc(imgHeader);

  if (!img.IsValid())
  {
    return xiiStatus("Allocated xiiImage for LUT data is not valid.");
  }



  for (xiiUInt32 b = 0; b < lutSize; ++b)
  {
    for (xiiUInt32 g = 0; g < lutSize; ++g)
    {
      for (xiiUInt32 r = 0; r < lutSize; ++r)
      {
        const xiiVec3 val = cubeReader.GetLUTEntry(r, g, b);

        xiiColor        col(val.x, val.y, val.z);
        xiiColorGammaUB colUb(col);

        xiiColorGammaUB* pPixel = img.GetPixelPointer<xiiColorGammaUB>(0, 0, 0, r, g, b);

        *pPixel = colUb;
      }
    }
  }

  xiiDeferredFileWriter file;
  file.SetOutput(szTargetFile);
  XII_SUCCEED_OR_RETURN(AssetHeader.Write(file));

  xiiTexFormat texFormat;
  texFormat.m_bSRGB         = true;
  texFormat.m_AddressModeU  = xiiImageAddressMode::Clamp;
  texFormat.m_AddressModeV  = xiiImageAddressMode::Clamp;
  texFormat.m_AddressModeW  = xiiImageAddressMode::Clamp;
  texFormat.m_TextureFilter = xiiTextureFilterSetting::FixedBilinear;

  texFormat.WriteTextureHeader(file);

  xiiDdsFileFormat fmt;
  if (fmt.WriteImage(file, img, "dds").Failed())
    return xiiStatus(xiiFmt("Writing image to target file failed: '{0}'", szTargetFile));

  if (file.Close().Failed())
    return xiiStatus(xiiFmt("Writing to target file failed: '{0}'", szTargetFile));

  return xiiStatus(XII_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLUTAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiLUTAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiLUTAssetDocumentGenerator::xiiLUTAssetDocumentGenerator()
{
  AddSupportedFileType("cube");
}

xiiLUTAssetDocumentGenerator::~xiiLUTAssetDocumentGenerator() = default;

void xiiLUTAssetDocumentGenerator::GetImportModes(xiiStringView sParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  xiiStringBuilder baseOutputFile = sParentDirRelativePath;

  const xiiStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  xiiAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
  info.m_Priority                       = xiiAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative      = baseOutputFile;

  info.m_sName = "LUTImport.Cube";
  info.m_sIcon = ":/AssetIcons/LUT.png";
}

xiiStatus xiiLUTAssetDocumentGenerator::Generate(xiiStringView sDataDirRelativePath, const xiiAssetDocumentGenerator::Info& info, xiiDocument*& out_pGeneratedDocument)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, xiiDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return xiiStatus("Could not create target document");

  xiiLUTAssetDocument* pAssetDoc = xiiDynamicCast<xiiLUTAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return xiiStatus("Target document is not a valid xiiLUTAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sDataDirRelativePath);

  return xiiStatus(XII_SUCCESS);
}
