#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <GraphicsCore/BakedProbes/ProbeTreeSectorResource.h>

xiiProbeTreeSectorResourceDescriptor::xiiProbeTreeSectorResourceDescriptor()        = default;
xiiProbeTreeSectorResourceDescriptor::~xiiProbeTreeSectorResourceDescriptor()       = default;
xiiProbeTreeSectorResourceDescriptor& xiiProbeTreeSectorResourceDescriptor::operator=(xiiProbeTreeSectorResourceDescriptor&& other) = default;

void xiiProbeTreeSectorResourceDescriptor::Clear()
{
  m_ProbePositions.Clear();
  m_SkyVisibility.Clear();
}

xiiUInt64 xiiProbeTreeSectorResourceDescriptor::GetHeapMemoryUsage() const
{
  xiiUInt64 uiMemUsage = 0;
  uiMemUsage += m_ProbePositions.GetHeapMemoryUsage();
  uiMemUsage += m_SkyVisibility.GetHeapMemoryUsage();
  return uiMemUsage;
}

static xiiTypeVersion s_ProbeTreeResourceDescriptorVersion = 1;
xiiResult             xiiProbeTreeSectorResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream << m_vGridOrigin;
  inout_stream << m_vProbeSpacing;
  inout_stream << m_vProbeCount;

  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_ProbePositions));
  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_SkyVisibility));

  return XII_SUCCESS;
}

xiiResult xiiProbeTreeSectorResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  Clear();

  const xiiTypeVersion version = inout_stream.ReadVersion(s_ProbeTreeResourceDescriptorVersion);
  XII_IGNORE_UNUSED(version);

  inout_stream >> m_vGridOrigin;
  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_vProbeCount;

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ProbePositions));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_SkyVisibility));

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProbeTreeSectorResource, 1, xiiRTTIDefaultAllocator<xiiProbeTreeSectorResource>);
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiProbeTreeSectorResource);
// clang-format on

xiiProbeTreeSectorResource::xiiProbeTreeSectorResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiProbeTreeSectorResource::~xiiProbeTreeSectorResource() = default;

xiiResourceLoadDesc xiiProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  m_Desc.Clear();

  return res;
}

xiiResourceLoadDesc xiiProbeTreeSectorResource::UpdateContent(xiiStreamReader* Stream)
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
    xiiString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  xiiProbeTreeSectorResourceDescriptor descriptor;
  if (descriptor.Deserialize(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
}

void xiiProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiProbeTreeSectorResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Desc.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

xiiResourceLoadDesc xiiProbeTreeSectorResource::CreateResource(xiiProbeTreeSectorResourceDescriptor&& descriptor)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  m_Desc = std::move(descriptor);

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_BakedProbes_Implementation_ProbeTreeSectorResource);
