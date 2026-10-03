/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>

namespace
{
  template <typename T>
  bool TryGetMaterialParameter(const xiiMaterialInstance& material, xiiStringView sName, T& out_value)
  {
    const xiiVariant value = material.GetParameter(xiiMaterialParameterId::Make(sName));
    if (!value.IsValid() || !value.CanConvertTo<T>())
      return false;

    out_value = value.ConvertTo<T>();
    return true;
  }

  void ResolveMaterialColor(const xiiMaterialInstance& material, xiiStringView sName, xiiVec4& inout_value, bool bPreserveAlpha)
  {
    const float fPreviousAlpha = inout_value.w;
    xiiColor    color;
    if (TryGetMaterialParameter(material, sName, color))
    {
      inout_value = xiiVec4(color.r, color.g, color.b, color.a);
      if (bPreserveAlpha)
        inout_value.w = fPreviousAlpha;
      return;
    }

    xiiVec4 vector4;
    if (TryGetMaterialParameter(material, sName, vector4))
    {
      inout_value = vector4;
      if (bPreserveAlpha)
        inout_value.w = fPreviousAlpha;
      return;
    }

    xiiVec3 vector3;
    if (TryGetMaterialParameter(material, sName, vector3))
      inout_value = xiiVec4(vector3.x, vector3.y, vector3.z, inout_value.w);
  }

  xiiUInt32 ResolveMaterialTexture(const xiiMaterialInstanceSnapshot& snapshot, xiiStringView sName)
  {
    if (snapshot.m_pSchema == nullptr)
      return xiiInvalidIndex;

    const xiiUInt32 uiTextureIndex = snapshot.m_pSchema->FindTextureIndex(xiiMaterialParameterId::Make(sName));
    return uiTextureIndex != xiiInvalidIndex && uiTextureIndex < snapshot.m_ResourceBindings.GetCount()
             ? snapshot.m_ResourceBindings[uiTextureIndex].m_uiBindlessIndex
             : xiiInvalidIndex;
  }

