#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

constexpr xiiUInt32 s_uiSkinningBufferIndex = 2;

XII_IMPLEMENT_WORLD_MODULE(xiiRenderDataManager);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderDataManager, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderDataManager::xiiRenderDataManager(xiiWorld* pWorld)
  : xiiWorldModule(pWorld)
{
  xiiRenderWorld::GetExtractionEvent().AddEventHandler(xiiMakeDelegate(&xiiRenderDataManager::OnExtractionEvent, this));

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(xiiPerInstanceData);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable
  desc.m_BufferFlags = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::ShaderResource;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Static Instance Data"));
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Dynamic Instance Data"));

  // Skinning buffer
  desc.m_uiStructSize = sizeof(xiiShaderTransform);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable

  XII_ASSERT_DEBUG(m_Buffers.GetCount() == s_uiSkinningBufferIndex, "Unexpected buffer index");
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Skinning Data"));
}

xiiRenderDataManager::~xiiRenderDataManager()
{
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(xiiMakeDelegate(&xiiRenderDataManager::OnExtractionEvent, this));

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  for (auto& hBuffer : m_Buffers)
  {
    pDevice->DestroyDynamicBuffer(hBuffer);
  }
}

void xiiRenderDataManager::Initialize()
{
  {
    auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderDataManager::CompactSkinningDataBuffer, this);
    desc.m_Phase = xiiWorldUpdatePhase::PostTransform;
    desc.m_fPriority = -1000.0f;

    RegisterUpdateFunction(desc);
  }
}

xiiArrayPtr<xiiPerInstanceData> xiiRenderDataManager::GetOrCreateInstanceData(const xiiComponent* pOwnerComponent, bool bDynamic, xiiGALDynamicBufferHandle& out_hBuffer, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount /*= 1*/) const
{
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiBufferIndex = bDynamic ? 1 : 0;

  if (inout_instanceDataOffset.IsInvalidated() == false && inout_instanceDataOffset.m_uiIsDynamic != uiBufferIndex)
  {
    // The instance data was allocated in a different buffer, need to re-allocate.
    auto pOldInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[inout_instanceDataOffset.m_uiIsDynamic]);
    pOldInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }

  out_hBuffer = m_Buffers[uiBufferIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiBufferIndex ? m_ExtractionData.m_pBuffers[uiBufferIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  if (inout_instanceDataOffset.IsInvalidated())
  {
    xiiComponentHandle hOwnerComponent = pOwnerComponent != nullptr ? pOwnerComponent->GetHandle() : xiiComponentHandle();
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(hOwnerComponent, uiCount, xiiGALDynamicBuffer::AllocateFlags::None, xiiFrameAllocator::GetCurrentAllocator());
    inout_instanceDataOffset.m_uiIsDynamic = uiBufferIndex;
  }

  return pInstanceDataBuffer->MapForWriting<xiiPerInstanceData>(inout_instanceDataOffset.m_uiOffset);
}

void xiiRenderDataManager::DeleteInstanceData(xiiInstanceDataOffset& inout_instanceDataOffset) const
{
  XII_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    const xiiUInt32 uiBufferIndex = inout_instanceDataOffset.m_uiIsDynamic;

    auto pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiBufferIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

xiiUInt32 xiiRenderDataManager::RegisterCustomInstanceData(const xiiGALBufferCreationDescription& desc, xiiStringView sDebugName, xiiDelegate<void()> beforeUploadCallback /*= {}*/)
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
  {
    auto pBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[i]);
    if (pBuffer->GetDescription() == desc && pBuffer->GetDebugName() == sDebugName)
    {
      return i;
    }
  }

  xiiUInt32 uiBufferIndex = m_Buffers.GetCount();

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, sDebugName));

  if (beforeUploadCallback.IsValid())
  {
    m_BeforeUploadCallbacks.EnsureCount(uiBufferIndex + 1);
    m_BeforeUploadCallbacks[uiBufferIndex] = beforeUploadCallback;
  }

  return uiBufferIndex;
}

