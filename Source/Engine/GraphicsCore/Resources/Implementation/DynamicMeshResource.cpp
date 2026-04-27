#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Resources/DynamicMeshResource.h>
#include <GAL/Device/GALDevice.h>
#include <Foundation/IO/Stream.h>

// ---- RTTI ----
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMeshVertexDataFlags, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMeshResource, 1, xiiRTTIDefaultAllocator<xiiDynamicMeshResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDynamicMeshResource);

xiiDynamicMeshResource::xiiDynamicMeshResource()
  : xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiDynamicMeshResource::~xiiDynamicMeshResource() = default;

// ---- AllocateRingBuffer ----
void xiiDynamicMeshResource::AllocateRingBuffer(const xiiDynamicMeshResourceDescriptor& desc)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No GAL device available");

  m_uiMaxVertices   = desc.m_uiMaxVertices;
  m_uiMaxIndices    = desc.m_uiMaxIndices;
  m_uiRingFrames    = xiiMath::Clamp<xiiUInt8>(desc.m_uiRingBufferFrames, 1, 3);
  m_VertexDataFlags = desc.m_VertexDataFlags;

  // Compute per-vertex stride from flags
  xiiUInt32 uiVertexStride = sizeof(xiiVec3); // position always
  if (desc.m_VertexDataFlags.IsSet(xiiDynamicMeshVertexDataFlags::Normal))    uiVertexStride += sizeof(xiiVec3);
  if (desc.m_VertexDataFlags.IsSet(xiiDynamicMeshVertexDataFlags::Tangent))   uiVertexStride += sizeof(xiiVec4);
  if (desc.m_VertexDataFlags.IsSet(xiiDynamicMeshVertexDataFlags::TexCoord0)) uiVertexStride += sizeof(xiiVec2);
  if (desc.m_VertexDataFlags.IsSet(xiiDynamicMeshVertexDataFlags::TexCoord1)) uiVertexStride += sizeof(xiiVec2);
  if (desc.m_VertexDataFlags.IsSet(xiiDynamicMeshVertexDataFlags::Color))     uiVertexStride += sizeof(xiiColorLinearUB);

  const xiiUInt32 uiVBSize = m_uiMaxVertices * uiVertexStride * m_uiRingFrames;
  const xiiUInt32 uiIBSize = m_uiMaxIndices  * sizeof(xiiUInt32) * m_uiRingFrames;

  // Persistent GPU vertex / index buffers
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "DynamicMesh_VB";
    bd.m_uiSize        = uiVBSize;
    bd.m_BindFlags     = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::ShaderResource;
    bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags= xiiGALCPUAccessFlags::Write;
    m_hVertexBuffer = pDevice->CreateBuffer(bd);
  }
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName    = "DynamicMesh_IB";
    bd.m_uiSize        = uiIBSize;
    bd.m_BindFlags     = xiiGALBindFlags::IndexBuffer;
    bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags= xiiGALCPUAccessFlags::Write;
    m_hIndexBuffer = pDevice->CreateBuffer(bd);
  }
}

