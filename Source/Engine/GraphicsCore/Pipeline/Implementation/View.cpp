/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

xiiCVarFloat cvar_DynamicRenderingTargetMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");
xiiCVarFloat cvar_DynamicRenderingMinScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");
xiiCVarFloat cvar_DynamicRenderingMaxScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "Maximum allowed render scale (1.0 = native resolution).");

namespace
{
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 2, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ENUM_ACCESSOR_PROPERTY("RenderPath", xiiRenderPath, GetRenderPath, SetRenderPath),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiBuiltinRenderPipelineRuntime* xiiView::GetBuiltinRuntime() const
{
  auto* runtime = static_cast<xiiBuiltinRenderPipelineRuntime*>(xiiViewRenderResourceManager::GetContext(m_hPipelineRuntime));
  XII_ASSERT_DEV(runtime != nullptr, "View pipeline runtime is unavailable after subsystem shutdown.");
  return runtime;
}

xiiView::xiiView(xiiWorld* pWorld) :
  m_pWorld(pWorld)
{
  m_DisplayOutputSettings = xiiDisplayOutputManager::GetDefaults();
  m_hRenderPipeline       = xiiRenderPipelineResource::GetDefaultPipeline();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No default device available. A view requires a device to initialize its resources.");

  m_hPipelineRuntime = xiiViewRenderResourceManager::CreateContext(*this);
  XII_ASSERT_DEV(!m_hPipelineRuntime.IsInvalidated(), "Failed to create the view render-resource context.");
  XII_VERIFY(m_RenderGraphContext.Initialize("View Render Graph").Succeeded(), "Failed to create the view render graph context.");
  XII_VERIFY(m_GpuSceneContext.Initialize().Succeeded(), "Failed to create the view GPU scene context.");
  m_hGpuVisibilityContext = xiiGpuVisibilityManager::CreateContext();
  XII_ASSERT_DEV(!m_hGpuVisibilityContext.IsInvalidated(), "Failed to create the view GPU visibility context.");
  XII_VERIFY(GetBuiltinRuntime()->m_Resources.m_LightingSystem.Initialize().Succeeded(), "Failed to create the view lighting context.");

  UpdateRenderResolutionState();
}

xiiResult xiiView::SetDisplayOutputSettings(const xiiDisplayOutputSettings& settings)
{
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;

  m_DisplayOutputSettings                                                   = settings;
  GetBuiltinRuntime()->m_Resources.m_TemporalPasses.m_bExposureHistoryValid = false;
  return XII_SUCCESS;
}

const xiiDisplayOutputSettings& xiiView::GetDisplayOutputSettings() const
{
  return m_DisplayOutputSettings;
}

void xiiView::InvalidateTemporalHistory()
{
  xiiGpuVisibilityManager::InvalidateHiZHistory(m_hGpuVisibilityContext);
  GetBuiltinRuntime()->InvalidateTemporalHistory();
  GetBuiltinRuntime()->m_Instance.InvalidateTemporalHistory();
}

xiiView::~xiiView()
{
  m_InternalId.Invalidate();

  // High-level subsystem shutdown may have invalidated the handle before a late world teardown.
  // In that case all persistent resources (including the lighting context) are already released.
  if (auto* pResources = static_cast<xiiBuiltinRenderPipelineRuntime*>(xiiViewRenderResourceManager::GetContext(m_hPipelineRuntime)))
  {
    pResources->m_Resources.m_LightingSystem.Shutdown();
  }
  xiiGpuVisibilityManager::DestroyContext(m_hGpuVisibilityContext);
  m_hGpuVisibilityContext = {};
  m_GpuSceneContext.Shutdown();
  m_RenderGraphContext.Shutdown();
  xiiViewRenderResourceManager::DestroyContext(m_hPipelineRuntime);
  m_hPipelineRuntime = {};
}

bool xiiView::SynchronizeGpuScene(xiiUInt64 uiFrameIndex)
{
  if (m_pExtractedData == nullptr || !m_GpuSceneContext.IsInitialized() || !xiiGpuVisibilityManager::IsValid(m_hGpuVisibilityContext))
    return false;

  return m_GpuSceneContext.SynchronizeExtractedMeshes(*m_pExtractedData, uiFrameIndex).Succeeded();
}

xiiResult xiiView::SetSensorProfile(xiiSensorProfileHandle hProfile)
{
  if (!hProfile.IsInvalidated())
  {
    xiiSensorProfile profile;
    if (xiiSensorRenderingManager::GetProfile(hProfile, profile).Failed())
      return XII_FAILURE;
  }

  auto& outputPasses = GetBuiltinRuntime()->m_Resources.m_OutputPasses;
  if (outputPasses.m_hSensorProfile == hProfile)
    return XII_SUCCESS;

  outputPasses.m_hSensorProfile = hProfile;
  if (outputPasses.m_pSensorOutputTexture != nullptr)
  {
    outputPasses.m_RetiredSensorOutputTextures.PushBack(std::move(outputPasses.m_pSensorOutputTexture));
  }
  return XII_SUCCESS;
}

xiiSensorProfileHandle xiiView::GetSensorProfile() const
{
  return GetBuiltinRuntime()->m_Resources.m_OutputPasses.m_hSensorProfile;
}

xiiSharedPtr<xiiGALTexture> xiiView::GetSensorOutputTexture() const
{
  return GetBuiltinRuntime()->m_Resources.m_OutputPasses.m_pSensorOutputTexture;
}

void xiiView::SetRenderScale(float fRenderScale)
{
  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fRenderScale = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fRenderScale, fDynamicMin, fDynamicMax);

  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale, fDynamicMin, fDynamicMax);
  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  UpdateRenderResolutionState();
}