  xiiGpuSurfaceMaterial ResolveSurfaceMaterial(const xiiMaterialInstance& material, const xiiMaterialInstanceSnapshot& snapshot)
  {
    xiiGpuSurfaceMaterial result;
    result.BaseColorOpacity          = xiiVec4(1.0f);
    result.EmissiveColorAndRoughness = xiiVec4(0.0f, 0.0f, 0.0f, 0.5f);
    result.SurfaceParameters         = xiiVec4(0.0f, 0.5f, 0.0f, 1.0f);
    result.LayerParameters           = xiiVec4(0.5f, 1.0f, 0.0f, 0.0f);
    result.TextureIndices0           = xiiVec4U32(xiiInvalidIndex);
    result.TextureIndices1           = xiiVec4U32(xiiInvalidIndex);
    result.Metadata                  = xiiVec4U32(static_cast<xiiUInt32>(xiiMaterialShadingModel::Lit), 0U,
                                                 static_cast<xiiUInt32>(xiiMaterialAlphaMode::Opaque), static_cast<xiiUInt32>(xiiMaterialBlendMode::Opaque));
    result.AdvancedParameters        = xiiVec4(0.0f, 1.5f, 0.0f, 0.5f);

    ResolveMaterialColor(material, "BaseColor", result.BaseColorOpacity, false);
    ResolveMaterialColor(material, "EmissiveColor", result.EmissiveColorAndRoughness, true);
    TryGetMaterialParameter(material, "Roughness", result.EmissiveColorAndRoughness.w);
    TryGetMaterialParameter(material, "Metallic", result.SurfaceParameters.x);
    TryGetMaterialParameter(material, "Specular", result.SurfaceParameters.y);
    TryGetMaterialParameter(material, "Transmission", result.SurfaceParameters.z);
    TryGetMaterialParameter(material, "OcclusionStrength", result.SurfaceParameters.w);
    TryGetMaterialParameter(material, "AlphaCutoff", result.LayerParameters.x);
    TryGetMaterialParameter(material, "NormalScale", result.LayerParameters.y);
    TryGetMaterialParameter(material, "ClearCoat", result.LayerParameters.z);
    TryGetMaterialParameter(material, "ClearCoatRoughness", result.LayerParameters.w);
    TryGetMaterialParameter(material, "Thickness", result.AdvancedParameters.x);
    TryGetMaterialParameter(material, "IndexOfRefraction", result.AdvancedParameters.y);
    TryGetMaterialParameter(material, "Anisotropy", result.AdvancedParameters.z);
    TryGetMaterialParameter(material, "SheenRoughness", result.AdvancedParameters.w);

    const xiiMaterialRuntimeState& runtimeState = snapshot.m_RuntimeState;
    result.Metadata = xiiVec4U32(
      runtimeState.m_ShadingModel.GetValue(), runtimeState.m_FeatureFlags.GetValue(),
      static_cast<xiiUInt32>(runtimeState.IsMasked() ? xiiMaterialAlphaMode::Mask : runtimeState.m_AlphaMode.GetValue()),
      runtimeState.m_BlendMode.GetValue());
    result.TextureIndices0 = xiiVec4U32(
      ResolveMaterialTexture(snapshot, "BaseColorTexture"), ResolveMaterialTexture(snapshot, "NormalTexture"),
      ResolveMaterialTexture(snapshot, "MetallicRoughnessTexture"), ResolveMaterialTexture(snapshot, "OcclusionTexture"));
    result.TextureIndices1 = xiiVec4U32(
      ResolveMaterialTexture(snapshot, "EmissiveTexture"), ResolveMaterialTexture(snapshot, "HeightTexture"),
      ResolveMaterialTexture(snapshot, "ClearCoatTexture"), ResolveMaterialTexture(snapshot, "TransmissionTexture"));
    return result;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSurfaceMaterial, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuSurfaceMaterial>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BaseColorOpacity", BaseColorOpacity),
    XII_MEMBER_PROPERTY("EmissiveColorAndRoughness", EmissiveColorAndRoughness),
    XII_MEMBER_PROPERTY("SurfaceParameters", SurfaceParameters),
    XII_MEMBER_PROPERTY("LayerParameters", LayerParameters),
    XII_MEMBER_PROPERTY("TextureIndices0", TextureIndices0),
    XII_MEMBER_PROPERTY("TextureIndices1", TextureIndices1),
    XII_MEMBER_PROPERTY("Metadata", Metadata),
    XII_MEMBER_PROPERTY("AdvancedParameters", AdvancedParameters),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

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
  m_pBuffer                               = pDevice->CreateBuffer(bufferDescription);
  if (m_pBuffer == nullptr)
  {
    Shutdown();
    return XII_FAILURE;
  }

  m_pBuffer->SetDebugName("Material GPU Storage");

  bufferDescription.m_uiSize              = static_cast<xiiUInt64>(sizeof(xiiGpuSurfaceMaterial)) * description.m_uiMaxMaterials * description.m_uiFramesInFlight;
  bufferDescription.m_uiElementByteStride = sizeof(xiiGpuSurfaceMaterial);
  m_pSurfaceBuffer                        = pDevice->CreateBuffer(bufferDescription);
  if (m_pSurfaceBuffer == nullptr)
  {
    Shutdown();
    return XII_FAILURE;
  }
  m_pSurfaceBuffer->SetDebugName("Surface Material GPU Storage");

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
  m_pSurfaceBuffer.Clear();
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
  Slot& slot       = m_Slots[uiSlot];
  slot.m_pInstance = std::move(pInstance);
  xiiMemoryUtils::ZeroFill(slot.m_LastUploadedRevision.GetData(), slot.m_LastUploadedRevision.GetCount());
  ++m_uiActiveCount;

  xiiMaterialGpuHandle handle;
  handle.m_uiSlot       = uiSlot;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

bool xiiMaterialGpuStorage::ReplaceMaterial(xiiMaterialGpuHandle handle, xiiSharedPtr<xiiMaterialInstance> pInstance)
{
  if (pInstance == nullptr)
    return false;

  const xiiSharedPtr<const xiiMaterialSchema> pSchema = pInstance->GetSchema();
  XII_LOCK(m_Mutex);
  if (!IsValidHandleLocked(handle) || pSchema == nullptr || pSchema->GetParameterBlockSize() > m_Description.m_uiMaxParameterBytes)
    return false;

  Slot& slot = m_Slots[handle.m_uiSlot];
  if (slot.m_pInstance == pInstance)
    return true;

  slot.m_pInstance = std::move(pInstance);
  xiiMemoryUtils::ZeroFill(slot.m_LastUploadedRevision.GetData(), slot.m_LastUploadedRevision.GetCount());
  return true;
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

  RetiredSlot& retired     = m_RetiredSlots.ExpandAndGetRef();
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

xiiUInt32 xiiMaterialGpuStorage::GetSurfaceBaseIndex(xiiUInt64 uiFrameIndex) const
{
  XII_LOCK(m_Mutex);
  return m_Description.m_uiFramesInFlight > 0U ? static_cast<xiiUInt32>(uiFrameIndex % m_Description.m_uiFramesInFlight) * m_Description.m_uiMaxMaterials : 0U;
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

void xiiMaterialGpuStorage::GetActiveMaterials(xiiDynamicArray<xiiSharedPtr<xiiMaterialInstance>>& out_materials) const
{
  XII_LOCK(m_Mutex);
  out_materials.Clear();
  out_materials.Reserve(m_uiActiveCount);

  for (const Slot& slot : m_Slots)
  {
    if (slot.m_pInstance != nullptr)
      out_materials.PushBack(slot.m_pInstance);
  }
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

    xiiMaterialGpuUpload& upload   = out_batch.m_Uploads.ExpandAndGetRef();
    upload.m_Handle.m_uiSlot       = i;
    upload.m_Handle.m_uiGeneration = slot.m_uiGeneration;
    upload.m_uiRevision            = snapshot.m_uiRevision;
    upload.m_uiDestinationOffset   = static_cast<xiiUInt32>((static_cast<xiiUInt64>(uiFrameSlice) * m_Description.m_uiMaxMaterials + i) * m_uiMaterialStride);
    upload.m_uiSurfaceDestinationOffset = static_cast<xiiUInt32>((static_cast<xiiUInt64>(uiFrameSlice) * m_Description.m_uiMaxMaterials + i) * sizeof(xiiGpuSurfaceMaterial));
    upload.m_Data.SetCount(m_uiMaterialStride);
    xiiMemoryUtils::ZeroFill(upload.m_Data.GetData(), upload.m_Data.GetCount());
    xiiMemoryUtils::Copy(upload.m_Data.GetData(), snapshot.m_ParameterData.GetData(), snapshot.m_ParameterData.GetCount());
    upload.m_SurfaceData = ResolveSurfaceMaterial(*slot.m_pInstance, snapshot);
  }
}

xiiMaterialGpuStorage::UploadHandles xiiMaterialGpuStorage::AddUploadPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  xiiMaterialGpuUploadBatch batch;
  GatherUploads(uiFrameIndex, batch);

  auto pass = graph.AddPass<UploadPassData>(
    "Material Upload",
    xiiGALCommandQueueFlags::Transfer,
    [this](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hBuffer = builder.ImportBuffer("Material GPU Storage", m_pBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hBuffer = builder.WriteBuffer(data.m_hBuffer, xiiGALResourceStateFlags::CopyDestination);
      data.m_hSurfaceBuffer = builder.ImportBuffer("Surface Material GPU Storage", m_pSurfaceBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hSurfaceBuffer = builder.WriteBuffer(data.m_hSurfaceBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(data.m_hSurfaceBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALBuffer* pBuffer = context.GetBuffer(data.m_hBuffer);
      xiiGALBuffer* pSurfaceBuffer = context.GetBuffer(data.m_hSurfaceBuffer);
      for (const xiiMaterialGpuUpload& upload : data.m_Batch.m_Uploads)
      {
        context.GetCommandList().UpdateBuffer(pBuffer, upload.m_uiDestinationOffset, upload.m_Data);
        context.GetCommandList().UpdateBuffer(
          pSurfaceBuffer, upload.m_uiSurfaceDestinationOffset,
          xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&upload.m_SurfaceData), sizeof(upload.m_SurfaceData)));
      }

      data.m_pStorage->MarkUploadsRecorded(data.m_Batch);
    },
    true);

  pass.first->m_Batch    = std::move(batch);
  pass.first->m_pStorage = this;
  UploadHandles result;
  result.m_hParameterData      = pass.first->m_hBuffer;
  result.m_hSurfaceData        = pass.first->m_hSurfaceBuffer;
  result.m_uiSurfaceBaseIndex  = GetSurfaceBaseIndex(uiFrameIndex);
  return result;
}

xiiRenderGraphBufferHandle xiiMaterialGpuStorage::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  return AddUploadPasses(graph, uiFrameIndex).m_hParameterData;
}

void xiiMaterialGpuStorage::MarkUploadsRecorded(const xiiMaterialGpuUploadBatch& batch)
{
  XII_LOCK(m_Mutex);
  if (m_Description.m_uiFramesInFlight == 0U)
    return;

  const xiiUInt32 uiFrameSlice = static_cast<xiiUInt32>(batch.m_uiFrameIndex % m_Description.m_uiFramesInFlight);
  m_uiLastUploadCount          = batch.m_Uploads.GetCount();
  m_uiLastUploadBytes          = 0ULL;
  for (const xiiMaterialGpuUpload& upload : batch.m_Uploads)
  {
    if (!IsValidHandleLocked(upload.m_Handle))
      continue;

    m_Slots[upload.m_Handle.m_uiSlot].m_LastUploadedRevision[uiFrameSlice] = upload.m_uiRevision;
    m_uiLastUploadBytes += upload.m_Data.GetCount() + sizeof(xiiGpuSurfaceMaterial);
  }
}

bool xiiMaterialGpuStorage::IsValidHandleLocked(xiiMaterialGpuHandle handle) const
{
  return handle.IsValid() && handle.m_uiSlot < m_Slots.GetCount() && m_Slots[handle.m_uiSlot].m_uiGeneration == handle.m_uiGeneration && m_Slots[handle.m_uiSlot].m_pInstance != nullptr;
}
