#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Device/Device.h>

xiiCVarFloat cvar_DynamicRenderingTargetMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");
xiiCVarFloat cvar_DynamicRenderingMinScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");
xiiCVarFloat cvar_DynamicRenderingMaxScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "Maximum allowed render scale (1.0 = native resolution).");

xiiCVarInt cvar_ClusterX("Rendering.Clustering.CountX", 16, xiiCVarFlags::Default, "Cluster grid X count.");
xiiCVarInt cvar_ClusterY("Rendering.Clustering.CountY", 9, xiiCVarFlags::Default, "Cluster grid Y count.");
xiiCVarInt cvar_ClusterZ("Rendering.Clustering.CountZ", 24, xiiCVarFlags::Default, "Cluster grid Z count.");

namespace
{
  // Shared constants (sizes of persistent GPU buffers, aligned to typical instance budgets)
  static constexpr xiiUInt32 k_uiMaxInstances    = 65536U; ///< The maximum number of drawable objects in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.
  static constexpr xiiUInt32 k_uiMaxLights       = 1024U;  ///< The maximum number of active lights in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.
  static constexpr xiiUInt32 k_uiMaxMaterialBins = 512U;   ///< The maximum number of distinct (mesh x material) draw bins in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiView::xiiView()
{
  m_pRenderGraph   = XII_DEFAULT_NEW(xiiRenderGraph);
  m_pExtractedData = XII_DEFAULT_NEW(xiiExtractedRenderData);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No default device available. A view requires a device to initialize its resources.");

  m_ViewPassResources.m_Profiler.Initialize(pDevice);
  m_ResourceCache.Initialize(pDevice);
}

xiiView::~xiiView()
{
  m_ViewPassResources.m_Profiler.Shutdown();
  m_ResourceCache.Shutdown();
}

void xiiView::RunDynamicResolutionPID(xiiRenderGraphBlackboard& blackboard)
{
  // Try the GPU profiler's resolved duration from 2 frames ago.
  // Falls back to CPU wall-clock when the profiler ring hasn't warmed up yet.
  float fGpuTimeMs = m_ViewPassResources.m_Profiler.GetPassDurationMs("FrameTotal");
  if (fGpuTimeMs <= 0.0f)
  {
    fGpuTimeMs = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;
  }

  m_ViewPassResources.m_DynamicResolution.m_fLastGpuFrameTimeMs = fGpuTimeMs;

  // PID controller.
  const float fMin       = xiiMath::Max(cvar_DynamicRenderingMinScale.GetValue(), 0.25f);
  const float fMax       = xiiMath::Min(cvar_DynamicRenderingMaxScale.GetValue(), 1.0f);
  const float fTarget    = xiiMath::Max(cvar_DynamicRenderingTargetMs.GetValue(), 0.1f);
  const float fDeltaTime = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral + fError * fDeltaTime, -1.0f, 1.0f);

  const float fDerivative = (fError - m_ViewPassResources.m_DynamicResolution.m_fPreviousError) / xiiMath::Max(fDeltaTime, 0.001f);
  const float fPID        = 0.35f * fError + 0.05f * m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral + 0.15f * fDerivative;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fCurrentScale + fPID, fMin, fMax);
  const float fDelta   = xiiMath::Clamp(fDesired - m_ViewPassResources.m_DynamicResolution.m_fCurrentScale, -0.10f, 0.10f);

  m_ViewPassResources.m_DynamicResolution.m_fCurrentScale  = m_ViewPassResources.m_DynamicResolution.m_fCurrentScale + fDelta;
  m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale = xiiMath::Lerp(m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale, m_ViewPassResources.m_DynamicResolution.m_fCurrentScale, 0.20f);
  m_ViewPassResources.m_DynamicResolution.m_fPreviousError = fError;

  const float fScale = m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale;

  // Align to even pixels to satisfy 2x2 tile constraints.
  const xiiUInt32 uiWidth  = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.width * fScale) & ~1U;
  const xiiUInt32 uiHeight = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.height * fScale) & ~1U;

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), xiiMath::Max(uiWidth, 2U));
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), xiiMath::Max(uiHeight, 2U));
}

