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

  // Align to even pixels to satisfy 2×2 tile constraints.
  const xiiUInt32 uiWidth  = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.width * fScale) & ~1U;
  const xiiUInt32 uiHeight = static_cast<xiiUInt32>(m_Data.m_ViewPortRect.height * fScale) & ~1U;

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), xiiMath::Max(uiWidth, 2U));
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), xiiMath::Max(uiHeight, 2U));
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
