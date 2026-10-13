/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelinePassData.h>

namespace xiiBuiltinPipelineUtils
{
  // A pass may temporarily use transfer commands, but subsequent graph barriers still expect
  // the resource state declared by that pass. Restore it before returning from the callback.
  inline void UpdateBufferPreservingState(xiiGALCommandList& commandList, xiiGALBuffer* buffer, xiiUInt32 offset, xiiArrayPtr<const xiiUInt8> data)
  {
    const auto state = buffer->GetResourceState();
    commandList.UpdateBuffer(buffer, offset, data);
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = buffer;
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commandList.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  }
  inline void RestoreTextureState(xiiGALCommandList& commandList, xiiGALTexture* texture, xiiBitflags<xiiGALResourceStateFlags> state)
  {
    if (texture->GetResourceState() == state)
      return;
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = texture;
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commandList.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  }
  inline void ClearRenderTargetPreservingState(xiiGALCommandList& commandList, xiiGALTextureView* view, const xiiColor& color)
  {
    const auto state = view->GetTexture()->GetResourceState();
    commandList.ClearRenderTargetView(view, color);
    RestoreTextureState(commandList, view->GetTexture(), state);
  }
  inline void ClearDepthPreservingState(xiiGALCommandList& commandList, xiiGALTextureView* view, bool depth, bool stencil, float value, xiiUInt8 stencilValue)
  {
    const auto state = view->GetTexture()->GetResourceState();
    commandList.ClearDepthStencilView(view, depth, stencil, value, stencilValue);
    RestoreTextureState(commandList, view->GetTexture(), state);
  }


  // Shared constants (sizes of persistent GPU buffers, aligned to typical instance budgets)
  static constexpr xiiUInt32 k_uiMaxInstances = 65536U; ///< The maximum number of drawable objects in one frame. This is used to dimension GPU buffers, so it should be set generously to avoid out-of-memory situations, but not excessively to avoid wasting memory.

  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasWidth  = 4096U; ///< The width of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The height will be the same as the width, and each cascade will be allocated a quadrant of the atlas.
  static constexpr xiiUInt32 k_uiDirectionalShadowAtlasHeight = 4096U; ///< The height of the directional shadow atlas. This should be sized to fit the maximum number of cascades per directional light (currently 4) at the desired resolution (e.g. 1024x1024 per cascade). The width will be the same as the height, and each cascade will be allocated a quadrant of the atlas.

  inline bool IsRenderDataTypeName(const xiiRenderData* pRenderData, xiiStringView sTypeName)
  {
    const xiiRTTI* pType = pRenderData != nullptr ? pRenderData->GetDynamicRTTI() : nullptr;
    return pType != nullptr && pType->GetTypeName().IsEqual_NoCase(sTypeName);
  }

  inline xiiUInt32 CountRenderDataByTypeName(const xiiArrayPtr<xiiRenderData* const>& renderData, xiiStringView sTypeName)
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

  inline xiiUInt32 GetDrawCommandCapacity(const xiiRenderGraphBlackboard& blackboard)
  {
    xiiUInt32  uiDrawCommandCapacity   = 0U;
    const bool bHasDrawCommandCapacity = blackboard.TryGet(xiiRGBlackboardKeys::k_DrawCommandCapacity, uiDrawCommandCapacity);
    XII_IGNORE_UNUSED(bHasDrawCommandCapacity);
    return uiDrawCommandCapacity;
  }

  inline xiiUInt32 GetShadowCommandCapacity(const xiiRenderGraphBlackboard& blackboard)
  {
    xiiUInt32  uiShadowCommandCapacity   = 0U;
    const bool bHasShadowCommandCapacity = blackboard.TryGet(xiiRGBlackboardKeys::k_ShadowCommandCapacity, uiShadowCommandCapacity);
    XII_IGNORE_UNUSED(bHasShadowCommandCapacity);
    return uiShadowCommandCapacity;
  }

  inline const xiiDirectionalLightRenderData* SelectMainDirectionalLight(const xiiArrayPtr<xiiRenderData* const>& renderData)
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

  inline const xiiSkyAtmosphereRenderData* SelectSkyAtmosphere(const xiiArrayPtr<xiiRenderData* const>& renderData)
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

  inline const xiiVolumetricCloudRenderData* SelectVolumetricCloudLayer(const xiiArrayPtr<xiiRenderData* const>& renderData)
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



  inline xiiRenderGraphBufferHandle CreateTemporalDenoiseConstants(xiiRenderGraphBuilder& builder, xiiStringView sName)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = sizeof(xiiTemporalDenoiseConstants);
    description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
    description.m_Usage          = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    return builder.WriteBuffer(sName, description, xiiGALResourceStateFlags::ConstantBuffer);
  }


  inline xiiAtmosphereConstants MakeAtmosphereConstants(xiiAtmosphereLUTHandle hCache)
  {
    const xiiAtmosphereSettings& settings = xiiAtmosphereManager::GetConfiguration(hCache);

    xiiAtmosphereConstants constants            = {};
    constants.PlanetAtmosphereRadiiScaleHeights = xiiVec4(settings.m_fPlanetRadiusKm, settings.m_fAtmosphereRadiusKm, settings.m_fRayleighScaleHeightKm, settings.m_fMieScaleHeightKm);
    constants.RayleighScattering                = xiiVec4(settings.m_vRayleighScattering, 0.0f);
    constants.MieScatteringAndPhase             = xiiVec4(settings.m_vMieScattering, settings.m_fMiePhaseG);
    constants.MieAbsorption                     = xiiVec4(settings.m_vMieAbsorption, 0.0f);
    constants.OzoneAbsorption                   = xiiVec4(settings.m_vOzoneAbsorption, 0.0f);
    constants.PlanetUpAndGroundAltitudeMeters   = xiiVec4(settings.m_vPlanetUpDirection.GetNormalized(), settings.m_fGroundAltitudeMeters);
    constants.SunDiskParameters                 = xiiVec4(settings.m_fSunLimbDarkening, 0.0f, 0.0f, 0.0f);
    constants.SampleCounts                      = xiiVec4U32(settings.m_uiTransmittanceIntegrationSteps, settings.m_uiMultiScatterSqrtSamples,
                                                             settings.m_uiSkyViewIntegrationSteps, settings.m_uiAerialPerspectiveSteps);
    return constants;
  }

  inline xiiRenderGraphBufferHandle CreateAtmosphereConstantsBuffer(xiiRenderGraphBuilder& builder, xiiStringView sName)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = sizeof(xiiAtmosphereConstants);
    description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
    description.m_Usage          = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    return builder.WriteBuffer(sName, description, xiiGALResourceStateFlags::ConstantBuffer);
  }

  inline void UploadAtmosphereConstants(xiiGALCommandList& cmd, xiiGALBuffer* pBuffer, const xiiAtmosphereConstants& constants)
  {
    xiiGALMapHelper<xiiAtmosphereConstants> mappedConstants(cmd, pBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mappedConstants = constants;
  }


  static constexpr xiiUInt32 k_uiDecalTileSize               = 16U;
  static constexpr xiiUInt32 k_uiMaxProjectedDecalsPerTile   = 12U;
  static constexpr xiiUInt32 k_uiMaxProjectedDecalTileStride = 1U + k_uiMaxProjectedDecalsPerTile;
} // namespace xiiBuiltinPipelineUtils
