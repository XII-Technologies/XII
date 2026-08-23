/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradientResource, 1, xiiRTTIDefaultAllocator<xiiColorGradientResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiColorGradientResource);

xiiColorGradientResource::xiiColorGradientResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiColorGradientResource, xiiColorGradientResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDescription xiiColorGradientResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

xiiResourceLoadDescription xiiColorGradientResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiColorGradientResource::UpdateContent", GetResourceIdOrDescription());

  xiiResourceLoadDescription res;
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

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<xiiUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<xiiUInt32>(sizeof(m_Descriptor));
}

void xiiColorGradientResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 1;

  ref_stream << uiVersion;

  m_Gradient.Save(ref_stream);
}

void xiiColorGradientResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;

  ref_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(ref_stream);
}



XII_STATICLINK_FILE(Core, Core_Curves_Implementation_ColorGradientResource);
