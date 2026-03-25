#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>
#include <GraphicsCore/Pipeline/Passes/InstanceUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/LodSelectionPass.h>
#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/RayTracedShadowsPass.h>
#include <GraphicsCore/Pipeline/Passes/SkinningPass.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/FrameConstants.h>

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

  m_pFrameSetupPass = XII_DEFAULT_NEW(xiiRenderGraphFrameSetupPass);
  m_pFrameSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupFrameSetupCommandList, this));

  m_pGpuDrivenVisibilityPass = XII_DEFAULT_NEW(xiiRenderGraphGpuVisibilityPass);
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupGpuDrivenVisibilityCommandList, this));
  m_pGpuDrivenVisibilityPass->SetPostDispatchCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::OnGpuDrivenVisibilityPostDispatch, this));
  m_bGpuVisibilityUseInternalIndirectDispatch = true;

  m_pInstanceUpdatePass = XII_DEFAULT_NEW(xiiRenderGraphInstanceUpdatePass);
  m_pInstanceUpdatePass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupInstanceUpdateCommandList, this));
  m_pInstanceUpdatePass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pLodSelectionPass = XII_DEFAULT_NEW(xiiRenderGraphLodSelectionPass);
  m_pLodSelectionPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupLodSelectionCommandList, this));
  m_pLodSelectionPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pRayTracedShadowsPass = XII_DEFAULT_NEW(xiiRenderGraphRayTracedShadowsPass);
  m_pRayTracedShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupRayTracedShadowsCommandList, this));
  m_pRayTracedShadowsPass->SetDispatchRayTracingFunc(xiiMakeDelegate(&xiiRenderDataManager::DispatchRayTracedShadowsCommandList, this));

  m_pDynamicResolutionPass = XII_DEFAULT_NEW(xiiRenderGraphDynamicResolutionPass);
  m_pDynamicResolutionPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDynamicResolutionCommandList, this));
  m_pDynamicResolutionPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pSkinningPass = XII_DEFAULT_NEW(xiiRenderGraphSkinningPass);
  m_pSkinningPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupSkinningAndMorphCommandList, this));
  m_pSkinningPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pPerFrameBufferUploadPass = XII_DEFAULT_NEW(xiiRenderGraphPerFrameBufferUploadPass);
  m_pPerFrameBufferUploadPass->SetUploadCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::UploadPerFrameBufferDataCommandList, this));

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

void xiiRenderDataManager::AddFrameSetupPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= true*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pFrameSetupPass != nullptr, "Frame-setup pass must be initialized.");

  EnsureFrameSetupResources();

  if (m_pPreviousFrameStatsBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Graphics>("xiiRenderDataManager::AddFrameSetupPass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pPreviousFrameStatsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_PreviousFrameStatsSample), sizeof(m_PreviousFrameStatsSample))).AssertSuccess();
  }

  m_pFrameSetupPass->SetEnabled(bEnablePass);

  if (m_pPreviousFrameStatsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("PreviousFrameStats"), m_pPreviousFrameStatsBuffer);
  }

  if (m_pFrameConstantsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameConstants"), m_pFrameConstantsBuffer);
  }

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer);
  }

  if (m_pFrameTimestampRangesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameTimestampRanges"), m_pFrameTimestampRangesBuffer);
  }

  inout_runtime.AddPass(m_pFrameSetupPass.Borrow());
}

void xiiRenderDataManager::AddDynamicResolutionPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= true*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDynamicResolutionPass != nullptr, "Dynamic-resolution pass must be initialized.");

  EnsureDynamicResolutionResources(1U);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::AddDynamicResolutionPass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pDynamicResolutionFrameTimingBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vDynamicResolutionFrameTimingSample), sizeof(m_vDynamicResolutionFrameTimingSample))).AssertSuccess();

    if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
    {
      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pDynamicResolutionCameraVelocityBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vDynamicResolutionCameraVelocitySample), sizeof(m_vDynamicResolutionCameraVelocitySample))).AssertSuccess();
    }
  }

  m_pDynamicResolutionPass->SetEnabled(bEnableDispatch);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer);
  }

  if (m_pDynamicResolutionBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("DynamicResolutionData"), m_pDynamicResolutionBuffer);
  }

  if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("CameraVelocityData"), m_pDynamicResolutionCameraVelocityBuffer);
  }

  inout_runtime.AddPass(m_pDynamicResolutionPass.Borrow());
}

