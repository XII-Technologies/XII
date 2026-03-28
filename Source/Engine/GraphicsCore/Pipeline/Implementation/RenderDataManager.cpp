#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/Passes/ClusterGridBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/CoarseFrustumCullingPass.h>
#include <GraphicsCore/Pipeline/Passes/ContactShadowsPass.h>
#include <GraphicsCore/Pipeline/Passes/DecalResolvePass.h>
#include <GraphicsCore/Pipeline/Passes/DepthPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/DrawCommandBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/GpuDrivenVisibilityPass.h>
#include <GraphicsCore/Pipeline/Passes/HiZBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/HiZOcclusionCullingPass.h>
#include <GraphicsCore/Pipeline/Passes/InstanceUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/LightListBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowRenderPass.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/LodSelectionPass.h>
#include <GraphicsCore/Pipeline/Passes/NormalRoughnessPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/OccluderDepthPass.h>
#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/RayTracedShadowsPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCascadeSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCasterCullingPass.h>
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

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgCustomInstanceDataOffsetChanged, 1, xiiRTTIDefaultAllocator<xiiMsgCustomInstanceDataOffsetChanged>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) : xiiWorldModule(pWorld)
{
  xiiRenderWorld::GetExtractionEvent().AddEventHandler(xiiMakeDelegate(&xiiRenderWorldModule::OnExtractionEvent, this));

  // Frame-setup is now registered via the render-graph builder; no module-owned pass object.

  m_pGpuDrivenVisibilityPass = XII_DEFAULT_NEW(xiiRenderGraphGpuVisibilityPass);
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupGpuDrivenVisibilityCommandList, this));
  m_pGpuDrivenVisibilityPass->SetPostDispatchCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::OnGpuDrivenVisibilityPostDispatch, this));
  m_bGpuVisibilityUseInternalIndirectDispatch = true;

  m_pInstanceUpdatePass = XII_DEFAULT_NEW(xiiRenderGraphInstanceUpdatePass);
  m_pInstanceUpdatePass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupInstanceUpdateCommandList, this));
  m_pInstanceUpdatePass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pCoarseFrustumCullingPass = XII_DEFAULT_NEW(xiiRenderGraphCoarseFrustumCullingPass);
  m_pCoarseFrustumCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupCoarseFrustumCullingCommandList, this));
  m_pCoarseFrustumCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pOccluderDepthPass = XII_DEFAULT_NEW(xiiRenderGraphOccluderDepthPass);
  m_pOccluderDepthPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupOccluderDepthPrepassCommandList, this));
  m_pOccluderDepthPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawOccluderDepthPrepassCommandList, this));

  m_pHiZBuildPass = XII_DEFAULT_NEW(xiiRenderGraphHiZBuildPass);
  m_pHiZBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupHiZPyramidBuildCommandList, this));
  m_pHiZBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pHiZOcclusionCullingPass = XII_DEFAULT_NEW(xiiRenderGraphHiZOcclusionCullingPass);
  m_pHiZOcclusionCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupHiZOcclusionCullingCommandList, this));
  m_pHiZOcclusionCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pHiZOcclusionCullingPass->SetCandidateInstancesResourceName(xiiMakeHashedString("GpuVisibleCandidates"));
  m_pHiZOcclusionCullingPass->SetCandidateInstanceCountResourceName(xiiMakeHashedString("GpuVisibleCandidateCount"));
  m_pHiZOcclusionCullingPass->SetVisibleInstancesResourceName(xiiMakeHashedString("GpuVisibleInstances"));
  m_pHiZOcclusionCullingPass->SetVisibleInstanceCountResourceName(xiiMakeHashedString("GpuVisibleInstanceCount"));

  m_pDrawCommandBuildPass = XII_DEFAULT_NEW(xiiRenderGraphDrawCommandBuildPass);
  m_pDrawCommandBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDrawIndirectCommandBuildCommandList, this));
  m_pDrawCommandBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pDrawCommandBuildPass->SetVisibleInstancesResourceName(xiiMakeHashedString("GpuVisibleInstances"));
  m_pDrawCommandBuildPass->SetMaterialBinsResourceName(xiiMakeHashedString("GpuMaterialBins"));
  m_pDrawCommandBuildPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pDrawCommandBuildPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));

  m_pMainDepthPrepassPass = XII_DEFAULT_NEW(xiiRenderGraphDepthPrepassPass);
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupMainDepthPrepassCommandList, this));
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawMainDepthPrepassCommandList, this));
  m_pMainDepthPrepassPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pMainDepthPrepassPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));
  m_pMainDepthPrepassPass->SetDepthBufferResourceName(xiiMakeHashedString("SceneDepth"));

  m_pNormalRoughnessPrepassPass = XII_DEFAULT_NEW(xiiRenderGraphNormalRoughnessPrepassPass);
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupNormalRoughnessPrepassCommandList, this));
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawNormalRoughnessPrepassCommandList, this));
  m_pNormalRoughnessPrepassPass->SetIndirectCommandBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCommands"));
  m_pNormalRoughnessPrepassPass->SetIndirectCountBufferResourceName(xiiMakeHashedString("GpuIndirectDrawCounts"));
  m_pNormalRoughnessPrepassPass->SetDepthResourceName(xiiMakeHashedString("SceneDepth"));
  m_pNormalRoughnessPrepassPass->SetNormalRoughnessResourceName(xiiMakeHashedString("SceneNormalRoughness"));

  m_pShadowCascadeSetupPass = XII_DEFAULT_NEW(xiiRenderGraphShadowCascadeSetupPass);
  m_pShadowCascadeSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDirectionalCascadeSetupCommandList, this));
  m_pShadowCascadeSetupPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pShadowCascadeSetupPass->SetCameraDataResourceName(xiiMakeHashedString("FrameConstants"));
  m_pShadowCascadeSetupPass->SetCascadeParamsResourceName(xiiMakeHashedString("ShadowCascadeParams"));
  m_pShadowCascadeSetupPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));

  m_pShadowCasterCullingPass = XII_DEFAULT_NEW(xiiRenderGraphShadowCasterCullingPass);
  m_pShadowCasterCullingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDirectionalShadowCullingCommandList, this));
  m_pShadowCasterCullingPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pShadowCasterCullingPass->SetInstanceDataResourceName(xiiMakeHashedString("GpuSceneBounds"));
  m_pShadowCasterCullingPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));
  m_pShadowCasterCullingPass->SetShadowVisibleListResourceName(xiiMakeHashedString("ShadowVisibleList"));
  m_pShadowCasterCullingPass->SetShadowVisibleCountResourceName(xiiMakeHashedString("ShadowVisibleCount"));

  m_pLocalLightShadowSetupPass = XII_DEFAULT_NEW(xiiRenderGraphLocalLightShadowSetupPass);
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupLocalLightShadowAtlasAllocationCommandList, this));
  m_pLocalLightShadowSetupPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pLocalLightShadowSetupPass->SetLocalShadowRequestsResourceName(xiiMakeHashedString("LocalShadowRequests"));
  m_pLocalLightShadowSetupPass->SetLocalShadowAllocatorParamsResourceName(xiiMakeHashedString("LocalShadowAllocatorParams"));
  m_pLocalLightShadowSetupPass->SetLocalShadowAtlasPlacementsResourceName(xiiMakeHashedString("LocalShadowAtlasPlacements"));

  m_pContactShadowsPass = XII_DEFAULT_NEW(xiiRenderGraphContactShadowsPass);
  m_pContactShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupContactShadowCommandList, this));
  m_pContactShadowsPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pContactShadowsPass->SetSceneDepthResourceName(xiiMakeHashedString("SceneDepth"));
  m_pContactShadowsPass->SetSceneNormalRoughnessResourceName(xiiMakeHashedString("SceneNormalRoughness"));
  m_pContactShadowsPass->SetLightParamsResourceName(xiiMakeHashedString("ContactShadowLightParams"));
  m_pContactShadowsPass->SetContactShadowTermResourceName(xiiMakeHashedString("ScreenSpaceContactShadowTerm"));

  m_pClusterGridBuildPass = XII_DEFAULT_NEW(xiiRenderGraphClusterGridBuildPass);
  m_pClusterGridBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupClusterGridBuildCommandList, this));
  m_pClusterGridBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pClusterGridBuildPass->SetCameraFrustumResourceName(xiiMakeHashedString("ClusterCameraFrustum"));
  m_pClusterGridBuildPass->SetDepthRangeResourceName(xiiMakeHashedString("ClusterDepthRange"));
  m_pClusterGridBuildPass->SetClusterDescriptorsResourceName(xiiMakeHashedString("ClusterDescriptors"));

  m_pLightListBuildPass = XII_DEFAULT_NEW(xiiRenderGraphLightListBuildPass);
  m_pLightListBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupLightListConstructionCommandList, this));
  m_pLightListBuildPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pLightListBuildPass->SetVisibleLightsResourceName(xiiMakeHashedString("VisibleLightList"));
  m_pLightListBuildPass->SetClusterGridResourceName(xiiMakeHashedString("ClusterDescriptors"));
  m_pLightListBuildPass->SetClusterDepthInfoResourceName(xiiMakeHashedString("ClusterDepthRange"));
  m_pLightListBuildPass->SetClusterLightIndicesResourceName(xiiMakeHashedString("ClusterLightIndices"));
  m_pLightListBuildPass->SetClusterLightPrefixSumsResourceName(xiiMakeHashedString("ClusterLightPrefixSums"));

  m_pDecalClassificationPass = XII_DEFAULT_NEW(xiiRenderGraphDecalResolvePass);
  m_pDecalClassificationPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDecalClassificationCommandList, this));
  m_pDecalClassificationPass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pDecalClassificationPass->SetDecalVolumesResourceName(xiiMakeHashedString("DecalVolumes"));
  m_pDecalClassificationPass->SetSceneDepthResourceName(xiiMakeHashedString("SceneDepth"));
  m_pDecalClassificationPass->SetDecalTileListsResourceName(xiiMakeHashedString("DecalTileLists"));

  m_pDecalResolvePass = XII_DEFAULT_NEW(xiiRenderGraphDecalResolvePass);
  m_pDecalResolvePass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDecalResolveCommandList, this));
  m_pDecalResolvePass->SetDispatchThreadGroupCount(1U, 1U, 1U);
  m_pDecalResolvePass->SetGBufferTargetsResourceName(xiiMakeHashedString("GBufferTargets"));
  m_pDecalResolvePass->SetDecalTileListsInputResourceName(xiiMakeHashedString("DecalTileLists"));
  m_pDecalResolvePass->SetUpdatedMaterialAttributesResourceName(xiiMakeHashedString("UpdatedMaterialAttributes"));

  m_pLocalLightShadowRenderingPass = XII_DEFAULT_NEW(xiiRenderGraphLocalLightShadowRenderPass);
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupSpotAndPointShadowRenderingCommandList, this));
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawSpotAndPointShadowRenderingCommandList, this));
  m_pLocalLightShadowRenderingPass->SetLocalShadowCastersResourceName(xiiMakeHashedString("LocalShadowCasters"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowMaterialBinsResourceName(xiiMakeHashedString("LocalShadowMaterialBins"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowModeBinsResourceName(xiiMakeHashedString("LocalShadowModeBins"));
  m_pLocalLightShadowRenderingPass->SetLocalShadowAtlasPagesResourceName(xiiMakeHashedString("LocalShadowAtlasPages"));

  m_pDirectionalShadowRenderingPass = XII_DEFAULT_NEW(xiiRenderGraphShadowMapRenderPass);
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDirectionalShadowRenderingCommandList, this));
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawDirectionalShadowRenderingCommandList, this));
  m_pDirectionalShadowRenderingPass->SetShadowVisibleListResourceName(xiiMakeHashedString("ShadowVisibleList"));
  m_pDirectionalShadowRenderingPass->SetShadowVisibleCountResourceName(xiiMakeHashedString("ShadowVisibleCount"));
  m_pDirectionalShadowRenderingPass->SetShadowCascadeDataResourceName(xiiMakeHashedString("ShadowCascadeData"));
  m_pDirectionalShadowRenderingPass->SetShadowDepthAtlasResourceName(xiiMakeHashedString("ShadowDepthAtlas"));

  m_pLodSelectionPass = XII_DEFAULT_NEW(xiiRenderGraphLodSelectionPass);
  m_pLodSelectionPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupLodSelectionCommandList, this));
  m_pLodSelectionPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pRayTracedShadowsPass = XII_DEFAULT_NEW(xiiRenderGraphRayTracedShadowsPass);
  m_pRayTracedShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupRayTracedShadowsCommandList, this));
  m_pRayTracedShadowsPass->SetDispatchRayTracingFunc(xiiMakeDelegate(&xiiRenderWorldModule::DispatchRayTracedShadowsCommandList, this));

  // Dynamic-resolution is handled via builder-style registration now; no module-owned pass object.

  m_pSkinningPass = XII_DEFAULT_NEW(xiiRenderGraphSkinningPass);
  m_pSkinningPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupSkinningAndMorphCommandList, this));
  m_pSkinningPass->SetDispatchThreadGroupCount(1U, 1U, 1U);

  m_pPerFrameBufferUploadPass = XII_DEFAULT_NEW(xiiRenderGraphPerFrameBufferUploadPass);
  m_pPerFrameBufferUploadPass->SetUploadCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::UploadPerFrameBufferDataCommandList, this));

  // Keep indices stable for callers that expect static/dynamic/skinning slots.
  m_Buffers.SetCount(3);
}

