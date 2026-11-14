/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Components/Render/RenderWorldSettingsComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

#include <GraphicsCore/Debug/DebugRenderSettings.h>
#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>
#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorCapture.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>


namespace
{
  static xiiUInt32 ComputeDynamicResolutionDimension(xiiUInt32 uiNativeDimension, float fScale)
  {
    const xiiUInt32 uiScaled = static_cast<xiiUInt32>(xiiMath::Ceil(static_cast<float>(uiNativeDimension) * fScale));
    return xiiMath::Max(uiScaled, 1U);
  }
  static void ComputeDynamicScaleBounds(const xiiDynamicResolutionSettings& settings, float fRenderScale, float& out_fDynamicMin, float& out_fDynamicMax)
  {
    const float fFinalMinScale = xiiMath::Max(settings.m_fMinimumRenderScale, 0.25f);
    const float fFinalMaxScale = xiiMath::Min(settings.m_fMaximumRenderScale, 1.0f);
    const float fBaseScale     = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);

    out_fDynamicMin = xiiMath::Clamp(fFinalMinScale / fBaseScale, 0.1f, 1.0f);
    out_fDynamicMax = xiiMath::Clamp(fFinalMaxScale / fBaseScale, 0.1f, 1.0f);

    if (!settings.m_bEnabled)
      out_fDynamicMin = out_fDynamicMax = 1.0f;
    if (out_fDynamicMin > out_fDynamicMax)
    {
      out_fDynamicMin = out_fDynamicMax;
    }
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 4, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("ShowFPS", GetShowFrameStats, SetShowFrameStats)->AddAttributes(new xiiDefaultValueAttribute(false)),
      XII_ENUM_ACCESSOR_PROPERTY("RenderPath", xiiRenderPath, GetRenderPath, SetRenderPath),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiView::SetRenderTargetView(xiiGALTextureView* pRenderTargetView)
{
  m_pRenderTargetView = pRenderTargetView == nullptr ? xiiSharedPtr<xiiGALTextureView>() :
                                                       xiiSharedPtr<xiiGALTextureView>(pRenderTargetView, pRenderTargetView->GetDevice()->GetAllocator());
}

xiiRenderPipelineInstance* xiiView::GetPipelineInstance() const
{
  auto* runtime = xiiViewRenderResourceManager::GetContext(m_hPipelineRuntime);
  XII_ASSERT_DEV(runtime != nullptr, "View pipeline runtime is unavailable after subsystem shutdown.");
  return runtime;
}

const xiiRenderPipelineInstance& xiiView::GetRenderPipelineInstance() const
{
  return (*GetPipelineInstance());
}

xiiView::xiiView(xiiWorld* pWorld) :
  m_pWorld(pWorld)
{
  m_DisplayOutputSettings = xiiDisplayOutputManager::GetDefaults();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No default device available. A view requires a device to initialize its resources.");

  m_hPipelineRuntime = xiiViewRenderResourceManager::CreateContext();
  XII_ASSERT_DEV(!m_hPipelineRuntime.IsInvalidated(), "Failed to create the view render-resource context.");
  m_hRenderGraph = xiiRenderGraphManager::CreateGraph("View Render Graph", xiiMakeDelegate(&xiiView::BuildRenderGraph, this));
  XII_ASSERT_DEV(!m_hRenderGraph.IsInvalidated(), "Failed to create the view render graph.");

  UpdateRenderResolutionState();
}

xiiResult xiiView::BuildRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiView* pView)
{
  XII_ASSERT_DEV(pView == this, "A view must execute its own render graph.");
  XII_IGNORE_UNUSED(pView);
  blackboard.Set(xiiRGBlackboardKeys::k_FrameIndex, static_cast<xiiUInt32>(graph.GetFrameIndex()));

  if (m_RenderGraphBuilder.IsValid())
  {
    m_RenderGraphBuilder(*this, graph, blackboard);
  }
  else if (BuildRenderPipeline(graph, blackboard).Failed())
  {
    xiiLog::Error("Failed to build render pipeline for view '{}': {}", GetName(), GetRenderPipelineInstance().GetLastBuildError());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiView::SetDisplayOutputSettings(const xiiDisplayOutputSettings& settings)
{
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;

  auto  candidate = m_RenderConfiguration;
  auto* post      = candidate.EditSubsystem<xiiPostProcessRenderSettings>();
  post->m_Display = settings;
  xiiDynamicArray<xiiRenderSettingProperty> properties;
  post->GetProperties(properties);
  for (const auto& property : properties)
    if (!post->IsOverridden(property.m_sPath))
      post->m_Overrides.PushBack(property.m_sPath);
  XII_SUCCEED_OR_RETURN(SetRenderConfiguration(candidate));
  if (!xiiReflectionUtils::IsEqual(&settings, &m_DisplayOutputSettings, xiiGetStaticRTTI<xiiDisplayOutputSettings>()))
  {
    m_DisplayOutputSettings = settings;
    ++m_uiDisplayOutputSettingsRevision;
  }
  return XII_SUCCESS;
}

const xiiDisplayOutputSettings& xiiView::GetDisplayOutputSettings() const
{
  return m_DisplayOutputSettings;
}

void xiiView::InvalidateTemporalHistory()
{
  GetPipelineInstance()->InvalidateTemporalHistory();
}

xiiView::~xiiView()
{
  m_InternalId.Invalidate();

  xiiRenderGraphManager::DestroyGraph(m_hRenderGraph).AssertSuccess("Failed to destroy the view render graph.");
  xiiViewRenderResourceManager::DestroyContext(m_hPipelineRuntime);
  m_hPipelineRuntime = {};
}

const xiiGpuInstanceData* xiiView::GetGpuInstanceData() const
{
  const auto* module = m_pWorld ? static_cast<const xiiWorld*>(m_pWorld)->GetModule<xiiRenderWorldModule>() : nullptr;
  return module ? module->GetGpuInstanceData() : nullptr;
}

xiiResult xiiView::SetSensorProfile(xiiSensorProfileHandle hProfile)
{
  if (!hProfile.IsInvalidated())
  {
    xiiSensorProfile profile;
    if (xiiSensorRenderingManager::GetProfile(hProfile, profile).Failed())
      return XII_FAILURE;
  }

  m_hSensorProfile = hProfile;
  return XII_SUCCESS;
}

xiiSensorProfileHandle xiiView::GetSensorProfile() const
{
  return m_hSensorProfile;
}

xiiSharedPtr<xiiGALTexture> xiiView::GetSensorOutputTexture() const
{
  const auto& instance = GetRenderPipelineInstance();
  if (const auto* pipeline = instance.GetPipeline())
    for (xiiUInt32 index = 0; index < pipeline->GetDescriptor().m_Passes.GetCount(); ++index)
      if (const auto* sensor = xiiDynamicCast<const xiiSensorCapture*>(instance.GetPass(index)))
        if (auto output = sensor->GetViewOutput(m_hSensorProfile))
          return output;
  return {};
}

void xiiView::SetRenderScale(float fRenderScale)
{
  if (!xiiMath::IsFinite(fRenderScale))
    return;
  const float scale = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);
  if (m_RenderConfiguration.SetOverride<xiiDynamicResolutionSettings>("RenderScale", scale).Succeeded())
    ApplyRenderScale(scale);
}

void xiiView::ApplyRenderScale(float fRenderScale)
{
  if (m_DynamicResolution.m_fRenderScale == fRenderScale)
    return;
  m_DynamicResolution.m_fRenderScale = fRenderScale;

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(GetDynamicResolutionSettings(), m_DynamicResolution.m_fRenderScale, fDynamicMin, fDynamicMax);

  m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_DynamicResolution.m_fCurrentScale, fDynamicMin, fDynamicMax);
  m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  UpdateRenderResolutionState();
}