void xiiRenderDataManager::AddSkinningAndMorphPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pSkinningPass != nullptr, "Skinning pass must be initialized.");

  const xiiUInt32 uiDeformerElementCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureSkinningAndMorphResources(uiDeformerElementCount);

  if (m_pSkinningInputBuffer != nullptr || m_pMorphWeightsBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::AddSkinningAndMorphPass");
    if (m_pSkinningInputBuffer != nullptr)
    {
      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pSkinningInputBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vSkinningInputSample), sizeof(m_vSkinningInputSample))).AssertSuccess();
    }

    if (m_pMorphWeightsBuffer != nullptr)
    {
      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pMorphWeightsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vMorphWeightsSample), sizeof(m_vMorphWeightsSample))).AssertSuccess();
    }
  }

  m_pSkinningPass->SetEnabled(bEnableDispatch);
  m_pSkinningPass->SetDispatchThreadGroupCount(uiDeformerElementCount, 1U, 1U);

  if (m_pSkinningInputBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SkinningInput"), m_pSkinningInputBuffer);
  }

  if (m_pSkinningBonePaletteBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("BonePalette"), m_pSkinningBonePaletteBuffer);
  }

  if (m_pMorphWeightsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("MorphWeights"), m_pMorphWeightsBuffer);
  }

  if (m_pSkinnedVerticesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SkinnedVertices"), m_pSkinnedVerticesBuffer);
  }

  inout_runtime.AddPass(m_pSkinningPass.Borrow());
}

void xiiRenderDataManager::AddInstanceTransformAndBoundsUpdatePass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pInstanceUpdatePass != nullptr, "Instance update pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureSkinningAndMorphResources(uiInstanceCount);
  EnsureInstanceUpdateResources(uiInstanceCount);

  if (m_pSceneTransformsBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::AddInstanceTransformAndBoundsUpdatePass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pSceneTransformsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_SceneTransformsSample), sizeof(m_SceneTransformsSample))).AssertSuccess();
  }

  m_pInstanceUpdatePass->SetEnabled(bEnableDispatch);
  m_pInstanceUpdatePass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  if (m_pSceneTransformsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneTransforms"), m_pSceneTransformsBuffer);
  }

  if (m_pSkinnedVerticesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SkinnedVertices"), m_pSkinnedVerticesBuffer);
  }

  if (m_pUpdatedGpuSceneInstancesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneInstances"), m_pUpdatedGpuSceneInstancesBuffer);
  }

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer);
  }

  inout_runtime.AddPass(m_pInstanceUpdatePass.Borrow());
}

void xiiRenderDataManager::AddLodSelectionAndMeshletClassificationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLodSelectionPass != nullptr, "LOD selection pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureFrameSetupResources();
  EnsureInstanceUpdateResources(uiInstanceCount);
  EnsureLodSelectionResources(uiInstanceCount);

  m_pLodSelectionPass->SetEnabled(bEnableDispatch);
  m_pLodSelectionPass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  xiiSharedPtr<xiiGALBuffer> pSceneInstancesBuffer = m_pUpdatedGpuSceneInstancesBuffer != nullptr ? m_pUpdatedGpuSceneInstancesBuffer : m_pGpuSceneInstancesBuffer;
  if (pSceneInstancesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneInstances"), pSceneInstancesBuffer);
  }

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer);
  }

  if (m_pFrameConstantsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameConstants"), m_pFrameConstantsBuffer);
  }

  if (m_pGpuLodSelectionsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuLodSelections"), m_pGpuLodSelectionsBuffer);
  }

  if (m_pGpuDrawMetadataBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuDrawMetadata"), m_pGpuDrawMetadataBuffer);
  }

  inout_runtime.AddPass(m_pLodSelectionPass.Borrow());
}

