#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Resources/MeshletResource.h>
#include <GAL/Device/GALDevice.h>
#include <Foundation/IO/Stream.h>

// ---- RTTI ----
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletResource, 1, xiiRTTIDefaultAllocator<xiiMeshletResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// ---- Serialization format version ----
static constexpr xiiUInt32 k_uiMeshletResourceVersion = 1u;

// ---- Boilerplate ----
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMeshletResource);

xiiMeshletResource::xiiMeshletResource()
  : xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiMeshletResource::~xiiMeshletResource() = default;

// ---- GetLODRange ----
void xiiMeshletResource::GetLODRange(xiiUInt8 uiLOD, xiiUInt32& out_uiOffset, xiiUInt32& out_uiCount) const
{
  XII_ASSERT_DEV(uiLOD < m_uiNumLODs, "LOD index {} out of range (max {})", uiLOD, m_uiNumLODs);
  out_uiOffset = m_LODMeshletOffsets[uiLOD];
  out_uiCount  = m_LODMeshletCounts[uiLOD];
}

// ---- CreateGPUBuffers ----
void xiiMeshletResource::CreateGPUBuffers(const xiiMeshletResourceDescriptor& desc)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No GAL device available");

  auto CreateStructuredBuffer = [&](xiiStringView sDebugName, xiiArrayPtr<const xiiUInt8> data, xiiUInt32 uiStride) -> xiiGALBufferHandle
  {
    if (data.IsEmpty())
      return {};

    xiiGALBufferCreationDescription desc2;
    desc2.m_sDebugName         = sDebugName;
    desc2.m_uiSize             = data.GetCount();
    desc2.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    desc2.m_ResourceUsage      = xiiGALResourceUsage::Immutable;
    desc2.m_uiElementByteStride= uiStride;
    desc2.m_Mode               = xiiGALBufferMode::Structured;

    return pDevice->CreateBuffer(desc2, xiiGALBufferInitialData{data.GetPtr(), data.GetCount()});
  };

  // Descriptor buffer
  m_hDescriptorBuffer = CreateStructuredBuffer(
    "MeshletDescriptors",
    xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(desc.m_Meshlets.GetData()), desc.m_Meshlets.GetCount() * sizeof(xiiMeshletDescriptor)),
    sizeof(xiiMeshletDescriptor));

  // Bounds buffer
  m_hBoundsBuffer = CreateStructuredBuffer(
    "MeshletBounds",
    xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(desc.m_MeshletBounds.GetData()), desc.m_MeshletBounds.GetCount() * sizeof(xiiMeshletBounds)),
    sizeof(xiiMeshletBounds));

  // Packed vertex buffer
  m_hVertexBuffer = CreateStructuredBuffer(
    "MeshletVertices",
    xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(desc.m_PackedVertices.GetData()), desc.m_PackedVertices.GetCount() * sizeof(xiiPackedMeshletVertex)),
    sizeof(xiiPackedMeshletVertex));

  // Triangle index list (R8 raw buffer)
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName         = "MeshletTriangles";
    bd.m_uiSize             = desc.m_TriangleIndices.GetCount();
    bd.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage      = xiiGALResourceUsage::Immutable;
    bd.m_Mode               = xiiGALBufferMode::Raw;
    m_hTriangleBuffer = pDevice->CreateBuffer(bd, xiiGALBufferInitialData{desc.m_TriangleIndices.GetData(), bd.m_uiSize});
  }

  // AABB decode constants
  m_hAABBBuffer = CreateStructuredBuffer(
    "MeshletAABBs",
    xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(desc.m_MeshletAABBs.GetData()), desc.m_MeshletAABBs.GetCount() * sizeof(xiiMeshletAABB)),
    sizeof(xiiMeshletAABB));
}

