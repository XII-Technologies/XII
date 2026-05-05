/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Reflection/Implementation/Casts.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Lighting/LightingSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <cmath>

namespace
{
  static xiiVec3 NormalizeOrFallback(xiiVec3 vDirection, const xiiVec3& vFallback)
  {
    vDirection.NormalizeIfNotZero(vFallback).IgnoreResult();
    return vDirection;
  }

  static xiiVec4 MakeVec4(const xiiColor& color, float fAlpha)
  {
    return xiiVec4(color.r, color.g, color.b, fAlpha);
  }

  static xiiVec4 MakeVec4(const xiiVec3& vVector, float fW)
  {
    return xiiVec4(vVector.x, vVector.y, vVector.z, fW);
  }

  static float GetSafeRange(float fRange, float fIntensity)
  {
    if (fRange > 0.0f)
    {
      return fRange;
    }

    return xiiLightComponent::CalculateEffectiveRange(0.0f, fIntensity);
  }

  static float GetClusterLogFarOverNear(float fNearPlane, float fFarPlane)
  {
    return xiiMath::Log2(xiiMath::Max(fFarPlane, fNearPlane + 0.001f) / xiiMath::Max(fNearPlane, 0.001f));
  }
} // namespace

xiiLightingSystem::xiiLightingSystem() = default;

xiiLightingSystem::~xiiLightingSystem()
{
  Shutdown();
}

void xiiLightingSystem::Initialize(xiiSharedPtr<xiiGALDevice> pDevice)
{
  m_pDevice = std::move(pDevice);
  m_LightData.Reserve(m_Settings.m_uiMaxActiveLights);

  EnsureGpuResources();
}

void xiiLightingSystem::Shutdown()
{
  m_pCameraConstantsBuffer.Clear();
  m_pLightConstantsBuffer.Clear();
  m_pGlobalConstantsBuffer.Clear();
  m_pLightDataBuffer.Clear();
  m_pDevice.Clear();
  m_LightData.Clear();
}