void xiiView::UpdateFrameStats()
{
  if (!m_bShowFrameStats)
  {
    m_LastStatsFrame    = {};
    m_StatsInterval     = {};
    m_uiStatsFrames     = 0;
    m_fDisplayedFrameMs = 0.0;
    return;
  }
  const auto now = xiiTime::Now();
  if (m_LastStatsFrame.IsPositive())
  {
    m_StatsInterval += now - m_LastStatsFrame;
    ++m_uiStatsFrames;
    if (m_StatsInterval.GetSeconds() >= 0.5)
    {
      m_fDisplayedFrameMs = m_StatsInterval.GetMilliseconds() / m_uiStatsFrames;
      m_StatsInterval     = {};
      m_uiStatsFrames     = 0;
    }
  }
  m_LastStatsFrame = now;
  if (m_fDisplayedFrameMs > 0.0)
    xiiDebugRenderer::DrawInfoText(*this, xiiDebugTextPlacement::BottomLeft, "FPS",
                                   xiiFmt("{} FPS, {} ms | {} x {}", xiiArgF(1000.0 / m_fDisplayedFrameMs, 1), xiiArgF(m_fDisplayedFrameMs, 2), GetRenderResolutionWidth(), GetRenderResolutionHeight()), xiiColor::LightGreen);
}

