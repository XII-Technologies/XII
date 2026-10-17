/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

/// Captures one camera's culling parameters and component selection into immutable graph data.
class XII_GRAPHICSCORE_DLL xiiVisibilityViewPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisibilityViewPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin         m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                                m_uiVisibilityMask  = 0xFFFFFFFFU;
  xiiUInt32                                m_uiRequiredFlags   = xiiGpuInstanceFlags::Enabled;
  xiiUInt32                                m_uiExcludedFlags   = 0;
  float                                    m_fLodHysteresis    = 0.1f;
  float                                    m_fLodScreenScale   = 1.0f;
  bool                                     m_bUseViewSelection = true;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  void                                     InvalidateTemporalHistory() override { m_uiLastLodToken = 65534U; }
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }

private:
  xiiUInt32 m_uiLastLodToken = 65534U;
};
