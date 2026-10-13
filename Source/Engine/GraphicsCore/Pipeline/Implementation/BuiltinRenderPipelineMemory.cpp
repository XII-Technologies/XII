/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineMemory.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>

xiiResult xiiBuiltinRenderPipelineRuntime::PrepareShadowResources()
{
  auto& shadows = m_Resources.m_ShadowPasses;
  auto  ensure  = [&](xiiSharedPtr<xiiGALTexture>& texture, xiiGALResourceDimension::Enum type, xiiUInt32 width, xiiUInt32 height, xiiUInt32 slices, xiiStringView name) -> xiiResult {
    xiiGALTextureCreationDescription description;
    description.m_Type               = type;
    description.m_Format             = xiiGALResourceFormat::D32Float;
    description.m_Size               = {width, height};
    description.m_uiArraySizeOrDepth = slices;
    description.m_uiMipLevels        = 1U;
    description.m_BindFlags          = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage              = xiiGALResourceUsage::Default;
    if (texture != nullptr && texture->GetDescription() == description)
      return XII_SUCCESS;
    texture = nullptr;
    if (xiiGALDevice::HasDefaultDevice())
      texture = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    if (texture == nullptr)
    {
      xiiLog::Error("Failed to allocate '{}' ({} x {}, {} slices).", name, width, height, slices);
      return XII_FAILURE;
    }
    texture->SetDebugName(name);
    return XII_SUCCESS;
  };

  // Preserve the array schema expected by lighting shaders, but only reserve full-sized
  // shadow maps when there is work for them. GAL retires replaced allocations after GPU use.
  const bool      directional = shadows.m_uiActiveCascadeCount > 0U;
  const xiiUInt32 width       = directional ? xiiBuiltinPipelineUtils::k_uiDirectionalShadowAtlasWidth : 1U;
  const xiiUInt32 height      = directional ? xiiBuiltinPipelineUtils::k_uiDirectionalShadowAtlasHeight : 1U;
  if (shadows.m_pDirectionalShadowAtlas == nullptr || shadows.m_pDirectionalShadowAtlas->GetDescription().m_Size != xiiSizeU32(width, height))
    for (auto& view : shadows.m_pDirectionalShadowCascadeViews)
      view = nullptr;
  XII_SUCCEED_OR_RETURN(ensure(shadows.m_pDirectionalShadowAtlas, xiiGALResourceDimension::Texture2DArray, width, height, 4U, "Directional Shadow Atlas"));
  for (xiiUInt32 cascade = 0; cascade < shadows.m_uiActiveCascadeCount; ++cascade)
  {
    auto& view = shadows.m_pDirectionalShadowCascadeViews[cascade];
    if (view != nullptr)
      continue;
    xiiGALTextureViewCreationDescription description;
    description.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
    description.m_uiFirstArrayOrDepthSlice  = cascade;
    description.m_uiArrayOrDepthSlicesCount = 1U;
    view                                    = shadows.m_pDirectionalShadowAtlas->CreateView(description);
    if (view == nullptr)
    {
      xiiLog::Error("Failed to create directional shadow cascade view {}.", cascade);
      return XII_FAILURE;
    }
  }
  const xiiUInt32 localSize = m_Resources.m_LightingSystem.GetLocalShadowStatistics().m_uiAllocatedFaces == 0U ? 1U : m_Resources.m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;
  return ensure(shadows.m_pLocalShadowAtlas, xiiGALResourceDimension::Texture2D, localSize, localSize, 1U, "Local Shadow Atlas");
}

xiiResult xiiBuiltinRenderPipelineRuntime::PrepareSensorOutput()
{
  auto& resources = m_Resources.m_OutputPasses;
  if (resources.m_hSensorProfile.IsInvalidated())
    return XII_SUCCESS;
  xiiSensorProfile profile;
  if (xiiSensorRenderingManager::GetProfile(resources.m_hSensorProfile, profile).Failed())
  {
    xiiLog::Error("Could not resolve calibrated sensor profile.");
    return XII_FAILURE;
  }
  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA32Float;
  description.m_Size        = {profile.m_uiResolutionX, profile.m_uiResolutionY};
  description.m_uiMipLevels = 1;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  if (resources.m_pSensorOutputTexture != nullptr && resources.m_pSensorOutputTexture->GetDescription() == description)
    return XII_SUCCESS;
  resources.m_pSensorOutputTexture = nullptr;
  if (xiiGALDevice::HasDefaultDevice())
    resources.m_pSensorOutputTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  if (resources.m_pSensorOutputTexture == nullptr)
  {
    xiiLog::Error("Failed to allocate calibrated sensor output ({} x {}, format {}).", profile.m_uiResolutionX, profile.m_uiResolutionY, description.m_Format.GetValue());
    return XII_FAILURE;
  }
  resources.m_pSensorOutputTexture->SetDebugName("Calibrated Sensor Output");
  return XII_SUCCESS;
}