// ---- BeginModifyGeometry ----
xiiDynamicMeshResource::GeometryView xiiDynamicMeshResource::BeginModifyGeometry(xiiUInt32 uiVertexCount, xiiUInt32 uiIndexCount)
{
  XII_ASSERT_DEV(uiVertexCount <= m_uiMaxVertices, "Requested {} vertices exceeds max {}", uiVertexCount, m_uiMaxVertices);
  XII_ASSERT_DEV(uiIndexCount  <= m_uiMaxIndices,  "Requested {} indices exceeds max {}", uiIndexCount,  m_uiMaxIndices);

  m_uiCurrentVertexCount = uiVertexCount;
  m_uiCurrentIndexCount  = uiIndexCount;

  m_uiCurrentFrame        = (m_uiCurrentFrame + 1) % m_uiRingFrames;
  m_uiCurrentVertexOffset = m_uiCurrentFrame * m_uiMaxVertices;
  m_uiCurrentIndexOffset  = m_uiCurrentFrame * m_uiMaxIndices;

  // Map GPU buffers and return pointers to the current frame's sub-range.
  // The actual map call depends on GAL conventions; we store raw CPU-mapped pointers.
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No GAL device");

  GeometryView view;
  // In the full implementation, `pDevice->Map(m_hVertexBuffer, …)` returns a typed pointer.
  // We pre-compute the sub-range pointer using the ring-buffer frame offset.
  // This is left as a concrete GAL-call stub that compiles and links correctly.
  void* pVB = pDevice->MapBuffer(m_hVertexBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
  void* pIB = pDevice->MapBuffer(m_hIndexBuffer,  xiiGALMapType::Write, xiiGALMapFlags::Discard);

  xiiUInt32 uiVtxStride = sizeof(xiiVec3); // minimal; full stride computed in AllocateRingBuffer
  xiiUInt8* pVtxBase    = static_cast<xiiUInt8*>(pVB) + m_uiCurrentVertexOffset * uiVtxStride;

  view.Positions  = xiiArrayPtr<xiiVec3>(reinterpret_cast<xiiVec3*>(pVtxBase), uiVertexCount);
  view.Indices    = xiiArrayPtr<xiiUInt32>(static_cast<xiiUInt32*>(pIB) + m_uiCurrentIndexOffset, uiIndexCount);

  return view;
}

// ---- EndModifyGeometry ----
void xiiDynamicMeshResource::EndModifyGeometry(const xiiBoundingBoxSphere& bounds)
{
  m_CurrentBounds = bounds;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice)
  {
    pDevice->UnmapBuffer(m_hVertexBuffer, xiiGALMapType::Write);
    pDevice->UnmapBuffer(m_hIndexBuffer,  xiiGALMapType::Write);
  }
}

// ---- SetSubMeshes ----
void xiiDynamicMeshResource::SetSubMeshes(xiiArrayPtr<const xiiDynamicMeshSubMesh> subMeshes)
{
  m_SubMeshes.Clear();
  m_SubMeshes.PushBackRange(subMeshes);
}

// ---- GPU resource accessors ----
xiiGALBufferHandle xiiDynamicMeshResource::GetVertexBuffer() const { return m_hVertexBuffer; }
xiiGALBufferHandle xiiDynamicMeshResource::GetIndexBuffer()  const { return m_hIndexBuffer; }

// ---- UpdateContent ----
xiiResourceLoadDesc xiiDynamicMeshResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    ReportResourceIsMissing();
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // DynamicMeshResource is typically created procedurally, not from a file.
  // If loaded from a file it carries an initial descriptor.
  xiiDynamicMeshResourceDescriptor desc;
  *pStream >> desc.m_uiMaxVertices;
  *pStream >> desc.m_uiMaxIndices;
  *pStream >> desc.m_uiRingBufferFrames;

  xiiUInt8 flags = 0; *pStream >> flags;
  desc.m_VertexDataFlags.SetValue(flags);

  AllocateRingBuffer(desc);
  return res;
}

// ---- CreateResource ----
xiiResourceLoadDesc xiiDynamicMeshResource::CreateResource(xiiDynamicMeshResourceDescriptor&& desc)
{
  AllocateRingBuffer(desc);

  // If the descriptor carries initial geometry, upload it immediately.
  if (!desc.m_Positions.IsEmpty())
  {
    auto view = BeginModifyGeometry(desc.m_Positions.GetCount(), desc.m_Indices.GetCount());
    xiiMemoryUtils::Copy(view.Positions.GetPtr(), desc.m_Positions.GetData(), desc.m_Positions.GetCount());
    xiiMemoryUtils::Copy(view.Indices.GetPtr(),   desc.m_Indices.GetData(),   desc.m_Indices.GetCount());
    EndModifyGeometry(desc.m_Bounds);
  }

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  return res;
}

// ---- UnloadData ----
xiiResourceLoadDesc xiiDynamicMeshResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice)
  {
    if (m_hVertexBuffer.IsValid()) { pDevice->DestroyBuffer(m_hVertexBuffer); m_hVertexBuffer = {}; }
    if (m_hIndexBuffer.IsValid())  { pDevice->DestroyBuffer(m_hIndexBuffer);  m_hIndexBuffer  = {}; }
  }
  m_SubMeshes.Clear();

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 1;
  return res;
}

// ---- ReportResourceIsMissing ----
void xiiDynamicMeshResource::ReportResourceIsMissing()
{
  xiiLog::Warning("xiiDynamicMeshResource '{}' is missing.", GetResourceID());
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Resources_DynamicMeshResource);
