/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGeometryResidencyState, 1)
  XII_ENUM_CONSTANTS(xiiGeometryResidencyState::Unloaded, xiiGeometryResidencyState::Requested, xiiGeometryResidencyState::Loading)
  XII_ENUM_CONSTANTS(xiiGeometryResidencyState::Resident, xiiGeometryResidencyState::EvictPending, xiiGeometryResidencyState::Failed)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryHandle>)
{
  XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Index", m_uiIndex), XII_MEMBER_PROPERTY("Generation", m_uiGeneration), } XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryLodSource, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryLodSource>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MeshBuffer", GetMeshBufferResourceId, SetMeshBufferResourceId),
    XII_MEMBER_PROPERTY("MinimumScreenCoverage", m_fMinimumScreenCoverage),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

xiiString xiiGeometryLodSource::GetMeshBufferResourceId() const
{
  return m_hMeshBuffer.IsValid() ? xiiString(m_hMeshBuffer.GetResourceID()) : xiiString();
}

void xiiGeometryLodSource::SetMeshBufferResourceId(xiiString sResourceId)
{
  m_hMeshBuffer = sResourceId.IsEmpty() ? xiiMeshBufferResourceHandle() : xiiResourceManager::LoadResource<xiiMeshBufferResource>(sResourceId);
}

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Lods", m_Lods),
    XII_MEMBER_PROPERTY("StreamingPriority", m_uiStreamingPriority),
    XII_MEMBER_PROPERTY("Pinned", m_bPinned),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuGeometryLod, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuGeometryLod>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VertexBufferIndex", m_uiVertexBufferIndex),
    XII_MEMBER_PROPERTY("IndexBufferIndex", m_uiIndexBufferIndex),
    XII_MEMBER_PROPERTY("MeshletBufferIndex", m_uiMeshletBufferIndex),
    XII_MEMBER_PROPERTY("MeshletRemapBufferIndex", m_uiMeshletRemapBufferIndex),
    XII_MEMBER_PROPERTY("MeshletPrimitiveBufferIndex", m_uiMeshletPrimitiveBufferIndex),
    XII_MEMBER_PROPERTY("VertexCount", m_uiVertexCount),
    XII_MEMBER_PROPERTY("IndexCount", m_uiIndexCount),
    XII_MEMBER_PROPERTY("MeshletCount", m_uiMeshletCount),
    XII_MEMBER_PROPERTY("MinimumScreenCoverage", m_fMinimumScreenCoverage),
    XII_MEMBER_PROPERTY("IndexType", m_uiIndexType),
    XII_MEMBER_PROPERTY("MeshletMetadataOffset", m_uiMeshletMetadataOffset),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuGeometryRecord, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuGeometryRecord>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BoundsCenterRadius", m_BoundsCenterRadius),
    XII_MEMBER_PROPERTY("BoundsExtents", m_BoundsExtents),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("Lods", GetReflectedLodCount, GetReflectedLod),
    XII_MEMBER_PROPERTY("LodCount", m_uiLodCount),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    XII_MEMBER_PROPERTY("ResidentLodMask", m_uiResidentLodMask),
    XII_MEMBER_PROPERTY("Flags", m_uiFlags),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryResidencyStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryResidencyStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GeometryCount", m_uiGeometryCount),
    XII_MEMBER_PROPERTY("ResidentGeometryCount", m_uiResidentGeometryCount),
    XII_MEMBER_PROPERTY("PendingGeometryCount", m_uiPendingGeometryCount),
    XII_MEMBER_PROPERTY("ResidentBytes", m_uiResidentBytes),
    XII_MEMBER_PROPERTY("BudgetBytes", m_uiBudgetBytes),
    XII_MEMBER_PROPERTY("UploadedBytes", m_uiUploadedBytes),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

xiiGeometryResidencyManager::~xiiGeometryResidencyManager() { Shutdown(); }