xiiRenderWorldModule::~xiiRenderWorldModule()
{
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(xiiMakeDelegate(&xiiRenderWorldModule::OnExtractionEvent, this));
}

void xiiRenderWorldModule::Initialize()
{
}

void xiiRenderWorldModule::DeleteAllCachedRenderData()
{
  xiiRenderWorld::DeleteAllCachedRenderData();
}

void xiiRenderWorldModule::DeleteCachedRenderData(const xiiGameObjectHandle& hOwnerObject, const xiiComponentHandle& hOwnerComponent)
{
  xiiRenderWorld::DeleteCachedRenderData(hOwnerObject, hOwnerComponent);
}

void xiiRenderWorldModule::DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pOwnerObject)
{
  xiiRenderWorld::DeleteCachedRenderDataForObjectRecursive(pOwnerObject);
}

void xiiRenderWorldModule::ResetRenderDataCache(xiiView& ref_view)
{
  xiiRenderWorld::ResetRenderDataCache(ref_view);
}

xiiArrayPtr<xiiPerInstanceData> xiiRenderWorldModule::GetOrCreateInstanceData(const xiiComponent* pOwnerComponent, bool bDynamic, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount /*= 1*/) const
{
  XII_IGNORE_UNUSED(pOwnerComponent);
  XII_IGNORE_UNUSED(bDynamic);

  static thread_local xiiDynamicArray<xiiPerInstanceData> s_InstanceData;
  s_InstanceData.SetCount(uiCount);

  out_pBuffer                         = nullptr;
  inout_instanceDataOffset.m_uiOffset = 0;

  return s_InstanceData;
}

void xiiRenderWorldModule::DeleteInstanceData(xiiInstanceDataOffset& inout_instanceDataOffset) const
{
  inout_instanceDataOffset = {};
}

xiiUInt32 xiiRenderWorldModule::RegisterCustomInstanceData(const xiiGALBufferCreationDescription& desc, xiiStringView sDebugName, xiiDelegate<void()> beforeUploadCallback /*= {}*/)
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

xiiByteArrayPtr xiiRenderWorldModule::GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiStructByteSize, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount) const
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  XII_IGNORE_UNUSED(pOwnerComponent);

  static thread_local xiiDynamicArray<xiiUInt8> s_CustomData;
  s_CustomData.SetCountUninitialized(uiStructByteSize * uiCount);

  out_pBuffer                         = nullptr;
  inout_instanceDataOffset.m_uiOffset = 0;

  return xiiByteArrayPtr(s_CustomData.GetData(), s_CustomData.GetCount());
}

void xiiRenderWorldModule::DeleteCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  inout_instanceDataOffset = {};
}

void xiiRenderWorldModule::CompactCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiMaxSteps)
{
  XII_IGNORE_UNUSED(uiCustomDataIndex);
  XII_IGNORE_UNUSED(uiMaxSteps);
}

xiiArrayPtr<xiiShaderTransform> xiiRenderWorldModule::GetOrCreateSkinningData(const xiiComponent* pOwnerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiNumTransforms) const
{
  XII_IGNORE_UNUSED(pOwnerComponent);

  static thread_local xiiDynamicArray<xiiShaderTransform> s_SkinningData;
  s_SkinningData.SetCount(uiNumTransforms);

  inout_instanceDataOffset.m_uiOffset = 0;
  return s_SkinningData;
}

xiiArrayPtr<const xiiShaderTransform> xiiRenderWorldModule::GetSkinningData(const xiiCustomInstanceDataOffset& instanceDataOffset) const
{
  XII_IGNORE_UNUSED(instanceDataOffset);

  static thread_local xiiDynamicArray<xiiShaderTransform> s_Empty;
  return s_Empty;
}

void xiiRenderWorldModule::DeleteSkinningData(xiiCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  inout_instanceDataOffset = {};
}

xiiSharedPtr<xiiGALDynamicBuffer> xiiRenderWorldModule::GetSkinningDataBuffer() const
{
  if (m_Buffers.GetCount() > s_uiSkinningBufferIndex)
  {
    return m_Buffers[s_uiSkinningBufferIndex];
  }

  return nullptr;
}

void xiiRenderWorldModule::BeginGpuDrivenBuild()
{
  XII_LOCK(m_Mutex);

  m_GpuDrivenInstances.Clear();
  m_GpuDrivenVisibleInstanceIndices.Clear();
}

void xiiRenderWorldModule::AddGpuDrivenInstance(const xiiTransform& globalTransform, const xiiBoundingSphere& bounds, xiiUInt32 uiMeshId, xiiUInt32 uiMaterialId, xiiUInt32 uiFlags /*= 0U*/)
{
  XII_LOCK(m_Mutex);

  xiiGpuDrivenInstance& instance = m_GpuDrivenInstances.ExpandAndGetRef();
  instance.m_ObjectToWorld       = globalTransform.GetAsMat4();
  instance.m_Bounds              = bounds;
  instance.m_uiMeshId            = uiMeshId;
  instance.m_uiMaterialId        = uiMaterialId;
  instance.m_uiFlags             = uiFlags;
}