////////// GPU occlusion readback //////////
//
// Reads the oldest staging buffer in the 3-frame ring (from 2 frames ago).
// This provides the GPU frame time used by the PID (the result has already been applied by RunDynamicResolutionPID before BeginSetup, so this pass simply keeps the readback ring rotating).

struct xiiOcclusionReadbackData
{
  xiiUInt32 m_uiReadSlot = 0; ///< Index into the 3-frame ring of staging buffers to read from this frame (the one written by the GPU 2 frames ago).
};

void xiiView::SetupOcclusionReadback(xiiOcclusionReadbackData& data, xiiRGBuilder& builder)
{
  data.m_uiReadSlot = (m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1u) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRGPassContext& context)
{
  xiiSharedPtr<xiiGALBuffer>& pStaging = m_ViewPassResources.m_VisibilityPasses.m_pOcclusionReadbackRing[data.m_uiReadSlot];

  if (pStaging == nullptr)
  {
    // Buffer not yet populated - skip.
    // Advance the write slot so the next stage can write into it next frame.
    m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot = (m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
    return;
  }

  // Map the staging buffer (CPU readable, GPU wrote 2 frames ago).
  xiiGALCommandList& cmd   = context.GetCommandList();
  void*              pData = nullptr;
  if (cmd.MapBuffer(pStaging, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pData).Succeeded())
  {
    // The staging buffer holds a single float: total GPU frame time in nanoseconds.
    // (Written by the previous frame's FrameTotal sentinel query readback.)
    // We don't store it here, the profiler's GetPassDurationMs("FrameTotal") path already does it.
    xiiLog::Debug("Occlusion readback: GPU frame time from 2 frames ago = {0} ms", *static_cast<float*>(pData) / 1'000'000.0f);
    cmd.UnmapBuffer(pStaging, xiiGALMapType::Read);
  }

  // Advance ring.
  m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot = (m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
}


void xiiView::BuildStage1_Visibility(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  graph.AddPass<xiiOcclusionReadbackData>("GpuOcclusionReadback", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupOcclusionReadback, this), xiiMakeDelegate(&xiiView::ExecuteOcclusionReadback, this));

#if 0
  xiiView* self = this; // captured by [this] lambdas

  // 1a. GPU occlusion readback (rotates ring buffer, side-effects = true)
  graph.AddPass<OcclusionReadbackData>(
    "GpuOcclusionReadback",
    xiiGALCommandQueueFlags::Compute,
    [self](OcclusionReadbackData& data, xiiRGBuilder& b) { SetupOcclusionReadback(*self, data, b); },
    [self](const OcclusionReadbackData& data, xiiRGPassContext& c) { ExecuteOcclusionReadback(*self, data, c); });

  // 1b. Frustum culling
  graph.AddPass<FrustumCullData>(
    "FrustumCulling",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](FrustumCullData& data, xiiRGBuilder& b) { SetupFrustumCulling(*self, data, b); },
    [self](const FrustumCullData& data, xiiRGPassContext& c) { ExecuteFrustumCulling(*self, data, c); });

  // 1c. LOD selection
  graph.AddPass<LODSelectData>(
    "LODSelection",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](LODSelectData& data, xiiRGBuilder& b) { SetupLODSelection(*self, data, b, blackboard); },
    [self](const LODSelectData& data, xiiRGPassContext& c) { ExecuteLODSelection(*self, data, c); });

  // 1d. Instance update (world matrices & bounds)
  graph.AddPass<InstanceUpdateData>(
    "InstanceUpdate",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](InstanceUpdateData& data, xiiRGBuilder& b) { SetupInstanceUpdate(*self, data, b, blackboard); },
    [self](const InstanceUpdateData& data, xiiRGPassContext& c) { ExecuteInstanceUpdate(*self, data, c); });

  // 1e. Draw command build (GPU-driven indirect args)
  graph.AddPass<DrawBuildData>(
    "DrawCommandBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](DrawBuildData& data, xiiRGBuilder& b) { SetupDrawBuild(*self, data, b, blackboard); },
    [self](const DrawBuildData& data, xiiRGPassContext& c) { ExecuteDrawBuild(*self, data, c); });

  // 1f. Shadow caster list build
  graph.AddPass<ShadowCasterBuildData>(
    "ShadowCasterListBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ShadowCasterBuildData& data, xiiRGBuilder& b) { SetupShadowCasterBuild(*self, data, b, blackboard); },
    [self](const ShadowCasterBuildData& data, xiiRGPassContext& c) { ExecuteShadowCasterBuild(*self, data, c); });

  // 1g. Cluster grid build
  graph.AddPass<ClusterBuildData>(
    "ClusterGridBuild",
    xiiGALCommandQueueFlags::Compute,
    [self](ClusterBuildData& data, xiiRGBuilder& b) { SetupClusterBuild(*self, data, b); },
    [self](const ClusterBuildData& data, xiiRGPassContext& c) { ExecuteClusterBuild(*self, data, c); });

  // 1h. Light list build
  graph.AddPass<LightListData>(
    "LightListBuild",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](LightListData& data, xiiRGBuilder& b) { SetupLightListBuild(*self, data, b, blackboard); },
    [self](const LightListData& data, xiiRGPassContext& c) { ExecuteLightListBuild(*self, data, c); });

  // 1i. Reflection probe selection
  graph.AddPass<ReflProbeSelectData>(
    "ReflectionProbeSelection",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](ReflProbeSelectData& data, xiiRGBuilder& b) { SetupReflProbeSelect(*self, data, b, blackboard); },
    [self](const ReflProbeSelectData& data, xiiRGPassContext& c) { ExecuteReflProbeSelect(*self, data, c); });

  // 1j. Volumetric froxel grid allocation
  graph.AddPass<FroxelAllocData>(
    "VolumetricGridAlloc",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](FroxelAllocData& data, xiiRGBuilder& b) { SetupFroxelAlloc(*self, data, b, blackboard); },
    [self](const FroxelAllocData& data, xiiRGPassContext& c) { ExecuteFroxelAlloc(*self, data, c); });
