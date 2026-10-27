/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <Shaders/Pipeline/Passes/Shadows/SpotShadowConstants.h>

namespace xiiSpotShadowPassUtils
{
  inline xiiResult Validate(xiiRenderPipelinePassContext& context, const xiiSpotShadowSnapshot* shadows, xiiRenderGraphTextureHandle atlas)
  {
    if (!shadows)
      return atlas.IsValid() ? context.ReportFailure("Spotlight atlas requires its projection snapshot.") : XII_SUCCESS;
    const auto  count = shadows->m_Views.GetCount(), resolution = shadows->m_uiResolution;
    const auto* image     = context.m_Graph.GetTextureDescription(atlas);
    const auto* receivers = context.m_Graph.GetBufferDescription(shadows->m_hReceivers);
    if (!count || count > 4 || resolution < 64 || resolution > 16384 || !image || image->m_Format != xiiGALResourceFormat::D32Float || image->m_Type != xiiGALResourceDimension::Texture2D || image->m_uiSampleCount != 1 || image->m_uiArraySizeOrDepth != 1 || !image->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ||
        image->m_Size != xiiSizeU32(resolution * (count > 1 ? 2U : 1U), resolution * (count > 2 ? 2U : 1U)) || !xiiScreenSpacePassUtils::Uniform(context, shadows->m_hConstants, sizeof(xiiSpotShadowConstants)) ||
        !receivers || receivers->m_Mode != xiiGALBufferMode::Structured || receivers->m_uiElementByteStride != sizeof(xiiVec2U32) || !receivers->m_uiSize || receivers->m_uiSize % sizeof(xiiVec2U32) || !receivers->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Spotlight sampling requires a matching completed D32 atlas and exact sampling/receiver layouts.");
    return XII_SUCCESS;
  }
} // namespace xiiSpotShadowPassUtils