void xiiRenderWorldModule::EndGpuDrivenBuild()
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

  auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderWorldModule::EndGpuDrivenBuild");
  if (!m_GpuDrivenInstances.IsEmpty())
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pGpuSceneInstancesBuffer, 0U, xiiMakeArrayPtr(m_GpuDrivenInstances.GetData(), m_GpuDrivenInstances.GetCount()).ToByteArray()).AssertSuccess();
  }

  if (m_pGpuVisibilityDispatchArgumentsBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pGpuVisibilityDispatchArgumentsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&dispatchArguments), sizeof(dispatchArguments))).AssertSuccess();
  }
}

xiiArrayPtr<const xiiGpuDrivenInstance> xiiRenderWorldModule::GetGpuDrivenInstances() const
{
  XII_LOCK(m_Mutex);

  static thread_local xiiDynamicArray<xiiGpuDrivenInstance> s_GpuDrivenInstancesSnapshot;
  s_GpuDrivenInstancesSnapshot = m_GpuDrivenInstances;
  return s_GpuDrivenInstancesSnapshot;
}

void xiiRenderWorldModule::SetGpuDrivenVisibleInstanceIndices(xiiArrayPtr<const xiiUInt32> visibleInstanceIndices)
{
  XII_LOCK(m_Mutex);

  m_GpuDrivenVisibleInstanceIndices.SetCountUninitialized(visibleInstanceIndices.GetCount());
  if (!visibleInstanceIndices.IsEmpty())
  {
    xiiMemoryUtils::Copy(m_GpuDrivenVisibleInstanceIndices.GetData(), visibleInstanceIndices.GetPtr(), visibleInstanceIndices.GetCount());
  }
}

xiiArrayPtr<const xiiUInt32> xiiRenderWorldModule::GetGpuDrivenVisibleInstanceIndices() const
{
  XII_LOCK(m_Mutex);

  static thread_local xiiDynamicArray<xiiUInt32> s_GpuDrivenVisibleInstanceIndicesSnapshot;
  s_GpuDrivenVisibleInstanceIndicesSnapshot = m_GpuDrivenVisibleInstanceIndices;
  return s_GpuDrivenVisibleInstanceIndicesSnapshot;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderWorldModule::GetGpuDrivenSceneInstancesBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuSceneInstancesBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderWorldModule::GetGpuDrivenVisibleInstancesBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuVisibleInstancesBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderWorldModule::GetGpuDrivenVisibleInstanceCountBuffer() const
{
  XII_LOCK(m_Mutex);
  return m_pGpuVisibleInstanceCountBuffer;
}

bool xiiRenderWorldModule::TryGetGpuDrivenVisibleInstanceCountReadback(xiiUInt32& out_uiVisibleInstanceCount, bool bWaitForCompletion /*= false*/) const
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

  auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Transfer>("xiiRenderWorldModule::TryGetGpuDrivenVisibleInstanceCountReadback");

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

void xiiRenderWorldModule::AddGpuDrivenVisibilityPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::SetGpuDrivenVisibilityThreadGroupSize(xiiUInt32 uiThreadGroupSize) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_uiGpuVisibilityThreadGroupSize = xiiMath::Max(1U, uiThreadGroupSize);
  m_pGpuDrivenVisibilityPass->SetThreadGroupSize(m_uiGpuVisibilityThreadGroupSize);
}

void xiiRenderWorldModule::SetGpuDrivenVisibilityDirectDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY /*= 1U*/, xiiUInt32 uiThreadGroupCountZ /*= 1U*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->SetDirectDispatchThreadGroupCount(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void xiiRenderWorldModule::SetGpuDrivenVisibilityIndirectDispatchArguments(xiiSharedPtr<xiiGALBuffer> pIndirectDispatchArguments, xiiUInt64 uiDispatchArgumentOffset /*= 0U*/, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode /*= xiiGALStateTransitionMode::Transition*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_bGpuVisibilityUseInternalIndirectDispatch = false;
  m_pGpuDrivenVisibilityPass->SetIndirectDispatchArguments(pIndirectDispatchArguments, uiDispatchArgumentOffset, bufferTransitionMode);
}

void xiiRenderWorldModule::SetGpuDrivenVisibilityUseInternalIndirectDispatch(bool bEnable) const
{
  XII_LOCK(m_Mutex);

  m_bGpuVisibilityUseInternalIndirectDispatch = bEnable;
}

void xiiRenderWorldModule::SetGpuDrivenVisibilitySetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearGpuDrivenVisibilitySetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pGpuDrivenVisibilityPass != nullptr, "GPU-driven visibility pass must be initialized.");
  m_pGpuDrivenVisibilityPass->ClearSetupCommandListFunc();
}

void xiiRenderWorldModule::AddRayTracedShadowsPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetEnabled(bEnableDispatch);
  inout_runtime.AddPass(m_pRayTracedShadowsPass.Borrow());
}

void xiiRenderWorldModule::AddFrameSetupPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= true*/) const
{
  XII_LOCK(m_Mutex);

  EnsureFrameSetupResources();

  if (m_pPreviousFrameStatsBuffer != nullptr)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (pCommandList)
    {
      pCommandList->Begin();
      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList.Borrow(), m_pPreviousFrameStatsBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_PreviousFrameStatsSample), sizeof(m_PreviousFrameStatsSample))).AssertSuccess();
      pCommandList->End();

      xiiGALCommandQueue* pCommandQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_ASSERT_DEBUG(pCommandQueue != nullptr, "Failed to get command queue for the specified flags!");
      pCommandQueue->Submit(pCommandList);
    }
  }
  // Keep legacy enabled flag behavior preserved via registration; no module-owned pass object.

  // Builder-style registration using typed pass data.
  struct FrameSetupData
  {
    xiiRGBufferHandle m_hPreviousFrameStats;
    xiiRGBufferHandle m_hFrameConstants;
    xiiRGBufferHandle m_hFrameTiming;
    xiiRGBufferHandle m_hFrameTimestampRanges;
  };

  auto [pData, hPass] = inout_runtime.AddPass<FrameSetupData>(
    xiiMakeHashedString("FrameSetup"),
    xiiGALCommandQueueFlags::Graphics,
    [this](FrameSetupData& data, xiiRGBuilder& builder)
    {
      if (m_pPreviousFrameStatsBuffer != nullptr)
      {
        data.m_hPreviousFrameStats = builder.ImportBuffer(xiiMakeHashedString("PreviousFrameStats"), m_pPreviousFrameStatsBuffer, xiiGALResourceStateFlags::ShaderResource);
        builder.ReadBuffer(data.m_hPreviousFrameStats, xiiGALResourceStateFlags::ShaderResource);
      }

      if (m_pFrameConstantsBuffer != nullptr)
      {
        data.m_hFrameConstants = builder.ImportBuffer(xiiMakeHashedString("FrameConstants"), m_pFrameConstantsBuffer, xiiGALResourceStateFlags::ConstantBuffer);
        builder.WriteBuffer(data.m_hFrameConstants, xiiGALResourceStateFlags::ConstantBuffer);
      }

      if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
      {
        data.m_hFrameTiming = builder.ImportBuffer(xiiMakeHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer, xiiGALResourceStateFlags::ShaderResource);
        builder.WriteBuffer(data.m_hFrameTiming, xiiGALResourceStateFlags::ShaderResource);
      }

      if (m_pFrameTimestampRangesBuffer != nullptr)
      {
        data.m_hFrameTimestampRanges = builder.ImportBuffer(xiiMakeHashedString("FrameTimestampRanges"), m_pFrameTimestampRangesBuffer, xiiGALResourceStateFlags::UnorderedAccess);
        builder.WriteBuffer(data.m_hFrameTimestampRanges, xiiGALResourceStateFlags::UnorderedAccess);
      }

      builder.SetPassSideEffects(true);
    },
    [this](const FrameSetupData& data, xiiRGPassContext& ctx)
    {
      xiiGALCommandList& commandList = ctx.GetCommandList();

      // Mirror prior setup: bind the frame constants constant buffer.
      if (m_pFrameConstantsBuffer != nullptr)
      {
        commandList.ResolveAndSetConstantBuffer(xiiTempHashedString("xiiFrameConstants"), m_pFrameConstantsBuffer);
      }

      // The original FrameSetup pass allowed a setup callback to run on the command list
      // to bind additional resources. That behavior is preserved via the module's
      // SetupFrameSetupCommandList which previously was invoked by the legacy pass.
      // Since that function only binds the constant buffer, we have already replicated
      // its behavior here.
    },
    /*bHasSideEffects=*/true
  );
}