xiiResult xiiGeometryResidencyManager::Initialize(xiiGALDevice* pDevice, xiiUInt32 uiMaxGeometries, xiiUInt32 uiFramesInFlight, xiiUInt64 uiBudgetBytes, xiiUInt32 uiMaxMeshlets)
{
  Shutdown();
  if (pDevice == nullptr || uiMaxGeometries == 0U || uiFramesInFlight == 0U || uiFramesInFlight > 64U || uiMaxMeshlets == 0U)
    return XII_FAILURE;

  const xiiUInt64 uiSize = static_cast<xiiUInt64>(sizeof(xiiGpuGeometryRecord)) * uiMaxGeometries * uiFramesInFlight;
  if (uiSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  xiiGALBufferCreationDescription desc;
  desc.m_uiSize = static_cast<xiiUInt32>(uiSize);
  desc.m_uiElementByteStride = sizeof(xiiGpuGeometryRecord);
  desc.m_BindFlags = xiiGALBindFlags::ShaderResource;
  desc.m_Mode = xiiGALBufferMode::Structured;
  desc.m_Usage = xiiGALResourceUsage::Mutable;
  m_pMetadataBuffer = pDevice->CreateBuffer(desc);
  if (m_pMetadataBuffer == nullptr)
    return XII_FAILURE;
  m_pMetadataBuffer->SetDebugName("GPU Geometry Metadata");

  desc.m_uiSize = uiMaxMeshlets * sizeof(xiiMeshlet);
  desc.m_uiElementByteStride = sizeof(xiiMeshlet);
  m_pMeshletMetadataBuffer = pDevice->CreateBuffer(desc);
  if (m_pMeshletMetadataBuffer == nullptr)
  {
    Shutdown();
    return XII_FAILURE;
  }
  m_pMeshletMetadataBuffer->SetDebugName("GPU Meshlet Metadata Arena");
  m_FreeMeshletRanges.PushBack({0U, uiMaxMeshlets});

  m_Slots.SetCount(uiMaxGeometries);
  m_FreeSlots.Reserve(uiMaxGeometries);
  for (xiiUInt32 i = uiMaxGeometries; i > 0U; --i)
    m_FreeSlots.PushBack(i - 1U);
  m_uiFramesInFlight = uiFramesInFlight;
  m_uiAllFrameMask = uiFramesInFlight == 64U ? xiiMath::MaxValue<xiiUInt64>() : (xiiUInt64(1) << uiFramesInFlight) - 1U;
  m_uiBudgetBytes = uiBudgetBytes;
  return XII_SUCCESS;
}

void xiiGeometryResidencyManager::Shutdown()
{
  m_pMetadataBuffer.Clear();
  m_pMeshletMetadataBuffer.Clear();
  m_Slots.Clear();
  m_FreeSlots.Clear();
  m_FreeMeshletRanges.Clear();
  m_PendingMeshletUploads.Clear();
  m_uiFramesInFlight = 0U;
  m_uiAllFrameMask = 0U;
  m_uiBudgetBytes = 0U;
  m_uiResidentBytes = 0U;
  m_uiLastUploadedBytes = 0U;
}

xiiGeometryHandle xiiGeometryResidencyManager::RegisterGeometry(const xiiGeometryDescription& description)
{
  if (m_pMetadataBuffer == nullptr || m_FreeSlots.IsEmpty() || description.m_Lods.IsEmpty() || description.m_Lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
    return {};

  const xiiUInt32 uiIndex = m_FreeSlots.PeekBack();
  m_FreeSlots.PopBack();
  Slot& slot = m_Slots[uiIndex];
  slot.m_Description = description;
  slot.m_GpuRecord = {};
  slot.m_GpuRecord.m_uiGeneration = slot.m_uiGeneration;
  slot.m_GpuRecord.m_uiLodCount = description.m_Lods.GetCount();
  slot.m_State = xiiGeometryResidencyState::Unloaded;
  slot.m_bAllocated = true;
  slot.m_uiDirtyFrameMask = m_uiAllFrameMask;
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaOffset, XII_ARRAY_SIZE(slot.m_uiMeshletArenaOffset));
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaCount, XII_ARRAY_SIZE(slot.m_uiMeshletArenaCount));

  xiiGeometryHandle handle;
  handle.m_uiIndex = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiGeometryResidencyManager::UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle)) return;
  Slot& slot = m_Slots[handle.m_uiIndex];
  slot.m_State = xiiGeometryResidencyState::EvictPending;
  slot.m_uiRetireFrame = uiFrameIndex;
}

