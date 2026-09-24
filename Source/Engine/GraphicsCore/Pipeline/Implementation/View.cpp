/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Components/Render/DecalComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

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

  static xiiUInt32 ComputeDynamicResolutionDimension(xiiUInt32 uiNativeDimension, float fScale)
  {
    const xiiUInt32 uiScaled = static_cast<xiiUInt32>(static_cast<float>(uiNativeDimension) * fScale) & ~1U;
    return xiiMath::Max(uiScaled, 2U);
  }

  static void ComputeDynamicScaleBounds(float fRenderScale, float& out_fDynamicMin, float& out_fDynamicMax)
  {
    const float fFinalMinScale = xiiMath::Max(cvar_DynamicRenderingMinScale.GetValue(), 0.25f);
    const float fFinalMaxScale = xiiMath::Min(cvar_DynamicRenderingMaxScale.GetValue(), 1.0f);
    const float fBaseScale     = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);

    out_fDynamicMin = xiiMath::Clamp(fFinalMinScale / fBaseScale, 0.1f, 1.0f);
    out_fDynamicMax = xiiMath::Clamp(fFinalMaxScale / fBaseScale, 0.1f, 1.0f);

    if (out_fDynamicMin > out_fDynamicMax)
    {
      out_fDynamicMin = out_fDynamicMax;
    }
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiView::xiiView(xiiWorld* pWorld) :
  m_pWorld(pWorld)
{
  m_pRenderGraph = XII_DEFAULT_NEW(xiiRenderGraph);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No default device available. A view requires a device to initialize its resources.");

  m_ViewPassResources.m_Profiler.Initialize(pDevice);
  m_ViewPassResources.m_LightingSystem.Initialize(pDevice);
  m_ResourceCache.Initialize(pDevice);

  UpdateRenderResolutionState();
}

xiiView::~xiiView()
{
  m_InternalId.Invalidate();

  m_ViewPassResources.m_LightingSystem.Shutdown();
  m_ViewPassResources.m_Profiler.Shutdown();

  m_ResourceCache.Shutdown();
}

void xiiView::SetRenderScale(float fRenderScale)
{
  m_ViewPassResources.m_DynamicResolution.m_fRenderScale = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(m_ViewPassResources.m_DynamicResolution.m_fRenderScale, fDynamicMin, fDynamicMax);

  m_ViewPassResources.m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fCurrentScale, fDynamicMin, fDynamicMax);
  m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  UpdateRenderResolutionState();
}

float xiiView::GetRenderScale() const
{
  return m_ViewPassResources.m_DynamicResolution.m_fRenderScale;
}

void xiiView::UpdateCachedMatrices() const
{
  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != m_pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP                             = true;
    m_uiLastCameraOrientationModification = m_pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = m_pCamera->GetViewMatrix(xiiCameraEye::Left);
    m_Data.m_ViewMatrix[1] = m_pCamera->GetViewMatrix(xiiCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != m_pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP                          = true;
    m_uiLastCameraSettingsModification = m_pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio         = fViewportAspectRatio;


    m_pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], xiiCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    m_pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], xiiCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i]        = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void xiiView::UpdateRenderResolutionState() const
{
  const float fRenderScale = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fDynamicScale = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  m_Data.m_fRenderResolutionScale = xiiMath::Clamp(fDynamicScale * fRenderScale, 0.1f, 1.0f);

  const xiiUInt32 uiNativeWidth  = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.width));
  const xiiUInt32 uiNativeHeight = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.height));

  m_Data.m_uiRenderResolutionWidth  = ComputeDynamicResolutionDimension(uiNativeWidth, m_Data.m_fRenderResolutionScale);
  m_Data.m_uiRenderResolutionHeight = ComputeDynamicResolutionDimension(uiNativeHeight, m_Data.m_fRenderResolutionScale);
}

void xiiView::RunDynamicResolutionPID()
{
  // Try the GPU profiler's resolved duration from 2 frames ago.
  // Falls back to CPU wall-clock when the profiler ring hasn't warmed up yet.
  float fGpuTimeMs = m_ViewPassResources.m_Profiler.GetFrameDurationMs();
  if (fGpuTimeMs <= 0.0f)
  {
    fGpuTimeMs = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;
  }

  m_ViewPassResources.m_DynamicResolution.m_fLastGpuFrameTimeMs = fGpuTimeMs;

  // PID controller.
  const float fRenderScale = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);
  float       fDynamicMin  = 0.1f;
  float       fDynamicMax  = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fTarget    = xiiMath::Max(cvar_DynamicRenderingTargetMs.GetValue(), 0.1f);
  const float fDeltaTime = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral + fError * fDeltaTime, -1.0f, 1.0f);

  const float fDerivative = (fError - m_ViewPassResources.m_DynamicResolution.m_fPreviousError) / xiiMath::Max(fDeltaTime, 0.001f);
  const float fPID        = 0.35f * fError + 0.05f * m_ViewPassResources.m_DynamicResolution.m_fErrorIntegral + 0.15f * fDerivative;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fCurrentScale + fPID, fDynamicMin, fDynamicMax);
  const float fDelta   = xiiMath::Clamp(fDesired - m_ViewPassResources.m_DynamicResolution.m_fCurrentScale, -0.10f, 0.10f);

  m_ViewPassResources.m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_ViewPassResources.m_DynamicResolution.m_fCurrentScale + fDelta, fDynamicMin, fDynamicMax);
  m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(xiiMath::Lerp(m_ViewPassResources.m_DynamicResolution.m_fSmoothedScale, m_ViewPassResources.m_DynamicResolution.m_fCurrentScale, 0.20f), fDynamicMin, fDynamicMax);
  m_ViewPassResources.m_DynamicResolution.m_fPreviousError = fError;

  UpdateRenderResolutionState();
}

float xiiView::GetRenderResolutionScale() const
{
  UpdateRenderResolutionState();
  return m_Data.m_fRenderResolutionScale;
}

xiiUInt32 xiiView::GetRenderResolutionWidth() const
{
  UpdateRenderResolutionState();
  return m_Data.m_uiRenderResolutionWidth;
}

xiiUInt32 xiiView::GetRenderResolutionHeight() const
{
  UpdateRenderResolutionState();
  return m_Data.m_uiRenderResolutionHeight;
}

////////// Lighting Data Upload //////////
//
// Uploads per-view camera, frame, and light data into persistent GAL buffers once per frame.
// A tiny graph token makes the dependency explicit for clustering and all downstream lighting passes.

struct xiiLightingDataUploadData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hLightingDataReady; ///< UAV out token written after persistent lighting buffers have been uploaded.
};

void xiiView::SetupLightingDataUpload(xiiLightingDataUploadData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = 4U;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hLightingDataReady = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightingDataReady, description, xiiGALResourceStateFlags::UnorderedAccess);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteLightingDataUpload(const xiiLightingDataUploadData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightingDataUpload");
  {
    m_ViewPassResources.m_LightingSystem.UploadFrameData(cmd);

    const xiiUInt32 uiReadyToken = 1U;
    cmd.UpdateBuffer(context.GetBuffer(data.m_hLightingDataReady), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&uiReadyToken), sizeof(uiReadyToken)));
  }
  cmd.EndDebugGroup();
}

////////// GPU occlusion readback //////////
//
// Reads the oldest staging buffer in the 3-frame ring (from 2 frames ago).
// This provides the GPU frame time used by the PID (the result has already been applied by RunDynamicResolutionPID before BeginSetup, so this pass simply keeps the readback ring rotating).

struct xiiOcclusionReadbackData
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiReadSlot = 0; ///< Index into the 3-frame ring of staging buffers to read from this frame (the one written by the GPU 2 frames ago).
};