void xiiRenderWorldModule::AddDynamicResolutionPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= true*/) const
{
  XII_LOCK(m_Mutex);

  EnsureDynamicResolutionResources(1U);

  if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderWorldModule::AddDynamicResolutionPass");
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pDynamicResolutionFrameTimingBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vDynamicResolutionFrameTimingSample), sizeof(m_vDynamicResolutionFrameTimingSample))).AssertSuccess();

    if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
    {
      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandListScope.GetCommandList().Borrow(), m_pDynamicResolutionCameraVelocityBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vDynamicResolutionCameraVelocitySample), sizeof(m_vDynamicResolutionCameraVelocitySample))).AssertSuccess();
    }
  }

  // Builder-style registration: declare imported buffers and record commands via existing setup function.
  struct DynamicResolutionData
  {
    xiiRGBufferHandle m_hFrameTiming;
    xiiRGBufferHandle m_hDynamicResolutionData;
    xiiRGBufferHandle m_hCameraVelocity;
  };

  auto [pData, hPass] = inout_runtime.AddPass<DynamicResolutionData>(
    xiiMakeHashedString("DynamicResolution"),
    xiiGALCommandQueueFlags::Compute,
    [this](DynamicResolutionData& data, xiiRGBuilder& builder)
    {
      if (m_pDynamicResolutionFrameTimingBuffer != nullptr)
      {
        data.m_hFrameTiming = builder.ImportBuffer(xiiMakeHashedString("FrameTimingData"), m_pDynamicResolutionFrameTimingBuffer, xiiGALResourceStateFlags::ShaderResource);
        builder.ReadBuffer(data.m_hFrameTiming, xiiGALResourceStateFlags::ShaderResource);
      }

      if (m_pDynamicResolutionBuffer != nullptr)
      {
        data.m_hDynamicResolutionData = builder.ImportBuffer(xiiMakeHashedString("DynamicResolutionData"), m_pDynamicResolutionBuffer, xiiGALResourceStateFlags::UnorderedAccess);
        builder.WriteBuffer(data.m_hDynamicResolutionData, xiiGALResourceStateFlags::UnorderedAccess);
      }

      if (m_pDynamicResolutionCameraVelocityBuffer != nullptr)
      {
        data.m_hCameraVelocity = builder.ImportBuffer(xiiMakeHashedString("CameraVelocityData"), m_pDynamicResolutionCameraVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
        builder.ReadBuffer(data.m_hCameraVelocity, xiiGALResourceStateFlags::ShaderResource);
      }
    },
    [this](const DynamicResolutionData& data, xiiRGPassContext& ctx)
    {
      xiiGALCommandList& commandList = ctx.GetCommandList();

      // Reuse the existing setup implementation to bind pipeline and views.
      SetupDynamicResolutionCommandList(commandList, ctx);

      // Dispatch a single workgroup by default (legacy default was 1,1,1).
      // If more advanced thread-group computation is required, migrate that logic here.
      commandList.Dispatch(1U, 1U, 1U);
    }
  );
}

void xiiRenderWorldModule::AddSkinningAndMorphPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pSkinningPass != nullptr, "Skinning pass must be initialized.");

  const xiiUInt32 uiDeformerElementCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureSkinningAndMorphResources(uiDeformerElementCount);

  if (m_pSkinningInputBuffer != nullptr || m_pMorphWeightsBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderWorldModule::AddSkinningAndMorphPass");
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

void xiiRenderWorldModule::AddInstanceTransformAndBoundsUpdatePass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pInstanceUpdatePass != nullptr, "Instance update pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureSkinningAndMorphResources(uiInstanceCount);
  EnsureInstanceUpdateResources(uiInstanceCount);

  if (m_pSceneTransformsBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderWorldModule::AddInstanceTransformAndBoundsUpdatePass");
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

void xiiRenderWorldModule::AddCoarseFrustumCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pCoarseFrustumCullingPass != nullptr, "Coarse frustum culling pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureInstanceUpdateResources(uiInstanceCount);
  EnsureCoarseFrustumCullingResources(uiInstanceCount);

  if (m_pCameraFrustumPlanesBuffer != nullptr)
  {
    auto pCommandListScope = xiiRenderContext::BeginCommandListScope<xiiRenderContext::CommandListType::Compute>("xiiRenderWorldModule::AddCoarseFrustumCullingPass");
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

void xiiRenderWorldModule::AddOccluderDepthPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
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

void xiiRenderWorldModule::AddHiZPyramidBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddHiZOcclusionCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddDrawIndirectCommandBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddMainDepthPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
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

void xiiRenderWorldModule::AddOptionalNormalRoughnessPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
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

void xiiRenderWorldModule::AddDirectionalCascadeSetupPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddDirectionalShadowCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddDirectionalShadowRenderingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");

  const xiiUInt32 uiInstanceCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDirectionalShadowRenderingResources(uiInstanceCount);

  m_pDirectionalShadowRenderingPass->SetEnabled(bEnablePass);

  xiiSharedPtr<xiiGALBuffer> pVisibleList  = m_pDirectionalShadowRenderingVisibleListBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleListBuffer : m_pShadowCasterVisibleListBuffer;
  xiiSharedPtr<xiiGALBuffer> pVisibleCount = m_pDirectionalShadowRenderingVisibleCountBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleCountBuffer : m_pShadowCasterVisibleCountBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData  = m_pDirectionalShadowRenderingCascadeDataBuffer != nullptr ? m_pDirectionalShadowRenderingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

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

void xiiRenderWorldModule::AddLocalLightShadowAtlasAllocationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddSpotAndPointShadowRenderingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass /*= false*/) const
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

void xiiRenderWorldModule::AddContactShadowPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddClusterGridBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pClusterGridBuildPass != nullptr, "Cluster-grid build pass must be initialized.");

  const xiiUInt32 uiClusterCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureClusterGridBuildResources(uiClusterCount);

  m_pClusterGridBuildPass->SetEnabled(bEnableDispatch);
  m_pClusterGridBuildPass->SetDispatchThreadGroupCount(uiClusterCount, 1U, 1U);

  xiiSharedPtr<xiiGALBuffer> pCameraFrustum = m_pClusterCameraFrustumBuffer != nullptr ? m_pClusterCameraFrustumBuffer : m_pCameraFrustumPlanesBuffer;

  if (pCameraFrustum != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterCameraFrustum"), pCameraFrustum);
  }

  if (m_pClusterDepthRangeBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterDepthRange"), m_pClusterDepthRangeBuffer);
  }

  if (m_pClusterDescriptorsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterDescriptors"), m_pClusterDescriptorsBuffer);
  }

  inout_runtime.AddPass(m_pClusterGridBuildPass.Borrow());
}

void xiiRenderWorldModule::AddLightListConstructionPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLightListBuildPass != nullptr, "Light-list construction pass must be initialized.");

  const xiiUInt32 uiClusterCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  const xiiUInt32 uiLightCount   = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureLightListConstructionResources(uiClusterCount, uiLightCount);

  m_pLightListBuildPass->SetEnabled(bEnableDispatch);
  m_pLightListBuildPass->SetDispatchThreadGroupCount(uiClusterCount, 1U, 1U);

  if (m_pVisibleLightListBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("VisibleLightList"), m_pVisibleLightListBuffer);
  }

  if (m_pClusterDescriptorsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterDescriptors"), m_pClusterDescriptorsBuffer);
  }

  if (m_pClusterDepthRangeBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterDepthRange"), m_pClusterDepthRangeBuffer);
  }

  if (m_pClusterLightIndicesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterLightIndices"), m_pClusterLightIndicesBuffer);
  }

  if (m_pClusterLightPrefixSumsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("ClusterLightPrefixSums"), m_pClusterLightPrefixSumsBuffer);
  }

  inout_runtime.AddPass(m_pLightListBuildPass.Borrow());
}

void xiiRenderWorldModule::AddDecalClassificationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalClassificationPass != nullptr, "Decal classification pass must be initialized.");

  const xiiUInt32 uiTileCount  = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  const xiiUInt32 uiDecalCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDecalClassificationResources(uiTileCount, uiDecalCount);

  m_pDecalClassificationPass->SetEnabled(bEnableDispatch);
  m_pDecalClassificationPass->SetDispatchThreadGroupCount(uiTileCount, 1U, 1U);

  if (m_pDecalVolumesBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("DecalVolumes"), m_pDecalVolumesBuffer);
  }

  if (m_pDecalClassificationDepthResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("SceneDepth"), m_pDecalClassificationDepthResource);
  }

  if (m_pDecalTileListsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("DecalTileLists"), m_pDecalTileListsBuffer);
  }

  inout_runtime.AddPass(m_pDecalClassificationPass.Borrow());
}

void xiiRenderWorldModule::AddDecalResolvePass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalResolvePass != nullptr, "Decal resolve pass must be initialized.");

  EnsureDecalResolveResources();

  const xiiUInt32 uiTileCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  m_pDecalResolvePass->SetEnabled(bEnableDispatch);
  m_pDecalResolvePass->SetDispatchThreadGroupCount(uiTileCount, 1U, 1U);

  if (m_pDecalResolveGBufferResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("GBufferTargets"), m_pDecalResolveGBufferResource);
  }

  if (m_pDecalResolveTileListsBuffer != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("DecalTileLists"), m_pDecalResolveTileListsBuffer);
  }

  if (m_pDecalResolveOutputResource != nullptr)
  {
    inout_runtime.SetResource(xiiMakeHashedString("UpdatedMaterialAttributes"), m_pDecalResolveOutputResource);
  }

  inout_runtime.AddPass(m_pDecalResolvePass.Borrow());
}