void xiiLightingSystem::BuildFrameData(const xiiView& view, const xiiExtractedRenderData& extractedData, xiiUInt32 uiFrameIndex)
{
  ResetFrameData();

  const xiiUInt32 uiRenderWidth  = xiiMath::Max(view.GetRenderResolutionWidth(), 1U);
  const xiiUInt32 uiRenderHeight = xiiMath::Max(view.GetRenderResolutionHeight(), 1U);

  m_uiClusterCountX     = (uiRenderWidth + m_Settings.m_uiClusterTileSize - 1U) / m_Settings.m_uiClusterTileSize;
  m_uiClusterCountY     = (uiRenderHeight + m_Settings.m_uiClusterTileSize - 1U) / m_Settings.m_uiClusterTileSize;
  m_uiTotalClusterCount = m_uiClusterCountX * m_uiClusterCountY * m_Settings.m_uiClusterDepthSlices;

  const xiiCamera* pCamera = view.GetCamera();
  if (pCamera != nullptr)
  {
    const xiiVec3 vCameraPosition = pCamera->GetPosition();
    const xiiVec3 vCameraForward  = NormalizeOrFallback(pCamera->GetDirForwards(), xiiVec3(1.0f, 0.0f, 0.0f));

    m_CameraConstants.m_ViewProjectionMatrix        = view.GetViewProjectionMatrix(xiiCameraEye::Left);
    m_CameraConstants.m_InverseViewProjectionMatrix = view.GetInverseViewProjectionMatrix(xiiCameraEye::Left);
    m_CameraConstants.m_CameraPositionAndNearPlane  = xiiVec4(vCameraPosition, pCamera->GetNearPlane());
    m_CameraConstants.m_CameraForwardAndFarPlane    = xiiVec4(vCameraForward, pCamera->GetFarPlane());

    m_LightConstants.m_fClusterNearPlane      = xiiMath::Max(pCamera->GetNearPlane(), 0.001f);
    m_LightConstants.m_fClusterLogFarOverNear = GetClusterLogFarOverNear(m_LightConstants.m_fClusterNearPlane, pCamera->GetFarPlane());
  }

  m_LightConstants.m_uiClusterCountX          = m_uiClusterCountX;
  m_LightConstants.m_uiClusterCountY          = m_uiClusterCountY;
  m_LightConstants.m_uiClusterCountZ          = m_Settings.m_uiClusterDepthSlices;
  m_LightConstants.m_uiMaxLightsPerCluster    = m_Settings.m_uiMaxLightsPerCluster;
  m_LightConstants.m_AmbientLightColor        = MakeVec4(m_Settings.m_AmbientLightColor, 1.0f);
  m_LightConstants.m_fIndirectLightIntensity  = m_Settings.m_fIndirectLightIntensity;
  m_LightConstants.m_fContactShadowLength     = m_Settings.m_fContactShadowLength;
  m_LightConstants.m_fContactShadowThickness  = m_Settings.m_fContactShadowThickness;
  m_LightConstants.m_uiContactShadowSteps     = m_Settings.m_uiContactShadowSteps;
  m_LightConstants.m_uiLocalShadowTileSize    = m_Settings.m_uiLocalShadowTileSize;
  m_LightConstants.m_fVolumetricFogDensity    = m_Settings.m_fVolumetricFogDensity;
  m_LightConstants.m_fVolumetricHeightFalloff = m_Settings.m_fVolumetricHeightFalloff;
  m_LightConstants.m_fVolumetricBaseHeight    = m_Settings.m_fVolumetricBaseHeight;
  m_LightConstants.m_fVolumetricAnisotropy    = m_Settings.m_fVolumetricAnisotropy;

  m_GlobalConstants.m_uiFrameIndex = uiFrameIndex;
  if (xiiClock* pClock = xiiClock::GetGlobalClock())
  {
    m_GlobalConstants.m_fDeltaTimeMs = static_cast<float>(pClock->GetTimeDiff().GetMilliseconds());
    m_GlobalConstants.m_fGlobalTime  = static_cast<float>(pClock->GetAccumulatedTime().GetSeconds());
    m_GlobalConstants.m_fWorldTime   = m_GlobalConstants.m_fGlobalTime;
  }
  m_GlobalConstants.m_RenderScaleJitter = xiiVec4(view.GetRenderResolutionScale(), 0.0f, 0.0f, 0.0f);

  float fBestDirectionalIntensity = -1.0f;

  for (const xiiRenderData* pRenderData : extractedData.GetAllRenderData())
  {
    if (const xiiDirectionalLightRenderData* pDirectionalLight = xiiDynamicCast<const xiiDirectionalLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const xiiColor lightColor = EvaluateLightColor(pDirectionalLight->m_LightColor, pDirectionalLight->m_uiTemperature);
      const xiiVec3  vDirection = NormalizeOrFallback(pDirectionalLight->m_vDirection, xiiVec3(0.0f, 0.0f, -1.0f));

      lightData.m_PositionAndInvRange   = xiiVec4::MakeZero();
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(LightType::Directional));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pDirectionalLight->m_fIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(0.0f, pDirectionalLight->m_fRadius, 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pDirectionalLight->m_bCastShadows ? 1.0f : 0.0f, 0.0f, pDirectionalLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = xiiVec4::MakeZero();

      if (AppendLight(lightData, LightType::Directional) && pDirectionalLight->m_fIntensity > fBestDirectionalIntensity)
      {
        fBestDirectionalIntensity                         = pDirectionalLight->m_fIntensity;
        m_LightConstants.m_MainLightDirectionAndIntensity = MakeVec4(vDirection, pDirectionalLight->m_fIntensity);
        m_LightConstants.m_MainLightColor                 = MakeVec4(lightColor, 1.0f);
      }

      continue;
    }

    if (const xiiPointLightRenderData* pPointLight = xiiDynamicCast<const xiiPointLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pPointLight->m_fRange, pPointLight->m_fIntensity), 0.001f);
      const xiiVec3  vPosition  = pPointLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pPointLight->m_qGlobalRotation * xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(1.0f, 0.0f, 0.0f));
      const xiiColor lightColor = EvaluateLightColor(pPointLight->m_LightColor, pPointLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(LightType::Point));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pPointLight->m_fIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, pPointLight->m_fRadius, pPointLight->m_fLength, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(1.0f, -1.0f, 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pPointLight->m_bCastShadows ? 1.0f : 0.0f, pPointLight->m_fShadowFadeOutRange, pPointLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = MakeVec4(vPosition, fRange + pPointLight->m_fLength * 0.5f);

      AppendLight(lightData, LightType::Point);
      continue;
    }

    if (const xiiSpotLightRenderData* pSpotLight = xiiDynamicCast<const xiiSpotLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pSpotLight->m_fRange, pSpotLight->m_fIntensity), 0.001f);
      const xiiVec3  vPosition  = pSpotLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pSpotLight->m_qGlobalRotation * xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(1.0f, 0.0f, 0.0f));
      const xiiColor lightColor = EvaluateLightColor(pSpotLight->m_LightColor, pSpotLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(LightType::Spot));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pSpotLight->m_fIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, pSpotLight->m_fRadius, 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(xiiMath::Cos(pSpotLight->m_InnerSpotAngle * 0.5f), xiiMath::Cos(pSpotLight->m_OuterSpotAngle * 0.5f), 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pSpotLight->m_bCastShadows ? 1.0f : 0.0f, pSpotLight->m_fShadowFadeOutRange, pSpotLight->m_fRadius, 0.0f);

      const xiiBoundingSphere boundingSphere = pSpotLight->m_GlobalBounds.GetSphere();
      lightData.m_BoundsCenterAndRadius      = MakeVec4(boundingSphere.m_vCenter, xiiMath::Max(boundingSphere.m_fRadius, fRange));

      AppendLight(lightData, LightType::Spot);
      continue;
    }

    if (const xiiRectangleAreaLightRenderData* pRectangleLight = xiiDynamicCast<const xiiRectangleAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(xiiLightComponent::CalculateEffectiveRange(0.0f, pRectangleLight->m_fIntensity), 0.001f);
      const xiiVec3  vPosition  = pRectangleLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pRectangleLight->m_qGlobalRotation * xiiVec3(-1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f));
      const xiiColor lightColor = EvaluateLightColor(pRectangleLight->m_LightColor, pRectangleLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(LightType::Rectangle));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pRectangleLight->m_fIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, 0.0f, 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(1.0f, -1.0f, xiiMath::Max(pRectangleLight->m_vExtents.x, 0.001f), xiiMath::Max(pRectangleLight->m_vExtents.y, 0.001f));
      lightData.m_ShadowData            = xiiVec4(pRectangleLight->m_bCastShadows ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
      lightData.m_BoundsCenterAndRadius = MakeVec4(vPosition, fRange + pRectangleLight->m_vExtents.GetLength() * 0.5f);

      AppendLight(lightData, LightType::Rectangle);
      continue;
    }

    if (const xiiDiscAreaLightRenderData* pDiscLight = xiiDynamicCast<const xiiDiscAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pDiscLight->m_fRange, pDiscLight->m_fIntensity), 0.001f);
      const xiiVec3  vPosition  = pDiscLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pDiscLight->m_qGlobalRotation * xiiVec3(-1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f));
      const xiiColor lightColor = EvaluateLightColor(pDiscLight->m_LightColor, pDiscLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(LightType::Disc));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pDiscLight->m_fIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, xiiMath::Max(pDiscLight->m_fRadius, 0.001f), 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(1.0f, -1.0f, xiiMath::Max(pDiscLight->m_fRadius, 0.001f), 0.0f);
      lightData.m_ShadowData            = xiiVec4(pDiscLight->m_bCastShadows ? 1.0f : 0.0f, pDiscLight->m_fShadowFadeOutRange, pDiscLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = MakeVec4(vPosition, fRange + pDiscLight->m_fRadius);

      AppendLight(lightData, LightType::Disc);
      continue;
    }
  }

  m_LightConstants.m_uiActiveLightCount = m_Stats.m_uiActiveLightCount;
}

