#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/DynamicResolutionDecisionPass.h>
#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>
#include <GraphicsCore/Pipeline/Passes/RayTracedShadowsPass.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

constexpr xiiUInt32 s_uiSkinningBufferIndex = 2;

namespace
{
  struct GpuDrivenDispatchArguments
  {
    xiiUInt32 m_uiThreadGroupCountX = 0U;
    xiiUInt32 m_uiThreadGroupCountY = 1U;
    xiiUInt32 m_uiThreadGroupCountZ = 1U;
  };
} // namespace

XII_IMPLEMENT_WORLD_MODULE(xiiRenderDataManager);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderDataManager, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgCustomInstanceDataOffsetChanged, 1, xiiRTTIDefaultAllocator<xiiMsgCustomInstanceDataOffsetChanged>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderDataManager::xiiRenderDataManager(xiiWorld* pWorld) : xiiWorldModule(pWorld)
{
  xiiRenderWorld::GetExtractionEvent().AddEventHandler(xiiMakeDelegate(&xiiRenderDataManager::OnExtractionEvent, this));

  m_pGpuDrivenVisibilityPass = XII_DEFAULT_NEW(xiiRenderGraphGpuVisibilityPass);
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupGpuDrivenVisibilityCommandList, this));
  m_pGpuDrivenVisibilityPass->SetPostDispatchCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::OnGpuDrivenVisibilityPostDispatch, this));
  m_bGpuVisibilityUseInternalIndirectDispatch = true;

  m_pRayTracedShadowsPass = XII_DEFAULT_NEW(xiiRenderGraphRayTracedShadowsPass);
  m_pRayTracedShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupRayTracedShadowsCommandList, this));
  m_pRayTracedShadowsPass->SetDispatchRayTracingFunc(xiiMakeDelegate(&xiiRenderDataManager::DispatchRayTracedShadowsCommandList, this));

  m_pDynamicResolutionDecisionPass = XII_DEFAULT_NEW(xiiRenderGraphDynamicResolutionDecisionPass);
  m_pDynamicResolutionDecisionPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDynamicResolutionDecisionCommandList, this));
  m_pDynamicResolutionDecisionPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  // Keep indices stable for callers that expect static/dynamic/skinning slots.
  m_Buffers.SetCount(3);
}

xiiRenderDataManager::~xiiRenderDataManager()
{
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(xiiMakeDelegate(&xiiRenderDataManager::OnExtractionEvent, this));
}

void xiiRenderDataManager::Initialize()
{
}

xiiArrayPtr<xiiPerInstanceData> xiiRenderDataManager::GetOrCreateInstanceData(const xiiComponent* pOwnerComponent, bool bDynamic, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount /*= 1*/) const
{
  XII_IGNORE_UNUSED(pOwnerComponent);
  XII_IGNORE_UNUSED(bDynamic);

  static thread_local xiiDynamicArray<xiiPerInstanceData> s_InstanceData;
  s_InstanceData.SetCount(uiCount);

  out_pBuffer                         = nullptr;
  inout_instanceDataOffset.m_uiOffset = 0;

  return s_InstanceData;
}

void xiiRenderDataManager::DeleteInstanceData(xiiInstanceDataOffset& inout_instanceDataOffset) const
{
  inout_instanceDataOffset = {};
}

xiiUInt32 xiiRenderDataManager::RegisterCustomInstanceData(const xiiGALBufferCreationDescription& desc, xiiStringView sDebugName, xiiDelegate<void()> beforeUploadCallback /*= {}*/)
{
  XII_IGNORE_UNUSED(desc);
  XII_IGNORE_UNUSED(sDebugName);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiBufferIndex = m_Buffers.GetCount();
  m_Buffers.PushBack(nullptr);

  if (beforeUploadCallback.IsValid())
  {
    m_BeforeUploadCallbacks.EnsureCount(uiBufferIndex + 1);
    m_BeforeUploadCallbacks[uiBufferIndex] = beforeUploadCallback;
  }

  return uiBufferIndex;
}

