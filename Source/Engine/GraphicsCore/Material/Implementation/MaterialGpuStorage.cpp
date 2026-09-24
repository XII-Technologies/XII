/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialGpuStorageDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialGpuStorageDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxMaterials", m_uiMaxMaterials),
    XII_MEMBER_PROPERTY("MaxParameterBytes", m_uiMaxParameterBytes),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialGpuStorageStatistics, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialGpuStorageStatistics>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Capacity", m_uiCapacity),
    XII_MEMBER_PROPERTY("ActiveMaterials", m_uiActiveMaterials),
    XII_MEMBER_PROPERTY("RetiredMaterials", m_uiRetiredMaterials),
    XII_MEMBER_PROPERTY("LastUploadCount", m_uiLastUploadCount),
    XII_MEMBER_PROPERTY("LastUploadBytes", m_uiLastUploadBytes),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiMaterialGpuStorage::~xiiMaterialGpuStorage()
{
  Shutdown();
}

xiiResult xiiMaterialGpuStorage::Initialize(xiiGALDevice* pDevice, const xiiMaterialGpuStorageDescription& description)
{
  Shutdown();

  if (pDevice == nullptr || description.m_uiMaxMaterials == 0U || description.m_uiMaxParameterBytes == 0U || description.m_uiFramesInFlight == 0U)
    return XII_FAILURE;

  m_Description      = description;
  m_uiMaterialStride = xiiMemoryUtils::AlignSize(description.m_uiMaxParameterBytes, 16U);

  const xiiUInt64 uiBufferSize = static_cast<xiiUInt64>(m_uiMaterialStride) * description.m_uiMaxMaterials * description.m_uiFramesInFlight;
  if (uiBufferSize > xiiMath::MaxValue<xiiUInt32>())
  {
    Shutdown();
    return XII_FAILURE;
  }

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiSize              = uiBufferSize;
  bufferDescription.m_uiElementByteStride = m_uiMaterialStride;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
  m_pBuffer                                = pDevice->CreateBuffer(bufferDescription);
  if (m_pBuffer == nullptr)
  {
    Shutdown();
    return XII_FAILURE;
  }

  m_pBuffer->SetDebugName("Material GPU Storage");
  m_Slots.SetCount(description.m_uiMaxMaterials);
  m_FreeSlots.Reserve(description.m_uiMaxMaterials);
  for (xiiUInt32 i = description.m_uiMaxMaterials; i > 0U; --i)
  {
    Slot& slot = m_Slots[i - 1U];
    slot.m_LastUploadedRevision.SetCount(description.m_uiFramesInFlight);
    xiiMemoryUtils::ZeroFill(slot.m_LastUploadedRevision.GetData(), slot.m_LastUploadedRevision.GetCount());
    m_FreeSlots.PushBack(i - 1U);
  }

  return XII_SUCCESS;
}

void xiiMaterialGpuStorage::Shutdown()
{
  XII_LOCK(m_Mutex);
  m_pBuffer.Clear();
  m_Slots.Clear();
  m_FreeSlots.Clear();
  m_RetiredSlots.Clear();
  m_Description       = {};
  m_uiMaterialStride  = 0U;
  m_uiActiveCount     = 0U;
  m_uiLastUploadCount = 0U;
  m_uiLastUploadBytes = 0ULL;
}