void xiiLightingSystem::UploadFrameData(xiiGALCommandList& ref_commandList)
{
  EnsureGpuResources();

  if (m_pCameraConstantsBuffer == nullptr || m_pLightConstantsBuffer == nullptr || m_pGlobalConstantsBuffer == nullptr || m_pLightDataBuffer == nullptr)
    return;

  {
    xiiGALMapHelper<PerFrameCameraConstants> pConstants(ref_commandList, m_pCameraConstantsBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *pConstants = m_CameraConstants;
  }

  {
    xiiGALMapHelper<PerFrameLightConstants> pConstants(ref_commandList, m_pLightConstantsBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *pConstants = m_LightConstants;
  }

  {
    xiiGALMapHelper<PerFrameGlobalConstants> pConstants(ref_commandList, m_pGlobalConstantsBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *pConstants = m_GlobalConstants;
  }

  {
    xiiGALMapHelper<xiiUInt8> pLights(ref_commandList, m_pLightDataBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);

    const xiiUInt64 uiUploadSize = static_cast<xiiUInt64>(m_Settings.m_uiMaxActiveLights) * sizeof(xiiGpuLightData);
    xiiMemoryUtils::ZeroFill(pLights.GetMappedData(), static_cast<size_t>(uiUploadSize));

    if (!m_LightData.IsEmpty())
    {
      const xiiArrayPtr<const xiiUInt8> lightBytes = m_LightData.GetByteArrayPtr();
      xiiMemoryUtils::Copy(pLights.GetMappedData(), lightBytes.GetPtr(), lightBytes.GetCount());
    }
  }
}

void xiiLightingSystem::BindFrameConstants(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  ref_commandList.ResolveAndSetConstantBuffer("PerFrameCamera", m_pCameraConstantsBuffer.Borrow(), shaderStages);
  ref_commandList.ResolveAndSetConstantBuffer("PerFrameLight", m_pLightConstantsBuffer.Borrow(), shaderStages);
  ref_commandList.ResolveAndSetConstantBuffer("PerFrameGlobal", m_pGlobalConstantsBuffer.Borrow(), shaderStages);
}

void xiiLightingSystem::BindLightData(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  if (m_pLightDataBuffer != nullptr)
  {
    ref_commandList.ResolveAndSetShaderResourceBufferView("g_Lights", m_pLightDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource), shaderStages);
  }
}

void xiiLightingSystem::BindLightingResources(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  BindFrameConstants(ref_commandList, shaderStages);
  BindLightData(ref_commandList, shaderStages);
}

void xiiLightingSystem::WriteBlackboard(xiiRenderGraphBlackboard& ref_blackboard) const
{
  ref_blackboard.Set(xiiRGBlackboardKeys::k_ActiveLightCount, m_Stats.m_uiActiveLightCount);
}

xiiGALBuffer* xiiLightingSystem::GetLightDataBuffer() const
{
  return m_pLightDataBuffer.Borrow();
}

void xiiLightingSystem::EnsureGpuResources()
{
  if (m_pDevice == nullptr)
  {
    m_pDevice = xiiGALDevice::GetDefaultDevice();
  }

  if (m_pDevice == nullptr)
    return;

  if (m_pCameraConstantsBuffer == nullptr)
  {
    m_pCameraConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(m_pDevice.Borrow(), sizeof(PerFrameCameraConstants), "Lighting PerFrameCamera");
  }

  if (m_pLightConstantsBuffer == nullptr)
  {
    m_pLightConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(m_pDevice.Borrow(), sizeof(PerFrameLightConstants), "Lighting PerFrameLight");
  }

  if (m_pGlobalConstantsBuffer == nullptr)
  {
    m_pGlobalConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(m_pDevice.Borrow(), sizeof(PerFrameGlobalConstants), "Lighting PerFrameGlobal");
  }

  const xiiUInt64 uiRequiredLightBufferSize = static_cast<xiiUInt64>(m_Settings.m_uiMaxActiveLights) * sizeof(xiiGpuLightData);
  if (m_pLightDataBuffer == nullptr || m_pLightDataBuffer->GetSize() < uiRequiredLightBufferSize)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiGpuLightData);
    description.m_uiSize              = uiRequiredLightBufferSize;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

    m_pLightDataBuffer = m_pDevice->CreateBuffer(description);
    if (m_pLightDataBuffer != nullptr)
    {
      m_pLightDataBuffer->SetDebugName("Lighting LightData");
    }
  }
}