void xiiGeometryResidencyManager::RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle)) return;
  Slot& slot = m_Slots[handle.m_uiIndex];
  slot.m_uiLastUsedFrame = uiFrameIndex;
  slot.m_uiRequestedLod = xiiMath::Min(uiMinimumLod, slot.m_Description.m_Lods.GetCount() - 1U);

  bool bRequiresStreaming = false;
  for (xiiUInt32 i = slot.m_uiRequestedLod; i < slot.m_Description.m_Lods.GetCount(); ++i)
  {
    if ((slot.m_GpuRecord.m_uiResidentLodMask & XII_BIT(i)) == 0U)
    {
      bRequiresStreaming = true;
      xiiResourceManager::PreloadResource(slot.m_Description.m_Lods[i].m_hMeshBuffer);
    }
  }

  // A resident geometry may receive a finer request later. Keep the existing LODs live while
  // loading the missing range; BuildResidentRecord commits the additions transactionally.
  if (bRequiresStreaming && slot.m_State != xiiGeometryResidencyState::EvictPending)
    slot.m_State = xiiGeometryResidencyState::Requested;
}

void xiiGeometryResidencyManager::Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (IsValid(handle)) m_Slots[handle.m_uiIndex].m_uiLastUsedFrame = uiFrameIndex;
}

bool xiiGeometryResidencyManager::BuildResidentRecord(Slot& slot, xiiUInt64& inout_uiUploadBudget)
{
  xiiGpuGeometryRecord record = slot.m_GpuRecord;
  xiiUInt64 uiNewBytes = 0U;

  struct PendingAllocation
  {
    xiiUInt32 m_uiLod = 0U;
    xiiUInt32 m_uiOffset = 0U;
    xiiUInt32 m_uiCount = 0U;
  };
  xiiHybridArray<PendingAllocation, xiiGpuGeometryRecord::s_uiMaxLods> allocations;
  xiiHybridArray<UploadPassData::MeshletUpload, xiiGpuGeometryRecord::s_uiMaxLods> uploads;
  xiiUInt32 uiBoundsLod = record.m_uiResidentLodMask != 0U ? xiiMath::FirstBitLow(record.m_uiResidentLodMask) : xiiInvalidIndex;

  auto rollback = [&]() {
    for (const PendingAllocation& allocation : allocations)
      FreeMeshlets(allocation.m_uiOffset, allocation.m_uiCount);
  };

  for (xiiUInt32 i = slot.m_uiRequestedLod; i < slot.m_Description.m_Lods.GetCount(); ++i)
  {
    if ((record.m_uiResidentLodMask & XII_BIT(i)) != 0U)
      continue;

    const xiiGeometryLodSource& source = slot.m_Description.m_Lods[i];
    xiiResourceLock<xiiMeshBufferResource> mesh(source.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
    if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      rollback();
      return false;
    }

    xiiGpuGeometryLod& lod = record.m_Lods[i];
    lod.m_uiVertexCount = mesh->GetVertexCount();
    lod.m_uiIndexCount = mesh->GetIndexCount();
    lod.m_uiMeshletCount = mesh->GetMeshletCount();
    lod.m_fMinimumScreenCoverage = source.m_fMinimumScreenCoverage;
    lod.m_uiIndexType = mesh->GetIndexType().GetValue();
    uiNewBytes += static_cast<xiiUInt64>(mesh->GetVertexCount()) * mesh->GetVertexStride();
    uiNewBytes += static_cast<xiiUInt64>(mesh->GetIndexCount()) * (mesh->GetIndexType() == xiiGALValueType::UInt16 ? 2U : 4U);
    uiNewBytes += static_cast<xiiUInt64>(mesh->GetMeshletCount()) * sizeof(xiiMeshlet);
    record.m_uiResidentLodMask |= XII_BIT(i);

    if (lod.m_uiMeshletCount > 0U)
    {
      xiiUInt32 uiArenaOffset = 0U;
      if (!AllocateMeshlets(lod.m_uiMeshletCount, uiArenaOffset))
      {
        rollback();
        return false;
      }
      lod.m_uiMeshletMetadataOffset  = uiArenaOffset;

      allocations.PushBack({i, uiArenaOffset, lod.m_uiMeshletCount});
      UploadPassData::MeshletUpload& upload = uploads.ExpandAndGetRef();
      upload.m_uiOffset = uiArenaOffset * sizeof(xiiMeshlet);
      upload.m_Meshlets = mesh->GetMeshlets();
      for (xiiMeshlet& meshlet : upload.m_Meshlets)
        meshlet.m_uiLodIndex = static_cast<xiiUInt16>(i);
    }

    if (i < uiBoundsLod)
    {
      const xiiBoundingBoxSphere& bounds = mesh->GetBounds();
      record.m_BoundsCenterRadius = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fSphereRadius);
      record.m_BoundsExtents = xiiVec4(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z, 0.0f);
      uiBoundsLod = i;
    }
  }

  if (uiNewBytes > inout_uiUploadBudget)
  {
    rollback();
    return false;
  }

  for (const PendingAllocation& allocation : allocations)
  {
    slot.m_uiMeshletArenaOffset[allocation.m_uiLod] = allocation.m_uiOffset;
    slot.m_uiMeshletArenaCount[allocation.m_uiLod] = allocation.m_uiCount;
  }
  for (UploadPassData::MeshletUpload& upload : uploads)
    m_PendingMeshletUploads.PushBack(std::move(upload));

  inout_uiUploadBudget -= uiNewBytes;
  slot.m_uiResidentBytes += uiNewBytes;
  m_uiResidentBytes += uiNewBytes;
  slot.m_GpuRecord = record;
  slot.m_State = xiiGeometryResidencyState::Resident;
  slot.m_uiDirtyFrameMask = m_uiAllFrameMask;
  return true;
}

