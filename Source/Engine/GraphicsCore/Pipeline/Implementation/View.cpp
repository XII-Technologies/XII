/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Components/Fog/VolumetricCloudComponent.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Render/DecalComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
#include <Shaders/Pipeline/Passes/Atmosphere/CloudShadowConstants.h>
#include <Shaders/Pipeline/Passes/Denoising/TemporalDenoiseConstants.h>
#include <Shaders/Pipeline/Passes/Exposure/ExposureAdaptationConstants.h>
#include <Shaders/Pipeline/Passes/Exposure/ExposureHistogramConstants.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/ReSTIRGIConstants.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/SSGIConstants.h>
#include <Shaders/Pipeline/Passes/HiZPyramid/HiZBuildConstants.h>
#include <Shaders/Pipeline/Passes/HiZPyramid/HiZOcclusionConstants.h>
#include <Shaders/Pipeline/Passes/LightClustering/LightClusteringConstants.h>
#include <Shaders/Pipeline/Passes/MotionVectors/MotionVectorConstants.h>
#include <Shaders/Pipeline/Passes/Output/BloomConstants.h>
#include <Shaders/Pipeline/Passes/Output/ColorGradingConstants.h>
#include <Shaders/Pipeline/Passes/Output/FinalBlitConstants.h>
#include <Shaders/Pipeline/Passes/Output/ToneMappingConstants.h>
#include <Shaders/Pipeline/Passes/ReSTIR/ReSTIRDIConstants.h>
#include <Shaders/Pipeline/Passes/Reflections/SSRConstants.h>
#include <Shaders/Pipeline/Passes/Refraction/SSRefractionConstants.h>
#include <Shaders/Pipeline/Passes/Sensors/SensorOutputConstants.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/Temporal/TAAConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/DrawCommandBuildConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/FrustumCullingConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/InstanceUpdateConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/LODSelectionConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/ShadowCasterCullingConstants.h>
#include <Shaders/Pipeline/Passes/Volumetrics/VolumetricMediumConstants.h>
#include <Shaders/Pipeline/Passes/Volumetrics/VolumetricTemporalConstants.h>
#include <Shaders/Pipeline/ReflectionProbeData.h>

xiiCVarFloat cvar_DynamicRenderingTargetMs("Rendering.DynamicResolution.TargetFrameTimeMs", 16.0f, xiiCVarFlags::Default, "Target GPU frame time in milliseconds. The CPU PID controller drives render scale to meet this.");
xiiCVarFloat cvar_DynamicRenderingMinScale("Rendering.DynamicResolution.MinimumRenderScale", 0.5f, xiiCVarFlags::Default, "Minimum allowed render scale (0.5 = 50% of native resolution in each direction).");
xiiCVarFloat cvar_DynamicRenderingMaxScale("Rendering.DynamicResolution.MaximumRenderScale", 1.0f, xiiCVarFlags::Default, "Maximum allowed render scale (1.0 = native resolution).");

namespace
{
  // Shared constants (sizes of persistent GPU buffers, aligned to typical instance budgets)
  static constexpr xiiUInt32 k_uiMaxInstances = 65536U; ///< The maximum number of drawable objects in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.

  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasWidth  = 4096U; ///< The width of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The height will be the same as the width, and each cascade will be allocated a quadrant of the atlas.
  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasHeight = 4096U; ///< The height of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The width will be the same as the height, and each cascade will be allocated a quadrant of the atlas.

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

  static xiiUInt32 GetDrawCommandCapacity(const xiiRenderGraphBlackboard& blackboard)
  {
    xiiUInt32  uiDrawCommandCapacity   = 0U;
    const bool bHasDrawCommandCapacity = blackboard.TryGet(xiiRGBlackboardKeys::k_DrawCommandCapacity, uiDrawCommandCapacity);
    XII_IGNORE_UNUSED(bHasDrawCommandCapacity);
    return uiDrawCommandCapacity;
  }

  static xiiUInt32 GetShadowCommandCapacity(const xiiRenderGraphBlackboard& blackboard)
  {
    xiiUInt32  uiShadowCommandCapacity   = 0U;
    const bool bHasShadowCommandCapacity = blackboard.TryGet(xiiRGBlackboardKeys::k_ShadowCommandCapacity, uiShadowCommandCapacity);
    XII_IGNORE_UNUSED(bHasShadowCommandCapacity);
    return uiShadowCommandCapacity;
  }

  static const xiiDirectionalLightRenderData* SelectMainDirectionalLight(const xiiArrayPtr<xiiRenderData* const>& renderData)
  {
    const xiiDirectionalLightRenderData* pBest = nullptr;
    for (const xiiRenderData* pRenderData : renderData)
    {
      const xiiDirectionalLightRenderData* pDirectional = xiiDynamicCast<const xiiDirectionalLightRenderData*>(pRenderData);
      if (pDirectional == nullptr)
        continue;

      if (pBest == nullptr || pDirectional->m_fPhotometricIntensity > pBest->m_fPhotometricIntensity ||
          (pDirectional->m_fPhotometricIntensity == pBest->m_fPhotometricIntensity && pDirectional->m_uiSortingKey < pBest->m_uiSortingKey))
      {
        pBest = pDirectional;
      }
    }
    return pBest;
  }

  static const xiiSkyAtmosphereRenderData* SelectSkyAtmosphere(const xiiArrayPtr<xiiRenderData* const>& renderData)
  {
    const xiiSkyAtmosphereRenderData* pBest = nullptr;
    for (const xiiRenderData* pRenderData : renderData)
    {
      const xiiSkyAtmosphereRenderData* pAtmosphere = xiiDynamicCast<const xiiSkyAtmosphereRenderData*>(pRenderData);
      if (pAtmosphere == nullptr)
        continue;

      if (pBest == nullptr || pAtmosphere->m_iPriority > pBest->m_iPriority ||
          (pAtmosphere->m_iPriority == pBest->m_iPriority && pAtmosphere->m_uiSortingKey < pBest->m_uiSortingKey))
      {
        pBest = pAtmosphere;
      }
    }
    return pBest;
  }

  static const xiiVolumetricCloudRenderData* SelectVolumetricCloudLayer(const xiiArrayPtr<xiiRenderData* const>& renderData)
  {
    const xiiVolumetricCloudRenderData* pBest = nullptr;
    for (const xiiRenderData* pRenderData : renderData)
    {
      const xiiVolumetricCloudRenderData* pCloud = xiiDynamicCast<const xiiVolumetricCloudRenderData*>(pRenderData);
      if (pCloud == nullptr)
        continue;

      if (pBest == nullptr || pCloud->m_iPriority > pBest->m_iPriority ||
          (pCloud->m_iPriority == pBest->m_iPriority && pCloud->m_uiSortingKey < pBest->m_uiSortingKey))
      {
        pBest = pCloud;
      }
    }
    return pBest;
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

  static bool EnsureTemporalHistoryTextures(xiiSharedPtr<xiiGALTexture> (&textures)[2], xiiGALResourceFormat::Enum format, xiiUInt32 uiWidth, xiiUInt32 uiHeight)
  {
    auto MatchesResolution = [uiWidth, uiHeight](const xiiSharedPtr<xiiGALTexture>& pTexture) {
      return pTexture != nullptr && pTexture->GetDescription().m_Size.width == uiWidth && pTexture->GetDescription().m_Size.height == uiHeight;
    };

    if (MatchesResolution(textures[0]) && MatchesResolution(textures[1]))
      return false;

    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = format;
    description.m_Size.width  = uiWidth;
    description.m_Size.height = uiHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    textures[0] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    textures[1] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    XII_ASSERT_DEV(textures[0] != nullptr && textures[1] != nullptr, "Failed to allocate temporal lighting history textures.");
    return true;
  }

  static xiiRenderGraphBufferHandle CreateTemporalDenoiseConstants(xiiRenderGraphBuilder& builder, xiiStringView sName)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = sizeof(xiiTemporalDenoiseConstants);
    description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
    description.m_Usage          = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    return builder.WriteBuffer(sName, description, xiiGALResourceStateFlags::ConstantBuffer);
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiView::ViewPassResources* xiiView::ResolveViewPassResources(xiiViewRenderResourceContextHandle handle)
{
  auto* pResources = static_cast<ViewPassResources*>(xiiViewRenderResourceManager::GetContext(handle));
  XII_ASSERT_DEV(pResources != nullptr, "The view render-resource context is not initialized or was invalidated by subsystem shutdown.");
  return pResources;
}

xiiView::xiiView(xiiWorld* pWorld) :
  m_pWorld(pWorld)
{
  m_DisplayOutputSettings = xiiDisplayOutputManager::GetDefaults();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "No default device available. A view requires a device to initialize its resources.");

  m_ViewPassResources.m_Handle = xiiViewRenderResourceManager::CreateContext();
  XII_ASSERT_DEV(m_ViewPassResources.m_Handle.IsValid(), "Failed to create the view render-resource context.");
  XII_VERIFY(m_RenderGraphContext.Initialize("View Render Graph").Succeeded(), "Failed to create the view render graph context.");
  XII_VERIFY(m_ViewPassResources->m_LightingSystem.Initialize().Succeeded(), "Failed to create the view lighting context.");

  UpdateRenderResolutionState();
}

xiiResult xiiView::SetDisplayOutputSettings(const xiiDisplayOutputSettings& settings)
{
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;

  m_DisplayOutputSettings                                       = settings;
  m_ViewPassResources->m_TemporalPasses.m_bExposureHistoryValid = false;
  return XII_SUCCESS;
}

const xiiDisplayOutputSettings& xiiView::GetDisplayOutputSettings() const
{
  return m_DisplayOutputSettings;
}

void xiiView::InvalidateTemporalHistory()
{
  m_ViewPassResources->m_DepthPasses.m_bMotionHistoryValid                  = false;
  m_ViewPassResources->m_ShadowPasses.m_bRayTracedShadowHistoryValid        = false;
  m_ViewPassResources->m_LightingPrepPasses.m_bAmbientOcclusionHistoryValid = false;
  m_ViewPassResources->m_LightingPasses.m_bRTGIReservoirHistoryValid        = false;
  m_ViewPassResources->m_LightingPasses.m_bRTGIHistoryValid                 = false;
  m_ViewPassResources->m_LightingPasses.m_bRTReflectionHistoryValid         = false;
  m_ViewPassResources->m_LightingPasses.m_bDirectReservoirHistoryValid      = false;
  m_ViewPassResources->m_LightingPasses.m_bVolumetricHistoryValid           = false;
  m_ViewPassResources->m_TemporalPasses.m_uiTAAHistoryWriteIndex            = 0U;
  m_ViewPassResources->m_TemporalPasses.m_bTAAHistoryValid                  = false;
  m_ViewPassResources->m_TemporalPasses.m_bExposureHistoryValid             = false;
}

xiiView::~xiiView()
{
  m_InternalId.Invalidate();

  // High-level subsystem shutdown may have invalidated the handle before a late world teardown.
  // In that case all persistent resources (including the lighting context) are already released.
  if (auto* pResources = static_cast<ViewPassResources*>(xiiViewRenderResourceManager::GetContext(m_ViewPassResources.m_Handle)))
  {
    pResources->m_LightingSystem.Shutdown();
  }
  m_RenderGraphContext.Shutdown();
  xiiViewRenderResourceManager::DestroyContext(m_ViewPassResources.m_Handle);
  m_ViewPassResources.m_Handle = {};
}

xiiResult xiiView::SetSensorProfile(xiiSensorProfileHandle hProfile)
{
  if (hProfile.IsValid())
  {
    xiiSensorProfile profile;
    if (xiiSensorRenderingManager::GetProfile(hProfile, profile).Failed())
      return XII_FAILURE;
  }

  auto& outputPasses = m_ViewPassResources->m_OutputPasses;
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
  return m_ViewPassResources->m_OutputPasses.m_hSensorProfile;
}

xiiSharedPtr<xiiGALTexture> xiiView::GetSensorOutputTexture() const
{
  return m_ViewPassResources->m_OutputPasses.m_pSensorOutputTexture;
}

void xiiView::SetRenderScale(float fRenderScale)
{
  m_ViewPassResources->m_DynamicResolution.m_fRenderScale = xiiMath::Clamp(fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(m_ViewPassResources->m_DynamicResolution.m_fRenderScale, fDynamicMin, fDynamicMax);

  m_ViewPassResources->m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fCurrentScale, fDynamicMin, fDynamicMax);
  m_ViewPassResources->m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

  UpdateRenderResolutionState();
}

float xiiView::GetRenderScale() const
{
  return m_ViewPassResources->m_DynamicResolution.m_fRenderScale;
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
  const float fRenderScale = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);