#endif
}


void xiiView::BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Stage 0: CPU dynamic resolution PID (pre-graph, writes to blackboard).
  // Must happen before BeginSetup so passes see the correct render dimensions.
  RunDynamicResolutionPID(blackboard);

  XII_ASSERT_DEV(blackboard.Contains(xiiRGBlackboardKeys::k_RenderWidth) && blackboard.Contains(xiiRGBlackboardKeys::k_RenderHeight), "Dynamic resolution PID did not write render dimensions to the blackboard.");

  // Each stage adds its passes to the graph. Dependency ordering is handled by the render graph compiler (topological sort + culling).

  BuildStage1_Visibility(graph, blackboard);
  BuildStage2_Shadows(graph, blackboard);
  BuildStage3_Depth(graph, blackboard);
  BuildStage4_GBuffer(graph, blackboard);
  BuildStage5_LightingPrep(graph, blackboard);
  BuildStage6_MainLighting(graph, blackboard);
  BuildStage7_Forward(graph, blackboard);
  BuildStage8_Transparency(graph, blackboard);
  BuildStage9_ScreenSpace(graph, blackboard);
  BuildStage10_Temporal(graph, blackboard);
  BuildStage11_PostProcess(graph, blackboard);
  BuildStage12_Output(graph, blackboard);
}

// static
xiiSharedPtr<xiiGALComputePipelineState> xiiView::EnsureComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline, xiiStringView sShaderPath)
{
  if (inout_pPipeline != nullptr)
    return inout_pPipeline;

  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, /*bBlockTillLoaded=*/true);

  xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  XII_ASSERT_DEV(pPermutation.IsValid(), "Failed to load shader permutation: '{}'.", sShaderPath);

  xiiGALComputePipelineStateCreationDescription description;
  description.m_pComputeShader             = pPermutation->GetGALShader(xiiGALShaderType::Compute);
  description.m_pPipelineResourceSignature = pPermutation->GetPipelineResourceSignature();

  inout_pPipeline = xiiGALPipelineCache::GetPipeline(description);
  XII_ASSERT_DEV(inout_pPipeline != nullptr, "Failed to create compute pipeline: '{}'.", sShaderPath);

  return inout_pPipeline;
}