float xiiView::GetRenderScale() const
{
  return m_DynamicResolution.m_fRenderScale;
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


    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& projection = m_Data.m_ProjectionMatrix[i];
      m_pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, projection, static_cast<xiiCameraEye>(i), xiiClipSpaceDepthRange::ZeroToOne);
      // Cameras retain their general-purpose near=0 / far=1 convention. Native rendering uses
      // reversed Z (near=1 / far=0), including supplied asymmetric stereo projections.
      projection.SetRow(2, projection.GetRow(3) - projection.GetRow(2));
      m_Data.m_InverseProjectionMatrix[i] = projection.GetInverse(0.0f);
    }
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
  if (!xiiMath::IsFinite(m_DynamicResolution.m_fRenderScale) || !xiiMath::IsFinite(m_DynamicResolution.m_fSmoothedScale))
  {
    m_Data.m_fRenderResolutionScale  = 0;
    m_Data.m_uiRenderResolutionWidth = m_Data.m_uiRenderResolutionHeight = 0;
    return;
  }
  const float fRenderScale = xiiMath::Clamp(m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(GetDynamicResolutionSettings(), fRenderScale, fDynamicMin, fDynamicMax);

  const float fDynamicScale = xiiMath::Clamp(m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  m_Data.m_fRenderResolutionScale = xiiMath::Clamp(fDynamicScale * fRenderScale, 0.1f, 1.0f);
  if (!xiiMath::IsFinite(m_Data.m_fRenderResolutionScale))
  {
    m_Data.m_uiRenderResolutionWidth = m_Data.m_uiRenderResolutionHeight = 0;
    return;
  }

  if (!xiiMath::IsFinite(m_Data.m_ViewPortRect.width) || !xiiMath::IsFinite(m_Data.m_ViewPortRect.height) ||
      m_Data.m_ViewPortRect.width <= 0 || m_Data.m_ViewPortRect.height <= 0 || m_Data.m_ViewPortRect.width > 65536 || m_Data.m_ViewPortRect.height > 65536)
  {
    m_Data.m_uiRenderResolutionWidth = m_Data.m_uiRenderResolutionHeight = 0;
    return;
  }
  const xiiUInt32 uiNativeWidth  = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.width));
  const xiiUInt32 uiNativeHeight = static_cast<xiiUInt32>(xiiMath::Max(1.0f, m_Data.m_ViewPortRect.height));

  m_Data.m_uiRenderResolutionWidth  = ComputeDynamicResolutionDimension(uiNativeWidth, m_Data.m_fRenderResolutionScale);
  m_Data.m_uiRenderResolutionHeight = ComputeDynamicResolutionDimension(uiNativeHeight, m_Data.m_fRenderResolutionScale);
}

void xiiView::RunDynamicResolutionPID()
{
  const auto& settings = GetDynamicResolutionSettings();
  if (!settings.m_bEnabled)
    return;
  // Unavailable GPU timing must not turn CPU loading stalls into a resolution change.
  float fGpuTimeMs = GetProfiler().GetGraphDurationMs(GetRenderGraph()->GetId().m_Data);
  if (fGpuTimeMs <= 0.0f)
  {
    return;
  }


  // PID controller.
  const float fRenderScale = xiiMath::Clamp(m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);
  float       fDynamicMin  = 0.1f;
  float       fDynamicMax  = 1.0f;
  ComputeDynamicScaleBounds(GetDynamicResolutionSettings(), fRenderScale, fDynamicMin, fDynamicMax);

  const float fTarget    = xiiMath::Max(settings.m_fTargetFrameTimeMs, 0.1f);
  const float fDeltaTime = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  m_DynamicResolution.m_fErrorIntegral = xiiMath::Clamp(m_DynamicResolution.m_fErrorIntegral + fError * fDeltaTime, -settings.m_fIntegralLimit, settings.m_fIntegralLimit);

  const float fDerivative = (fError - m_DynamicResolution.m_fPreviousError) / xiiMath::Max(fDeltaTime, 0.001f);
  const float fPID        = settings.m_fProportionalGain * fError + settings.m_fIntegralGain * m_DynamicResolution.m_fErrorIntegral + settings.m_fDerivativeGain * fDerivative;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(m_DynamicResolution.m_fCurrentScale + fPID, fDynamicMin, fDynamicMax);
  const float fDelta   = xiiMath::Clamp(fDesired - m_DynamicResolution.m_fCurrentScale, -settings.m_fMaximumScaleStep, settings.m_fMaximumScaleStep);

  m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_DynamicResolution.m_fCurrentScale + fDelta, fDynamicMin, fDynamicMax);
  m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(xiiMath::Lerp(m_DynamicResolution.m_fSmoothedScale, m_DynamicResolution.m_fCurrentScale, settings.m_fSmoothing), fDynamicMin, fDynamicMax);
  m_DynamicResolution.m_fPreviousError = fError;

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

