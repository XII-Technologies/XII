#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/CoarseFrustumCullingPass.h>
#include <GraphicsCore/Pipeline/Passes/ContactShadowsPass.h>
#include <GraphicsCore/Pipeline/Passes/DepthPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/DrawCommandBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>
#include <GraphicsCore/Pipeline/Passes/HiZBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/HiZOcclusionCullingPass.h>
#include <GraphicsCore/Pipeline/Passes/InstanceUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/LodSelectionPass.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowRenderPass.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/NormalRoughnessPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/OccluderDepthPass.h>
#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/RayTracedShadowsPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCasterCullingPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCascadeSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowMapRenderPass.h>
#include <GraphicsCore/Pipeline/Passes/SkinningPass.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
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

  struct ShadowCascadeSetupParams
  {
    xiiVec4 m_vSunDirection   = xiiVec4(0.0f, -1.0f, 0.0f, 0.0f);
    xiiVec4 m_vSplitDistances = xiiVec4(10.0f, 30.0f, 80.0f, 200.0f);
  };

  struct DirectionalShadowAtlasParams
  {
    xiiVec4 m_vAtlasPacking = xiiVec4(0.5f, 0.5f, 0.0f, 0.0f);
    xiiVec4 m_vTexelSnap    = xiiVec4(1.0f, 1.0f, 1.0f, 0.0f);
  };

  struct LocalLightShadowAllocatorParams
  {
    xiiVec4 m_vDeterministic = xiiVec4(0.0f, 1.0f, 1024.0f, 1024.0f);
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

  m_pCoarseFrustumCullingPass = XII_DEFAULT_NEW(xiiRenderGraphCoarseFrustumCullingPass);
  m_pCoarseFrustumCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupCoarseFrustumCullingCommandList, this));
  m_pCoarseFrustumCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pOccluderDepthPass = XII_DEFAULT_NEW(xiiRenderGraphOccluderDepthPass);
  m_pOccluderDepthPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupOccluderDepthPrepassCommandList, this));
  m_pOccluderDepthPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawOccluderDepthPrepassCommandList, this));

  m_pHiZBuildPass = XII_DEFAULT_NEW(xiiRenderGraphHiZBuildPass);
  m_pHiZBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupHiZPyramidBuildCommandList, this));
  m_pHiZBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pHiZOcclusionCullingPass = XII_DEFAULT_NEW(xiiRenderGraphHiZOcclusionCullingPass);
  m_pHiZOcclusionCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupHiZOcclusionCullingCommandList, this));
  m_pHiZOcclusionCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pHiZOcclusionCullingPass->SetCandidateInstancesResourceName(xiiMakeHashedString("GpuVisibleCandidates"));
  m_pHiZOcclusionCullingPass->SetCandidateInstanceCountResourceName(xiiMakeHashedString("GpuVisibleCandidateCount"));
  m_pHiZOcclusionCullingPass->SetVisibleInstancesResourceName(xiiMakeHashedString("GpuVisibleInstances"));
  m_pHiZOcclusionCullingPass->SetVisibleInstanceCountResourceName(xiiMakeHashedString("GpuVisibleInstanceCount"));

  m_pDrawCommandBuildPass = XII_DEFAULT_NEW(xiiRenderGraphDrawCommandBuildPass);
  m_pDrawCommandBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDrawIndirectCommandBuildCommandList, this));
  m_pDrawCommandBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pDrawCommandBuildPass->SetVisibleInstancesResourceName(xiiMakeHashedString("GpuVisibleInstances"));
  m_pDrawCommandBuildPass->SetMaterialBinsResourceName(xiiMakeHashedString("GpuMaterialBins"));
  m_pDrawCommandBuildPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pDrawCommandBuildPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));

  m_pMainDepthPrepassPass = XII_DEFAULT_NEW(xiiRenderGraphDepthPrepassPass);
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupMainDepthPrepassCommandList, this));
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawMainDepthPrepassCommandList, this));
  m_pMainDepthPrepassPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pMainDepthPrepassPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));
  m_pMainDepthPrepassPass->SetDepthBufferResourceName(xiiMakeHashedString("SceneDepth"));

  m_pNormalRoughnessPrepassPass = XII_DEFAULT_NEW(xiiRenderGraphNormalRoughnessPrepassPass);
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupNormalRoughnessPrepassCommandList, this));
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawNormalRoughnessPrepassCommandList, this));
  m_pNormalRoughnessPrepassPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pNormalRoughnessPrepassPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));
  m_pNormalRoughnessPrepassPass->SetDepthResourceName(xiiMakeHashedString("SceneDepth"));
  m_pNormalRoughnessPrepassPass->SetNormalRoughnessResourceName(xiiMakeHashedString("SceneNormalRoughness"));

  m_pShadowCascadeSetupPass = XII_DEFAULT_NEW(xiiRenderGraphShadowCascadeSetupPass);
  m_pShadowCascadeSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDirectionalCascadeSetupCommandList, this));
  m_pShadowCascadeSetupPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pShadowCascadeSetupPass->SetCameraDataResourceName(xiiMakeHashedString("FrameConstants"));
  m_pShadowCascadeSetupPass->SetCascadeParamsResourceName(xiiMakeHashedString("ShadowCascadeParams"));
  m_pShadowCascadeSetupPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));

  m_pShadowCasterCullingPass = XII_DEFAULT_NEW(xiiRenderGraphShadowCasterCullingPass);
  m_pShadowCasterCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDirectionalShadowCullingCommandList, this));
  m_pShadowCasterCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pShadowCasterCullingPass->SetInstanceDataResourceName(xiiMakeHashedString("GpuSceneBounds"));
  m_pShadowCasterCullingPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));
  m_pShadowCasterCullingPass->SetShadowVisibleListResourceName(xiiMakeHashedString("ShadowVisibleList"));
  m_pShadowCasterCullingPass->SetShadowVisibleCountResourceName(xiiMakeHashedString("ShadowVisibleCount"));

  m_pLocalLightShadowSetupPass = XII_DEFAULT_NEW(xiiRenderGraphLocalLightShadowSetupPass);
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupLocalLightShadowAtlasAllocationCommandList, this));
  m_pLocalLightShadowSetupPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pLocalLightShadowSetupPass->SetLocalShadowRequestsResourceName(xiiMakeHashedString("LocalShadowRequests"));
  m_pLocalLightShadowSetupPass->SetLocalShadowAllocatorParamsResourceName(xiiMakeHashedString("LocalShadowAllocatorParams"));
  m_pLocalLightShadowSetupPass->SetLocalShadowAtlasPlacementsResourceName(xiiMakeHashedString("LocalShadowAtlasPlacements"));

  m_pContactShadowsPass = XII_DEFAULT_NEW(xiiRenderGraphContactShadowsPass);
  m_pContactShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupContactShadowCommandList, this));
  m_pContactShadowsPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pContactShadowsPass->SetSceneDepthResourceName(xiiMakeHashedString("SceneDepth"));
  m_pContactShadowsPass->SetSceneNormalRoughnessResourceName(xiiMakeHashedString("SceneNormalRoughness"));
  m_pContactShadowsPass->SetLightParamsResourceName(xiiMakeHashedString("ContactShadowLightParams"));
  m_pContactShadowsPass->SetContactShadowTermResourceName(xiiMakeHashedString("ScreenSpaceContactShadowTerm"));

  m_pLocalLightShadowRenderingPass = XII_DEFAULT_NEW(xiiRenderGraphLocalLightShadowRenderPass);
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupSpotAndPointShadowRenderingCommandList, this));
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawSpotAndPointShadowRenderingCommandList, this));
  m_pLocalLightShadowRenderingPass->SetLocalShadowCastersResourceName(xiiMakeHashedString("LocalShadowCasters"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowMaterialBinsResourceName(xiiMakeHashedString("LocalShadowMaterialBins"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowModeBinsResourceName(xiiMakeHashedString("LocalShadowModeBins"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowAtlasPagesResourceName(xiiMakeHashedString("LocalShadowAtlasPages"));

  m_pDirectionalShadowRenderingPass = XII_DEFAULT_NEW(xiiRenderGraphShadowMapRenderPass);
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDirectionalShadowRenderingCommandList, this));
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawDirectionalShadowRenderingCommandList, this));
  m_pDirectionalShadowRenderingPass->SetShadowVisibleListResourceName(xiiMakeHashedString("ShadowVisibleList"));
  m_pDirectionalShadowRenderingPass->SetShadowVisibleCountResourceName(xiiMakeHashedString("ShadowVisibleCount"));
  m_pDirectionalShadowRenderingPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));
  m_pDirectionalShadowRenderingPass->SetShadowDepthAtlasResourceName(xiiMakeHashedString("ShadowDepthAtlas"));

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