xiiMaterialGpuHandle xiiMaterialGpuStorage::RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance)
{
  if (pInstance == nullptr)
    return {};

  const xiiSharedPtr<const xiiMaterialSchema> pSchema = pInstance->GetSchema();
  XII_LOCK(m_Mutex);
  if (pSchema == nullptr || pSchema->GetParameterBlockSize() > m_Description.m_uiMaxParameterBytes || m_pBuffer == nullptr || m_FreeSlots.IsEmpty())
    return {};

  const xiiUInt32 uiSlot = m_FreeSlots.PeekBack();
  m_FreeSlots.PopBack();
  Slot& slot      = m_Slots[uiSlot];
  slot.m_pInstance = std::move(pInstance);
  xiiMemoryUtils::ZeroFill(slot.m_LastUploadedRevision.GetData(), slot.m_LastUploadedRevision.GetCount());
  ++m_uiActiveCount;

  xiiMaterialGpuHandle handle;
  handle.m_uiSlot       = uiSlot;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiMaterialGpuStorage::UnregisterMaterial(xiiMaterialGpuHandle handle, xiiUInt64 uiFrameIndex)
{
  XII_LOCK(m_Mutex);
  if (!IsValidHandleLocked(handle))
    return;

  Slot& slot = m_Slots[handle.m_uiSlot];
  slot.m_pInstance.Clear();
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  --m_uiActiveCount;

  RetiredSlot& retired = m_RetiredSlots.ExpandAndGetRef();
  retired.m_uiSlot         = handle.m_uiSlot;
  retired.m_uiLastUseFrame = uiFrameIndex;
}

void xiiMaterialGpuStorage::CollectGarbage(xiiUInt64 uiCompletedFrame)
{
  XII_LOCK(m_Mutex);
  for (xiiUInt32 i = m_RetiredSlots.GetCount(); i > 0U; --i)
  {
    if (m_RetiredSlots[i - 1U].m_uiLastUseFrame > uiCompletedFrame)
      continue;

    m_FreeSlots.PushBack(m_RetiredSlots[i - 1U].m_uiSlot);
    m_RetiredSlots.RemoveAtAndSwap(i - 1U);
  }
}

bool xiiMaterialGpuStorage::IsValidHandle(xiiMaterialGpuHandle handle) const
{
  XII_LOCK(m_Mutex);
  return IsValidHandleLocked(handle);
}

xiiSharedPtr<xiiMaterialInstance> xiiMaterialGpuStorage::GetMaterial(xiiMaterialGpuHandle handle) const
{
  XII_LOCK(m_Mutex);
  return IsValidHandleLocked(handle) ? m_Slots[handle.m_uiSlot].m_pInstance : nullptr;
}

xiiUInt32 xiiMaterialGpuStorage::GetGpuOffset(xiiMaterialGpuHandle handle, xiiUInt64 uiFrameIndex) const
{
  XII_LOCK(m_Mutex);
  if (!IsValidHandleLocked(handle))
    return xiiInvalidIndex;

  const xiiUInt64 uiFrameSlice = uiFrameIndex % m_Description.m_uiFramesInFlight;
  return static_cast<xiiUInt32>((uiFrameSlice * m_Description.m_uiMaxMaterials + handle.m_uiSlot) * m_uiMaterialStride);
}

xiiMaterialGpuStorageStatistics xiiMaterialGpuStorage::GetStatistics() const
{
  XII_LOCK(m_Mutex);
  xiiMaterialGpuStorageStatistics result;
  result.m_uiCapacity         = m_Slots.GetCount();
  result.m_uiActiveMaterials  = m_uiActiveCount;
  result.m_uiRetiredMaterials = m_RetiredSlots.GetCount();
  result.m_uiLastUploadCount  = m_uiLastUploadCount;
  result.m_uiLastUploadBytes  = m_uiLastUploadBytes;
  return result;
}

void xiiMaterialGpuStorage::GatherUploads(xiiUInt64 uiFrameIndex, xiiMaterialGpuUploadBatch& out_batch) const
{
  XII_LOCK(m_Mutex);
  out_batch.m_uiFrameIndex = uiFrameIndex;
  out_batch.m_Uploads.Clear();
  if (m_pBuffer == nullptr)
    return;

  const xiiUInt32 uiFrameSlice = static_cast<xiiUInt32>(uiFrameIndex % m_Description.m_uiFramesInFlight);
  for (xiiUInt32 i = 0U; i < m_Slots.GetCount(); ++i)
  {
    const Slot& slot = m_Slots[i];
    if (slot.m_pInstance == nullptr)
      continue;

    xiiMaterialInstanceSnapshot snapshot;
    slot.m_pInstance->CreateSnapshot(snapshot);
    if (slot.m_LastUploadedRevision[uiFrameSlice] == snapshot.m_uiRevision)
      continue;

    xiiMaterialGpuUpload& upload = out_batch.m_Uploads.ExpandAndGetRef();
    upload.m_Handle.m_uiSlot       = i;
    upload.m_Handle.m_uiGeneration = slot.m_uiGeneration;
    upload.m_uiRevision            = snapshot.m_uiRevision;
    upload.m_uiDestinationOffset   = static_cast<xiiUInt32>((static_cast<xiiUInt64>(uiFrameSlice) * m_Description.m_uiMaxMaterials + i) * m_uiMaterialStride);
    upload.m_Data.SetCount(m_uiMaterialStride);
    xiiMemoryUtils::ZeroFill(upload.m_Data.GetData(), upload.m_Data.GetCount());
    xiiMemoryUtils::Copy(upload.m_Data.GetData(), snapshot.m_ParameterData.GetData(), snapshot.m_ParameterData.GetCount());
  }
}

xiiRenderGraphBufferHandle xiiMaterialGpuStorage::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  xiiMaterialGpuUploadBatch batch;
  GatherUploads(uiFrameIndex, batch);

  auto pass = graph.AddPass<UploadPassData>(
    "Material Upload",
    xiiGALCommandQueueFlags::Transfer,
    [this](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hBuffer = builder.ImportBuffer("Material GPU Storage", m_pBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hBuffer = builder.WriteBuffer(data.m_hBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALBuffer* pBuffer = context.GetBuffer(data.m_hBuffer);
      for (const xiiMaterialGpuUpload& upload : data.m_Batch.m_Uploads)
        context.GetCommandList().UpdateBuffer(pBuffer, upload.m_uiDestinationOffset, upload.m_Data);

      data.m_pStorage->MarkUploadsRecorded(data.m_Batch);
    },
    true);

  pass.first->m_Batch    = std::move(batch);
  pass.first->m_pStorage = this;
  return pass.first->m_hBuffer;
}

void xiiMaterialGpuStorage::MarkUploadsRecorded(const xiiMaterialGpuUploadBatch& batch)
{
  XII_LOCK(m_Mutex);
  if (m_Description.m_uiFramesInFlight == 0U)
    return;

  const xiiUInt32 uiFrameSlice = static_cast<xiiUInt32>(batch.m_uiFrameIndex % m_Description.m_uiFramesInFlight);
  m_uiLastUploadCount = batch.m_Uploads.GetCount();
  m_uiLastUploadBytes = 0ULL;
  for (const xiiMaterialGpuUpload& upload : batch.m_Uploads)
  {
    if (!IsValidHandleLocked(upload.m_Handle))
      continue;

    m_Slots[upload.m_Handle.m_uiSlot].m_LastUploadedRevision[uiFrameSlice] = upload.m_uiRevision;
    m_uiLastUploadBytes += upload.m_Data.GetCount();
  }
}

bool xiiMaterialGpuStorage::IsValidHandleLocked(xiiMaterialGpuHandle handle) const
{
  return handle.IsValid() && handle.m_uiSlot < m_Slots.GetCount() && m_Slots[handle.m_uiSlot].m_uiGeneration == handle.m_uiGeneration && m_Slots[handle.m_uiSlot].m_pInstance != nullptr;
}