void xiiGeometryResidencyManager::EnforceBudget(xiiUInt64 uiCompletedFrame)
{
  while (m_uiResidentBytes > m_uiBudgetBytes)
  {
    xiiUInt32 uiVictim = xiiInvalidIndex;
    xiiUInt64 uiOldest = xiiMath::MaxValue<xiiUInt64>();
    for (xiiUInt32 i = 0; i < m_Slots.GetCount(); ++i)
    {
      const Slot& slot = m_Slots[i];
      if (slot.m_bAllocated && slot.m_State == xiiGeometryResidencyState::Resident && !slot.m_Description.m_bPinned && slot.m_uiLastUsedFrame <= uiCompletedFrame && slot.m_uiLastUsedFrame < uiOldest)
      {
        uiOldest = slot.m_uiLastUsedFrame;
        uiVictim = i;
      }
    }
    if (uiVictim == xiiInvalidIndex) break;
    Slot& victim = m_Slots[uiVictim];
    m_uiResidentBytes -= victim.m_uiResidentBytes;
    victim.m_uiResidentBytes = 0U;
    victim.m_GpuRecord.m_uiResidentLodMask = 0U;
    ReleaseMeshletAllocations(victim);
    victim.m_State = xiiGeometryResidencyState::Unloaded;
    victim.m_uiDirtyFrameMask = m_uiAllFrameMask;
  }
}

void xiiGeometryResidencyManager::ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes)
{
  xiiDynamicArray<xiiUInt32> streamingQueue;
  for (xiiUInt32 i = 0; i < m_Slots.GetCount(); ++i)
  {
    Slot& slot = m_Slots[i];
    if (!slot.m_bAllocated) continue;
    if (slot.m_State == xiiGeometryResidencyState::Requested || slot.m_State == xiiGeometryResidencyState::Loading)
      streamingQueue.PushBack(i);
    if (slot.m_State == xiiGeometryResidencyState::EvictPending && slot.m_uiRetireFrame <= uiCompletedFrame)
    {
      m_uiResidentBytes -= slot.m_uiResidentBytes;
      ReleaseMeshletAllocations(slot);
      slot.m_bAllocated = false;
      slot.m_Description.m_Lods.Clear();
      slot.m_uiResidentBytes = 0U;
      ++slot.m_uiGeneration;
      if (slot.m_uiGeneration == 0U) slot.m_uiGeneration = 1U;
      m_FreeSlots.PushBack(i);
    }
  }

  // Streaming priority is the primary authoring control. Recency breaks equal-priority ties so
  // actively visible content wins a constrained upload budget, while the slot index keeps the
  // schedule deterministic for captures and simulation replay.
  streamingQueue.Sort([this](xiiUInt32 lhsIndex, xiiUInt32 rhsIndex) {
    const Slot& lhs = m_Slots[lhsIndex];
    const Slot& rhs = m_Slots[rhsIndex];
    if (lhs.m_Description.m_uiStreamingPriority != rhs.m_Description.m_uiStreamingPriority)
      return lhs.m_Description.m_uiStreamingPriority > rhs.m_Description.m_uiStreamingPriority;
    if (lhs.m_uiLastUsedFrame != rhs.m_uiLastUsedFrame)
      return lhs.m_uiLastUsedFrame > rhs.m_uiLastUsedFrame;
    return lhsIndex < rhsIndex;
  });

  for (xiiUInt32 uiSlotIndex : streamingQueue)
  {
    Slot& slot = m_Slots[uiSlotIndex];
    slot.m_State = xiiGeometryResidencyState::Loading;
    BuildResidentRecord(slot, uiUploadBudgetBytes);
  }

  EnforceBudget(uiCompletedFrame);
  XII_IGNORE_UNUSED(uiFrameIndex);
}

