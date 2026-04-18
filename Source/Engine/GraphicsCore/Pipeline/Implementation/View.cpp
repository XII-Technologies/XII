#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/HiZPyramid/HiZBuildConstants.h>
#include <Shaders/Pipeline/Passes/LightClustering/LightClusteringConstants.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

xiiCVarFloat cvar_DynamicRenderingTargetMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");
xiiCVarFloat cvar_DynamicRenderingMinScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");
xiiCVarFloat cvar_DynamicRenderingMaxScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "Maximum allowed render scale (1.0 = native resolution).");

namespace
{
  // Shared constants (sizes of persistent GPU buffers, aligned to typical instance budgets)
  static constexpr xiiUInt32 k_uiMaxInstances    = 65536U; ///< The maximum number of drawable objects in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.
  static constexpr xiiUInt32 k_uiMaxLights       = 1024U;  ///< The maximum number of active lights in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.
  static constexpr xiiUInt32 k_uiMaxMaterialBins = 512U;   ///< The maximum number of distinct (mesh x material) draw bins in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.

  static constexpr xiiUInt32 k_uiMaxReflectionProbes = 64U; ///< The maximum number of active reflection probes in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.

  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasWidth  = 4096U; ///< The width of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The height will be the same as the width, and each cascade will be allocated a quadrant of the atlas.
  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasHeight = 4096U; ///< The height of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The width will be the same as the height, and each cascade will be allocated a quadrant of the atlas.
  static constexpr xiiUInt32 k_uiLocalShadowAtlasSize         = 4096U; ///< The size of the local shadow atlas. This should be sized to fit the maximum number of local shadows in one frame. The atlas will be a single 2D texture for spot and point lights.

  static bool IsRenderDataTypeName(const xiiRenderData* pRenderData, xiiStringView sTypeName)
  {
    const xiiRTTI* pType = pRenderData != nullptr ? pRenderData->GetDynamicRTTI() : nullptr;
    return pType != nullptr && pType->GetTypeName().IsEqual_NoCase(sTypeName);
  }

  static xiiUInt32 CountRenderDataByTypeName(const xiiArrayPtr<xiiRenderData* const>& renderData, xiiStringView sTypeName)
  {
    xiiUInt32 uiCount = 0;
    for (const xiiRenderData* pRenderData : renderData)
    {
      if (IsRenderDataTypeName(pRenderData, sTypeName))
      {
        ++uiCount;
      }
    }

    return uiCount;
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiView::xiiView()
{
  m_pRenderGraph = XII_DEFAULT_NEW(xiiRenderGraph);

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
  data.m_uiReadSlot = (m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;

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
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
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
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
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
  data.m_hSurvivors      = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceLOD    = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, xiiGALResourceStateFlags::ShaderResource);
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
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);

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
  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);

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
  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = 4U * k_uiMaxReflectionProbes; ///< \todo : use a well defined constant for a reasonable upper limit.
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
    cmd.DispatchCompute({(k_uiMaxReflectionProbes + 63U) / 64U, 1U, 1U});
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

////////// GPU Shadow Cascade Setup Data //////////
//
// Sets up shadow cascades for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of cascade matrices and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.

struct xiiShadowCascadeSetupData
{
  xiiRGBufferHandle m_hCascadeMatrices;                              ///< UAV out (structured buffer of float4x4 cascade view-projection matrices, one per cascade, consumed by Shadow Passes).
  xiiUInt32         m_uiActiveCascades = 0U;                         ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes.
  xiiVec3           m_vLightDir        = xiiVec3(0.0f, -1.0f, 0.0f); ///< Direction of the main directional light, used for computing cascade splits and matrices.
  float             m_fNearPlane       = 0.1f;                       ///< Near plane distance for shadow cascades, used for computing cascade splits and matrices.
  float             m_fFarPlane        = 1000.0f;                    ///< Far plane distance for shadow cascades, used for computing cascade splits and matrices.
};

void xiiView::SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRGBuilder& builder)
{
  data.m_uiActiveCascades = 3U;
  data.m_vLightDir        = xiiVec3(0.0f, -1.0f, 0.0f);
  data.m_fNearPlane       = m_pCamera->GetNearPlane();
  data.m_fFarPlane        = m_pCamera->GetFarPlane();

  // Walk extracted data to find the first directional light.
  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  for (xiiRenderData* pRenderData : renderData)
  {
    if (IsRenderDataTypeName(pRenderData, "xiiDirectionalLightRenderData"))
    {
      data.m_vLightDir        = -pRenderData->m_GlobalTransform.GetColumn(2).GetAsVec3().GetNormalized();
      data.m_uiActiveCascades = 3U; // Could read from component property via msg if exposed.
      break;
    }
  }

  // GPU buffer: ShadowCascadeConstants (float4x4[4] + float4 + uint + pad3)
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiShadowCascadeConstants);
  description.m_uiSize              = description.m_uiElementByteStride;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hCascadeMatrices           = builder.WriteBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pCascadeSetupPipeline, "Shaders/Pipeline/ShadowCascadeSetup.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteShadowCascadeSetup(const xiiShadowCascadeSetupData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowCascadeSetup");
  {
    {
      xiiGALMapHelper<xiiShadowCascadeConstants> pConstants(cmd, context.GetBuffer(data.m_hCascadeMatrices), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pConstants->ActiveCascadeCount = data.m_uiActiveCascades;

      // Cascade split depths: practical split scheme based on camera range.
      const float fRange = data.m_fFarPlane - data.m_fNearPlane;
      for (xiiUInt32 i = 0; i < 4; ++i)
      {
        const float t                               = static_cast<float>(i + 1) / 4.0f;
        pConstants->CascadeSplitDepths.GetData()[i] = data.m_fNearPlane + fRange * t * t; // quadratic split
      }

      // Cascade view-projection matrices are computed on CPU, written once per directional light.
      // (Full implementation would call xiiView::ComputeCascadeViewProjection; simplified for now.)
      for (xiiUInt32 i = 0; i < 4; ++i)
      {
        pConstants->CascadeViewProjection[i] = xiiMat4::MakeIdentity();
      }
    }

    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pCascadeSetupPipeline);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_CascadeOut", context.GetBuffer(data.m_hCascadeMatrices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({1U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Directional Shadow Data //////////
//
// Collects all GPU resources related to directional shadow rendering for the current frame, including cascade matrices, shadow caster lists, and shadow atlases.

struct xiiDirectionalShadowData
{
  xiiRGBufferHandle  m_hCascadeMatrices;        ///< SRV in (structured buffer of float4x4 cascade view-projection matrices, one per cascade, from this frame's Shadow Cascade Setup pass).
  xiiRGBufferHandle  m_hShadowCasterCommands;   ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per cascade-per-bin, from this frame's Shadow Caster Build pass).
  xiiRGTextureHandle m_hDirectionalShadowAtlas; ///< SRV in (texture atlas for directional shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32          m_uiActiveCascades = 3U;   ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes and main lighting pass.
};

void xiiView::SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRGBuilder& builder)
{
  // Persistent directional shadow atlas (D32 float array of 4 slices).
  if (!m_ViewPassResources.m_ShadowPasses.m_pDirectionalShadowAtlas)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                           = xiiGALResourceDimension::Texture2DArray;
    description.m_Format                                         = xiiGALResourceFormat::D32Float;
    description.m_Size.width                                     = k_uiDirectionalShadowAtlasWidth;
    description.m_Size.height                                    = k_uiDirectionalShadowAtlasHeight;
    description.m_uiArraySizeOrDepth                             = 4U;
    description.m_uiMipLevels                                    = 1U;
    description.m_BindFlags                                      = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                          = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_ShadowPasses.m_pDirectionalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hCascadeMatrices        = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ShaderResource);
  data.m_hShadowCasterCommands   = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hDirectionalShadowAtlas = builder.ImportTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, m_ViewPassResources.m_ShadowPasses.m_pDirectionalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hDirectionalShadowAtlas = builder.WriteTexture(data.m_hDirectionalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_uiActiveCascades        = 3U;

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DirectionalShadowMaps");
  {
    xiiGALTexture* pAtlas = context.GetTexture(data.m_hDirectionalShadowAtlas);

    xiiStringBuilder sb;
    for (xiiUInt32 uiCascade = 0; uiCascade < data.m_uiActiveCascades; ++uiCascade)
    {
      xiiGALScopedDebugGroup debugGroup(cmd, sb);

      // Bind atlas slice as depth-stencil.
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
      viewDescription.m_uiFirstArrayOrDepthSlice  = uiCascade;
      viewDescription.m_uiArrayOrDepthSlicesCount = 1U;

      // Set viewport matching atlas tile.
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(k_uiDirectionalShadowAtlasWidth), static_cast<float>(k_uiDirectionalShadowAtlasHeight), 0.0f, 1.0f});
      cmd.ClearDepthStencilView(pAtlas->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);

      if (m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline)
      {
        cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline);

        xiiGALDrawIndexedIndirectDescription indexedIndirectDrawDescription;
        indexedIndirectDrawDescription.m_IndexType             = xiiGALValueType::UInt32;
        indexedIndirectDrawDescription.m_pBuffer               = context.GetBuffer(data.m_hShadowCasterCommands);
        indexedIndirectDrawDescription.m_uiDrawArgumentOffset  = uiCascade * 20U; // offset per cascade DrawIndexedIndirectArguments (uint index count, uint instance count, uint start index location, int base vertex location, uint start instance location).
        indexedIndirectDrawDescription.m_uiDrawArgumentStride  = 20U;             // stride per cascade DrawIndexedIndirectArguments (uint index count, uint instance count, uint start index location, int base vertex location, uint start instance location).
        indexedIndirectDrawDescription.m_uiDrawCount           = 1U;              // one draw call per cascade, with instance count in argument buffer specifying how many instances to draw for that cascade.
        indexedIndirectDrawDescription.m_BufferStateTransition = xiiGALStateTransitionMode::Transition;

        // Cascade index uploaded via push constant / cbuffer update.
        cmd.DrawIndexedIndirect(indexedIndirectDrawDescription);
      }
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Spot Shadow Data //////////
//
// Collects all GPU resources related to spot shadow rendering for the current frame, including shadow caster lists and shadow atlases.