void xiiView::SetupOcclusionReadback(xiiOcclusionReadbackData& data, xiiRenderGraphBuilder& builder)
{
  data.m_uiReadSlot = (m_ViewPassResources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRenderGraphPassContext& context)
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
    cmd.UnmapBuffer(pStaging, xiiGALMapType::Read).AssertSuccess("Failed to unmap occlusion readback buffer.");
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hInstanceBounds;     ///< SRV in (structured buffer of xiiBoundingSphere, one per instance, from previous frame's Instance Update).
  xiiRenderGraphBufferHandle m_hLODMetadata;        ///< SRV in (structured buffer of LOD metadata, one per instance, from previous frame's LOD Selection).
  xiiRenderGraphBufferHandle m_hVisibleCandidates;  ///< UAV out (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, consumed by Instance Update and Draw Build).
  xiiUInt32                  m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupFrustumCull(xiiFrustumCullData& data, xiiRenderGraphBuilder& builder)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Ensure persistent instance bounds buffer exists.
  if (!m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                              = 32U; // float3 center + float radius + float3 extents + float pad.
    description.m_uiSize                                           = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                        = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                             = xiiGALBufferMode::Structured;
    description.m_Usage                                            = xiiGALResourceUsage::Default;
    m_ViewPassResources.m_VisibilityPasses.m_pInstanceBoundsBuffer = pDevice->CreateBuffer(description);
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
    m_ViewPassResources.m_VisibilityPasses.m_pInstanceMatrixBuffer = pDevice->CreateBuffer(description);
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

void xiiView::ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hVisibleCandidates;  ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRenderGraphBufferHandle m_hInstanceBounds;     ///< SRV in (structured buffer of xiiBoundingSphere, one per instance, from previous frame's Instance Update).
  xiiRenderGraphBufferHandle m_hInstanceLOD;        ///< UAV out (structured buffer of uint, one per instance, packed LOD level + meshlet offset, consumed by Draw Build).
  xiiUInt32                  m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupLODSelect(xiiLODSelectData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteLODSelect(const xiiLODSelectData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hVisibleCandidates;  ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRenderGraphBufferHandle m_hInstanceMatrices;   ///< UAV out (structured buffer of instance world matrices, one per instance, consumed by next frame's LOD Selection and Frustum Culling).
  xiiRenderGraphBufferHandle m_hInstanceBoundsOut;  ///< UAV out (structured buffer of xiiBoundingSphere, one per instance, consumed by next frame's Frustum Culling).
  xiiUInt32                  m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hSurvivors;          ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRenderGraphBufferHandle m_hInstanceLOD;        ///< SRV in (structured buffer of uint, one per instance, packed LOD level + meshlet offset, from this frame's LOD Selection).
  xiiRenderGraphBufferHandle m_hDrawCommands;       ///< UAV out (structured buffer of DrawIndexedIndirectArguments, one per draw bin, consumed by GBuffer and Shadow Passes).
  xiiRenderGraphBufferHandle m_hDrawCounts;         ///< UAV out (structured buffer of uint, one per draw bin, used for indirect count in multi-draw scenarios).
  xiiUInt32                  m_uiInstanceCount = 0; ///< Number of instances to process (from previous frame's Instance Update). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hVisibleCandidates;    ///< SRV in (structured buffer of uint, [0]=count, [1..]=indices of visible instances for current frame, from this frame's Frustum Culling).
  xiiRenderGraphBufferHandle m_hShadowCasterCommands; ///< UAV out (structured buffer of uint, [0]=count, [1..]=indices of shadow-casting instances for current frame, consumed by Shadow Passes).
};

void xiiView::SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hLightingDataReady;  ///< SRV in dependency token that ensures persistent lighting buffers are uploaded.
  xiiRenderGraphBufferHandle m_hClusterConstants;   ///< SRV in (structured buffer of cluster build constants, including cluster counts and depth range, consumed by the Cluster Build pass).
  xiiRenderGraphBufferHandle m_hClusterDescriptors; ///< UAV out (structured buffer of cluster descriptors, one per cluster, consumed by main lighting pass).
};

void xiiView::SetupClusterBuild(xiiClusterBuildData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiTotalClusters = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 32U; // float4 min + float4 max per cluster AABB
  description.m_uiSize              = description.m_uiElementByteStride * uiTotalClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hClusterDescriptors        = builder.WriteBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pClusterBuildPipeline, "Shaders/Pipeline/ClusterGridBuild.xiiShader");

  description.m_uiElementByteStride = 0;
  description.m_uiSize              = sizeof(xiiLightClusteringConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;

  data.m_hClusterConstants = builder.WriteBuffer("xiiLightClusteringConstants", description, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiView::ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ClusterGridBuild");
  {
    xiiUInt32 uiClusterCountX = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetClusterCountX(), 1U);
    xiiUInt32 uiClusterCountY = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetClusterCountY(), 1U);
    xiiUInt32 uiClusterCountZ = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetClusterCountZ(), 1U);
    xiiUInt32 uiTotalClusters = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetTotalClusterCount(), 1U);

    {
      xiiGALMapHelper<xiiLightClusteringConstants> pClusteringConstants(cmd, context.GetBuffer(data.m_hClusterConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pClusteringConstants->ClusterCountX       = uiClusterCountX;
      pClusteringConstants->ClusterCountY       = uiClusterCountY;
      pClusteringConstants->ClusterCountZ       = uiClusterCountZ;
      pClusteringConstants->TotalClusters       = uiTotalClusters;
      pClusteringConstants->NearPlane           = m_pCamera->GetNearPlane();
      pClusteringConstants->FarPlane            = m_pCamera->GetFarPlane();
      pClusteringConstants->LogFarOverNear      = xiiMath::Log2(pClusteringConstants->FarPlane / pClusteringConstants->NearPlane);
      pClusteringConstants->TilePixelsX         = m_ViewPassResources.m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->TilePixelsY         = m_ViewPassResources.m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->MaxLightsPerCluster = m_ViewPassResources.m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster;
      pClusteringConstants->ActiveLightCount    = m_ViewPassResources.m_LightingSystem.GetActiveLightCount();
    }

    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pClusterBuildPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterDescriptors;    ///< SRV in (structured buffer of cluster descriptors, one per cluster, from this frame's Cluster Build).
  xiiRenderGraphBufferHandle m_hClusterConstants;      ///< Constant buffer in (cluster dimensions and active light count).
  xiiRenderGraphBufferHandle m_hLightIndexBuffer;      ///< SRV in (structured buffer of uint, one per light, containing light type and other metadata, from extraction).
  xiiRenderGraphBufferHandle m_hLightGridBuffer;       ///< UAV out (structured buffer of uint, containing compact light lists per cluster, consumed by main lighting pass).
  xiiUInt32                  m_uiActiveLightCount = 0; ///< Number of active lights to process (from extraction). This is used to avoid processing the entire buffer when only a subset is populated.
  xiiUInt32                  m_uiTotalClusters    = 0; ///< Number of clusters that need a compact light list.
};

void xiiView::SetupLightListBuild(xiiLightListData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);
  data.m_hClusterConstants   = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);

  const xiiUInt32 uiMaxClusters    = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetTotalClusterCount(), 1U);
  const xiiUInt32 uiMaxLightsPerCl = xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster, 1U);

  xiiGALBufferCreationDescription indexBufferDescription;
  indexBufferDescription.m_uiElementByteStride = 4U;
  indexBufferDescription.m_uiSize              = 4U * uiMaxClusters * uiMaxLightsPerCl;
  indexBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  indexBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightIndexBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, indexBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription gridBufferDescription;
  gridBufferDescription.m_uiElementByteStride = 8U; // uint2 (offset, count) per cluster
  gridBufferDescription.m_uiSize              = gridBufferDescription.m_uiElementByteStride * uiMaxClusters;
  gridBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  gridBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightGridBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, gridBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiActiveLightCount = m_ViewPassResources.m_LightingSystem.GetActiveLightCount();
  data.m_uiTotalClusters    = uiMaxClusters;

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_VisibilityPasses.m_pLightListPipeline, "Shaders/Pipeline/LightListBuild.xiiShader");
}

void xiiView::ExecuteLightListBuild(const xiiLightListData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListBuild");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pLightListPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources.m_LightingSystem.BindLightData(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusters + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Proble Select Data //////////
//
// Selects relevant reflection probes for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of reflection probe indices and a bitmask of which probes affect which instances, which are then consumed by the main lighting pass for reflection probe sampling.

struct xiiReflectionProbeSelectData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterDescriptors; ///< SRV in (structured buffer of cluster descriptors, one per cluster, from this frame's Cluster Build).
  xiiRenderGraphBufferHandle m_hProbeMask;          ///< UAV out (structured buffer of uint, one per instance, bitmask of which reflection probes affect each instance, consumed by main lighting pass).
};

void xiiView::SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hFroxelMetadata;
  xiiRenderGraphTextureHandle m_hFroxelScattering;
  xiiUInt32                   m_uiRenderWidth  = 1920U;
  xiiUInt32                   m_uiRenderHeight = 1080U;
};

void xiiView::SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription froxelMetadataBufferDescription;
  froxelMetadataBufferDescription.m_uiElementByteStride = 16U;                                                                      // per-froxel density + phase + depth + extinction
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

void xiiView::ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricGridAllocation");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_VisibilityPasses.m_pFroxelSetupPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetadata", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(128u + 7U) / 8U, (72U + 7U) / 8U, 64U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Cascade Setup Data //////////
//
// Sets up shadow cascades for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of cascade matrices and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.

struct xiiShadowCascadeSetupData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hCascadeMatrices;                              ///< UAV out (structured buffer of float4x4 cascade view-projection matrices, one per cascade, consumed by Shadow Passes).
  xiiUInt32                  m_uiActiveCascades = 0U;                         ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes.
  xiiVec3                    m_vLightDirection  = xiiVec3(0.0f, -1.0f, 0.0f); ///< Direction of the main directional light, used for computing cascade splits and matrices.
  float                      m_fNearPlane       = 0.1f;                       ///< Near plane distance for shadow cascades, used for computing cascade splits and matrices.
  float                      m_fFarPlane        = 1000.0f;                    ///< Far plane distance for shadow cascades, used for computing cascade splits and matrices.
};

void xiiView::SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRenderGraphBuilder& builder)
{
  data.m_uiActiveCascades = 3U;
  data.m_vLightDirection  = xiiVec3(0.0f, -1.0f, 0.0f);
  data.m_fNearPlane       = m_pCamera->GetNearPlane();
  data.m_fFarPlane        = m_pCamera->GetFarPlane();

  // Walk extracted data to find the first directional light.

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  for (xiiRenderData* pRenderData : renderData)
  {
    if (IsRenderDataTypeName(pRenderData, "xiiDirectionalLightRenderData"))
    {
      data.m_vLightDirection  = -pRenderData->m_GlobalTransform.m_qRotation.GetVectorPart();
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

void xiiView::ExecuteShadowCascadeSetup(const xiiShadowCascadeSetupData& data, xiiRenderGraphPassContext& context)
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

////////// GPU Local Shadow Atlas Allocation //////////
//
// Allocates deterministic atlas descriptors for all shadow-casting local lights.

struct xiiLocalShadowAtlasAllocationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hLightingDataReady;
  xiiRenderGraphBufferHandle m_hLocalShadowAtlasDescriptors;
};

void xiiView::SetupLocalShadowAtlasAllocation(xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 32U;
  description.m_uiSize              = description.m_uiElementByteStride * k_uiMaxLights;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hLocalShadowAtlasDescriptors = builder.WriteBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlasAllocationPipeline, "Shaders/Pipeline/LocalLightShadowAtlasAllocation.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteLocalShadowAtlasAllocation(const xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LocalShadowAtlasAllocation");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlasAllocationPipeline);
    m_ViewPassResources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LocalShadowAtlasDescs", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(xiiMath::Max(m_ViewPassResources.m_LightingSystem.GetActiveLightCount(), 1U) + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Directional Shadow Data //////////
//
// Collects all GPU resources related to directional shadow rendering for the current frame, including cascade matrices, shadow caster lists, and shadow atlases.

struct xiiDirectionalShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hCascadeMatrices;        ///< SRV in (structured buffer of float4x4 cascade view-projection matrices, one per cascade, from this frame's Shadow Cascade Setup pass).
  xiiRenderGraphBufferHandle  m_hShadowCasterCommands;   ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per cascade-per-bin, from this frame's Shadow Caster Build pass).
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas; ///< SRV in (texture atlas for directional shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiActiveCascades = 3U;   ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes and main lighting pass.
};

void xiiView::SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per spot light, from this frame's Shadow Caster Build pass).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;    ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiSpotLightCount = 0; ///< Number of active spot lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

