#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
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
}

XII_IMPLEMENT_WORLD_MODULE(xiiRenderDataManager);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderDataManager, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgCustomInstanceDataOffsetChanged, 1, xiiRTTIDefaultAllocator<xiiMsgCustomInstanceDataOffsetChanged>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderDataManager::xiiRenderDataManager(xiiWorld* pWorld)
  : xiiWorldModule(pWorld)
{
  xiiRenderWorld::GetExtractionEvent().AddEventHandler(xiiMakeDelegate(&xiiRenderDataManager::OnExtractionEvent, this));

  m_pGpuDrivenVisibilityPass = XII_DEFAULT_NEW(xiiRenderGraphGpuVisibilityPass);
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupGpuDrivenVisibilityCommandList, this));

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

  out_pBuffer = nullptr;
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

  out_pBuffer = nullptr;
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
    const xiiUInt32 uiThreadGroupSize = xiiMath::Max(1U, m_uiGpuVisibilityThreadGroupSize);
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
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::IndirectDrawArguments;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = sizeof(GpuDrivenDispatchArguments);

    m_pGpuVisibilityDispatchArgumentsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuVisibilityDispatchArgumentsBuffer != nullptr)
    {
      m_pGpuVisibilityDispatchArgumentsBuffer->SetDebugName("RenderDataManager::GpuVisibilityDispatchArguments");
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

    const xiiShaderPermutationResourceHandle hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hGpuDrivenVisibilityShader, s_PermutationVars, true);
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
    pipelineDescription.m_pComputeShader            = pComputeShader;

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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderDataManager);