  float fDynamicMin = 0.1f;
  float fDynamicMax = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fDynamicScale = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fSmoothedScale, fDynamicMin, fDynamicMax);

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

  m_ViewPassResources->m_DynamicResolution.m_fLastGpuFrameTimeMs = fGpuTimeMs;

  // PID controller.
  const float fRenderScale = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fRenderScale, 0.1f, 1.0f);
  float       fDynamicMin  = 0.1f;
  float       fDynamicMax  = 1.0f;
  ComputeDynamicScaleBounds(fRenderScale, fDynamicMin, fDynamicMax);

  const float fTarget    = xiiMath::Max(cvar_DynamicRenderingTargetMs.GetValue(), 0.1f);
  const float fDeltaTime = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;

  const float fError = (fTarget - fGpuTimeMs) / fTarget;

  // Anti-windup clamp on integral.
  m_ViewPassResources->m_DynamicResolution.m_fErrorIntegral = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fErrorIntegral + fError * fDeltaTime, -1.0f, 1.0f);

  const float fDerivative = (fError - m_ViewPassResources->m_DynamicResolution.m_fPreviousError) / xiiMath::Max(fDeltaTime, 0.001f);
  const float fPID        = 0.35f * fError + 0.05f * m_ViewPassResources->m_DynamicResolution.m_fErrorIntegral + 0.15f * fDerivative;

  // Clamp per-frame delta to avoid oscillation.
  const float fDesired = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fCurrentScale + fPID, fDynamicMin, fDynamicMax);
  const float fDelta   = xiiMath::Clamp(fDesired - m_ViewPassResources->m_DynamicResolution.m_fCurrentScale, -0.10f, 0.10f);

  m_ViewPassResources->m_DynamicResolution.m_fCurrentScale  = xiiMath::Clamp(m_ViewPassResources->m_DynamicResolution.m_fCurrentScale + fDelta, fDynamicMin, fDynamicMax);
  m_ViewPassResources->m_DynamicResolution.m_fSmoothedScale = xiiMath::Clamp(xiiMath::Lerp(m_ViewPassResources->m_DynamicResolution.m_fSmoothedScale, m_ViewPassResources->m_DynamicResolution.m_fCurrentScale, 0.20f), fDynamicMin, fDynamicMax);
  m_ViewPassResources->m_DynamicResolution.m_fPreviousError = fError;

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
    m_ViewPassResources->m_LightingSystem.UploadFrameData(cmd);

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
  data.m_uiReadSlot = (m_ViewPassResources->m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRenderGraphPassContext& context)
{
  xiiSharedPtr<xiiGALBuffer>& pStaging = m_ViewPassResources->m_VisibilityPasses.m_pOcclusionReadbackRing[data.m_uiReadSlot];

  if (pStaging == nullptr)
  {
    // Buffer not yet populated - skip.
    // Advance the write slot so the next stage can write into it next frame.
    m_ViewPassResources->m_VisibilityPasses.m_uiReadbackWriteSlot = (m_ViewPassResources->m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
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
  m_ViewPassResources->m_VisibilityPasses.m_uiReadbackWriteSlot = (m_ViewPassResources->m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % ViewPassResources::VisibilityPasses::s_uiReadbackRingSize;
}

////////// GPU Frustum Culling //////////
//
// Culls the current frame's extracted mesh packets against the view frustum on the GPU.
// The compact output is consumed by LOD selection, instance update, and indirect draw construction.

struct xiiFrustumCullInstanceBounds
{
  xiiVec4 m_CenterRadius;
  xiiVec4 m_Extents;
};

static_assert(sizeof(xiiFrustumCullInstanceBounds) == 32U);

struct xiiFrustumCullData
{
  xiiRenderGraphBufferHandle m_hInstanceBounds;    ///< SRV in (one world-space bound per extracted mesh packet).
  xiiRenderGraphBufferHandle m_hVisibleCandidates; ///< UAV out ([0]=count, [1..]=visible packet indices).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiFrustumCullInstanceBounds, xiiAlignedAllocatorWrapper> m_InstanceBounds;
  xiiFrustumCullingConstants                                                m_Constants       = {};
  xiiUInt32                                                                 m_uiInstanceCount = 0U;
};

void xiiView::SetupFrustumCull(xiiFrustumCullData& data, xiiRenderGraphBuilder& builder)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_InstanceBounds.Reserve(xiiMath::Min(renderData.GetCount(), k_uiMaxInstances));
  for (const xiiRenderData* pRenderData : renderData)
  {
    const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
    if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
      continue;

    xiiFrustumCullInstanceBounds& bounds = data.m_InstanceBounds.ExpandAndGetRef();
    bounds.m_CenterRadius                = xiiVec4(pMesh->m_GlobalBounds.m_vCenter.x, pMesh->m_GlobalBounds.m_vCenter.y, pMesh->m_GlobalBounds.m_vCenter.z, pMesh->m_GlobalBounds.m_fSphereRadius);
    bounds.m_Extents                     = xiiVec4(pMesh->m_GlobalBounds.m_vBoxHalfExtents.x, pMesh->m_GlobalBounds.m_vBoxHalfExtents.y, pMesh->m_GlobalBounds.m_vBoxHalfExtents.z, 0.0f);

    if (data.m_InstanceBounds.GetCount() == k_uiMaxInstances)
      break;
  }
  data.m_uiInstanceCount = data.m_InstanceBounds.GetCount();
  GetBlackboard().Set(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);

  // Ensure persistent instance bounds buffer exists.
  if (!m_ViewPassResources->m_VisibilityPasses.m_pInstanceBoundsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                               = 32U; // float3 center + float radius + float3 extents + float pad.
    description.m_uiSize                                            = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                         = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                              = xiiGALBufferMode::Structured;
    description.m_Usage                                             = xiiGALResourceUsage::Default;
    m_ViewPassResources->m_VisibilityPasses.m_pInstanceBoundsBuffer = pDevice->CreateBuffer(description);
  }

  // Import persistent instance bounds as read-only SRV. Their contents are refreshed before dispatch.
  data.m_hInstanceBounds = builder.ImportBuffer("InstanceBoundsIn", m_ViewPassResources->m_VisibilityPasses.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds = builder.ReadBuffer(data.m_hInstanceBounds, xiiGALResourceStateFlags::ShaderResource);

  // Transient visible candidate buffer.
  xiiGALBufferCreationDescription visibleCandidateBufferDescription;
  visibleCandidateBufferDescription.m_uiElementByteStride = 4U;                         // uint
  visibleCandidateBufferDescription.m_uiSize              = 4U + 4U * k_uiMaxInstances; // [0]=count + indices
  visibleCandidateBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  visibleCandidateBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  visibleCandidateBufferDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hVisibleCandidates                               = builder.WriteBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, visibleCandidateBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiFrustumCullingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiFrustumCullingConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  if (GetCullingCamera() != nullptr)
  {
    xiiFrustum frustum;
    ComputeCullingFrustum(frustum);
    data.m_Constants.FrustumPlane0 = frustum.GetPlane(xiiFrustum::NearPlane).GetAsVec4();
    data.m_Constants.FrustumPlane1 = frustum.GetPlane(xiiFrustum::LeftPlane).GetAsVec4();
    data.m_Constants.FrustumPlane2 = frustum.GetPlane(xiiFrustum::RightPlane).GetAsVec4();
    data.m_Constants.FrustumPlane3 = frustum.GetPlane(xiiFrustum::FarPlane).GetAsVec4();
    data.m_Constants.FrustumPlane4 = frustum.GetPlane(xiiFrustum::BottomPlane).GetAsVec4();
    data.m_Constants.FrustumPlane5 = frustum.GetPlane(xiiFrustum::TopPlane).GetAsVec4();
  }
  else
  {
    data.m_uiInstanceCount = 0U;
  }
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pFrustumCullPipeline, "Shaders/Pipeline/CoarseFrustumCulling.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("FrustumCulling");
  {
    if (!data.m_InstanceBounds.IsEmpty())
    {
      const xiiUInt8* pBoundsBytes = reinterpret_cast<const xiiUInt8*>(data.m_InstanceBounds.GetData());
      cmd.UpdateBuffer(context.GetBuffer(data.m_hInstanceBounds), 0U, xiiMakeArrayPtr(pBoundsBytes, data.m_InstanceBounds.GetCount() * sizeof(xiiFrustumCullInstanceBounds)));
    }

    const xiiUInt32 uiZero = 0U;
    cmd.UpdateBuffer(context.GetBuffer(data.m_hVisibleCandidates), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    {
      xiiGALMapHelper<xiiFrustumCullingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pFrustumCullPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiFrustumCullingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleOut", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU LOD Selection //////////
//
// Resolves the LOD selected during mesh extraction into a GPU-visible metadata stream. The
// residency-aware xiiGpuVisibilitySystem performs fully GPU-driven LOD selection for GPU scenes.

struct xiiLODSelectData
{
  xiiRenderGraphBufferHandle m_hVisibleCandidates; ///< SRV in ([0]=count, [1..]=visible packet indices).
  xiiRenderGraphBufferHandle m_hPreferredLOD;      ///< SRV in (extracted LOD and flags for every packet).
  xiiRenderGraphBufferHandle m_hInstanceLOD;       ///< UAV out (packed LOD, material bin, and flags).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiUInt32> m_PreferredLOD;
  xiiLODSelectionConstants   m_Constants       = {};
  xiiUInt32                  m_uiInstanceCount = 0U;
};

void xiiView::SetupLODSelect(xiiLODSelectData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);

  const bool bHasExtractedMeshCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  data.m_PreferredLOD.Reserve(data.m_uiInstanceCount);
  if (m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_pExtractedData->GetAllRenderData())
    {
      const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
      if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
        continue;

      const xiiUInt32 uiLod   = xiiMath::Min(pMesh->m_uiLODIndex, 0xFFU);
      const xiiUInt32 uiFlags = pMesh->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) ? XII_BIT(0) : 0U;
      data.m_PreferredLOD.PushBack(uiLod | (uiFlags << 16U));
      if (data.m_PreferredLOD.GetCount() == data.m_uiInstanceCount)
        break;
    }
  }
  data.m_uiInstanceCount         = data.m_PreferredLOD.GetCount();
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiGALBufferCreationDescription preferredDescription;
  preferredDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  preferredDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(1U, data.m_uiInstanceCount);
  preferredDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  preferredDescription.m_Mode                = xiiGALBufferMode::Structured;
  preferredDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hPreferredLOD                       = builder.WriteBuffer("ExtractedLODMetadata", preferredDescription, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U; // packed uint: LOD level + meshlet offset
  description.m_uiSize              = description.m_uiElementByteStride * k_uiMaxInstances;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceLOD               = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiLODSelectionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiLODSelectionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pLODSelectPipeline, "Shaders/Pipeline/LodSelection.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteLODSelect(const xiiLODSelectData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LODSelection");
  {
    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    cmd.UpdateBuffer(context.GetBuffer(data.m_hPreferredLOD), 0U,
                     xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_PreferredLOD.GetData()), data.m_PreferredLOD.GetCount() * sizeof(xiiUInt32)));
    {
      xiiGALMapHelper<xiiLODSelectionConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pLODSelectPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiLODSelectionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_PreferredLOD", context.GetBuffer(data.m_hPreferredLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_InstanceLODOut", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Instance Update //////////
//
// Compacts extracted transforms and bounds into GPU rendering streams using the visible packet list.

struct alignas(16) xiiInstanceUpdateSource
{
  xiiShaderTransform m_GlobalTransform;
  xiiVec4            m_CenterRadius;
  xiiVec4            m_Extents;
};

static_assert(sizeof(xiiInstanceUpdateSource) == 80U);

struct xiiInstanceUpdateData
{
  xiiRenderGraphBufferHandle m_hInstanceSource;    ///< SRV in (current extracted transform and bounds).
  xiiRenderGraphBufferHandle m_hInstanceMatrices;  ///< UAV out (world transforms indexed by packet index).
  xiiRenderGraphBufferHandle m_hInstanceBoundsOut; ///< UAV out (world bounds indexed by packet index).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiInstanceUpdateSource, xiiAlignedAllocatorWrapper> m_InstanceSource;
  xiiInstanceUpdateConstants                                           m_Constants       = {};
  xiiUInt32                                                            m_uiInstanceCount = 0U;
};

void xiiView::SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRenderGraphBuilder& builder)
{
  xiiUInt32  uiExtractedMeshCount   = 0U;
  const bool bHasExtractedMeshCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, uiExtractedMeshCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  data.m_InstanceSource.Reserve(uiExtractedMeshCount);
  if (m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_pExtractedData->GetAllRenderData())
    {
      const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
      if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
        continue;

      xiiInstanceUpdateSource& source = data.m_InstanceSource.ExpandAndGetRef();
      source.m_GlobalTransform        = pMesh->m_GlobalTransform;
      source.m_CenterRadius           = xiiVec4(pMesh->m_GlobalBounds.m_vCenter.x, pMesh->m_GlobalBounds.m_vCenter.y, pMesh->m_GlobalBounds.m_vCenter.z, pMesh->m_GlobalBounds.m_fSphereRadius);
      source.m_Extents                = xiiVec4(pMesh->m_GlobalBounds.m_vBoxHalfExtents.x, pMesh->m_GlobalBounds.m_vBoxHalfExtents.y, pMesh->m_GlobalBounds.m_vBoxHalfExtents.z, 0.0f);

      if (data.m_InstanceSource.GetCount() == uiExtractedMeshCount)
        break;
    }
  }
  data.m_uiInstanceCount         = data.m_InstanceSource.GetCount();
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiGALBufferCreationDescription sourceDescription;
  sourceDescription.m_uiElementByteStride = sizeof(xiiInstanceUpdateSource);
  sourceDescription.m_uiSize              = sizeof(xiiInstanceUpdateSource) * xiiMath::Max(1U, data.m_uiInstanceCount);
  sourceDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  sourceDescription.m_Mode                = xiiGALBufferMode::Structured;
  sourceDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hInstanceSource                  = builder.WriteBuffer("ExtractedInstanceSource", sourceDescription, xiiGALResourceStateFlags::ShaderResource);

  // Ensure persistent matrix buffer.
  if (!m_ViewPassResources->m_VisibilityPasses.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                               = 48U; // float4x3 (3 rows x 4 floats)
    description.m_uiSize                                            = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                         = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                              = xiiGALBufferMode::Structured;
    description.m_Usage                                             = xiiGALResourceUsage::Default;
    m_ViewPassResources->m_VisibilityPasses.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_hInstanceMatrices = builder.ImportBuffer("InstanceWorldMatrices", m_ViewPassResources->m_VisibilityPasses.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hInstanceMatrices = builder.WriteBuffer(data.m_hInstanceMatrices, xiiGALResourceStateFlags::UnorderedAccess);

  // Output AABB buffer for HiZ culling.
  xiiGALBufferCreationDescription boundsBufferDescription;
  boundsBufferDescription.m_uiElementByteStride = 32U;
  boundsBufferDescription.m_uiSize              = boundsBufferDescription.m_uiElementByteStride * k_uiMaxInstances;
  boundsBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  boundsBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceBoundsOut                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, boundsBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiInstanceUpdateConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiInstanceUpdateConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pInstanceUpdatePipeline, "Shaders/Pipeline/InstanceUpdate.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("InstanceUpdate");
  {
    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    cmd.UpdateBuffer(context.GetBuffer(data.m_hInstanceSource), 0U,
                     xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_InstanceSource.GetData()), data.m_InstanceSource.GetCount() * sizeof(xiiInstanceUpdateSource)));
    {
      xiiGALMapHelper<xiiInstanceUpdateConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pInstanceUpdatePipeline);
    cmd.ResolveAndSetConstantBuffer("xiiInstanceUpdateConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceSource", context.GetBuffer(data.m_hInstanceSource)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_MatricesOut", context.GetBuffer(data.m_hInstanceMatrices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_BoundsOut", context.GetBuffer(data.m_hInstanceBoundsOut)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Draw Command Build //////////
//
// Compacts exact draw ranges extracted from mesh resources into an indexed-indirect stream.

struct xiiExtractedDrawCommand
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndexCountPerInstance = 0U;
  xiiUInt32 m_uiInstanceCount         = 0U;
  xiiUInt32 m_uiStartIndexLocation    = 0U;
  xiiInt32  m_iBaseVertexLocation     = 0;
  xiiUInt32 m_uiStartInstanceLocation = 0U;
};

static_assert(sizeof(xiiExtractedDrawCommand) == 20U);

struct xiiDrawBuildData
{
  xiiRenderGraphBufferHandle m_hSurvivors;
  xiiRenderGraphBufferHandle m_hInstanceLOD;
  xiiRenderGraphBufferHandle m_hSourceCommands;
  xiiRenderGraphBufferHandle m_hDrawCommands;
  xiiRenderGraphBufferHandle m_hDrawCounts;
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiExtractedDrawCommand> m_SourceCommands;
  xiiDrawCommandBuildConstants             m_Constants       = {};
  xiiUInt32                                m_uiInstanceCount = 0U;
  bool                                     m_bUploadSource   = false;
};

void xiiView::SetupDrawBuildResources(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder, xiiStringView sCandidateBuffer, xiiStringView sCommandBuffer, xiiStringView sCountBuffer, bool bUploadSource)
{
  data.m_hSurvivors    = builder.ReadBuffer(sCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceLOD  = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_bUploadSource = bUploadSource;

  xiiUInt32  uiExtractedMeshCount   = 0U;
  const bool bHasExtractedMeshCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, uiExtractedMeshCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  if (bUploadSource)
  {
    data.m_SourceCommands.Reserve(uiExtractedMeshCount);
    if (m_pExtractedData != nullptr)
    {
      for (const xiiRenderData* pRenderData : m_pExtractedData->GetAllRenderData())
      {
        const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
        if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
          continue;

        xiiExtractedDrawCommand& command  = data.m_SourceCommands.ExpandAndGetRef();
        command.m_uiIndexCountPerInstance = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(static_cast<xiiUInt64>(pMesh->m_uiPrimitiveCount) * 3ULL, xiiMath::MaxValue<xiiUInt32>()));
        command.m_uiInstanceCount         = 1U;
        command.m_uiStartIndexLocation    = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(static_cast<xiiUInt64>(pMesh->m_uiFirstPrimitive) * 3ULL, xiiMath::MaxValue<xiiUInt32>()));
        command.m_iBaseVertexLocation     = 0;
        command.m_uiStartInstanceLocation = data.m_SourceCommands.GetCount() - 1U;

        if (data.m_SourceCommands.GetCount() == uiExtractedMeshCount)
          break;
      }
    }
    data.m_uiInstanceCount = data.m_SourceCommands.GetCount();
  }
  else
  {
    data.m_uiInstanceCount = uiExtractedMeshCount;
  }

  data.m_Constants.InstanceCount   = data.m_uiInstanceCount;
  data.m_Constants.MaxCommandCount = data.m_uiInstanceCount;
  GetBlackboard().Set(xiiRGBlackboardKeys::k_DrawCommandCapacity, data.m_uiInstanceCount);

  if (bUploadSource)
  {
    xiiGALBufferCreationDescription sourceDescription;
    sourceDescription.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
    sourceDescription.m_uiSize              = sizeof(xiiExtractedDrawCommand) * xiiMath::Max(1U, data.m_uiInstanceCount);
    sourceDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    sourceDescription.m_Mode                = xiiGALBufferMode::Structured;
    sourceDescription.m_Usage               = xiiGALResourceUsage::Default;
    data.m_hSourceCommands                  = builder.WriteBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, sourceDescription, xiiGALResourceStateFlags::ShaderResource);
  }
  else
  {
    data.m_hSourceCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  }

  xiiGALBufferCreationDescription commandDescription;
  commandDescription.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
  commandDescription.m_uiSize              = sizeof(xiiExtractedDrawCommand) * xiiMath::Max(1U, data.m_uiInstanceCount);
  commandDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  commandDescription.m_Mode                = xiiGALBufferMode::Structured;
  commandDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hDrawCommands                     = builder.WriteBuffer(sCommandBuffer, commandDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription counterBufferDescription;
  counterBufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  counterBufferDescription.m_uiSize              = sizeof(xiiUInt32);
  counterBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  counterBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hDrawCounts                             = builder.WriteBuffer(sCountBuffer, counterBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiDrawCommandBuildConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiDrawCommandBuildConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pDrawBuildPipeline, "Shaders/Pipeline/DrawCommandBuild.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::SetupCoarseDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder)
{
  SetupDrawBuildResources(data, builder, xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiRGBlackboardKeys::k_CoarseDrawIndirectCommands, xiiRGBlackboardKeys::k_CoarseDrawCountBuffer, true);
}

void xiiView::SetupDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder)
{
  SetupDrawBuildResources(data, builder, xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiRGBlackboardKeys::k_DrawCountBuffer, false);
}

void xiiView::ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DrawCommandBuild");
  {
    const xiiUInt32 uiZero = 0U;
    cmd.UpdateBuffer(context.GetBuffer(data.m_hDrawCounts), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    xiiDynamicArray<xiiExtractedDrawCommand> zeroCommands;
    zeroCommands.SetCount(xiiMath::Max(1U, data.m_uiInstanceCount));
    xiiMemoryUtils::ZeroFill(zeroCommands.GetData(), zeroCommands.GetCount());
    cmd.UpdateBuffer(context.GetBuffer(data.m_hDrawCommands), 0U,
                     xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCommands.GetData()), zeroCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    if (data.m_bUploadSource)
    {
      cmd.UpdateBuffer(context.GetBuffer(data.m_hSourceCommands), 0U,
                       xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_SourceCommands.GetData()), data.m_SourceCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));
    }
    {
      xiiGALMapHelper<xiiDrawCommandBuildConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pDrawBuildPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiDrawCommandBuildConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Survivors", context.GetBuffer(data.m_hSurvivors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceLOD", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SourceCommands", context.GetBuffer(data.m_hSourceCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawArgs", context.GetBuffer(data.m_hDrawCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawCounts", context.GetBuffer(data.m_hDrawCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Caster List Build //////////
//
// Builds one global caster command block for local lights and one culled block per cascade.

struct xiiShadowCasterBuildData
{
  xiiRenderGraphBufferHandle m_hSourceCommands;
  xiiRenderGraphBufferHandle m_hInstanceBounds;
  xiiRenderGraphBufferHandle m_hCascadeConstants;
  xiiRenderGraphBufferHandle m_hShadowCasterCommands;
  xiiRenderGraphBufferHandle m_hShadowCasterCounts;
  xiiRenderGraphBufferHandle m_hConstants;

  xiiShadowCasterCullingConstants m_Constants       = {};
  xiiUInt32                       m_uiInstanceCount = 0U;
};

void xiiView::SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSourceCommands   = builder.ReadBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds   = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCascadeConstants = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);

  const bool bHasExtractedMeshCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);
  data.m_Constants.InstanceCount             = data.m_uiInstanceCount;
  data.m_Constants.MaxCommandCount           = data.m_uiInstanceCount;
  data.m_Constants.CullingActiveCascadeCount = m_ViewPassResources->m_ShadowPasses.m_uiActiveCascadeCount;
  GetBlackboard().Set(xiiRGBlackboardKeys::k_ShadowCommandCapacity, data.m_uiInstanceCount);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
  description.m_uiSize              = description.m_uiElementByteStride * xiiMath::Max(1U, data.m_uiInstanceCount) * 5U; // Global block followed by four cascade blocks.
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hShadowCasterCommands      = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, description, xiiGALResourceStateFlags::UnorderedAccess);

  description.m_uiElementByteStride = sizeof(xiiUInt32);
  description.m_uiSize              = sizeof(xiiUInt32) * 5U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hShadowCasterCounts        = builder.WriteBuffer("ShadowCasterCommandCounts", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiShadowCasterCullingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiShadowCasterCullingConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pShadowCasterBuildPipeline, "Shaders/Pipeline/ShadowCasterCulling.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowCasterListBuild");
  {
    xiiDynamicArray<xiiExtractedDrawCommand> zeroCommands;
    zeroCommands.SetCount(xiiMath::Max(1U, data.m_uiInstanceCount) * 5U);
    xiiMemoryUtils::ZeroFill(zeroCommands.GetData(), zeroCommands.GetCount());
    cmd.UpdateBuffer(context.GetBuffer(data.m_hShadowCasterCommands), 0U,
                     xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCommands.GetData()), zeroCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));

    xiiUInt32 zeroCounts[5] = {};
    cmd.UpdateBuffer(context.GetBuffer(data.m_hShadowCasterCounts), 0U,
                     xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCounts), sizeof(zeroCounts)));

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    {
      xiiGALMapHelper<xiiShadowCasterCullingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pShadowCasterBuildPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCasterCullingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SourceCommands", context.GetBuffer(data.m_hSourceCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceBounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawArgs", context.GetBuffer(data.m_hShadowCasterCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawCounts", context.GetBuffer(data.m_hShadowCasterCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
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
  builder.SetPassAllowMerge(false);

  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiTotalClusters = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 32U; // float4 min + float4 max per cluster AABB
  description.m_uiSize              = description.m_uiElementByteStride * uiTotalClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hClusterDescriptors        = builder.WriteBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pClusterBuildPipeline, "Shaders/Pipeline/ClusterGridBuild.xiiShader");

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
    xiiUInt32 uiClusterCountX = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetClusterCountX(), 1U);
    xiiUInt32 uiClusterCountY = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetClusterCountY(), 1U);
    xiiUInt32 uiClusterCountZ = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetClusterCountZ(), 1U);
    xiiUInt32 uiTotalClusters = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetTotalClusterCount(), 1U);

    {
      xiiGALMapHelper<xiiLightClusteringConstants> pClusteringConstants(cmd, context.GetBuffer(data.m_hClusterConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pClusteringConstants->ClusterCountX       = uiClusterCountX;
      pClusteringConstants->ClusterCountY       = uiClusterCountY;
      pClusteringConstants->ClusterCountZ       = uiClusterCountZ;
      pClusteringConstants->TotalClusters       = uiTotalClusters;
      pClusteringConstants->NearPlane           = m_pCamera->GetNearPlane();
      pClusteringConstants->FarPlane            = m_pCamera->GetFarPlane();
      pClusteringConstants->LogFarOverNear      = xiiMath::Log2(pClusteringConstants->FarPlane / pClusteringConstants->NearPlane);
      pClusteringConstants->TilePixelsX         = m_ViewPassResources->m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->TilePixelsY         = m_ViewPassResources->m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->MaxLightsPerCluster = m_ViewPassResources->m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster;
      pClusteringConstants->ActiveLightCount    = m_ViewPassResources->m_LightingSystem.GetActiveLightCount();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pClusterBuildPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
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

struct xiiLightListClearData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterConstants;
  xiiRenderGraphBufferHandle m_hLightGridBuffer;
  xiiUInt32                  m_uiTotalClusters = 0U;
};

void xiiView::SetupLightListClear(xiiLightListClearData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterConstants = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);

  const xiiUInt32 uiMaxClusters = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 8U; // uint2 (fixed list offset, atomic count) per cluster
  description.m_uiSize              = description.m_uiElementByteStride * uiMaxClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightGridBuffer           = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiTotalClusters            = uiMaxClusters;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pLightListClearPipeline, "Shaders/Pipeline/LightListClear.xiiShader");
}

void xiiView::ExecuteLightListClear(const xiiLightListClearData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListClear");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pLightListClearPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusters + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

struct xiiLightListData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterConstants;      ///< Constant buffer in (cluster dimensions and active light count).
  xiiRenderGraphBufferHandle m_hLightIndexBuffer;      ///< SRV in (structured buffer of uint, one per light, containing light type and other metadata, from extraction).
  xiiRenderGraphBufferHandle m_hLightGridBuffer;       ///< UAV out (structured buffer of uint, containing compact light lists per cluster, consumed by main lighting pass).
  xiiUInt32                  m_uiActiveLightCount = 0; ///< Number of active lights to process (from extraction). This is used to avoid processing the entire buffer when only a subset is populated.
};

void xiiView::SetupLightListBuild(xiiLightListData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterConstants = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);

  const xiiUInt32 uiMaxClusters    = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetTotalClusterCount(), 1U);
  const xiiUInt32 uiMaxLightsPerCl = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster, 1U);

  xiiGALBufferCreationDescription indexBufferDescription;
  indexBufferDescription.m_uiElementByteStride = 4U;
  indexBufferDescription.m_uiSize              = 4U * uiMaxClusters * uiMaxLightsPerCl;
  indexBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  indexBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightIndexBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, indexBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hLightGridBuffer = builder.WriteBuffer(builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiActiveLightCount = m_ViewPassResources->m_LightingSystem.GetActiveLightCount();

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pLightListPipeline, "Shaders/Pipeline/LightListBuild.xiiShader");
}

void xiiView::ExecuteLightListBuild(const xiiLightListData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListBuild");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pLightListPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources->m_LightingSystem.BindLightData(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(xiiMath::Max(data.m_uiActiveLightCount, 1U) + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Probe Select Data //////////
//
// Selects relevant reflection probes for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of reflection probe indices and a bitmask of which probes affect which instances, which are then consumed by the main lighting pass for reflection probe sampling.

struct xiiReflectionProbeSelectData
{
  xiiRenderGraphBufferHandle                                         m_hClusterDescriptors; ///< SRV in: world-space lighting-cluster AABBs.
  xiiRenderGraphBufferHandle                                         m_hProbeData;          ///< SRV in: active probe transforms, volumes, and texture slots.
  xiiRenderGraphBufferHandle                                         m_hProbeConstants;     ///< Constant buffer in: active probe and cluster counts.
  xiiRenderGraphBufferHandle                                         m_hProbeClusters;      ///< UAV out: primary and secondary probe indices per cluster.
  xiiDynamicArray<xiiGPUReflectionProbe, xiiAlignedAllocatorWrapper> m_Probes;
  xiiUInt32                                                          m_uiTotalClusterCount = 1U;
};

void xiiView::SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);

  auto& reflectionResources = m_ViewPassResources->m_LightingPrepPasses;
  reflectionResources.m_ReflectionProbeTextures.Clear();

  if (reflectionResources.m_pFallbackReflectionProbeTexture == nullptr)
  {
    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    textureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    textureDescription.m_Size.width         = 1U;
    textureDescription.m_Size.height        = 1U;
    textureDescription.m_uiArraySizeOrDepth = 6U;
    textureDescription.m_uiMipLevels        = 1U;
    textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    textureDescription.m_Usage              = xiiGALResourceUsage::Immutable;

    const xiiUInt32                                  uiBlackPixel = 0xFF000000U;
    xiiHybridArray<xiiGALTextureSubResourceData, 6U> initialData;
    for (xiiUInt32 uiFace = 0U; uiFace < 6U; ++uiFace)
    {
      xiiGALTextureSubResourceData& faceData = initialData.ExpandAndGetRef();
      faceData.m_pData                       = xiiMakeByteBlobPtr(static_cast<const void*>(&uiBlackPixel), sizeof(uiBlackPixel));
      faceData.m_uiStride                    = sizeof(uiBlackPixel);
      faceData.m_uiDepthStride               = sizeof(uiBlackPixel);
    }

    xiiGALTextureData textureData(initialData);
    reflectionResources.m_pFallbackReflectionProbeTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(textureDescription, &textureData);
    if (reflectionResources.m_pFallbackReflectionProbeTexture != nullptr)
      reflectionResources.m_pFallbackReflectionProbeTexture->SetDebugName("ReflectionProbe::FallbackBlackCube");
  }

  xiiHybridArray<const xiiReflectionCaptureRenderData*, XII_MAX_REFLECTION_PROBES> candidates;
  if (m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_pExtractedData->GetAllRenderData())
    {
      if (const xiiReflectionCaptureRenderData* pProbe = xiiDynamicCast<const xiiReflectionCaptureRenderData*>(pRenderData))
        candidates.PushBack(pProbe);
    }
  }

  candidates.Sort([](const xiiReflectionCaptureRenderData* pLeft, const xiiReflectionCaptureRenderData* pRight) {
    if (pLeft->m_iPriority != pRight->m_iPriority)
      return pLeft->m_iPriority > pRight->m_iPriority;
    return pLeft->m_uiSortingKey < pRight->m_uiSortingKey;
  });

  data.m_Probes.Reserve(xiiMath::Min(candidates.GetCount(), XII_MAX_REFLECTION_PROBES));
  for (const xiiReflectionCaptureRenderData* pProbe : candidates)
  {
    if (data.m_Probes.GetCount() >= XII_MAX_REFLECTION_PROBES)
      break;

    xiiResourceLock<xiiTextureCubeResource> reflectionMap(pProbe->m_hReflectionMap, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!reflectionMap.IsValid())
      continue;

    xiiSharedPtr<xiiGALTexture> pTexture = reflectionMap->GetGALTexture();
    if (pTexture == nullptr)
      continue;

    const xiiVec3 vAbsoluteScale   = pProbe->m_GlobalTransform.m_vScale.Abs();
    const float   fMaximumScale    = xiiMath::Max(vAbsoluteScale.x, xiiMath::Max(vAbsoluteScale.y, vAbsoluteScale.z));
    const float   fInfluenceRadius = pProbe->m_InfluenceShape == xiiReflectionProbeInfluenceShape::Sphere ?
        pProbe->m_fSphereRadius * fMaximumScale :
        pProbe->m_vHalfExtents.CompMul(vAbsoluteScale).GetLength();

    xiiGPUReflectionProbe& gpuProbe = data.m_Probes.ExpandAndGetRef();
    gpuProbe.WorldToProbe           = pProbe->m_GlobalTransform.GetInverse().GetAsMat4();
    gpuProbe.PositionAndRadius      = xiiVec4(pProbe->m_GlobalTransform.m_vPosition, xiiMath::Max(fInfluenceRadius, 0.01f));
    gpuProbe.HalfExtentsAndBlend    = xiiVec4(pProbe->m_vHalfExtents, pProbe->m_fBlendDistance);
    gpuProbe.ProbeParameters        = xiiVec4(pProbe->m_fSphereRadius, pProbe->m_fIntensity, pProbe->m_fSaturation, static_cast<float>(pProbe->m_InfluenceShape.GetValue()));
    gpuProbe.Metadata               = xiiVec4U32(static_cast<xiiUInt32>(pProbe->m_uiSortingKey), data.m_Probes.GetCount() - 1U, static_cast<xiiUInt32>(pProbe->m_iPriority), pProbe->m_bParallaxCorrected ? 1U : 0U);

    reflectionResources.m_ReflectionProbeTextures.PushBack(std::move(pTexture));
  }

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiGPUReflectionProbe);
  description.m_uiSize              = sizeof(xiiGPUReflectionProbe) * XII_MAX_REFLECTION_PROBES;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hProbeData                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, description, xiiGALResourceStateFlags::ShaderResource);

  description.m_uiElementByteStride = 0U;
  description.m_uiSize              = sizeof(xiiReflectionProbeConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  data.m_hProbeConstants            = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeConstants, description, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_uiTotalClusterCount        = xiiMath::Max(m_ViewPassResources->m_LightingSystem.GetTotalClusterCount(), 1U);
  description.m_uiElementByteStride = sizeof(xiiVec2U32);
  description.m_uiSize              = sizeof(xiiVec2U32) * data.m_uiTotalClusterCount;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
  data.m_hProbeClusters             = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pProbeSelectPipeline, "Shaders/Pipeline/ReflectionProbeSelection.xiiShader");
}