struct xiiSpotShadowData
{
  xiiRGBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per spot light, from this frame's Shadow Caster Build pass).
  xiiRGTextureHandle m_hLocalShadowAtlas;     ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32          m_uiSpotLightCount = 0;  ///< Number of active spot lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

void xiiView::SetupSpotShadowData(xiiSpotShadowData& data, xiiRGBuilder& builder)
{
  if (!m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlas)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                     = xiiGALResourceDimension::Texture2D;
    description.m_Format                                   = xiiGALResourceFormat::D32Float;
    description.m_Size.width                               = k_uiLocalShadowAtlasSize;
    description.m_Size.height                              = k_uiLocalShadowAtlasSize;
    description.m_uiMipLevels                              = 1U;
    description.m_BindFlags                                = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                    = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hShadowCasterCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlas     = builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas     = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiSpotLightCount                            = CountRenderDataByTypeName(renderData, "xiiSpotLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  if (data.m_uiSpotLightCount == 0u || !m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("SpotLightShadows");
  {
    xiiGALTexture* pAtlas = context.GetTexture(data.m_hLocalShadowAtlas);

    // For each spot light, render into its atlas tile.
    // Atlas allocation managed by LocalLightShadowAtlasAllocation pass (deferred to full impl).
    cmd.ClearDepthStencilView(pAtlas->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline);
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(k_uiLocalShadowAtlasSize), static_cast<float>(k_uiLocalShadowAtlasSize), 0.0f, 1.0f});
    cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands)}); // // Indirect multi-draw from shadow caster argument buffer.
  }
  cmd.EndDebugGroup();
}

////////// GPU Point Shadow Data //////////
//
// Collects all GPU resources related to point shadow rendering for the current frame, including shadow caster lists and shadow atlases.