bool xiiGeometryResidencyManager::SetBindlessIndices(xiiGeometryHandle handle, xiiUInt32 uiLod, xiiUInt32 uiVertex, xiiUInt32 uiIndex, xiiUInt32 uiMeshlet, xiiUInt32 uiRemap, xiiUInt32 uiPrimitive)
{
  if (!IsValid(handle) || uiLod >= m_Slots[handle.m_uiIndex].m_GpuRecord.m_uiLodCount) return false;
  xiiGpuGeometryLod& lod = m_Slots[handle.m_uiIndex].m_GpuRecord.m_Lods[uiLod];
  lod.m_uiVertexBufferIndex = uiVertex;
  lod.m_uiIndexBufferIndex = uiIndex;
  lod.m_uiMeshletBufferIndex = uiMeshlet;
  lod.m_uiMeshletRemapBufferIndex = uiRemap;
  lod.m_uiMeshletPrimitiveBufferIndex = uiPrimitive;
  m_Slots[handle.m_uiIndex].m_uiDirtyFrameMask = m_uiAllFrameMask;
  return true;
}

bool xiiGeometryResidencyManager::IsValid(xiiGeometryHandle handle) const
{
  return handle.IsValid() && handle.m_uiIndex < m_Slots.GetCount() && m_Slots[handle.m_uiIndex].m_bAllocated && m_Slots[handle.m_uiIndex].m_uiGeneration == handle.m_uiGeneration;
}

xiiEnum<xiiGeometryResidencyState> xiiGeometryResidencyManager::GetState(xiiGeometryHandle handle) const
{
  if (IsValid(handle))
    return m_Slots[handle.m_uiIndex].m_State;
  return xiiEnum<xiiGeometryResidencyState>(xiiGeometryResidencyState::Unloaded);
}

const xiiGpuGeometryRecord* xiiGeometryResidencyManager::GetGpuRecord(xiiGeometryHandle handle) const
{
  return IsValid(handle) ? &m_Slots[handle.m_uiIndex].m_GpuRecord : nullptr;
}

xiiGeometryResidencyStats xiiGeometryResidencyManager::GetStats() const
{
  xiiGeometryResidencyStats stats;
  stats.m_uiBudgetBytes = m_uiBudgetBytes;
  stats.m_uiResidentBytes = m_uiResidentBytes;
  stats.m_uiUploadedBytes = m_uiLastUploadedBytes;
  for (const Slot& slot : m_Slots)
  {
    if (!slot.m_bAllocated) continue;
    ++stats.m_uiGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Resident) ++stats.m_uiResidentGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Requested || slot.m_State == xiiGeometryResidencyState::Loading) ++stats.m_uiPendingGeometryCount;
  }
  return stats;
}