void xiiView::SetupSpotShadowData(xiiSpotShadowData& data, xiiRenderGraphBuilder& builder)
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

  data.m_hShadowCasterCommands        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, m_ViewPassResources.m_ShadowPasses.m_pLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas            = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiSpotLightCount                            = CountRenderDataByTypeName(renderData, "xiiSpotLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per point light, from this frame's Shadow Caster Build pass).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;     ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiPointLightCount = 0; ///< Number of active point lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

void xiiView::SetupPointShadowData(xiiPointShadowData& data, xiiRenderGraphBuilder& builder)
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

  data.m_hShadowCasterCommands        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas            = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiPointLightCount                           = CountRenderDataByTypeName(renderData, "xiiPointLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecutePointShadowData(const xiiPointShadowData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hRTRawShadowMask; ///< UAV out (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by Shadow Denoise Pass).
  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< SRV in (depth texture from main render pass, used for ray-traced shadow ray generation and occlusion testing).
};

void xiiView::SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1u;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTRawShadowMask   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pRayTracedShadowPipeline, "Shaders/Pipeline/RTShadow.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("Ray-Traced Shadows");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pRayTracedShadowPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Denoise Data //////////
//
// Collects all GPU resources related to shadow denoising for the current frame, including raw shadow masks and final shadow masks.

struct xiiShadowDenoiseData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hRTRawShadowMask;   ///< SRV in (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by this pass).
  xiiRenderGraphTextureHandle m_hRTFinalShadowMask; ///< UAV out (texture containing final denoised ray-traced shadow masks, written by this pass, read by main lighting pass).
};

void xiiView::SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hRTRawShadowMask = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTFinalShadowMask = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

void xiiView::ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowDenoise");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pShadowDenoisePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Input", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Contact Shadow Data //////////
//
// Collects all GPU resources related to contact shadow rendering for the current frame, including scene depth and contact shadow masks.

struct xiiContactShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;    ///< SRV in (depth texture from main render pass, used for contact shadow ray generation and occlusion testing).
  xiiRenderGraphTextureHandle m_hContactShadow; ///< UAV out (texture containing contact shadow masks, written by this pass, read by main lighting pass).
};

void xiiView::SetupContactShadowData(xiiContactShadowData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_Type           = xiiGALResourceDimension::Texture2D;
  desc.m_Format         = xiiGALResourceFormat::R8UNormalized;
  desc.m_Size.width     = GetRenderResolutionWidth();
  desc.m_Size.height    = GetRenderResolutionHeight();
  desc.m_uiMipLevels    = 1U;
  desc.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hContactShadow = builder.WriteTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ShadowPasses.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

void xiiView::ExecuteContactShadowData(const xiiContactShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ContactShadows");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ShadowPasses.m_pContactShadowPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ContactShadowOut", context.GetTexture(data.m_hContactShadow)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Depth Prepass Data //////////
//
// Collects all GPU resources related to depth prepass rendering for the current frame, including the scene depth target and indirect draw commands.

struct xiiDepthPrepassData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthStencil out (full-resolution reversed-Z scene depth, written by this pass and consumed by later depth-dependent passes).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin, from this frame's Draw Build pass).
};

void xiiView::SetupDepthPrepass(xiiDepthPrepassData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::D32Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSceneDepth        = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, description, xiiGALResourceStateFlags::DepthWrite);

  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDepthPrepass(const xiiDepthPrepassData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DepthPrepass");
  {
    xiiGALTexture* pDepth = context.GetTexture(data.m_hSceneDepth);
    cmd.ClearDepthStencilView(pDepth->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture written by Depth Prepass, used as mip-0 source for Hi-Z generation).
  xiiRenderGraphTextureHandle m_hHiZPyramid;      ///< UnorderedAccess out (R32F max-depth hierarchy texture, consumed by Hi-Z occlusion culling and depth-aware effects).
  xiiUInt32                   m_uiMipLevels = 1U; ///< Number of mips in the Hi-Z pyramid, derived from the current viewport size.
};

void xiiView::SetupHiZPyramid(xiiHiZPyramidData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiBaseWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiBaseHeight = GetRenderResolutionHeight();

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

void xiiView::ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZPyramid");
  {
    if (m_ViewPassResources.m_DepthPasses.m_pHiZBuildPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_DepthPasses.m_pHiZBuildPipeline);

      xiiGALTexture* pDepth = context.GetTexture(data.m_hSceneDepth);
      xiiGALTexture* pHiZ   = context.GetTexture(data.m_hHiZPyramid);

      xiiUInt32 uiSrcWidth  = GetRenderResolutionWidth();
      xiiUInt32 uiSrcHeight = GetRenderResolutionHeight();

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHiZPyramid;         ///< ShaderResource in (Hi-Z pyramid generated by this frame's Hi-Z Pyramid pass).
  xiiRenderGraphBufferHandle  m_hVisibleCandidates;  ///< ShaderResource in (visible instance candidate list generated by this frame's Frustum Culling pass).
  xiiRenderGraphBufferHandle  m_hSurvivingInstances; ///< UnorderedAccess out (instance list surviving Hi-Z occlusion culling, consumed by later depth/lighting passes).
  xiiRenderGraphBufferHandle  m_hInstanceBounds;     ///< ShaderResource in (instance bounds buffer for occlusion testing).
  xiiUInt32                   m_uiInstanceCount = 0; ///< Number of instances to process.
};

void xiiView::SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthStencil inout (scene depth target reused for depth-tested motion vector rendering).
  xiiRenderGraphTextureHandle m_hVelocityBuffer;       ///< RenderTarget out (screen-space velocity buffer written by this pass).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupMotionVectors(xiiMotionVectorsData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityBuffer    = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MotionVectors");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hVelocityBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hVelocityInput;   ///< ShaderResource in (screen-space velocity buffer generated by the Motion Vectors pass).
  xiiRenderGraphTextureHandle m_hVelocityDilated; ///< UnorderedAccess out (dilated velocity buffer replacing the velocity blackboard key for downstream consumers).
};

void xiiView::SetupVelocityDilation(xiiVelocityDilationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hVelocityInput = builder.ReadTexture(xiiRGBlackboardKeys::k_VelocityBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityDilated   = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_DepthPasses.m_pVelocityDilationPipeline, "Shaders/Pipeline/Downscale.xiiShader");
}

void xiiView::ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRenderGraphPassContext& context)
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
      cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU G-Buffer Base Data //////////
//
// Collects all GPU resources related to base G-Buffer generation for the current frame, including the scene depth input, four G-Buffer targets, and indirect draw commands.

struct xiiGBufferBaseData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested G-Buffer rendering).
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;        ///< RenderTarget out (albedo and AO target).
  xiiRenderGraphTextureHandle m_hGBufferNormal;        ///< RenderTarget out (encoded normal target).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;      ///< RenderTarget out (material properties target).
  xiiRenderGraphTextureHandle m_hGBufferEmissive;      ///< RenderTarget out (emissive target).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupGBufferBase(xiiGBufferBaseData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
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

void xiiView::ExecuteGBufferBase(const xiiGBufferBaseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GBufferBase");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.0f, 1.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());

    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested rendering).
  xiiRenderGraphTextureHandle m_hNormalRoughness;      ///< RenderTarget out (compact normal/roughness/specular buffer consumed by GTAO and lighting prep passes).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

void xiiView::SetupNormalRoughnessPrepass(xiiNormalRoughnessPrepassData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hNormalRoughness   = builder.WriteTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteNormalRoughnessPrepass(const xiiNormalRoughnessPrepassData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("NormalRoughnessPrepass");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.5f, 1.0f));
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hBRDFLut;                 ///< Imported persistent BRDF LUT texture.
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch BRDF LUT generation.
};

void xiiView::SetupBRDFLutGeneration(xiiBRDFLutGenerationData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTransmittanceLUT;        ///< Imported persistent atmosphere transmittance LUT texture.
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch transmittance LUT generation.
};

void xiiView::SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hMultiScatterLUT;         ///< Imported persistent atmosphere multi-scatter LUT texture.
  xiiRenderGraphTextureHandle m_hTransmittanceLUT;        ///< ShaderResource in (atmosphere transmittance LUT).
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch multi-scatter LUT generation.
};

void xiiView::SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRenderGraphBuilder& builder)
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

void xiiView::ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphTextureHandle m_hMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRenderGraphTextureHandle m_hSkyRadiance;      ///< UnorderedAccess out (sky radiance texture used by later lighting passes).
};

void xiiView::SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSkyRadiance       = builder.WriteTexture(xiiRGBlackboardKeys::k_SkyRadiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pSkyIrradiancePipeline, "Shaders/Pipeline/ReflectionIrradiance.xiiShader");
}

void xiiView::ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SkyIrradianceConvolution");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pSkyIrradiancePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SkyOut", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Probe Convolution Data //////////
//
// Collects all GPU resources related to reflection probe specular convolution.

struct xiiReflectionProbeConvolutionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hReflectionProbeMask; ///< ShaderResource in (per-probe visibility/selection mask).
  xiiRenderGraphTextureHandle m_hBRDFLut;             ///< ShaderResource in (precomputed BRDF LUT for filtered specular).
};

void xiiView::SetupReflectionProbeConvolution(xiiReflectionProbeConvolutionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hReflectionProbeMask = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut             = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pReflProbeConvPipeline, "Shaders/Pipeline/ReflectionFilteredSpecular.xiiShader");
}

void xiiView::ExecuteReflectionProbeConvolution(const xiiReflectionProbeConvolutionData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hFroxelMetadata;   ///< ShaderResource in (froxel metadata buffer).
  xiiRenderGraphTextureHandle m_hFroxelScattering; ///< UnorderedAccess inout (froxel scattering texture).
};

void xiiView::SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelMetadata   = builder.ReadBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hFroxelScattering = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pFroxelFogInitPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
}

void xiiView::ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDDGIIrradiance; ///< UnorderedAccess out (DDGI irradiance result texture).
  xiiRenderGraphTextureHandle m_hSceneDepth;     ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;  ///< ShaderResource in (GBuffer normal texture).
};

void xiiView::SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDDGIIrradiance    = builder.WriteTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pDDGIProbePipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

void xiiView::ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DDGIProbeSampling");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pDDGIProbePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DDGIOut", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Data //////////
//
// Collects all GPU resources related to ground-truth ambient occlusion generation.

struct xiiGroundTruthAmbientOcclusionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;          ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hNormalRoughness;     ///< ShaderResource in (normal/roughness buffer).
  xiiRenderGraphTextureHandle m_hRawAmbientOcclusion; ///< UnorderedAccess out (raw ambient occlusion result).
};

void xiiView::SetupGroundTruthAmbientOcclusion(xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hNormalRoughness = builder.ReadTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type          = xiiGALResourceDimension::Texture2D;
  description.m_Format        = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width    = GetRenderResolutionWidth();
  description.m_Size.height   = GetRenderResolutionHeight();
  description.m_uiMipLevels   = 1U;
  description.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage         = xiiGALResourceUsage::Default;
  data.m_hRawAmbientOcclusion = builder.WriteTexture(xiiRGBlackboardKeys::k_RawAOTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pGTAOPipeline, "Shaders/Pipeline/GTAO.xiiShader");
}

void xiiView::ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusion");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pGTAOPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Denoise Data //////////
//
// Collects all GPU resources related to denoising ground-truth ambient occlusion.

struct xiiGroundTruthAmbientOcclusionDenoiseData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hRawAmbientOcclusion;    ///< ShaderResource in (raw ambient occlusion texture).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion; ///< UnorderedAccess out (denoised ambient occlusion texture).
};

void xiiView::SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hRawAmbientOcclusion = builder.ReadTexture(xiiRGBlackboardKeys::k_RawAOTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width       = GetRenderResolutionWidth();
  description.m_Size.height      = GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hStableAmbientOcclusion = builder.WriteTexture(xiiRGBlackboardKeys::k_StableAOTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPrepPasses.m_pGTAODenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

void xiiView::ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusionDenoise");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPrepPasses.m_pGTAODenoisePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_Input", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Output", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Direct Lighting Data //////////
//
// Collects all GPU resources related to deferred direct lighting.

struct xiiDeferredDirectLightingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hGBufferAlbedo;            ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle m_hGBufferNormal;            ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;          ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hSceneDepth;               ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion;   ///< ShaderResource in (stable ambient occlusion).
  xiiRenderGraphTextureHandle m_hRayTracedFinalShadowMask; ///< ShaderResource in (denoised ray traced shadows).
  xiiRenderGraphTextureHandle m_hContactShadowTerm;        ///< ShaderResource in (contact shadow mask).
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas;   ///< ShaderResource in (directional shadow atlas).
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;         ///< ShaderResource in (local light shadow atlas).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;      ///< ShaderResource in (cluster light grid).
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;     ///< ShaderResource in (cluster light indices).
  xiiRenderGraphTextureHandle m_hDirectLightingBuffer; ///< UnorderedAccess out (direct lighting HDR buffer).
};

void xiiView::SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hGBufferAlbedo               = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal               = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial             = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                  = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion      = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalShadowMask    = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hContactShadowTerm           = builder.ReadTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDirectionalShadowAtlas      = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer             = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer            = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = GetRenderResolutionWidth();
  description.m_Size.height    = GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hDirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pDirectLightingPipeline, "Shaders/Pipeline/DirectLighting.xiiShader");
}

void xiiView::ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredDirectLighting");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pDirectLightingPipeline);
    m_ViewPassResources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RTShadow", context.GetTexture(data.m_hRayTracedFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ContactShadow", context.GetTexture(data.m_hContactShadowTerm)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_LocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LocalShadowAtlasDescs", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DirectOut", context.GetTexture(data.m_hDirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Indirect Lighting Data //////////
//
// Collects all GPU resources related to deferred indirect lighting.

struct xiiDeferredIndirectLightingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hGBufferAlbedo;          ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle m_hGBufferNormal;          ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;        ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hSceneDepth;             ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion; ///< ShaderResource in (stable ambient occlusion).
  xiiRenderGraphTextureHandle m_hBRDFLut;                ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle m_hDDGIIrradiance;         ///< ShaderResource in (DDGI irradiance texture).
  xiiRenderGraphTextureHandle m_hSkyRadiance;            ///< ShaderResource in (sky radiance texture).
  xiiRenderGraphTextureHandle m_hIndirectLightingBuffer; ///< UnorderedAccess out (indirect lighting HDR buffer).
};

void xiiView::SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRenderGraphBuilder& builder)
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
  description.m_Size.width       = GetRenderResolutionWidth();
  description.m_Size.height      = GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hIndirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pIndirectLightingPipeline, "Shaders/Pipeline/IndirectLighting.xiiShader");
}

void xiiView::ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredIndirectLighting");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pIndirectLightingPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

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
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Global Illumination Data //////////
//
// Collects all GPU resources related to ray traced global illumination final gather.

struct xiiRayTracedGlobalIlluminationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;                       ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;                    ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hIndirectLightingInput;            ///< ShaderResource in (deferred indirect lighting input).
  xiiRenderGraphTextureHandle m_hRayTracedRawGlobalIllumination;   ///< UnorderedAccess out (raw RT GI texture).
  xiiRenderGraphTextureHandle m_hRayTracedFinalGlobalIllumination; ///< UnorderedAccess out (final RT GI texture).
};

void xiiView::SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth            = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLightingInput = builder.ReadTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                       = xiiGALResourceDimension::Texture2D;
  description.m_Format                     = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width                 = GetRenderResolutionWidth();
  description.m_Size.height                = GetRenderResolutionHeight();
  description.m_uiMipLevels                = 1U;
  description.m_BindFlags                  = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage                      = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawGlobalIllumination   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawGI, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hRayTracedFinalGlobalIllumination = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pRTGIPipeline, "Shaders/Pipeline/RTGIFinalGather.xiiShader");
}

void xiiView::ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphPassContext& context)
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
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Data //////////
//
// Collects all GPU resources related to ray traced reflections.

struct xiiRayTracedReflectionsData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;                ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;             ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;           ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hBRDFLut;                   ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle m_hRayTracedRawReflections;   ///< UnorderedAccess out (raw RT reflections texture).
  xiiRenderGraphTextureHandle m_hRayTracedFinalReflections; ///< UnorderedAccess out (final RT reflections texture).
};

void xiiView::SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut         = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                = xiiGALResourceDimension::Texture2D;
  description.m_Format              = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width          = GetRenderResolutionWidth();
  description.m_Size.height         = GetRenderResolutionHeight();
  description.m_uiMipLevels         = 1U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawReflections   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawReflections, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hRayTracedFinalReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalReflections, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pRTReflectionPipeline, "Shaders/Pipeline/RTReflection.xiiShader");
}

void xiiView::ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRenderGraphPassContext& context)
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
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen Space Reflections Data //////////
//
// Collects all GPU resources related to screen-space reflections.

struct xiiScreenSpaceReflectionsData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;             ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;          ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;        ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;          ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections; ///< UnorderedAccess out (screen-space reflections texture).
};

void xiiView::SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor   = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = GetRenderResolutionWidth();
  description.m_Size.height      = GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hScreenSpaceReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_SSRTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pSSRPipeline, "Shaders/Pipeline/SSR.xiiShader");
}

void xiiView::ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRenderGraphPassContext& context)
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
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Integration Data //////////
//
// Collects all GPU resources related to volumetric fog integration.

struct xiiVolumetricFogIntegrationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelScatteringBuffer; ///< ShaderResource in (froxel scattering buffer).
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;        ///< ShaderResource in (cluster light grid).
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;       ///< ShaderResource in (cluster light indices).
  xiiRenderGraphTextureHandle m_hVolumetricScattering;   ///< UnorderedAccess out (integrated volumetric scattering).
};

void xiiView::SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelScatteringBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer        = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer       = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = GetRenderResolutionWidth();
  description.m_Size.height    = GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hVolumetricScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_VolumetricScattering, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pVolumetricIntegratePipeline, "Shaders/Pipeline/VolumetricLightIntegration.xiiShader");
}

void xiiView::ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogIntegrate");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_LightingPasses.m_pVolumetricIntegratePipeline);

    m_ViewPassResources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScatteringBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_VolumetricOut", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Temporal Reprojection Data //////////
//
// Collects all GPU resources related to volumetric fog temporal reprojection.

struct xiiVolumetricFogTemporalReprojectionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelHistory;        ///< ShaderResource in (history froxel volume from previous frame).
  xiiRenderGraphTextureHandle m_hVolumetricScattering; ///< UnorderedAccess in/out (current volumetric scattering buffer).
};

void xiiView::SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphBuilder& builder)
{
  if (m_ViewPassResources.m_LightingPasses.m_pFroxelHistoryBuffer == nullptr)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = xiiGALResourceDimension::Texture3D;
    description.m_Format             = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width         = GetRenderResolutionWidth();
    description.m_Size.height        = GetRenderResolutionHeight();
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

void xiiView::ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphPassContext& context)
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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hAtmosphereTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphTextureHandle m_hAtmosphereMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRenderGraphTextureHandle m_hVolumetricScattering;       ///< ShaderResource in (volumetric scattering buffer).
  xiiRenderGraphTextureHandle m_hSkyRadiance;                ///< ShaderResource in (sky radiance texture).
};

void xiiView::SetupAtmosphereComposite(xiiAtmosphereCompositeData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering       = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSkyRadiance                = builder.ReadTexture(xiiRGBlackboardKeys::k_SkyRadiance, xiiGALResourceStateFlags::ShaderResource);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_LightingPasses.m_pAtmosphereCompositePipeline, "Shaders/Pipeline/AtmosphereComposite.xiiShader");
}

void xiiView::ExecuteAtmosphereComposite(const xiiAtmosphereCompositeData& data, xiiRenderGraphPassContext& context)
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
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Forward Opaque Data //////////
//
// Collects all GPU resources related to the forward opaque pass.

struct xiiForwardOpaqueData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;                ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hDirectLighting;            ///< ShaderResource in (direct lighting texture).
  xiiRenderGraphTextureHandle m_hIndirectLighting;          ///< ShaderResource in (indirect lighting texture).
  xiiRenderGraphTextureHandle m_hRayTracedFinalGI;          ///< ShaderResource in (final ray-traced global illumination texture).
  xiiRenderGraphTextureHandle m_hRayTracedFinalReflections; ///< ShaderResource in (final ray-traced reflections texture).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections;    ///< ShaderResource in (screen-space reflections texture).
  xiiRenderGraphTextureHandle m_hVolumetricScattering;      ///< ShaderResource in (volumetric scattering texture).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion;    ///< ShaderResource in (stable ambient occlusion texture).
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;           ///< ShaderResource in (cluster light grid buffer).
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;          ///< ShaderResource in (cluster light index buffer).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands;      ///< IndirectArgument in (draw indirect commands).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;             ///< RenderTarget out (composited HDR scene color).
};