void xiiLightingSystem::ResetFrameData()
{
  m_LightData.Clear();
  m_Stats = {};

  m_CameraConstants = {};
  m_LightConstants  = {};
  m_GlobalConstants = {};
}

bool xiiLightingSystem::AppendLight(xiiGpuLightData lightData, LightType type)
{
  if (m_LightData.GetCount() >= m_Settings.m_uiMaxActiveLights)
  {
    ++m_Stats.m_uiSkippedLightCount;
    return false;
  }

  lightData.m_UserData.x = static_cast<float>(m_LightData.GetCount());

  m_LightData.PushBack(lightData);
  ++m_Stats.m_uiActiveLightCount;

  if (type == LightType::Directional)
  {
    ++m_Stats.m_uiDirectionalLightCount;
  }
  else
  {
    ++m_Stats.m_uiLocalLightCount;
  }

  return true;
}

xiiColor xiiLightingSystem::EvaluateTemperatureColor(xiiUInt32 uiTemperature)
{
  const float fTemperature = static_cast<float>(xiiMath::Clamp(uiTemperature, 1000U, 50000U)) / 100.0f;

  float fRed   = 1.0f;
  float fGreen = 1.0f;
  float fBlue  = 1.0f;

  if (fTemperature <= 66.0f)
  {
    fRed   = 1.0f;
    fGreen = xiiMath::Clamp(0.39008158f * static_cast<float>(std::log(fTemperature)) - 0.63184144f, 0.0f, 1.0f);
    fBlue  = fTemperature <= 19.0f ? 0.0f : xiiMath::Clamp(0.5432068f * static_cast<float>(std::log(fTemperature - 10.0f)) - 1.1962541f, 0.0f, 1.0f);
  }
  else
  {
    fRed   = xiiMath::Clamp(1.2929362f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.13320476f)), 0.0f, 1.0f);
    fGreen = xiiMath::Clamp(1.1298909f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.07551485f)), 0.0f, 1.0f);
    fBlue  = 1.0f;
  }

  return xiiColor(fRed, fGreen, fBlue, 1.0f);
}

xiiColor xiiLightingSystem::EvaluateLightColor(const xiiColorLinearUB& color, xiiUInt32 uiTemperature)
{
  xiiColor result = color.ToLinearFloat() * EvaluateTemperatureColor(uiTemperature);
  result.a        = 1.0f;
  return result;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LightingSystem);