void xiiRenderWorldModule::AddLodSelectionAndMeshletClassificationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch /*= false*/) const
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

void xiiRenderWorldModule::AddPerFrameBufferUploadPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableUploads /*= true*/) const
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

void xiiRenderWorldModule::SetDynamicResolutionFrameTimingSample(const xiiVec4& vFrameTimingSample) const
{
  XII_LOCK(m_Mutex);

  m_vDynamicResolutionFrameTimingSample = vFrameTimingSample;
}

void xiiRenderWorldModule::SetDynamicResolutionCameraVelocitySample(const xiiVec4& vCameraVelocitySample) const
{
  XII_LOCK(m_Mutex);

  m_vDynamicResolutionCameraVelocitySample = vCameraVelocitySample;
}

void xiiRenderWorldModule::SetSkinningInputSample(const xiiVec4& vSkinningInputSample) const
{
  XII_LOCK(m_Mutex);

  m_vSkinningInputSample = vSkinningInputSample;
}

void xiiRenderWorldModule::SetMorphWeightsSample(const xiiVec4& vMorphWeightsSample) const
{
  XII_LOCK(m_Mutex);

  m_vMorphWeightsSample = vMorphWeightsSample;
}

void xiiRenderWorldModule::SetSceneTransformsSample(const xiiShaderTransform& sceneTransformSample) const
{
  XII_LOCK(m_Mutex);

  m_SceneTransformsSample = sceneTransformSample;
}

void xiiRenderWorldModule::SetCoarseFrustumPlaneSample(xiiUInt32 uiPlaneIndex, const xiiVec4& vPlane) const
{
  if (uiPlaneIndex >= XII_ARRAY_SIZE(m_vCoarseFrustumPlaneSamples))
  {
    return;
  }

  XII_LOCK(m_Mutex);

  m_vCoarseFrustumPlaneSamples[uiPlaneIndex] = vPlane;
}

void xiiRenderWorldModule::SetShadowCascadeSunDirectionSample(const xiiVec4& vSunDirection) const
{
  XII_LOCK(m_Mutex);

  m_vShadowCascadeSunDirectionSample = vSunDirection;
}

void xiiRenderWorldModule::SetShadowCascadeSplitDistanceSample(xiiUInt32 uiSplitIndex, float fSplitDistance) const
{
  if (uiSplitIndex >= XII_ARRAY_SIZE(m_fShadowCascadeSplitDistances))
  {
    return;
  }

  XII_LOCK(m_Mutex);

  m_fShadowCascadeSplitDistances[uiSplitIndex] = xiiMath::Max(0.0f, fSplitDistance);
}

void xiiRenderWorldModule::SetDirectionalShadowAtlasPackingSample(const xiiVec4& vAtlasPackingSample) const
{
  XII_LOCK(m_Mutex);

  m_vDirectionalShadowAtlasPackingSample = vAtlasPackingSample;
}

void xiiRenderWorldModule::SetDirectionalShadowTexelSnapSample(const xiiVec4& vTexelSnapSample) const
{
  XII_LOCK(m_Mutex);

  m_vDirectionalShadowTexelSnapSample = vTexelSnapSample;
}

void xiiRenderWorldModule::SetLocalLightShadowAllocatorDeterministicSample(const xiiVec4& vDeterministicSample) const
{
  XII_LOCK(m_Mutex);

  m_vLocalLightShadowAllocatorDeterministicSample = vDeterministicSample;
}

void xiiRenderWorldModule::SetContactShadowLightParamsSample(const xiiVec4& vLightParamsSample) const
{
  XII_LOCK(m_Mutex);

  m_vContactShadowLightParamsSample = vLightParamsSample;
}

void xiiRenderWorldModule::SetClusterDepthRangeSample(const xiiVec4& vDepthRangeSample) const
{
  XII_LOCK(m_Mutex);

  m_vClusterDepthRangeSample = vDepthRangeSample;
}

void xiiRenderWorldModule::SetOccluderDepthPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pOccluderDepthResource = pDepthResource;
}

void xiiRenderWorldModule::SetOccluderDepthPrepassInstanceListResource(xiiSharedPtr<xiiGALBuffer> pInstanceListResource) const
{
  XII_LOCK(m_Mutex);

  m_pOccluderInstanceListBuffer = pInstanceListResource;
}

void xiiRenderWorldModule::SetHiZDepthSourceResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZDepthSourceResource = pDepthResource;
}

void xiiRenderWorldModule::SetHiZDepthPyramidResource(xiiSharedPtr<xiiGALResource> pDepthPyramidResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZDepthPyramidResource = pDepthPyramidResource;
}

void xiiRenderWorldModule::SetHiZOcclusionCandidateInstancesResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstancesResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZOcclusionCandidateInstancesBuffer = pCandidateInstancesResource;
}

void xiiRenderWorldModule::SetHiZOcclusionCandidateInstanceCountResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstanceCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pHiZOcclusionCandidateInstanceCountBuffer = pCandidateInstanceCountResource;
}

void xiiRenderWorldModule::SetDrawIndirectMaterialBinsResource(xiiSharedPtr<xiiGALBuffer> pMaterialBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuMaterialBinsBuffer = pMaterialBinsResource;
}

void xiiRenderWorldModule::SetDrawIndirectCommandBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCommandBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCommandsBuffer = pIndirectCommandBufferResource;
}

void xiiRenderWorldModule::SetDrawIndirectCountBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCountBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCountsBuffer = pIndirectCountBufferResource;
}

void xiiRenderWorldModule::SetMainDepthPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pMainDepthPrepassDepthResource = pDepthResource;
}

void xiiRenderWorldModule::SetMainDepthPrepassIndirectCommandBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCommandBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCommandsBuffer = pIndirectCommandBufferResource;
}

void xiiRenderWorldModule::SetMainDepthPrepassIndirectCountBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCountBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pGpuIndirectDrawCountsBuffer = pIndirectCountBufferResource;
}

void xiiRenderWorldModule::SetNormalRoughnessPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pNormalRoughnessPrepassDepthResource = pDepthResource;
}

void xiiRenderWorldModule::SetNormalRoughnessPrepassOutputResource(xiiSharedPtr<xiiGALResource> pNormalRoughnessResource) const
{
  XII_LOCK(m_Mutex);

  m_pNormalRoughnessPrepassOutputResource = pNormalRoughnessResource;
}

void xiiRenderWorldModule::SetDirectionalShadowCullingSceneBoundsResource(xiiSharedPtr<xiiGALBuffer> pSceneBoundsResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterCullingSceneBoundsBuffer = pSceneBoundsResource;
}

void xiiRenderWorldModule::SetDirectionalShadowCullingCascadeDataResource(xiiSharedPtr<xiiGALBuffer> pCascadeDataResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterCullingCascadeDataBuffer = pCascadeDataResource;
}

void xiiRenderWorldModule::SetDirectionalShadowCullingVisibleListResource(xiiSharedPtr<xiiGALBuffer> pVisibleListResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterVisibleListBuffer = pVisibleListResource;
}

void xiiRenderWorldModule::SetDirectionalShadowCullingVisibleCountResource(xiiSharedPtr<xiiGALBuffer> pVisibleCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pShadowCasterVisibleCountBuffer = pVisibleCountResource;
}

void xiiRenderWorldModule::SetDirectionalShadowDepthAtlasResource(xiiSharedPtr<xiiGALResource> pDepthAtlasResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowDepthAtlasResource = pDepthAtlasResource;
}

void xiiRenderWorldModule::SetDirectionalShadowRenderingVisibleListResource(xiiSharedPtr<xiiGALBuffer> pVisibleListResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingVisibleListBuffer = pVisibleListResource;
}

void xiiRenderWorldModule::SetDirectionalShadowRenderingVisibleCountResource(xiiSharedPtr<xiiGALBuffer> pVisibleCountResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingVisibleCountBuffer = pVisibleCountResource;
}

void xiiRenderWorldModule::SetDirectionalShadowRenderingCascadeDataResource(xiiSharedPtr<xiiGALBuffer> pCascadeDataResource) const
{
  XII_LOCK(m_Mutex);

  m_pDirectionalShadowRenderingCascadeDataBuffer = pCascadeDataResource;
}

void xiiRenderWorldModule::SetLocalLightShadowRequestsResource(xiiSharedPtr<xiiGALBuffer> pRequestsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowRequestsBuffer = pRequestsResource;
}

void xiiRenderWorldModule::SetLocalLightShadowAtlasPlacementsResource(xiiSharedPtr<xiiGALBuffer> pPlacementsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowAtlasPlacementsBuffer = pPlacementsResource;
}

void xiiRenderWorldModule::SetLocalLightShadowCastersResource(xiiSharedPtr<xiiGALBuffer> pCastersResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowCastersBuffer = pCastersResource;
}

void xiiRenderWorldModule::SetLocalLightShadowMaterialBinsResource(xiiSharedPtr<xiiGALBuffer> pMaterialBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowMaterialBinsBuffer = pMaterialBinsResource;
}

void xiiRenderWorldModule::SetLocalLightShadowModeBinsResource(xiiSharedPtr<xiiGALBuffer> pModeBinsResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalLightShadowModeBinsBuffer = pModeBinsResource;
}