xiiByteArrayPtr xiiRenderDataManager::GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiStructByteSize, const xiiComponent* pOwnerComponent, xiiGALDynamicBufferHandle& out_hBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount) const
{
  XII_LOCK(m_Mutex);

  out_hBuffer = m_Buffers[uiCustomDataIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiCustomDataIndex ? m_ExtractionData.m_pBuffers[uiCustomDataIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  XII_ASSERT_DEV(pInstanceDataBuffer->GetDescription().m_uiStructSize == uiStructByteSize, "Requested struct size {} does not match the registered size {}.", uiStructByteSize, pInstanceDataBuffer->GetDescription().m_uiStructSize);

  if (inout_instanceDataOffset.IsInvalidated())
  {
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(pOwnerComponent->GetHandle(), uiCount, xiiGALDynamicBuffer::AllocateFlags::None, xiiFrameAllocator::GetCurrentAllocator());
  }

  return pInstanceDataBuffer->MapBytesForWriting(inout_instanceDataOffset.m_uiOffset);
}

void xiiRenderDataManager::DeleteCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  XII_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    auto pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

void xiiRenderDataManager::CompactCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiMaxSteps)
{
  XII_LOCK(m_Mutex);

  auto pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

  xiiTempHybridArray<xiiGALDynamicBuffer::ChangedAllocation, 16> changedAllocations;
  pInstanceDataBuffer->RunCompactionSteps(changedAllocations, uiMaxSteps);

  for (const auto& changedAllocation : changedAllocations)
  {
    xiiComponentHandle hComponent(xiiComponentId(changedAllocation.m_uiUserData));
    xiiComponent* pComponent = nullptr;
    XII_VERIFY(GetWorld()->TryGetComponent(hComponent, pComponent), "Invalid component handle");

    xiiMsgCustomInstanceDataOffsetChanged msg;
    msg.m_NewOffset.m_uiOffset = changedAllocation.m_uiNewOffset;
    XII_VERIFY(pComponent->SendMessage(msg), "Component of type '{}' did not handle xiiMsgCustomInstanceDataOffsetChanged.", pComponent->GetDynamicRTTI()->GetTypeName());
  }
}

xiiArrayPtr<xiiShaderTransform> xiiRenderDataManager::GetOrCreateSkinningData(const xiiComponent* pOwnerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiNumTransforms) const
{
  xiiGALDynamicBufferHandle hDummy;
  return GetOrCreateCustomInstanceData<xiiShaderTransform>(s_uiSkinningBufferIndex, pOwnerComponent, hDummy, inout_instanceDataOffset, uiNumTransforms);
}

xiiArrayPtr<const xiiShaderTransform> xiiRenderDataManager::GetSkinningData(const xiiCustomInstanceDataOffset& instanceDataOffset) const
{
  XII_LOCK(m_Mutex);

  auto pInstanceDataBuffer = xiiGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[s_uiSkinningBufferIndex]);

  return pInstanceDataBuffer->MapForReading<xiiShaderTransform>(instanceDataOffset.m_uiOffset);
}

void xiiRenderDataManager::DeleteSkinningData(xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  DeleteCustomInstanceData(s_uiSkinningBufferIndex, inout_instanceDataOffset);
}

xiiGALDynamicBufferHandle xiiRenderDataManager::GetSkinningDataBuffer() const
{
  return GetCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void xiiRenderDataManager::CompactSkinningDataBuffer(const UpdateContext& context)
{
  CompactCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void xiiRenderDataManager::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (e.m_Type == xiiRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    m_ExtractionData.m_pBuffers.SetCount(m_Buffers.GetCount());

    for (xiiUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      m_ExtractionData.m_pBuffers[i] = pDevice->GetDynamicBuffer(m_Buffers[i]);
    }
  }
  else if (e.m_Type == xiiRenderWorldExtractionEvent::Type::EndExtraction)
  {
    for (xiiUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      if (m_BeforeUploadCallbacks.GetCount() > i && m_BeforeUploadCallbacks[i].IsValid())
      {
        m_BeforeUploadCallbacks[i]();
      }

      m_ExtractionData.m_pBuffers[i]->UploadChangesForNextFrame();
    }

    m_ExtractionData.m_pBuffers.Clear();
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderDataManager);