void xiiView::SetupForwardOpaque(xiiForwardOpaqueData& data, xiiRenderGraphBuilder& builder)
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
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor     = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteForwardOpaque(const xiiForwardOpaqueData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ForwardOpaque");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupForwardMasked(xiiForwardMaskedData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteForwardMasked(const xiiForwardMaskedData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ForwardMasked");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupHairRendering(xiiHairRenderingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteHairRendering(const xiiHairRenderingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HairRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRenderGraphTextureHandle m_hPlanarReflectionMap;  ///< ShaderResource in (planar reflection map).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupWaterRendering(xiiWaterRenderingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hPlanarReflectionMap  = builder.ReadTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteWaterRendering(const xiiWaterRenderingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("WaterRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

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
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;   ///< UnorderedAccess in/out (HDR scene color).
  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferMaterial; ///< ShaderResource in (material G-Buffer).
};

void xiiView::SetupSubsurfaceScattering(xiiSubsurfaceScatteringData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
}

void xiiView::ExecuteSubsurfaceScattering(const xiiSubsurfaceScatteringData& data, xiiRenderGraphPassContext& context)
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

      const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
      const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Eye Shader Data //////////
//
// Collects all GPU resources related to the eye shading pass.

struct xiiEyeShaderData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< RenderTarget in/out (HDR scene color).
  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthWrite in/out (scene depth texture).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupEyeShader(xiiEyeShaderData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiView::ExecuteEyeShader(const xiiEyeShaderData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("EyeShader");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_ViewPassResources.m_ForwardPasses.m_pEyePipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_ForwardPasses.m_pEyePipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Particle Simulate Data //////////
//
// Collects all GPU resources related to the particle simulation pass.

struct xiiGPUParticleSimulateData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hParticleState;          ///< UnorderedAccess in/out (persistent particle state buffer).
  xiiRenderGraphBufferHandle m_hParticleConstants;      ///< ConstantBuffer in (simulation time step and live count).
  xiiUInt32                  m_uiParticleCount = 65536; ///< Number of particles to simulate.
};

struct alignas(16) xiiGPUParticleSimulateConstants
{
  XII_DECLARE_POD_TYPE();

  float     m_fDeltaTimeS      = 0.0f;
  float     m_fGravityScale    = 1.0f;
  float     m_fDragCoefficient = 0.05f;
  float     m_fTurbulenceScale = 0.0f;
  xiiVec3   m_vWindVelocity    = xiiVec3::MakeZero();
  xiiUInt32 m_uiActiveCount    = 0U;
};

static_assert((sizeof(xiiGPUParticleSimulateConstants) % 16U) == 0U);

void xiiView::SetupGPUParticleSimulate(xiiGPUParticleSimulateData& data, xiiRenderGraphBuilder& builder)
{
  constexpr xiiUInt32 uiDefaultParticleCapacity = xiiParticleSystemConstants::s_uiDefaultMaxParticles;

  if (!m_ViewPassResources.m_TransparencyPasses.m_pParticleStateBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiParticleGPUState);
    description.m_uiSize              = description.m_uiElementByteStride * uiDefaultParticleCapacity;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_ViewPassResources.m_TransparencyPasses.m_pParticleStateBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    m_ViewPassResources.m_TransparencyPasses.m_uiParticleCapacity   = uiDefaultParticleCapacity;
  }

  data.m_hParticleState  = builder.ImportBuffer("ParticleState", m_ViewPassResources.m_TransparencyPasses.m_pParticleStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hParticleState  = builder.WriteBuffer(data.m_hParticleState, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiParticleCount = xiiMath::Max(1U, m_ViewPassResources.m_TransparencyPasses.m_uiParticleCapacity);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiGPUParticleSimulateConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;

  data.m_hParticleConstants = builder.WriteBuffer("ParticleSimConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TransparencyPasses.m_pParticleSimulatePipeline, "Shaders/Pipeline/GPUParticleSimulate.xiiShader");

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteGPUParticleSimulate(const xiiGPUParticleSimulateData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GPUParticleSimulate");
  {
    {
      xiiGALMapHelper<xiiGPUParticleSimulateConstants> pConstants(cmd, context.GetBuffer(data.m_hParticleConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->m_fDeltaTimeS      = xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 1.0f / 15.0f);
      pConstants->m_fGravityScale    = 1.0f;
      pConstants->m_fDragCoefficient = 0.05f;
      pConstants->m_fTurbulenceScale = 0.0f;
      pConstants->m_vWindVelocity    = xiiVec3::MakeZero();
      pConstants->m_uiActiveCount    = data.m_uiParticleCount;
    }

    cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pParticleSimulatePipeline);
    cmd.ResolveAndSetConstantBuffer("ParticleSimConstants", context.GetBuffer(data.m_hParticleConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Particles", context.GetBuffer(data.m_hParticleState)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiParticleCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

namespace
{
  static constexpr xiiUInt32 k_uiDecalTileSize               = 16U;
  static constexpr xiiUInt32 k_uiMaxProjectedDecalsPerTile   = 12U;
  static constexpr xiiUInt32 k_uiMaxProjectedDecalTileStride = 1U + k_uiMaxProjectedDecalsPerTile;
} // namespace

////////// GPU Decal Upload Data //////////
//
// Uploads extracted projected and mesh decal instances into a GPU-visible structured buffer and imports the active atlas set.

struct alignas(16) xiiGPUDecalInstance
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_WorldToDecal      = xiiMat4::MakeIdentity();
  xiiVec4   m_AtlasUVRect       = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);
  xiiVec4   m_Tint              = xiiVec4(1.0f);
  xiiVec4   m_UVOffsetScale     = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);
  xiiVec4   m_ExtentsOpacity    = xiiVec4(1.0f, 1.0f, 0.25f, 1.0f);
  xiiVec4   m_WorldCenterRadius = xiiVec4::MakeZero();
  xiiVec4   m_SurfaceParams     = xiiVec4(1.0f, 0.5f, 0.0f, 0.0f);
  xiiUInt32 m_uiChannelMask     = 0U;
  xiiUInt32 m_uiMode            = 0U;
  xiiUInt32 m_uiPriority        = 0U;
  xiiUInt32 m_uiFlags           = 0U;
};

static_assert((sizeof(xiiGPUDecalInstance) % 16U) == 0U);

struct xiiDecalUploadData
{
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;

  xiiDynamicArray<xiiGPUDecalInstance, xiiAlignedAllocatorWrapper> m_Decals;
  xiiUInt32                                                        m_uiDecalCount = 0U;
};

void xiiView::SetupDecalUpload(xiiDecalUploadData& data, xiiRenderGraphBuilder& builder)
{
  auto EnsureFallbackTexture = [&](xiiSharedPtr<xiiGALTexture>& inout_pTexture, xiiUInt32 uiClearValue, xiiStringView sDebugName) {
    if (inout_pTexture != nullptr)
      return;

    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
    textureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
    textureDescription.m_Size.width  = 1U;
    textureDescription.m_Size.height = 1U;
    textureDescription.m_uiMipLevels = 1U;
    textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
    textureDescription.m_Usage       = xiiGALResourceUsage::Default;

    xiiUInt32 uiPixel = uiClearValue;

    xiiHybridArray<xiiGALTextureSubResourceData, 1U> initData;
    xiiGALTextureSubResourceData&                    subResourceData = initData.ExpandAndGetRef();
    subResourceData.m_pData                                          = xiiMakeByteBlobPtr(static_cast<const void*>(&uiPixel), sizeof(uiPixel));
    subResourceData.m_uiStride                                       = sizeof(uiPixel);
    subResourceData.m_uiDepthStride                                  = sizeof(uiPixel);

    xiiGALTextureData textureData(initData);
    inout_pTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(textureDescription, &textureData);
    if (inout_pTexture)
    {
      inout_pTexture->SetDebugName(sDebugName);
    }
  };

  auto EnsureFallbackAtlases = [&]() {
    EnsureFallbackTexture(m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture, 0xFFFFFFFFU, "FallbackDecalAtlas::Albedo");
    EnsureFallbackTexture(m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture, 0xFFFF8080U, "FallbackDecalAtlas::Normal");
    EnsureFallbackTexture(m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture, 0x00FF0080U, "FallbackDecalAtlas::Material");
    EnsureFallbackTexture(m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture, 0x00000000U, "FallbackDecalAtlas::Emissive");

    if (m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalAtlasSampler == nullptr)
    {
      xiiGALSamplerCreationDescription samplerDescription                   = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
      samplerDescription.m_AddressU                                         = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressV                                         = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressW                                         = xiiGALTextureAddressMode::Clamp;
      m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalAtlasSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(samplerDescription);
    }
  };

  EnsureFallbackAtlases();

  xiiSharedPtr<xiiGALTexture> pAtlasAlbedo   = m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasNormal   = m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasMaterial = m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasEmissive = m_ViewPassResources.m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture;

  xiiDecalAtlasResourceHandle hSelectedAtlas;

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_Decals.Reserve(renderData.GetCount());

  const xiiRTTI* pDecalType = xiiGetStaticRTTI<xiiDecalRenderData>();

  for (const xiiRenderData* pBaseRenderData : renderData)
  {
    if (pBaseRenderData == nullptr || pBaseRenderData->GetDynamicRTTI() == nullptr || !pBaseRenderData->GetDynamicRTTI()->IsDerivedFrom(pDecalType))
      continue;

    const xiiDecalRenderData* pDecal = static_cast<const xiiDecalRenderData*>(pBaseRenderData);
    if (pDecal->m_fOpacity <= 0.0f || pDecal->m_ChannelMask.IsNoFlagSet())
      continue;

    if (pDecal->m_hAtlas.IsValid() && hSelectedAtlas.IsValid() && pDecal->m_hAtlas != hSelectedAtlas)
      continue;

    if (pDecal->m_hAtlas.IsValid() && !hSelectedAtlas.IsValid())
    {
      xiiResourceLock<xiiDecalAtlasResource> pAtlas(pDecal->m_hAtlas, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAtlas)
      {
        hSelectedAtlas = pDecal->m_hAtlas;
        pAtlasAlbedo   = pAtlas->GetAlbedoTexture() != nullptr ? pAtlas->GetAlbedoTexture() : pAtlasAlbedo;
        pAtlasNormal   = pAtlas->GetNormalTexture() != nullptr ? pAtlas->GetNormalTexture() : pAtlasNormal;
        pAtlasMaterial = pAtlas->GetMaterialTexture() != nullptr ? pAtlas->GetMaterialTexture() : pAtlasMaterial;
        pAtlasEmissive = pAtlas->GetEmissiveTexture() != nullptr ? pAtlas->GetEmissiveTexture() : pAtlasEmissive;
      }
    }

    xiiGPUDecalInstance& gpuDecal = data.m_Decals.ExpandAndGetRef();
    gpuDecal.m_WorldToDecal       = pDecal->m_GlobalTransform.GetInverse().GetAsMat4();
    gpuDecal.m_AtlasUVRect        = pDecal->m_vAtlasUVRect;
    gpuDecal.m_Tint               = xiiVec4(pDecal->m_Tint.r, pDecal->m_Tint.g, pDecal->m_Tint.b, pDecal->m_Tint.a);
    gpuDecal.m_UVOffsetScale      = xiiVec4(pDecal->m_vUVOffset.x, pDecal->m_vUVOffset.y, pDecal->m_vUVScale.x, pDecal->m_vUVScale.y);
    gpuDecal.m_ExtentsOpacity     = xiiVec4(pDecal->m_vExtents.x, pDecal->m_vExtents.y, pDecal->m_vExtents.z, pDecal->m_fOpacity);

    const auto boundsSphere      = pDecal->m_GlobalBounds.GetSphere();
    gpuDecal.m_WorldCenterRadius = xiiVec4(boundsSphere.m_vCenter.x, boundsSphere.m_vCenter.y, boundsSphere.m_vCenter.z, boundsSphere.m_fRadius);
    gpuDecal.m_SurfaceParams     = xiiVec4(pDecal->m_fNormalBlend, pDecal->m_fRoughness, pDecal->m_fMetallic, pDecal->m_fEmissive);
    gpuDecal.m_uiChannelMask     = pDecal->m_ChannelMask.GetValue();
    gpuDecal.m_uiMode            = pDecal->m_Mode.GetValue();
    gpuDecal.m_uiPriority        = pDecal->m_uiPriority;
  }

  data.m_uiDecalCount = data.m_Decals.GetCount();
  m_Blackboard.Set(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiElementByteStride = sizeof(xiiGPUDecalInstance);
  bufferDescription.m_uiSize              = xiiMath::Max<xiiUInt32>(1U, data.m_uiDecalCount) * bufferDescription.m_uiElementByteStride;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hDecalData                       = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hAtlasAlbedo   = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, pAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal   = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, pAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, pAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, pAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDecalUpload(const xiiDecalUploadData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_Decals.IsEmpty())
    return;

  xiiGALBuffer* pDecalBuffer = context.GetBuffer(data.m_hDecalData);
  if (pDecalBuffer == nullptr)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DecalUpload");
  {
    const xiiUInt8* pBytes = reinterpret_cast<const xiiUInt8*>(data.m_Decals.GetData());
    cmd.UpdateBuffer(pDecalBuffer, 0U, xiiMakeArrayPtr(pBytes, data.m_Decals.GetCount() * sizeof(xiiGPUDecalInstance)));
  }
  cmd.EndDebugGroup();
}

////////// GPU Decal Cull & Batch Data //////////
//
// Performs GPU-side frustum culling and builds projected tile bins plus a compact mesh-decal list.

struct xiiDecalCullBatchData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hVisibleList;
  xiiRenderGraphBufferHandle  m_hProjectedTileList;
  xiiRenderGraphBufferHandle  m_hMeshDrawCommands;

  xiiUInt32 m_uiDecalCount = 0U;
  xiiUInt32 m_uiTileCount  = 0U;
};

void xiiView::SetupDecalCullBatch(xiiDecalCullBatchData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData  = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);

  const bool bHasDecalCount = m_Blackboard.TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasDecalCount);

  const xiiUInt32 uiTileCountX = (GetRenderResolutionWidth() + k_uiDecalTileSize - 1U) / k_uiDecalTileSize;
  const xiiUInt32 uiTileCountY = (GetRenderResolutionHeight() + k_uiDecalTileSize - 1U) / k_uiDecalTileSize;
  data.m_uiTileCount           = uiTileCountX * uiTileCountY;

  xiiGALBufferCreationDescription visibleDescription;
  visibleDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  visibleDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(2U, data.m_uiDecalCount + 1U);
  visibleDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  visibleDescription.m_Mode                = xiiGALBufferMode::Structured;
  visibleDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hVisibleList = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalVisibleList, visibleDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription tileDescription;
  tileDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  tileDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(1U, data.m_uiTileCount * k_uiMaxProjectedDecalTileStride);
  tileDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  tileDescription.m_Mode                = xiiGALBufferMode::Structured;
  tileDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hProjectedTileList = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalTileList, tileDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription meshCommandDescription;
  meshCommandDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  meshCommandDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(2U, data.m_uiDecalCount + 1U);
  meshCommandDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  meshCommandDescription.m_Mode                = xiiGALBufferMode::Structured;
  meshCommandDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hMeshDrawCommands = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalDrawCommands, meshCommandDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TransparencyPasses.m_pDecalCullBatchPipeline, "Shaders/Pipeline/DecalClassification.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDecalCullBatch(const xiiDecalCullBatchData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  auto ClearStructuredUIntBuffer = [&cmd](xiiGALBuffer* pBuffer) {
    const xiiUInt32            uiValueCount = static_cast<xiiUInt32>(pBuffer->GetDescription().m_uiSize / sizeof(xiiUInt32));
    xiiDynamicArray<xiiUInt32> zeroData;
    zeroData.SetCount(uiValueCount);
    for (xiiUInt32& uiValue : zeroData)
    {
      uiValue = 0U;
    }

    const xiiUInt8* pBytes = reinterpret_cast<const xiiUInt8*>(zeroData.GetData());
    cmd.UpdateBuffer(pBuffer, 0U, xiiMakeArrayPtr(pBytes, zeroData.GetCount() * sizeof(xiiUInt32)));
  };

  xiiGALBuffer* pVisibleList     = context.GetBuffer(data.m_hVisibleList);
  xiiGALBuffer* pProjectedTiles  = context.GetBuffer(data.m_hProjectedTileList);
  xiiGALBuffer* pMeshDrawCommand = context.GetBuffer(data.m_hMeshDrawCommands);

  cmd.BeginDebugGroup("DecalCullBatch");
  {
    ClearStructuredUIntBuffer(pVisibleList);
    ClearStructuredUIntBuffer(pProjectedTiles);
    ClearStructuredUIntBuffer(pMeshDrawCommand);

    cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pDecalCullBatchPipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleDecals", pVisibleList->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_TileLists", pProjectedTiles->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_MeshDecalCommands", pMeshDrawCommand->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiDecalCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Projected Decal Resolve Data //////////
//
// Resolves projected deferred decals into the live G-Buffer surfaces.

struct xiiProjectedDecalResolveData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hProjectedTileList;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;

  xiiUInt32 m_uiDecalCount = 0U;
};

void xiiView::SetupProjectedDecalResolve(xiiProjectedDecalResolveData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData         = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProjectedTileList = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalTileList, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasAlbedo       = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal       = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial     = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive     = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  data.m_hGBufferAlbedo   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferNormal   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferMaterial = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferEmissive = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  const bool bHasProjectedDecalCount = m_Blackboard.TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasProjectedDecalCount);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TransparencyPasses.m_pSSDecalResolvePipeline, "Shaders/Pipeline/DecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ProjectedDecalResolve");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pSSDecalResolvePipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_TileLists", context.GetBuffer(data.m_hProjectedTileList)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasAlbedo", context.GetTexture(data.m_hAtlasAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasNormal", context.GetTexture(data.m_hAtlasNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasMaterial", context.GetTexture(data.m_hAtlasMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasEmissive", context.GetTexture(data.m_hAtlasEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Mesh Decal Resolve Data //////////
//
// Resolves mesh decals as a distinct GPU pass using the mesh-decal list built during classification.

struct xiiMeshDecalDrawData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hMeshDrawCommands;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;

  xiiUInt32 m_uiDecalCount = 0U;
};

void xiiView::SetupMeshDecalDraw(xiiMeshDecalDrawData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth       = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMeshDrawCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasAlbedo      = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal      = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial    = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive    = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  data.m_hGBufferAlbedo   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferNormal   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferMaterial = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferEmissive = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  const bool bHasMeshDecalCount = m_Blackboard.TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasMeshDecalCount);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TransparencyPasses.m_pMeshDecalResolvePipeline, "Shaders/Pipeline/MeshDecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MeshDecalDraw");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pMeshDecalResolvePipeline);
    m_ViewPassResources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_MeshDecalCommands", context.GetBuffer(data.m_hMeshDrawCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasAlbedo", context.GetTexture(data.m_hAtlasAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasNormal", context.GetTexture(data.m_hAtlasNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasMaterial", context.GetTexture(data.m_hAtlasMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasEmissive", context.GetTexture(data.m_hAtlasEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Weighted Blended OIT Data //////////
//
// Collects all GPU resources related to weighted blended transparency accumulation and resolve.

struct xiiWeightedBlendedOITData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< UnorderedAccess in/out (HDR scene color target).
  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth for translucent geometry).
  xiiRenderGraphTextureHandle m_hOITAccumulate;        ///< RenderTarget out / ShaderResource in (weighted accumulation target).
  xiiRenderGraphTextureHandle m_hOITReveal;            ///< RenderTarget out / ShaderResource in (reveal target).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (draw indirect commands).
};

void xiiView::SetupWeightedBlendedOIT(xiiWeightedBlendedOITData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription accumulateDescription;
  accumulateDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  accumulateDescription.m_Format      = xiiGALResourceFormat::RGBA16Float;
  accumulateDescription.m_Size.width  = uiRenderWidth;
  accumulateDescription.m_Size.height = uiRenderHeight;
  accumulateDescription.m_uiMipLevels = 1U;
  accumulateDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  accumulateDescription.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hOITAccumulate               = builder.WriteTexture(xiiRGBlackboardKeys::k_OITAccumulateBuffer, accumulateDescription, xiiGALResourceStateFlags::RenderTarget);

  xiiGALTextureCreationDescription revealDescription;
  revealDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  revealDescription.m_Format      = xiiGALResourceFormat::R8UNormalized;
  revealDescription.m_Size.width  = uiRenderWidth;
  revealDescription.m_Size.height = uiRenderHeight;
  revealDescription.m_uiMipLevels = 1U;
  revealDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  revealDescription.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hOITReveal               = builder.WriteTexture(xiiRGBlackboardKeys::k_OITRevealBuffer, revealDescription, xiiGALResourceStateFlags::RenderTarget);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TransparencyPasses.m_pOITResolvePipeline, "Shaders/Pipeline/OITResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteWeightedBlendedOIT(const xiiWeightedBlendedOITData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("WeightedBlendedOIT");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hOITAccumulate)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hOITReveal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(1.0f, 1.0f, 1.0f, 1.0f));
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(uiRenderWidth), static_cast<float>(uiRenderHeight), 0.0f, 1.0f});

    if (m_ViewPassResources.m_TransparencyPasses.m_pTranslucentPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pTranslucentPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands)});
    }

    if (m_ViewPassResources.m_TransparencyPasses.m_pOITResolvePipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources.m_TransparencyPasses.m_pOITResolvePipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_OITAccum", context.GetTexture(data.m_hOITAccumulate)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_OITReveal", context.GetTexture(data.m_hOITReveal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen-Space Global Illumination Data //////////
//
// Collects all GPU resources related to the SSGI pass.

struct xiiScreenSpaceGlobalIlluminationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;    ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal; ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hHDRIn;         ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hSSGIOut;       ///< UnorderedAccess out (screen-space GI term).
};

void xiiView::SetupScreenSpaceGlobalIllumination(xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn         = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSSGIOut           = builder.WriteTexture("SSGITerm", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ScreenSpacePasses.m_pSSGIPipeline, "Shaders/Pipeline/SSGI.xiiShader");
}

void xiiView::ExecuteScreenSpaceGlobalIllumination(const xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("SSGI");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ScreenSpacePasses.m_pSSGIPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSGIOut", context.GetTexture(data.m_hSSGIOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen-Space Refraction Data //////////
//
// Collects all GPU resources related to screen-space refraction.

struct xiiScreenSpaceRefractionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;    ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal; ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hHDRIn;         ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hHDROut;        ///< UnorderedAccess in/out (HDR scene color target).
};

void xiiView::SetupScreenSpaceRefraction(xiiScreenSpaceRefractionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn         = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDROut        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_ScreenSpacePasses.m_pSSRefractionPipeline, "Shaders/Pipeline/SSRefraction.xiiShader");
}