void xiiRenderWorldModule::SetLocalLightShadowAtlasPagesResource(xiiSharedPtr<xiiGALResource> pAtlasPagesResource) const
{
  XII_LOCK(m_Mutex);

  m_pLocalShadowAtlasPagesResource = pAtlasPagesResource;
}

void xiiRenderWorldModule::SetContactShadowDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowDepthResource = pDepthResource;
}

void xiiRenderWorldModule::SetContactShadowNormalRoughnessResource(xiiSharedPtr<xiiGALResource> pNormalRoughnessResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowNormalRoughnessResource = pNormalRoughnessResource;
}

void xiiRenderWorldModule::SetContactShadowLightParamsResource(xiiSharedPtr<xiiGALBuffer> pLightParamsResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowLightParamsBuffer = pLightParamsResource;
}

void xiiRenderWorldModule::SetContactShadowOutputResource(xiiSharedPtr<xiiGALResource> pContactShadowTermResource) const
{
  XII_LOCK(m_Mutex);

  m_pContactShadowTermResource = pContactShadowTermResource;
}

void xiiRenderWorldModule::SetClusterCameraFrustumResource(xiiSharedPtr<xiiGALBuffer> pCameraFrustumResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterCameraFrustumBuffer = pCameraFrustumResource;
}

void xiiRenderWorldModule::SetClusterDepthRangeResource(xiiSharedPtr<xiiGALBuffer> pDepthRangeResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterDepthRangeBuffer = pDepthRangeResource;
}

void xiiRenderWorldModule::SetClusterDescriptorsResource(xiiSharedPtr<xiiGALBuffer> pClusterDescriptorsResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterDescriptorsBuffer = pClusterDescriptorsResource;
}

void xiiRenderWorldModule::SetVisibleLightListResource(xiiSharedPtr<xiiGALBuffer> pVisibleLightListResource) const
{
  XII_LOCK(m_Mutex);

  m_pVisibleLightListBuffer = pVisibleLightListResource;
}

void xiiRenderWorldModule::SetClusterDepthInfoResource(xiiSharedPtr<xiiGALBuffer> pDepthInfoResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterDepthRangeBuffer = pDepthInfoResource;
}

void xiiRenderWorldModule::SetClusterLightIndicesResource(xiiSharedPtr<xiiGALBuffer> pClusterLightIndicesResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterLightIndicesBuffer = pClusterLightIndicesResource;
}

void xiiRenderWorldModule::SetClusterLightPrefixSumsResource(xiiSharedPtr<xiiGALBuffer> pClusterLightPrefixSumsResource) const
{
  XII_LOCK(m_Mutex);

  m_pClusterLightPrefixSumsBuffer = pClusterLightPrefixSumsResource;
}

void xiiRenderWorldModule::SetDecalVolumesResource(xiiSharedPtr<xiiGALBuffer> pDecalVolumesResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalVolumesBuffer = pDecalVolumesResource;
}

void xiiRenderWorldModule::SetDecalClassificationDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalClassificationDepthResource = pDepthResource;
}

void xiiRenderWorldModule::SetDecalTileListsResource(xiiSharedPtr<xiiGALBuffer> pDecalTileListsResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalTileListsBuffer = pDecalTileListsResource;
}

void xiiRenderWorldModule::SetDecalResolveGBufferResource(xiiSharedPtr<xiiGALResource> pGBufferResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalResolveGBufferResource = pGBufferResource;
}

void xiiRenderWorldModule::SetDecalResolveTileListsResource(xiiSharedPtr<xiiGALBuffer> pDecalTileListsResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalResolveTileListsBuffer = pDecalTileListsResource;
}

void xiiRenderWorldModule::SetDecalResolveOutputResource(xiiSharedPtr<xiiGALResource> pUpdatedMaterialAttributesResource) const
{
  XII_LOCK(m_Mutex);

  m_pDecalResolveOutputResource = pUpdatedMaterialAttributesResource;
}

void xiiRenderWorldModule::SetOccluderDepthPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetOccluderDepthPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderWorldModule::ClearOccluderDepthPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupOccluderDepthPrepassCommandList, this));
}

void xiiRenderWorldModule::ClearOccluderDepthPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pOccluderDepthPass != nullptr, "Occluder depth pass must be initialized.");
  m_pOccluderDepthPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawOccluderDepthPrepassCommandList, this));
}

void xiiRenderWorldModule::SetMainDepthPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetMainDepthPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderWorldModule::ClearMainDepthPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupMainDepthPrepassCommandList, this));
}

void xiiRenderWorldModule::ClearMainDepthPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pMainDepthPrepassPass != nullptr, "Main depth prepass pass must be initialized.");
  m_pMainDepthPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawMainDepthPrepassCommandList, this));
}

void xiiRenderWorldModule::SetNormalRoughnessPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetNormalRoughnessPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(drawFunc);
}

void xiiRenderWorldModule::ClearNormalRoughnessPrepassSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupNormalRoughnessPrepassCommandList, this));
}

void xiiRenderWorldModule::ClearNormalRoughnessPrepassDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pNormalRoughnessPrepassPass != nullptr, "Normal-roughness prepass must be initialized.");
  m_pNormalRoughnessPrepassPass->SetDrawCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawNormalRoughnessPrepassCommandList, this));
}

void xiiRenderWorldModule::SetDirectionalShadowRenderingSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetDirectionalShadowRenderingDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(drawFunc);
}

void xiiRenderWorldModule::ClearDirectionalShadowRenderingSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDirectionalShadowRenderingCommandList, this));
}

void xiiRenderWorldModule::ClearDirectionalShadowRenderingDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDirectionalShadowRenderingPass != nullptr, "Directional shadow rendering pass must be initialized.");
  m_pDirectionalShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawDirectionalShadowRenderingCommandList, this));
}

void xiiRenderWorldModule::SetLocalLightShadowAtlasAllocationSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowSetupPass != nullptr, "Local-light shadow atlas allocation pass must be initialized.");
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearLocalLightShadowAtlasAllocationSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowSetupPass != nullptr, "Local-light shadow atlas allocation pass must be initialized.");
  m_pLocalLightShadowSetupPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupLocalLightShadowAtlasAllocationCommandList, this));
}

void xiiRenderWorldModule::SetSpotAndPointShadowRenderingSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetSpotAndPointShadowRenderingDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(drawFunc);
}

void xiiRenderWorldModule::ClearSpotAndPointShadowRenderingSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupSpotAndPointShadowRenderingCommandList, this));
}

void xiiRenderWorldModule::ClearSpotAndPointShadowRenderingDrawFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLocalLightShadowRenderingPass != nullptr, "Spot/point shadow rendering pass must be initialized.");
  m_pLocalLightShadowRenderingPass->SetExecuteCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::DrawSpotAndPointShadowRenderingCommandList, this));
}

void xiiRenderWorldModule::SetContactShadowSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pContactShadowsPass != nullptr, "Contact shadow pass must be initialized.");
  m_pContactShadowsPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearContactShadowSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pContactShadowsPass != nullptr, "Contact shadow pass must be initialized.");
  m_pContactShadowsPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupContactShadowCommandList, this));
}

void xiiRenderWorldModule::SetClusterGridBuildSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pClusterGridBuildPass != nullptr, "Cluster-grid build pass must be initialized.");
  m_pClusterGridBuildPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearClusterGridBuildSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pClusterGridBuildPass != nullptr, "Cluster-grid build pass must be initialized.");
  m_pClusterGridBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupClusterGridBuildCommandList, this));
}

void xiiRenderWorldModule::SetLightListConstructionSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLightListBuildPass != nullptr, "Light-list construction pass must be initialized.");
  m_pLightListBuildPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearLightListConstructionSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pLightListBuildPass != nullptr, "Light-list construction pass must be initialized.");
  m_pLightListBuildPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupLightListConstructionCommandList, this));
}

void xiiRenderWorldModule::SetDecalClassificationSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalClassificationPass != nullptr, "Decal classification pass must be initialized.");
  m_pDecalClassificationPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearDecalClassificationSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalClassificationPass != nullptr, "Decal classification pass must be initialized.");
  m_pDecalClassificationPass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDecalClassificationCommandList, this));
}

void xiiRenderWorldModule::SetDecalResolveSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalResolvePass != nullptr, "Decal resolve pass must be initialized.");
  m_pDecalResolvePass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::ClearDecalResolveSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pDecalResolvePass != nullptr, "Decal resolve pass must be initialized.");
  m_pDecalResolvePass->SetSetupCommandListFunc(xiiMakeDelegate(&xiiRenderWorldModule::SetupDecalResolveCommandList, this));
}

void xiiRenderWorldModule::SetPerFrameUploadCameraConstantsSample(const xiiPerFrameCameraUploadData& cameraConstantsSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameCameraConstantsSample = cameraConstantsSample;
}

void xiiRenderWorldModule::SetPerFrameUploadLightDataSample(const xiiPerFrameLightUploadData& lightDataSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameLightDataSample = lightDataSample;
}

void xiiRenderWorldModule::SetPerFrameUploadGlobalParamsSample(const xiiPerFrameGlobalUploadData& globalParamsSample) const
{
  XII_LOCK(m_Mutex);

  m_PerFrameGlobalParamsSample = globalParamsSample;
}

