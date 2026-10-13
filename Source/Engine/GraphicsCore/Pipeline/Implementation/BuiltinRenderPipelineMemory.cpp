/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineMemory.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>

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
  auto ensure = [&](xiiSharedPtr<xiiGALTexture>(&textures)[2], xiiGALResourceFormat::Enum format, bool& valid, xiiStringView name) -> xiiResult {
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