void xiiView::ExecuteScreenSpaceRefraction(const xiiScreenSpaceRefractionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("SSRefraction");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_ScreenSpacePasses.m_pSSRefractionPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDROut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Planar Reflections Data //////////
//
// Collects all GPU resources related to planar reflection rendering.

struct xiiPlanarReflectionsData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hPlanarTarget; ///< RenderTarget out (planar reflection render target).
};

void xiiView::SetupPlanarReflections(xiiPlanarReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  if (!m_ViewPassResources.m_ScreenSpacePasses.m_pPlanarReflectionTarget)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = xiiMath::Max(1U, uiRenderWidth / 2U);
    description.m_Size.height = xiiMath::Max(1U, uiRenderHeight / 2U);
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    m_ViewPassResources.m_ScreenSpacePasses.m_pPlanarReflectionTarget = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hPlanarTarget = builder.ImportTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, m_ViewPassResources.m_ScreenSpacePasses.m_pPlanarReflectionTarget, xiiGALResourceStateFlags::RenderTarget);
  data.m_hPlanarTarget = builder.WriteTexture(data.m_hPlanarTarget, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecutePlanarReflections(const xiiPlanarReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("PlanarReflections");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hPlanarTarget)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    // Secondary view reflection rendering is scheduled by the render world module.
  }
  cmd.EndDebugGroup();
}