void xiiView::SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> hint)
{
  m_Data.m_CameraUsageHint = hint;

  if (m_hSensorProfile.IsInvalidated())
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
      m_hSensorProfile = xiiSensorRenderingManager::GetDefaultProfileHandle(sensorType);
  }
}

void xiiView::SetRenderPipelineResource(const xiiRenderPipelineResourceHandle& pipeline)
{
  if (m_hRenderPipeline == pipeline)
    return;
  m_hRenderPipeline                = pipeline;
  m_bPipelineSynchronizationFailed = false;
  InvalidateTemporalHistory();
}

xiiResult xiiView::SynchronizeRenderPipeline()
{
  auto* runtime = GetPipelineInstance();
  if (m_RenderGraphBuilder.IsValid())
  {
    m_bPipelineSynchronizationFailed = false;
    m_ExtractionRequirements         = m_CustomExtractionRequirements;
    m_RuntimeFeatures                = m_CustomRuntimeFeatures;
    return XII_SUCCESS;
  }
  const auto& configuration = ResolveRenderConfiguration();
  bool        changed       = false;
  auto        outputFormat  = xiiGALResourceFormat::Unknown;
  if (m_pRenderTargetView)
    outputFormat = static_cast<xiiGALResourceFormat::Enum>(m_pRenderTargetView->GetDescription().m_Format.GetValue());
  else if (m_pSwapChain)
    outputFormat = static_cast<xiiGALResourceFormat::Enum>(m_pSwapChain->GetDescription().m_ColorBufferFormat.GetValue());
  if (runtime->Synchronize(m_hRenderPipeline, &changed, configuration, outputFormat).Failed())
  {
    if (!m_bPipelineSynchronizationFailed)
    {
      if (m_hRenderPipeline.IsValid())
        xiiLog::Error("View '{}' cannot load or compile render pipeline resource '{}'.", GetName(), m_hRenderPipeline.GetResourceID());
      else
        xiiLog::Error("View '{}' has no selected render pipeline resource.", GetName());
    }
    m_bPipelineSynchronizationFailed = true;
    m_ExtractionRequirements         = {};
    m_RuntimeFeatures                = {};
    InvalidateTemporalHistory();
    return XII_FAILURE;
  }
  m_bPipelineSynchronizationFailed = false;
  ApplyRenderScale(GetDynamicResolutionSettings().m_fRenderScale);
  if (const auto* post = runtime->GetRenderConfiguration().FindSubsystem<xiiPostProcessRenderSettings>())
    if (post->m_Display != m_DisplayOutputSettings)
    {
      m_DisplayOutputSettings = post->m_Display;
      ++m_uiDisplayOutputSettingsRevision;
    }
  if (changed)
  {
    InvalidateTemporalHistory();
  }
  if (const auto* debug = runtime->GetRenderConfiguration().FindSubsystem<xiiDebugRenderSettings>())
  {
    if (debug->IsOverridden("Visualization"))
      SetViewRenderMode(debug->m_Visualization);
    if (debug->IsOverridden("FrameStatistics"))
      SetShowFrameStats(debug->m_bFrameStatistics);
  }
  m_ExtractionRequirements = runtime->GetPipeline()->GetExtractionRequirements();
  m_RuntimeFeatures        = runtime->GetPipeline()->GetRuntimeFeatures();
  return XII_SUCCESS;
}

xiiResult xiiView::SetRenderConfiguration(const xiiRenderConfiguration& configuration)
{
  if (m_RenderConfiguration == configuration)
    return XII_SUCCESS;
  if (!configuration.IsValid())
    return XII_FAILURE;
  m_RenderConfiguration = configuration;
  return XII_SUCCESS;
}

const xiiRenderSubsystemSettings* xiiView::GetRenderSubsystemSettings(const xiiRTTI* type) const
{
  return GetPipelineInstance()->GetRenderConfiguration().FindSubsystem(type);
}

const xiiDynamicResolutionSettings& xiiView::GetDynamicResolutionSettings() const
{
  if (const auto* settings = GetRenderSettings<xiiDynamicResolutionSettings>())
    return *settings;
  if (const auto* settings = m_RenderConfiguration.FindSubsystem<xiiDynamicResolutionSettings>())
    return *settings;
  static const xiiDynamicResolutionSettings defaults;
  return defaults;
}

