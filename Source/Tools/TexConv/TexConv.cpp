#include <TexConv/TexConvPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

xiiTexConv::xiiTexConv() :
  xiiApplication("TexConv")
{
}

xiiResult xiiTexConv::BeforeCoreSystemsStartup()
{
  xiiStartup::AddApplicationTag("tool");
  xiiStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void xiiTexConv::AfterCoreSystemsStartup()
{
  xiiFileSystem::AddDataDirectory("", "App", ":", xiiFileSystem::AllowWrites).IgnoreResult();

  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
}

void xiiTexConv::BeforeCoreSystemsShutdown()
{
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

xiiResult xiiTexConv::DetectOutputFormat()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = xiiTexConvOutputType::None;
    return XII_SUCCESS;
  }

  xiiStringBuilder sExt = xiiPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D          = true;
    m_bOutputSupports3D          = true;
    m_bOutputSupportsCube        = true;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = true;
    m_bOutputSupportsFiltering   = false;
    m_bOutputSupportsCompression = true;
    return XII_SUCCESS;
  }
  if (sExt == "TGA" || sExt == "PNG")
  {
    m_bOutputSupports2D          = true;
    m_bOutputSupports3D          = false;
    m_bOutputSupportsCube        = false;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = false;
    m_bOutputSupportsFiltering   = false;
    m_bOutputSupportsCompression = false;
    return XII_SUCCESS;
  }
  if (sExt == "XIITEXTURE2D")
  {
    m_bOutputSupports2D          = true;
    m_bOutputSupports3D          = false;
    m_bOutputSupportsCube        = false;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = true;
    m_bOutputSupportsFiltering   = true;
    m_bOutputSupportsCompression = true;
    return XII_SUCCESS;
  }
  if (sExt == "XIITEXTURE3D")
  {
    m_bOutputSupports2D          = false;
    m_bOutputSupports3D          = true;
    m_bOutputSupportsCube        = false;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = true;
    m_bOutputSupportsFiltering   = true;
    m_bOutputSupportsCompression = true;
    return XII_SUCCESS;
  }
  if (sExt == "XIITEXTURECUBE")
  {
    m_bOutputSupports2D          = false;
    m_bOutputSupports3D          = false;
    m_bOutputSupportsCube        = true;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = true;
    m_bOutputSupportsFiltering   = true;
    m_bOutputSupportsCompression = true;
    return XII_SUCCESS;
  }
  if (sExt == "XIITEXTUREATLAS")
  {
    m_bOutputSupports2D          = false;
    m_bOutputSupports3D          = false;
    m_bOutputSupportsCube        = false;
    m_bOutputSupportsAtlas       = true;
    m_bOutputSupportsMipmaps     = true;
    m_bOutputSupportsFiltering   = true;
    m_bOutputSupportsCompression = true;
    return XII_SUCCESS;
  }
  if (sExt == "XIIIMAGEDATA")
  {
    m_bOutputSupports2D          = true;
    m_bOutputSupports3D          = false;
    m_bOutputSupportsCube        = false;
    m_bOutputSupportsAtlas       = false;
    m_bOutputSupportsMipmaps     = false;
    m_bOutputSupportsFiltering   = false;
    m_bOutputSupportsCompression = false;
    return XII_SUCCESS;
  }

  xiiLog::Error("Output file uses unsupported file format '{}'", sExt);
  return XII_FAILURE;
}

bool xiiTexConv::IsTexFormat() const
{
  const xiiStringView ext = xiiPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("xii");
}

xiiResult xiiTexConv::WriteTexFile(xiiStreamWriter& ref_stream, const xiiImage& image)
{
  xiiAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  XII_SUCCEED_OR_RETURN(asset.Write(ref_stream));

  xiiTexFormat texFormat;
  texFormat.m_bSRGB         = xiiImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU  = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV  = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW  = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(ref_stream);

  xiiDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(ref_stream, image, "dds").Failed())
  {
    xiiLog::Error("Failed to write DDS image chunk to xiiTex file.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConv::WriteOutputFile(const char* szFile, const xiiImage& image)
{
  if (xiiPathUtils::HasExtension(szFile, "xiiImageData"))
  {
    xiiDeferredFileWriter file;
    file.SetOutput(szFile);

    xiiAssetFileHeader asset;
    asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    if (asset.Write(file).Failed())
    {
      xiiLog::Error("Failed to write asset header to file.");
      return XII_FAILURE;
    }

    xiiUInt8 uiVersion = 1;
    file << uiVersion;

    xiiUInt8 uiFormat = 1; // 1 == PNG
    file << uiFormat;

    xiiStbImageFileFormats pngWriter;
    if (pngWriter.WriteImage(file, image, "png").Failed())
    {
      xiiLog::Error("Failed to write data as PNG to xiiImageData file.");
      return XII_FAILURE;
    }

    return file.Close();
  }
  else if (IsTexFormat())
  {
    xiiDeferredFileWriter file;
    file.SetOutput(szFile);

    XII_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(szFile);
  }
}

xiiApplication::Execution xiiTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
    return xiiApplication::Execution::Quit;

  if (m_Processor.Process().Failed())
    return xiiApplication::Execution::Quit;

  if (m_Processor.m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas)
  {
    xiiDeferredFileWriter file;
    file.SetOutput(m_sOutputFile);

    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    header.Write(file).IgnoreResult();

    m_Processor.m_TextureAtlas.CopyToStream(file).IgnoreResult();

    if (file.Close().Succeeded())
    {
      SetReturnCode(0);
    }
    else
    {
      xiiLog::Error("Failed to write atlas output image.");
    }

    return xiiApplication::Execution::Quit;
  }

  if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
  {
    if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
    {
      xiiLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return xiiApplication::Execution::Quit;
    }

    xiiLog::Success("Wrote main result to '{}'", m_sOutputFile);
  }

  if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      xiiLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return xiiApplication::Execution::Quit;
    }

    xiiLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
  }

  if (!m_sOutputLowResFile.IsEmpty())
  {
    // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
    xiiOSFile::DeleteFile(m_sOutputLowResFile).IgnoreResult();

    if (m_Processor.m_LowResOutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
      {
        xiiLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
        return xiiApplication::Execution::Quit;
      }

      xiiLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
    }
  }

  SetReturnCode(0);
  return xiiApplication::Execution::Quit;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiTexConv);