void xiiRenderDataManager::AddPerFrameBufferUploadPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableUploads /*= true*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pPerFrameBufferUploadPass != nullptr, "Per-frame upload pass must be initialized.");

  EnsurePerFrameUploadResources(m_uiPerFrameUploadRingSize);

  m_pPerFrameBufferUploadPass->SetEnabled(bEnableUploads);

  if (m_pPerFrameCameraConstantsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("CameraConstants"), m_pPerFrameCameraConstantsBuffer);
  }

  if (m_pPerFrameLightDataBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameLightData"), m_pPerFrameLightDataBuffer);
  }

  if (m_pPerFrameGlobalParamsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GlobalParams"), m_pPerFrameGlobalParamsBuffer);
  }

  inout_runtime.AddPass(m_pPerFrameBufferUploadPass.Borrow());

  if (bEnableUploads)
  {
    m_uiPerFrameUploadWriteIndex = (m_uiPerFrameUploadWriteIndex + 1U) % xiiMath::Max(1U, m_uiPerFrameUploadRingSize);
  }
}

void xiiRenderDataManager::SetDynamicResolutionFrameTimingSample(const xiiVec4& vFrameTimingSample) const
{
  XII_LOCK(m_Mutex);

  m_vDynamicResolutionFrameTimingSample = vFrameTimingSample;
}

void xiiRenderDataManager::SetDynamicResolutionCameraVelocitySample(const xiiVec4& vCameraVelocitySample) const
{
  XII_LOCK(m_Mutex);

  m_vDynamicResolutionCameraVelocitySample = vCameraVelocitySample;
}

void xiiRenderDataManager::SetSkinningInputSample(const xiiVec4& vSkinningInputSample) const
{
  XII_LOCK(m_Mutex);

  m_vSkinningInputSample = vSkinningInputSample;
}

void xiiRenderDataManager::SetMorphWeightsSample(const xiiVec4& vMorphWeightsSample) const
{
  XII_LOCK(m_Mutex);

  m_vMorphWeightsSample = vMorphWeightsSample;
}

void xiiRenderDataManager::SetSceneTransformsSample(const xiiShaderTransform& sceneTransformSample) const
{
  XII_LOCK(m_Mutex);

  m_SceneTransformsSample = sceneTransformSample;
}

void xiiRenderDataManager::SetPerFrameUploadCameraConstantsSample(const xiiPerFrameCameraUploadData& cameraConstantsSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameCameraConstantsSample = cameraConstantsSample;
}

void xiiRenderDataManager::SetPerFrameUploadLightDataSample(const xiiPerFrameLightUploadData& lightDataSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameLightDataSample = lightDataSample;
}

void xiiRenderDataManager::SetPerFrameUploadGlobalParamsSample(const xiiPerFrameGlobalUploadData& globalParamsSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameGlobalParamsSample = globalParamsSample;
}

void xiiRenderDataManager::SetPreviousFrameStatsSample(const xiiPreviousFrameStats& previousFrameStatsSample) const
{
  XII_LOCK(m_Mutex);

  m_PreviousFrameStatsSample = previousFrameStatsSample;
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

void xiiRenderDataManager::EnsureDynamicResolutionResources(xiiUInt32 uiElementCount) const
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

  if (m_pDynamicResolutionCameraVelocityBuffer == nullptr || m_pDynamicResolutionCameraVelocityBuffer->GetDescription().m_uiSize < uiRequiredSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiRequiredSize;

    m_pDynamicResolutionCameraVelocityBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
    {
      m_pDynamicResolutionCameraVelocityBuffer->SetDebugName("RenderDataManager::CameraVelocityData");
    }
  }

  if (m_pDynamicResolutionBuffer == nullptr || m_pDynamicResolutionBuffer->GetDescription().m_uiSize < uiRequiredSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiRequiredSize;

    m_pDynamicResolutionBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDynamicResolutionBuffer != nullptr)
    {
      m_pDynamicResolutionBuffer->SetDebugName("RenderDataManager::DynamicResolutionData");
    }
  }
}

void xiiRenderDataManager::EnsureSkinningAndMorphResources(xiiUInt32 uiElementCount) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedElementCount = xiiMath::Max(1U, uiElementCount);

  const xiiUInt64 uiInputSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pSkinningInputBuffer == nullptr || m_pSkinningInputBuffer->GetDescription().m_uiSize < uiInputSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiInputSize;

    m_pSkinningInputBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pSkinningInputBuffer != nullptr)
    {
      m_pSkinningInputBuffer->SetDebugName("RenderDataManager::SkinningInput");
    }
  }

  const xiiUInt64 uiBonePaletteSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiShaderTransform);
  if (m_pSkinningBonePaletteBuffer == nullptr || m_pSkinningBonePaletteBuffer->GetDescription().m_uiSize < uiBonePaletteSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiShaderTransform);
    bufferDescription.m_uiSize              = uiBonePaletteSize;

    m_pSkinningBonePaletteBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pSkinningBonePaletteBuffer != nullptr)
    {
      m_pSkinningBonePaletteBuffer->SetDebugName("RenderDataManager::BonePalette");
    }
  }

  const xiiUInt64 uiMorphWeightsSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pMorphWeightsBuffer == nullptr || m_pMorphWeightsBuffer->GetDescription().m_uiSize < uiMorphWeightsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiMorphWeightsSize;

    m_pMorphWeightsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pMorphWeightsBuffer != nullptr)
    {
      m_pMorphWeightsBuffer->SetDebugName("RenderDataManager::MorphWeights");
    }
  }

  const xiiUInt64 uiSkinnedVerticesSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pSkinnedVerticesBuffer == nullptr || m_pSkinnedVerticesBuffer->GetDescription().m_uiSize < uiSkinnedVerticesSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiSkinnedVerticesSize;

    m_pSkinnedVerticesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pSkinnedVerticesBuffer != nullptr)
    {
      m_pSkinnedVerticesBuffer->SetDebugName("RenderDataManager::SkinnedVertices");
    }
  }
}

void xiiRenderDataManager::EnsureInstanceUpdateResources(xiiUInt32 uiElementCount) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedElementCount = xiiMath::Max(1U, uiElementCount);

  const xiiUInt64 uiSceneTransformsSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiShaderTransform);
  if (m_pSceneTransformsBuffer == nullptr || m_pSceneTransformsBuffer->GetDescription().m_uiSize < uiSceneTransformsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiShaderTransform);
    bufferDescription.m_uiSize              = uiSceneTransformsSize;

    m_pSceneTransformsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pSceneTransformsBuffer != nullptr)
    {
      m_pSceneTransformsBuffer->SetDebugName("RenderDataManager::SceneTransforms");
    }
  }

  const xiiUInt64 uiSceneInstancesSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiGpuDrivenInstance);
  if (m_pUpdatedGpuSceneInstancesBuffer == nullptr || m_pUpdatedGpuSceneInstancesBuffer->GetDescription().m_uiSize < uiSceneInstancesSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiGpuDrivenInstance);
    bufferDescription.m_uiSize              = uiSceneInstancesSize;

    m_pUpdatedGpuSceneInstancesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pUpdatedGpuSceneInstancesBuffer != nullptr)
    {
      m_pUpdatedGpuSceneInstancesBuffer->SetDebugName("RenderDataManager::UpdatedGpuSceneInstances");
    }
  }

  const xiiUInt64 uiSceneBoundsSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pGpuSceneBoundsBuffer == nullptr || m_pGpuSceneBoundsBuffer->GetDescription().m_uiSize < uiSceneBoundsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiSceneBoundsSize;

    m_pGpuSceneBoundsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuSceneBoundsBuffer != nullptr)
    {
      m_pGpuSceneBoundsBuffer->SetDebugName("RenderDataManager::GpuSceneBounds");
    }
  }
}

void xiiRenderDataManager::EnsureLodSelectionResources(xiiUInt32 uiElementCount) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedElementCount = xiiMath::Max(1U, uiElementCount);

  const xiiUInt64 uiLodSelectionSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pGpuLodSelectionsBuffer == nullptr || m_pGpuLodSelectionsBuffer->GetDescription().m_uiSize < uiLodSelectionSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiLodSelectionSize;

    m_pGpuLodSelectionsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuLodSelectionsBuffer != nullptr)
    {
      m_pGpuLodSelectionsBuffer->SetDebugName("RenderDataManager::GpuLodSelections");
    }
  }

  const xiiUInt64 uiDrawMetadataSize = static_cast<xiiUInt64>(uiResolvedElementCount) * sizeof(xiiVec4);
  if (m_pGpuDrawMetadataBuffer == nullptr || m_pGpuDrawMetadataBuffer->GetDescription().m_uiSize < uiDrawMetadataSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiDrawMetadataSize;

    m_pGpuDrawMetadataBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuDrawMetadataBuffer != nullptr)
    {
      m_pGpuDrawMetadataBuffer->SetDebugName("RenderDataManager::GpuDrawMetadata");
    }
  }
}

void xiiRenderDataManager::EnsureFrameSetupResources() const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureDynamicResolutionResources(1U);

  if (m_pPreviousFrameStatsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiPreviousFrameStats);
    bufferDescription.m_uiSize              = sizeof(xiiPreviousFrameStats);

    m_pPreviousFrameStatsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pPreviousFrameStatsBuffer != nullptr)
    {
      m_pPreviousFrameStatsBuffer->SetDebugName("RenderDataManager::PreviousFrameStats");
    }
  }

  if (m_pFrameConstantsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Undefined;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = 0U;
    bufferDescription.m_uiSize              = sizeof(xiiFrameConstants);

    m_pFrameConstantsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pFrameConstantsBuffer != nullptr)
    {
      m_pFrameConstantsBuffer->SetDebugName("RenderDataManager::FrameConstants");
    }
  }

  if (m_pFrameTimestampRangesBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiFrameTimestampRange);
    bufferDescription.m_uiSize              = 8U * sizeof(xiiFrameTimestampRange);

    m_pFrameTimestampRangesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pFrameTimestampRangesBuffer != nullptr)
    {
      m_pFrameTimestampRangesBuffer->SetDebugName("RenderDataManager::FrameTimestampRanges");
    }
  }
}

void xiiRenderDataManager::EnsurePerFrameUploadResources(xiiUInt32 uiRingSize) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedRingSize = xiiMath::Max(1U, uiRingSize);

  auto EnsureUploadBuffer = [&](xiiSharedPtr<xiiGALBuffer>& inout_pBuffer, xiiStringView sDebugName, xiiUInt32 uiStructSize)
  {
    const xiiUInt64 uiRequiredSize = static_cast<xiiUInt64>(uiResolvedRingSize) * uiStructSize;

    if (inout_pBuffer != nullptr && inout_pBuffer->GetDescription().m_uiSize >= uiRequiredSize)
    {
      return;
    }

    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = uiStructSize;
    bufferDescription.m_uiSize              = uiRequiredSize;

    inout_pBuffer = pDevice->CreateBuffer(bufferDescription);
    if (inout_pBuffer != nullptr)
    {
      inout_pBuffer->SetDebugName(sDebugName);
    }
  };

  EnsureUploadBuffer(m_pPerFrameCameraConstantsBuffer, "RenderDataManager::PerFrameCameraConstants", sizeof(xiiPerFrameCameraUploadData));
  EnsureUploadBuffer(m_pPerFrameLightDataBuffer, "RenderDataManager::PerFrameLightData", sizeof(xiiPerFrameLightUploadData));
  EnsureUploadBuffer(m_pPerFrameGlobalParamsBuffer, "RenderDataManager::PerFrameGlobalParams", sizeof(xiiPerFrameGlobalUploadData));
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

  xiiSharedPtr<xiiGALBuffer> pSceneInstancesBuffer = m_pUpdatedGpuSceneInstancesBuffer != nullptr ? m_pUpdatedGpuSceneInstancesBuffer : m_pGpuSceneInstancesBuffer;
  if (pSceneInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneInstances"), pSceneInstancesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
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

void xiiRenderDataManager::SetupDynamicResolutionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDynamicResolutionResources(1U);

  if (m_hDynamicResolutionShader.IsValid() == false)
  {
    m_hDynamicResolutionShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DynamicResolution.xiiShader");
  }

  if (m_pDynamicResolutionPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDynamicResolutionShader, s_PermutationVars, true);
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

    m_pDynamicResolutionPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pDynamicResolutionPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pDynamicResolutionPipelineState);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("CameraVelocityData"), m_pDynamicResolutionCameraVelocityBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pDynamicResolutionBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("DynamicResolutionData"), m_pDynamicResolutionBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupSkinningAndMorphCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureSkinningAndMorphResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_hSkinningShader.IsValid() == false)
  {
    m_hSkinningShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Skinning.xiiShader");
  }

  if (m_pSkinningPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hSkinningShader, s_PermutationVars, true);
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

    m_pSkinningPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pSkinningPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pSkinningPipelineState);

  if (m_pSkinningInputBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("SkinningInput"), m_pSkinningInputBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pSkinningBonePaletteBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("BonePalette"), m_pSkinningBonePaletteBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pMorphWeightsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("MorphWeights"), m_pMorphWeightsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pSkinnedVerticesBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("SkinnedVertices"), m_pSkinnedVerticesBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupInstanceUpdateCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureInstanceUpdateResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_hInstanceUpdateShader.IsValid() == false)
  {
    m_hInstanceUpdateShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/InstanceUpdate.xiiShader");
  }

  if (m_pInstanceUpdatePipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hInstanceUpdateShader, s_PermutationVars, true);
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

    m_pInstanceUpdatePipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pInstanceUpdatePipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pInstanceUpdatePipelineState);

  if (m_pSceneTransformsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("SceneTransforms"), m_pSceneTransformsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pSkinnedVerticesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("SkinnedVertices"), m_pSkinnedVerticesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pUpdatedGpuSceneInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuSceneInstances"), m_pUpdatedGpuSceneInstancesBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupLodSelectionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureFrameSetupResources();
  EnsureInstanceUpdateResources(uiInstanceCount);
  EnsureLodSelectionResources(uiInstanceCount);

  if (m_hLodSelectionShader.IsValid() == false)
  {
    m_hLodSelectionShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LodSelection.xiiShader");
  }

  if (m_pLodSelectionPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hLodSelectionShader, s_PermutationVars, true);
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

    m_pLodSelectionPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pLodSelectionPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pLodSelectionPipelineState);

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  xiiSharedPtr<xiiGALBuffer> pSceneInstancesBuffer = m_pUpdatedGpuSceneInstancesBuffer != nullptr ? m_pUpdatedGpuSceneInstancesBuffer : m_pGpuSceneInstancesBuffer;
  if (pSceneInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneInstances"), pSceneInstancesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pFrameConstantsBuffer != nullptr)
  {
    commandList.ResolveAndSetConstantBuffer(xiiTempHashedString("xiiFrameConstants"), m_pFrameConstantsBuffer);
  }

  if (m_pGpuLodSelectionsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuLodSelections"), m_pGpuLodSelectionsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pGpuDrawMetadataBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuDrawMetadata"), m_pGpuDrawMetadataBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupFrameSetupCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_pFrameConstantsBuffer != nullptr)
  {
    commandList.ResolveAndSetConstantBuffer(xiiTempHashedString("xiiFrameConstants"), m_pFrameConstantsBuffer);
  }
}

void xiiRenderDataManager::UploadPerFrameBufferDataCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiRingWriteIndex = m_uiPerFrameUploadWriteIndex;

  const xiiUInt32 uiCameraWriteOffset = uiRingWriteIndex * sizeof(xiiPerFrameCameraUploadData);
  const xiiUInt32 uiLightWriteOffset  = uiRingWriteIndex * sizeof(xiiPerFrameLightUploadData);
  const xiiUInt32 uiGlobalWriteOffset = uiRingWriteIndex * sizeof(xiiPerFrameGlobalUploadData);

  if (m_pPerFrameCameraConstantsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pPerFrameCameraConstantsBuffer, uiCameraWriteOffset, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_PerFrameCameraConstantsSample), sizeof(m_PerFrameCameraConstantsSample))).AssertSuccess();
  }

  if (m_pPerFrameLightDataBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pPerFrameLightDataBuffer, uiLightWriteOffset, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_PerFrameLightDataSample), sizeof(m_PerFrameLightDataSample))).AssertSuccess();
  }

  if (m_pPerFrameGlobalParamsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pPerFrameGlobalParamsBuffer, uiGlobalWriteOffset, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_PerFrameGlobalParamsSample), sizeof(m_PerFrameGlobalParamsSample))).AssertSuccess();
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
