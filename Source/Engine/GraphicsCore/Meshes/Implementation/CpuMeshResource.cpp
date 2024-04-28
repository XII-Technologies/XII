#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GraphicsCore/Meshes/CpuMeshResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCpuMeshResource, 1, xiiRTTIDefaultAllocator<xiiCpuMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiCpuMeshResource);
// clang-format on

xiiCpuMeshResource::xiiCpuMeshResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiResourceLoadDesc xiiCpuMeshResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_State                      = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable    = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_Descriptor.Clear();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::Unloaded;
  }

  return res;
}

xiiResourceLoadDesc xiiCpuMeshResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiMeshResourceDescriptor desc;
  xiiResourceLoadDesc       res;
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

  if (m_Descriptor.Load(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiCpuMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiCpuMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiCpuMeshResource, xiiMeshResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_CpuMeshResource);
