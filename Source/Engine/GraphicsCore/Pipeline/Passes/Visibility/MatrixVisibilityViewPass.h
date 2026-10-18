/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

/// Captures an explicit matrix query's culling parameters and component selection into immutable graph data.
class XII_GRAPHICSCORE_DLL xiiMatrixVisibilityViewPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMatrixVisibilityViewPass, xiiRenderPipelinePass);

public:
  /// Optional exact candidate stream. Empty selections use one invalid-index sentinel.
  xiiRenderPipelineBufferInputPin          m_InputCandidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                                m_uiVisibilityMask  = 0xFFFFFFFFU;
  xiiUInt32                                m_uiRequiredFlags   = xiiGpuInstanceFlags::Enabled;
  xiiUInt32                                m_uiExcludedFlags   = 0;
  float                                    m_fLodHysteresis    = 0.1f;
  float                                    m_fLodScreenScale   = 1.0f;
  bool                                     m_bUseViewSelection = false;
  xiiMat4                                  m_ViewProjection    = xiiMat4::MakeIdentity();
  xiiVec3                                  m_CameraPosition    = xiiVec3::MakeZero();
  xiiUInt32                                m_uiWidth = 1, m_uiHeight = 1;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  void                                     InvalidateTemporalHistory() override { m_uiLastLodToken = 65534U; }
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return xiiRenderExtractionCategory::Geometry; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return xiiRenderPipelineFeature::GpuScene; }

private:
  xiiUInt32 m_uiLastLodToken = 65534U;
};