xiiResult xiiBuiltinRenderPipelineRuntime::PrepareTemporalResources(xiiStringView stage)
{
  if (stage != "Lighting" && stage != "OpaqueComposition" && stage != "TemporalAntiAliasing" && stage != "AutoExposure")
    return XII_SUCCESS;
  const xiiUInt32 width  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 height = m_View.GetRenderResolutionHeight();
  if (!xiiGALDevice::HasDefaultDevice())
  {
    xiiLog::Error("Failed to allocate temporal resources for '{}' without a graphics device.", stage);
    return XII_FAILURE;
  }
  const auto device = xiiGALDevice::GetDefaultDevice();
  auto       ensure = [&](xiiSharedPtr<xiiGALTexture>(&textures)[2], xiiGALResourceFormat::Enum format, bool& valid, xiiStringView name) -> xiiResult {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = format;
    description.m_Size        = {width, height};
    description.m_uiMipLevels = 1;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;
    if (xiiBuiltinPipelineUtils::EnsureTemporalHistoryTextures(textures, description, valid, [pDevice = device.Borrow()](const auto& desc) {
          return pDevice->CreateTexture(desc);
        }).Failed())
    {
      // Obsolete images cannot serve the requested frame. Release them so the next frame can
      // retry without retaining the old allocation alongside its replacements. GAL defers GPU deletion.
      textures[0] = nullptr;
      textures[1] = nullptr;
      valid       = false;
      xiiLog::Error("Failed to allocate '{}' history ({} x {}, format {}).", name, width, height, format);
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  };
  auto& lighting = m_Resources.m_LightingPasses;
  if (stage == "Lighting")
  {
    auto& shadows     = m_Resources.m_ShadowPasses;
    auto& preparation = m_Resources.m_LightingPrepPasses;
    XII_SUCCEED_OR_RETURN(ensure(shadows.m_pRayTracedShadowHistory, xiiGALResourceFormat::R8UNormalized, shadows.m_bRayTracedShadowHistoryValid, "Ray-traced shadow"));
    XII_SUCCEED_OR_RETURN(ensure(preparation.m_pAmbientOcclusionHistory, xiiGALResourceFormat::R8UNormalized, preparation.m_bAmbientOcclusionHistoryValid, "Ambient occlusion"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pDirectReservoirHistory, xiiGALResourceFormat::RGBA32UInt, lighting.m_bDirectReservoirHistoryValid, "Direct-light reservoir"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pDirectSurfaceHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bDirectReservoirHistoryValid, "Direct-light surface"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTGIReservoirSampleHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bRTGIReservoirHistoryValid, "GI reservoir sample"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTGIReservoirStateHistory, xiiGALResourceFormat::RG32Float, lighting.m_bRTGIReservoirHistoryValid, "GI reservoir state"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTGIReservoirSurfaceHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bRTGIReservoirHistoryValid, "GI reservoir surface"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTGIHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bRTGIHistoryValid, "GI denoise"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTReflectionHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bRTReflectionHistoryValid, "Reflection radiance"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTReflectionHitDistanceHistory, xiiGALResourceFormat::R16Float, lighting.m_bRTReflectionHistoryValid, "Reflection hit distance"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTReflectionMomentsHistory, xiiGALResourceFormat::RG16Float, lighting.m_bRTReflectionHistoryValid, "Reflection moments"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pRTReflectionIdentityHistory, xiiGALResourceFormat::RG32UInt, lighting.m_bRTReflectionHistoryValid, "Reflection identity"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pVolumetricHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bVolumetricHistoryValid, "Volumetric scattering"));
  }
  else if (stage == "OpaqueComposition")
  {
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pSSRHistory, xiiGALResourceFormat::RGBA16Float, lighting.m_bSSRHistoryValid, "Screen-space reflection"));
    XII_SUCCEED_OR_RETURN(ensure(lighting.m_pSSRIdentityHistory, xiiGALResourceFormat::RG32UInt, lighting.m_bSSRHistoryValid, "Screen-space reflection identity"));
  }
  else if (stage == "TemporalAntiAliasing")
  {
    auto&       temporal = m_Resources.m_TemporalPasses;
    const auto* previous = temporal.m_pTAAHistoryBuffers[0].Borrow();
    XII_SUCCEED_OR_RETURN(ensure(temporal.m_pTAAHistoryBuffers, xiiGALResourceFormat::RGBA16Float, temporal.m_bTAAHistoryValid, "Temporal anti-aliasing"));
    if (previous != temporal.m_pTAAHistoryBuffers[0].Borrow())
      temporal.m_uiTAAHistoryWriteIndex = 0;
  }
  else if (stage == "AutoExposure" && !m_Resources.m_TemporalPasses.m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = 4;
    description.m_uiSize              = sizeof(float) * 2;
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;
    auto buffer                       = device->CreateBuffer(description);
    if (buffer == nullptr)
    {
      xiiLog::Error("Failed to allocate temporal exposure buffer ({} bytes).", description.m_uiSize);
      return XII_FAILURE;
    }
    m_Resources.m_TemporalPasses.m_pExposureBuffer       = std::move(buffer);
    m_Resources.m_TemporalPasses.m_bExposureHistoryValid = false;
  }
  return XII_SUCCESS;
}