void xiiRenderWorldModule::SetPreviousFrameStatsSample(const xiiPreviousFrameStats& previousFrameStatsSample) const
{
  XII_LOCK(m_Mutex);

  m_PreviousFrameStatsSample = previousFrameStatsSample;
}

void xiiRenderWorldModule::SetRayTracedShadowsDenoiserHistoryEnabled(bool bEnable) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDenoiserHistoryEnabled(bEnable);
}

void xiiRenderWorldModule::SetRayTracedShadowsSceneTlasResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetSceneTlasResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsDepthResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDepthResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsNormalResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetNormalResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsLightDataResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetLightDataResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsShadowMaskResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetShadowMaskResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsHistoryInputResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetHistoryInputResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsHistoryOutputResourceName(xiiHashedString sResourceName) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetHistoryOutputResourceName(sResourceName);
}

void xiiRenderWorldModule::SetRayTracedShadowsSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetSetupCommandListFunc(setupFunc);
}

void xiiRenderWorldModule::SetRayTracedShadowsDispatchFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> dispatchFunc) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->SetDispatchRayTracingFunc(dispatchFunc);
}

void xiiRenderWorldModule::ClearRayTracedShadowsSetupFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->ClearSetupCommandListFunc();
}

void xiiRenderWorldModule::ClearRayTracedShadowsDispatchFunc() const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(m_pRayTracedShadowsPass != nullptr, "Ray-traced shadows pass must be initialized.");
  m_pRayTracedShadowsPass->ClearDispatchRayTracingFunc();
}

void xiiRenderWorldModule::CompactSkinningDataBuffer(const UpdateContext& context)
{
  XII_IGNORE_UNUSED(context);
}

void xiiRenderWorldModule::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  XII_IGNORE_UNUSED(e);
}

void xiiRenderWorldModule::EnsureGpuDrivenVisibilityResources(xiiUInt32 uiInstanceCapacity) const
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

void xiiRenderWorldModule::EnsureDynamicResolutionResources(xiiUInt32 uiElementCount) const
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

void xiiRenderWorldModule::EnsureSkinningAndMorphResources(xiiUInt32 uiElementCount) const
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

void xiiRenderWorldModule::EnsureInstanceUpdateResources(xiiUInt32 uiElementCount) const
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

void xiiRenderWorldModule::EnsureCoarseFrustumCullingResources(xiiUInt32 uiElementCount) const
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

void xiiRenderWorldModule::EnsureLodSelectionResources(xiiUInt32 uiElementCount) const
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

void xiiRenderWorldModule::EnsureDrawIndirectCommandBuildResources(xiiUInt32 uiDrawCapacity) const
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

void xiiRenderWorldModule::EnsureMainDepthPrepassResources(xiiUInt32 uiInstanceCapacity) const
{
  EnsureDrawIndirectCommandBuildResources(xiiMath::Max(1U, uiInstanceCapacity));

  if (m_pMainDepthPrepassDepthResource == nullptr)
  {
    m_pMainDepthPrepassDepthResource = m_pOccluderDepthResource;
  }
}

void xiiRenderWorldModule::EnsureNormalRoughnessPrepassResources(xiiUInt32 uiInstanceCapacity) const
{
  EnsureMainDepthPrepassResources(xiiMath::Max(1U, uiInstanceCapacity));

  if (m_pNormalRoughnessPrepassDepthResource == nullptr)
  {
    m_pNormalRoughnessPrepassDepthResource = m_pMainDepthPrepassDepthResource != nullptr ? m_pMainDepthPrepassDepthResource : m_pOccluderDepthResource;
  }
}

void xiiRenderWorldModule::EnsureDirectionalCascadeSetupResources() const
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

void xiiRenderWorldModule::EnsureDirectionalShadowCullingResources(xiiUInt32 uiInstanceCapacity) const
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

void xiiRenderWorldModule::EnsureDirectionalShadowRenderingResources(xiiUInt32 uiInstanceCapacity) const
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

void xiiRenderWorldModule::EnsureLocalLightShadowAtlasAllocationResources(xiiUInt32 uiRequestCapacity) const
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

void xiiRenderWorldModule::EnsureSpotAndPointShadowRenderingResources(xiiUInt32 uiCasterCapacity) const
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

void xiiRenderWorldModule::EnsureContactShadowResources() const
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

void xiiRenderWorldModule::EnsureClusterGridBuildResources(xiiUInt32 uiClusterCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureCoarseFrustumCullingResources(uiClusterCapacity);

  if (m_pClusterCameraFrustumBuffer == nullptr)
  {
    m_pClusterCameraFrustumBuffer = m_pCameraFrustumPlanesBuffer;
  }

  if (m_pClusterDepthRangeBuffer == nullptr)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = sizeof(xiiVec4);

    m_pClusterDepthRangeBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pClusterDepthRangeBuffer != nullptr)
    {
      m_pClusterDepthRangeBuffer->SetDebugName("RenderDataManager::ClusterDepthRange");
    }
  }

  const xiiUInt32 uiResolvedClusterCapacity = xiiMath::Max(1U, uiClusterCapacity);
  const xiiUInt64 uiDescriptorBufferSize    = static_cast<xiiUInt64>(uiResolvedClusterCapacity) * sizeof(xiiVec4);

  if (m_pClusterDescriptorsBuffer == nullptr || m_pClusterDescriptorsBuffer->GetDescription().m_uiSize < uiDescriptorBufferSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiDescriptorBufferSize;

    m_pClusterDescriptorsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pClusterDescriptorsBuffer != nullptr)
    {
      m_pClusterDescriptorsBuffer->SetDebugName("RenderDataManager::ClusterDescriptors");
    }
  }
}

void xiiRenderWorldModule::EnsureLightListConstructionResources(xiiUInt32 uiClusterCapacity, xiiUInt32 uiLightCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  EnsureClusterGridBuildResources(uiClusterCapacity);
  EnsurePerFrameUploadResources(m_uiPerFrameUploadRingSize);

  if (m_pVisibleLightListBuffer == nullptr)
  {
    m_pVisibleLightListBuffer = m_pPerFrameLightDataBuffer;
  }

  const xiiUInt32 uiResolvedClusterCapacity = xiiMath::Max(1U, uiClusterCapacity);
  const xiiUInt32 uiResolvedLightCapacity   = xiiMath::Max(1U, uiLightCapacity);

  const xiiUInt64 uiCompactIndexCapacity = static_cast<xiiUInt64>(uiResolvedClusterCapacity) * static_cast<xiiUInt64>(xiiMath::Min(64U, uiResolvedLightCapacity));
  const xiiUInt64 uiCompactIndicesSize   = xiiMath::Max<xiiUInt64>(1ULL, uiCompactIndexCapacity) * sizeof(xiiUInt32);

  if (m_pClusterLightIndicesBuffer == nullptr || m_pClusterLightIndicesBuffer->GetDescription().m_uiSize < uiCompactIndicesSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiCompactIndicesSize;

    m_pClusterLightIndicesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pClusterLightIndicesBuffer != nullptr)
    {
      m_pClusterLightIndicesBuffer->SetDebugName("RenderDataManager::ClusterLightIndices");
    }
  }

  const xiiUInt64 uiPrefixSumsSize = static_cast<xiiUInt64>(uiResolvedClusterCapacity) * sizeof(xiiUInt32);
  if (m_pClusterLightPrefixSumsBuffer == nullptr || m_pClusterLightPrefixSumsBuffer->GetDescription().m_uiSize < uiPrefixSumsSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiPrefixSumsSize;

    m_pClusterLightPrefixSumsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pClusterLightPrefixSumsBuffer != nullptr)
    {
      m_pClusterLightPrefixSumsBuffer->SetDebugName("RenderDataManager::ClusterLightPrefixSums");
    }
  }
}

void xiiRenderWorldModule::EnsureDecalClassificationResources(xiiUInt32 uiTileCapacity, xiiUInt32 uiDecalCapacity) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Default GAL device must be available.");

  const xiiUInt32 uiResolvedTileCapacity  = xiiMath::Max(1U, uiTileCapacity);
  const xiiUInt32 uiResolvedDecalCapacity = xiiMath::Max(1U, uiDecalCapacity);

  if (m_pDecalClassificationDepthResource == nullptr)
  {
    m_pDecalClassificationDepthResource = m_pMainDepthPrepassDepthResource != nullptr ? m_pMainDepthPrepassDepthResource : m_pOccluderDepthResource;
  }

  const xiiUInt64 uiDecalVolumesSize = static_cast<xiiUInt64>(uiResolvedDecalCapacity) * sizeof(xiiVec4);
  if (m_pDecalVolumesBuffer == nullptr || m_pDecalVolumesBuffer->GetDescription().m_uiSize < uiDecalVolumesSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    bufferDescription.m_uiElementByteStride = sizeof(xiiVec4);
    bufferDescription.m_uiSize              = uiDecalVolumesSize;

    m_pDecalVolumesBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDecalVolumesBuffer != nullptr)
    {
      m_pDecalVolumesBuffer->SetDebugName("RenderDataManager::DecalVolumes");
    }
  }

  const xiiUInt64 uiTileListSize = static_cast<xiiUInt64>(uiResolvedTileCapacity) * 8ULL * sizeof(xiiUInt32);
  if (m_pDecalTileListsBuffer == nullptr || m_pDecalTileListsBuffer->GetDescription().m_uiSize < uiTileListSize)
  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
    bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
    bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
    bufferDescription.m_uiSize              = uiTileListSize;

    m_pDecalTileListsBuffer = pDevice->CreateBuffer(bufferDescription);
    if (m_pDecalTileListsBuffer != nullptr)
    {
      m_pDecalTileListsBuffer->SetDebugName("RenderDataManager::DecalTileLists");
    }
  }
}