struct xiiPointShadowData
{
  xiiRGBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per point light, from this frame's Shadow Caster Build pass).
  xiiRGTextureHandle m_hLocalShadowAtlas;     ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32          m_uiPointLightCount = 0; ///< Number of active point lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

void xiiView::SetupPointShadowData(xiiPointShadowData& data, xiiRGBuilder& builder)
{
  data.m_hShadowCasterCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlas     = builder.WriteTexture(builder.DeclareTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, {}), xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiPointLightCount                           = CountRenderDataByTypeName(renderData, "xiiPointLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecutePointShadowData(const xiiPointShadowData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  if (data.m_uiPointLightCount == 0U || !m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("PointLightCubeShadows");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDepthPipeline);

    // Each point light: 6 draw calls placing results into 6 atlas tiles.
    for (xiiUInt32 uiFace = 0; uiFace < data.m_uiPointLightCount * 6U; ++uiFace)
    {
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands), 1U, uiFace * 20U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray-Traced Shadow Data //////////
//
// Collects all GPU resources related to ray-traced shadow rendering for the current frame, including raw shadow masks and scene depth.

struct xiiRayTracedShadowData
{
  xiiRGTextureHandle m_hRTRawShadowMask; ///< UAV out (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by Shadow Denoise Pass).
  xiiRGTextureHandle m_hSceneDepth;      ///< SRV in (depth texture from main render pass, used for ray-traced shadow ray generation and occlusion testing).
};

void xiiView::SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1u;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTRawShadowMask   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline, "Shaders/Pipeline/RTShadow.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("Ray-Traced Shadows");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline); // reusing slot for RT pipeline
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(context.GetBlackboard().GetRef<xiiUInt32>(xiiRGBlackboardKeys::k_RenderWidth) + 7U) / 8U, (context.GetBlackboard().GetRef<xiiUInt32>(xiiRGBlackboardKeys::k_RenderHeight) + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Denoise Data //////////
//
// Collects all GPU resources related to shadow denoising for the current frame, including raw shadow masks and final shadow masks.

struct xiiShadowDenoiseData
{
  xiiRGTextureHandle m_hRTRawShadowMask;   ///< SRV in (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by this pass).
  xiiRGTextureHandle m_hRTFinalShadowMask; ///< UAV out (texture containing final denoised ray-traced shadow masks, written by this pass, read by main lighting pass).
};

void xiiView::SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRGBuilder& builder)
{
  data.m_hRTRawShadowMask = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTFinalShadowMask = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

void xiiView::ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowDenoise");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Input", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Contact Shadow Data //////////
//
// Collects all GPU resources related to contact shadow rendering for the current frame, including scene depth and contact shadow masks.

struct xiiContactShadowData
{
  xiiRGTextureHandle m_hSceneDepth;    ///< SRV in (depth texture from main render pass, used for contact shadow ray generation and occlusion testing).
  xiiRGTextureHandle m_hContactShadow; ///< UAV out (texture containing contact shadow masks, written by this pass, read by main lighting pass).
};

void xiiView::SetupContactShadowData(xiiContactShadowData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_Type           = xiiGALResourceDimension::Texture2D;
  desc.m_Format         = xiiGALResourceFormat::R8UNormalized;
  desc.m_Size.width     = m_Data.m_ViewPortRect.width;
  desc.m_Size.height    = m_Data.m_ViewPortRect.height;
  desc.m_uiMipLevels    = 1U;
  desc.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hContactShadow = builder.WriteTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

void xiiView::ExecuteContactShadowData(const xiiContactShadowData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ContactShadows");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pContactShadowPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ContactShadowOut", context.GetTexture(data.m_hContactShadow)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Depth Prepass Data //////////
//
// Collects all GPU resources related to depth prepass rendering for the current frame, including the scene depth target and indirect draw commands.

struct xiiDepthPrepassData
{
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthStencil out (full-resolution reversed-Z scene depth, written by this pass and consumed by later depth-dependent passes).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin, from this frame's Draw Build pass).
};

void xiiView::SetupDepthPrepass(xiiDepthPrepassData& data, xiiRGBuilder& builder)
{
  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::D32Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSceneDepth        = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, description, xiiGALResourceStateFlags::DepthWrite);

  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDepthPrepass(const xiiDepthPrepassData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DepthPrepass");
  {
    xiiGALTexture* pDepth = context.GetTexture(data.m_hSceneDepth);
    cmd.ClearDepthStencilView(pDepth->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_DepthPasses.m_pDepthPrepassPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pDepthPrepassPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Pyramid Data //////////
//
// Collects all GPU resources related to hierarchical depth generation for the current frame, including the scene depth source and Hi-Z pyramid target.

struct xiiHiZPyramidData
{
  xiiRGTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture written by Depth Prepass, used as mip-0 source for Hi-Z generation).
  xiiRGTextureHandle m_hHiZPyramid;      ///< UnorderedAccess out (R32F max-depth hierarchy texture, consumed by Hi-Z occlusion culling and depth-aware effects).
  xiiUInt32          m_uiMipLevels = 1U; ///< Number of mips in the Hi-Z pyramid, derived from the current viewport size.
};

void xiiView::SetupHiZPyramid(xiiHiZPyramidData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiBaseWidth  = xiiMath::Max(static_cast<xiiUInt32>(m_Data.m_ViewPortRect.width), 1U);
  const xiiUInt32 uiBaseHeight = xiiMath::Max(static_cast<xiiUInt32>(m_Data.m_ViewPortRect.height), 1U);

  xiiUInt32 uiMipWidth  = uiBaseWidth;
  xiiUInt32 uiMipHeight = uiBaseHeight;
  while (uiMipWidth > 1U || uiMipHeight > 1U)
  {
    uiMipWidth  = xiiMath::Max(uiMipWidth >> 1U, 1U);
    uiMipHeight = xiiMath::Max(uiMipHeight >> 1U, 1U);
    ++data.m_uiMipLevels;
  }

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R32Float;
  description.m_Size.width  = uiBaseWidth;
  description.m_Size.height = uiBaseHeight;
  description.m_uiMipLevels = data.m_uiMipLevels;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHiZPyramid        = builder.WriteTexture(xiiRGBlackboardKeys::k_HiZPyramid, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_DepthPasses.m_pHiZBuildPipeline, "Shaders/Pipeline/HiZBuild.xiiShader");
}

void xiiView::ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZPyramid");
  {
    if (m_ViewPassResources.m_DepthPasses.m_pHiZBuildPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pHiZBuildPipeline);

      xiiGALTexture* pDepth = context.GetTexture(data.m_hSceneDepth);
      xiiGALTexture* pHiZ   = context.GetTexture(data.m_hHiZPyramid);

      xiiUInt32 uiSrcWidth  = xiiMath::Max(static_cast<xiiUInt32>(m_Data.m_ViewPortRect.width), 1U);
      xiiUInt32 uiSrcHeight = xiiMath::Max(static_cast<xiiUInt32>(m_Data.m_ViewPortRect.height), 1U);

      for (xiiUInt32 uiMip = 0U; uiMip + 1U < data.m_uiMipLevels; ++uiMip)
      {
        const xiiUInt32 uiDestinationWidth  = xiiMath::Max(uiSrcWidth >> 1U, 1U);
        const xiiUInt32 uiDestinationHeight = xiiMath::Max(uiSrcHeight >> 1U, 1U);

        if (uiMip == 0U)
        {
          cmd.ResolveAndSetShaderResourceTextureView("g_DepthSrc", pDepth->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
        }
        else
        {
          cmd.ResolveAndSetShaderResourceTextureView("g_DepthSrc", pHiZ->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
        }

        cmd.ResolveAndSetUnorderedAccessTextureView("g_HiZOut", pHiZ->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
        cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
        cmd.DispatchCompute({(uiDestinationWidth + 7U) / 8U, (uiDestinationHeight + 7U) / 8U, 1U});

        uiSrcWidth  = uiDestinationWidth;
        uiSrcHeight = uiDestinationHeight;
      }
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Occlusion Culling Data //////////
//
// Collects all GPU resources related to Hi-Z occlusion culling for the current frame, including the Hi-Z pyramid input, candidate instance list, and surviving output list.

struct xiiHiZOcclusionCullData
{
  xiiRGTextureHandle m_hHiZPyramid;         ///< ShaderResource in (Hi-Z pyramid generated by this frame's Hi-Z Pyramid pass).
  xiiRGBufferHandle  m_hVisibleCandidates;  ///< ShaderResource in (visible instance candidate list generated by this frame's Frustum Culling pass).
  xiiRGBufferHandle  m_hSurvivingInstances; ///< UnorderedAccess out (instance list surviving Hi-Z occlusion culling, consumed by later depth/lighting passes).
  xiiRGBufferHandle  m_hInstanceBounds;     ///< ShaderResource in (instance bounds buffer for occlusion testing).
  xiiUInt32          m_uiInstanceCount = 0; ///< Number of instances to process.
};

void xiiView::SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRGBuilder& builder)
{
  data.m_hHiZPyramid        = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = 4U + 4U * k_uiMaxInstances;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hSurvivingInstances        = builder.WriteBuffer(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiInstanceCount = k_uiMaxInstances;

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_DepthPasses.m_pHiZOcclusionCullPipeline, "Shaders/Pipeline/HiZOcclusionCulling.xiiShader");
}

void xiiView::ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZOcclusionCull");
  {
    if (m_ViewPassResources.m_DepthPasses.m_pHiZOcclusionCullPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pHiZOcclusionCullPipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_HiZPyramid", context.GetTexture(data.m_hHiZPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Candidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_Survivors", context.GetBuffer(data.m_hSurvivingInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Motion Vectors Data //////////
//
// Collects all GPU resources related to motion vector rendering for the current frame, including scene depth and the velocity render target.

struct xiiMotionVectorsData
{
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthStencil inout (scene depth target reused for depth-tested motion vector rendering).
  xiiRGTextureHandle m_hVelocityBuffer;       ///< RenderTarget out (screen-space velocity buffer written by this pass).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupMotionVectors(xiiMotionVectorsData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityBuffer    = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MotionVectors");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hVelocityBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_DepthPasses.m_pMotionVectorPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pMotionVectorPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Velocity Dilation Data //////////
//
// Collects all GPU resources related to velocity dilation for the current frame, including the input velocity texture and the dilated velocity output texture.

struct xiiVelocityDilationData
{
  xiiRGTextureHandle m_hVelocityInput;   ///< ShaderResource in (screen-space velocity buffer generated by the Motion Vectors pass).
  xiiRGTextureHandle m_hVelocityDilated; ///< UnorderedAccess out (dilated velocity buffer replacing the velocity blackboard key for downstream consumers).
};

void xiiView::SetupVelocityDilation(xiiVelocityDilationData& data, xiiRGBuilder& builder)
{
  data.m_hVelocityInput = builder.ReadTexture(xiiRGBlackboardKeys::k_VelocityBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityDilated   = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_DepthPasses.m_pVelocityDilationPipeline, "Shaders/Pipeline/Downscale.xiiShader");
}

void xiiView::ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VelocityDilation");
  {
    if (m_ViewPassResources.m_DepthPasses.m_pVelocityDilationPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pVelocityDilationPipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_Input", context.GetTexture(data.m_hVelocityInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hVelocityDilated)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU G-Buffer Base Data //////////
//
// Collects all GPU resources related to base G-Buffer generation for the current frame, including the scene depth input, four G-Buffer targets, and indirect draw commands.

struct xiiGBufferBaseData
{
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested G-Buffer rendering).
  xiiRGTextureHandle m_hGBufferAlbedo;        ///< RenderTarget out (albedo and AO target).
  xiiRGTextureHandle m_hGBufferNormal;        ///< RenderTarget out (encoded normal target).
  xiiRGTextureHandle m_hGBufferMaterial;      ///< RenderTarget out (material properties target).
  xiiRGTextureHandle m_hGBufferEmissive;      ///< RenderTarget out (emissive target).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupGBufferBase(xiiGBufferBaseData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;

  description.m_Format  = xiiGALResourceFormat::RGBA8UNormalized;
  data.m_hGBufferAlbedo = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format  = xiiGALResourceFormat::RG16SNormalized;
  data.m_hGBufferNormal = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferNormal, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  data.m_hGBufferMaterial = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferMaterial, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format    = xiiGALResourceFormat::RGBA16Float;
  data.m_hGBufferEmissive = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferEmissive, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteGBufferBase(const xiiGBufferBaseData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GBufferBase");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.0f, 1.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());

    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_GBufferPasses.m_pGBufferPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_GBufferPasses.m_pGBufferPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Normal Roughness Prepass Data //////////
//
// Collects all GPU resources related to compact normal-roughness generation for the current frame.

struct xiiNormalRoughnessPrepassData
{
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested rendering).
  xiiRGTextureHandle m_hNormalRoughness;      ///< RenderTarget out (compact normal/roughness/specular buffer consumed by GTAO and lighting prep passes).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupNormalRoughnessPrepass(xiiNormalRoughnessPrepassData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hNormalRoughness   = builder.WriteTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteNormalRoughnessPrepass(const xiiNormalRoughnessPrepassData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("NormalRoughnessPrepass");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.5f, 1.0f));
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_GBufferPasses.m_pNormalRoughnessPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_GBufferPasses.m_pNormalRoughnessPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU BRDF LUT Generation Data //////////
//
// Collects all GPU resources related to BRDF LUT generation, persisted across frames.

struct xiiBRDFLutGenerationData
{
  xiiRGTextureHandle m_hBRDFLut;                 ///< Imported persistent BRDF LUT texture.
  bool               m_bNeedsGeneration = false; ///< Whether this frame must dispatch BRDF LUT generation.
};

void xiiView::SetupBRDFLutGeneration(xiiBRDFLutGenerationData& data, xiiRGBuilder& builder)
{
  if (!m_ViewPassResources.m_LightingPrepPasses.m_pBRDFLut)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                  = xiiGALResourceDimension::Texture2D;
    description.m_Format                                = xiiGALResourceFormat::RG16Float;
    description.m_Size.width                            = 256U;
    description.m_Size.height                           = 256U;
    description.m_uiMipLevels                           = 1U;
    description.m_BindFlags                             = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                 = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_LightingPrepPasses.m_pBRDFLut = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_bNeedsGeneration = !m_ViewPassResources.m_LightingPrepPasses.m_bBRDFLutGenerated;

  data.m_hBRDFLut = builder.ImportTexture(xiiRGBlackboardKeys::k_BRDFLut, m_ViewPassResources.m_LightingPrepPasses.m_pBRDFLut, data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);

  if (data.m_bNeedsGeneration)
  {
    data.m_hBRDFLut = builder.WriteTexture(data.m_hBRDFLut, xiiGALResourceStateFlags::UnorderedAccess);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pBRDFLutPipeline, "Shaders/Pipeline/BRDFLUTGenerate.xiiShader");
}

void xiiView::ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRGPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("BRDFLUTGenerate");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pBRDFLutPipeline);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BRDFLutOut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({32U, 32U, 1U});
    m_ViewPassResources.m_LightingPrepPasses.m_bBRDFLutGenerated = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Transmittance Data //////////
//
// Collects all GPU resources related to atmosphere transmittance LUT generation, persisted across frames.

struct xiiAtmosphereTransmittanceData
{
  xiiRGTextureHandle m_hTransmittanceLUT;        ///< Imported persistent atmosphere transmittance LUT texture.
  bool               m_bNeedsGeneration = false; ///< Whether this frame must dispatch transmittance LUT generation.
};

void xiiView::SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRGBuilder& builder)
{
  if (!m_ViewPassResources.m_LightingPrepPasses.m_pAtmTransmittanceLUT)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                              = xiiGALResourceDimension::Texture2D;
    description.m_Format                                            = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width                                        = 256U;
    description.m_Size.height                                       = 64U;
    description.m_uiMipLevels                                       = 1U;
    description.m_BindFlags                                         = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                             = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_LightingPrepPasses.m_pAtmTransmittanceLUT = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    data.m_bNeedsGeneration                                         = true;
  }

  data.m_hTransmittanceLUT = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, m_ViewPassResources.m_LightingPrepPasses.m_pAtmTransmittanceLUT, data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);

  if (data.m_bNeedsGeneration)
  {
    data.m_hTransmittanceLUT = builder.WriteTexture(data.m_hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pAtmTransmittancePipeline, "Shaders/Pipeline/AtmosphereTransmittance.xiiShader");
}

void xiiView::ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRGPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereTransmittanceLUT");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pAtmTransmittancePipeline);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TransmittanceOut", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({32U, 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Multi-Scatter Data //////////
//
// Collects all GPU resources related to atmosphere multi-scatter LUT generation, persisted across frames.

struct xiiAtmosphereMultiScatterData
{
  xiiRGTextureHandle m_hMultiScatterLUT;         ///< Imported persistent atmosphere multi-scatter LUT texture.
  xiiRGTextureHandle m_hTransmittanceLUT;        ///< ShaderResource in (atmosphere transmittance LUT).
  bool               m_bNeedsGeneration = false; ///< Whether this frame must dispatch multi-scatter LUT generation.
};

void xiiView::SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRGBuilder& builder)
{
  if (!m_ViewPassResources.m_LightingPrepPasses.m_pAtmMultiScatterLUT)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                             = xiiGALResourceDimension::Texture2D;
    description.m_Format                                           = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width                                       = 32U;
    description.m_Size.height                                      = 32U;
    description.m_uiMipLevels                                      = 1U;
    description.m_BindFlags                                        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                            = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_LightingPrepPasses.m_pAtmMultiScatterLUT = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    data.m_bNeedsGeneration                                        = true;
  }

  data.m_hTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMultiScatterLUT  = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, m_ViewPassResources.m_LightingPrepPasses.m_pAtmMultiScatterLUT, data.m_bNeedsGeneration ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);

  if (data.m_bNeedsGeneration)
  {
    data.m_hMultiScatterLUT = builder.WriteTexture(data.m_hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pAtmMultiScatterPipeline, "Shaders/Pipeline/AtmosphereMultiScatter.xiiShader");
}

void xiiView::ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRGPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereMultiScatterLUT");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pAtmMultiScatterPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_MultiScatterOut", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({4U, 4U, 1U});
    m_ViewPassResources.m_LightingPrepPasses.m_bAtmLutsGenerated = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Sky Irradiance Convolution Data //////////
//
// Collects all GPU resources related to sky irradiance convolution.

struct xiiSkyIrradianceConvolutionData
{
  xiiRGTextureHandle m_hTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRGTextureHandle m_hMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRGTextureHandle m_hSkyRadiance;      ///< UnorderedAccess out (sky radiance texture used by later lighting passes).
};

void xiiView::SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRGBuilder& builder)
{
  data.m_hTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSkyRadiance       = builder.WriteTexture(xiiRGBlackboardKeys::k_SkyRadiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pSkyIrradiancePipeline, "Shaders/Pipeline/ReflectionIrradiance.xiiShader");
}

void xiiView::ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SkyIrradianceConvolution");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pSkyIrradiancePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SkyOut", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Probe Convolution Data //////////
//
// Collects all GPU resources related to reflection probe specular convolution.

struct xiiReflectionProbeConvolutionData
{
  xiiRGBufferHandle  m_hReflectionProbeMask; ///< ShaderResource in (per-probe visibility/selection mask).
  xiiRGTextureHandle m_hBRDFLut;             ///< ShaderResource in (precomputed BRDF LUT for filtered specular).
};

void xiiView::SetupReflectionProbeConvolution(xiiReflectionProbeConvolutionData& data, xiiRGBuilder& builder)
{
  data.m_hReflectionProbeMask = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut             = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pReflProbeConvPipeline, "Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
}

void xiiView::ExecuteReflectionProbeConvolution(const xiiReflectionProbeConvolutionData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReflectionProbeConvolution");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pReflProbeConvPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ProbeMask", context.GetBuffer(data.m_hReflectionProbeMask)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({8U, 8U, 6U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Initialization Data //////////
//
// Collects all GPU resources related to volumetric fog froxel initialization.

struct xiiVolumetricFogInitializationData
{
  xiiRGBufferHandle  m_hFroxelMetadata;   ///< ShaderResource in (froxel metadata buffer).
  xiiRGTextureHandle m_hFroxelScattering; ///< UnorderedAccess inout (froxel scattering texture).
};

void xiiView::SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRGBuilder& builder)
{
  data.m_hFroxelMetadata   = builder.ReadBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hFroxelScattering = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pFroxelFogInitPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

void xiiView::ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogInitialization");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pFroxelFogInitPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_FroxelMeta", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScatterOut", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 8U});
  }
  cmd.EndDebugGroup();
}

////////// GPU DDGI Probe Sampling Data //////////
//
// Collects all GPU resources related to DDGI final gather probe sampling.

struct xiiDDGIProbeSamplingData
{
  xiiRGTextureHandle m_hDDGIIrradiance; ///< UnorderedAccess out (DDGI irradiance result texture).
  xiiRGTextureHandle m_hSceneDepth;     ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hGBufferNormal;  ///< ShaderResource in (GBuffer normal texture).
};

void xiiView::SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDDGIIrradiance    = builder.WriteTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pDDGIProbePipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

void xiiView::ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DDGIProbeSampling");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pDDGIProbePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DDGIOut", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Data //////////
//
// Collects all GPU resources related to ground-truth ambient occlusion generation.

struct xiiGroundTruthAmbientOcclusionData
{
  xiiRGTextureHandle m_hSceneDepth;          ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hNormalRoughness;     ///< ShaderResource in (normal/roughness buffer).
  xiiRGTextureHandle m_hRawAmbientOcclusion; ///< UnorderedAccess out (raw ambient occlusion result).
};

void xiiView::SetupGroundTruthAmbientOcclusion(xiiGroundTruthAmbientOcclusionData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hNormalRoughness = builder.ReadTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type          = xiiGALResourceDimension::Texture2D;
  description.m_Format        = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width    = m_Data.m_ViewPortRect.width;
  description.m_Size.height   = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels   = 1U;
  description.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage         = xiiGALResourceUsage::Default;
  data.m_hRawAmbientOcclusion = builder.WriteTexture(xiiRGBlackboardKeys::k_RawAOTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pGTAOPipeline, "Shaders/Pipeline/GTAO.xiiShader");
}

void xiiView::ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusion");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pGTAOPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Denoise Data //////////
//
// Collects all GPU resources related to denoising ground-truth ambient occlusion.

struct xiiGroundTruthAmbientOcclusionDenoiseData
{
  xiiRGTextureHandle m_hRawAmbientOcclusion;    ///< ShaderResource in (raw ambient occlusion texture).
  xiiRGTextureHandle m_hStableAmbientOcclusion; ///< UnorderedAccess out (denoised ambient occlusion texture).
};

void xiiView::SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRGBuilder& builder)
{
  data.m_hRawAmbientOcclusion = builder.ReadTexture(xiiRGBlackboardKeys::k_RawAOTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width       = m_Data.m_ViewPortRect.width;
  description.m_Size.height      = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hStableAmbientOcclusion = builder.WriteTexture(xiiRGBlackboardKeys::k_StableAOTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pGTAODenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

void xiiView::ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusionDenoise");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pGTAODenoisePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Input", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Direct Lighting Data //////////
//
// Collects all GPU resources related to deferred direct lighting.

struct xiiDeferredDirectLightingData
{
  xiiRGTextureHandle m_hGBufferAlbedo;            ///< ShaderResource in (G-Buffer albedo).
  xiiRGTextureHandle m_hGBufferNormal;            ///< ShaderResource in (G-Buffer normal).
  xiiRGTextureHandle m_hGBufferMaterial;          ///< ShaderResource in (G-Buffer material).
  xiiRGTextureHandle m_hSceneDepth;               ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hStableAmbientOcclusion;   ///< ShaderResource in (stable ambient occlusion).
  xiiRGTextureHandle m_hRayTracedFinalShadowMask; ///< ShaderResource in (denoised ray traced shadows).
  xiiRGTextureHandle m_hContactShadowTerm;        ///< ShaderResource in (contact shadow mask).
  xiiRGTextureHandle m_hDirectionalShadowAtlas;   ///< ShaderResource in (directional shadow atlas).
  xiiRGBufferHandle  m_hLightGridBuffer;          ///< ShaderResource in (cluster light grid).
  xiiRGBufferHandle  m_hLightIndexBuffer;         ///< ShaderResource in (cluster light indices).
  xiiRGTextureHandle m_hDirectLightingBuffer;     ///< UnorderedAccess out (direct lighting HDR buffer).
};

void xiiView::SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRGBuilder& builder)
{
  data.m_hGBufferAlbedo            = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal            = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial          = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth               = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion   = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalShadowMask = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hContactShadowTerm        = builder.ReadTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDirectionalShadowAtlas   = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer          = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer         = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = m_Data.m_ViewPortRect.width;
  description.m_Size.height    = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hDirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pDirectLightingPipeline, "Shaders/Pipeline/DirectLighting.xiiShader");
}

void xiiView::ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredDirectLighting");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pDirectLightingPipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RTShadow", context.GetTexture(data.m_hRayTracedFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ContactShadow", context.GetTexture(data.m_hContactShadowTerm)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DirectOut", context.GetTexture(data.m_hDirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Indirect Lighting Data //////////
//
// Collects all GPU resources related to deferred indirect lighting.

struct xiiDeferredIndirectLightingData
{
  xiiRGTextureHandle m_hGBufferAlbedo;          ///< ShaderResource in (G-Buffer albedo).
  xiiRGTextureHandle m_hGBufferNormal;          ///< ShaderResource in (G-Buffer normal).
  xiiRGTextureHandle m_hGBufferMaterial;        ///< ShaderResource in (G-Buffer material).
  xiiRGTextureHandle m_hSceneDepth;             ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hStableAmbientOcclusion; ///< ShaderResource in (stable ambient occlusion).
  xiiRGTextureHandle m_hBRDFLut;                ///< ShaderResource in (BRDF lookup texture).
  xiiRGTextureHandle m_hDDGIIrradiance;         ///< ShaderResource in (DDGI irradiance texture).
  xiiRGTextureHandle m_hSkyRadiance;            ///< ShaderResource in (sky radiance texture).
  xiiRGTextureHandle m_hIndirectLightingBuffer; ///< UnorderedAccess out (indirect lighting HDR buffer).
};

void xiiView::SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRGBuilder& builder)
{
  data.m_hGBufferAlbedo          = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal          = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial        = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth             = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut                = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDDGIIrradiance         = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSkyRadiance            = builder.ReadTexture(xiiRGBlackboardKeys::k_SkyRadiance, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = m_Data.m_ViewPortRect.width;
  description.m_Size.height      = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hIndirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pIndirectLightingPipeline, "Shaders/Pipeline/IndirectLighting.xiiShader");
}

void xiiView::ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredIndirectLighting");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pIndirectLightingPipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIIr", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SkyRadiance", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_IndirectOut", context.GetTexture(data.m_hIndirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Global Illumination Data //////////
//
// Collects all GPU resources related to ray traced global illumination final gather.

struct xiiRayTracedGlobalIlluminationData
{
  xiiRGTextureHandle m_hSceneDepth;                       ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hGBufferNormal;                    ///< ShaderResource in (G-Buffer normal).
  xiiRGTextureHandle m_hIndirectLightingInput;            ///< ShaderResource in (deferred indirect lighting input).
  xiiRGTextureHandle m_hRayTracedRawGlobalIllumination;   ///< UnorderedAccess out (raw RT GI texture).
  xiiRGTextureHandle m_hRayTracedFinalGlobalIllumination; ///< UnorderedAccess out (final RT GI texture).
};

void xiiView::SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth            = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLightingInput = builder.ReadTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                       = xiiGALResourceDimension::Texture2D;
  description.m_Format                     = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width                 = m_Data.m_ViewPortRect.width;
  description.m_Size.height                = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels                = 1U;
  description.m_BindFlags                  = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage                      = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawGlobalIllumination   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawGI, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hRayTracedFinalGlobalIllumination = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pRTGIPipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

void xiiView::ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTGIFinalGather");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pRTGIPipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_IndirectIn", context.GetTexture(data.m_hIndirectLightingInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIRaw", context.GetTexture(data.m_hRayTracedRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIFinal", context.GetTexture(data.m_hRayTracedFinalGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Data //////////
//
// Collects all GPU resources related to ray traced reflections.

struct xiiRayTracedReflectionsData
{
  xiiRGTextureHandle m_hSceneDepth;                ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hGBufferNormal;             ///< ShaderResource in (G-Buffer normal).
  xiiRGTextureHandle m_hGBufferMaterial;           ///< ShaderResource in (G-Buffer material).
  xiiRGTextureHandle m_hBRDFLut;                   ///< ShaderResource in (BRDF lookup texture).
  xiiRGTextureHandle m_hRayTracedRawReflections;   ///< UnorderedAccess out (raw RT reflections texture).
  xiiRGTextureHandle m_hRayTracedFinalReflections; ///< UnorderedAccess out (final RT reflections texture).
};

void xiiView::SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut         = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                = xiiGALResourceDimension::Texture2D;
  description.m_Format              = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width          = m_Data.m_ViewPortRect.width;
  description.m_Size.height         = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels         = 1U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawReflections   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawReflections, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hRayTracedFinalReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalReflections, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pRTReflectionPipeline, "Shaders/Pipeline/RTReflection.xiiShader");
}

void xiiView::ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTReflections");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pRTReflectionPipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflRaw", context.GetTexture(data.m_hRayTracedRawReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflFinal", context.GetTexture(data.m_hRayTracedFinalReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen Space Reflections Data //////////
//
// Collects all GPU resources related to screen-space reflections.

struct xiiScreenSpaceReflectionsData
{
  xiiRGTextureHandle m_hSceneDepth;             ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hGBufferNormal;          ///< ShaderResource in (G-Buffer normal).
  xiiRGTextureHandle m_hGBufferMaterial;        ///< ShaderResource in (G-Buffer material).
  xiiRGTextureHandle m_hHDRSceneColor;          ///< ShaderResource in (current HDR scene color).
  xiiRGTextureHandle m_hScreenSpaceReflections; ///< UnorderedAccess out (screen-space reflections texture).
};

void xiiView::SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor   = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = m_Data.m_ViewPortRect.width;
  description.m_Size.height      = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hScreenSpaceReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_SSRTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pSSRPipeline, "Shaders/Pipeline/SSR.xiiShader");
}

void xiiView::ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSR");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pSSRPipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSROut", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Integration Data //////////
//
// Collects all GPU resources related to volumetric fog integration.

struct xiiVolumetricFogIntegrationData
{
  xiiRGTextureHandle m_hFroxelScatteringBuffer; ///< ShaderResource in (froxel scattering buffer).
  xiiRGBufferHandle  m_hLightGridBuffer;        ///< ShaderResource in (cluster light grid).
  xiiRGTextureHandle m_hVolumetricScattering;   ///< UnorderedAccess out (integrated volumetric scattering).
};

void xiiView::SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRGBuilder& builder)
{
  data.m_hFroxelScatteringBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer        = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = m_Data.m_ViewPortRect.width;
  description.m_Size.height    = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hVolumetricScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_VolumetricScattering, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pVolumetricIntegratePipeline, "Shaders/Pipeline/VolumetricLightIntegration.xiiShader");
}

void xiiView::ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogIntegrate");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pVolumetricIntegratePipeline);

    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScatteringBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_VolumetricOut", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Temporal Reprojection Data //////////
//
// Collects all GPU resources related to volumetric fog temporal reprojection.

struct xiiVolumetricFogTemporalReprojectionData
{
  xiiRGTextureHandle m_hFroxelHistory;        ///< ShaderResource in (history froxel volume from previous frame).
  xiiRGTextureHandle m_hVolumetricScattering; ///< UnorderedAccess in/out (current volumetric scattering buffer).
};

void xiiView::SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRGBuilder& builder)
{
  if (m_ViewPassResources.m_LightingPasses.m_pFroxelHistoryBuffer == nullptr)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = xiiGALResourceDimension::Texture3D;
    description.m_Format             = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width         = 128U;
    description.m_Size.height        = 72U;
    description.m_uiArraySizeOrDepth = 64U;
    description.m_uiMipLevels        = 1U;
    description.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage              = xiiGALResourceUsage::Default;

    m_ViewPassResources.m_LightingPasses.m_pFroxelHistoryBuffer = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hFroxelHistory        = builder.ImportTexture("FroxelHistory", m_ViewPassResources.m_LightingPasses.m_pFroxelHistoryBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hFroxelHistory        = builder.ReadTexture(data.m_hFroxelHistory, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pVolumetricTemporalPipeline, "Shaders/Pipeline/VolumetricFogTemporalRep.xiiShader");
}

void xiiView::ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogTemporalRep");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pVolumetricTemporalPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelHistory", context.GetTexture(data.m_hFroxelHistory)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelBlended", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 8U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Composite Data //////////
//
// Collects all GPU resources related to atmosphere compositing.

struct xiiAtmosphereCompositeData
{
  xiiRGTextureHandle m_hAtmosphereTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRGTextureHandle m_hAtmosphereMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRGTextureHandle m_hVolumetricScattering;       ///< ShaderResource in (volumetric scattering buffer).
  xiiRGTextureHandle m_hSkyRadiance;                ///< ShaderResource in (sky radiance texture).
};

void xiiView::SetupAtmosphereComposite(xiiAtmosphereCompositeData& data, xiiRGBuilder& builder)
{
  data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering       = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSkyRadiance                = builder.ReadTexture(xiiRGBlackboardKeys::k_SkyRadiance, xiiGALResourceStateFlags::ShaderResource);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pAtmosphereCompositePipeline, "Shaders/Pipeline/AtmosphereComposite.xiiShader");
}

void xiiView::ExecuteAtmosphereComposite(const xiiAtmosphereCompositeData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereComposite");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pAtmosphereCompositePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hAtmosphereTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hAtmosphereMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VolumetricFog", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PrevSkyRadiance", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_Data.m_ViewPortRect.width + 7U) / 8U, (m_Data.m_ViewPortRect.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Forward Opaque Data //////////
//
// Collects all GPU resources related to the forward opaque pass.

struct xiiForwardOpaqueData
{
  xiiRGTextureHandle m_hSceneDepth;                ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hDirectLighting;            ///< ShaderResource in (direct lighting texture).
  xiiRGTextureHandle m_hIndirectLighting;          ///< ShaderResource in (indirect lighting texture).
  xiiRGTextureHandle m_hRayTracedFinalGI;          ///< ShaderResource in (final ray-traced global illumination texture).
  xiiRGTextureHandle m_hRayTracedFinalReflections; ///< ShaderResource in (final ray-traced reflections texture).
  xiiRGTextureHandle m_hScreenSpaceReflections;    ///< ShaderResource in (screen-space reflections texture).
  xiiRGTextureHandle m_hVolumetricScattering;      ///< ShaderResource in (volumetric scattering texture).
  xiiRGTextureHandle m_hStableAmbientOcclusion;    ///< ShaderResource in (stable ambient occlusion texture).
  xiiRGBufferHandle  m_hLightGridBuffer;           ///< ShaderResource in (cluster light grid buffer).
  xiiRGBufferHandle  m_hLightIndexBuffer;          ///< ShaderResource in (cluster light index buffer).
  xiiRGBufferHandle  m_hDrawIndirectCommands;      ///< IndirectArgument in (draw indirect commands).
  xiiRGTextureHandle m_hHDRSceneColor;             ///< RenderTarget out (composited HDR scene color).
};

void xiiView::SetupForwardOpaque(xiiForwardOpaqueData& data, xiiRGBuilder& builder)
{
  data.m_hSceneDepth                = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDirectLighting            = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLighting          = builder.ReadTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalGI          = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalReflections = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hScreenSpaceReflections    = builder.ReadTexture(xiiRGBlackboardKeys::k_SSRTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering      = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion    = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer           = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer          = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDrawIndirectCommands      = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_Data.m_ViewPortRect.width;
  description.m_Size.height = m_Data.m_ViewPortRect.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor     = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteForwardOpaque(const xiiForwardOpaqueData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ForwardOpaque");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_ForwardPasses.m_pForwardOpaquePipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pForwardOpaquePipeline);

      cmd.ResolveAndSetShaderResourceTextureView("g_DirectLight", context.GetTexture(data.m_hDirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_IndirectLight", context.GetTexture(data.m_hIndirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_RTRefl", context.GetTexture(data.m_hRayTracedFinalReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_RTGI", context.GetTexture(data.m_hRayTracedFinalGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_SSR", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_Volumetric", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_AO", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Forward Masked Data //////////
//
// Collects all GPU resources related to the forward masked pass.

struct xiiForwardMaskedData
{
  xiiRGTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupForwardMasked(xiiForwardMaskedData& data, xiiRGBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteForwardMasked(const xiiForwardMaskedData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ForwardMasked");
  {
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_ForwardPasses.m_pForwardMaskedPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pForwardMaskedPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hair Rendering Data //////////
//
// Collects all GPU resources related to the hair rendering pass.

struct xiiHairRenderingData
{
  xiiRGTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupHairRendering(xiiHairRenderingData& data, xiiRGBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteHairRendering(const xiiHairRenderingData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HairRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_ForwardPasses.m_pHairPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pHairPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Water Rendering Data //////////
//
// Collects all GPU resources related to the water rendering pass.

struct xiiWaterRenderingData
{
  xiiRGTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRGTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRGTextureHandle m_hPlanarReflectionMap;  ///< ShaderResource in (planar reflection map).
  xiiRGBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupWaterRendering(xiiWaterRenderingData& data, xiiRGBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hPlanarReflectionMap  = builder.ReadTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteWaterRendering(const xiiWaterRenderingData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("WaterRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});

    if (m_ViewPassResources.m_ForwardPasses.m_pWaterPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pWaterPipeline);
      if (data.m_hPlanarReflectionMap.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_PlanarRefl", context.GetTexture(data.m_hPlanarReflectionMap)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Subsurface Scattering Data //////////
//
// Collects all GPU resources related to the screen-space subsurface scattering pass.

struct xiiSubsurfaceScatteringData
{
  xiiRGTextureHandle m_hHDRSceneColor;   ///< UnorderedAccess in/out (HDR scene color).
  xiiRGTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRGTextureHandle m_hGBufferMaterial; ///< ShaderResource in (material G-Buffer).
};

void xiiView::SetupSubsurfaceScattering(xiiSubsurfaceScatteringData& data, xiiRGBuilder& builder)
{
  data.m_hHDRSceneColor   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
}

void xiiView::ExecuteSubsurfaceScattering(const xiiSubsurfaceScatteringData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SubsurfaceScattering");
  {
    if (m_ViewPassResources.m_ForwardPasses.m_pSSSComputePipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pSSSComputePipeline);
      if (data.m_hSceneDepth.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      }
      if (data.m_hGBufferMaterial.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      }
      if (data.m_hHDRSceneColor.IsValid())
      {
        cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRInOut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt32 uiRenderWidth  = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.width));
      const xiiUInt32 uiRenderHeight = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.height));
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}
void xiiView::BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // CPU dynamic resolution PID (pre-graph, writes to blackboard). Must happen before BeginSetup so passes see the correct render dimensions.
  RunDynamicResolutionPID(blackboard);

  XII_ASSERT_DEV(blackboard.Contains(xiiRGBlackboardKeys::k_RenderWidth) && blackboard.Contains(xiiRGBlackboardKeys::k_RenderHeight), "Dynamic resolution PID did not write render dimensions to the blackboard.");

  // Each stage adds its passes to the graph. Dependency ordering is handled by the render graph compiler (topological sort + culling).

  // Visibility preparation passes, which produce data consumed by the main render passes in later stages.
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

  // Shadow preparation passes, which produce data consumed by the main shadow pass in later stages.
  graph.AddPass<xiiShadowCascadeSetupData>("ShadowCascadeSetup", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowCascadeSetup, this), xiiMakeDelegate(&xiiView::ExecuteShadowCascadeSetup, this));
  graph.AddPass<xiiDirectionalShadowData>("DirectionalShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDirectionalShadowData, this), xiiMakeDelegate(&xiiView::ExecuteDirectionalShadowData, this));
  graph.AddPass<xiiSpotShadowData>("SpotShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupSpotShadowData, this), xiiMakeDelegate(&xiiView::ExecuteSpotShadowData, this));
  graph.AddPass<xiiPointShadowData>("PointShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupPointShadowData, this), xiiMakeDelegate(&xiiView::ExecutePointShadowData, this));
  graph.AddPass<xiiRayTracedShadowData>("RayTracedShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupRayTracedShadowData, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedShadowData, this));
  graph.AddPass<xiiShadowDenoiseData>("ShadowDenoise", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupShadowDenoiseData, this), xiiMakeDelegate(&xiiView::ExecuteShadowDenoiseData, this));
  graph.AddPass<xiiContactShadowData>("ContactShadow", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupContactShadowData, this), xiiMakeDelegate(&xiiView::ExecuteContactShadowData, this));

  // Depth and motion prepasses, which produce depth and motion data consumed by later passes.
  graph.AddPass<xiiDepthPrepassData>("DepthPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDepthPrepass, this), xiiMakeDelegate(&xiiView::ExecuteDepthPrepass, this));
  graph.AddPass<xiiHiZPyramidData>("HiZPyramid", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZPyramid, this), xiiMakeDelegate(&xiiView::ExecuteHiZPyramid, this));
  graph.AddPass<xiiHiZOcclusionCullData>("HiZOcclusionCull", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZOcclusionCull, this), xiiMakeDelegate(&xiiView::ExecuteHiZOcclusionCull, this));
  graph.AddPass<xiiMotionVectorsData>("MotionVectors", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupMotionVectors, this), xiiMakeDelegate(&xiiView::ExecuteMotionVectors, this));
  graph.AddPass<xiiVelocityDilationData>("VelocityDilation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVelocityDilation, this), xiiMakeDelegate(&xiiView::ExecuteVelocityDilation, this));

  // G-Buffer generation passes, which produce material surfaces consumed by lighting stages.
  graph.AddPass<xiiGBufferBaseData>("GBufferBase", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupGBufferBase, this), xiiMakeDelegate(&xiiView::ExecuteGBufferBase, this));
  graph.AddPass<xiiNormalRoughnessPrepassData>("NormalRoughnessPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupNormalRoughnessPrepass, this), xiiMakeDelegate(&xiiView::ExecuteNormalRoughnessPrepass, this));

  // Lighting preparation passes, which generate lookup textures and lighting auxiliaries.
  graph.AddPass<xiiBRDFLutGenerationData>("BRDFLUTGenerate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupBRDFLutGeneration, this), xiiMakeDelegate(&xiiView::ExecuteBRDFLutGeneration, this));
  graph.AddPass<xiiAtmosphereTransmittanceData>("AtmosphereTransmittanceLUT", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAtmosphereTransmittance, this), xiiMakeDelegate(&xiiView::ExecuteAtmosphereTransmittance, this));
  graph.AddPass<xiiAtmosphereMultiScatterData>("AtmosphereMultiScatterLUT", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAtmosphereMultiScatter, this), xiiMakeDelegate(&xiiView::ExecuteAtmosphereMultiScatter, this));
  graph.AddPass<xiiSkyIrradianceConvolutionData>("SkyIrradianceConvolution", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupSkyIrradianceConvolution, this), xiiMakeDelegate(&xiiView::ExecuteSkyIrradianceConvolution, this));
  graph.AddPass<xiiReflectionProbeConvolutionData>("ReflectionProbeConvolution", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReflectionProbeConvolution, this), xiiMakeDelegate(&xiiView::ExecuteReflectionProbeConvolution, this));
  graph.AddPass<xiiVolumetricFogInitializationData>("VolumetricFogInitialization", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogInitialization, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogInitialization, this));
  graph.AddPass<xiiDDGIProbeSamplingData>("DDGIProbeSampling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDDGIProbeSampling, this), xiiMakeDelegate(&xiiView::ExecuteDDGIProbeSampling, this));
  graph.AddPass<xiiGroundTruthAmbientOcclusionData>("GroundTruthAmbientOcclusion", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGroundTruthAmbientOcclusion, this), xiiMakeDelegate(&xiiView::ExecuteGroundTruthAmbientOcclusion, this));
  graph.AddPass<xiiGroundTruthAmbientOcclusionDenoiseData>("GroundTruthAmbientOcclusionDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGroundTruthAmbientOcclusionDenoise, this), xiiMakeDelegate(&xiiView::ExecuteGroundTruthAmbientOcclusionDenoise, this));

  // Main lighting passes, which produce direct and indirect lighting results.
  graph.AddPass<xiiDeferredDirectLightingData>("DeferredDirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDirectLighting, this), xiiMakeDelegate(&xiiView::ExecuteDirectLighting, this));
  graph.AddPass<xiiDeferredIndirectLightingData>("DeferredIndirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupIndirectLighting, this), xiiMakeDelegate(&xiiView::ExecuteIndirectLighting, this));
  graph.AddPass<xiiRayTracedGlobalIlluminationData>("RTGIFinalGather", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedGlobalIllumination, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedGlobalIllumination, this));
  graph.AddPass<xiiRayTracedReflectionsData>("RTReflections", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedReflections, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedReflections, this));
  graph.AddPass<xiiScreenSpaceReflectionsData>("SSR", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceReflections, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceReflections, this));
  graph.AddPass<xiiVolumetricFogIntegrationData>("VolumetricFogIntegrate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogIntegration, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogIntegration, this));
  graph.AddPass<xiiVolumetricFogTemporalReprojectionData>("VolumetricFogTemporalRep", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogTemporalReprojection, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogTemporalReprojection, this));
  graph.AddPass<xiiAtmosphereCompositeData>("VolumetricLightAccumulate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAtmosphereComposite, this), xiiMakeDelegate(&xiiView::ExecuteAtmosphereComposite, this));

  // Forward rendering passes, which composite main scene color from lighting buffers and forward geometry.
  graph.AddPass<xiiForwardOpaqueData>("ForwardOpaque", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupForwardOpaque, this), xiiMakeDelegate(&xiiView::ExecuteForwardOpaque, this));
  graph.AddPass<xiiForwardMaskedData>("ForwardMasked", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupForwardMasked, this), xiiMakeDelegate(&xiiView::ExecuteForwardMasked, this));
  graph.AddPass<xiiHairRenderingData>("HairRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupHairRendering, this), xiiMakeDelegate(&xiiView::ExecuteHairRendering, this));
  graph.AddPass<xiiWaterRenderingData>("WaterRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupWaterRendering, this), xiiMakeDelegate(&xiiView::ExecuteWaterRendering, this));
  graph.AddPass<xiiSubsurfaceScatteringData>("SubsurfaceScattering", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupSubsurfaceScattering, this), xiiMakeDelegate(&xiiView::ExecuteSubsurfaceScattering, this));
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