void xiiView::ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReflectionProbeSelection");
  {
    {
      xiiGALMapHelper<xiiUInt8> pProbeBytes(cmd, context.GetBuffer(data.m_hProbeData), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      xiiMemoryUtils::ZeroFill(pProbeBytes.GetMappedData(), sizeof(xiiGPUReflectionProbe) * XII_MAX_REFLECTION_PROBES);
      if (!data.m_Probes.IsEmpty())
      {
        const xiiArrayPtr<const xiiUInt8> probeBytes = data.m_Probes.GetByteArrayPtr();
        xiiMemoryUtils::Copy(pProbeBytes.GetMappedData(), probeBytes.GetPtr(), probeBytes.GetCount());
      }
    }
    {
      xiiGALMapHelper<xiiReflectionProbeConstants> pConstants(cmd, context.GetBuffer(data.m_hProbeConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ActiveProbeCount        = data.m_Probes.GetCount();
      pConstants->TotalClusterCount       = data.m_uiTotalClusterCount;
      pConstants->_ReflectionProbePadding = xiiVec2U32::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pProbeSelectPipeline);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReflectionProbeConstants", context.GetBuffer(data.m_hProbeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ReflectionProbeClusters", context.GetBuffer(data.m_hProbeClusters)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusterCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Froxel Allocation Data //////////
//
// Allocates froxels for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of froxel metadata and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.

struct xiiFroxelAllocationData
{
  xiiRenderGraphBufferHandle  m_hFroxelMetadata;
  xiiRenderGraphTextureHandle m_hFroxelScattering;
  xiiRenderGraphBufferHandle  m_hVolumetricMedia;
  xiiRenderGraphBufferHandle  m_hVolumetricConstants;
  xiiGpuVolumetricMediumArray m_Media;
  xiiUInt32                   m_uiMediumBufferCapacity = 1U;
  xiiUInt32                   m_uiRenderWidth          = 1920U;
  xiiUInt32                   m_uiRenderHeight         = 1080U;
};

void xiiView::SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription froxelMetadataBufferDescription;
  froxelMetadataBufferDescription.m_uiElementByteStride = 32U;                                                                      // scattering/extinction + emission/phase
  froxelMetadataBufferDescription.m_uiSize              = froxelMetadataBufferDescription.m_uiElementByteStride * 128U * 72U * 64U; // froxel volume
  froxelMetadataBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
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

  if (xiiVolumetricMediumManager::IsSubsystemInitialized())
  {
    const xiiVec3 vViewPosition = m_pCamera != nullptr ? m_pCamera->GetCenterPosition() : xiiVec3::MakeZero();
    xiiVolumetricMediumManager::GatherGpuMedia(vViewPosition, data.m_Media);
  }

  data.m_uiMediumBufferCapacity = xiiMath::Max(data.m_Media.GetCount(), 1U);
  xiiGALBufferCreationDescription mediumBufferDescription;
  mediumBufferDescription.m_uiElementByteStride = sizeof(xiiGpuVolumetricMedium);
  mediumBufferDescription.m_uiSize              = sizeof(xiiGpuVolumetricMedium) * data.m_uiMediumBufferCapacity;
  mediumBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  mediumBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  mediumBufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  mediumBufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hVolumetricMedia                       = builder.WriteBuffer("Volumetric Media", mediumBufferDescription, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantBufferDescription;
  constantBufferDescription.m_uiSize         = sizeof(xiiVolumetricMediumConstants);
  constantBufferDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantBufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantBufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hVolumetricConstants                = builder.WriteBuffer("Volumetric Medium Constants", constantBufferDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_VisibilityPasses.m_pFroxelSetupPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricGridAllocation");
  {
    {
      xiiGALMapHelper<xiiUInt8> pMediumBytes(cmd, context.GetBuffer(data.m_hVolumetricMedia), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      xiiMemoryUtils::ZeroFill(pMediumBytes.GetMappedData(), sizeof(xiiGpuVolumetricMedium) * data.m_uiMediumBufferCapacity);
      if (!data.m_Media.IsEmpty())
      {
        const xiiArrayPtr<const xiiUInt8> mediumBytes = data.m_Media.GetByteArrayPtr();
        xiiMemoryUtils::Copy(pMediumBytes.GetMappedData(), mediumBytes.GetPtr(), mediumBytes.GetCount());
      }
    }
    {
      xiiGALMapHelper<xiiVolumetricMediumConstants> pConstants(cmd, context.GetBuffer(data.m_hVolumetricConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ActiveMediumCount        = data.m_Media.GetCount();
      pConstants->_VolumetricMediumPadding = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_VisibilityPasses.m_pFroxelSetupPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VolumetricMedia", context.GetBuffer(data.m_hVolumetricMedia)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVolumetricMediumConstants", context.GetBuffer(data.m_hVolumetricConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetadata", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
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

  xiiRenderGraphBufferHandle m_hCascadeMatrices;      ///< CPU-uploaded constant buffer of cascade view-projection matrices and split depths.
  xiiUInt32                  m_uiActiveCascades = 0U; ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes.
  xiiMat4                    m_CascadeViewProjection[4];
  xiiVec4                    m_vCascadeSplitDepths = xiiVec4::MakeZero();
  xiiVec4                    m_vCascadeWorldRadii  = xiiVec4::MakeZero();
};

void xiiView::SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRenderGraphBuilder& builder)
{
  for (xiiMat4& mCascadeViewProjection : data.m_CascadeViewProjection)
  {
    mCascadeViewProjection = xiiMat4::MakeIdentity();
  }

  xiiVec3                                 vLightDirection                   = xiiVec3(0.0f, 0.0f, -1.0f);
  const xiiArrayPtr<xiiRenderData* const> renderData                        = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  const xiiDirectionalLightRenderData*    pMainDirectional                  = SelectMainDirectionalLight(renderData);
  const bool                              bHasShadowCastingDirectionalLight = pMainDirectional != nullptr && pMainDirectional->m_bCastShadows;
  if (bHasShadowCastingDirectionalLight)
  {
    vLightDirection = pMainDirectional->m_vDirection;
  }

  xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
  xiiShadowCascadeSettings                       settings;
  settings.m_uiShadowMapResolution = k_uiDirectionalShadowAtlasWidth;
  const float fAspectRatio         = static_cast<float>(GetRenderResolutionWidth()) / static_cast<float>(xiiMath::Max(GetRenderResolutionHeight(), 1U));
  if (bHasShadowCastingDirectionalLight && xiiShadowCascadeUtils::Build(*m_pCamera, fAspectRatio, vLightDirection, settings, cascades).Succeeded())
  {
    data.m_uiActiveCascades = cascades.GetCount();
    for (xiiUInt32 uiCascade = 0U; uiCascade < cascades.GetCount(); ++uiCascade)
    {
      data.m_CascadeViewProjection[uiCascade]         = cascades[uiCascade].m_mViewProjection;
      data.m_vCascadeSplitDepths.GetData()[uiCascade] = cascades[uiCascade].m_fSplitFar;
      data.m_vCascadeWorldRadii.GetData()[uiCascade]  = cascades[uiCascade].m_fWorldRadius;
    }
  }
  m_ViewPassResources->m_ShadowPasses.m_uiActiveCascadeCount = data.m_uiActiveCascades;

  // GPU constant buffer: ShadowCascadeConstants (float4x4[4] + float4 + uint + pad3).
  // Keep the resource type consistent with DECLARE_CONSTANT_BUFFER_AUTO so the
  // same reflected declaration can be consumed by raster and compute passes.
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 0U;
  description.m_uiSize              = sizeof(xiiShadowCascadeConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hCascadeMatrices           = builder.WriteBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, description, xiiGALResourceStateFlags::ConstantBuffer);

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

      pConstants->CascadeSplitDepths = data.m_vCascadeSplitDepths;
      pConstants->CascadeWorldRadii  = data.m_vCascadeWorldRadii;
      for (xiiUInt32 i = 0; i < 4; ++i)
      {
        pConstants->CascadeViewProjection[i] = data.m_CascadeViewProjection[i];
      }
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Local Shadow Atlas Allocation //////////
//
// Allocates deterministic atlas descriptors for all shadow-casting local lights.

struct xiiLocalShadowAtlasAllocationData
{
  xiiRenderGraphBufferHandle   m_hLocalShadowAtlasDescriptors;
  xiiLocalShadowAtlasDataArray m_ShadowData;
};

void xiiView::SetupLocalShadowAtlasAllocation(xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiLocalShadowAtlasData);
  description.m_uiSize              = description.m_uiElementByteStride * m_ViewPassResources->m_LightingSystem.GetSettings().m_uiMaxActiveLights;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hLocalShadowAtlasDescriptors = builder.WriteBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, description, xiiGALResourceStateFlags::CopyDestination);

  data.m_ShadowData.PushBackRange(m_ViewPassResources->m_LightingSystem.GetLocalShadowData());

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteLocalShadowAtlasAllocation(const xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LocalShadowAtlasUpload");
  {
    if (!data.m_ShadowData.IsEmpty())
    {
      cmd.UpdateBuffer(context.GetBuffer(data.m_hLocalShadowAtlasDescriptors), 0U,
                       xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_ShadowData.GetData()), data.m_ShadowData.GetCount() * sizeof(xiiLocalShadowAtlasData)));
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Directional Shadow Data //////////
//
// Collects all GPU resources related to directional shadow rendering for the current frame, including cascade matrices, shadow caster lists, and shadow atlases.

struct xiiDirectionalShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hCascadeMatrices;        ///< ConstantBuffer in (cascade view-projection matrices and split depths from this frame's Shadow Cascade Setup pass).
  xiiRenderGraphBufferHandle  m_hShadowCasterCommands;   ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per cascade-per-bin, from this frame's Shadow Caster Build pass).
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas; ///< SRV in (texture atlas for directional shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiActiveCascades = 3U;   ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes and main lighting pass.
};

void xiiView::SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRenderGraphBuilder& builder)
{
  // Persistent directional shadow atlas (D32 float array of 4 slices).
  if (!m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowAtlas)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                            = xiiGALResourceDimension::Texture2DArray;
    description.m_Format                                          = xiiGALResourceFormat::D32Float;
    description.m_Size.width                                      = k_uiDirectionalShadowAtlasWidth;
    description.m_Size.height                                     = k_uiDirectionalShadowAtlasHeight;
    description.m_uiArraySizeOrDepth                              = 4U;
    description.m_uiMipLevels                                     = 1U;
    description.m_BindFlags                                       = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                           = xiiGALResourceUsage::Default;
    m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);

    XII_ASSERT_DEV(m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowAtlas != nullptr, "Failed to create the directional shadow atlas.");
    for (xiiUInt32 uiCascade = 0U; uiCascade < 4U; ++uiCascade)
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
      viewDescription.m_uiFirstArrayOrDepthSlice  = uiCascade;
      viewDescription.m_uiArrayOrDepthSlicesCount = 1U;

      m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade] = m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowAtlas->CreateView(viewDescription);
      XII_ASSERT_DEV(m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade] != nullptr, "Failed to create directional shadow cascade view {}.", uiCascade);
    }
  }

  data.m_hCascadeMatrices        = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hShadowCasterCommands   = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hDirectionalShadowAtlas = builder.ImportTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hDirectionalShadowAtlas = builder.WriteTexture(data.m_hDirectionalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_uiActiveCascades        = m_ViewPassResources->m_ShadowPasses.m_uiActiveCascadeCount;

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DirectionalShadowMaps");
  {
    xiiStringBuilder sb;
    for (xiiUInt32 uiCascade = 0; uiCascade < data.m_uiActiveCascades; ++uiCascade)
    {
      sb.SetFormat("DirectionalShadowCascade{}", uiCascade);
      xiiGALScopedDebugGroup debugGroup(cmd, sb);

      xiiGALTextureView* pCascadeDepthView = m_ViewPassResources->m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade];
      XII_ASSERT_DEV(pCascadeDepthView != nullptr, "Directional shadow cascade view {} is unavailable.", uiCascade);

      // Clear and render only the current array slice. Clearing the default
      // array DSV here would erase every previously rendered cascade.
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(k_uiDirectionalShadowAtlasWidth), static_cast<float>(k_uiDirectionalShadowAtlasHeight), 0.0f, 1.0f});
      cmd.ClearDepthStencilView(pCascadeDepthView, true, false, 0.0f, 0U);

      if (m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline)
      {
        cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline);

        xiiGALDrawIndexedIndirectDescription indexedIndirectDrawDescription;
        indexedIndirectDrawDescription.m_IndexType             = xiiGALValueType::UInt32;
        indexedIndirectDrawDescription.m_pBuffer               = context.GetBuffer(data.m_hShadowCasterCommands);
        const xiiUInt32 uiShadowCommandCapacity                = GetShadowCommandCapacity(GetBlackboard());
        indexedIndirectDrawDescription.m_uiDrawArgumentOffset  = static_cast<xiiUInt64>(uiCascade + 1U) * uiShadowCommandCapacity * sizeof(xiiExtractedDrawCommand);
        indexedIndirectDrawDescription.m_uiDrawArgumentStride  = 20U; // stride per cascade DrawIndexedIndirectArguments (uint index count, uint instance count, uint start index location, int base vertex location, uint start instance location).
        indexedIndirectDrawDescription.m_uiDrawCount           = uiShadowCommandCapacity;
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
  if (!m_ViewPassResources->m_ShadowPasses.m_pLocalShadowAtlas)
  {
    const xiiUInt32                  uiAtlasSize = m_ViewPassResources->m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;
    xiiGALTextureCreationDescription description;
    description.m_Type                                      = xiiGALResourceDimension::Texture2D;
    description.m_Format                                    = xiiGALResourceFormat::D32Float;
    description.m_Size.width                                = uiAtlasSize;
    description.m_Size.height                               = uiAtlasSize;
    description.m_uiMipLevels                               = 1U;
    description.m_BindFlags                                 = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                     = xiiGALResourceUsage::Default;
    m_ViewPassResources->m_ShadowPasses.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hShadowCasterCommands        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, m_ViewPassResources->m_ShadowPasses.m_pLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas            = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_pExtractedData != nullptr ? m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiSpotLightCount                            = CountRenderDataByTypeName(renderData, "xiiSpotLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SpotLightShadows");
  {
    xiiGALTexture* pAtlas = context.GetTexture(data.m_hLocalShadowAtlas);

    // The atlas is persistent and must be initialized every frame even before
    // a local shadow raster pipeline is available. Reversed-Z far depth is
    // zero, so an unpopulated allocation deterministically evaluates as lit.
    cmd.ClearDepthStencilView(pAtlas->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.0f, 0U);

    if (data.m_uiSpotLightCount > 0U && m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline)
    {
      // For each spot light, render into its collision-free atlas tile.
      cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline);
      const float fAtlasSize = static_cast<float>(m_ViewPassResources->m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize);
      cmd.SetViewport({0.0f, 0.0f, fAtlasSize, fAtlasSize, 0.0f, 1.0f});
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands), GetShadowCommandCapacity(GetBlackboard())});
    }
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
  if (!m_ViewPassResources->m_ShadowPasses.m_pLocalShadowAtlas)
  {
    const xiiUInt32                  uiAtlasSize = m_ViewPassResources->m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;
    xiiGALTextureCreationDescription description;
    description.m_Type                                      = xiiGALResourceDimension::Texture2D;
    description.m_Format                                    = xiiGALResourceFormat::D32Float;
    description.m_Size.width                                = uiAtlasSize;
    description.m_Size.height                               = uiAtlasSize;
    description.m_uiMipLevels                               = 1U;
    description.m_BindFlags                                 = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                     = xiiGALResourceUsage::Default;
    m_ViewPassResources->m_ShadowPasses.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
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

  if (data.m_uiPointLightCount == 0U || !m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("PointLightCubeShadows");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pShadowDepthPipeline);

    // Each point light: 6 draw calls placing results into 6 atlas tiles.
    for (xiiUInt32 uiFace = 0; uiFace < data.m_uiPointLightCount * 6U; ++uiFace)
    {
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands), GetShadowCommandCapacity(GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray-Traced Shadow Data //////////
//
// Collects all GPU resources related to ray-traced shadow rendering for the current frame, including raw shadow masks and scene depth.

struct xiiRayTracedShadowData
{
  xiiRenderGraphTextureHandle                 m_hRTRawShadowMask; ///< UAV out (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by Shadow Denoise Pass).
  xiiRenderGraphTextureHandle                 m_hSceneDepth;      ///< SRV in (depth texture from main render pass, used for ray-traced shadow ray generation and occlusion testing).
  xiiRenderGraphTextureHandle                 m_hGBufferNormal;   ///< SRV in (surface normal used to offset the shadow-ray origin).
  xiiRenderGraphBufferHandle                  m_hSceneDependency; ///< BuildASRead dependency on this frame's TLAS build.
  xiiRenderGraphBufferHandle                  m_hMaterialData;    ///< SRV in (canonical material records used by any-hit alpha testing).
  xiiRenderGraphBufferHandle                  m_hGeometryData;    ///< SRV in (geometry addressing records used by any-hit alpha testing).
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

void xiiView::SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1u;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTRawShadowMask   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto&       shadowPasses      = m_ViewPassResources->m_ShadowPasses;
  const auto& lightingPasses    = m_ViewPassResources->m_LightingPasses;
  data.m_bUseHardwareRayTracing = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingShadowResources();
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = shadowPasses.m_pRayTracedShadowPipeline;
    data.m_uiShaderRecordStride = shadowPasses.m_uiRayTracedShadowShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT Shadow Shader Binding Table", shadowPasses.m_pRayTracedShadowShaderBindingTable, shadowPasses.m_pRayTracedShadowShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
  }
  else
  {
    xiiView::EnsureComputePipeline(shadowPasses.m_pRayTracedShadowFallbackPipeline, "Shaders/Pipeline/RTShadowFallback.xiiShader");
  }

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("Ray-Traced Shadows");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", xiiGALShaderType::RayAnyHit);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
      xiiGALUpdateSBTDescription sbtUpdate;
      sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
      sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
      sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
      sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
      sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
      cmd.UpdateSBT(sbtUpdate);

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, uiRenderWidth, uiRenderHeight);
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pRayTracedShadowFallbackPipeline);
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Denoise Data //////////
//
// Collects all GPU resources related to shadow denoising for the current frame, including raw shadow masks and final shadow masks.

struct xiiShadowDenoiseData
{
  xiiRenderGraphTextureHandle m_hRTRawShadowMask;
  xiiRenderGraphTextureHandle m_hPreviousShadowMask;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hRTFinalShadowMask;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

void xiiView::SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_ShadowPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pRayTracedShadowHistory, xiiGALResourceFormat::R8UNormalized, uiWidth, uiHeight))
  {
    resources.m_bRayTracedShadowHistoryValid = false;
  }

  data.m_bHistoryValid       = resources.m_bRayTracedShadowHistoryValid;
  data.m_hRTRawShadowMask    = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth         = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity           = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface    = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousShadowMask = builder.ReadTexture(builder.ImportTexture("RT Shadow Previous History", resources.m_pRayTracedShadowHistory[uiPreviousSlot], resources.m_pRayTracedShadowHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hRTFinalShadowMask  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, resources.m_pRayTracedShadowHistory[uiCurrentSlot], resources.m_pRayTracedShadowHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants          = CreateTemporalDenoiseConstants(builder, "RT Shadow Temporal Denoise Constants");

  xiiView::EnsureComputePipeline(resources.m_pShadowDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiView::ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.88f;
      pConstants->DepthThreshold  = 0.05f;
      pConstants->NormalThreshold = 0.90f;
      pConstants->SpatialWeight   = 0.25f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 2U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pShadowDenoisePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_ShadowPasses.m_bRayTracedShadowHistoryValid = true;
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
  builder.SetPassAllowMerge(false);

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

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_ShadowPasses.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

void xiiView::ExecuteContactShadowData(const xiiContactShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ContactShadows");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_ShadowPasses.m_pContactShadowPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
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

  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_CoarseDrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

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

    if (m_ViewPassResources->m_DepthPasses.m_pDepthPrepassPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_DepthPasses.m_pDepthPrepassPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Pyramid Data //////////
//
// Collects all GPU resources related to hierarchical depth generation for the current frame, including the scene depth source and Hi-Z pyramid target.

struct xiiHiZPyramidData
{
  xiiRenderGraphTextureHandle m_hSceneDepth; ///< ShaderResource in (scene depth texture written by Depth Prepass, used as mip-0 source for Hi-Z generation).
  xiiRenderGraphTextureHandle m_hHiZPyramid; ///< UnorderedAccess out (R32F reversed-Z minimum hierarchy consumed by occlusion and depth-aware effects).
  xiiRenderGraphBufferHandle  m_hConstants;
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

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiHiZBuildConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiHiZBuildConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_DepthPasses.m_pHiZBuildPipeline, "Shaders/Pipeline/HiZBuild.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZPyramid");
  {
    if (m_ViewPassResources->m_DepthPasses.m_pHiZBuildPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_DepthPasses.m_pHiZBuildPipeline);

      xiiGALTexture* pDepth     = context.GetTexture(data.m_hSceneDepth);
      xiiGALTexture* pHiZ       = context.GetTexture(data.m_hHiZPyramid);
      xiiGALBuffer*  pConstants = context.GetBuffer(data.m_hConstants);

      xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> sourceViews;
      xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> destinationViews;
      sourceViews.SetCount(data.m_uiMipLevels);
      destinationViews.SetCount(data.m_uiMipLevels);
      for (xiiUInt32 uiMip = 0U; uiMip < data.m_uiMipLevels; ++uiMip)
      {
        xiiGALTextureViewCreationDescription viewDescription;
        viewDescription.m_ViewType          = xiiGALTextureViewType::ShaderResource;
        viewDescription.m_uiMostDetailedMip = uiMip;
        viewDescription.m_uiMipLevelCount   = 1U;
        sourceViews[uiMip]                  = pHiZ->CreateView(viewDescription);

        viewDescription.m_ViewType = xiiGALTextureViewType::UnorderedAccess;
        destinationViews[uiMip]    = pHiZ->CreateView(viewDescription);
        XII_ASSERT_DEV(sourceViews[uiMip] != nullptr && destinationViews[uiMip] != nullptr, "Failed to create Hi-Z mip views.");
      }

      auto transitionMip = [&cmd, pHiZ](xiiUInt32 uiMip, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState) {
        xiiGALStateTransitionDescription transition;
        transition.m_pResource       = pHiZ;
        transition.m_uiFirstMipLevel = uiMip;
        transition.m_uiMipLevelCount = 1U;
        transition.m_OldState        = oldState;
        transition.m_NewState        = newState;
        cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
      };

      // The reduction intentionally keeps completed mips in ShaderResource while the next mip
      // remains UnorderedAccess. Whole-resource tracking resumes after the loop.
      pHiZ->SetResourceState(xiiGALResourceStateFlags::Unknown);
      cmd.ResolveAndSetConstantBuffer("xiiHiZBuildConstants", pConstants, xiiGALShaderType::Compute);

      xiiUInt32 uiSrcWidth  = GetRenderResolutionWidth();
      xiiUInt32 uiSrcHeight = GetRenderResolutionHeight();

      for (xiiUInt32 uiMip = 0U; uiMip < data.m_uiMipLevels; ++uiMip)
      {
        const bool      bCopyDepth          = uiMip == 0U;
        const xiiUInt32 uiDestinationWidth  = bCopyDepth ? uiSrcWidth : xiiMath::Max(uiSrcWidth >> 1U, 1U);
        const xiiUInt32 uiDestinationHeight = bCopyDepth ? uiSrcHeight : xiiMath::Max(uiSrcHeight >> 1U, 1U);

        {
          xiiGALMapHelper<xiiHiZBuildConstants> constants(cmd, pConstants, xiiGALMapType::Write, xiiGALMapFlags::Discard);
          constants->SrcSize = xiiVec2U32(uiSrcWidth, uiSrcHeight);
          constants->DstSize = xiiVec2U32(uiDestinationWidth, uiDestinationHeight);
          constants->Reduce  = bCopyDepth ? 0U : 1U;
        }
        xiiGALStateTransitionDescription constantsTransition;
        constantsTransition.m_pResource       = pConstants;
        constantsTransition.m_OldState        = xiiGALResourceStateFlags::CopyDestination;
        constantsTransition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
        constantsTransition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        cmd.TransitionResourceStates(xiiMakeArrayPtr(&constantsTransition, 1U));

        xiiGALTextureView* pSourceView = bCopyDepth ? pDepth->GetDefaultView(xiiGALTextureViewType::ShaderResource).Borrow() : sourceViews[uiMip - 1U].Borrow();
        cmd.ResolveAndSetShaderResourceTextureView("g_DepthSrc", pSourceView, xiiGALShaderType::Compute);
        cmd.ResolveAndSetUnorderedAccessTextureView("g_HiZOut", destinationViews[uiMip].Borrow(), xiiGALShaderType::Compute);
        cmd.CommitShaderResources(xiiGALStateTransitionMode::None).AssertSuccess();
        cmd.DispatchCompute({(uiDestinationWidth + 7U) / 8U, (uiDestinationHeight + 7U) / 8U, 1U});

        if (uiMip + 1U < data.m_uiMipLevels)
          transitionMip(uiMip, xiiGALResourceStateFlags::UnorderedAccess, xiiGALResourceStateFlags::ShaderResource);

        uiSrcWidth  = uiDestinationWidth;
        uiSrcHeight = uiDestinationHeight;
      }

      for (xiiUInt32 uiMip = 0U; uiMip + 1U < data.m_uiMipLevels; ++uiMip)
        transitionMip(uiMip, xiiGALResourceStateFlags::ShaderResource, xiiGALResourceStateFlags::UnorderedAccess);
      pHiZ->SetResourceState(xiiGALResourceStateFlags::UnorderedAccess);
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Occlusion Culling Data //////////
//
// Collects all GPU resources related to Hi-Z occlusion culling for the current frame, including the Hi-Z pyramid input, candidate instance list, and surviving output list.

struct xiiHiZOcclusionCullData
{
  xiiRenderGraphTextureHandle m_hHiZPyramid;         ///< ShaderResource in (Hi-Z pyramid generated by this frame's Hi-Z Pyramid pass).
  xiiRenderGraphBufferHandle  m_hVisibleCandidates;  ///< ShaderResource in (visible instance candidate list generated by this frame's Frustum Culling pass).
  xiiRenderGraphBufferHandle  m_hSurvivingInstances; ///< UnorderedAccess out (instance list surviving Hi-Z occlusion culling, consumed by later depth/lighting passes).
  xiiRenderGraphBufferHandle  m_hInstanceBounds;     ///< ShaderResource in (instance bounds buffer for occlusion testing).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiHiZOcclusionConstants    m_Constants       = {};
  xiiUInt32                   m_uiInstanceCount = 0U; ///< Maximum number of candidates to process.
};

void xiiView::SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHiZPyramid        = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  const bool                      bHasExtractedMeshCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);
  description.m_uiElementByteStride = sizeof(xiiUInt32);
  description.m_uiSize              = sizeof(xiiUInt32) * (xiiMath::Max(1U, data.m_uiInstanceCount) + 1U);
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hSurvivingInstances        = builder.WriteBuffer(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiHiZOcclusionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiHiZOcclusionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_Constants.ViewProjectionMatrix = GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_Constants.HiZSize              = xiiVec2U32(GetRenderResolutionWidth(), GetRenderResolutionHeight());
  data.m_Constants.HiZMipCount          = 1U;
  for (xiiUInt32 uiWidth = GetRenderResolutionWidth(), uiHeight = GetRenderResolutionHeight(); uiWidth > 1U || uiHeight > 1U;)
  {
    uiWidth  = xiiMath::Max(uiWidth >> 1U, 1U);
    uiHeight = xiiMath::Max(uiHeight >> 1U, 1U);
    ++data.m_Constants.HiZMipCount;
  }
  data.m_Constants.CandidateCapacity = data.m_uiInstanceCount;
  data.m_Constants.DepthBias         = 0.0001f;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_DepthPasses.m_pHiZOcclusionCullPipeline, "Shaders/Pipeline/HiZOcclusionCulling.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZOcclusionCull");
  {
    const xiiUInt32 uiZero = 0U;
    cmd.UpdateBuffer(context.GetBuffer(data.m_hSurvivingInstances), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    if (data.m_uiInstanceCount > 0U && m_ViewPassResources->m_DepthPasses.m_pHiZOcclusionCullPipeline)
    {
      {
        xiiGALMapHelper<xiiHiZOcclusionConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *constants = data.m_Constants;
      }

      cmd.SetPipelineState(m_ViewPassResources->m_DepthPasses.m_pHiZOcclusionCullPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiHiZOcclusionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_HiZPyramid", context.GetTexture(data.m_hHiZPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Candidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_SurvivingOut", context.GetBuffer(data.m_hSurvivingInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
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

  xiiRenderGraphTextureHandle m_hSceneDepth;     ///< ShaderResource in (current reversed-Z scene depth).
  xiiRenderGraphTextureHandle m_hVelocityBuffer; ///< RenderTarget out (current-to-previous NDC velocity).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiMotionVectorConstants    m_Constants             = {};
  xiiMat4                     m_CurrentViewProjection = xiiMat4::MakeIdentity();
  xiiVec2                     m_vCurrentJitter        = xiiVec2::MakeZero();
};

void xiiView::SetupMotionVectors(xiiMotionVectorsData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityBuffer    = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiMotionVectorConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Motion Vector Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  auto& depthPasses                             = m_ViewPassResources->m_DepthPasses;
  data.m_CurrentViewProjection                  = GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_vCurrentJitter                         = xiiVec2::MakeZero();
  data.m_Constants.PreviousViewProjectionMatrix = depthPasses.m_bMotionHistoryValid ? depthPasses.m_PreviousViewProjectionMatrix : data.m_CurrentViewProjection;
  data.m_Constants.CurrentJitter                = data.m_vCurrentJitter;
  data.m_Constants.PreviousJitter               = depthPasses.m_bMotionHistoryValid ? depthPasses.m_vPreviousJitter : data.m_vCurrentJitter;

  builder.SetPassAllowMerge(false);
  builder.SetPassRenderPassManaged(true);
}

void xiiView::ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList&                        cmd = context.GetCommandList();
  xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline;
  if (data.m_hSceneDepth.IsValid() && data.m_hVelocityBuffer.IsValid() && data.m_hConstants.IsValid() && context.GetRenderPass() != nullptr)
  {
    pPipeline = xiiView::EnsureGraphicsPipeline(m_ViewPassResources->m_DepthPasses.m_pMotionVectorPipeline, "Shaders/Pipeline/MotionVectors.xiiShader", context.GetRenderPass(), context.GetSubpassIndex());
  }

  cmd.BeginDebugGroup("MotionVectors");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hVelocityBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(GetRenderResolutionWidth()), static_cast<float>(GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (pPipeline != nullptr)
    {
      {
        xiiGALMapHelper<xiiMotionVectorConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *constants = data.m_Constants;
      }

      cmd.SetPipelineState(pPipeline.Borrow());
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Pixel);
      cmd.ResolveAndSetConstantBuffer("xiiMotionVectorConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.Draw({3U, 1U, 0U, 0U});

      auto& depthPasses                          = m_ViewPassResources->m_DepthPasses;
      depthPasses.m_PreviousViewProjectionMatrix = data.m_CurrentViewProjection;
      depthPasses.m_vPreviousJitter              = data.m_vCurrentJitter;
      depthPasses.m_bMotionHistoryValid          = true;
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
  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (reversed-Z depth used to select the foreground surface).
  xiiRenderGraphTextureHandle m_hVelocityDilated; ///< UnorderedAccess out (depth-aware velocity consumed by temporal passes).
};

void xiiView::SetupVelocityDilation(xiiVelocityDilationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hVelocityInput = builder.ReadTexture(xiiRGBlackboardKeys::k_VelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityDilated   = builder.WriteTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_DepthPasses.m_pVelocityDilationPipeline, "Shaders/Pipeline/VelocityDilation.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VelocityDilation");
  {
    if (m_ViewPassResources->m_DepthPasses.m_pVelocityDilationPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_DepthPasses.m_pVelocityDilationPipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_VelocityInput", context.GetTexture(data.m_hVelocityInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_VelocityOutput", context.GetTexture(data.m_hVelocityDilated)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
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

    if (m_ViewPassResources->m_GBufferPasses.m_pGBufferPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_GBufferPasses.m_pGBufferPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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

    if (m_ViewPassResources->m_GBufferPasses.m_pNormalRoughnessPipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_GBufferPasses.m_pNormalRoughnessPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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
  builder.SetPassAllowMerge(false);

  XII_VERIFY(xiiLightingManager::EnsureBRDFLUTResources().Succeeded(), "The shared BRDF LUT resource is unavailable.");
  xiiSharedPtr<xiiGALTexture> pBRDFLUT = xiiLightingManager::GetBRDFLUT();
  data.m_bNeedsGeneration              = xiiLightingManager::IsBRDFLUTGenerationPending();

  data.m_hBRDFLut = builder.ImportTexture(xiiRGBlackboardKeys::k_BRDFLut, pBRDFLUT, pBRDFLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hBRDFLut = builder.WriteTexture(data.m_hBRDFLut, xiiGALResourceStateFlags::UnorderedAccess);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pBRDFLutPipeline, "Shaders/Pipeline/BRDFLUTGenerate.xiiShader");
}

void xiiView::ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("BRDFLUTGenerate");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pBRDFLutPipeline);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BRDFLutOut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({32U, 32U, 1U});
    xiiLightingManager::MarkBRDFLUTGenerated();
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Transmittance Data //////////
//
// Collects all GPU resources related to atmosphere transmittance LUT generation, persisted across frames.

namespace
{
  static xiiAtmosphereConstants MakeAtmosphereConstants(xiiAtmosphereLUTHandle hCache)
  {
    const xiiAtmosphereSettings& settings = xiiAtmosphereManager::GetConfiguration(hCache);

    xiiAtmosphereConstants constants            = {};
    constants.PlanetAtmosphereRadiiScaleHeights = xiiVec4(settings.m_fPlanetRadiusKm, settings.m_fAtmosphereRadiusKm, settings.m_fRayleighScaleHeightKm, settings.m_fMieScaleHeightKm);
    constants.RayleighScattering                = xiiVec4(settings.m_vRayleighScattering, 0.0f);
    constants.MieScatteringAndPhase             = xiiVec4(settings.m_vMieScattering, settings.m_fMiePhaseG);
    constants.MieAbsorption                     = xiiVec4(settings.m_vMieAbsorption, 0.0f);
    constants.OzoneAbsorption                   = xiiVec4(settings.m_vOzoneAbsorption, 0.0f);
    constants.PlanetUpAndGroundAltitudeMeters   = xiiVec4(settings.m_vPlanetUpDirection.GetNormalized(), settings.m_fGroundAltitudeMeters);
    constants.SampleCounts                      = xiiVec4U32(settings.m_uiTransmittanceIntegrationSteps, settings.m_uiMultiScatterSqrtSamples, 0U, 0U);
    return constants;
  }

  static xiiRenderGraphBufferHandle CreateAtmosphereConstantsBuffer(xiiRenderGraphBuilder& builder, xiiStringView sName)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = sizeof(xiiAtmosphereConstants);
    description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
    description.m_Usage          = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    return builder.WriteBuffer(sName, description, xiiGALResourceStateFlags::ConstantBuffer);
  }

  static void UploadAtmosphereConstants(xiiGALCommandList& cmd, xiiGALBuffer* pBuffer, const xiiAtmosphereConstants& constants)
  {
    xiiGALMapHelper<xiiAtmosphereConstants> mappedConstants(cmd, pBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mappedConstants = constants;
  }
} // namespace

struct xiiAtmosphereTransmittanceData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< Imported persistent atmosphere transmittance LUT texture.
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters used by the integration.
  xiiAtmosphereConstants      m_Constants;
  xiiAtmosphereLUTHandle      m_hCache;
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch transmittance LUT generation.
};

void xiiView::SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hCache = m_ViewPassResources->m_LightingPrepPasses.m_hAtmosphereLUT;
  if (!data.m_hCache.IsValid())
  {
    data.m_hCache = xiiAtmosphereManager::GetDefaultLUTHandle();
  }
  XII_VERIFY(xiiAtmosphereManager::EnsureGpuResources(data.m_hCache).Succeeded(), "Atmosphere LUT resources are unavailable.");
  data.m_bNeedsGeneration                       = xiiAtmosphereManager::IsGenerationPending(data.m_hCache);
  xiiSharedPtr<xiiGALTexture> pTransmittanceLUT = xiiAtmosphereManager::GetTransmittanceLUT(data.m_hCache);
  data.m_hTransmittanceLUT                      = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, pTransmittanceLUT, pTransmittanceLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hTransmittanceLUT = builder.WriteTexture(data.m_hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants        = CreateAtmosphereConstantsBuffer(builder, "AtmosphereTransmittanceConstants");
    data.m_Constants         = MakeAtmosphereConstants(data.m_hCache);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pAtmTransmittancePipeline, "Shaders/Pipeline/AtmosphereTransmittance.xiiShader");
}

void xiiView::ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereTransmittanceLUT");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pAtmTransmittancePipeline);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TransmittanceLUT", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
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

  xiiRenderGraphTextureHandle m_hMultiScatterLUT;  ///< Imported persistent atmosphere multi-scatter LUT texture.
  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters used by the integration.
  xiiAtmosphereConstants      m_Constants;
  xiiAtmosphereLUTHandle      m_hCache;
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch multi-scatter LUT generation.
};

void xiiView::SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hCache = m_ViewPassResources->m_LightingPrepPasses.m_hAtmosphereLUT;
  if (!data.m_hCache.IsValid())
  {
    data.m_hCache = xiiAtmosphereManager::GetDefaultLUTHandle();
  }
  XII_VERIFY(xiiAtmosphereManager::EnsureGpuResources(data.m_hCache).Succeeded(), "Atmosphere LUT resources are unavailable.");
  data.m_bNeedsGeneration                      = xiiAtmosphereManager::IsGenerationPending(data.m_hCache);
  data.m_hTransmittanceLUT                     = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  xiiSharedPtr<xiiGALTexture> pMultiScatterLUT = xiiAtmosphereManager::GetMultiScatterLUT(data.m_hCache);
  data.m_hMultiScatterLUT                      = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, pMultiScatterLUT, pMultiScatterLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hMultiScatterLUT = builder.WriteTexture(data.m_hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants       = CreateAtmosphereConstantsBuffer(builder, "AtmosphereMultiScatterConstants");
    data.m_Constants        = MakeAtmosphereConstants(data.m_hCache);
  }

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pAtmMultiScatterPipeline, "Shaders/Pipeline/AtmosphereMultiScatter.xiiShader");
}

void xiiView::ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereMultiScatterLUT");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pAtmMultiScatterPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TransmittanceLUT", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_MultiScatterLUT", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({4U, 4U, 1U});
    xiiAtmosphereManager::MarkLUTsGenerated(data.m_hCache);
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
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters.
  xiiAtmosphereConstants      m_Constants;
};

void xiiView::SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

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
  data.m_hConstants         = CreateAtmosphereConstantsBuffer(builder, "SkyRadianceConstants");
  data.m_Constants          = MakeAtmosphereConstants(m_ViewPassResources->m_LightingPrepPasses.m_hAtmosphereLUT);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pSkyIrradiancePipeline, "Shaders/Pipeline/SkyRadiance.xiiShader");
}

void xiiView::ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SkyIrradianceConvolution");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pSkyIrradiancePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SkyOut", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
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

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pFroxelFogInitPipeline, "Shaders/Pipeline/FroxelScatteringInitialize.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogInitialization");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pFroxelFogInitPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
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
  xiiRenderGraphTextureHandle m_hProbeIrradianceAtlas;
  xiiRenderGraphTextureHandle m_hProbeDistanceAtlas;
  xiiRenderGraphBufferHandle  m_hProbeStates;
  xiiRenderGraphBufferHandle  m_hProbeConstants;
};

void xiiView::SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal        = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeIrradianceAtlas = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIProbeIrradianceAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeDistanceAtlas   = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIProbeDistanceAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeStates          = builder.ReadBuffer(xiiRGBlackboardKeys::k_DDGIProbeStates, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeConstants       = builder.ReadBuffer(xiiRGBlackboardKeys::k_DDGIProbeConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDDGIIrradiance    = builder.WriteTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pDDGIProbePipeline, "Shaders/Pipeline/DDGIFinalGather.xiiShader");
}

void xiiView::ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DDGIProbeSampling");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pDDGIProbePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants", context.GetBuffer(data.m_hProbeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIIrradianceAtlas", context.GetTexture(data.m_hProbeIrradianceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIDistanceAtlas", context.GetTexture(data.m_hProbeDistanceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_DDGIProbeStates", context.GetBuffer(data.m_hProbeStates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DDGIOut", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

struct xiiSparseVoxelRadianceGatherData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphBufferHandle  m_hRadiancePool;
  xiiRenderGraphBufferHandle  m_hPageTable;
  xiiRenderGraphBufferHandle  m_hLevelData;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiRenderGraphTextureHandle m_hIrradiance;
};

void xiiView::SetupSparseVoxelRadianceGather(xiiSparseVoxelRadianceGatherData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRadiancePool  = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelRadiancePool, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPageTable     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelPageTable, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLevelData     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelLevelData, xiiGALResourceStateFlags::ShaderResource);
  data.m_hConstants     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hIrradiance        = builder.WriteTexture(xiiRGBlackboardKeys::k_SparseVoxelIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPrepPasses.m_pSparseVoxelGatherPipeline, "Shaders/Pipeline/SparseVoxelRadianceGather.xiiShader");
}

void xiiView::ExecuteSparseVoxelRadianceGather(const xiiSparseVoxelRadianceGatherData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SparseVoxelRadianceGather");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pSparseVoxelGatherPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSparseVoxelRadianceConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelPageTable", context.GetBuffer(data.m_hPageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelLevels", context.GetBuffer(data.m_hLevelData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelRadiancePool", context.GetBuffer(data.m_hRadiancePool)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SparseVoxelIrradianceOut", context.GetTexture(data.m_hIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
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
  xiiRenderGraphTextureHandle                 m_hSceneDepth;          ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                 m_hNormalRoughness;     ///< ShaderResource in (normal/roughness buffer).
  xiiRenderGraphTextureHandle                 m_hRawAmbientOcclusion; ///< UnorderedAccess out (raw ambient occlusion result).
  xiiRenderGraphBufferHandle                  m_hSceneDependency;
  xiiRenderGraphBufferHandle                  m_hMaterialData;
  xiiRenderGraphBufferHandle                  m_hGeometryData;
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
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

  auto&       lightingPrepPasses = m_ViewPassResources->m_LightingPrepPasses;
  const auto& lightingPasses     = m_ViewPassResources->m_LightingPasses;
  data.m_bUseHardwareRayTracing  = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingAmbientOcclusionResources();
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = lightingPrepPasses.m_pRTAOPipeline;
    data.m_uiShaderRecordStride = lightingPrepPasses.m_uiRTAOShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT AO Shader Binding Table", lightingPrepPasses.m_pRTAOShaderBindingTable, lightingPrepPasses.m_pRTAOShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
    builder.SetPassAllowMerge(false);
  }
  else
  {
    xiiView::EnsureComputePipeline(lightingPrepPasses.m_pGTAOFallbackPipeline, "Shaders/Pipeline/GTAO.xiiShader");
  }
}

void xiiView::ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusion");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", xiiGALShaderType::RayAnyHit);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
      xiiGALUpdateSBTDescription sbtUpdate;
      sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
      sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
      sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
      sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
      sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
      cmd.UpdateSBT(sbtUpdate);

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, GetRenderResolutionWidth(), GetRenderResolutionHeight());
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pGTAOFallbackPipeline);
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Denoise Data //////////
//
// Collects all GPU resources related to denoising ground-truth ambient occlusion.

struct xiiGroundTruthAmbientOcclusionDenoiseData
{
  xiiRenderGraphTextureHandle m_hRawAmbientOcclusion;      ///< ShaderResource in (raw ambient visibility).
  xiiRenderGraphTextureHandle m_hPreviousAmbientOcclusion; ///< ShaderResource in (previous temporally accumulated visibility).
  xiiRenderGraphTextureHandle m_hSceneDepth;               ///< ShaderResource in (current reversed-Z depth).
  xiiRenderGraphTextureHandle m_hGBufferNormal;            ///< ShaderResource in (current octahedral normal).
  xiiRenderGraphTextureHandle m_hVelocity;                 ///< ShaderResource in (current-to-previous NDC velocity).
  xiiRenderGraphTextureHandle m_hPreviousSurface;          ///< ShaderResource in (previous normal and linear depth).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion;   ///< UnorderedAccess out (persistent denoised visibility).
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

void xiiView::SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_LightingPrepPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pAmbientOcclusionHistory, xiiGALResourceFormat::R8UNormalized, uiWidth, uiHeight))
  {
    resources.m_bAmbientOcclusionHistoryValid = false;
  }

  data.m_bHistoryValid             = resources.m_bAmbientOcclusionHistoryValid;
  data.m_hRawAmbientOcclusion      = builder.ReadTexture(xiiRGBlackboardKeys::k_RawAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth               = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal            = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                 = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface          = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousAmbientOcclusion = builder.ReadTexture(builder.ImportTexture("AO Previous History", resources.m_pAmbientOcclusionHistory[uiPreviousSlot], resources.m_pAmbientOcclusionHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion   = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_StableAOTexture, resources.m_pAmbientOcclusionHistory[uiCurrentSlot], resources.m_pAmbientOcclusionHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hConstants = CreateTemporalDenoiseConstants(builder, "AO Temporal Denoise Constants");

  xiiView::EnsureComputePipeline(resources.m_pAOTemporalDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiView::ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusionDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.92f;
      pConstants->DepthThreshold  = 0.08f;
      pConstants->NormalThreshold = 0.82f;
      pConstants->SpatialWeight   = 0.35f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 0U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPrepPasses.m_pAOTemporalDenoisePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPrepPasses.m_bAmbientOcclusionHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Direct Lighting Data //////////
//
// Collects all GPU resources related to deferred direct lighting.

struct xiiReSTIRDITemporalData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;
  xiiRenderGraphTextureHandle m_hPreviousReservoir;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiRenderGraphTextureHandle m_hTemporalReservoir;
  bool                        m_bHistoryValid = false;
};

void xiiView::SetupReSTIRDITemporal(xiiReSTIRDITemporalData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight = GetRenderResolutionHeight();

  auto HistoryMatchesResolution = [uiWidth, uiHeight](const xiiSharedPtr<xiiGALTexture>& pTexture) {
    return pTexture != nullptr && pTexture->GetDescription().m_Size.width == uiWidth && pTexture->GetDescription().m_Size.height == uiHeight;
  };

  auto& lightingPasses = m_ViewPassResources->m_LightingPasses;
  bool  bHistoryReset  = false;

  if (!HistoryMatchesResolution(lightingPasses.m_pDirectReservoirHistory[0]) ||
      !HistoryMatchesResolution(lightingPasses.m_pDirectReservoirHistory[1]))
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA32UInt;
    description.m_Size.width  = uiWidth;
    description.m_Size.height = uiHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    for (xiiUInt32 uiSlot = 0U; uiSlot < 2U; ++uiSlot)
    {
      lightingPasses.m_pDirectReservoirHistory[uiSlot] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    }
    bHistoryReset = true;
  }

  if (!HistoryMatchesResolution(lightingPasses.m_pDirectSurfaceHistory[0]) ||
      !HistoryMatchesResolution(lightingPasses.m_pDirectSurfaceHistory[1]))
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = uiWidth;
    description.m_Size.height = uiHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    for (xiiUInt32 uiSlot = 0U; uiSlot < 2U; ++uiSlot)
    {
      lightingPasses.m_pDirectSurfaceHistory[uiSlot] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    }
    bHistoryReset = true;
  }

  if (bHistoryReset)
    lightingPasses.m_bDirectReservoirHistoryValid = false;

  data.m_bHistoryValid           = lightingPasses.m_bDirectReservoirHistoryValid;
  const xiiUInt32 uiPreviousSlot = (lightingPasses.m_uiFrameIndex + 1U) & 1U;

  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity          = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer   = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer  = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReservoir = builder.ReadTexture(builder.ImportTexture("ReSTIRDIReservoirPrevious", lightingPasses.m_pDirectReservoirHistory[uiPreviousSlot], lightingPasses.m_pDirectReservoirHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface   = builder.ReadTexture(builder.ImportTexture("ReSTIRDISurfacePrevious", lightingPasses.m_pDirectSurfaceHistory[uiPreviousSlot], lightingPasses.m_pDirectSurfaceHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRDIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hConstants                     = builder.WriteBuffer("xiiReSTIRDIConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA32UInt;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hTemporalReservoir = builder.WriteTexture("ReSTIRDITemporalReservoir", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pReSTIRDITemporalPipeline, "Shaders/Pipeline/ReSTIRDITemporal.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteReSTIRDITemporal(const xiiReSTIRDITemporalData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDITemporal");
  {
    {
      xiiGALMapHelper<xiiReSTIRDIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding     = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pReSTIRDITemporalPipeline);
    m_ViewPassResources->m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources->m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRDIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousReservoir", context.GetTexture(data.m_hPreviousReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalReservoir", context.GetTexture(data.m_hTemporalReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

struct xiiReSTIRDISpatialData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hTemporalReservoir;
  xiiRenderGraphTextureHandle m_hReservoir;
  xiiRenderGraphTextureHandle m_hReservoirSurface;
};

void xiiView::SetupReSTIRDISpatial(xiiReSTIRDISpatialData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  auto&           lightingPasses = m_ViewPassResources->m_LightingPasses;

  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalReservoir = builder.ReadTexture("ReSTIRDITemporalReservoir", xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoir         = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DirectLightReservoir, lightingPasses.m_pDirectReservoirHistory[uiCurrentSlot], lightingPasses.m_pDirectReservoirHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirSurface  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DirectLightReservoirSurface, lightingPasses.m_pDirectSurfaceHistory[uiCurrentSlot], lightingPasses.m_pDirectSurfaceHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pReSTIRDISpatialPipeline, "Shaders/Pipeline/ReSTIRDISpatial.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteReSTIRDISpatial(const xiiReSTIRDISpatialData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDISpatial");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pReSTIRDISpatialPipeline);
    m_ViewPassResources->m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources->m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalReservoir", context.GetTexture(data.m_hTemporalReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirOut", context.GetTexture(data.m_hReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SurfaceOut", context.GetTexture(data.m_hReservoirSurface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPasses.m_bDirectReservoirHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

struct xiiDeferredDirectLightingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle       m_hGBufferAlbedo;            ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle       m_hGBufferNormal;            ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle       m_hGBufferMaterial;          ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle       m_hSceneDepth;               ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle       m_hStableAmbientOcclusion;   ///< ShaderResource in (stable ambient occlusion).
  xiiRenderGraphTextureHandle       m_hRayTracedFinalShadowMask; ///< ShaderResource in (denoised ray traced shadows).
  xiiRenderGraphTextureHandle       m_hContactShadowTerm;        ///< ShaderResource in (contact shadow mask).
  xiiRenderGraphBufferHandle        m_hShadowCascadeConstants;   ///< ConstantBuffer in (directional cascade matrices, split depths, and active count).
  xiiRenderGraphTextureHandle       m_hDirectionalShadowAtlas;   ///< ShaderResource in (directional shadow atlas).
  xiiRenderGraphTextureHandle       m_hLocalShadowAtlas;         ///< ShaderResource in (local light shadow atlas).
  xiiRenderGraphBufferHandle        m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle        m_hLightGridBuffer;      ///< ShaderResource in (cluster light grid).
  xiiRenderGraphBufferHandle        m_hLightIndexBuffer;     ///< ShaderResource in (cluster light indices).
  xiiRenderGraphTextureHandle       m_hDirectLightReservoir; ///< ShaderResource in (spatially reused ReSTIR DI sample).
  xiiRenderGraphTextureHandle       m_hReservoirSurface;     ///< ShaderResource in (history state transition and dependency).
  xiiRenderGraphBufferHandle        m_hCloudShadowConstants; ///< ConstantBuffer in (world-space cloud shadow projection).
  xiiRenderGraphBufferHandle        m_hVirtualShadowPageTable;
  xiiRenderGraphTextureHandle       m_hVirtualShadowAtlas;
  xiiRenderGraphBufferHandle        m_hVirtualShadowSamplingConstants;
  xiiRenderGraphTextureHandle       m_hDirectLightingBuffer; ///< UnorderedAccess out (direct lighting HDR buffer).
  xiiCloudShadowConstants           m_CloudShadowConstants;
  xiiVirtualShadowSamplingConstants m_VirtualShadowConstants;
};

void xiiView::SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hGBufferAlbedo               = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal               = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial             = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                  = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion      = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalShadowMask    = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hContactShadowTerm           = builder.ReadTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, xiiGALResourceStateFlags::ShaderResource);
  data.m_hShadowCascadeConstants      = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hDirectionalShadowAtlas      = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer             = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer            = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDirectLightReservoir        = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoir, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoirSurface            = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoirSurface, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVirtualShadowPageTable      = builder.ReadBuffer(xiiRGBlackboardKeys::k_VirtualShadowPageTable, xiiGALResourceStateFlags::ShaderResource);

  data.m_hVirtualShadowAtlas = builder.ReadTexture(xiiRGBlackboardKeys::k_VirtualShadowAtlas, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiCloudShadowConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hCloudShadowConstants          = builder.WriteBuffer(xiiRGBlackboardKeys::k_CloudShadowConstants, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  constantsDescription.m_uiSize                                 = sizeof(xiiVirtualShadowSamplingConstants);
  data.m_hVirtualShadowSamplingConstants                        = builder.WriteBuffer(xiiRGBlackboardKeys::k_VirtualShadowSamplingConstants, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
  const xiiVirtualShadowMapSettings& virtualShadowSettings      = xiiVirtualShadowMapManager::GetConfiguration();
  const xiiVirtualShadowMapStats     virtualShadowStats         = xiiVirtualShadowMapManager::GetStats();
  data.m_VirtualShadowConstants.VirtualShadowPageTableBaseIndex = 0U;
  data.m_VirtualShadowConstants.VirtualShadowPageTableCapacity  = 0U;
  XII_IGNORE_UNUSED(GetBlackboard().TryGet(xiiRGBlackboardKeys::k_VirtualShadowTableBaseIndex, data.m_VirtualShadowConstants.VirtualShadowPageTableBaseIndex));
  XII_IGNORE_UNUSED(GetBlackboard().TryGet(xiiRGBlackboardKeys::k_VirtualShadowTableCapacity, data.m_VirtualShadowConstants.VirtualShadowPageTableCapacity));
  data.m_VirtualShadowConstants.VirtualShadowResolution          = virtualShadowSettings.m_uiVirtualResolution;
  data.m_VirtualShadowConstants.VirtualShadowPageSize            = virtualShadowSettings.m_uiPageSize;
  data.m_VirtualShadowConstants.VirtualShadowPhysicalAtlasWidth  = virtualShadowStats.m_uiPhysicalAtlasWidth;
  data.m_VirtualShadowConstants.VirtualShadowPhysicalAtlasHeight = virtualShadowStats.m_uiPhysicalAtlasHeight;
  data.m_VirtualShadowConstants.VirtualShadowDirectionalLightId  = 0U;
  data.m_VirtualShadowConstants.VirtualShadowEnabled             = 0U;
  if (m_pExtractedData != nullptr)
  {
    const xiiDirectionalLightRenderData* pMainDirectional = SelectMainDirectionalLight(m_pExtractedData->GetAllRenderData());
    if (pMainDirectional != nullptr && pMainDirectional->m_bCastShadows)
    {
      data.m_VirtualShadowConstants.VirtualShadowDirectionalLightId = static_cast<xiiUInt32>(pMainDirectional->m_uiSortingKey) & 0x00FFFFFFU;
      data.m_VirtualShadowConstants.VirtualShadowEnabled            = 1U;
    }
  }

  const auto& cloudState                                 = m_ViewPassResources->m_LightingPasses.m_CloudShadowState;
  data.m_CloudShadowConstants.LayerOriginAndInvScale     = cloudState.m_vLayerOriginAndInvScale;
  data.m_CloudShadowConstants.ProjectionAxisUAndDetail   = cloudState.m_vProjectionAxisUAndDetail;
  data.m_CloudShadowConstants.ProjectionAxisVAndCoverage = cloudState.m_vProjectionAxisVAndCoverage;
  data.m_CloudShadowConstants.LayerNormalAndOpticalDepth = cloudState.m_vLayerNormalAndOpticalDepth;
  data.m_CloudShadowConstants.WindStrengthAndEnabled     = cloudState.m_vWindStrengthAndEnabled;

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = GetRenderResolutionWidth();
  description.m_Size.height    = GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hDirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pDirectLightingPipeline, "Shaders/Pipeline/DirectLighting.xiiShader");
}

void xiiView::ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredDirectLighting");
  {
    {
      xiiGALMapHelper<xiiCloudShadowConstants> pConstants(cmd, context.GetBuffer(data.m_hCloudShadowConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_CloudShadowConstants;
    }
    {
      xiiGALMapHelper<xiiVirtualShadowSamplingConstants> pConstants(cmd, context.GetBuffer(data.m_hVirtualShadowSamplingConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_VirtualShadowConstants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pDirectLightingPipeline);
    m_ViewPassResources->m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources->m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RTShadow", context.GetTexture(data.m_hRayTracedFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ContactShadow", context.GetTexture(data.m_hContactShadowTerm)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hShadowCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiCloudShadowConstants", context.GetBuffer(data.m_hCloudShadowConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVirtualShadowSamplingConstants", context.GetBuffer(data.m_hVirtualShadowSamplingConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VirtualShadowAtlas", context.GetTexture(data.m_hVirtualShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VirtualShadowPageTable", context.GetBuffer(data.m_hVirtualShadowPageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_LocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LocalShadowAtlasDescs", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DirectLightReservoir", context.GetTexture(data.m_hDirectLightReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
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
  xiiRenderGraphTextureHandle                                            m_hGBufferAlbedo;           ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle                                            m_hGBufferNormal;           ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                                            m_hGBufferMaterial;         ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle                                            m_hSceneDepth;              ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                                            m_hStableAmbientOcclusion;  ///< ShaderResource in (stable ambient occlusion).
  xiiRenderGraphTextureHandle                                            m_hBRDFLut;                 ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle                                            m_hDDGIIrradiance;          ///< ShaderResource in (DDGI irradiance texture).
  xiiRenderGraphTextureHandle                                            m_hSparseVoxelIrradiance;   ///< ShaderResource in (far-field sparse voxel irradiance).
  xiiRenderGraphTextureHandle                                            m_hSkyRadiance;             ///< ShaderResource in (sky radiance texture).
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeData;     ///< ShaderResource in (active local reflection probes).
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeClusters; ///< ShaderResource in (two selected probes per cluster).
  xiiHybridArray<xiiRenderGraphTextureHandle, XII_MAX_REFLECTION_PROBES> m_hReflectionProbeTextures;
  xiiRenderGraphTextureHandle                                            m_hFallbackReflectionProbe;
  xiiRenderGraphTextureHandle                                            m_hIndirectLightingBuffer; ///< UnorderedAccess out (indirect lighting HDR buffer).
  xiiRenderGraphTextureHandle                                            m_hEnvironmentSpecular;    ///< UnorderedAccess out (probe/sky specular contribution for tier replacement).
};

void xiiView::SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hGBufferAlbedo           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth              = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion  = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut                 = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDDGIIrradiance          = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSparseVoxelIrradiance   = builder.ReadTexture(xiiRGBlackboardKeys::k_SparseVoxelIrradiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSkyRadiance             = builder.ReadTexture(xiiRGBlackboardKeys::k_SkyRadiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReflectionProbeData     = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReflectionProbeClusters = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, xiiGALResourceStateFlags::ShaderResource);

  auto& reflectionResources = m_ViewPassResources->m_LightingPrepPasses;
  XII_ASSERT_DEV(reflectionResources.m_pFallbackReflectionProbeTexture != nullptr, "Reflection probe selection must initialize the fallback cubemap before indirect lighting setup.");
  data.m_hFallbackReflectionProbe = builder.ReadTexture(
    builder.ImportTexture("ReflectionProbeFallback", reflectionResources.m_pFallbackReflectionProbeTexture, reflectionResources.m_pFallbackReflectionProbeTexture->GetResourceState()),
    xiiGALResourceStateFlags::ShaderResource);

  data.m_hReflectionProbeTextures.Reserve(reflectionResources.m_ReflectionProbeTextures.GetCount());
  for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < reflectionResources.m_ReflectionProbeTextures.GetCount(); ++uiProbeIndex)
  {
    xiiStringBuilder sResourceName;
    sResourceName.SetFormat("ReflectionProbeTexture_{0}", uiProbeIndex);
    data.m_hReflectionProbeTextures.PushBack(builder.ReadTexture(
      builder.ImportTexture(sResourceName, reflectionResources.m_ReflectionProbeTextures[uiProbeIndex], reflectionResources.m_ReflectionProbeTextures[uiProbeIndex]->GetResourceState()),
      xiiGALResourceStateFlags::ShaderResource));
  }

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = GetRenderResolutionWidth();
  description.m_Size.height      = GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hIndirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hEnvironmentSpecular    = builder.WriteTexture(xiiRGBlackboardKeys::k_EnvironmentSpecular, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pIndirectLightingPipeline, "Shaders/Pipeline/IndirectLighting.xiiShader");
}

void xiiView::ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredIndirectLighting");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pIndirectLightingPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIIr", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SparseVoxelIrradiance", context.GetTexture(data.m_hSparseVoxelIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SkyRadiance", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hReflectionProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeClusters", context.GetBuffer(data.m_hReflectionProbeClusters)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);

    xiiHybridArray<xiiGALTextureView*, XII_MAX_REFLECTION_PROBES> reflectionProbeViews;
    reflectionProbeViews.SetCount(XII_MAX_REFLECTION_PROBES);
    xiiGALTextureView* pFallbackView = context.GetTexture(data.m_hFallbackReflectionProbe)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < XII_MAX_REFLECTION_PROBES; ++uiProbeIndex)
    {
      reflectionProbeViews[uiProbeIndex] = uiProbeIndex < data.m_hReflectionProbeTextures.GetCount() ?
        context.GetTexture(data.m_hReflectionProbeTextures[uiProbeIndex])->GetDefaultView(xiiGALTextureViewType::ShaderResource) :
        pFallbackView;
    }
    cmd.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeTextures", 0U, reflectionProbeViews, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_IndirectOut", context.GetTexture(data.m_hIndirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_EnvironmentSpecularOut", context.GetTexture(data.m_hEnvironmentSpecular)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
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
  xiiRenderGraphTextureHandle                 m_hSceneDepth;                     ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                 m_hGBufferNormal;                  ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                 m_hGBufferAlbedo;                  ///< ShaderResource in (surface albedo for diffuse bounce response).
  xiiRenderGraphTextureHandle                 m_hRayTracedRawGlobalIllumination; ///< UnorderedAccess out (raw RT GI texture).
  xiiRenderGraphBufferHandle                  m_hSceneDependency;
  xiiRenderGraphBufferHandle                  m_hMaterialData;
  xiiRenderGraphBufferHandle                  m_hGeometryData;
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

void xiiView::SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                     = xiiGALResourceDimension::Texture2D;
  description.m_Format                   = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width               = GetRenderResolutionWidth();
  description.m_Size.height              = GetRenderResolutionHeight();
  description.m_uiMipLevels              = 1U;
  description.m_BindFlags                = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage                    = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawGlobalIllumination = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto& lightingPasses                     = m_ViewPassResources->m_LightingPasses;
  data.m_bUseHardwareRayTracing            = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingGlobalIlluminationResources();
  lightingPasses.m_bRTGIAvailableThisFrame = data.m_bUseHardwareRayTracing;
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = lightingPasses.m_pRTGIPipeline;
    data.m_uiShaderRecordStride = lightingPasses.m_uiRTGIShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT GI Shader Binding Table", lightingPasses.m_pRTGIShaderBindingTable, lightingPasses.m_pRTGIShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
    builder.SetPassAllowMerge(false);
  }
  else
  {
    xiiView::EnsureComputePipeline(lightingPasses.m_pRTGIFallbackPipeline, "Shaders/Pipeline/RTGIFallback.xiiShader");
  }
}

void xiiView::ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTGIFinalGather");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIRaw", context.GetTexture(data.m_hRayTracedRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      const xiiBitflags<xiiGALShaderType> hitStages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", hitStages);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", hitStages);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
      xiiGALUpdateSBTDescription sbtUpdate;
      sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
      sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
      sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
      sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
      sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
      cmd.UpdateSBT(sbtUpdate);

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, GetRenderResolutionWidth(), GetRenderResolutionHeight());
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pRTGIFallbackPipeline);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIRaw", context.GetTexture(data.m_hRayTracedRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// ReSTIR Global Illumination Temporal Reuse //////////

struct xiiReSTIRGITemporalData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hCurrentRadiance;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSample;
  xiiRenderGraphTextureHandle m_hPreviousState;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hTemporalSample;
  xiiRenderGraphTextureHandle m_hTemporalState;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

void xiiView::SetupReSTIRGITemporal(xiiReSTIRGITemporalData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiPreviousSlot = (m_ViewPassResources->m_LightingPasses.m_uiFrameIndex + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_LightingPasses;

  const bool bSampleHistoryRecreated  = EnsureTemporalHistoryTextures(resources.m_pRTGIReservoirSampleHistory, xiiGALResourceFormat::RGBA16Float, uiWidth, uiHeight);
  const bool bStateHistoryRecreated   = EnsureTemporalHistoryTextures(resources.m_pRTGIReservoirStateHistory, xiiGALResourceFormat::RG32Float, uiWidth, uiHeight);
  const bool bSurfaceHistoryRecreated = EnsureTemporalHistoryTextures(resources.m_pRTGIReservoirSurfaceHistory, xiiGALResourceFormat::RGBA16Float, uiWidth, uiHeight);
  const bool bHistoryRecreated        = bSampleHistoryRecreated || bStateHistoryRecreated || bSurfaceHistoryRecreated;
  if (bHistoryRecreated)
    resources.m_bRTGIReservoirHistoryValid = false;

  data.m_bHistoryValid    = resources.m_bRTGIReservoirHistoryValid && resources.m_bRTGIAvailableThisFrame;
  data.m_hCurrentRadiance = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity        = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSample  = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous Sample", resources.m_pRTGIReservoirSampleHistory[uiPreviousSlot], resources.m_pRTGIReservoirSampleHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousState   = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous State", resources.m_pRTGIReservoirStateHistory[uiPreviousSlot], resources.m_pRTGIReservoirStateHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous Surface", resources.m_pRTGIReservoirSurfaceHistory[uiPreviousSlot], resources.m_pRTGIReservoirSurfaceHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hTemporalSample    = builder.WriteTexture("ReSTIR GI Temporal Sample", description, xiiGALResourceStateFlags::UnorderedAccess);

  description.m_Format  = xiiGALResourceFormat::RG32Float;
  data.m_hTemporalState = builder.WriteTexture("ReSTIR GI Temporal State", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("ReSTIR GI Temporal Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(resources.m_pReSTIRGITemporalPipeline, "Shaders/Pipeline/ReSTIRGITemporal.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteReSTIRGITemporal(const xiiReSTIRGITemporalData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRGITemporal");
  {
    {
      xiiGALMapHelper<xiiReSTIRGIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid   = data.m_bHistoryValid ? 1U : 0U;
      pConstants->MaxTemporalM   = 20U;
      pConstants->SpatialSamples = 4U;
      pConstants->RadianceClamp  = 64.0f;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pReSTIRGITemporalPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentRadiance", context.GetTexture(data.m_hCurrentRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSample", context.GetTexture(data.m_hPreviousSample)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousState", context.GetTexture(data.m_hPreviousState)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalSample", context.GetTexture(data.m_hTemporalSample)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalState", context.GetTexture(data.m_hTemporalState)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// ReSTIR Global Illumination Spatial Reuse //////////

struct xiiReSTIRGISpatialData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hTemporalSample;
  xiiRenderGraphTextureHandle m_hTemporalState;
  xiiRenderGraphTextureHandle m_hReservoirSample;
  xiiRenderGraphTextureHandle m_hReservoirState;
  xiiRenderGraphTextureHandle m_hReservoirSurface;
  xiiRenderGraphTextureHandle m_hResampledRadiance;
  xiiRenderGraphBufferHandle  m_hConstants;
};

void xiiView::SetupReSTIRGISpatial(xiiReSTIRGISpatialData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth       = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight      = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  auto&           resources     = m_ViewPassResources->m_LightingPasses;

  data.m_hSceneDepth       = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal    = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo    = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalSample   = builder.ReadTexture("ReSTIR GI Temporal Sample", xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalState    = builder.ReadTexture("ReSTIR GI Temporal State", xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoirSample  = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current Sample", resources.m_pRTGIReservoirSampleHistory[uiCurrentSlot], resources.m_pRTGIReservoirSampleHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirState   = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current State", resources.m_pRTGIReservoirStateHistory[uiCurrentSlot], resources.m_pRTGIReservoirStateHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirSurface = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current Surface", resources.m_pRTGIReservoirSurfaceHistory[uiCurrentSlot], resources.m_pRTGIReservoirSurfaceHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hResampledRadiance = builder.WriteTexture(xiiRGBlackboardKeys::k_RTResampledGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("ReSTIR GI Spatial Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(resources.m_pReSTIRGISpatialPipeline, "Shaders/Pipeline/ReSTIRGISpatial.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteReSTIRGISpatial(const xiiReSTIRGISpatialData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRGISpatial");
  {
    {
      xiiGALMapHelper<xiiReSTIRGIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid   = 1U;
      pConstants->MaxTemporalM   = 20U;
      pConstants->SpatialSamples = 4U;
      pConstants->RadianceClamp  = 64.0f;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pReSTIRGISpatialPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalSample", context.GetTexture(data.m_hTemporalSample)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalState", context.GetTexture(data.m_hTemporalState)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirSampleOut", context.GetTexture(data.m_hReservoirSample)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirStateOut", context.GetTexture(data.m_hReservoirState)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirSurfaceOut", context.GetTexture(data.m_hReservoirSurface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResampledRadianceOut", context.GetTexture(data.m_hResampledRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPasses.m_bRTGIReservoirHistoryValid = m_ViewPassResources->m_LightingPasses.m_bRTGIAvailableThisFrame;
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Global Illumination Denoise Data //////////

struct xiiRayTracedGlobalIlluminationDenoiseData
{
  xiiRenderGraphTextureHandle m_hRawGlobalIllumination;
  xiiRenderGraphTextureHandle m_hPreviousGlobalIllumination;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hFinalGlobalIllumination;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
  bool                        m_bSignalValid  = false;
};

void xiiView::SetupRayTracedGlobalIlluminationDenoise(xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_LightingPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pRTGIHistory, xiiGALResourceFormat::RGBA16Float, uiWidth, uiHeight))
  {
    resources.m_bRTGIHistoryValid = false;
  }

  data.m_bSignalValid                = resources.m_bRTGIAvailableThisFrame;
  data.m_bHistoryValid               = resources.m_bRTGIHistoryValid && data.m_bSignalValid;
  data.m_hRawGlobalIllumination      = builder.ReadTexture(xiiRGBlackboardKeys::k_RTResampledGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                 = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal              = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                   = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface            = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousGlobalIllumination = builder.ReadTexture(builder.ImportTexture("RTGI Previous History", resources.m_pRTGIHistory[uiPreviousSlot], resources.m_pRTGIHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hFinalGlobalIllumination    = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalGI, resources.m_pRTGIHistory[uiCurrentSlot], resources.m_pRTGIHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hConstants = CreateTemporalDenoiseConstants(builder, "RTGI Temporal Denoise Constants");

  xiiView::EnsureComputePipeline(resources.m_pRTGITemporalDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiView::ExecuteRayTracedGlobalIlluminationDenoise(const xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTGITemporalDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.94f;
      pConstants->DepthThreshold  = 0.06f;
      pConstants->NormalThreshold = 0.86f;
      pConstants->SpatialWeight   = 0.20f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 1U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pRTGITemporalDenoisePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hFinalGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPasses.m_bRTGIHistoryValid = data.m_bSignalValid;
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Data //////////
//
// Collects all GPU resources related to ray traced reflections.

struct xiiRayTracedReflectionsData
{
  xiiRenderGraphTextureHandle                 m_hSceneDepth;              ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                 m_hGBufferNormal;           ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                 m_hGBufferMaterial;         ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle                 m_hBRDFLut;                 ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle                 m_hRayTracedRawReflections; ///< UnorderedAccess out (raw RT reflections texture).
  xiiRenderGraphBufferHandle                  m_hSceneDependency;         ///< BuildASRead dependency on this frame's TLAS build.
  xiiRenderGraphBufferHandle                  m_hMaterialData;            ///< ShaderResource canonical material records indexed by TLAS instance.
  xiiRenderGraphBufferHandle                  m_hGeometryData;            ///< ShaderResource geometry records indexed by TLAS instance.
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;      ///< RayTracing in (shader group records).
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

void xiiView::SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut         = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type              = xiiGALResourceDimension::Texture2D;
  description.m_Format            = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width        = GetRenderResolutionWidth();
  description.m_Size.height       = GetRenderResolutionHeight();
  description.m_uiMipLevels       = 1U;
  description.m_BindFlags         = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage             = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawReflections, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto& lightingPasses                             = m_ViewPassResources->m_LightingPasses;
  data.m_bUseHardwareRayTracing                    = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingReflectionResources();
  lightingPasses.m_bRTReflectionAvailableThisFrame = data.m_bUseHardwareRayTracing;
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = lightingPasses.m_pRTReflectionPipeline;
    data.m_uiShaderRecordStride = lightingPasses.m_uiRTReflectionShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT Reflection Shader Binding Table", lightingPasses.m_pRTReflectionShaderBindingTable, lightingPasses.m_pRTReflectionShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
  }
  else
  {
    // Preserve the SSR/probe fallback contract on devices or frames without a valid TLAS.
    xiiView::EnsureComputePipeline(lightingPasses.m_pRTReflectionFallbackPipeline, "Shaders/Pipeline/RTReflectionFallback.xiiShader");
  }
}

void xiiView::ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTReflections");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflRaw", context.GetTexture(data.m_hRayTracedRawReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      const xiiBitflags<xiiGALShaderType> hitStages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", hitStages);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", hitStages);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
      xiiGALUpdateSBTDescription sbtUpdate;
      sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
      sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
      sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
      sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
      sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
      cmd.UpdateSBT(sbtUpdate);

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, GetRenderResolutionWidth(), GetRenderResolutionHeight());
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pRTReflectionFallbackPipeline);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflRaw", context.GetTexture(data.m_hRayTracedRawReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Denoise Data //////////

struct xiiRayTracedReflectionsDenoiseData
{
  xiiRenderGraphTextureHandle m_hRawReflections;
  xiiRenderGraphTextureHandle m_hPreviousReflections;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hFinalReflections;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
  bool                        m_bSignalValid  = false;
};

void xiiView::SetupRayTracedReflectionsDenoise(xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_LightingPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pRTReflectionHistory, xiiGALResourceFormat::RGBA16Float, uiWidth, uiHeight))
  {
    resources.m_bRTReflectionHistoryValid = false;
  }

  data.m_bSignalValid         = resources.m_bRTReflectionAvailableThisFrame;
  data.m_bHistoryValid        = resources.m_bRTReflectionHistoryValid && data.m_bSignalValid;
  data.m_hRawReflections      = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth          = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal       = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity            = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface     = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReflections = builder.ReadTexture(builder.ImportTexture("RT Reflection Previous History", resources.m_pRTReflectionHistory[uiPreviousSlot], resources.m_pRTReflectionHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hFinalReflections    = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalReflections, resources.m_pRTReflectionHistory[uiCurrentSlot], resources.m_pRTReflectionHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants           = CreateTemporalDenoiseConstants(builder, "RT Reflection Temporal Denoise Constants");

  xiiView::EnsureComputePipeline(resources.m_pRTReflectionTemporalDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiView::ExecuteRayTracedReflectionsDenoise(const xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTReflectionTemporalDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.90f;
      pConstants->DepthThreshold  = 0.04f;
      pConstants->NormalThreshold = 0.92f;
      pConstants->SpatialWeight   = 0.12f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 1U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pRTReflectionTemporalDenoisePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRawReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hFinalReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPasses.m_bRTReflectionHistoryValid = data.m_bSignalValid;
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
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;          ///< ShaderResource in (base color and metallic F0 source).
  xiiRenderGraphTextureHandle m_hBRDFLut;                ///< ShaderResource in (split-sum material response).
  xiiRenderGraphTextureHandle m_hHiZPyramid;             ///< ShaderResource in (hierarchical depth used for ray traversal).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;          ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections; ///< UnorderedAccess out (screen-space reflections texture).
  xiiRenderGraphBufferHandle  m_hConstants;              ///< ConstantBuffer in (SSR quality and traversal settings).
  xiiSSRConstants             m_Constants;
};

void xiiView::SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut         = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHiZPyramid      = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZPyramid, xiiGALResourceStateFlags::ShaderResource);
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

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiUInt32 uiMipWidth  = GetRenderResolutionWidth();
  xiiUInt32 uiMipHeight = GetRenderResolutionHeight();
  xiiUInt32 uiMipCount  = 1U;
  while (uiMipWidth > 1U || uiMipHeight > 1U)
  {
    uiMipWidth  = xiiMath::Max(uiMipWidth >> 1U, 1U);
    uiMipHeight = xiiMath::Max(uiMipHeight >> 1U, 1U);
    ++uiMipCount;
  }

  data.m_Constants.MaxSteps      = 96U;
  data.m_Constants.Thickness     = 0.0025f;
  data.m_Constants.MaxRoughness  = 0.72f;
  data.m_Constants.StrideZCutoff = 0.25f;
  data.m_Constants.InvResolution = xiiVec2(1.0f / static_cast<float>(GetRenderResolutionWidth()), 1.0f / static_cast<float>(GetRenderResolutionHeight()));
  data.m_Constants.HiZMipCount   = uiMipCount;
  data.m_Constants._Padding      = 0.0f;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pSSRPipeline, "Shaders/Pipeline/SSR.xiiShader");
}

void xiiView::ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSR");
  {
    {
      xiiGALMapHelper<xiiSSRConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pSSRPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HiZPyramid", context.GetTexture(data.m_hHiZPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSROut", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// Hybrid Reflection Composite Data //////////
//
// Replaces the probe/sky specular contribution already present in the base
// opaque scene with the highest-confidence reflection tier available.

struct xiiHybridReflectionCompositeData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hEnvironmentSpecular;    ///< ShaderResource in (Tier 2 local probe / sky BRDF response).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections; ///< ShaderResource in (Tier 1 screen-space response + confidence).
  xiiRenderGraphTextureHandle m_hRayTracedReflections;   ///< ShaderResource in (Tier 3 traced response + validity).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;          ///< UnorderedAccess in/out (authoritative scene radiance).
};

void xiiView::SetupHybridReflectionComposite(xiiHybridReflectionCompositeData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hEnvironmentSpecular    = builder.ReadTexture(xiiRGBlackboardKeys::k_EnvironmentSpecular, xiiGALResourceStateFlags::ShaderResource);
  data.m_hScreenSpaceReflections = builder.ReadTexture(xiiRGBlackboardKeys::k_SSRTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedReflections   = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor          = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pReflectionCompositePipeline, "Shaders/Pipeline/ReflectionComposite.xiiShader");
}

void xiiView::ExecuteHybridReflectionComposite(const xiiHybridReflectionCompositeData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HybridReflectionComposite");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pReflectionCompositePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_EnvironmentSpecular", context.GetTexture(data.m_hEnvironmentSpecular)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ScreenSpaceReflection", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedReflection", context.GetTexture(data.m_hRayTracedReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Light Injection Data //////////
//
// Evaluates shadowed directional and clustered local lighting once per froxel.

struct xiiVolumetricLightInjectionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelScattering; ///< UnorderedAccess inout (initialized scattering and injected light).
  xiiRenderGraphBufferHandle  m_hFroxelMetadata;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;
  xiiRenderGraphBufferHandle  m_hShadowCascadeConstants;
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle  m_hCloudShadowConstants;
};

void xiiView::SetupVolumetricLightInjection(xiiVolumetricLightInjectionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelScattering            = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hFroxelMetadata              = builder.ReadBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer             = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer            = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hShadowCascadeConstants      = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hDirectionalShadowAtlas      = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCloudShadowConstants        = builder.ReadBuffer(xiiRGBlackboardKeys::k_CloudShadowConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pVolumetricLightInjectionPipeline, "Shaders/Pipeline/VolumetricLightInjection.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVolumetricLightInjection(const xiiVolumetricLightInjectionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricLightInjection");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pVolumetricLightInjectionPipeline);
    m_ViewPassResources->m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_ViewPassResources->m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_FroxelMeta", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hShadowCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VolumetricDirectionalShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VolumetricLocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VolumetricLocalShadowData", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiCloudShadowConstants", context.GetBuffer(data.m_hCloudShadowConstants), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 8U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Integration Data //////////
//
// Collects all GPU resources related to volumetric fog integration.

struct xiiVolumetricFogIntegrationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelScatteringBuffer; ///< ShaderResource in (pre-lit froxel scattering buffer).
  xiiRenderGraphTextureHandle m_hFroxelIntegratedBuffer; ///< UnorderedAccess out (front-to-back scattering prefix volume).
};

void xiiView::SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelScatteringBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type               = xiiGALResourceDimension::Texture3D;
  description.m_Format             = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width         = 128U;
  description.m_Size.height        = 72U;
  description.m_uiArraySizeOrDepth = 64U;
  description.m_uiMipLevels        = 1U;
  description.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hFroxelIntegratedBuffer   = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelIntegratedBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pVolumetricIntegratePipeline, "Shaders/Pipeline/VolumetricLightIntegration.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogIntegrate");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pVolumetricIntegratePipeline);

    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScatteringBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelIntegratedOut", context.GetTexture(data.m_hFroxelIntegratedBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Resolve Data //////////
//
// Samples the integrated froxel prefix at the opaque surface depth.

struct xiiVolumetricFogResolveData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelIntegratedBuffer;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hVolumetricScattering;
};

void xiiView::SetupVolumetricFogResolve(xiiVolumetricFogResolveData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelIntegratedBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelIntegratedBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth             = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = GetRenderResolutionWidth();
  description.m_Size.height    = GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hVolumetricScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_VolumetricScatteringRaw, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pVolumetricResolvePipeline, "Shaders/Pipeline/VolumetricFogResolve.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVolumetricFogResolve(const xiiVolumetricFogResolveData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogResolve");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pVolumetricResolvePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelIntegrated", context.GetTexture(data.m_hFroxelIntegratedBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
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

  xiiRenderGraphTextureHandle m_hCurrentScattering;  ///< ShaderResource in (current integrated scattering).
  xiiRenderGraphTextureHandle m_hPreviousScattering; ///< ShaderResource in (previous temporally filtered scattering).
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hVolumetricScattering; ///< UnorderedAccess out (current persistent history and final signal).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiMat4                     m_PreviousViewProjection = xiiMat4::MakeIdentity();
  bool                        m_bHistoryValid          = false;
};

void xiiView::SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_ViewPassResources->m_LightingPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pVolumetricHistory, xiiGALResourceFormat::RGBA16Float, uiWidth, uiHeight))
  {
    resources.m_bVolumetricHistoryValid = false;
  }

  const auto& depthPasses       = m_ViewPassResources->m_DepthPasses;
  data.m_bHistoryValid          = resources.m_bVolumetricHistoryValid && depthPasses.m_bMotionHistoryValid;
  data.m_PreviousViewProjection = depthPasses.m_bMotionHistoryValid ? depthPasses.m_PreviousViewProjectionMatrix : GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_hCurrentScattering     = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScatteringRaw, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousScattering    = builder.ReadTexture(builder.ImportTexture("Volumetric Scattering Previous", resources.m_pVolumetricHistory[uiPreviousSlot], resources.m_pVolumetricHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth            = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity              = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface       = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_VolumetricScattering, resources.m_pVolumetricHistory[uiCurrentSlot], resources.m_pVolumetricHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiVolumetricTemporalConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Volumetric Temporal Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(resources.m_pVolumetricTemporalPipeline, "Shaders/Pipeline/VolumetricFogTemporalRep.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogTemporalRep");
  {
    {
      xiiGALMapHelper<xiiVolumetricTemporalConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->PreviousViewProjectionMatrix = data.m_PreviousViewProjection;
      pConstants->HistoryWeight                = 0.92f;
      pConstants->DepthThreshold               = 0.08f;
      pConstants->NormalThreshold              = 0.80f;
      pConstants->SpatialWeight                = 0.10f;
      pConstants->HistoryValid                 = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding                     = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pVolumetricTemporalPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVolumetricTemporalConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelCurrent", context.GetTexture(data.m_hCurrentScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelHistory", context.GetTexture(data.m_hPreviousScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelBlended", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_ViewPassResources->m_LightingPasses.m_bVolumetricHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Composite Data //////////
//
// Collects all GPU resources related to atmosphere compositing.

struct xiiAtmosphereCompositeData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;                 ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hAtmosphereTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphTextureHandle m_hAtmosphereMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;              ///< UnorderedAccess in/out (sky and aerial perspective composite).
  xiiRenderGraphBufferHandle  m_hConstants;                  ///< Physical atmosphere parameters.
  xiiAtmosphereConstants      m_Constants;
};

void xiiView::SetupAtmosphereComposite(xiiAtmosphereCompositeData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth                 = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor              = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants                  = CreateAtmosphereConstantsBuffer(builder, "AtmosphereCompositeConstants");
  data.m_Constants                   = MakeAtmosphereConstants(m_ViewPassResources->m_LightingPrepPasses.m_hAtmosphereLUT);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_LightingPasses.m_pAtmosphereCompositePipeline, "Shaders/Pipeline/AtmosphereComposite.xiiShader");
}

void xiiView::ExecuteAtmosphereComposite(const xiiAtmosphereCompositeData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereComposite");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_ViewPassResources->m_LightingPasses.m_pAtmosphereCompositePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hAtmosphereTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hAtmosphereMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// Calibrated Sensor Output Data //////////

struct xiiSensorOutputData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneRadiance;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hSceneNormal;
  xiiRenderGraphTextureHandle m_hBaseColor;
  xiiRenderGraphTextureHandle m_hSensorOutput;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSensorProfile            m_Profile;
  xiiUInt32                   m_uiFrameIndex = 0U;
};

void xiiView::SetupSensorOutput(xiiSensorOutputData& data, xiiRenderGraphBuilder& builder)
{
  auto& resources = m_ViewPassResources->m_OutputPasses;
  if (xiiSensorRenderingManager::GetProfile(resources.m_hSensorProfile, data.m_Profile).Failed())
    return;

  const auto MatchesProfile = [&data](const xiiSharedPtr<xiiGALTexture>& pTexture) {
    return pTexture != nullptr && pTexture->GetDescription().m_Size.width == data.m_Profile.m_uiResolutionX &&
      pTexture->GetDescription().m_Size.height == data.m_Profile.m_uiResolutionY;
  };

  if (!MatchesProfile(resources.m_pSensorOutputTexture))
  {
    if (resources.m_pSensorOutputTexture != nullptr)
      resources.m_RetiredSensorOutputTextures.PushBack(std::move(resources.m_pSensorOutputTexture));

    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA32Float;
    description.m_Size.width  = data.m_Profile.m_uiResolutionX;
    description.m_Size.height = data.m_Profile.m_uiResolutionY;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
    if (pDevice == nullptr)
      return;

    resources.m_pSensorOutputTexture = pDevice->CreateTexture(description);
    if (resources.m_pSensorOutputTexture == nullptr)
      return;
    resources.m_pSensorOutputTexture->SetDebugName("Calibrated Sensor Output");
  }

  data.m_hSceneRadiance = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBaseColor     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSensorOutput  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_SensorOutput, resources.m_pSensorOutputTexture, resources.m_pSensorOutputTexture->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSensorOutputConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Sensor Output Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_uiFrameIndex                   = m_ViewPassResources->m_LightingPasses.m_uiFrameIndex;

  builder.ExportTexture(data.m_hSensorOutput, xiiGALResourceStateFlags::ShaderResource);
  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
  xiiView::EnsureComputePipeline(resources.m_pSensorOutputPipeline, "Shaders/Pipeline/SensorOutput.xiiShader");
}

void xiiView::ExecuteSensorOutput(const xiiSensorOutputData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_hSceneRadiance.IsValid() || !data.m_hSceneDepth.IsValid() || !data.m_hSceneNormal.IsValid() ||
      !data.m_hBaseColor.IsValid() || !data.m_hSensorOutput.IsValid() || !data.m_hConstants.IsValid())
    return;

  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("CalibratedSensorOutput");
  {
    {
      xiiGALMapHelper<xiiSensorOutputConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->Intrinsics               = xiiVec4(data.m_Profile.m_fFocalLengthXPixels, data.m_Profile.m_fFocalLengthYPixels, data.m_Profile.m_fPrincipalPointXPixels, data.m_Profile.m_fPrincipalPointYPixels);
      pConstants->RangeAndExposure         = xiiVec4(data.m_Profile.m_fNearPlaneMeters, data.m_Profile.m_fFarPlaneMeters, data.m_Profile.m_fExposureSeconds, data.m_Profile.m_fRollingShutterSeconds);
      pConstants->SpectralSensitivityAndQE = xiiVec4(data.m_Profile.m_vSpectralSensitivity.x, data.m_Profile.m_vSpectralSensitivity.y, data.m_Profile.m_vSpectralSensitivity.z, data.m_Profile.m_fQuantumEfficiency);
      pConstants->SignalConversion         = xiiVec4(data.m_Profile.m_fRadianceToElectrons, data.m_Profile.m_fAnalogGain, data.m_Profile.m_fSaturationElectrons, data.m_Profile.m_fWavelengthNanometers);
      pConstants->NoiseParameters          = xiiVec4(data.m_Profile.m_fReadNoiseElectrons, data.m_Profile.m_fShotNoiseScale, data.m_Profile.m_fDepthNoiseStandardDeviationMeters, data.m_Profile.m_fDepthNoiseScalePerMeter);
      pConstants->OutputDescription        = xiiVec4U32(data.m_Profile.m_Type.GetValue(), data.m_Profile.m_NoiseModel.GetValue(), data.m_Profile.m_uiOutputBitDepth, data.m_Profile.m_uiNoiseSeed);
      pConstants->FrameAndResolution       = xiiVec4U32(data.m_uiFrameIndex, data.m_Profile.m_uiResolutionX, data.m_Profile.m_uiResolutionY, data.m_Profile.m_Shutter.GetValue());
    }

    cmd.SetPipelineState(m_ViewPassResources->m_OutputPasses.m_pSensorOutputPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSensorOutputConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneRadiance", context.GetTexture(data.m_hSceneRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneNormal", context.GetTexture(data.m_hSceneNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BaseColor", context.GetTexture(data.m_hBaseColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SensorOutput", context.GetTexture(data.m_hSensorOutput)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Profile.m_uiResolutionX + 7U) / 8U, (data.m_Profile.m_uiResolutionY + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Forward Opaque Data //////////
//
// Collects all GPU resources related to the forward opaque pass.

struct xiiForwardOpaqueData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDirectLighting;       ///< ShaderResource in (direct lighting texture).
  xiiRenderGraphTextureHandle m_hIndirectLighting;     ///< ShaderResource in (indirect lighting including probe/sky specular).
  xiiRenderGraphTextureHandle m_hGBufferEmissive;      ///< ShaderResource in (surface emissive radiance).
  xiiRenderGraphTextureHandle m_hRayTracedFinalGI;     ///< ShaderResource in (optional near-field traced GI).
  xiiRenderGraphTextureHandle m_hVolumetricScattering; ///< ShaderResource in (integrated scattering and transmittance).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< UnorderedAccess out (base scene radiance before reflection tier replacement).
};

void xiiView::SetupForwardOpaque(xiiForwardOpaqueData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDirectLighting       = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLighting     = builder.ReadTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalGI     = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor     = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  builder.SetPassAllowMerge(false);
  xiiView::EnsureComputePipeline(m_ViewPassResources->m_ForwardPasses.m_pForwardOpaquePipeline, "Shaders/Pipeline/OpaqueComposite.xiiShader");
}

void xiiView::ExecuteForwardOpaque(const xiiForwardOpaqueData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ForwardOpaque");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pForwardOpaquePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_DirectLight", context.GetTexture(data.m_hDirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_IndirectLight", context.GetTexture(data.m_hIndirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Emissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Volumetric", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedGI", context.GetTexture(data.m_hRayTracedFinalGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
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

    if (m_ViewPassResources->m_ForwardPasses.m_pForwardMaskedPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pForwardMaskedPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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

    if (m_ViewPassResources->m_ForwardPasses.m_pHairPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pHairPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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

    if (m_ViewPassResources->m_ForwardPasses.m_pWaterPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pWaterPipeline);
      if (data.m_hPlanarReflectionMap.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_PlanarRefl", context.GetTexture(data.m_hPlanarReflectionMap)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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
    if (m_ViewPassResources->m_ForwardPasses.m_pSSSComputePipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pSSSComputePipeline);
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

    if (m_ViewPassResources->m_ForwardPasses.m_pEyePipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources->m_ForwardPasses.m_pEyePipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
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

  if (!m_ViewPassResources->m_TransparencyPasses.m_pParticleStateBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiParticleGPUState);
    description.m_uiSize              = description.m_uiElementByteStride * uiDefaultParticleCapacity;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_ViewPassResources->m_TransparencyPasses.m_pParticleStateBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    m_ViewPassResources->m_TransparencyPasses.m_uiParticleCapacity   = uiDefaultParticleCapacity;
  }

  data.m_hParticleState  = builder.ImportBuffer("ParticleState", m_ViewPassResources->m_TransparencyPasses.m_pParticleStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hParticleState  = builder.WriteBuffer(data.m_hParticleState, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiParticleCount = xiiMath::Max(1U, m_ViewPassResources->m_TransparencyPasses.m_uiParticleCapacity);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiGPUParticleSimulateConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;

  data.m_hParticleConstants = builder.WriteBuffer("ParticleSimConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TransparencyPasses.m_pParticleSimulatePipeline, "Shaders/Pipeline/GPUParticleSimulate.xiiShader");

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

    cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pParticleSimulatePipeline);
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
    EnsureFallbackTexture(m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture, 0xFFFFFFFFU, "FallbackDecalAtlas::Albedo");
    EnsureFallbackTexture(m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture, 0xFFFF8080U, "FallbackDecalAtlas::Normal");
    EnsureFallbackTexture(m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture, 0x00FF0080U, "FallbackDecalAtlas::Material");
    EnsureFallbackTexture(m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture, 0x00000000U, "FallbackDecalAtlas::Emissive");

    if (m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalAtlasSampler == nullptr)
    {
      xiiGALSamplerCreationDescription samplerDescription                    = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
      samplerDescription.m_AddressU                                          = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressV                                          = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressW                                          = xiiGALTextureAddressMode::Clamp;
      m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalAtlasSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(samplerDescription);
    }
  };

  EnsureFallbackAtlases();

  xiiSharedPtr<xiiGALTexture> pAtlasAlbedo   = m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasNormal   = m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasMaterial = m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasEmissive = m_ViewPassResources->m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture;

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
  GetBlackboard().Set(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);

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

  const bool bHasDecalCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
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

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TransparencyPasses.m_pDecalCullBatchPipeline, "Shaders/Pipeline/DecalClassification.xiiShader");

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

    cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pDecalCullBatchPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

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

  const bool bHasProjectedDecalCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasProjectedDecalCount);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TransparencyPasses.m_pSSDecalResolvePipeline, "Shaders/Pipeline/DecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ProjectedDecalResolve");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pSSDecalResolvePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

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

  const bool bHasMeshDecalCount = GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasMeshDecalCount);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TransparencyPasses.m_pMeshDecalResolvePipeline, "Shaders/Pipeline/MeshDecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MeshDecalDraw");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pMeshDecalResolvePipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

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

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TransparencyPasses.m_pOITResolvePipeline, "Shaders/Pipeline/OITResolve.xiiShader");

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

    if (m_ViewPassResources->m_TransparencyPasses.m_pTranslucentPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pTranslucentPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(GetBlackboard())});
    }

    if (m_ViewPassResources->m_TransparencyPasses.m_pOITResolvePipeline)
    {
      cmd.SetPipelineState(m_ViewPassResources->m_TransparencyPasses.m_pOITResolvePipeline);
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

  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;   ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;   ///< ShaderResource in (surface albedo and material AO).
  xiiRenderGraphTextureHandle m_hGBufferMaterial; ///< ShaderResource in (surface metallic response).
  xiiRenderGraphTextureHandle m_hHDRIn;           ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphTextureHandle m_hSSGIOut;         ///< UnorderedAccess out (screen-space diffuse GI contribution).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSSGIConstants            m_Constants;
};

void xiiView::SetupScreenSpaceGlobalIllumination(xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn           = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiRenderWidth;
  description.m_Size.height = uiRenderHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSSGIOut           = builder.WriteTexture(xiiRGBlackboardKeys::k_SSGITexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSGIConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  const xiiLightingSystemSettings& settings = m_ViewPassResources->m_LightingSystem.GetSettings();
  data.m_Constants.RayLength                = xiiMath::Max(settings.m_fSSGIRayLength, 0.0f);
  data.m_Constants.SampleCount              = xiiMath::Clamp(settings.m_uiSSGISampleCount, 1U, 32U);
  data.m_Constants.Thickness                = xiiMath::Max(settings.m_fSSGIThickness, 0.001f);
  data.m_Constants.Intensity                = xiiMath::Max(settings.m_fSSGIIntensity, 0.0f);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_ScreenSpacePasses.m_pSSGIPipeline, "Shaders/Pipeline/SSGI.xiiShader");
}

void xiiView::ExecuteScreenSpaceGlobalIllumination(const xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("SSGI");
  {
    {
      xiiGALMapHelper<xiiSSGIConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }
    cmd.SetPipelineState(m_ViewPassResources->m_ScreenSpacePasses.m_pSSGIPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSGIOut", context.GetTexture(data.m_hSSGIOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

struct xiiScreenSpaceGlobalIlluminationCompositeData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSSGI;
  xiiRenderGraphTextureHandle m_hHDRScene;
};

void xiiView::SetupScreenSpaceGlobalIlluminationComposite(xiiScreenSpaceGlobalIlluminationCompositeData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);
  data.m_hSSGI     = builder.ReadTexture(xiiRGBlackboardKeys::k_SSGITexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRScene = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  xiiView::EnsureComputePipeline(m_ViewPassResources->m_ScreenSpacePasses.m_pSSGICompositePipeline, "Shaders/Pipeline/SSGIComposite.xiiShader");
}

void xiiView::ExecuteScreenSpaceGlobalIlluminationComposite(const xiiScreenSpaceGlobalIlluminationCompositeData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SSGIComposite");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_ScreenSpacePasses.m_pSSGICompositePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_SSGI", context.GetTexture(data.m_hSSGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRScene)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(GetRenderResolutionWidth() + 7U) / 8U, (GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen-Space Refraction Data //////////
//
// Collects all GPU resources related to screen-space refraction.

struct xiiScreenSpaceRefractionSnapshotData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSource; ///< CopySource in (completed scene color before refraction).
  xiiRenderGraphTextureHandle m_hSnapshot;  ///< CopyDestination out (immutable source for neighborhood sampling).
};

void xiiView::SetupScreenSpaceRefractionSnapshot(xiiScreenSpaceRefractionSnapshotData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSource = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::CopySource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = GetRenderResolutionWidth();
  description.m_Size.height = GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSnapshot          = builder.WriteTexture(xiiRGBlackboardKeys::k_RefractionSceneColorInput, description, xiiGALResourceStateFlags::CopyDestination);

  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteScreenSpaceRefractionSnapshot(const xiiScreenSpaceRefractionSnapshotData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SSRefractionSnapshot");
  {
    cmd.CopyTexture(context.GetTexture(data.m_hHDRSource), context.GetTexture(data.m_hSnapshot));
  }
  cmd.EndDebugGroup();
}

struct xiiScreenSpaceRefractionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;   ///< ShaderResource in (G-Buffer normal texture).
  xiiRenderGraphTextureHandle m_hGBufferMaterial; ///< ShaderResource in (material feature flags).
  xiiRenderGraphTextureHandle m_hHDRIn;           ///< ShaderResource in (immutable pre-refraction HDR snapshot).
  xiiRenderGraphTextureHandle m_hHDROut;          ///< UnorderedAccess out (authoritative HDR scene color target).
  xiiRenderGraphBufferHandle  m_hConstants;       ///< ConstantBuffer in (refraction tuning parameters).
  xiiSSRefractionConstants    m_Constants;
};

void xiiView::SetupScreenSpaceRefraction(xiiScreenSpaceRefractionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRIn           = builder.ReadTexture(xiiRGBlackboardKeys::k_RefractionSceneColorInput, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDROut          = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRefractionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRefractionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  const xiiLightingSystemSettings& settings = m_ViewPassResources->m_LightingSystem.GetSettings();
  data.m_Constants.PerturbScale             = xiiMath::Clamp(settings.m_fSSRefractionScale, 0.0f, 0.5f);
  data.m_Constants.MaxDistance              = xiiMath::Clamp(settings.m_fSSRefractionMaxDistance, 0.0f, 0.5f);
  data.m_Constants.Chromatic                = xiiMath::Clamp(settings.m_fSSRefractionChromatic, 0.0f, 0.1f);
  data.m_Constants.Padding                  = 0.0f;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_ScreenSpacePasses.m_pSSRefractionPipeline, "Shaders/Pipeline/SSRefraction.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiView::ExecuteScreenSpaceRefraction(const xiiScreenSpaceRefractionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("SSRefraction");
  {
    {
      xiiGALMapHelper<xiiSSRefractionConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_ScreenSpacePasses.m_pSSRefractionPipeline);
    m_ViewPassResources->m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRefractionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
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

  if (!m_ViewPassResources->m_ScreenSpacePasses.m_pPlanarReflectionTarget)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = xiiMath::Max(1U, uiRenderWidth / 2U);
    description.m_Size.height = xiiMath::Max(1U, uiRenderHeight / 2U);
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    m_ViewPassResources->m_ScreenSpacePasses.m_pPlanarReflectionTarget = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hPlanarTarget = builder.ImportTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, m_ViewPassResources->m_ScreenSpacePasses.m_pPlanarReflectionTarget, xiiGALResourceStateFlags::RenderTarget);
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
  xiiRenderGraphBufferHandle  m_hConstants;
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

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureHistogramConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Histogram Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TemporalPasses.m_pLuminanceHistogramPipeline, "Shaders/Pipeline/ExposureHistogram.xiiShader");
}

void xiiView::ExecuteLuminanceHistogram(const xiiLuminanceHistogramData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("LuminanceHistogram");
  {
    const xiiExposureSettings& settings = m_DisplayOutputSettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureHistogramConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, 1.0f / fRange, fRange);
      pConstants->InputResolution   = xiiVec2U32(uiRenderWidth, uiRenderHeight);
      pConstants->_Padding          = xiiVec2::MakeZero();
    }

    xiiUInt32 zeroHistogram[256] = {};
    cmd.UpdateBuffer(context.GetBuffer(data.m_hHistogram), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroHistogram), sizeof(zeroHistogram)));
    cmd.SetPipelineState(m_ViewPassResources->m_TemporalPasses.m_pLuminanceHistogramPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiExposureHistogramConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRInput", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 15U) / 16U, (uiRenderHeight + 15U) / 16U, 1U});
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
  xiiRenderGraphBufferHandle m_hConstants;
  bool                       m_bHistoryValid = false;
};

void xiiView::SetupAutoExposure(xiiAutoExposureData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHistogram = builder.ReadBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, xiiGALResourceStateFlags::ShaderResource);

  if (!m_ViewPassResources->m_TemporalPasses.m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = 4U;
    description.m_uiSize              = sizeof(float) * 2U;
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_ViewPassResources->m_TemporalPasses.m_pExposureBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_hExposure     = builder.ImportBuffer(xiiRGBlackboardKeys::k_CurrentExposure, m_ViewPassResources->m_TemporalPasses.m_pExposureBuffer, m_ViewPassResources->m_TemporalPasses.m_pExposureBuffer->GetResourceState());
  data.m_hExposure     = builder.WriteBuffer(data.m_hExposure, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_bHistoryValid = m_ViewPassResources->m_TemporalPasses.m_bExposureHistoryValid;

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureAdaptationConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Adaptation Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TemporalPasses.m_pAutoExposurePipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader");
}

void xiiView::ExecuteAutoExposure(const xiiAutoExposureData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AutoExposure");
  {
    const xiiExposureSettings& settings = m_DisplayOutputSettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureAdaptationConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, fRange, 0.0f);
      pConstants->Metering          = xiiVec4(settings.m_fLowPercentile, settings.m_fHighPercentile, settings.m_fAdaptationSpeedBright, settings.m_fAdaptationSpeedDark);
      pConstants->Exposure          = xiiVec4(xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 0.25f), settings.m_fMinimumEV100, settings.m_fMaximumEV100, settings.m_fExposureCompensation);
      pConstants->ManualExposure    = GetCamera() != nullptr ? GetCamera()->GetExposure() : 1.0f;
      pConstants->AutomaticExposure = settings.m_Mode == xiiExposureMode::Automatic ? 1U : 0U;
      pConstants->HistoryValid      = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding          = 0.0f;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_TemporalPasses.m_pAutoExposurePipeline);
    cmd.ResolveAndSetConstantBuffer("xiiExposureAdaptationConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({1U, 1U, 1U});
    m_ViewPassResources->m_TemporalPasses.m_bExposureHistoryValid = true;
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
  xiiRenderGraphTextureHandle m_hDepth;    ///< ShaderResource in (reversed-Z scene depth).
  xiiRenderGraphTextureHandle m_hHistory;  ///< ShaderResource in (history color).
  xiiRenderGraphTextureHandle m_hTAAOut;   ///< UnorderedAccess out (TAA resolved color).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiTAAConstants             m_Constants           = {};
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  bool                        m_bHistoryValid       = false;
};

void xiiView::SetupTemporalAntiAliasing(xiiTemporalAntiAliasingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = GetRenderResolutionHeight();

  data.m_hHDRIn    = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  auto& temporalResources = m_ViewPassResources->m_TemporalPasses;
  bool  bRecreateHistory  = false;
  for (const xiiSharedPtr<xiiGALTexture>& pHistory : temporalResources.m_pTAAHistoryBuffers)
  {
    bRecreateHistory |= pHistory == nullptr || pHistory->GetDescription().m_Size != xiiSizeU32(uiRenderWidth, uiRenderHeight);
  }

  if (bRecreateHistory)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size        = xiiSizeU32(uiRenderWidth, uiRenderHeight);
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    for (xiiSharedPtr<xiiGALTexture>& pHistory : temporalResources.m_pTAAHistoryBuffers)
    {
      if (pHistory != nullptr)
        temporalResources.m_RetiredTAAHistoryBuffers.PushBack(pHistory);

      pHistory = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    }

    temporalResources.m_uiTAAHistoryWriteIndex = 0U;
    temporalResources.m_bTAAHistoryValid       = false;
  }

  data.m_uiHistoryWriteIndex = temporalResources.m_uiTAAHistoryWriteIndex;
  data.m_bHistoryValid       = temporalResources.m_bTAAHistoryValid;

  const xiiUInt32 uiHistoryReadIndex = 1U - data.m_uiHistoryWriteIndex;
  data.m_hHistory                    = builder.ImportTexture("TAAHistory", temporalResources.m_pTAAHistoryBuffers[uiHistoryReadIndex], temporalResources.m_pTAAHistoryBuffers[uiHistoryReadIndex]->GetResourceState());
  data.m_hHistory                    = builder.ReadTexture(data.m_hHistory, xiiGALResourceStateFlags::ShaderResource);

  data.m_hTAAOut = builder.ImportTexture(xiiRGBlackboardKeys::k_TAAResolvedColor, temporalResources.m_pTAAHistoryBuffers[data.m_uiHistoryWriteIndex], temporalResources.m_pTAAHistoryBuffers[data.m_uiHistoryWriteIndex]->GetResourceState());
  data.m_hTAAOut = builder.WriteTexture(data.m_hTAAOut, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiTAAConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("TAA Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_Constants.HistoryValid     = data.m_bHistoryValid ? 1U : 0U;
  data.m_Constants.BaseBlendAlpha   = 0.08f;
  data.m_Constants.MotionBlendScale = 12.0f;
  data.m_Constants.Padding          = 0.0f;

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TemporalPasses.m_pTAAPipeline, "Shaders/Pipeline/TAA.xiiShader");
}

void xiiView::ExecuteTemporalAntiAliasing(const xiiTemporalAntiAliasingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = GetRenderResolutionHeight();

  cmd.BeginDebugGroup("TAA");
  {
    {
      xiiGALMapHelper<xiiTAAConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }

    cmd.SetPipelineState(m_ViewPassResources->m_TemporalPasses.m_pTAAPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiTAAConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentFrame", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VelocityBuffer", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HistoryFrame", context.GetTexture(data.m_bHistoryValid ? data.m_hHistory : data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Resolved", context.GetTexture(data.m_hTAAOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});

    m_ViewPassResources->m_TemporalPasses.m_uiTAAHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
    m_ViewPassResources->m_TemporalPasses.m_bTAAHistoryValid       = true;
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
  const xiiUInt32 uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32 uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  data.m_hTAAIn = builder.ReadTexture(xiiRGBlackboardKeys::k_TAAResolvedColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hUpscaled          = builder.WriteTexture(xiiRGBlackboardKeys::k_UpscaledColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_TemporalPasses.m_pUpscalePipeline, "Shaders/Pipeline/CASUpscale.xiiShader");
}

void xiiView::ExecuteUpscale(const xiiUpscaleData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32    uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  cmd.BeginDebugGroup("Upscale");
  {
    cmd.SetPipelineState(m_ViewPassResources->m_TemporalPasses.m_pUpscalePipeline);
    cmd.ResolveAndSetShaderResourceTextureView("g_TAAIn", context.GetTexture(data.m_hTAAIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Upscaled", context.GetTexture(data.m_hUpscaled)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
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
  xiiRenderGraphBufferHandle  m_hConstants;
};

void xiiView::SetupBloom(xiiBloomData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32 uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  data.m_hHDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_UpscaledColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hBloom             = builder.WriteTexture(xiiRGBlackboardKeys::k_BloomTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiBloomConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Bloom Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_PostProcessPasses.m_pBloomPipeline, "Shaders/Pipeline/BloomChain.xiiShader");
}

void xiiView::ExecuteBloom(const xiiBloomData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32    uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  cmd.BeginDebugGroup("Bloom");
  {
    {
      xiiGALMapHelper<xiiBloomConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->BloomParameters = xiiVec4(m_DisplayOutputSettings.m_fBloomThreshold, m_DisplayOutputSettings.m_fBloomKnee, m_DisplayOutputSettings.m_fBloomRadius, 0.0f);
    }

    cmd.SetPipelineState(m_ViewPassResources->m_PostProcessPasses.m_pBloomPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiBloomConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BloomOut", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Color Grading Data //////////
//
// Collects all GPU resources related to color grading.

struct xiiColorGradingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDisplayLinear; ///< ShaderResource in (tone-mapped display-linear input).
  xiiRenderGraphTextureHandle m_hGraded;        ///< UnorderedAccess out (graded display-linear output).
  xiiRenderGraphBufferHandle  m_hConstants;
};

void xiiView::SetupColorGrading(xiiColorGradingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32 uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  data.m_hDisplayLinear = builder.ReadTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hGraded            = builder.WriteTexture(xiiRGBlackboardKeys::k_GradedColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiColorGradingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Color Grading Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_PostProcessPasses.m_pColorGradingPipeline, "Shaders/Pipeline/ColorGrading.xiiShader");
}

void xiiView::ExecuteColorGrading(const xiiColorGradingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32    uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  cmd.BeginDebugGroup("ColorGrading");
  {
    const xiiColorGradingSettings& settings = m_DisplayOutputSettings.m_ColorGrading;
    {
      xiiGALMapHelper<xiiColorGradingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ColorAdjustments = xiiVec4(settings.m_fSaturation, settings.m_fContrast, settings.m_fVignetteStrength, settings.m_fVignetteRoundness);
      pConstants->FilmGrain        = xiiVec4(settings.m_fFilmGrainStrength, static_cast<float>(context.GetFrameIndex() & 0x00FFFFFFU), 0.0f, 0.0f);
    }

    cmd.SetPipelineState(m_ViewPassResources->m_PostProcessPasses.m_pColorGradingPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiColorGradingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DisplayLinear", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GradedOut", context.GetTexture(data.m_hGraded)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Tone Mapping Data //////////
//
// Collects all GPU resources related to tone mapping from HDR to LDR.

struct xiiToneMappingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRInput;
  xiiRenderGraphTextureHandle m_hBloom;
  xiiRenderGraphBufferHandle  m_hExposure;
  xiiRenderGraphTextureHandle m_hDisplayLinear;
  xiiRenderGraphBufferHandle  m_hConstants;
};

void xiiView::SetupToneMapping(xiiToneMappingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32 uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  data.m_hHDRInput = builder.ReadTexture(xiiRGBlackboardKeys::k_UpscaledColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBloom    = builder.ReadTexture(xiiRGBlackboardKeys::k_BloomTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hExposure = builder.ReadBuffer(xiiRGBlackboardKeys::k_CurrentExposure, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDisplayLinear     = builder.WriteTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiToneMappingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Tone Mapping Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiView::EnsureComputePipeline(m_ViewPassResources->m_PostProcessPasses.m_pToneMappingPipeline, "Shaders/Pipeline/ToneMapping.xiiShader");
}

void xiiView::ExecuteToneMapping(const xiiToneMappingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().width, 1.0f));
  const xiiUInt32    uiOutputHeight = static_cast<xiiUInt32>(xiiMath::Max(GetViewport().height, 1.0f));

  cmd.BeginDebugGroup("ToneMapping");
  {
    {
      xiiGALMapHelper<xiiToneMappingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->Operator           = m_DisplayOutputSettings.m_ToneMappingOperator.GetValue();
      pConstants->OutputMode         = m_DisplayOutputSettings.m_OutputMode.GetValue();
      pConstants->BloomStrength      = m_DisplayOutputSettings.m_fBloomStrength;
      pConstants->PaperWhiteNits     = m_DisplayOutputSettings.m_fPaperWhiteNits;
      pConstants->MaximumDisplayNits = m_DisplayOutputSettings.m_fMaximumDisplayNits;
      pConstants->_Padding           = xiiVec3::MakeZero();
    }

    cmd.SetPipelineState(m_ViewPassResources->m_PostProcessPasses.m_pToneMappingPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiToneMappingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRInput", context.GetTexture(data.m_hHDRInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BloomTex", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_LDROut", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Final Blit Data //////////
//
// Collects all GPU resources related to final backbuffer presentation.

struct xiiFinalBlitData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDisplayLinear; ///< ShaderResource in (tone-mapped scene-linear display signal).
  xiiRenderGraphTextureHandle m_hBackbuffer;    ///< RenderTarget out (swapchain backbuffer).
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bApplySRGBTransfer = false;
};

void xiiView::SetupFinalBlit(xiiFinalBlitData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDisplayLinear = builder.ReadTexture(xiiRGBlackboardKeys::k_GradedColor, xiiGALResourceStateFlags::ShaderResource);

  if (const xiiGALSwapChain* pSwapChain = GetSwapChain(); pSwapChain != nullptr)
  {
    xiiSharedPtr<xiiGALTexture> pBackbufferTexture = pSwapChain->GetBackBufferTexture();
    if (pBackbufferTexture)
    {
      data.m_hBackbuffer        = builder.ImportTexture("Backbuffer", pBackbufferTexture, xiiGALResourceStateFlags::RenderTarget);
      data.m_hBackbuffer        = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
      data.m_bApplySRGBTransfer = !xiiGALResourceFormat::IsSrgb(pBackbufferTexture->GetDescription().m_Format);
    }
  }

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiFinalBlitConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Final Blit Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
  builder.SetPassRenderPassManaged(data.m_hBackbuffer.IsValid());
}

void xiiView::ExecuteFinalBlit(const xiiFinalBlitData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList&                        cmd = context.GetCommandList();
  xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline;
  if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && context.GetRenderPass() != nullptr)
  {
    pPipeline = xiiView::EnsureGraphicsPipeline(m_ViewPassResources->m_OutputPasses.m_pFinalBlitPipeline, "Shaders/Pipeline/FinalBlit.xiiShader", context.GetRenderPass(), context.GetSubpassIndex());
  }

  cmd.BeginDebugGroup("BackbufferPresent");
  {
    if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && pPipeline != nullptr)
    {
      {
        xiiGALMapHelper<xiiFinalBlitConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        pConstants->OutputMode         = m_DisplayOutputSettings.m_OutputMode.GetValue();
        pConstants->ApplySRGBTransfer  = data.m_bApplySRGBTransfer ? 1U : 0U;
        pConstants->PaperWhiteNits     = m_DisplayOutputSettings.m_fPaperWhiteNits;
        pConstants->MaximumDisplayNits = m_DisplayOutputSettings.m_fMaximumDisplayNits;
      }

      cmd.ClearRenderTargetView(context.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
      cmd.SetViewport({0.0f, 0.0f, m_Data.m_ViewPortRect.width, m_Data.m_ViewPortRect.height, 0.0f, 1.0f});
      cmd.SetPipelineState(pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiFinalBlitConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_FinalColor", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
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

  const xiiRayTracingSceneManager::BuildHandles rayTracingScene      = xiiRayTracingSceneManager::AddBuildPass(graph, uiFrameIndex);
  m_ViewPassResources->m_LightingPasses.m_pRayTracingScene           = rayTracingScene.m_pTopLevelAS;
  m_ViewPassResources->m_LightingPasses.m_hRayTracingSceneDependency = rayTracingScene.m_hSceneDependency;
  m_ViewPassResources->m_LightingPasses.m_hRayTracingMaterialData    = rayTracingScene.m_hMaterialData;
  m_ViewPassResources->m_LightingPasses.m_hRayTracingGeometryData    = rayTracingScene.m_hGeometryData;

  if (m_pExtractedData != nullptr)
  {
    m_ViewPassResources->m_LightingSystem.BuildFrameData(*this, *m_pExtractedData, uiFrameIndex);
  }

  xiiAtmosphereLUTHandle& hAtmosphereLUT = m_ViewPassResources->m_LightingPrepPasses.m_hAtmosphereLUT;
  hAtmosphereLUT                         = xiiAtmosphereManager::GetDefaultLUTHandle();
  if (m_pExtractedData != nullptr)
  {
    if (const xiiSkyAtmosphereRenderData* pAtmosphere = SelectSkyAtmosphere(m_pExtractedData->GetAllRenderData()))
    {
      if (xiiAtmosphereManager::AcquireLUTs(pAtmosphere->m_AtmosphereSettings, hAtmosphereLUT).Failed())
      {
        hAtmosphereLUT = xiiAtmosphereManager::GetDefaultLUTHandle();
      }
    }
  }

  auto& cloudState = m_ViewPassResources->m_LightingPasses.m_CloudShadowState;
  cloudState       = {};
  if (m_pExtractedData != nullptr)
  {
    if (const xiiVolumetricCloudRenderData* pCloud = SelectVolumetricCloudLayer(m_pExtractedData->GetAllRenderData()))
    {
      const xiiVolumetricCloudSettings& settings  = pCloud->m_Settings;
      const float                       fInvScale = 1.0f / xiiMath::Max(settings.m_fShadowScaleMeters, 1.0f);
      cloudState.m_vLayerOriginAndInvScale        = xiiVec4(pCloud->m_vLayerOrigin.x, pCloud->m_vLayerOrigin.y, pCloud->m_vLayerOrigin.z, fInvScale);
      cloudState.m_vProjectionAxisUAndDetail      = xiiVec4(pCloud->m_vProjectionAxisU.x, pCloud->m_vProjectionAxisU.y, pCloud->m_vProjectionAxisU.z, xiiMath::Max(settings.m_fDetailScale, 1.0f));
      cloudState.m_vProjectionAxisVAndCoverage    = xiiVec4(pCloud->m_vProjectionAxisV.x, pCloud->m_vProjectionAxisV.y, pCloud->m_vProjectionAxisV.z, xiiMath::Saturate(settings.m_fCoverage));
      cloudState.m_vLayerNormalAndOpticalDepth    = xiiVec4(pCloud->m_vLayerNormal.x, pCloud->m_vLayerNormal.y, pCloud->m_vLayerNormal.z, xiiMath::Max(settings.m_fOpticalDepth, 0.0f));
      cloudState.m_vWindStrengthAndEnabled        = xiiVec4(settings.m_vWindVelocityMetersPerSecond.x, settings.m_vWindVelocityMetersPerSecond.y, xiiMath::Saturate(settings.m_fShadowStrength), settings.m_bCastShadows ? 1.0f : 0.0f);
    }
  }
  m_ViewPassResources->m_LightingSystem.WriteBlackboard(blackboard);
  m_ViewPassResources->m_LightingPasses.m_uiFrameIndex = uiFrameIndex;

  // Each stage adds its passes to the graph. Dependency ordering is handled by the render graph compiler (topological sort + culling).

  graph.AddPass<xiiLightingDataUploadData>("LightingDataUpload", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLightingDataUpload, this), xiiMakeDelegate(&xiiView::ExecuteLightingDataUpload, this));

  // Visibility preparation passes, which produce data consumed by the main render passes in later stages.
  graph.AddPass<xiiOcclusionReadbackData>("GpuOcclusionReadback", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupOcclusionReadback, this), xiiMakeDelegate(&xiiView::ExecuteOcclusionReadback, this));
  graph.AddPass<xiiFrustumCullData>("FrustumCulling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupFrustumCull, this), xiiMakeDelegate(&xiiView::ExecuteFrustumCull, this));
  graph.AddPass<xiiLODSelectData>("LODSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLODSelect, this), xiiMakeDelegate(&xiiView::ExecuteLODSelect, this));
  graph.AddPass<xiiInstanceUpdateData>("InstanceUpdate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupInstanceUpdate, this), xiiMakeDelegate(&xiiView::ExecuteInstanceUpdate, this));
  graph.AddPass<xiiDrawBuildData>("CoarseDrawCommandBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupCoarseDrawBuild, this), xiiMakeDelegate(&xiiView::ExecuteDrawBuild, this));
  graph.AddPass<xiiClusterBuildData>("ClusterGridBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupClusterBuild, this), xiiMakeDelegate(&xiiView::ExecuteClusterBuild, this));
  graph.AddPass<xiiLightListClearData>("LightListClear", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLightListClear, this), xiiMakeDelegate(&xiiView::ExecuteLightListClear, this));
  graph.AddPass<xiiLightListData>("LightListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLightListBuild, this), xiiMakeDelegate(&xiiView::ExecuteLightListBuild, this));
  graph.AddPass<xiiReflectionProbeSelectData>("ReflectionProbeSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReflectionProbeSelect, this), xiiMakeDelegate(&xiiView::ExecuteReflectionProbeSelect, this));
  graph.AddPass<xiiFroxelAllocationData>("VolumetricGridAllocation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupFroxelAllocation, this), xiiMakeDelegate(&xiiView::ExecuteFroxelAllocation, this));

  // Shadow preparation passes, which produce data consumed by the main shadow pass in later stages.
  auto shadowCascadePass = graph.AddPass<xiiShadowCascadeSetupData>("ShadowCascadeSetup", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowCascadeSetup, this), xiiMakeDelegate(&xiiView::ExecuteShadowCascadeSetup, this));
  graph.AddPass<xiiShadowCasterBuildData>("ShadowCasterListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowCasterBuild, this), xiiMakeDelegate(&xiiView::ExecuteShadowCasterBuild, this));
  graph.AddPass<xiiLocalShadowAtlasAllocationData>("LocalShadowAtlasUpload", xiiGALCommandQueueFlags::Transfer, xiiMakeDelegate(&xiiView::SetupLocalShadowAtlasAllocation, this), xiiMakeDelegate(&xiiView::ExecuteLocalShadowAtlasAllocation, this));
  graph.AddPass<xiiDirectionalShadowData>("DirectionalShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDirectionalShadowData, this), xiiMakeDelegate(&xiiView::ExecuteDirectionalShadowData, this));
  graph.AddPass<xiiSpotShadowData>("SpotShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupSpotShadowData, this), xiiMakeDelegate(&xiiView::ExecuteSpotShadowData, this));
  graph.AddPass<xiiPointShadowData>("PointShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupPointShadowData, this), xiiMakeDelegate(&xiiView::ExecutePointShadowData, this));
  graph.AddPass<xiiContactShadowData>("ContactShadow", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupContactShadowData, this), xiiMakeDelegate(&xiiView::ExecuteContactShadowData, this));

  // Depth and motion prepasses, which produce depth and motion data consumed by later passes.
  auto depthPrepass = graph.AddPass<xiiDepthPrepassData>("DepthPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupDepthPrepass, this), xiiMakeDelegate(&xiiView::ExecuteDepthPrepass, this));
  if (m_pExtractedData != nullptr)
  {
    const xiiDirectionalLightRenderData* pMainDirectional = SelectMainDirectionalLight(m_pExtractedData->GetAllRenderData());
    if (pMainDirectional != nullptr && pMainDirectional->m_bCastShadows)
    {
      xiiVirtualShadowMapManager::AddFeedbackPasses(graph, depthPrepass.first->m_hSceneDepth, shadowCascadePass.first->m_hCascadeMatrices,
                                                    GetRenderResolutionWidth(), GetRenderResolutionHeight(), GetInverseViewProjectionMatrix(xiiCameraEye::Left),
                                                    m_pCamera != nullptr ? m_pCamera->GetNearPlane() : 0.1f,
                                                    static_cast<xiiUInt32>(pMainDirectional->m_uiSortingKey), uiFrameIndex);
    }
  }
  graph.AddPass<xiiHiZPyramidData>("HiZPyramid", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZPyramid, this), xiiMakeDelegate(&xiiView::ExecuteHiZPyramid, this));
  graph.AddPass<xiiHiZOcclusionCullData>("HiZOcclusionCull", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHiZOcclusionCull, this), xiiMakeDelegate(&xiiView::ExecuteHiZOcclusionCull, this));
  graph.AddPass<xiiDrawBuildData>("DrawCommandBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDrawBuild, this), xiiMakeDelegate(&xiiView::ExecuteDrawBuild, this));
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
  graph.AddPass<xiiVolumetricFogInitializationData>("VolumetricFogInitialization", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogInitialization, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogInitialization, this));
  XII_IGNORE_UNUSED(xiiSparseVoxelRadianceManager::AddUpdatePass(graph, m_ViewPassResources->m_LightingSystem.BorrowSystem()));
  graph.AddPass<xiiSparseVoxelRadianceGatherData>("SparseVoxelRadianceGather", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupSparseVoxelRadianceGather, this), xiiMakeDelegate(&xiiView::ExecuteSparseVoxelRadianceGather, this));
  XII_IGNORE_UNUSED(xiiDDGIManager::AddUpdatePass(graph, m_ViewPassResources->m_LightingSystem.BorrowSystem()));
  graph.AddPass<xiiDDGIProbeSamplingData>("DDGIProbeSampling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDDGIProbeSampling, this), xiiMakeDelegate(&xiiView::ExecuteDDGIProbeSampling, this));
  graph.AddPass<xiiGroundTruthAmbientOcclusionData>("GroundTruthAmbientOcclusion", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGroundTruthAmbientOcclusion, this), xiiMakeDelegate(&xiiView::ExecuteGroundTruthAmbientOcclusion, this));

  // Main lighting passes, which produce direct and indirect lighting results.
  graph.AddPass<xiiReSTIRDITemporalData>("ReSTIRDITemporal", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReSTIRDITemporal, this), xiiMakeDelegate(&xiiView::ExecuteReSTIRDITemporal, this));
  graph.AddPass<xiiReSTIRDISpatialData>("ReSTIRDISpatial", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReSTIRDISpatial, this), xiiMakeDelegate(&xiiView::ExecuteReSTIRDISpatial, this));
  graph.AddPass<xiiRayTracedShadowData>("RayTracedShadowData", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedShadowData, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedShadowData, this));
  graph.AddPass<xiiShadowDenoiseData>("ShadowDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupShadowDenoiseData, this), xiiMakeDelegate(&xiiView::ExecuteShadowDenoiseData, this));
  graph.AddPass<xiiGroundTruthAmbientOcclusionDenoiseData>("GroundTruthAmbientOcclusionDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGroundTruthAmbientOcclusionDenoise, this), xiiMakeDelegate(&xiiView::ExecuteGroundTruthAmbientOcclusionDenoise, this));
  graph.AddPass<xiiDeferredDirectLightingData>("DeferredDirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupDirectLighting, this), xiiMakeDelegate(&xiiView::ExecuteDirectLighting, this));
  graph.AddPass<xiiDeferredIndirectLightingData>("DeferredIndirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupIndirectLighting, this), xiiMakeDelegate(&xiiView::ExecuteIndirectLighting, this));
  graph.AddPass<xiiRayTracedGlobalIlluminationData>("RTGIFinalGather", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedGlobalIllumination, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedGlobalIllumination, this));
  graph.AddPass<xiiReSTIRGITemporalData>("ReSTIRGITemporal", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReSTIRGITemporal, this), xiiMakeDelegate(&xiiView::ExecuteReSTIRGITemporal, this));
  graph.AddPass<xiiReSTIRGISpatialData>("ReSTIRGISpatial", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupReSTIRGISpatial, this), xiiMakeDelegate(&xiiView::ExecuteReSTIRGISpatial, this));
  graph.AddPass<xiiRayTracedGlobalIlluminationDenoiseData>("RTGITemporalDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedGlobalIlluminationDenoise, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedGlobalIlluminationDenoise, this));
  graph.AddPass<xiiRayTracedReflectionsData>("RTReflections", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedReflections, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedReflections, this));
  graph.AddPass<xiiRayTracedReflectionsDenoiseData>("RTReflectionTemporalDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupRayTracedReflectionsDenoise, this), xiiMakeDelegate(&xiiView::ExecuteRayTracedReflectionsDenoise, this));
  graph.AddPass<xiiVolumetricLightInjectionData>("VolumetricLightInjection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricLightInjection, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricLightInjection, this));
  graph.AddPass<xiiVolumetricFogIntegrationData>("VolumetricFogIntegrate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogIntegration, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogIntegration, this));
  graph.AddPass<xiiVolumetricFogResolveData>("VolumetricFogResolve", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogResolve, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogResolve, this));
  graph.AddPass<xiiVolumetricFogTemporalReprojectionData>("VolumetricFogTemporalRep", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupVolumetricFogTemporalReprojection, this), xiiMakeDelegate(&xiiView::ExecuteVolumetricFogTemporalReprojection, this));

  // Forward rendering passes, which composite main scene color from lighting buffers and forward geometry.
  graph.AddPass<xiiForwardOpaqueData>("ForwardOpaque", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupForwardOpaque, this), xiiMakeDelegate(&xiiView::ExecuteForwardOpaque, this));
  graph.AddPass<xiiForwardMaskedData>("ForwardMasked", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupForwardMasked, this), xiiMakeDelegate(&xiiView::ExecuteForwardMasked, this));
  graph.AddPass<xiiHairRenderingData>("HairRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupHairRendering, this), xiiMakeDelegate(&xiiView::ExecuteHairRendering, this));
  graph.AddPass<xiiWaterRenderingData>("WaterRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupWaterRendering, this), xiiMakeDelegate(&xiiView::ExecuteWaterRendering, this));
  graph.AddPass<xiiSubsurfaceScatteringData>("SubsurfaceScattering", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupSubsurfaceScattering, this), xiiMakeDelegate(&xiiView::ExecuteSubsurfaceScattering, this));
  graph.AddPass<xiiEyeShaderData>("EyeShader", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupEyeShader, this), xiiMakeDelegate(&xiiView::ExecuteEyeShader, this));

  // Resolve screen-space rays against complete opaque scene radiance, then
  // replace Tier 2 environment specular with the best valid Tier 3 / Tier 1
  // result. This ordering avoids the previous HDR read-before-create cycle.
  graph.AddPass<xiiScreenSpaceReflectionsData>("SSR", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceReflections, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceReflections, this));
  graph.AddPass<xiiHybridReflectionCompositeData>("HybridReflectionComposite", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupHybridReflectionComposite, this), xiiMakeDelegate(&xiiView::ExecuteHybridReflectionComposite, this));

  // Transparency and special material passes.
  graph.AddPass<xiiGPUParticleSimulateData>("GPUParticleSimulate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupGPUParticleSimulate, this), xiiMakeDelegate(&xiiView::ExecuteGPUParticleSimulate, this));
  graph.AddPass<xiiWeightedBlendedOITData>("WeightedBlendedOIT", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupWeightedBlendedOIT, this), xiiMakeDelegate(&xiiView::ExecuteWeightedBlendedOIT, this));

  // Screen-space effects.
  const xiiLightingSystemSettings& lightingSettings = m_ViewPassResources->m_LightingSystem.GetSettings();
  if (!m_ViewPassResources->m_LightingPasses.m_bRTGIAvailableThisFrame && lightingSettings.m_fSSGIIntensity > 0.0f && lightingSettings.m_fSSGIRayLength > 0.0f)
  {
    graph.AddPass<xiiScreenSpaceGlobalIlluminationData>("SSGI", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceGlobalIllumination, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceGlobalIllumination, this));
    graph.AddPass<xiiScreenSpaceGlobalIlluminationCompositeData>("SSGIComposite", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceGlobalIlluminationComposite, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceGlobalIlluminationComposite, this));
  }
  if (lightingSettings.m_fSSRefractionScale > 0.0f && lightingSettings.m_fSSRefractionMaxDistance > 0.0f)
  {
    graph.AddPass<xiiScreenSpaceRefractionSnapshotData>("SSRefractionSnapshot", xiiGALCommandQueueFlags::Transfer, xiiMakeDelegate(&xiiView::SetupScreenSpaceRefractionSnapshot, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceRefractionSnapshot, this));
    graph.AddPass<xiiScreenSpaceRefractionData>("SSRefraction", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupScreenSpaceRefraction, this), xiiMakeDelegate(&xiiView::ExecuteScreenSpaceRefraction, this));
  }
  graph.AddPass<xiiPlanarReflectionsData>("PlanarReflections", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupPlanarReflections, this), xiiMakeDelegate(&xiiView::ExecutePlanarReflections, this));
  graph.AddPass<xiiAtmosphereCompositeData>("AtmosphereComposite", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAtmosphereComposite, this), xiiMakeDelegate(&xiiView::ExecuteAtmosphereComposite, this));

  if (m_ViewPassResources->m_OutputPasses.m_hSensorProfile.IsValid())
  {
    graph.AddPass<xiiSensorOutputData>("CalibratedSensorOutput", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupSensorOutput, this), xiiMakeDelegate(&xiiView::ExecuteSensorOutput, this));
  }

  // Temporal reconstruction passes.
  graph.AddPass<xiiLuminanceHistogramData>("LuminanceHistogram", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupLuminanceHistogram, this), xiiMakeDelegate(&xiiView::ExecuteLuminanceHistogram, this));
  graph.AddPass<xiiAutoExposureData>("AutoExposure", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupAutoExposure, this), xiiMakeDelegate(&xiiView::ExecuteAutoExposure, this));
  graph.AddPass<xiiTemporalAntiAliasingData>("TAA", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupTemporalAntiAliasing, this), xiiMakeDelegate(&xiiView::ExecuteTemporalAntiAliasing, this));
  graph.AddPass<xiiUpscaleData>("Upscale", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupUpscale, this), xiiMakeDelegate(&xiiView::ExecuteUpscale, this));

  // Post-processing passes.
  graph.AddPass<xiiBloomData>("Bloom", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupBloom, this), xiiMakeDelegate(&xiiView::ExecuteBloom, this));
  graph.AddPass<xiiToneMappingData>("ToneMapping", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupToneMapping, this), xiiMakeDelegate(&xiiView::ExecuteToneMapping, this));

  // Debug and visualization passes.
  xiiDebugRenderer::AddRenderGraphPasses(graph);

  // Display-linear grading follows debug composition so overlays remain visible at presentation.
  graph.AddPass<xiiColorGradingData>("ColorGrading", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiView::SetupColorGrading, this), xiiMakeDelegate(&xiiView::ExecuteColorGrading, this));

  // Final output pass.
  graph.AddPass<xiiFinalBlitData>("BackbufferPresent", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiView::SetupFinalBlit, this), xiiMakeDelegate(&xiiView::ExecuteFinalBlit, this));
}

bool xiiView::EnsureRayTracingShadowResources()
{
  auto& shadowPasses = m_ViewPassResources->m_ShadowPasses;
  if (shadowPasses.m_pRayTracedShadowPipeline != nullptr && shadowPasses.m_pRayTracedShadowShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTShadow.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
  xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-traced shadow shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware shadow permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTShadowRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTShadowMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTShadowTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  shadowPasses.m_pRayTracedShadowPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (shadowPasses.m_pRayTracedShadowPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-traced shadow pipeline.");
    return false;
  }
  shadowPasses.m_pRayTracedShadowPipeline->SetDebugName("RT Shadow Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid shadow shader binding table alignment properties.");
    shadowPasses.m_pRayTracedShadowPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                           = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                        = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                            = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                             = xiiGALBufferMode::Raw;
  shadowPasses.m_pRayTracedShadowShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (shadowPasses.m_pRayTracedShadowShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware shadow shader binding table.");
    shadowPasses.m_pRayTracedShadowPipeline.Clear();
    return false;
  }

  shadowPasses.m_pRayTracedShadowShaderBindingTable->SetDebugName("RT Shadow Shader Binding Table");
  shadowPasses.m_uiRayTracedShadowShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiView::EnsureRayTracingAmbientOcclusionResources()
{
  auto& lightingPrepPasses = m_ViewPassResources->m_LightingPrepPasses;
  if (lightingPrepPasses.m_pRTAOPipeline != nullptr && lightingPrepPasses.m_pRTAOShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTAO.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
  xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the ray-traced ambient-occlusion shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The ambient-occlusion permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTAORayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTAOMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTAOTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPrepPasses.m_pRTAOPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPrepPasses.m_pRTAOPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the ray-traced ambient-occlusion pipeline.");
    return false;
  }
  lightingPrepPasses.m_pRTAOPipeline->SetDebugName("RT AO Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid ambient-occlusion shader binding table alignment properties.");
    lightingPrepPasses.m_pRTAOPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                      = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                   = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                       = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                        = xiiGALBufferMode::Raw;
  lightingPrepPasses.m_pRTAOShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPrepPasses.m_pRTAOShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the ambient-occlusion shader binding table.");
    lightingPrepPasses.m_pRTAOPipeline.Clear();
    return false;
  }

  lightingPrepPasses.m_pRTAOShaderBindingTable->SetDebugName("RT AO Shader Binding Table");
  lightingPrepPasses.m_uiRTAOShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiView::EnsureRayTracingGlobalIlluminationResources()
{
  auto& lightingPasses = m_ViewPassResources->m_LightingPasses;
  if (lightingPasses.m_pRTGIPipeline != nullptr && lightingPasses.m_pRTGIShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTGIFinalGather.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
  xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-traced GI shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware GI permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = 16U;
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTGIRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTGIMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTGITriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPasses.m_pRTGIPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPasses.m_pRTGIPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-traced GI pipeline.");
    return false;
  }
  lightingPasses.m_pRTGIPipeline->SetDebugName("RT GI Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid GI shader binding table alignment properties.");
    lightingPasses.m_pRTGIPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                  = uiRecordStride * 3U;
  sbtDescription.m_BindFlags               = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                   = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                    = xiiGALBufferMode::Raw;
  lightingPasses.m_pRTGIShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPasses.m_pRTGIShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware GI shader binding table.");
    lightingPasses.m_pRTGIPipeline.Clear();
    return false;
  }

  lightingPasses.m_pRTGIShaderBindingTable->SetDebugName("RT GI Shader Binding Table");
  lightingPasses.m_uiRTGIShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiView::EnsureRayTracingReflectionResources()
{
  auto& lightingPasses = m_ViewPassResources->m_LightingPasses;
  if (lightingPasses.m_pRTReflectionPipeline != nullptr && lightingPasses.m_pRTReflectionShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTReflection.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
  xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-tracing reflection shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware reflection permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = 16U;
  pipelineDescription.m_uiMaximumAttributeSize                   = 8U;

  xiiGALRayTracingGeneralShaderGroupDescription& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTReflectionRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;

  xiiGALRayTracingGeneralShaderGroupDescription& miss = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTReflectionMiss");
  miss.m_pShader = pRayMiss;

  xiiGALRayTracingTriangleHitShaderGroupDescription& hit = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTReflectionTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPasses.m_pRTReflectionPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPasses.m_pRTReflectionPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-tracing reflection pipeline.");
    return false;
  }
  lightingPasses.m_pRTReflectionPipeline->SetDebugName("RT Reflection Pipeline");

  const xiiGALRayTracingProperties& rayTracingProperties = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment      = xiiMath::Max(1U, rayTracingProperties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride       = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(rayTracingProperties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (rayTracingProperties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > rayTracingProperties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid shader binding table alignment properties.");
    lightingPasses.m_pRTReflectionPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                          = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                       = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                           = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                            = xiiGALBufferMode::Raw;
  lightingPasses.m_pRTReflectionShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPasses.m_pRTReflectionShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware reflection shader binding table.");
    lightingPasses.m_pRTReflectionPipeline.Clear();
    return false;
  }

  lightingPasses.m_pRTReflectionShaderBindingTable->SetDebugName("RT Reflection Shader Binding Table");
  lightingPasses.m_uiRTReflectionShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
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

// static
xiiSharedPtr<xiiGALGraphicsPipelineState> xiiView::EnsureGraphicsPipeline(xiiSharedPtr<xiiGALGraphicsPipelineState>& inout_pPipeline, xiiStringView sShaderPath, const xiiSharedPtr<xiiGALRenderPass>& pRenderPass, xiiUInt32 uiSubpassIndex)
{
  XII_ASSERT_DEV(pRenderPass != nullptr, "A graphics pipeline requires a compatible render pass.");

  if (inout_pPipeline != nullptr)
  {
    const xiiGALGraphicsPipelineDescription& pipelineDescription = inout_pPipeline->GetDescription().m_GraphicsPipeline;
    if (pipelineDescription.m_pRenderPass == pRenderPass && pipelineDescription.m_uiSubpassIndex == uiSubpassIndex)
      return inout_pPipeline;
  }

  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, /*bBlockTillLoaded=*/true);

  xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  XII_ASSERT_DEV(pPermutation.IsValid(), "Failed to load shader permutation: '{}'.", sShaderPath);

  xiiGALGraphicsPipelineStateCreationDescription description;
  description.m_pPipelineResourceSignature            = pPermutation->GetPipelineResourceSignature();
  description.m_pVertexShader                         = pPermutation->GetGALShader(xiiGALShaderType::Vertex);
  description.m_pPixelShader                          = pPermutation->GetGALShader(xiiGALShaderType::Pixel);
  description.m_GraphicsPipeline.m_pBlendState        = pPermutation->GetBlendState();
  description.m_GraphicsPipeline.m_pRasterizerState   = pPermutation->GetRasterizerState();
  description.m_GraphicsPipeline.m_pDepthStencilState = pPermutation->GetDepthStencilState();
  description.m_GraphicsPipeline.m_pRenderPass        = pRenderPass;
  description.m_GraphicsPipeline.m_uiSubpassIndex     = static_cast<xiiUInt8>(uiSubpassIndex);
  description.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;

  inout_pPipeline = xiiGALPipelineCache::GetPipeline(description);
  XII_ASSERT_DEV(inout_pPipeline != nullptr, "Failed to create graphics pipeline: '{}'.", sShaderPath);

  return inout_pPipeline;
}