////////// GPU Luminance Histogram Data //////////
//
// Collects all GPU resources related to luminance histogram generation.

struct xiiLuminanceHistogramData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRIn;     ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphBufferHandle  m_hHistogram; ///< UnorderedAccess out (256-bin luminance histogram).
};

void xiiView::SetupLuminanceHistogram(xiiLuminanceHistogramData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = description.m_uiElementByteStride * 256U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hHistogram                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TemporalPasses.m_pLuminanceHistogramPipeline, "Shaders/Pipeline/ExposureHistogram.xiiShader");
}

void xiiView::ExecuteLuminanceHistogram(const xiiLuminanceHistogramData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("LuminanceHistogram");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TemporalPasses.m_pLuminanceHistogramPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Auto Exposure Data //////////
//
// Collects all GPU resources related to histogram-based exposure adaptation.

struct xiiAutoExposureData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hHistogram; ///< ShaderResource in (luminance histogram).
  xiiRenderGraphBufferHandle m_hExposure;  ///< UnorderedAccess in/out (persistent exposure value).
};

void xiiView::SetupAutoExposure(xiiAutoExposureData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHistogram = builder.ReadBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, xiiGALResourceStateFlags::ShaderResource);

  if (!m_ViewPassResources.m_TemporalPasses.m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = 4U;
    description.m_uiSize              = 4U; // single float EV100 value
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_ViewPassResources.m_TemporalPasses.m_pExposureBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_hExposure = builder.ImportBuffer(xiiRGBlackboardKeys::k_CurrentExposure, m_ViewPassResources.m_TemporalPasses.m_pExposureBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hExposure = builder.WriteBuffer(data.m_hExposure, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TemporalPasses.m_pAutoExposurePipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader");
}

void xiiView::ExecuteAutoExposure(const xiiAutoExposureData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AutoExposure");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TemporalPasses.m_pAutoExposurePipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({1U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Temporal Anti-Aliasing Data //////////
//
// Collects all GPU resources related to temporal anti-aliasing resolve.

struct xiiTemporalAntiAliasingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRIn;    ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hVelocity; ///< ShaderResource in (motion vectors).
  xiiRenderGraphTextureHandle m_hHistory;  ///< ShaderResource in (history color).
  xiiRenderGraphTextureHandle m_hTAAOut;   ///< UnorderedAccess out (TAA resolved color).
};

void xiiView::SetupTemporalAntiAliasing(xiiTemporalAntiAliasingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hHDRIn    = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity = builder.ReadTexture(xiiRGBlackboardKeys::k_VelocityBuffer, xiiGALResourceStateFlags::ShaderResource);

  if (!m_ViewPassResources.m_TemporalPasses.m_pTAAHistoryBuffer)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = uiRenderWidth;
    description.m_Size.height = uiRenderHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    m_ViewPassResources.m_TemporalPasses.m_pTAAHistoryBuffer = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hHistory = builder.ImportTexture("TAAHistory", m_ViewPassResources.m_TemporalPasses.m_pTAAHistoryBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHistory = builder.ReadTexture(data.m_hHistory, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription taaOutputDescription;
  taaOutputDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  taaOutputDescription.m_Format      = xiiGALResourceFormat::RGBA16Float;
  taaOutputDescription.m_Size.width  = uiRenderWidth;
  taaOutputDescription.m_Size.height = uiRenderHeight;
  taaOutputDescription.m_uiMipLevels = 1U;
  taaOutputDescription.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  taaOutputDescription.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hTAAOut                     = builder.WriteTexture(xiiRGBlackboardKeys::k_TAAResolvedColor, taaOutputDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TemporalPasses.m_pTAAPipeline, "Shaders/Pipeline/TAA.xiiShader");
}

void xiiView::ExecuteTemporalAntiAliasing(const xiiTemporalAntiAliasingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("TAA");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TemporalPasses.m_pTAAPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRCurrent", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_History", context.GetTexture(data.m_hHistory)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TAAOut", context.GetTexture(data.m_hTAAOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Upscale Data //////////
//
// Collects all GPU resources related to temporal upscaling.

struct xiiUpscaleData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTAAIn;    ///< ShaderResource in (TAA resolved color).
  xiiRenderGraphTextureHandle m_hUpscaled; ///< UnorderedAccess out (upscaled HDR color).
};

void xiiView::SetupUpscale(xiiUpscaleData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hTAAIn = builder.ReadTexture(xiiRGBlackboardKeys::k_TAAResolvedColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hUpscaled          = builder.WriteTexture(xiiRGBlackboardKeys::k_UpscaledColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_TemporalPasses.m_pUpscalePipeline, "Shaders/Pipeline/CASUpscale.xiiShader");
}

void xiiView::ExecuteUpscale(const xiiUpscaleData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("Upscale");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_TemporalPasses.m_pUpscalePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_TAAIn", context.GetTexture(data.m_hTAAIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Upscaled", context.GetTexture(data.m_hUpscaled)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Bloom Data //////////
//
// Collects all GPU resources related to bloom generation.

struct xiiBloomData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRIn; ///< ShaderResource in (upscaled HDR input).
  xiiRenderGraphTextureHandle m_hBloom; ///< UnorderedAccess out (bloom result).
};

void xiiView::SetupBloom(xiiBloomData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hHDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_UpscaledColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hBloom             = builder.WriteTexture(xiiRGBlackboardKeys::k_BloomTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_PostProcessPasses.m_pBloomPipeline, "Shaders/Pipeline/BloomChain.xiiShader");
}

void xiiView::ExecuteBloom(const xiiBloomData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("Bloom");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_PostProcessPasses.m_pBloomPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BloomOut", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Color Grading Data //////////
//
// Collects all GPU resources related to color grading.

struct xiiColorGradingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRIn;  ///< ShaderResource in (upscaled HDR input).
  xiiRenderGraphTextureHandle m_hBloom;  ///< ShaderResource in (bloom result).
  xiiRenderGraphTextureHandle m_hGraded; ///< UnorderedAccess out (graded HDR output).
};

void xiiView::SetupColorGrading(xiiColorGradingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hHDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_UpscaledColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBloom = builder.ReadTexture(xiiRGBlackboardKeys::k_BloomTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hGraded            = builder.WriteTexture(xiiRGBlackboardKeys::k_GradedColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_PostProcessPasses.m_pColorGradingPipeline, "Shaders/Pipeline/ColorGrading.xiiShader");
}

void xiiView::ExecuteColorGrading(const xiiColorGradingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("ColorGrading");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_PostProcessPasses.m_pColorGradingPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Bloom", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Graded", context.GetTexture(data.m_hGraded)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Tone Mapping Data //////////
//
// Collects all GPU resources related to tone mapping from HDR to LDR.

struct xiiToneMappingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hGraded; ///< ShaderResource in (graded HDR input).
  xiiRenderGraphTextureHandle m_hLDROut; ///< UnorderedAccess out (tone-mapped LDR output).
};

void xiiView::SetupToneMapping(xiiToneMappingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hGraded = builder.ReadTexture(xiiRGBlackboardKeys::k_GradedColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hLDROut            = builder.WriteTexture(xiiRGBlackboardKeys::k_LDRSceneColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources.m_PostProcessPasses.m_pToneMappingPipeline, "Shaders/Pipeline/ToneMapping.xiiShader");
}

void xiiView::ExecuteToneMapping(const xiiToneMappingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("ToneMapping");
  {
    cmd.SetPipelineState(m_ViewPassResources.m_PostProcessPasses.m_pToneMappingPipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRGraded", context.GetTexture(data.m_hGraded)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_LDROut", context.GetTexture(data.m_hLDROut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Final Blit Data //////////
//
// Collects all GPU resources related to final backbuffer presentation.

struct xiiFinalBlitData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hLDRIn;      ///< ShaderResource in (final LDR scene color).
  xiiRenderGraphTextureHandle m_hBackbuffer; ///< RenderTarget out (swapchain backbuffer).
};

void xiiView::SetupFinalBlit(xiiFinalBlitData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hLDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_LDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  if (const xiiGALSwapChain* pSwapChain = GetSwapChain(); pSwapChain != nullptr)
  {
    xiiSharedPtr<xiiGALTexture> pBackbufferTexture = pSwapChain->GetBackBufferTexture();
    if (pBackbufferTexture)
    {
      data.m_hBackbuffer = builder.ImportTexture("Backbuffer", pBackbufferTexture, xiiGALResourceStateFlags::RenderTarget);
      data.m_hBackbuffer = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
    }
  }

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteFinalBlit(const xiiFinalBlitData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("BackbufferPresent");
  {
    if (data.m_hLDRIn.IsValid() && data.m_hBackbuffer.IsValid() && m_ViewPassResources.m_OutputPasses.m_pFinalBlitPipeline)
    {
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
      cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});
      cmd.SetPipelineState(m_ViewPassResources.m_OutputPasses.m_pFinalBlitPipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_LDRIn", context.GetTexture(data.m_hLDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.Draw({3U, 1U, 0U, 0U});
    }
  }
  cmd.EndDebugGroup();
}

void xiiView::BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // CPU dynamic resolution PID (pre-graph). View owns scale/resolution state.
  RunDynamicResolutionPID();

  xiiUInt32 uiFrameIndex = 0U;
  bool      bResult      = blackboard.TryGet(xiiRGBlackboardKeys::k_FrameIndex, uiFrameIndex);
  XII_IGNORE_UNUSED(bResult);

  if (m_pExtractedData != nullptr)
  {
    m_ViewPassResources.m_LightingSystem.BuildFrameData(*this, *m_pExtractedData, uiFrameIndex);
  }
  m_ViewPassResources.m_LightingSystem.WriteBlackboard(blackboard);

  // Each stage adds its passes to the graph. Dependency ordering is handled by the render graph compiler (topological sort + culling).

  graph.AddPass<xiiLightingDataUploadData>("LightingDataUpload", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLightingDataUpload, this), xiiMakeDelegate(&xiiView::ExecuteLightingDataUpload, this));

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
  graph.AddPass<xiiLocalShadowAtlasAllocationData>("LocalShadowAtlasAllocation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLocalShadowAtlasAllocation, this), xiiMakeDelegate(&xiiView::ExecuteLocalShadowAtlasAllocation, this));
  graph.AddPass<xiiDirectionalShadowData>("DirectionalShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDirectionalShadowData, this), xiiMakeDelegate(&xiiView::ExecuteDirectionalShadowData, this));
  graph.AddPass<xiiSpotShadowData>("SpotShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupSpotShadowData, this), xiiMakeDelegate(&xiiView::ExecuteSpotShadowData, this));
  graph.AddPass<xiiPointShadowData>("PointShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupPointShadowData, this), xiiMakeDelegate(&xiiView::ExecutePointShadowData, this));
  graph.AddPass<xiiRayTracedShadowData>("RayTracedShadowData", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedShadowData, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedShadowData, this));
  graph.AddPass<xiiShadowDenoiseData>("ShadowDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowDenoiseData, this), xiiMakeDelegate(&xiiView::ExecuteShadowDenoiseData, this));
  graph.AddPass<xiiContactShadowData>("ContactShadow", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupContactShadowData, this), xiiMakeDelegate(&xiiView::ExecuteContactShadowData, this));

  // Depth and motion prepasses, which produce depth and motion data consumed by later passes.
  graph.AddPass<xiiDepthPrepassData>("DepthPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDepthPrepass, this), xiiMakeDelegate(&xiiView::ExecuteDepthPrepass, this));
  graph.AddPass<xiiHiZPyramidData>("HiZPyramid", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZPyramid, this), xiiMakeDelegate(&xiiView::ExecuteHiZPyramid, this));
  graph.AddPass<xiiHiZOcclusionCullData>("HiZOcclusionCull", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZOcclusionCull, this), xiiMakeDelegate(&xiiView::ExecuteHiZOcclusionCull, this));
  graph.AddPass<xiiMotionVectorsData>("MotionVectors", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupMotionVectors, this), xiiMakeDelegate(&xiiView::ExecuteMotionVectors, this));
  graph.AddPass<xiiVelocityDilationData>("VelocityDilation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVelocityDilation, this), xiiMakeDelegate(&xiiView::ExecuteVelocityDilation, this));

  // G-Buffer generation passes, which produce material surfaces consumed by lighting stages.
  graph.AddPass<xiiGBufferBaseData>("GBufferBase", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupGBufferBase, this), xiiMakeDelegate(&xiiView::ExecuteGBufferBase, this));
  graph.AddPass<xiiNormalRoughnessPrepassData>("NormalRoughnessPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupNormalRoughnessPrepass, this), xiiMakeDelegate(&xiiView::ExecuteNormalRoughnessPrepass, this));

  // Decals update the G-Buffer before any lighting or screen-space shading consumes it.
  graph.AddPass<xiiDecalUploadData>("DecalUpload", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDecalUpload, this), xiiMakeDelegate(&xiiView::ExecuteDecalUpload, this));
  graph.AddPass<xiiDecalCullBatchData>("DecalCullBatch", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDecalCullBatch, this), xiiMakeDelegate(&xiiView::ExecuteDecalCullBatch, this));
  graph.AddPass<xiiProjectedDecalResolveData>("ProjectedDecalResolve", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupProjectedDecalResolve, this), xiiMakeDelegate(&xiiView::ExecuteProjectedDecalResolve, this));
  graph.AddPass<xiiMeshDecalDrawData>("MeshDecalDraw", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupMeshDecalDraw, this), xiiMakeDelegate(&xiiView::ExecuteMeshDecalDraw, this));

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
  graph.AddPass<xiiEyeShaderData>("EyeShader", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupEyeShader, this), xiiMakeDelegate(&xiiView::ExecuteEyeShader, this));

  // Transparency and special material passes.
  graph.AddPass<xiiGPUParticleSimulateData>("GPUParticleSimulate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGPUParticleSimulate, this), xiiMakeDelegate(&xiiView::ExecuteGPUParticleSimulate, this));
  graph.AddPass<xiiWeightedBlendedOITData>("WeightedBlendedOIT", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupWeightedBlendedOIT, this), xiiMakeDelegate(&xiiView::ExecuteWeightedBlendedOIT, this));

  // Screen-space effects.
  graph.AddPass<xiiScreenSpaceGlobalIlluminationData>("SSGI", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceGlobalIllumination, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceGlobalIllumination, this));
  graph.AddPass<xiiScreenSpaceRefractionData>("SSRefraction", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceRefraction, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceRefraction, this));
  graph.AddPass<xiiPlanarReflectionsData>("PlanarReflections", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupPlanarReflections, this), xiiMakeDelegate(&xiiView::ExecutePlanarReflections, this));

  // Temporal reconstruction passes.
  graph.AddPass<xiiLuminanceHistogramData>("LuminanceHistogram", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLuminanceHistogram, this), xiiMakeDelegate(&xiiView::ExecuteLuminanceHistogram, this));
  graph.AddPass<xiiAutoExposureData>("AutoExposure", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAutoExposure, this), xiiMakeDelegate(&xiiView::ExecuteAutoExposure, this));
  graph.AddPass<xiiTemporalAntiAliasingData>("TAA", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupTemporalAntiAliasing, this), xiiMakeDelegate(&xiiView::ExecuteTemporalAntiAliasing, this));
  graph.AddPass<xiiUpscaleData>("Upscale", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupUpscale, this), xiiMakeDelegate(&xiiView::ExecuteUpscale, this));

  // Post-processing passes.
  graph.AddPass<xiiBloomData>("Bloom", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupBloom, this), xiiMakeDelegate(&xiiView::ExecuteBloom, this));
  graph.AddPass<xiiColorGradingData>("ColorGrading", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupColorGrading, this), xiiMakeDelegate(&xiiView::ExecuteColorGrading, this));
  graph.AddPass<xiiToneMappingData>("ToneMapping", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupToneMapping, this), xiiMakeDelegate(&xiiView::ExecuteToneMapping, this));

  // Debug and visualization passes.
  xiiDebugRenderer::AddRenderGraphPasses(graph);

  // Final output pass.
  graph.AddPass<xiiFinalBlitData>("BackbufferPresent", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupFinalBlit, this), xiiMakeDelegate(&xiiView::ExecuteFinalBlit, this));
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
