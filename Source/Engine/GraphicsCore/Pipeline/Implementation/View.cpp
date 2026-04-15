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
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/LightClustering/LightClusteringConstants.h>

xiiCVarFloat cvar_DynamicRenderingTargetMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");
xiiCVarFloat cvar_DynamicRenderingMinScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");
xiiCVarFloat cvar_DynamicRenderingMaxScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "Maximum allowed render scale (1.0 = native resolution).");

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

////////// GPU Frustum Culling //////////
//
// Culls instances against the view frustum on the GPU, using instance bounds from the previous frame (updated by the Instance Update pass) and LOD metadata from the previous frame (updated by the LOD Selection pass).
// This is a compute pass that writes out a compact list of visible instance indices for the current frame, which is then consumed by the Instance Update pass to only update visible instances, and by the Draw Build pass to only draw visible instances.

struct xiiFrustumCullData
{
  xiiRGBufferHandle m_hInstanceBounds;     ///< SRV in (structured buffer of xiiBoundingSphere, one per instance, from previous frame's Instance Update).
  xiiRGBufferHandle m_hLODMetadata;        ///< SRV in (structured buffer of LOD metadata, one per instance, from previous frame's LOD Selection).
  xiiRGBufferHandle m_hVisibleCandidates;  ///< UAV out (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, consumed by Instance Update and Draw Build).
  xiiUInt32         m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupFrustumCull(xiiFrustumCullData& data, xiiRGBuilder& builder)
{
  // Ensure persistent instance bounds buffer exists.
  if (!m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                              = 32U; // float3 center + float radius + float3 extents + float pad
    description.m_uiSize                                           = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                        = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                             = xiiGALBufferMode::Structured;
    description.m_Usage                                            = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  // Import persistent instance bounds as read-only SRV.
  data.m_hInstanceBounds = builder.ImportBuffer("InstanceBoundsIn", m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds = builder.ReadBuffer(data.m_hInstanceBounds, xiiGALResourceStateFlags::ShaderResource);

  // LOD metadata (also persistent, updated by CPU each frame before dispatch).
  if (!m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                              = 4U; // packed uint: LOD + flags
    description.m_uiSize                                           = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                        = xiiGALBindFlags::ShaderResource;
    description.m_Mode                                             = xiiGALBufferMode::Structured;
    description.m_Usage                                            = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }
  data.m_hLODMetadata = builder.ImportBuffer("LODMetadataIn", m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLODMetadata = builder.ReadBuffer(data.m_hLODMetadata, xiiGALResourceStateFlags::ShaderResource);

  // Transient visible candidate buffer.
  xiiGALBufferCreationDescription visibleCandidateBufferDescription;
  visibleCandidateBufferDescription.m_uiElementByteStride = 4U;                         // uint
  visibleCandidateBufferDescription.m_uiSize              = 4U + 4U * k_uiMaxInstances; // [0]=count + indices
  visibleCandidateBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  visibleCandidateBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  visibleCandidateBufferDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hVisibleCandidates                               = builder.WriteBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, visibleCandidateBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiInstanceCount = k_uiMaxInstances; // Driven by CPU-side count from extraction.

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pFrustumCullPipeline, "Shaders/Pipeline/CoarseFrustumCulling.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("FrustumCulling");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pFrustumCullPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LODMetadata", context.GetBuffer(data.m_hLODMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleOut", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU LOD Selection //////////
//
// Selects LOD levels for visible instances on the GPU, using instance bounds from the previous frame (updated by the Instance Update pass) and LOD metadata from the previous frame (updated by the previous frame's LOD Selection pass).

struct xiiLODSelectData
{
  xiiRGBufferHandle m_hVisibleCandidates;  ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRGBufferHandle m_hInstanceBounds;     ///< SRV in (structured buffer of xiiBoundingSphere, one per instance, from previous frame's Instance Update).
  xiiRGBufferHandle m_hInstanceLOD;        ///< UAV out (structured buffer of uint, one per instance, packed LOD level + meshlet offset, consumed by Draw Build).
  xiiUInt32         m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupLODSelect(xiiLODSelectData& data, xiiRGBuilder& builder)
{
  data.m_hVisibleCandidates = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, {}), xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(builder.ImportBuffer("InstanceBoundsLOD", m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount    = k_uiMaxInstances;

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U; // packed uint: LOD level + meshlet offset
  description.m_uiSize              = description.m_uiElementByteStride * k_uiMaxInstances;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceLOD               = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pLODSelectPipeline, "Shaders/Pipeline/LodSelection.xiiShader");
}

void xiiView::ExecuteLODSelect(const xiiLODSelectData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LODSelection");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pLODSelectPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_InstanceLODOut", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Instance Update //////////
//
// Updates instance data on the GPU, including world matrices and bounds, for the next frame's LOD selection and rendering passes.

struct xiiInstanceUpdateData
{
  xiiRGBufferHandle m_hVisibleCandidates;  ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRGBufferHandle m_hInstanceMatrices;   ///< UAV out (structured buffer of instance world matrices, one per instance, consumed by next frame's LOD Selection and Frustum Culling).
  xiiRGBufferHandle m_hInstanceBoundsOut;  ///< UAV out (structured buffer of xiiBoundingSphere, one per instance, consumed by next frame's Frustum Culling).
  xiiUInt32         m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRGBuilder& builder)
{
  data.m_hVisibleCandidates = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, {}), xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount    = k_uiMaxInstances;

  // Ensure persistent matrix buffer.
  if (!m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                              = 48U; // float4x3 (3 rows x 4 floats)
    description.m_uiSize                                           = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                        = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                             = xiiGALBufferMode::Structured;
    description.m_Usage                                            = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_hInstanceMatrices = builder.ImportBuffer("InstanceWorldMatrices", m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hInstanceMatrices = builder.WriteBuffer(data.m_hInstanceMatrices, xiiGALResourceStateFlags::UnorderedAccess);

  // Output AABB buffer for HiZ culling.
  xiiGALBufferCreationDescription boundsBufferDescription;
  boundsBufferDescription.m_uiElementByteStride = 32U;
  boundsBufferDescription.m_uiSize              = boundsBufferDescription.m_uiElementByteStride * k_uiMaxInstances;
  boundsBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  boundsBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceBoundsOut                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, boundsBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pInstanceUpdatePipeline, "Shaders/Pipeline/InstanceUpdate.xiiShader");
}

void xiiView::ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("InstanceUpdate");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pInstanceUpdatePipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_VisibleIn", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_MatricesOut", context.GetBuffer(data.m_hInstanceMatrices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_BoundsOut", context.GetBuffer(data.m_hInstanceBoundsOut)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Draw Command Build //////////
//
// Builds indirect draw command buffers on the GPU, using the visible instance list from this frame's Frustum Culling and LOD selection from this frame's LOD Selection.
// This is a compute pass that writes out a DrawIndexedIndirectArguments buffer for each draw bin (mesh x material), which is then consumed by the main GBuffer and Shadow Passes to execute GPU-driven indirect draws.

struct xiiDrawBuildData
{
  xiiRGBufferHandle m_hSurvivors;          ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRGBufferHandle m_hInstanceLOD;        ///< SRV in (structured buffer of uint, one per instance, packed LOD level + meshlet offset, from this frame's LOD Selection).
  xiiRGBufferHandle m_hDrawCommands;       ///< UAV out (structured buffer of DrawIndexedIndirectArguments, one per draw bin, consumed by GBuffer and Shadow Passes).
  xiiRGBufferHandle m_hDrawCounts;         ///< UAV out (structured buffer of uint, one per draw bin, used for indirect count in multi-draw scenarios).
  xiiUInt32         m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupDrawBuild(xiiDrawBuildData& data, xiiRGBuilder& builder)
{
  data.m_hSurvivors      = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, {}), xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceLOD    = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, {}), xiiGALResourceStateFlags::ShaderResource);
  data.m_uiInstanceCount = k_uiMaxInstances;

  // Persistent indirect argument buffer (resized lazily).
  const xiiUInt32 uiArgStride = 20U; // DrawIndexedIndirectArguments: 5 x uint
  if (!m_ViewPassResources.m_VisibilityPasses.m_pDrawIndirectArgBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                               = uiArgStride;
    description.m_uiSize                                            = uiArgStride * k_uiMaxMaterialBins;
    description.m_BindFlags                                         = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
    description.m_Mode                                              = xiiGALBufferMode::Formatted;
    description.m_Usage                                             = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_VisibilityPasses.m_pDrawIndirectArgBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }
  data.m_hDrawCommands = builder.ImportBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, m_ViewPassResources.m_VisibilityPasses.m_pDrawIndirectArgBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hDrawCommands = builder.WriteBuffer(data.m_hDrawCommands, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription counterBufferDescription;
  counterBufferDescription.m_uiElementByteStride = 4U;
  counterBufferDescription.m_uiSize              = 4U * k_uiMaxMaterialBins;
  counterBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  counterBufferDescription.m_Mode                = xiiGALBufferMode::Formatted;
  data.m_hDrawCounts                             = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawCountBuffer, counterBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pDrawBuildPipeline, "Shaders/Pipeline/DrawCommandBuild.xiiShader");
}

void xiiView::ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DrawCommandBuild");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pDrawBuildPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Survivors", context.GetBuffer(data.m_hSurvivors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceLOD", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawArgs", context.GetBuffer(data.m_hDrawCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawCounts", context.GetBuffer(data.m_hDrawCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Caster List Build //////////
//
// Builds a list of shadow-casting instances for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass.
// This is a compute pass that writes out a compact list of shadow-casting instance indices for the current frame, which is then consumed by the Shadow Passes.

struct xiiShadowCasterBuildData
{
  xiiRGBufferHandle m_hVisibleCandidates;    ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRGBufferHandle m_hShadowCasterCommands; ///< UAV out (structured buffer of uint, [0]=count, [1..]=indices of shadow-casting instances for current frame, consumed by Shadow Passes).
};

void xiiView::SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRGBuilder& builder)
{
  data.m_hVisibleCandidates = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, {}), xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 20U;                                                          // DrawIndexedIndirectArguments per cascade-per-bin
  description.m_uiSize              = description.m_uiElementByteStride * k_uiMaxMaterialBins * 4U; // 4 cascades
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  description.m_Mode                = xiiGALBufferMode::Formatted;
  data.m_hShadowCasterCommands      = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pShadowCasterBuildPipeline, "Shaders/Pipeline/ShadowCasterCulling.xiiShader");
}

void xiiView::ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowCasterListBuild");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pShadowCasterBuildPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawArgs", context.GetBuffer(data.m_hShadowCasterCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(k_uiMaxInstances + 63u) / 64u, 1u, 1u});
  }
  cmd.EndDebugGroup();
}

////////// GPU Cluster Build Data //////////
//
// Builds a cluster grid for clustered shading on the GPU, using the depth buffer from the current frame's Depth Pre-Pass.
// This is a compute pass that writes out a structured buffer of cluster descriptors, which is then consumed by the main lighting pass for light culling and shading.

struct xiiClusterBuildData
{
  xiiRGBufferHandle m_hClusterConstants;   ///< SRV in (structured buffer of cluster build constants, including cluster counts and depth range, consumed by the Cluster Build pass).
  xiiRGBufferHandle m_hClusterDescriptors; ///< UAV out (structured buffer of cluster descriptors, one per cluster, consumed by main lighting pass).
};

void xiiView::SetupClusterBuild(xiiClusterBuildData& data, xiiRGBuilder& builder)
{
  const xiiUInt32 uiClusterCountX = (m_Data.m_ViewPortRect.width + XII_CLUSTER_TILE_SIZE - 1U) / XII_CLUSTER_TILE_SIZE;
  const xiiUInt32 uiClusterCountY = (m_Data.m_ViewPortRect.height + XII_CLUSTER_TILE_SIZE - 1U) / XII_CLUSTER_TILE_SIZE;
  const xiiUInt32 uiTotalClusters = uiClusterCountX * uiClusterCountY * XII_CLUSTER_Z_SLICES;

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 32U; // float4 min + float4 max per cluster AABB
  description.m_uiSize              = description.m_uiElementByteStride * uiTotalClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hClusterDescriptors        = builder.WriteBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pClusterBuildPipeline, "Shaders/Pipeline/ClusterGridBuild.xiiShader");

  description.m_uiElementByteStride = 0;
  description.m_uiSize              = sizeof(xiiLightClusteringConstants);
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;

  data.m_hClusterConstants = builder.WriteBuffer("xiiLightClusteringConstants", description, xiiGALResourceStateFlags::ShaderResource);
}

void xiiView::ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ClusterGridBuild");
  {
    xiiUInt32 uiClusterCountX = (m_Data.m_ViewPortRect.width + XII_CLUSTER_TILE_SIZE - 1U) / XII_CLUSTER_TILE_SIZE;
    xiiUInt32 uiClusterCountY = (m_Data.m_ViewPortRect.height + XII_CLUSTER_TILE_SIZE - 1U) / XII_CLUSTER_TILE_SIZE;
    xiiUInt32 uiClusterCountZ = XII_CLUSTER_Z_SLICES;
    xiiUInt32 uiTotalClusters = uiClusterCountX * uiClusterCountY * uiClusterCountZ;

    {
      xiiGALMapHelper<xiiLightClusteringConstants> pClusteringConstants(cmd, context.GetBuffer(data.m_hClusterConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pClusteringConstants->ClusterCountX       = uiClusterCountX;
      pClusteringConstants->ClusterCountY       = uiClusterCountY;
      pClusteringConstants->ClusterCountZ       = uiClusterCountZ;
      pClusteringConstants->TotalClusters       = uiTotalClusters;
      pClusteringConstants->NearPlane           = m_pCamera->GetNearPlane();
      pClusteringConstants->FarPlane            = m_pCamera->GetFarPlane();
      pClusteringConstants->LogFarOverNear      = xiiMath::Log2(pClusteringConstants->FarPlane / pClusteringConstants->NearPlane);
      pClusteringConstants->TilePixelsX         = XII_CLUSTER_TILE_SIZE;
      pClusteringConstants->TilePixelsY         = XII_CLUSTER_TILE_SIZE;
      pClusteringConstants->MaxLightsPerCluster = XII_MAX_LIGHTS_PER_CLUSTER;
      pClusteringConstants->ActiveLightCount    = 0; ///< \todo : write actual count of active lights from extraction.
    }

    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pClusterBuildPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ClustersOut", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

    const xiiUInt32 uiGroups = (uiTotalClusters + 63U) / 64U;
    cmd.DispatchCompute({uiGroups, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Light List Build Data //////////
//
// Builds light lists for clustered shading on the GPU, using the cluster grid from this frame's Cluster Build pass and the list of active lights from extraction.
// This is a compute pass that writes out structured buffers of light indices per cluster, which are then consumed by the main lighting pass for light culling and shading.

struct xiiLightListData
{
  xiiRGBufferHandle m_hClusterDescriptors;    ///< SRV in (structured buffer of cluster descriptors, one per cluster, from this frame's Cluster Build).
  xiiRGBufferHandle m_hLightIndexBuffer;      ///< SRV in (structured buffer of uint, one per light, containing light type and other metadata, from extraction).
  xiiRGBufferHandle m_hLightGridBuffer;       ///< UAV out (structured buffer of uint, containing compact light lists per cluster, consumed by main lighting pass).
  xiiUInt32         m_uiActiveLightCount = 0; ///< Number of active lights to process (from extraction). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupLightListBuild(xiiLightListData& data, xiiRGBuilder& builder)
{
  data.m_hClusterDescriptors = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, {}), xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiMaxClusters    = 16U * 9U * 24U; // worst case
  const xiiUInt32 uiMaxLightsPerCl = 256U;

  xiiGALBufferCreationDescription indexBufferDescription;
  indexBufferDescription.m_uiElementByteStride = 4U;
  indexBufferDescription.m_uiSize              = 4U * uiMaxClusters * uiMaxLightsPerCl;
  indexBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  indexBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightIndexBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, indexBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription gridBufferDescription;
  gridBufferDescription.m_uiElementByteStride = 8U; // uint2 (offset, count) per cluster
  gridBufferDescription.m_uiSize              = gridBufferDescription.m_uiElementByteStride * uiMaxClusters;
  gridBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  gridBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightGridBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, gridBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiActiveLightCount = 1024; ///< \todo : driven by actual count of active lights from extraction.

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pLightListPipeline, "Shaders/Pipeline/LightListBuild.xiiShader");
}

void xiiView::ExecuteLightListBuild(const xiiLightListData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListBuild");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pLightListPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiActiveLightCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Proble Select Data //////////
//
// Selects relevant reflection probes for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of reflection probe indices and a bitmask of which probes affect which instances, which are then consumed by the main lighting pass for reflection probe sampling.

struct xiiReflectionProbeSelectData
{
  xiiRGBufferHandle m_hClusterDescriptors; ///< SRV in (structured buffer of cluster descriptors, one per cluster, from this frame's Cluster Build).
  xiiRGBufferHandle m_hProbeMask;          ///< UAV out (structured buffer of uint, one per instance, bitmask of which reflection probes affect each instance, consumed by main lighting pass).
};

void xiiView::SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRGBuilder& builder)
{
  data.m_hClusterDescriptors = builder.ReadBuffer(builder.DeclareBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, {}), xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = 4U * 1024; ///< \todo : use a well defined constant for a reasonable upper limit.
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hProbeMask                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pProbeSelectPipeline, "Shaders/Pipeline/GpuDrivenVisibilityCulling.xiiShader");
}

void xiiView::ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReflectionProbeSelection");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pProbeSelectPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ProbeMask", context.GetBuffer(data.m_hProbeMask)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Froxel Allocation Data //////////
//
// Allocates froxels for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of froxel metadata and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.

struct xiiFroxelAllocationData
{
  xiiRGBufferHandle  m_hFroxelMetadata;
  xiiRGTextureHandle m_hFroxelScattering;
  xiiUInt32          m_uiRenderWidth  = 1920U;
  xiiUInt32          m_uiRenderHeight = 1080U;
};

void xiiView::SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRGBuilder& builder)
{
  xiiGALBufferCreationDescription froxelMetadataBufferDescription;
  froxelMetadataBufferDescription.m_uiElementByteStride = 32U;                                                                      // per-froxel density + phase + absorption
  froxelMetadataBufferDescription.m_uiSize              = froxelMetadataBufferDescription.m_uiElementByteStride * 128U * 72U * 64U; // froxel volume
  froxelMetadataBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  froxelMetadataBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hFroxelMetadata                                = builder.WriteBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, froxelMetadataBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription scatteringBufferDescription;
  scatteringBufferDescription.m_Type               = xiiGALResourceDimension::Texture3D;
  scatteringBufferDescription.m_Format             = xiiGALResourceFormat::RGBA16Float;
  scatteringBufferDescription.m_Size.width         = 128U;
  scatteringBufferDescription.m_Size.height        = 72U;
  scatteringBufferDescription.m_uiArraySizeOrDepth = 64U;
  scatteringBufferDescription.m_uiMipLevels        = 1U;
  scatteringBufferDescription.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  scatteringBufferDescription.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hFroxelScattering                         = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, scatteringBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pFroxelSetupPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

void xiiView::ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricGridAllocation");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pFroxelSetupPipeline);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetaOut", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScatteringOut", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(128u + 7U) / 8U, (72U + 7U) / 8U, 8U});
  }
  cmd.EndDebugGroup();
}

void xiiView::BuildStage1_Visibility(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  graph.AddPass<xiiOcclusionReadbackData>("GpuOcclusionReadback", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupOcclusionReadback, this), xiiMakeDelegate(&xiiView::ExecuteOcclusionReadback, this));
  graph.AddPass<xiiFrustumCullData>("FrustumCulling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupFrustumCull, this), xiiMakeDelegate(&xiiView::ExecuteFrustumCull, this));
  graph.AddPass<xiiLODSelectData>("LODSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLODSelect, this), xiiMakeDelegate(&xiiView::ExecuteLODSelect, this));
  graph.AddPass<xiiDrawBuildData>("DrawCommandBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDrawBuild, this), xiiMakeDelegate(&xiiView::ExecuteDrawBuild, this));
  graph.AddPass<xiiShadowCasterBuildData>("ShadowCasterListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowCasterBuild, this), xiiMakeDelegate(&xiiView::ExecuteShadowCasterBuild, this));
  graph.AddPass<xiiInstanceUpdateData>("InstanceUpdate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupInstanceUpdate, this), xiiMakeDelegate(&xiiView::ExecuteInstanceUpdate, this));
  graph.AddPass<xiiClusterBuildData>("ClusterGridBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupClusterBuild, this), xiiMakeDelegate(&xiiView::ExecuteClusterBuild, this));
  graph.AddPass<xiiLightListData>("LightListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLightListBuild, this), xiiMakeDelegate(&xiiView::ExecuteLightListBuild, this));
  graph.AddPass<xiiReflectionProbeSelectData>("ReflectionProbeSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReflectionProbeSelect, this), xiiMakeDelegate(&xiiView::ExecuteReflectionProbeSelect, this));
  graph.AddPass<xiiFroxelAllocationData>("VolumetricGridAllocation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupFroxelAllocation, this), xiiMakeDelegate(&xiiView::ExecuteFroxelAllocation, this));
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
