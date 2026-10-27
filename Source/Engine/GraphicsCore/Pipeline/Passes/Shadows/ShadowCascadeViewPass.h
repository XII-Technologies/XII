/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Produces matching visibility and raster inputs for one immutable directional cascade.
class XII_GRAPHICSCORE_DLL xiiShadowCascadeViewPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCascadeViewPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin            m_Cascades{xiiGetStaticRTTI<xiiShadowCascadeSnapshot>(), true};
  xiiRenderPipelineBufferOutputPin         m_View{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Candidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Raster{true, xiiGALBindFlags::UniformBuffer};
  xiiUInt32                                m_uiCascadeIndex = 0;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }
};