float xiiView::GetRenderScale() const
{
  return GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fRenderScale;
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

void xiiView::ComputeCullingFrustum(xiiFrustum& out_frustum) const
{
  const xiiCamera* pCullingCamera = GetCullingCamera();
  if (pCullingCamera == nullptr)
  {
    out_frustum = xiiFrustum::MakeInvalid();
    return;
  }

  const xiiRectFloat& viewport     = GetViewport();
  const float         fAspectRatio = viewport.HasNonZeroArea() ? viewport.width / viewport.height : 1.0f;

  xiiMat4 projectionMatrix;
  pCullingCamera->GetProjectionMatrix(fAspectRatio, projectionMatrix, xiiCameraEye::Left);
  out_frustum = xiiFrustum::MakeFromMVP(projectionMatrix * pCullingCamera->GetViewMatrix(xiiCameraEye::Left));
}

void xiiView::UpdateRenderResolutionState() const
{
  const float fRenderScale = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fDynamicScale = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

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
  float fGpuTimeMs = m_RenderGraphContext.GetProfiler().GetFrameDurationMs();
  if (fGpuTimeMs <= 0.0f)
  {
    fGpuTimeMs = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;
  }

  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fLastGpuFrameTimeMs = fGpuTimeMs;

  // PID controller.
  const float fRenderScale = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);
  float       fDynamicMin  = 0.1f;
  float       fDynamicMax  = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fTarget    = xiiMath::Max(cvar_DynamicRenderingTargetMs.GetValue(), 0.1f);
  const float fDeltaTime = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fErrorIntegral = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fErrorIntegral + fError * fDeltaTime, -1.0f, 1.0f);

  const float fDerivative = (fError - GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fPreviousError) / xiiMath::Max(fDeltaTime, 0.001f);
  const float fPID        = 0.35f * fError + 0.05f * GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fErrorIntegral + 0.15f * fDerivative;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale + fPID, fDynamicMin, fDynamicMax);
  const float fDelta   = xiiMath::Clamp(fDesired - GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale, -0.10f, 0.10f);

  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale + fDelta, fDynamicMin, fDynamicMax);
  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(xiiMath::Lerp(GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fSmoothedScale, GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fCurrentScale, 0.20f), fDynamicMin, fDynamicMax);
  GetBuiltinRuntime()->m_Resources.m_DynamicResolution.m_fPreviousError = fError;

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


void xiiView::SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> hint)
{
  m_Data.m_CameraUsageHint = hint;

  if (GetBuiltinRuntime()->m_Resources.m_OutputPasses.m_hSensorProfile.IsInvalidated())
  {
    xiiSensorType::Enum sensorType = xiiSensorType::ENUM_COUNT;
    switch (hint.GetValue())
    {
      case xiiCameraUsageHint::SensorRGB:
        sensorType = xiiSensorType::RGBCamera;
        break;
      case xiiCameraUsageHint::SensorInfrared:
        sensorType = xiiSensorType::InfraredCamera;
        break;
      case xiiCameraUsageHint::SensorDepth:
        sensorType = xiiSensorType::DepthCamera;
        break;
      case xiiCameraUsageHint::SensorLiDAR:
        sensorType = xiiSensorType::LiDAR;
        break;
      default:
        break;
    }

    if (sensorType != xiiSensorType::ENUM_COUNT)
      GetBuiltinRuntime()->m_Resources.m_OutputPasses.m_hSensorProfile = xiiSensorRenderingManager::GetDefaultProfileHandle(sensorType);
  }
}

void xiiView::SetRenderPipelineResource(const xiiRenderPipelineResourceHandle& pipeline)
{
  if (m_hRenderPipeline == pipeline)
    return;
  m_hRenderPipeline = pipeline;
  InvalidateTemporalHistory();
}

xiiResult xiiView::BuildRenderPipeline(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVirtualShadowMapManager::UploadHandles& shadows)
{
  auto* runtime = GetBuiltinRuntime();
  bool  changed = false;
  if (runtime->m_Instance.Synchronize(m_hRenderPipeline, &changed).Failed())
    return XII_FAILURE;
  if (changed)
    InvalidateTemporalHistory();
  RunDynamicResolutionPID();
  runtime->BeginFrame(shadows);
  return runtime->m_Instance.Build(this, graph, blackboard, m_PipelineParameters);
}

void xiiView::SetPipelineParameter(xiiStringView name, const xiiVariant& value)
{
  const auto* previous = m_PipelineParameters.GetValue(name);
  if (previous && *previous == value)
    return;
  m_PipelineParameters.Insert(name, value);
  InvalidateTemporalHistory();
}
void xiiView::RemovePipelineParameter(xiiStringView name)
{
  if (m_PipelineParameters.Remove(name))
    InvalidateTemporalHistory();
}