void xiiRenderDataManager::AddCoarseFrustumCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pCoarseFrustumCullingPass != nullptr, "Coarse frustum culling pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureInstanceUpdateResources(uiInstanceCount);
  EnsureCoarseFrustumCullingResources(uiInstanceCount);

  if (m_pCameraFrustumPlanesBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderDataManager::AddCoarseFrustumCullingPass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pCameraFrustumPlanesBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(m_vCoarseFrustumPlaneSamples), sizeof(m_vCoarseFrustumPlaneSamples))).AssertSuccess();
  }

  m_pCoarseFrustumCullingPass->SetEnabled(bEnableDispatch);
  m_pCoarseFrustumCullingPass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer);
  }

  if (m_pCameraFrustumPlanesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("CameraFrustumPlanes"), m_pCameraFrustumPlanesBuffer);
  }

  if (m_pGpuVisibleInstancesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleInstances"), m_pGpuVisibleInstancesBuffer);
  }

  if (m_pGpuVisibleInstanceCountBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleInstanceCount"), m_pGpuVisibleInstanceCountBuffer);
  }

  inout_runtime.AddPass(m_pCoarseFrustumCullingPass.Borrow());
}

void xiiRenderDataManager::AddOccluderDepthPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureGpuDrivenVisibilityResources(uiInstanceCount);

  if (m_pOccluderInstanceListBuffer == nullptr)
  {
    m_pOccluderInstanceListBuffer = m_pGpuVisibleInstancesBuffer;
  }

  m_pOccluderDepthPass->SetEnabled(bEnablePass);

  if (m_pOccluderInstanceListBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("OccluderInstances"), m_pOccluderInstanceListBuffer);
  }

  if (m_pOccluderDepthResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("OccluderDepth"), m_pOccluderDepthResource);
  }

  inout_runtime.AddPass(m_pOccluderDepthPass.Borrow());
}

void xiiRenderDataManager::AddHiZPyramidBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pHiZBuildPass != nullptr, "Hi-Z build pass must be initialized.");

  xiiSharedPtr<xiiGALResource> pDepthResource = m_pHiZDepthSourceResource != nullptr ? m_pHiZDepthSourceResource : m_pOccluderDepthResource;

  m_pHiZBuildPass->SetEnabled(bEnableDispatch);
  m_pHiZBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  if (pDepthResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepth"), pDepthResource);
  }

  if (m_pHiZDepthPyramidResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepthPyramid"), m_pHiZDepthPyramidResource);
  }

  inout_runtime.AddPass(m_pHiZBuildPass.Borrow());
}

void xiiRenderDataManager::AddHiZOcclusionCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pHiZOcclusionCullingPass != nullptr, "Hi-Z occlusion culling pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureGpuDrivenVisibilityResources(uiInstanceCount);

  xiiSharedPtr<xiiGALBuffer> pCandidateInstances = m_pHiZOcclusionCandidateInstancesBuffer != nullptr ? m_pHiZOcclusionCandidateInstancesBuffer : m_pGpuVisibleInstancesBuffer;
  xiiSharedPtr<xiiGALBuffer> pCandidateCount     = m_pHiZOcclusionCandidateInstanceCountBuffer != nullptr ? m_pHiZOcclusionCandidateInstanceCountBuffer : m_pGpuVisibleInstanceCountBuffer;

  if (m_pHiZOcclusionVisibleInstancesBuffer == nullptr || m_pHiZOcclusionVisibleInstancesBuffer->GetDescription().m_uiSize < m_pGpuVisibleInstancesBuffer->GetDescription().m_uiSize)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = m_pGpuVisibleInstancesBuffer->GetDescription().m_uiSize;

    m_pHiZOcclusionVisibleInstancesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pHiZOcclusionVisibleInstancesBuffer != nullptr)
    {
      m_pHiZOcclusionVisibleInstancesBuffer->SetDebugName("RenderDataManager::HiZOcclusionVisibleInstances");
    }
  }

  if (m_pHiZOcclusionVisibleInstanceCountBuffer == nullptr)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = sizeof(xiiUInt32);

    m_pHiZOcclusionVisibleInstanceCountBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pHiZOcclusionVisibleInstanceCountBuffer != nullptr)
    {
      m_pHiZOcclusionVisibleInstanceCountBuffer->SetDebugName("RenderDataManager::HiZOcclusionVisibleInstanceCount");
    }
  }

  m_pHiZOcclusionCullingPass->SetEnabled(bEnableDispatch);
  m_pHiZOcclusionCullingPass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  if (pCandidateInstances != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleCandidates"), pCandidateInstances);
  }

  if (pCandidateCount != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleCandidateCount"), pCandidateCount);
  }

  if (m_pHiZDepthPyramidResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepthPyramid"), m_pHiZDepthPyramidResource);
  }

  if (m_pHiZOcclusionVisibleInstancesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleInstances"), m_pHiZOcclusionVisibleInstancesBuffer);
  }

  if (m_pHiZOcclusionVisibleInstanceCountBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleInstanceCount"), m_pHiZOcclusionVisibleInstanceCountBuffer);
  }

  inout_runtime.AddPass(m_pHiZOcclusionCullingPass.Borrow());
}

void xiiRenderDataManager::AddDrawIndirectCommandBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDrawCommandBuildPass != nullptr, "Draw command build pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDrawIndirectCommandBuildResources(uiInstanceCount);

  xiiSharedPtr<xiiGALBuffer> pVisibleInstances = m_pHiZOcclusionVisibleInstancesBuffer != nullptr ? m_pHiZOcclusionVisibleInstancesBuffer : m_pGpuVisibleInstancesBuffer;
  xiiSharedPtr<xiiGALBuffer> pMaterialBins     = m_pGpuMaterialBinsBuffer != nullptr ? m_pGpuMaterialBinsBuffer : m_pGpuDrawMetadataBuffer;

  m_pDrawCommandBuildPass->SetEnabled(bEnableDispatch);
  m_pDrawCommandBuildPass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  if (pVisibleInstances != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuVisibleInstances"), pVisibleInstances);
  }

  if (pMaterialBins != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuMaterialBins"), pMaterialBins);
  }

  if (m_pGpuIndirectDrawCommandsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCommands"), m_pGpuIndirectDrawCommandsBuffer);
  }

  if (m_pGpuIndirectDrawCountsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCounts"), m_pGpuIndirectDrawCountsBuffer);
  }

  inout_runtime.AddPass(m_pDrawCommandBuildPass.Borrow());
}

