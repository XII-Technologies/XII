/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Resources/BlackboardTemplateResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBlackboardTemplateResource, 1, xiiRTTIDefaultAllocator<xiiBlackboardTemplateResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiBlackboardTemplateResource);

xiiBlackboardTemplateResource::xiiBlackboardTemplateResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiBlackboardTemplateResource::~xiiBlackboardTemplateResource() = default;

xiiResourceLoadDescription xiiBlackboardTemplateResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiBlackboardTemplateResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiBlackboardTemplateResource::UpdateContent", GetResourceDescription().GetData());

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

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  xiiBlackboardTemplateResourceDescriptor desc;
  if (desc.Deserialize(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(desc));

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiBlackboardTemplateResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiBlackboardTemplateResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Entries.GetHeapMemoryUsage();
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiBlackboardTemplateResource, xiiBlackboardTemplateResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResult xiiBlackboardTemplateResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));
  return XII_SUCCESS;
}

xiiResult xiiBlackboardTemplateResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));
  return XII_SUCCESS;
}