xiiByteArrayPtr xiiRenderDataManager::GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiStructByteSize, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount) const
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  XII_IGNORE_UNUSED(pOwnerComponent);

  static thread_local xiiDynamicArray<xiiUInt8> s_CustomData;
  s_CustomData.SetCountUninitialized(uiStructByteSize * uiCount);

  out_pBuffer                         = nullptr;
  inout_instanceDataOffset.m_uiOffset = 0;

  return xiiByteArrayPtr(s_CustomData.GetData(), s_CustomData.GetCount());
}

void xiiRenderDataManager::DeleteCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  inout_instanceDataOffset = {};
}

void xiiRenderDataManager::CompactCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiMaxSteps)
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  XII_IGNORE_UNUSED(uiMaxSteps);
}

xiiArrayPtr<xiiShaderTransform> xiiRenderDataManager::GetOrCreateSkinningData(const xiiComponent* pOwnerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiNumTransforms) const
{
  XII_IGNORE_UNUSED(pOwnerComponent);

  static thread_local xiiDynamicArray<xiiShaderTransform> s_SkinningData;
  s_SkinningData.SetCount(uiNumTransforms);

  inout_instanceDataOffset.m_uiOffset = 0;
  return s_SkinningData;
}

xiiArrayPtr<const xiiShaderTransform> xiiRenderDataManager::GetSkinningData(const xiiCustomInstanceDataOffset& instanceDataOffset) const
{
  XII_IGNORE_UNUSED(instanceDataOffset);

  static thread_local xiiDynamicArray<xiiShaderTransform> s_Empty;
  return s_Empty;
}

void xiiRenderDataManager::DeleteSkinningData(xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  inout_instanceDataOffset = {};
}

xiiSharedPtr<xiiGALDynamicBuffer> xiiRenderDataManager::GetSkinningDataBuffer() const
{
  if (m_Buffers.GetCount() > s_uiSkinningBufferIndex)
  {
    return m_Buffers[s_uiSkinningBufferIndex];
  }

  return nullptr;
}

void xiiRenderDataManager::BeginGpuDrivenBuild()
{
  XII_LOCK(m_Mutex);

  m_GpuDrivenInstances.Clear();
  m_GpuDrivenVisibleInstanceIndices.Clear();
}

void xiiRenderDataManager::AddGpuDrivenInstance(const xiiTransform& globalTransform, const xiiBoundingSphere& bounds, xiiUInt32 uiMeshId, xiiUInt32 uiMaterialId, xiiUInt32 uiFlags /*= 0U*/)
{
  XII_LOCK(m_Mutex);

  xiiGpuDrivenInstance& instance = m_GpuDrivenInstances.ExpandAndGetRef();
  instance.m_ObjectToWorld       = globalTransform.GetAsMat4();
  instance.m_Bounds              = bounds;
  instance.m_uiMeshId            = uiMeshId;
  instance.m_uiMaterialId        = uiMaterialId;
  instance.m_uiFlags             = uiFlags;
}

void xiiRenderDataManager::EndGpuDrivenBuild()
{
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiInstanceCapacity = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureGpuDrivenVisibilityResources(uiInstanceCapacity);

  GpuDrivenDispatchArguments dispatchArguments;
  if (!m_GpuDrivenInstances.IsEmpty())
  {
    const xiiUInt32 uiThreadGroupSize       = xiiMath::Max(1U, m_uiGpuVisibilityThreadGroupSize);
    dispatchArguments.m_uiThreadGroupCountX = (m_GpuDrivenInstances.GetCount() + (uiThreadGroupSize - 1U)) / uiThreadGroupSize;
  }

  auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::EndGpuDrivenBuild");
  if (!m_GpuDrivenInstances.IsEmpty())
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pGpuSceneInstancesBuffer, 0U, xiiMakeArrayPtr(m_GpuDrivenInstances.GetData(), m_GpuDrivenInstances.GetCount()).ToByteArray()).AssertSuccess();
  }

  if (m_pGpuVisibilityDispatchArgumentsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pGpuVisibilityDispatchArgumentsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&dispatchArguments), sizeof(dispatchArguments))).AssertSuccess();
  }
}