void xiiRenderDataManager::AddMainDepthPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureMainDepthPrepassResources(uiInstanceCount);

  xiiSharedPtr<xiiGALBuffer>   pIndirectCommands = m_pGpuIndirectDrawCommandsBuffer;
  xiiSharedPtr<xiiGALBuffer>   pIndirectCounts   = m_pGpuIndirectDrawCountsBuffer;
  xiiSharedPtr<xiiGALResource> pSceneDepth       = m_pMainDepthPrepassDepthResource != nullptr ? m_pMainDepthPrepassDepthResource : m_pOccluderDepthResource;

  m_pMainDepthPrepassPass->SetEnabled(bEnablePass);

  if (pIndirectCommands != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCommands"), pIndirectCommands);
  }

  if (pIndirectCounts != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCounts"), pIndirectCounts);
  }

  if (pSceneDepth != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepth"), pSceneDepth);
  }

  inout_runtime.AddPass(m_pMainDepthPrepassPass.Borrow());
}

void xiiRenderDataManager::AddOptionalNormalRoughnessPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureNormalRoughnessPrepassResources(uiInstanceCount);

  xiiSharedPtr<xiiGALBuffer>   pIndirectCommands = m_pGpuIndirectDrawCommandsBuffer;
  xiiSharedPtr<xiiGALBuffer>   pIndirectCounts   = m_pGpuIndirectDrawCountsBuffer;
  xiiSharedPtr<xiiGALResource> pSceneDepth       = m_pNormalRoughnessPrepassDepthResource != nullptr ? m_pNormalRoughnessPrepassDepthResource : m_pMainDepthPrepassDepthResource;

  m_pNormalRoughnessPrepassPass->SetEnabled(bEnablePass);

  if (pIndirectCommands != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCommands"), pIndirectCommands);
  }

  if (pIndirectCounts != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuIndirectDrawCounts"), pIndirectCounts);
  }

  if (pSceneDepth != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepth"), pSceneDepth);
  }

  if (m_pNormalRoughnessPrepassOutputResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneNormalRoughness"), m_pNormalRoughnessPrepassOutputResource);
  }

  inout_runtime.AddPass(m_pNormalRoughnessPrepassPass.Borrow());
}

void xiiRenderDataManager::AddDirectionalCascadeSetupPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pShadowCascadeSetupPass != nullptr, "Shadow cascade setup pass must be initialized.");

  EnsureDirectionalCascadeSetupResources();

  m_pShadowCascadeSetupPass->SetEnabled(bEnableDispatch);
  m_pShadowCascadeSetupPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  if (m_pFrameConstantsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("FrameConstants"), m_pFrameConstantsBuffer);
  }

  if (m_pShadowCascadeParamsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowCascadeParams"), m_pShadowCascadeParamsBuffer);
  }

  if (m_pShadowCascadeDataBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowCascadeData"), m_pShadowCascadeDataBuffer);
  }

  inout_runtime.AddPass(m_pShadowCascadeSetupPass.Borrow());
}

void xiiRenderDataManager::AddDirectionalShadowCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pShadowCasterCullingPass != nullptr, "Directional shadow culling pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDirectionalShadowCullingResources(uiInstanceCount);

  m_pShadowCasterCullingPass->SetEnabled(bEnableDispatch);
  m_pShadowCasterCullingPass->SetDispatchThreadGroupCount(uiInstanceCount, 1U, 1U);

  xiiSharedPtr<xiiGALBuffer> pSceneBounds = m_pShadowCasterCullingSceneBoundsBuffer != nullptr ? m_pShadowCasterCullingSceneBoundsBuffer : m_pGpuSceneBoundsBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData = m_pShadowCasterCullingCascadeDataBuffer != nullptr ? m_pShadowCasterCullingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

  if (pSceneBounds != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GpuSceneBounds"), pSceneBounds);
  }

  if (pCascadeData != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowCascadeData"), pCascadeData);
  }

  if (m_pShadowCasterVisibleListBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowVisibleList"), m_pShadowCasterVisibleListBuffer);
  }

  if (m_pShadowCasterVisibleCountBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowVisibleCount"), m_pShadowCasterVisibleCountBuffer);
  }

  inout_runtime.AddPass(m_pShadowCasterCullingPass.Borrow());
}

void xiiRenderDataManager::AddDirectionalShadowRenderingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDirectionalShadowRenderingResources(uiInstanceCount);

  m_pDirectionalShadowRenderingPass->SetEnabled(bEnablePass);

  xiiSharedPtr<xiiGALBuffer> pVisibleList = m_pDirectionalShadowRenderingVisibleListBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleListBuffer : m_pShadowCasterVisibleListBuffer;
  xiiSharedPtr<xiiGALBuffer> pVisibleCount = m_pDirectionalShadowRenderingVisibleCountBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleCountBuffer : m_pShadowCasterVisibleCountBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData = m_pDirectionalShadowRenderingCascadeDataBuffer != nullptr ? m_pDirectionalShadowRenderingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

  if (pVisibleList != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowVisibleList"), pVisibleList);
  }

  if (pVisibleCount != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowVisibleCount"), pVisibleCount);
  }

  if (pCascadeData != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowCascadeData"), pCascadeData);
  }

  if (m_pDirectionalShadowDepthAtlasResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ShadowDepthAtlas"), m_pDirectionalShadowDepthAtlasResource);
  }

  inout_runtime.AddPass(m_pDirectionalShadowRenderingPass.Borrow());
}

void xiiRenderDataManager::AddLocalLightShadowAtlasAllocationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowSetupPass != nullptr, "Local-light shadow atlas allocation pass must be initialized.");

  const xiiUInt32 uiRequestCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureLocalLightShadowAtlasAllocationResources(uiRequestCount);

  m_pLocalLightShadowSetupPass->SetEnabled(bEnableDispatch);
  m_pLocalLightShadowSetupPass->SetDispatchThreadGroupCount(uiRequestCount, 1U, 1U);

  if (m_pLocalLightShadowRequestsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowRequests"), m_pLocalLightShadowRequestsBuffer);
  }

  if (m_pLocalLightShadowAllocatorParamsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowAllocatorParams"), m_pLocalLightShadowAllocatorParamsBuffer);
  }

  if (m_pLocalLightShadowAtlasPlacementsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowAtlasPlacements"), m_pLocalLightShadowAtlasPlacementsBuffer);
  }

  inout_runtime.AddPass(m_pLocalLightShadowSetupPass.Borrow());
}

void xiiRenderDataManager::AddSpotAndPointShadowRenderingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");

  const xiiUInt32 uiCasterCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureSpotAndPointShadowRenderingResources(uiCasterCount);

  m_pLocalLightShadowRenderingPass->SetEnabled(bEnablePass);

  if (m_pLocalLightShadowCastersBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowCasters"), m_pLocalLightShadowCastersBuffer);
  }

  if (m_pLocalLightShadowMaterialBinsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowMaterialBins"), m_pLocalLightShadowMaterialBinsBuffer);
  }

  if (m_pLocalLightShadowModeBinsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowModeBins"), m_pLocalLightShadowModeBinsBuffer);
  }

  if (m_pLocalShadowAtlasPagesResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("LocalShadowAtlasPages"), m_pLocalShadowAtlasPagesResource);
  }

  inout_runtime.AddPass(m_pLocalLightShadowRenderingPass.Borrow());
}

