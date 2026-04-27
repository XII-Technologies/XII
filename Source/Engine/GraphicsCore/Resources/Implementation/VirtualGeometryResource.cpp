#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Resources/VirtualGeometryResource.h>
#include <Foundation/IO/Stream.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualGeometryResource, 1, xiiRTTIDefaultAllocator<xiiVirtualGeometryResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiVirtualGeometryResource);

xiiVirtualGeometryResource::xiiVirtualGeometryResource() : xiiResource(DoUpdate::OnAnyThread, 1) {}
xiiVirtualGeometryResource::~xiiVirtualGeometryResource() = default;

xiiResourceLoadDesc xiiVirtualGeometryResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc res;
  res.m_State = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    ReportResourceIsMissing();
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // Basic reading. For a full implementation this would load the DAG hierarchy into GPU buffers.
  *pStream >> m_Bounds;

  return res;
}

xiiResourceLoadDesc xiiVirtualGeometryResource::CreateResource(xiiVirtualGeometryResourceDescriptor&& desc)
{
  m_Bounds = desc.m_Bounds;
  // Upload desc.m_StreamData to GPU...

  xiiResourceLoadDesc res;
  res.m_State = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  return res;
}

xiiResourceLoadDesc xiiVirtualGeometryResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);
  // Free GPU buffers

  xiiResourceLoadDesc res;
  res.m_State = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 1;
  return res;
}

void xiiVirtualGeometryResource::ReportResourceIsMissing()
{
  xiiLog::Warning("xiiVirtualGeometryResource '{}' is missing.", GetResourceID());
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Resources_VirtualGeometryResource);
