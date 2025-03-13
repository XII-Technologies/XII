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

xiiLUTAssetDocument::xiiLUTAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiLUTAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
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

void xiiLUTAssetDocumentGenerator::GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<xiiAssetDocumentGenerator::ImportMode>& out_modes) const
{
  xiiAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
  info.m_Priority                             = xiiAssetDocGeneratorPriority::DefaultPriority;
  info.m_sName                                = "LUTImport.Cube";
  info.m_sIcon                                = ":/AssetIcons/LUT.svg";
}

xiiStatus xiiLUTAssetDocumentGenerator::Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  xiiStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(sOutFile);

  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  xiiDocument* pDoc = pApp->CreateDocument(sOutFile, xiiDocumentFlags::None);
  if (pDoc == nullptr)
    return xiiStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  xiiLUTAssetDocument* pAssetDoc = xiiDynamicCast<xiiLUTAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sInputFileRel.GetView());

  return xiiStatus(XII_SUCCESS);
}
