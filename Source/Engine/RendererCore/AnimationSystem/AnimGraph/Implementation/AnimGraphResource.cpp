#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphResource, 1, xiiRTTIDefaultAllocator<xiiAnimGraphResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiAnimGraphResource);
// clang-format on

xiiAnimGraphResource::xiiAnimGraphResource() :
  xiiResource(xiiResource::DoUpdate::OnAnyThread, 0)
{
}

xiiAnimGraphResource::~xiiAnimGraphResource() = default;

void xiiAnimGraphResource::DeserializeAnimGraphState(xiiAnimGraph& out)
{
  xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> wrapper(&m_Storage);
  xiiMemoryStreamReader                                 reader(&wrapper);
  out.Deserialize(reader).IgnoreResult();
}

xiiResourceLoadDesc xiiAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  m_Storage.Clear();
  m_Storage.Compact();

  xiiResourceLoadDesc d;
  d.m_State                      = xiiResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable    = 0;
  return d;
}

xiiResourceLoadDesc xiiAnimGraphResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  xiiUInt32 uiDateSize = 0;
  *Stream >> uiDateSize;
  m_Storage.SetCountUninitialized(uiDateSize);
  Stream->ReadBytes(m_Storage.GetData(), uiDateSize);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_Storage.GetHeapMemoryUsage();
}