void xiiRenderDataManager::AddContactShadowPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pContactShadowsPass != nullptr, "Contact shadow pass must be initialized.");

  EnsureContactShadowResources();

  const xiiUInt32 uiDispatchCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  m_pContactShadowsPass->SetEnabled(bEnableDispatch);
  m_pContactShadowsPass->SetDispatchThreadGroupCount(uiDispatchCount, 1U, 1U);

  if (m_pContactShadowDepthResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepth"), m_pContactShadowDepthResource);
  }

  if (m_pContactShadowNormalRoughnessResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneNormalRoughness"), m_pContactShadowNormalRoughnessResource);
  }

  if (m_pContactShadowLightParamsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ContactShadowLightParams"), m_pContactShadowLightParamsBuffer);
  }

  if (m_pContactShadowTermResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ScreenSpaceContactShadowTerm"), m_pContactShadowTermResource);
  }

  inout_runtime.AddPass(m_pContactShadowsPass.Borrow());
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

void xiiRenderDataManager::SetCoarseFrustumPlaneSample(xiiUInt32 uiPlaneIndex, const xiiVec4& vPlane) const
{
  if (uiPlaneIndex >= XII_ARRAY_SIZE(m_vCoarseFrustumPlaneSamples))
  {
    return;
  }

  XII_LOCK(m_Mutex);

  m_vCoarseFrustumPlaneSamples[uiPlaneIndex] = vPlane;
}

void xiiRenderDataManager::SetShadowCascadeSunDirectionSample(const xiiVec4& vSunDirection) const
{
  XII_LOCK(m_Mutex);

  m_vShadowCascadeSunDirectionSample = vSunDirection;
}

void xiiRenderDataManager::SetShadowCascadeSplitDistanceSample(xiiUInt32 uiSplitIndex, float fSplitDistance) const
{
  if (uiSplitIndex >= XII_ARRAY_SIZE(m_fShadowCascadeSplitDistances))
  {
    return;
  }

  XII_LOCK(m_Mutex);

  m_fShadowCascadeSplitDistances[uiSplitIndex] = xiiMath::Max(0.0f, fSplitDistance);
}

void xiiRenderDataManager::SetDirectionalShadowAtlasPackingSample(const xiiVec4& vAtlasPackingSample) const
{
  XII_LOCK(m_Mutex);

  m_vDirectionalShadowAtlasPackingSample = vAtlasPackingSample;
}

void xiiRenderDataManager::SetDirectionalShadowTexelSnapSample(const xiiVec4& vTexelSnapSample) const
{
  XII_LOCK(m_Mutex);

  m_vDirectionalShadowTexelSnapSample = vTexelSnapSample;
}

void xiiRenderDataManager::SetLocalLightShadowAllocatorDeterministicSample(const xiiVec4& vDeterministicSample) const
{
  XII_LOCK(m_Mutex);

  m_vLocalLightShadowAllocatorDeterministicSample = vDeterministicSample;
}

void xiiRenderDataManager::SetContactShadowLightParamsSample(const xiiVec4& vLightParamsSample) const
{
  XII_LOCK(m_Mutex);

  m_vContactShadowLightParamsSample = vLightParamsSample;
}

void xiiRenderDataManager::SetOccluderDepthPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pOccluderDepthResource = pDepthResource;
}

void xiiRenderDataManager::SetOccluderDepthPrepassInstanceListResource(xiiSharedPtr<xiiGALBuffer> pInstanceListResource) const
{
  XII_LOCK(m_Mutex);

  m_pOccluderInstanceListBuffer = pInstanceListResource;
}

void xiiRenderDataManager::SetHiZDepthSourceResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZDepthSourceResource = pDepthResource;
}

void xiiRenderDataManager::SetHiZDepthPyramidResource(xiiSharedPtr<xiiGALResource> pDepthPyramidResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZDepthPyramidResource = pDepthPyramidResource;
}

void xiiRenderDataManager::SetHiZOcclusionCandidateInstancesResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstancesResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZOcclusionCandidateInstancesBuffer = pCandidateInstancesResource;
}

void xiiRenderDataManager::SetHiZOcclusionCandidateInstanceCountResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstanceCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZOcclusionCandidateInstanceCountBuffer = pCandidateInstanceCountResource;
}

void xiiRenderDataManager::SetDrawIndirectMaterialBinsResource(xiiSharedPtr<xiiGALBuffer> pMaterialBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuMaterialBinsBuffer = pMaterialBinsResource;
}

void xiiRenderDataManager::SetDrawIndirectCommandBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCommandBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCommandsBuffer = pIndirectCommandBufferResource;
}

void xiiRenderDataManager::SetDrawIndirectCountBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCountBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCountsBuffer = pIndirectCountBufferResource;
}

void xiiRenderDataManager::SetMainDepthPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pMainDepthPrepassDepthResource = pDepthResource;
}

void xiiRenderDataManager::SetMainDepthPrepassIndirectCommandBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCommandBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCommandsBuffer = pIndirectCommandBufferResource;
}

void xiiRenderDataManager::SetMainDepthPrepassIndirectCountBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCountBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCountsBuffer = pIndirectCountBufferResource;
}

void xiiRenderDataManager::SetNormalRoughnessPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pNormalRoughnessPrepassDepthResource = pDepthResource;
}

void xiiRenderDataManager::SetNormalRoughnessPrepassOutputResource(xiiSharedPtr<xiiGALResource> pNormalRoughnessResource) const
{
  XII_LOCK(m_Mutex);

  m_pNormalRoughnessPrepassOutputResource = pNormalRoughnessResource;
}

void xiiRenderDataManager::SetDirectionalShadowCullingSceneBoundsResource(xiiSharedPtr<xiiGALBuffer> pSceneBoundsResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterCullingSceneBoundsBuffer = pSceneBoundsResource;
}

void xiiRenderDataManager::SetDirectionalShadowCullingCascadeDataResource(xiiSharedPtr<xiiGALBuffer> pCascadeDataResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterCullingCascadeDataBuffer = pCascadeDataResource;
}

void xiiRenderDataManager::SetDirectionalShadowCullingVisibleListResource(xiiSharedPtr<xiiGALBuffer> pVisibleListResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterVisibleListBuffer = pVisibleListResource;
}

void xiiRenderDataManager::SetDirectionalShadowCullingVisibleCountResource(xiiSharedPtr<xiiGALBuffer> pVisibleCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterVisibleCountBuffer = pVisibleCountResource;
}

void xiiRenderDataManager::SetDirectionalShadowDepthAtlasResource(xiiSharedPtr<xiiGALResource> pDepthAtlasResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowDepthAtlasResource = pDepthAtlasResource;
}

void xiiRenderDataManager::SetDirectionalShadowRenderingVisibleListResource(xiiSharedPtr<xiiGALBuffer> pVisibleListResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingVisibleListBuffer = pVisibleListResource;
}

void xiiRenderDataManager::SetDirectionalShadowRenderingVisibleCountResource(xiiSharedPtr<xiiGALBuffer> pVisibleCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingVisibleCountBuffer = pVisibleCountResource;
}

void xiiRenderDataManager::SetDirectionalShadowRenderingCascadeDataResource(xiiSharedPtr<xiiGALBuffer> pCascadeDataResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingCascadeDataBuffer = pCascadeDataResource;
}

