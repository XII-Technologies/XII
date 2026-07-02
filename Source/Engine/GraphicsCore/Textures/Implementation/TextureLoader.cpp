/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Textures/RenderToTexture2DResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Utilities/TextureFormat.h>

static xiiTextureResourceLoader s_TextureResourceLoader;

xiiCVarFloat cvar_StreamingTextureLoadDelay("Streaming.TextureLoadDelay", 0.0f, xiiCVarFlags::Save, "Artificial texture loading slowdown");

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, TextureResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::SetResourceTypeLoader<xiiTexture2DResource>(&s_TextureResourceLoader);
    xiiResourceManager::SetResourceTypeLoader<xiiTexture3DResource>(&s_TextureResourceLoader);
    xiiResourceManager::SetResourceTypeLoader<xiiTextureCubeResource>(&s_TextureResourceLoader);
    xiiResourceManager::SetResourceTypeLoader<xiiRenderToTexture2DResource>(&s_TextureResourceLoader);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeLoader<xiiTexture2DResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiTexture3DResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiTextureCubeResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiRenderToTexture2DResource>(nullptr);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiResourceLoadData xiiTextureResourceLoader::OpenDataStream(const xiiResource* pResource)
{
  LoadedData* pData = XII_DEFAULT_NEW(LoadedData);

  xiiResourceLoadData res;

  xiiStringView sResourceID = pResource->GetResourceID();

  // Solid Color Textures
  if (sResourceID.HasExtension("color") || sResourceID.StartsWith("#") || (!sResourceID.HasAnyExtension() && !xiiConversionUtils::IsStringUuid(sResourceID)))
  {
    xiiStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool                  bValidColor = false;
    const xiiColorGammaUB color       = xiiConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      xiiLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName);
    }

    pData->m_TexFormat.m_bSRGB = true;

    xiiImageHeader header;
    header.SetWidth(4);
    header.SetHeight(4);
    header.SetDepth(1);
    header.SetImageFormat(xiiImageFormat::R8G8B8A8_UNORM_SRGB);
    header.SetMipLevelCount(1);
    header.SetFaceCount(1);
    pData->m_Image.ResetAndAlloc(header);
    xiiUInt8* pPixels = pData->m_Image.GetPixelPointer<xiiUInt8>();

    for (xiiUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    xiiFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
      return res;

    const xiiStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
    res.m_sResourceDescription           = File.GetFilePathRelative().GetView();

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
    {
      xiiFileStats stat;
      if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (xiiTextureXX format), this is a hack to get the SRGB information for the texture
    const xiiStringBuilder sName = xiiPathUtils::GetFileName(sAbsolutePath);
    pData->m_TexFormat.m_bSRGB   = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("xiiBinTexture2D") || sAbsolutePath.HasExtension("xiiBinTexture3D") || sAbsolutePath.HasExtension("xiiBinTextureCube") || sAbsolutePath.HasExtension("xiiBinRenderTarget") || sAbsolutePath.HasExtension("xiiBinLUT"))
    {
      if (LoadTexFile(File, *pData).Failed())
        return res;
    }
    else
    {
      // read whatever format, as long as xiiImage supports it
      File.Close();

      if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
        return res;

      if (pData->m_Image.GetImageFormat() == xiiImageFormat::B8G8R8_UNORM)
      {
        /// \todo A conversion to B8G8R8X8_UNORM currently fails

        xiiLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath);
        if (xiiImageConversion::Convert(pData->m_Image, pData->m_Image, xiiImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  xiiMemoryStreamWriter w(&pData->m_Storage);

  WriteTextureLoadStream(w, *pData);

  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  if (cvar_StreamingTextureLoadDelay > 0)
  {
    xiiThreadUtils::Sleep(xiiTime::MakeFromSeconds(cvar_StreamingTextureLoadDelay));
  }

  return res;
}

void xiiTextureResourceLoader::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  XII_DEFAULT_DELETE(pData);
}

bool xiiTextureResourceLoader::IsResourceOutdated(const xiiResource* pResource) const
{
  // solid color textures are never outdated
  if (xiiPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

  // don't try to reload a file that cannot be found
  xiiStringBuilder sAbs;
  if (xiiFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    xiiFileStats stat;
    if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), xiiTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

xiiResult xiiTextureResourceLoader::LoadTexFile(xiiStreamReader& inout_stream, LoadedData& ref_data)
{
  // read the hash, ignore it
  xiiAssetFileHeader AssetHash;
  XII_SUCCEED_OR_RETURN(AssetHash.Read(inout_stream));

  ref_data.m_TexFormat.ReadHeader(inout_stream);

  if (ref_data.m_TexFormat.m_iRenderTargetResolutionX == 0)
  {
    xiiDdsFileFormat fmt;
    return fmt.ReadImage(inout_stream, ref_data.m_Image, "dds");
  }
  else
  {
    return XII_SUCCESS;
  }
}

void xiiTextureResourceLoader::WriteTextureLoadStream(xiiStreamWriter& w, const LoadedData& data)
{
  const xiiImage* pImage = &data.m_Image;
  w.WriteBytes(&pImage, sizeof(xiiImage*)).IgnoreResult();

  w << data.m_bIsFallback;
  data.m_TexFormat.WriteRenderTargetHeader(w);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_TextureLoader);