xiiGeometryResidencyManager::UploadHandles xiiGeometryResidencyManager::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  auto pass = graph.AddPass<UploadPassData>(
    "Geometry Metadata Upload", xiiGALCommandQueueFlags::Transfer,
    [this](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hGeometryBuffer = builder.ImportBuffer("GPU Geometry Metadata", m_pMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometryBuffer = builder.WriteBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.ImportBuffer("GPU Meshlet Metadata", m_pMeshletMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.WriteBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      for (const Upload& upload : data.m_Uploads)
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hGeometryBuffer), upload.m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&upload.m_Record), sizeof(upload.m_Record)));
      for (const UploadPassData::MeshletUpload& upload : data.m_MeshletUploads)
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hMeshletBuffer), upload.m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(upload.m_Meshlets.GetData()), upload.m_Meshlets.GetCount() * sizeof(xiiMeshlet)));
    }, true);

  m_uiLastUploadedBytes = 0U;
  const xiiUInt32 uiFrameSlice = static_cast<xiiUInt32>(uiFrameIndex % m_uiFramesInFlight);
  const xiiUInt64 uiFrameBit = xiiUInt64(1) << uiFrameSlice;
  for (xiiUInt32 i = 0; i < m_Slots.GetCount(); ++i)
  {
    Slot& slot = m_Slots[i];
    if (!slot.m_bAllocated || (slot.m_uiDirtyFrameMask & uiFrameBit) == 0U) continue;
    Upload& upload = pass.first->m_Uploads.ExpandAndGetRef();
    upload.m_uiOffset = (uiFrameSlice * m_Slots.GetCount() + i) * sizeof(xiiGpuGeometryRecord);
    upload.m_Record = slot.m_GpuRecord;
    slot.m_uiDirtyFrameMask &= ~uiFrameBit;
    m_uiLastUploadedBytes += sizeof(xiiGpuGeometryRecord);
  }
  pass.first->m_MeshletUploads = std::move(m_PendingMeshletUploads);
  m_PendingMeshletUploads.Clear();
  for (const UploadPassData::MeshletUpload& upload : pass.first->m_MeshletUploads)
    m_uiLastUploadedBytes += upload.m_Meshlets.GetCount() * sizeof(xiiMeshlet);

  UploadHandles result;
  result.m_hGeometryMetadata = pass.first->m_hGeometryBuffer;
  result.m_hMeshletMetadata  = pass.first->m_hMeshletBuffer;
  result.m_uiGeometryBaseIndex = uiFrameSlice * m_Slots.GetCount();
  return result;
}

bool xiiGeometryResidencyManager::AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset)
{
  for (xiiUInt32 i = 0; i < m_FreeMeshletRanges.GetCount(); ++i)
  {
    FreeRange& range = m_FreeMeshletRanges[i];
    if (range.m_uiCount < uiCount) continue;
    out_uiOffset = range.m_uiOffset;
    range.m_uiOffset += uiCount;
    range.m_uiCount -= uiCount;
    if (range.m_uiCount == 0U) m_FreeMeshletRanges.RemoveAtAndCopy(i);
    return true;
  }
  return false;
}

void xiiGeometryResidencyManager::FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount)
{
  if (uiCount == 0U) return;
  xiiUInt32 uiInsert = 0U;
  while (uiInsert < m_FreeMeshletRanges.GetCount() && m_FreeMeshletRanges[uiInsert].m_uiOffset < uiOffset) ++uiInsert;
  m_FreeMeshletRanges.InsertAt(uiInsert, {uiOffset, uiCount});
  if (uiInsert > 0U)
  {
    FreeRange& prev = m_FreeMeshletRanges[uiInsert - 1U];
    if (prev.m_uiOffset + prev.m_uiCount == m_FreeMeshletRanges[uiInsert].m_uiOffset)
    {
      prev.m_uiCount += m_FreeMeshletRanges[uiInsert].m_uiCount;
      m_FreeMeshletRanges.RemoveAtAndCopy(uiInsert);
      --uiInsert;
    }
  }
  if (uiInsert + 1U < m_FreeMeshletRanges.GetCount())
  {
    FreeRange& current = m_FreeMeshletRanges[uiInsert];
    const FreeRange& next = m_FreeMeshletRanges[uiInsert + 1U];
    if (current.m_uiOffset + current.m_uiCount == next.m_uiOffset)
    {
      current.m_uiCount += next.m_uiCount;
      m_FreeMeshletRanges.RemoveAtAndCopy(uiInsert + 1U);
    }
  }
}

void xiiGeometryResidencyManager::ReleaseMeshletAllocations(Slot& slot)
{
  for (xiiUInt32 i = 0; i < xiiGpuGeometryRecord::s_uiMaxLods; ++i)
  {
    FreeMeshlets(slot.m_uiMeshletArenaOffset[i], slot.m_uiMeshletArenaCount[i]);
    slot.m_uiMeshletArenaOffset[i] = 0U;
    slot.m_uiMeshletArenaCount[i] = 0U;
    slot.m_GpuRecord.m_Lods[i].m_uiMeshletMetadataOffset = 0U;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Geometry_Implementation_GeometryResidency);