void xiiRenderDataManager::SetLocalLightShadowRequestsResource(xiiSharedPtr<xiiGALBuffer> pRequestsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowRequestsBuffer = pRequestsResource;
}

void xiiRenderDataManager::SetLocalLightShadowAtlasPlacementsResource(xiiSharedPtr<xiiGALBuffer> pPlacementsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowAtlasPlacementsBuffer = pPlacementsResource;
}

void xiiRenderDataManager::SetLocalLightShadowCastersResource(xiiSharedPtr<xiiGALBuffer> pCastersResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowCastersBuffer = pCastersResource;
}

void xiiRenderDataManager::SetLocalLightShadowMaterialBinsResource(xiiSharedPtr<xiiGALBuffer> pMaterialBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowMaterialBinsBuffer = pMaterialBinsResource;
}

void xiiRenderDataManager::SetLocalLightShadowModeBinsResource(xiiSharedPtr<xiiGALBuffer> pModeBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowModeBinsBuffer = pModeBinsResource;
}

void xiiRenderDataManager::SetLocalLightShadowAtlasPagesResource(xiiSharedPtr<xiiGALResource> pAtlasPagesResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalShadowAtlasPagesResource = pAtlasPagesResource;
}

void xiiRenderDataManager::SetContactShadowDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowDepthResource = pDepthResource;
}

void xiiRenderDataManager::SetContactShadowNormalRoughnessResource(xiiSharedPtr<xiiGALResource> pNormalRoughnessResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowNormalRoughnessResource = pNormalRoughnessResource;
}

void xiiRenderDataManager::SetContactShadowLightParamsResource(xiiSharedPtr<xiiGALBuffer> pLightParamsResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowLightParamsBuffer = pLightParamsResource;
}

void xiiRenderDataManager::SetContactShadowOutputResource(xiiSharedPtr<xiiGALResource> pContactShadowTermResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowTermResource = pContactShadowTermResource;
}

void xiiRenderDataManager::SetOccluderDepthPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetOccluderDepthPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderDataManager::ClearOccluderDepthPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupOccluderDepthPrepassCommandList, this));
}

void xiiRenderDataManager::ClearOccluderDepthPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawOccluderDepthPrepassCommandList, this));
}

void xiiRenderDataManager::SetMainDepthPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetMainDepthPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderDataManager::ClearMainDepthPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupMainDepthPrepassCommandList, this));
}

void xiiRenderDataManager::ClearMainDepthPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawMainDepthPrepassCommandList, this));
}

void xiiRenderDataManager::SetNormalRoughnessPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetNormalRoughnessPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderDataManager::ClearNormalRoughnessPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupNormalRoughnessPrepassCommandList, this));
}

void xiiRenderDataManager::ClearNormalRoughnessPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawNormalRoughnessPrepassCommandList, this));
}

void xiiRenderDataManager::SetDirectionalShadowRenderingSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetDirectionalShadowRenderingDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(drawFunc);
}

void xiiRenderDataManager::ClearDirectionalShadowRenderingSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupDirectionalShadowRenderingCommandList, this));
}

void xiiRenderDataManager::ClearDirectionalShadowRenderingDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawDirectionalShadowRenderingCommandList, this));
}

void xiiRenderDataManager::SetLocalLightShadowAtlasAllocationSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowSetupPass != nullptr, "Local-light shadow atlas allocation pass must be initialized.");
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::ClearLocalLightShadowAtlasAllocationSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowSetupPass != nullptr, "Local-light shadow atlas allocation pass must be initialized.");
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupLocalLightShadowAtlasAllocationCommandList, this));
}

void xiiRenderDataManager::SetSpotAndPointShadowRenderingSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::SetSpotAndPointShadowRenderingDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(drawFunc);
}

void xiiRenderDataManager::ClearSpotAndPointShadowRenderingSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupSpotAndPointShadowRenderingCommandList, this));
}

void xiiRenderDataManager::ClearSpotAndPointShadowRenderingDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::DrawSpotAndPointShadowRenderingCommandList, this));
}

void xiiRenderDataManager::SetContactShadowSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pContactShadowsPass != nullptr, "Contact shadow pass must be initialized.");
  m_pContactShadowsPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderDataManager::ClearContactShadowSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pContactShadowsPass != nullptr, "Contact shadow pass must be initialized.");
  m_pContactShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderDataManager::SetupContactShadowCommandList, this));
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

void xiiRenderDataManager::EnsureCoarseFrustumCullingResources(xiiUInt32 uiElementCount) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureGpuDrivenVisibilityResources(xiiMath::Max(1U, uiElementCount));

  const xiiUInt64 uiPlaneBufferSize = 6ULL * sizeof(xiiVec4);
  if (m_pCameraFrustumPlanesBuffer == nullptr || m_pCameraFrustumPlanesBuffer->GetDescription().m_uiSize < uiPlaneBufferSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiPlaneBufferSize;

    m_pCameraFrustumPlanesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pCameraFrustumPlanesBuffer != nullptr)
    {
      m_pCameraFrustumPlanesBuffer->SetDebugName("RenderDataManager::CameraFrustumPlanes");
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

void xiiRenderDataManager::EnsureDrawIndirectCommandBuildResources(xiiUInt32 uiDrawCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureLodSelectionResources(xiiMath::Max(1U, uiDrawCapacity));

  if (m_pGpuMaterialBinsBuffer == nullptr)
  {
    m_pGpuMaterialBinsBuffer = m_pGpuDrawMetadataBuffer;
  }

  const xiiUInt32 uiResolvedDrawCapacity = xiiMath::Max(1U, uiDrawCapacity);
  const xiiUInt64 uiCommandBufferSize    = static_cast<xiiUInt64>(uiResolvedDrawCapacity) * sizeof(xiiUInt32) * 4ULL;

  if (m_pGpuIndirectDrawCommandsBuffer == nullptr || m_pGpuIndirectDrawCommandsBuffer->GetDescription().m_uiSize < uiCommandBufferSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Undefined;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = 0U;
    bufferDescription.m_uiSize              = uiCommandBufferSize;

    m_pGpuIndirectDrawCommandsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuIndirectDrawCommandsBuffer != nullptr)
    {
      m_pGpuIndirectDrawCommandsBuffer->SetDebugName("RenderDataManager::GpuIndirectDrawCommands");
    }
  }

  if (m_pGpuIndirectDrawCountsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Undefined;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = 0U;
    bufferDescription.m_uiSize              = sizeof(xiiUInt32);

    m_pGpuIndirectDrawCountsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pGpuIndirectDrawCountsBuffer != nullptr)
    {
      m_pGpuIndirectDrawCountsBuffer->SetDebugName("RenderDataManager::GpuIndirectDrawCounts");
    }
  }
}

void xiiRenderDataManager::EnsureMainDepthPrepassResources(xiiUInt32 uiInstanceCapacity) const
{
  EnsureDrawIndirectCommandBuildResources(xiiMath::Max(1U, uiInstanceCapacity));

  if (m_pMainDepthPrepassDepthResource == nullptr)
  {
    m_pMainDepthPrepassDepthResource = m_pOccluderDepthResource;
  }
}

void xiiRenderDataManager::EnsureNormalRoughnessPrepassResources(xiiUInt32 uiInstanceCapacity) const
{
  EnsureMainDepthPrepassResources(xiiMath::Max(1U, uiInstanceCapacity));

  if (m_pNormalRoughnessPrepassDepthResource == nullptr)
  {
    m_pNormalRoughnessPrepassDepthResource = m_pMainDepthPrepassDepthResource != nullptr ? m_pMainDepthPrepassDepthResource : m_pOccluderDepthResource;
  }
}

