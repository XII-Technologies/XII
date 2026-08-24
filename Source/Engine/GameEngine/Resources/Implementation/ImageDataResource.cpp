/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Resources/ImageDataResource.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImageDataResource, 1, xiiRTTIDefaultAllocator<xiiImageDataResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiImageDataResource);

xiiImageDataResource::xiiImageDataResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiImageDataResource::~xiiImageDataResource() = default;

xiiResourceLoadDescription xiiImageDataResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiImageDataResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiImageDataResource::UpdateContent", GetResourceDescription().GetData());

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  xiiImageDataResourceDescriptor desc;

  if (sAbsFilePath.HasExtension("xiiBinImageData"))
  {
    xiiAssetFileHeader AssetHash;
    if (AssetHash.Read(*Stream).Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }

    xiiUInt8 uiVersion    = 0;
    xiiUInt8 uiDataFormat = 0;

    *Stream >> uiVersion;
    *Stream >> uiDataFormat;

    if (uiVersion != 1 || uiDataFormat != 1)
    {
      xiiLog::Error("Unsupported xiiImageData file format or version");

      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }

    xiiStbImageFileFormats fmt;
    if (fmt.ReadImage(*Stream, desc.m_Image, "png").Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }
  }
  else
  {
    xiiStringBuilder ext;
    ext = sAbsFilePath.GetFileExtension();

    if (xiiImageFileFormat::GetReaderFormat(ext)->ReadImage(*Stream, desc.m_Image, ext).Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }
  }

  CreateResource(std::move(desc));

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiImageDataResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiImageDataResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += m_pDescriptor->m_Image.GetByteBlobPtr().GetCount();
  }
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiImageDataResource, xiiImageDataResourceDescriptor)
{
  m_pDescriptor = XII_DEFAULT_NEW(xiiImageDataResourceDescriptor);

  *m_pDescriptor = std::move(descriptor);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (m_pDescriptor->m_Image.Convert(xiiGALResourceFormat::RGBA32Float).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
  }

  return res;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Utils_Implementation_ImageDataResource);