xiiRenderConfiguration xiiView::GetEffectiveRenderConfiguration() const
{
  return GetPipelineInstance()->GetRenderConfiguration();
}

const xiiRenderConfiguration& xiiView::ResolveRenderConfiguration()
{
  const xiiRenderConfiguration          empty;
  const xiiRenderConfiguration*         scopes[] = {&empty, &empty, &empty, &m_RenderConfiguration};
  xiiSharedPtr<const xiiRenderPipeline> selected;
  if (m_hRenderPipeline.IsValid())
  {
    xiiResourceLock<xiiRenderPipelineResource> resource(m_hRenderPipeline, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (resource.IsValid())
      selected = resource->GetPipeline();
  }
  if (const auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance())
    if (const auto* pProfile = pApplication->GetPlatformProfile().GetTypeConfig<xiiRenderPipelineProfileConfig>())
      scopes[0] = &pProfile->m_RenderConfiguration;


  if (const auto* pWorld = GetWorld())
  {
    if (const auto* pManager = pWorld->GetComponentManager<xiiRenderWorldSettingsComponentManager>())
      if (const auto* pSettings = pManager->GetSingletonComponent())
        scopes[1] = &pSettings->m_RenderConfiguration;

    const xiiCameraComponent* pCamera = nullptr;
    if (!m_hRenderConfigurationCamera.IsInvalidated())
    {
      if (!pWorld->TryGetComponent(m_hRenderConfigurationCamera, pCamera) || !pCamera->IsActiveAndInitialized())
        pCamera = nullptr;
    }
    else if (GetCameraUsageHint() != xiiCameraUsageHint::None && GetCameraUsageHint() != xiiCameraUsageHint::EditorView && GetCameraUsageHint() != xiiCameraUsageHint::RenderTarget)
    {
      if (const auto* pManager = pWorld->GetComponentManager<xiiCameraComponentManager>())
        pCamera = pManager->GetCameraByUsageHint(GetCameraUsageHint());
    }
    if (pCamera)
      scopes[2] = &pCamera->m_RenderConfiguration;
  }

  bool changed = !m_bRenderConfigurationResolved || selected != m_pResolvedRenderPipeline;
  for (xiiUInt32 i = 0; !changed && i < XII_ARRAY_SIZE(scopes); ++i)
    changed = *scopes[i] != m_ResolvedScopeInputs[i];
  if (!changed)
    return m_ResolvedRenderConfiguration;

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(scopes); ++i)
    m_ResolvedScopeInputs[i] = *scopes[i];
  m_pResolvedRenderPipeline = selected;
  auto effective            = selected ? selected->GetSubsystemDefaults() : xiiRenderConfiguration{};
  effective.ApplyOverrides(*scopes[0], xiiRenderSettingScope::Project);
  if (selected)
    effective.ApplyOverrides(selected->GetDescriptor().m_Settings.m_RenderDefaults, xiiRenderSettingScope::Pipeline);
  effective.ApplyOverrides(*scopes[1], xiiRenderSettingScope::Scene);
  effective.ApplyOverrides(*scopes[2], xiiRenderSettingScope::Camera);
  effective.ApplyOverrides(*scopes[3], xiiRenderSettingScope::Viewport);
  m_ResolvedRenderConfiguration  = std::move(effective);
  m_bRenderConfigurationResolved = true;
  return m_ResolvedRenderConfiguration;
}

xiiResult xiiView::BuildRenderPipeline(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto* runtime = GetPipelineInstance();
  RunDynamicResolutionPID();
  xiiHybridArray<xiiRenderPipelineInputBinding, 1> inputs;
  if (const auto* pipeline = runtime->GetPipeline())
  {
    // Only this explicit root interface has view-owned semantics. Other root inputs
    // remain available to callers of the ordinary pipeline instance API.
    const auto targetInterface = pipeline->FindInterface("ViewTarget", true);
    if (targetInterface != xiiInvalidIndex)
    {
      const auto target = m_pRenderTargetView ? m_pRenderTargetView->GetTexture() : m_pSwapChain ? m_pSwapChain->GetBackBufferTexture() :
                                                                                                   xiiSharedPtr<xiiGALTexture>();
      if (target)
      {
        const auto imported = graph.AddPass<xiiRenderGraphTextureHandle>("View target interface", xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& handle, xiiRenderGraphBuilder& builder) { handle = builder.ImportTexture("ViewTarget", target, target->GetResourceState()); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {});
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, targetInterface, *imported.first));
      }
    }
  }
  const xiiResult result = runtime->Build(this, graph, blackboard, m_PipelineParameters, inputs);
  if (result.Failed())
    InvalidateTemporalHistory();
  return result;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_View);