void xiiRenderDataManager::EnsureDirectionalCascadeSetupResources() const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureFrameSetupResources();

  if (m_pShadowCascadeParamsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = 2U * sizeof(xiiVec4);

    m_pShadowCascadeParamsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pShadowCascadeParamsBuffer != nullptr)
    {
      m_pShadowCascadeParamsBuffer->SetDebugName("RenderDataManager::ShadowCascadeParams");
    }
  }

  if (m_pShadowCascadeDataBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = 8U * sizeof(xiiVec4);

    m_pShadowCascadeDataBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pShadowCascadeDataBuffer != nullptr)
    {
      m_pShadowCascadeDataBuffer->SetDebugName("RenderDataManager::ShadowCascadeData");
    }
  }
}

void xiiRenderDataManager::EnsureDirectionalShadowCullingResources(xiiUInt32 uiInstanceCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedInstanceCapacity = xiiMath::Max(1U, uiInstanceCapacity);

  if (m_pShadowCasterCullingSceneBoundsBuffer == nullptr)
  {
    EnsureInstanceUpdateResources(uiResolvedInstanceCapacity);
  }

  if (m_pShadowCasterCullingCascadeDataBuffer == nullptr)
  {
    EnsureDirectionalCascadeSetupResources();
  }

  const xiiUInt64 uiVisibleListSize = static_cast<xiiUInt64>(uiResolvedInstanceCapacity) * 4ULL * sizeof(xiiUInt32);
  if (m_pShadowCasterVisibleListBuffer == nullptr || m_pShadowCasterVisibleListBuffer->GetDescription().m_uiSize < uiVisibleListSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiVisibleListSize;

    m_pShadowCasterVisibleListBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pShadowCasterVisibleListBuffer != nullptr)
    {
      m_pShadowCasterVisibleListBuffer->SetDebugName("RenderDataManager::ShadowVisibleList");
    }
  }

  const xiiUInt64 uiVisibleCountSize = 4ULL * sizeof(xiiUInt32);
  if (m_pShadowCasterVisibleCountBuffer == nullptr || m_pShadowCasterVisibleCountBuffer->GetDescription().m_uiSize < uiVisibleCountSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiVisibleCountSize;

    m_pShadowCasterVisibleCountBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pShadowCasterVisibleCountBuffer != nullptr)
    {
      m_pShadowCasterVisibleCountBuffer->SetDebugName("RenderDataManager::ShadowVisibleCount");
    }
  }
}

void xiiRenderDataManager::EnsureDirectionalShadowRenderingResources(xiiUInt32 uiInstanceCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureDirectionalShadowCullingResources(xiiMath::Max(1U, uiInstanceCapacity));

  if (m_pDirectionalShadowRenderingVisibleListBuffer == nullptr)
  {
    m_pDirectionalShadowRenderingVisibleListBuffer = m_pShadowCasterVisibleListBuffer;
  }

  if (m_pDirectionalShadowRenderingVisibleCountBuffer == nullptr)
  {
    m_pDirectionalShadowRenderingVisibleCountBuffer = m_pShadowCasterVisibleCountBuffer;
  }

  if (m_pDirectionalShadowRenderingCascadeDataBuffer == nullptr)
  {
    m_pDirectionalShadowRenderingCascadeDataBuffer = m_pShadowCascadeDataBuffer;
  }

  if (m_pDirectionalShadowAtlasParamsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = 2U * sizeof(xiiVec4);

    m_pDirectionalShadowAtlasParamsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDirectionalShadowAtlasParamsBuffer != nullptr)
    {
      m_pDirectionalShadowAtlasParamsBuffer->SetDebugName("RenderDataManager::DirectionalShadowAtlasParams");
    }
  }
}

void xiiRenderDataManager::EnsureLocalLightShadowAtlasAllocationResources(xiiUInt32 uiRequestCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedRequestCapacity = xiiMath::Max(1U, uiRequestCapacity);

  if (m_pLocalLightShadowRequestsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiResolvedRequestCapacity) * sizeof(xiiVec4);

    m_pLocalLightShadowRequestsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowRequestsBuffer != nullptr)
    {
      m_pLocalLightShadowRequestsBuffer->SetDebugName("RenderDataManager::LocalShadowRequests");
    }
  }
  else if (m_pLocalLightShadowRequestsBuffer->GetDescription().m_uiSize < static_cast<xiiUInt64>(uiResolvedRequestCapacity) * sizeof(xiiVec4))
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiResolvedRequestCapacity) * sizeof(xiiVec4);

    m_pLocalLightShadowRequestsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowRequestsBuffer != nullptr)
    {
      m_pLocalLightShadowRequestsBuffer->SetDebugName("RenderDataManager::LocalShadowRequests");
    }
  }

  const xiiUInt64 uiPlacementsSize = static_cast<xiiUInt64>(uiResolvedRequestCapacity) * sizeof(xiiVec4);
  if (m_pLocalLightShadowAtlasPlacementsBuffer == nullptr || m_pLocalLightShadowAtlasPlacementsBuffer->GetDescription().m_uiSize < uiPlacementsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiPlacementsSize;

    m_pLocalLightShadowAtlasPlacementsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowAtlasPlacementsBuffer != nullptr)
    {
      m_pLocalLightShadowAtlasPlacementsBuffer->SetDebugName("RenderDataManager::LocalShadowAtlasPlacements");
    }
  }

  if (m_pLocalLightShadowAllocatorParamsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = sizeof(xiiVec4);

    m_pLocalLightShadowAllocatorParamsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowAllocatorParamsBuffer != nullptr)
    {
      m_pLocalLightShadowAllocatorParamsBuffer->SetDebugName("RenderDataManager::LocalShadowAllocatorParams");
    }
  }
}

void xiiRenderDataManager::EnsureSpotAndPointShadowRenderingResources(xiiUInt32 uiCasterCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedCasterCapacity = xiiMath::Max(1U, uiCasterCapacity);

  EnsureLocalLightShadowAtlasAllocationResources(uiResolvedCasterCapacity);

  if (m_pLocalLightShadowCastersBuffer == nullptr)
  {
    m_pLocalLightShadowCastersBuffer = m_pLocalLightShadowAtlasPlacementsBuffer;
  }

  const xiiUInt64 uiBinsSize = static_cast<xiiUInt64>(uiResolvedCasterCapacity) * sizeof(xiiUInt32);

  if (m_pLocalLightShadowMaterialBinsBuffer == nullptr || m_pLocalLightShadowMaterialBinsBuffer->GetDescription().m_uiSize < uiBinsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiBinsSize;

    m_pLocalLightShadowMaterialBinsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowMaterialBinsBuffer != nullptr)
    {
      m_pLocalLightShadowMaterialBinsBuffer->SetDebugName("RenderDataManager::LocalShadowMaterialBins");
    }
  }

  if (m_pLocalLightShadowModeBinsBuffer == nullptr || m_pLocalLightShadowModeBinsBuffer->GetDescription().m_uiSize < uiBinsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiBinsSize;

    m_pLocalLightShadowModeBinsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pLocalLightShadowModeBinsBuffer != nullptr)
    {
      m_pLocalLightShadowModeBinsBuffer->SetDebugName("RenderDataManager::LocalShadowModeBins");
    }
  }
}