void xiiRenderWorldModule::EnsureDecalResolveResources() const
{
  if (m_pDecalResolveGBufferResource == nullptr)
  {
    m_pDecalResolveGBufferResource = m_pNormalRoughnessPrepassOutputResource;
  }

  if (m_pDecalResolveTileListsBuffer == nullptr)
  {
    m_pDecalResolveTileListsBuffer = m_pDecalTileListsBuffer;
  }

  if (m_pDecalResolveOutputResource == nullptr)
  {
    m_pDecalResolveOutputResource = m_pNormalRoughnessPrepassOutputResource;
  }
}

void xiiRenderWorldModule::EnsureFrameSetupResources() const
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

void xiiRenderWorldModule::EnsurePerFrameUploadResources(xiiUInt32 uiRingSize) const
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

void xiiRenderWorldModule::SetupGpuDrivenVisibilityCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupDynamicResolutionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupSkinningAndMorphCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupInstanceUpdateCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupCoarseFrustumCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  xiiSharedPtr<xiiGALBuffer> pOccluderInstanceList = m_pOccluderInstanceListBuffer != nullptr ? m_pOccluderInstanceListBuffer : m_pGpuVisibleInstancesBuffer;
  if (pOccluderInstanceList != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("OccluderInstances"), pOccluderInstanceList->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Vertex);
  }
}

void xiiRenderWorldModule::DrawOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::SetupHiZPyramidBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupHiZOcclusionCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupDrawIndirectCommandBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupMainDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::DrawMainDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::SetupNormalRoughnessPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::DrawNormalRoughnessPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::SetupDirectionalCascadeSetupCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupDirectionalShadowCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupDirectionalShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

  xiiSharedPtr<xiiGALBuffer> pVisibleList  = m_pDirectionalShadowRenderingVisibleListBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleListBuffer : m_pShadowCasterVisibleListBuffer;
  xiiSharedPtr<xiiGALBuffer> pVisibleCount = m_pDirectionalShadowRenderingVisibleCountBuffer != nullptr ? m_pDirectionalShadowRenderingVisibleCountBuffer : m_pShadowCasterVisibleCountBuffer;
  xiiSharedPtr<xiiGALBuffer> pCascadeData  = m_pDirectionalShadowRenderingCascadeDataBuffer != nullptr ? m_pDirectionalShadowRenderingCascadeDataBuffer : m_pShadowCascadeDataBuffer;

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

void xiiRenderWorldModule::DrawDirectionalShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::SetupLocalLightShadowAtlasAllocationCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupSpotAndPointShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::DrawSpotAndPointShadowRenderingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::SetupContactShadowCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupClusterGridBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiClusterCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureClusterGridBuildResources(uiClusterCount);

  if (m_hClusterGridBuildShader.IsValid() == false)
  {
    m_hClusterGridBuildShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ClusterGridBuild.xiiShader");
  }

  if (m_pClusterGridBuildPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hClusterGridBuildShader, s_PermutationVars, true);
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

    m_pClusterGridBuildPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pClusterGridBuildPipelineState == nullptr)
  {
    return;
  }

  xiiSharedPtr<xiiGALBuffer> pCameraFrustum = m_pClusterCameraFrustumBuffer != nullptr ? m_pClusterCameraFrustumBuffer : m_pCameraFrustumPlanesBuffer;
  if (m_pClusterCameraFrustumBuffer == nullptr && pCameraFrustum != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, pCameraFrustum, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(m_vCoarseFrustumPlaneSamples), sizeof(m_vCoarseFrustumPlaneSamples))).AssertSuccess();
  }

  if (m_pClusterDepthRangeBuffer != nullptr)
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(&commandList, m_pClusterDepthRangeBuffer, 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&m_vClusterDepthRangeSample), sizeof(m_vClusterDepthRangeSample))).AssertSuccess();
  }

  commandList.SetPipelineState(m_pClusterGridBuildPipelineState);

  if (pCameraFrustum != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ClusterCameraFrustum"), pCameraFrustum->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pClusterDepthRangeBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ClusterDepthRange"), m_pClusterDepthRangeBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pClusterDescriptorsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ClusterDescriptors"), m_pClusterDescriptorsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderWorldModule::SetupLightListConstructionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiClusterCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  const xiiUInt32 uiLightCount   = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureLightListConstructionResources(uiClusterCount, uiLightCount);

  if (m_hLightListBuildShader.IsValid() == false)
  {
    m_hLightListBuildShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/LightListBuild.xiiShader");
  }

  if (m_pLightListBuildPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hLightListBuildShader, s_PermutationVars, true);
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

    m_pLightListBuildPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pLightListBuildPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pLightListBuildPipelineState);

  if (m_pVisibleLightListBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("VisibleLightList"), m_pVisibleLightListBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pClusterDescriptorsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ClusterDescriptors"), m_pClusterDescriptorsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pClusterDepthRangeBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("ClusterDepthRange"), m_pClusterDepthRangeBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pClusterLightIndicesBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ClusterLightIndices"), m_pClusterLightIndicesBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }

  if (m_pClusterLightPrefixSumsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("ClusterLightPrefixSums"), m_pClusterLightPrefixSumsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderWorldModule::SetupDecalClassificationCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  const xiiUInt32 uiTileCount  = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  const xiiUInt32 uiDecalCount = xiiMath::Max(1U, m_GpuDrivenInstances.GetCount());
  EnsureDecalClassificationResources(uiTileCount, uiDecalCount);

  if (m_hDecalClassificationShader.IsValid() == false)
  {
    m_hDecalClassificationShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DecalClassification.xiiShader");
  }

  if (m_pDecalClassificationPipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDecalClassificationShader, s_PermutationVars, true);
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

    m_pDecalClassificationPipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pDecalClassificationPipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pDecalClassificationPipelineState);

  if (m_pDecalVolumesBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("DecalVolumes"), m_pDecalVolumesBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pDecalClassificationDepthResource != nullptr)
  {
    if (xiiGALTexture* pDepthTexture = xiiDynamicCast<xiiGALTexture*>(m_pDecalClassificationDepthResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("SceneDepth"), pDepthTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pDecalTileListsBuffer != nullptr)
  {
    commandList.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString("DecalTileLists"), m_pDecalTileListsBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  }
}

void xiiRenderWorldModule::SetupDecalResolveCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  EnsureDecalResolveResources();

  if (m_hDecalResolveShader.IsValid() == false)
  {
    m_hDecalResolveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/DecalResolve.xiiShader");
  }

  if (m_pDecalResolvePipelineState == nullptr)
  {
    static const xiiHashTable<xiiHashedString, xiiHashedString> s_PermutationVars;

    const xiiShaderPermutationResourceHandle      hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hDecalResolveShader, s_PermutationVars, true);
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

    m_pDecalResolvePipelineState = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }

  if (m_pDecalResolvePipelineState == nullptr)
  {
    return;
  }

  commandList.SetPipelineState(m_pDecalResolvePipelineState);

  if (m_pDecalResolveGBufferResource != nullptr)
  {
    if (xiiGALTexture* pGBufferTexture = xiiDynamicCast<xiiGALTexture*>(m_pDecalResolveGBufferResource.Borrow()))
    {
      commandList.ResolveAndSetShaderResourceTextureView(xiiTempHashedString("GBufferTargets"), pGBufferTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
  }

  if (m_pDecalResolveTileListsBuffer != nullptr)
  {
    commandList.ResolveAndSetShaderResourceBufferView(xiiTempHashedString("DecalTileLists"), m_pDecalResolveTileListsBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  }

  if (m_pDecalResolveOutputResource != nullptr)
  {
    if (xiiGALTexture* pOutputTexture = xiiDynamicCast<xiiGALTexture*>(m_pDecalResolveOutputResource.Borrow()))
    {
      commandList.ResolveAndSetUnorderedAccessTextureView(xiiTempHashedString("UpdatedMaterialAttributes"), pOutputTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    }
  }
}

void xiiRenderWorldModule::SetupLodSelectionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupFrameSetupCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(executionContext);

  XII_LOCK(m_Mutex);

  if (m_pFrameConstantsBuffer != nullptr)
  {
    commandList.ResolveAndSetConstantBuffer(xiiTempHashedString("xiiFrameConstants"), m_pFrameConstantsBuffer);
  }
}

void xiiRenderWorldModule::UploadPerFrameBufferDataCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::OnGpuDrivenVisibilityPostDispatch(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
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

void xiiRenderWorldModule::SetupRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

void xiiRenderWorldModule::DispatchRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const
{
  XII_IGNORE_UNUSED(commandList);
  XII_IGNORE_UNUSED(executionContext);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderDataManager);
