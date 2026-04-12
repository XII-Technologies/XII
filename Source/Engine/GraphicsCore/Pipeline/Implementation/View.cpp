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

//
// CVars for dynamic resolution
//

xiiCVarFloat cvar_DrTargetMs(
  "Rendering.DynamicResolution.TargetFrameTimeMs",
  16.0f,
  xiiCVarFlags::Default,
  "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");

xiiCVarFloat cvar_DrMinScale(
  "Rendering.DynamicResolution.MinimumRenderScale",
  0.5f,
  xiiCVarFlags::Default,
  "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");

xiiCVarFloat cvar_DrMaxScale(
  "Rendering.DynamicResolution.MaximumRenderScale",
  1.0f,
  xiiCVarFlags::Default,
  "Maximum allowed render scale (1.0 = native resolution).");

//
// CVars for 3D light clustering
//

xiiCVarInt cvar_ClusterX("Rendering.Clustering.CountX", 16, xiiCVarFlags::Default, "Cluster grid X count.");
xiiCVarInt cvar_ClusterY("Rendering.Clustering.CountY", 9, xiiCVarFlags::Default, "Cluster grid Y count.");
xiiCVarInt cvar_ClusterZ("Rendering.Clustering.CountZ", 24, xiiCVarFlags::Default, "Cluster grid Z count.");

//
// Reflection
//

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

//
// Construction / destruction
//

xiiView::xiiView()
{
  m_pRenderGraph   = XII_DEFAULT_NEW(xiiRenderGraph);
  m_pExtractedData = XII_DEFAULT_NEW(xiiExtractedRenderData);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice != nullptr)
  {
    m_ViewPassResources.m_Profiler.Initialize(pDevice);
    m_ResourceCache.Initialize(pDevice);
  }
}

xiiView::~xiiView()
{
  m_ViewPassResources.m_Profiler.Shutdown();
  m_ResourceCache.Shutdown();
}

//
// Utility: lazy-load a compute pipeline
//

/*static*/ xiiSharedPtr<xiiGALComputePipelineState> xiiView::EnsureComputePipeline(
  xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline,
  xiiStringView                             sShaderPath)
{
  if (inout_pPipeline != nullptr)
    return inout_pPipeline;

  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVars;
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVars, /*bBlockTillLoaded=*/true);

  xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  XII_ASSERT_DEV(pPermutation.IsValid(), "Failed to load shader permutation: '{}'.", sShaderPath);

  xiiGALComputePipelineStateCreationDescription desc;
  desc.m_pComputeShader             = pPermutation->GetGALShader(xiiGALShaderType::Compute);
  desc.m_pPipelineResourceSignature = pPermutation->GetPipelineResourceSignature();

  inout_pPipeline = xiiGALPipelineCache::GetPipeline(desc);
  XII_ASSERT_DEV(inout_pPipeline != nullptr, "Failed to create compute pipeline: '{}'.", sShaderPath);

  return inout_pPipeline;
}

//
// CPU PID dynamic resolution (runs before BeginSetup)
//

void xiiView::RunDynamicResolutionPID(xiiRenderGraphBlackboard& blackboard)
{
  auto& pid = m_ViewPassResources.m_DynamicResolution;

  //  Timing source
  // Try the GPU profiler's resolved duration from 2 frames ago.
  // Falls back to CPU wall-clock when the profiler ring hasn't warmed up yet.
  const xiiHashedString sFrameTotal = xiiMakeHashedString("FrameTotal");
  float                 fGpuTimeMs  = m_ViewPassResources.m_Profiler.GetPassDurationMs(sFrameTotal);
  if (fGpuTimeMs <= 0.0f)
    fGpuTimeMs = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  pid.m_fLastGpuFrameTimeMs = fGpuTimeMs;

  //  PID controller
  const float fMin    = xiiMath::Max(cvar_DrMinScale.GetValue(), 0.25f);
  const float fMax    = xiiMath::Min(cvar_DrMaxScale.GetValue(), 1.0f);
  const float fTarget = xiiMath::Max(cvar_DrTargetMs.GetValue(), 0.1f);
  const float fDt     = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  pid.m_fErrorIntegral = xiiMath::Clamp(pid.m_fErrorIntegral + fError * fDt, -1.0f, 1.0f);

  const float fDeriv = (fError - pid.m_fPreviousError) / xiiMath::Max(fDt, 0.001f);
  const float fPID   = 0.35f * fError + 0.05f * pid.m_fErrorIntegral + 0.15f * fDeriv;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(pid.m_fCurrentScale + fPID, fMin, fMax);
  const float fDelta   = xiiMath::Clamp(fDesired - pid.m_fCurrentScale, -0.10f, 0.10f);

  pid.m_fCurrentScale  = pid.m_fCurrentScale + fDelta;
  pid.m_fSmoothedScale = xiiMath::Lerp(pid.m_fSmoothedScale, pid.m_fCurrentScale, 0.20f);
  pid.m_fPreviousError = fError;

  const float fScale = pid.m_fSmoothedScale;

  // Align to even pixels to satisfy 2×2 tile constraints.
  const auto uiW = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.width * fScale) & ~1u;
  const auto uiH = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.height * fScale) & ~1u;

  //  Publish to blackboard
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), xiiMath::Max(uiW, 2u));
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), xiiMath::Max(uiH, 2u));
}

//
// BuildDefaultRenderGraph - main dispatcher
//

void xiiView::BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  //  Stage 0: CPU dynamic resolution PID (pre-graph, writes to blackboard)
  // Must happen before BeginSetup so passes see the correct render dimensions.
  RunDynamicResolutionPID(blackboard);

  // Retrieve render dimensions for use in resource declarations.
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  //  Stages 1–12
  // Each stage adds its passes to the graph. Dependency ordering is handled by
  // the render graph compiler (topological sort + culling).

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