void xiiRenderDataManager::EnsureContactShadowResources() const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  if (m_pContactShadowDepthResource == nullptr)
  {
    m_pContactShadowDepthResource = m_pMainDepthPrepassDepthResource != nullptr ? m_pMainDepthPrepassDepthResource : m_pOccluderDepthResource;
  }

  if (m_pContactShadowNormalRoughnessResource == nullptr)
  {
    m_pContactShadowNormalRoughnessResource = m_pNormalRoughnessPrepassOutputResource;
  }

  if (m_pContactShadowLightParamsBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = sizeof(xiiVec4);

    m_pContactShadowLightParamsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pContactShadowLightParamsBuffer != nullptr)
    {
      m_pContactShadowLightParamsBuffer->SetDebugName("RenderDataManager::ContactShadowLightParams");
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

  auto EnsureUploadBuffer = [&](xiiSharedPtr<xiiGALBuffer>& inout_pBuffer, xiiStringView sDebugName, xiiUInt32 uiStructSize) {
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

void xiiRenderDataManager::SetupCoarseFrustumCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureInstanceUpdateResources(uiInstanceCount);
  EnsureCoarseFrustumCullingResources(uiInstanceCount);

  if (m_hCoarseFrustumCullingShader.IsValid() == false)
  {
    m_hCoarseFrustumCullingShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/CoarseFrustumCulling.xiiShader");
  }

  if (m_pCoarseFrustumCullingPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hCoarseFrustumCullingShader, s_PermutationVars, true);
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

    m_pCoarseFrustumCullingPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pCoarseFrustumCullingPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pCoarseFrustumCullingPipelineState);

  if (m_pGpuSceneBoundsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneBounds"), m_pGpuSceneBoundsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pCameraFrustumPlanesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("CameraFrustumPlanes"), m_pCameraFrustumPlanesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
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

void xiiRenderDataManager::SetupOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  xiiSharedPtr<xiiGALBuffer> pOccluderInstanceList = m_pOccluderInstanceListBuffer != nullptr ? m_pOccluderInstanceListBuffer : m_pGpuVisibleInstancesBuffer;
  if (pOccluderInstanceList != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("OccluderInstances"), pOccluderInstanceList->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }
}

void xiiRenderDataManager::DrawOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::SetupHiZPyramidBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_hHiZBuildShader.IsValid() == false)
  {
    m_hHiZBuildShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/HiZBuild.xiiShader");
  }

  if (m_pHiZBuildPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hHiZBuildShader, s_PermutationVars, true);
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

    m_pHiZBuildPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pHiZBuildPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pHiZBuildPipelineState);

  xiiSharedPtr<xiiGALResource> pDepthResource = m_pHiZDepthSourceResource != nullptr ? m_pHiZDepthSourceResource : m_pOccluderDepthResource;
  if (pDepthResource != nullptr)
  {
    if (xiiGALTexture* pDepthTexture = xiiDynamicCast<xiiGALTexture*>(pDepthResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneDepth"), pDepthTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pHiZDepthPyramidResource != nullptr)
  {
    if (xiiGALTexture* pDepthPyramidTexture = xiiDynamicCast<xiiGALTexture*>(m_pHiZDepthPyramidResource.Borrow()))
    {
      commandList.ResolveAndSetUnorderedAccessTextureView(xiiTempHashedString("SceneDepthPyramid"), pDepthPyramidTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    }
  }
}

void xiiRenderDataManager::SetupHiZOcclusionCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_hHiZOcclusionCullingShader.IsValid() == false)
  {
    m_hHiZOcclusionCullingShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/HiZOcclusionCulling.xiiShader");
  }

  if (m_pHiZOcclusionCullingPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hHiZOcclusionCullingShader, s_PermutationVars, true);
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

    m_pHiZOcclusionCullingPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pHiZOcclusionCullingPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pHiZOcclusionCullingPipelineState);

  xiiSharedPtr<xiiGALBuffer> pCandidateInstances = m_pHiZOcclusionCandidateInstancesBuffer != nullptr ? m_pHiZOcclusionCandidateInstancesBuffer : m_pGpuVisibleInstancesBuffer;
  xiiSharedPtr<xiiGALBuffer> pCandidateCount     = m_pHiZOcclusionCandidateInstanceCountBuffer != nullptr ? m_pHiZOcclusionCandidateInstanceCountBuffer : m_pGpuVisibleInstanceCountBuffer;

  if (pCandidateInstances != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuVisibleCandidates"), pCandidateInstances->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (pCandidateCount != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuVisibleCandidateCount"), pCandidateCount->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pHiZDepthPyramidResource != nullptr)
  {
    if (xiiGALTexture* pDepthPyramidTexture = xiiDynamicCast<xiiGALTexture*>(m_pHiZDepthPyramidResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneDepthPyramid"), pDepthPyramidTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pHiZOcclusionVisibleInstancesBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuVisibleInstances"), m_pHiZOcclusionVisibleInstancesBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pHiZOcclusionVisibleInstanceCountBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuVisibleInstanceCount"), m_pHiZOcclusionVisibleInstanceCountBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupDrawIndirectCommandBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDrawIndirectCommandBuildResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_hDrawCommandBuildShader.IsValid() == false)
  {
    m_hDrawCommandBuildShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DrawCommandBuild.xiiShader");
  }

  if (m_pDrawCommandBuildPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDrawCommandBuildShader, s_PermutationVars, true);
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

    m_pDrawCommandBuildPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pDrawCommandBuildPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pDrawCommandBuildPipelineState);

  xiiSharedPtr<xiiGALBuffer> pVisibleInstances = m_pHiZOcclusionVisibleInstancesBuffer != nullptr ? m_pHiZOcclusionVisibleInstancesBuffer : m_pGpuVisibleInstancesBuffer;
  xiiSharedPtr<xiiGALBuffer> pMaterialBins     = m_pGpuMaterialBinsBuffer != nullptr ? m_pGpuMaterialBinsBuffer : m_pGpuDrawMetadataBuffer;

  if (pVisibleInstances != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuVisibleInstances"), pVisibleInstances->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (pMaterialBins != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuMaterialBins"), pMaterialBins->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pGpuIndirectDrawCommandsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuIndirectDrawCommands"), m_pGpuIndirectDrawCommandsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pGpuIndirectDrawCountsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("GpuIndirectDrawCounts"), m_pGpuIndirectDrawCountsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupMainDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureMainDepthPrepassResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_pGpuIndirectDrawCommandsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuIndirectDrawCommands"), m_pGpuIndirectDrawCommandsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (m_pGpuIndirectDrawCountsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuIndirectDrawCounts"), m_pGpuIndirectDrawCountsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }
}