xiiArrayPtr<const xiiGpuDrivenInstance> xiiRenderDataManager::GetGpuDrivenInstances() const
{
  XII_LOCK(m_Mutex);

  static thread_local xiiDynamicArray<xiiGpuDrivenInstance> s_GpuDrivenInstancesSnapshot;
  s_GpuDrivenInstancesSnapshot = m_GpuDrivenInstances;
  return s_GpuDrivenInstancesSnapshot;
}

void xiiRenderDataManager::SetGpuDrivenVisibleInstanceIndices(xiiArrayPtr<const xiiUInt32> visibleInstanceIndices)
{
  XII_LOCK(m_Mutex);

  m_GpuDrivenVisibleInstanceIndices.SetCountUninitialized(visibleInstanceIndices.GetCount());
  if (!visibleInstanceIndices.IsEmpty())
  {
    xiiMemoryUtils::Copy(m_GpuDrivenVisibleInstanceIndices.GetData(), visibleInstanceIndices.GetPtr(), visibleInstanceIndices.GetCount());
  }
}

xiiArrayPtr<const xiiUInt32> xiiRenderDataManager::GetGpuDrivenVisibleInstanceIndices() const
{
  XII_LOCK(m_Mutex);

  static thread_local xiiDynamicArray<xiiUInt32> s_GpuDrivenVisibleInstanceIndicesSnapshot;
  s_GpuDrivenVisibleInstanceIndicesSnapshot = m_GpuDrivenVisibleInstanceIndices;
  return s_GpuDrivenVisibleInstanceIndicesSnapshot;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderDataManager::GetGpuDrivenSceneInstancesBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuSceneInstancesBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderDataManager::GetGpuDrivenVisibleInstancesBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuVisibleInstancesBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderDataManager::GetGpuDrivenVisibleInstanceCountBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuVisibleInstanceCountBuffer;
}

bool xiiRenderDataManager::TryGetGpuDrivenVisibleInstanceCountReadback(xiiUInt32& out_uiVisibleInstanceCount, bool bWaitForCompletion /*= false*/) const
{
  XII_LOCK(m_Mutex);

  if (m_pGpuVisibilityReadbackFence == nullptr || m_pGpuVisibleInstanceCountReadbackBuffer == nullptr || m_uiGpuVisibilityReadbackFenceValue == 0U)
  {
    return false;
  }

  if (bWaitForCompletion)
  {
    m_pGpuVisibilityReadbackFence->Wait(m_uiGpuVisibilityReadbackFenceValue);
  }

  const xiiUInt64 uiCompletedValue = m_pGpuVisibilityReadbackFence->GetCompletedValue();
  if (uiCompletedValue < m_uiGpuVisibilityReadbackFenceValue || uiCompletedValue == m_uiGpuVisibilityReadbackCompletedValue)
  {
    return false;
  }

  auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Transfer>("xiiRenderDataManager::TryGetGpuDrivenVisibleInstanceCountReadback");

  void* pMappedData = nullptr;
  if (pCommandListScope->MapBuffer(m_pGpuVisibleInstanceCountReadbackBuffer, xiiGALMapType::Read, xiiGALMapFlags::None, pMappedData).Failed())
  {
    return false;
  }

  out_uiVisibleInstanceCount = *reinterpret_cast<const xiiUInt32*>(pMappedData);
  pCommandListScope->UnmapBuffer(m_pGpuVisibleInstanceCountReadbackBuffer, xiiGALMapType::Read).AssertSuccess("Failed to unmap GPU visibility readback buffer.");

  m_uiGpuVisibilityReadbackCompletedValue = uiCompletedValue;
  return true;
}

void xiiRenderDataManager::AddGpuDrivenVisibilityPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");

  m_pGpuDrivenVisibilityPass->SetInstanceCount(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));
  m_pGpuDrivenVisibilityPass->SetDispatchEnabled(bEnableDispatch);

  if (m_bGpuVisibilityUseInternalIndirectDispatch && m_pGpuVisibilityDispatchArgumentsBuffer != nullptr)
  {
    m_pGpuDrivenVisibilityPass->SetIndirectDispatchArguments(m_pGpuVisibilityDispatchArgumentsBuffer, 0U, xiiGALStateTransitionMode::Transition);
  }
  else
  {
    m_pGpuDrivenVisibilityPass->SetIndirectDispatchArguments(nullptr, 0U, xiiGALStateTransitionMode::Transition);
  }

  inout_runtime.AddPass(m_pGpuDrivenVisibilityPass.Borrow());
}

