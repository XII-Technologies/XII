/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
/// Publishes shared world light/IES uploads and this view's explicit candidate stream.
/// Consumers must apply Candidates rather than assuming every union light belongs to the view.
class XII_GRAPHICSCORE_DLL xiiWorldLightsUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldLightsUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin           m_Before;
  xiiRenderPipelineOrderOutputPin          m_After;
  xiiRenderPipelineBufferOutputPin         m_Lights{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Profiles{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Counts{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Lights; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::Lighting; }
};