void xiiRenderDataManager::DrawMainDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::SetupNormalRoughnessPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureNormalRoughnessPrepassResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_pGpuIndirectDrawCommandsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuIndirectDrawCommands"), m_pGpuIndirectDrawCommandsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (m_pGpuIndirectDrawCountsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuIndirectDrawCounts"), m_pGpuIndirectDrawCountsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  xiiSharedPtr<xiiGALResource> pSceneDepth = m_pNormalRoughnessPrepassDepthResource != nullptr ? m_pNormalRoughnessPrepassDepthResource : m_pMainDepthPrepassDepthResource;
  if (pSceneDepth != nullptr)
  {
    if (xiiGALTexture* pDepthTexture = xiiDynamicCast<xiiGALTexture*>(pSceneDepth.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneDepth"), pDepthTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    }
  }
}

void xiiRenderDataManager::DrawNormalRoughnessPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::SetupDirectionalCascadeSetupCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDirectionalCascadeSetupResources();

  if (m_hShadowCascadeSetupShader.IsValid() == false)
  {
    m_hShadowCascadeSetupShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ShadowCascadeSetup.xiiShader");
  }

  if (m_pShadowCascadeSetupPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hShadowCascadeSetupShader, s_PermutationVars, true);
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

    m_pShadowCascadeSetupPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pShadowCascadeSetupPipelineState == nullptr)
  {
    return;
  }

  ShadowCascadeSetupParams params;
  params.m_vSunDirection = m_vShadowCascadeSunDirectionSample;
  params.m_vSplitDistances.Set(m_fShadowCascadeSplitDistances[0], m_fShadowCascadeSplitDistances[1], m_fShadowCascadeSplitDistances[2], m_fShadowCascadeSplitDistances[3]);

  if (m_pShadowCascadeParamsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pShadowCascadeParamsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&params), sizeof(params))).AssertSuccess();
  }

  commandList.SetPipelineState(m_pShadowCascadeSetupPipelineState);

  if (m_pFrameConstantsBuffer != nullptr)
  {
    commandList.ResolveAndSetConstantBuffer(xiiTempHashedString("xiiFrameConstants"), m_pFrameConstantsBuffer);
  }

  if (m_pShadowCascadeParamsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowCascadeParams"), m_pShadowCascadeParamsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pShadowCascadeDataBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ShadowCascadeData"), m_pShadowCascadeDataBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupDirectionalShadowCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDirectionalShadowCullingResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_hShadowCasterCullingShader.IsValid() == false)
  {
    m_hShadowCasterCullingShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ShadowCasterCulling.xiiShader");
  }

  if (m_pShadowCasterCullingPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hShadowCasterCullingShader, s_PermutationVars, true);
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

    m_pShadowCasterCullingPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pShadowCasterCullingPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pShadowCasterCullingPipelineState);

  xiiSharedPtr<xiiGALBuffer> pSceneBounds = m_pShadowCasterCullingSceneBoundsBuffer != nullptr ? m_pShadowCasterCullingSceneBoundsBuffer : m_pGpuSceneBoundsBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData = m_pShadowCasterCullingCascadeDataBuffer != nullptr ? m_pShadowCasterCullingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

  if (pSceneBounds != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("GpuSceneBounds"), pSceneBounds->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (pCascadeData != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowCascadeData"), pCascadeData->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pShadowCasterVisibleListBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ShadowVisibleList"), m_pShadowCasterVisibleListBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pShadowCasterVisibleCountBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ShadowVisibleCount"), m_pShadowCasterVisibleCountBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupDirectionalShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDirectionalShadowRenderingResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  DirectionalShadowAtlasParams atlasParams;
  atlasParams.m_vAtlasPacking = m_vDirectionalShadowAtlasPackingSample;
  atlasParams.m_vTexelSnap    = m_vDirectionalShadowTexelSnapSample;

  if (m_pDirectionalShadowAtlasParamsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pDirectionalShadowAtlasParamsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&atlasParams), sizeof(atlasParams))).AssertSuccess();
  }

  xiiSharedPtr<xiiGALBuffer> pVisibleList = m_pDirectionalShadowRenderingVisibleListBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleListBuffer : m_pShadowCasterVisibleListBuffer;
  xiiSharedPtr<xiiGALBuffer> pVisibleCount = m_pDirectionalShadowRenderingVisibleCountBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleCountBuffer : m_pShadowCasterVisibleCountBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData = m_pDirectionalShadowRenderingCascadeDataBuffer != nullptr ? m_pDirectionalShadowRenderingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

  if (pVisibleList != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowVisibleList"), pVisibleList->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (pVisibleCount != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowVisibleCount"), pVisibleCount->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (pCascadeData != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowCascadeData"), pCascadeData->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (m_pDirectionalShadowAtlasParamsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ShadowAtlasParams"), m_pDirectionalShadowAtlasParamsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }
}

void xiiRenderDataManager::DrawDirectionalShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::SetupLocalLightShadowAtlasAllocationCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiRequestCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureLocalLightShadowAtlasAllocationResources(uiRequestCount);

  if (m_hLocalLightShadowSetupShader.IsValid() == false)
  {
    m_hLocalLightShadowSetupShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LocalLightShadowAtlasAllocation.xiiShader");
  }

  if (m_pLocalLightShadowSetupPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hLocalLightShadowSetupShader, s_PermutationVars, true);
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

    m_pLocalLightShadowSetupPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pLocalLightShadowSetupPipelineState == nullptr)
  {
    return;
  }

  LocalLightShadowAllocatorParams allocatorParams;
  allocatorParams.m_vDeterministic = m_vLocalLightShadowAllocatorDeterministicSample;

  if (m_pLocalLightShadowAllocatorParamsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pLocalLightShadowAllocatorParamsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&allocatorParams), sizeof(allocatorParams))).AssertSuccess();
  }

  commandList.SetPipelineState(m_pLocalLightShadowSetupPipelineState);

  if (m_pLocalLightShadowRequestsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("LocalShadowRequests"), m_pLocalLightShadowRequestsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pLocalLightShadowAllocatorParamsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("LocalShadowAllocatorParams"), m_pLocalLightShadowAllocatorParamsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pLocalLightShadowAtlasPlacementsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("LocalShadowAtlasPlacements"), m_pLocalLightShadowAtlasPlacementsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderDataManager::SetupSpotAndPointShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureSpotAndPointShadowRenderingResources(xiiMath::Max(1U, m_GpuDrivenInstances.GetCount()));

  if (m_pLocalLightShadowCastersBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("LocalShadowCasters"), m_pLocalLightShadowCastersBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (m_pLocalLightShadowMaterialBinsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("LocalShadowMaterialBins"), m_pLocalLightShadowMaterialBinsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }

  if (m_pLocalLightShadowModeBinsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("LocalShadowModeBins"), m_pLocalLightShadowModeBinsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }
}

void xiiRenderDataManager::DrawSpotAndPointShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderDataManager::SetupContactShadowCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureContactShadowResources();

  if (m_hContactShadowsShader.IsValid() == false)
  {
    m_hContactShadowsShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ContactShadows.xiiShader");
  }

  if (m_pContactShadowsPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hContactShadowsShader, s_PermutationVars, true);
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

    m_pContactShadowsPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pContactShadowsPipelineState == nullptr)
  {
    return;
  }

  if (m_pContactShadowLightParamsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pContactShadowLightParamsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vContactShadowLightParamsSample), sizeof(m_vContactShadowLightParamsSample))).AssertSuccess();
  }

  commandList.SetPipelineState(m_pContactShadowsPipelineState);

  if (m_pContactShadowDepthResource != nullptr)
  {
    if (xiiGALTexture* pDepthTexture = xiiDynamicCast<xiiGALTexture*>(m_pContactShadowDepthResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneDepth"), pDepthTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pContactShadowNormalRoughnessResource != nullptr)
  {
    if (xiiGALTexture* pNormalTexture = xiiDynamicCast<xiiGALTexture*>(m_pContactShadowNormalRoughnessResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneNormalRoughness"), pNormalTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pContactShadowLightParamsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ContactShadowLightParams"), m_pContactShadowLightParamsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pContactShadowTermResource != nullptr)
  {
    if (xiiGALTexture* pContactTermTexture = xiiDynamicCast<xiiGALTexture*>(m_pContactShadowTermResource.Borrow()))
    {
      commandList.ResolveAndSetUnorderedAccessTextureView(xiiTempHashedString("ScreenSpaceContactShadowTerm"), pContactTermTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    }
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
