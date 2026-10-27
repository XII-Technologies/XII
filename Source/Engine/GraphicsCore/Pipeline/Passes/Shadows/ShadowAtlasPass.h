/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Allocates and clears an occurrence-local D32 atlas only for an active cascade set.
class XII_GRAPHICSCORE_DLL xiiShadowAtlasPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowAtlasPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin     m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineDataInputPin     m_Spots{xiiGetStaticRTTI<xiiSpotShadowSnapshot>(), true};
  xiiRenderPipelineTextureOutputPin m_Atlas{xiiGALResourceFormat::D32Float, true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};
