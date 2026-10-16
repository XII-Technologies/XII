/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Draws extracted world/view debug primitives over explicitly supplied targets.
class XII_GRAPHICSCORE_DLL xiiDebugOverlayPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDebugOverlayPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputDisplayLinearColor{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget};
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth;
  xiiRenderPipelineTextureOutputPin m_OutputDisplayLinearColor;
  xiiRenderPipelineTextureOutputPin m_OutputSceneDepth;

  bool m_bEnabled = true;

  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled ? xiiRenderExtractionCategory::Custom : xiiRenderExtractionCategory::None; }
};