void xiiRenderDataManager::SetGpuDrivenVisibilityThreadGroupSize(xiiUInt32 uiThreadGroupSize) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_uiGpuVisibilityThreadGroupSize = xiiMath::Max(1U, uiThreadGroupSize);
  m_pGpuDrivenVisibilityPass->SetThreadGroupSize(m_uiGpuVisibilityThreadGroupSize);
}

void xiiRenderDataManager::SetGpuDrivenVisibilityDirectDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->SetDirectDispatchThreadGroupCount(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void xiiRenderDataManager::SetGpuDrivenVisibilityIndirectDispatchArguments(xiiSharedPtr<xiiGALBuffer> pIndirectDispatchArguments, xiiUInt64 uiDispatchArgumentOffset /*= 0U*/, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode /*= xiiGALStateTransitionMode::Transition*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_bGpuVisibilityUseInternalIndirectDispatch = false;
  m_pGpuDrivenVisibilityPass->SetIndirectDispatchArguments(pIndirectDispatchArguments, uiDispatchArgumentOffset, bufferTransitionMode);
}

void xiiRenderDataManager::SetGpuDrivenVisibilityUseInternalIndirectDispatch(bool bEnable) const
{
  XII_LOCK(m_Mutex);

  m_bGpuVisibilityUseInternalIndirectDispatch = bEnable;
}

void xiiRenderDataManager::SetGpuDrivenVisibilitySetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::ClearGpuDrivenVisibilitySetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->ClearSetupCommandListFunc();
}

void xiiRenderDataManager::AddRayTracedShadowsPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetEnabled(bEnableDispatch);
  inout_runtime.AddPass(m_pRayTracedShadowsPass.Borrow());
}

void xiiRenderDataManager::AddDynamicResolutionDecisionPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= true*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDynamicResolutionDecisionPass != nullptr, "Dynamic-resolution decision pass must be initialized.");

  EnsureDynamicResolutionDecisionResources(1U);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::AddDynamicResolutionDecisionPass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pDynamicResolutionFrameTimingBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vDynamicResolutionFrameTimingSample), sizeof(m_vDynamicResolutionFrameTimingSample))).AssertSuccess();
  }

  m_pDynamicResolutionDecisionPass->SetEnabled(bEnableDispatch);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer);
  }

  if (m_pDynamicResolutionDecisionBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("DynamicResolutionData"), m_pDynamicResolutionDecisionBuffer);
  }

  inout_runtime.AddPass(m_pDynamicResolutionDecisionPass.Borrow());
}

void xiiRenderDataManager::SetDynamicResolutionFrameTimingSample(const xiiVec4& vFrameTimingSample) const
{
  XII_LOCK(m_Mutex);

  m_vDynamicResolutionFrameTimingSample = vFrameTimingSample;
}

void xiiRenderDataManager::SetRayTracedShadowsDenoiserHistoryEnabled(bool bEnable) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDenoiserHistoryEnabled(bEnable);
}

void xiiRenderDataManager::SetRayTracedShadowsSceneTlasResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetSceneTlasResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsDepthResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDepthResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsNormalResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetNormalResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsLightDataResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetLightDataResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsShadowMaskResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetShadowMaskResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsHistoryInputResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetHistoryInputResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsHistoryOutputResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetHistoryOutputResourceName(sResourceName);
}

void xiiRenderDataManager::SetRayTracedShadowsSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetRayTracedShadowsDispatchFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> dispatchFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDispatchRayTracingFunc(dispatchFunc);
}

void xiiRenderDataManager::ClearRayTracedShadowsSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->ClearSetupCommandListFunc();
}

void xiiRenderDataManager::ClearRayTracedShadowsDispatchFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->ClearDispatchRayTracingFunc();
}

void xiiRenderDataManager::CompactSkinningDataBuffer(const UpdateContext& context)
{
  XII_IGNORE_UNUSED(context);
}

void xiiRenderDataManager::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  XII_IGNORE_UNUSED(e);
}

void xiiRenderDataManager::EnsureGpuDrivenVisibilityResources(xiiUInt32 uiInstanceCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt64 uiInstanceBufferSize = static_cast<xiiUInt64>(uiInstanceCapacity) * sizeof(xiiGpuDrivenInstance);
  if (m_pGpuSceneInstancesBuffer == nullptr || m_pGpuSceneInstancesBuffer->GetDescription().m_uiSize < uiInstanceBufferSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiGpuDrivenInstance);
    bufferDescription.m_uiSize              = uiInstanceBufferSize;

    m_pGpuSceneInstancesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuSceneInstancesBuffer != nullptr)
    {
      m_pGpuSceneInstancesBuffer->SetDebugName("RenderDataManager::GpuSceneInstances");
    }
  }

  const xiiUInt64 uiVisibleInstancesBufferSize = static_cast<xiiUInt64>(uiInstanceCapacity) * sizeof(xiiUInt32);
  if (m_pGpuVisibleInstancesBuffer == nullptr || m_pGpuVisibleInstancesBuffer->GetDescription().m_uiSize < uiVisibleInstancesBufferSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiVisibleInstancesBufferSize;

    m_pGpuVisibleInstancesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuVisibleInstancesBuffer != nullptr)
    {
      m_pGpuVisibleInstancesBuffer->SetDebugName("RenderDataManager::GpuVisibleInstances");
    }
  }

  if (m_pGpuVisibleInstanceCountBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = sizeof(xiiUInt32);

    m_pGpuVisibleInstanceCountBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuVisibleInstanceCountBuffer != nullptr)
    {
      m_pGpuVisibleInstanceCountBuffer->SetDebugName("RenderDataManager::GpuVisibleInstanceCount");
    }
  }

  if (m_pGpuVisibilityDispatchArgumentsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Undefined;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::IndirectDrawArguments;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = 0U;
    bufferDescription.m_uiSize              = sizeof(GpuDrivenDispatchArguments);

    m_pGpuVisibilityDispatchArgumentsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuVisibilityDispatchArgumentsBuffer != nullptr)
    {
      m_pGpuVisibilityDispatchArgumentsBuffer->SetDebugName("RenderDataManager::GpuVisibilityDispatchArguments");
    }
  }

  if (m_pGpuVisibleInstanceCountReadbackBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Undefined;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::None;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Staging;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Read;
    bufferDescription.m_uiElementByteStride = 0U;
    bufferDescription.m_uiSize              = sizeof(xiiUInt32);

    m_pGpuVisibleInstanceCountReadbackBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuVisibleInstanceCountReadbackBuffer != nullptr)
    {
      m_pGpuVisibleInstanceCountReadbackBuffer->SetDebugName("RenderDataManager::GpuVisibleInstanceCountReadback");
    }
  }

  if (m_pGpuVisibilityReadbackFence == nullptr)
  {
    xiiGALFenceCreationDescription fenceDescription;
    fenceDescription.m_Type       = xiiGALFenceType::CpuWaitOnly;
    m_pGpuVisibilityReadbackFence = pDevice->CreateFence(fenceDescription);
    if (m_pGpuVisibilityReadbackFence != nullptr)
    {
      m_pGpuVisibilityReadbackFence->SetDebugName("RenderDataManager::GpuVisibilityReadbackFence");
    }
  }
}

void xiiRenderDataManager::EnsureDynamicResolutionDecisionResources(xiiUInt32 uiElementCount) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt64 uiRequiredSize = static_cast<xiiUInt64>(xiiMath::Max(1U, uiElementCount)) * sizeof(xiiVec4);

  if (m_pDynamicResolutionFrameTimingBuffer == nullptr || m_pDynamicResolutionFrameTimingBuffer->GetDescription().m_uiSize < uiRequiredSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiRequiredSize;

    m_pDynamicResolutionFrameTimingBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
    {
      m_pDynamicResolutionFrameTimingBuffer->SetDebugName("RenderDataManager::FrameTimingData");
    }
  }

  if (m_pDynamicResolutionDecisionBuffer == nullptr || m_pDynamicResolutionDecisionBuffer->GetDescription().m_uiSize < uiRequiredSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiRequiredSize;

    m_pDynamicResolutionDecisionBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDynamicResolutionDecisionBuffer != nullptr)
    {
      m_pDynamicResolutionDecisionBuffer->SetDebugName("RenderDataManager::DynamicResolutionData");
    }
  }
}

void xiiRenderDataManager::SetupGpuDrivenVisibilityCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_hGpuDrivenVisibilityShader.IsValid() == false)
  {
    m_hGpuDrivenVisibilityShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/GpuDrivenVisibilityCulling.xiiShader");
  }

  if (m_pGpuDrivenVisibilityPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hGpuDrivenVisibilityShader, s_PermutationVars, true);
    xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!pPermutation || pPermutation.GetAcquireResult() != xiiResourceAcquireResult::Final || !pPermutation->IsShaderValid())
    {
      return;
    }

    xiiSharedPtr<xiiGALShader> pComputeShader = pPermutation->GetGALShader(xiiGALShaderType::Compute);
    if (pComputeShader == nullptr)
    {
      return;
    }

    xiiGALComputePipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pPipelineResourceSignature = pPermutation->GetPipelineResourceSignature();
    pipelineDescription.m_pComputeShader             = pComputeShader;

    m_pGpuDrivenVisibilityPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pGpuDrivenVisibilityPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pGpuDrivenVisibilityPipelineState);

  if (m_pGpuSceneInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneInstances"), m_pGpuSceneInstancesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }
  if (m_pGpuVisibleInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuVisibleInstances"), m_pGpuVisibleInstancesBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
  if (m_pGpuVisibleInstanceCountBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuVisibleInstanceCount"), m_pGpuVisibleInstanceCountBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupDynamicResolutionDecisionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDynamicResolutionDecisionResources(1U);

  if (m_hDynamicResolutionDecisionShader.IsValid() == false)
  {
    m_hDynamicResolutionDecisionShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DynamicResolutionDecision.xiiShader");
  }

  if (m_pDynamicResolutionDecisionPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDynamicResolutionDecisionShader, s_PermutationVars, true);
    xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!pPermutation || pPermutation.GetAcquireResult() != xiiResourceAcquireResult::Final || !pPermutation->IsShaderValid())
    {
      return;
    }

    xiiSharedPtr<xiiGALShader> pComputeShader = pPermutation->GetGALShader(xiiGALShaderType::Compute);
    if (pComputeShader == nullptr)
    {
      return;
    }

    xiiGALComputePipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pPipelineResourceSignature = pPermutation->GetPipelineResourceSignature();
    pipelineDescription.m_pComputeShader             = pComputeShader;

    m_pDynamicResolutionDecisionPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pDynamicResolutionDecisionPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pDynamicResolutionDecisionPipelineState);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pDynamicResolutionDecisionBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("DynamicResolutionData"), m_pDynamicResolutionDecisionBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::OnGpuDrivenVisibilityPostDispatch(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_pGpuVisibleInstanceCountBuffer == nullptr || m_pGpuVisibleInstanceCountReadbackBuffer == nullptr || m_pGpuVisibilityReadbackFence == nullptr)
  {
    return;
  }

  commandList.CopyBufferRegion(m_pGpuVisibleInstanceCountBuffer, 0U, m_pGpuVisibleInstanceCountReadbackBuffer, 0U, sizeof(xiiUInt32));

  const xiiUInt64 uiFenceValue = m_uiGpuVisibilityReadbackFenceValue + 1U;
  commandList.EnqueueSignal(m_pGpuVisibilityReadbackFence, uiFenceValue);
  m_uiGpuVisibilityReadbackFenceValue = uiFenceValue;
}

void xiiRenderDataManager::SetupRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::DispatchRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderDataManager);