// ---- UpdateContent (load from stream) ----
xiiResourceLoadDesc xiiMeshletResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc res;
  res.m_State                    = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    ReportResourceIsMissing();
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiUInt32 uiVersion = 0;
  *pStream >> uiVersion;
  XII_ASSERT_DEV(uiVersion == k_uiMeshletResourceVersion, "Unknown MeshletResource version {}", uiVersion);

  xiiMeshletResourceDescriptor desc;

  *pStream >> desc.m_uiNumLODs;
  for (xiiUInt8 i = 0; i < desc.m_uiNumLODs; ++i)
  {
    *pStream >> desc.m_LODMeshletOffsets[i];
    *pStream >> desc.m_LODMeshletCounts[i];
    *pStream >> desc.m_LODScreenCoverage[i];
  }

  xiiUInt32 uiMeshletCount = 0; *pStream >> uiMeshletCount;
  desc.m_Meshlets.SetCount(uiMeshletCount);
  pStream->ReadBytes(desc.m_Meshlets.GetData(), uiMeshletCount * sizeof(xiiMeshletDescriptor));

  xiiUInt32 uiBoundsCount = 0; *pStream >> uiBoundsCount;
  desc.m_MeshletBounds.SetCount(uiBoundsCount);
  pStream->ReadBytes(desc.m_MeshletBounds.GetData(), uiBoundsCount * sizeof(xiiMeshletBounds));

  xiiUInt32 uiAABBCount = 0; *pStream >> uiAABBCount;
  desc.m_MeshletAABBs.SetCount(uiAABBCount);
  pStream->ReadBytes(desc.m_MeshletAABBs.GetData(), uiAABBCount * sizeof(xiiMeshletAABB));

  xiiUInt32 uiVertexCount = 0; *pStream >> uiVertexCount;
  desc.m_PackedVertices.SetCount(uiVertexCount);
  pStream->ReadBytes(desc.m_PackedVertices.GetData(), uiVertexCount * sizeof(xiiPackedMeshletVertex));

  xiiUInt32 uiVertexRemapCount = 0; *pStream >> uiVertexRemapCount;
  desc.m_VertexRemap.SetCount(uiVertexRemapCount);
  pStream->ReadBytes(desc.m_VertexRemap.GetData(), uiVertexRemapCount * sizeof(xiiUInt32));

  xiiUInt32 uiTriCount = 0; *pStream >> uiTriCount;
  desc.m_TriangleIndices.SetCount(uiTriCount);
  pStream->ReadBytes(desc.m_TriangleIndices.GetData(), uiTriCount);

  // Cache counts
  m_uiMeshletCount  = uiMeshletCount;
  m_uiVertexCount   = uiVertexCount;
  m_uiTriangleCount = uiTriCount / 3;
  m_uiNumLODs       = desc.m_uiNumLODs;
  for (xiiUInt8 i = 0; i < m_uiNumLODs; ++i)
  {
    m_LODMeshletOffsets[i]  = desc.m_LODMeshletOffsets[i];
    m_LODMeshletCounts[i]   = desc.m_LODMeshletCounts[i];
    m_LODScreenCoverage[i]  = desc.m_LODScreenCoverage[i];
  }

  // Upload to GPU (must happen on the render thread; resource system guarantees this for DoUpdate::OnAnyThread + deferred GPU creation)
  CreateGPUBuffers(desc);

  return res;
}

// ---- CreateResource (from descriptor, e.g. procedural) ----
xiiResourceLoadDesc xiiMeshletResource::CreateResource(xiiMeshletResourceDescriptor&& desc)
{
  m_uiMeshletCount  = desc.m_Meshlets.GetCount();
  m_uiVertexCount   = desc.m_PackedVertices.GetCount();
  m_uiTriangleCount = desc.m_TriangleIndices.GetCount() / 3;
  m_uiNumLODs       = desc.m_uiNumLODs;
  for (xiiUInt8 i = 0; i < m_uiNumLODs; ++i)
  {
    m_LODMeshletOffsets[i]  = desc.m_LODMeshletOffsets[i];
    m_LODMeshletCounts[i]   = desc.m_LODMeshletCounts[i];
    m_LODScreenCoverage[i]  = desc.m_LODScreenCoverage[i];
  }

  CreateGPUBuffers(desc);

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  return res;
}

// ---- UnloadData ----
xiiResourceLoadDesc xiiMeshletResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice)
  {
    if (m_hDescriptorBuffer.IsValid()) { pDevice->DestroyBuffer(m_hDescriptorBuffer); m_hDescriptorBuffer = {}; }
    if (m_hBoundsBuffer.IsValid())     { pDevice->DestroyBuffer(m_hBoundsBuffer);     m_hBoundsBuffer     = {}; }
    if (m_hVertexBuffer.IsValid())     { pDevice->DestroyBuffer(m_hVertexBuffer);     m_hVertexBuffer     = {}; }
    if (m_hTriangleBuffer.IsValid())   { pDevice->DestroyBuffer(m_hTriangleBuffer);   m_hTriangleBuffer   = {}; }
    if (m_hAABBBuffer.IsValid())       { pDevice->DestroyBuffer(m_hAABBBuffer);       m_hAABBBuffer       = {}; }
  }

  m_uiMeshletCount = m_uiVertexCount = m_uiTriangleCount = 0;

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 1;
  return res;
}

// ---- ReportResourceIsMissing ----
void xiiMeshletResource::ReportResourceIsMissing()
{
  xiiLog::Warning("xiiMeshletResource '{}' is missing. Meshlet rendering will be suppressed.", GetResourceID());
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Resources_MeshletResource);
