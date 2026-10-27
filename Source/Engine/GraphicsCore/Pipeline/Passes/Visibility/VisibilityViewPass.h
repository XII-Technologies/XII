/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

/// Captures one camera's culling parameters and component selection into immutable graph data.
class XII_GRAPHICSCORE_DLL xiiVisibilityViewPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisibilityViewPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>(), true};
  xiiVisibilityViewPass();
  ~xiiVisibilityViewPass() override;
  /// Optional exact candidate stream. Empty selections use one invalid-index sentinel.
  xiiRenderPipelineBufferInputPin          m_InputCandidates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin         m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                                m_uiVisibilityMask  = 0xFFFFFFFFU;
  xiiUInt32                                m_uiRequiredFlags   = xiiGpuInstanceFlags::Enabled;
  xiiUInt32                                m_uiExcludedFlags   = 0;
  float                                    m_fLodHysteresis    = 0.1f;
  float                                    m_fLodScreenScale   = 1.0f;
  bool                                     m_bUseViewSelection = true;
  bool                                     m_bEnabled          = true;
  /// With no GPU-supplied candidate stream, inspect the exact extracted view selection.
  /// GPU-supplied candidates use a conservative world-wide presence test.
  bool                                     m_bSkipEmptySelection = false;
  void                                     GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                                Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override;
  void                                     InvalidateTemporalHistory() override;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bEnabled ? xiiRenderExtractionCategory::Geometry : xiiRenderExtractionCategory::None; }
  xiiBitflags<xiiRenderPipelineFeature>    GetRuntimeFeatures() const override { return m_bEnabled ? xiiRenderPipelineFeature::GpuScene : xiiRenderPipelineFeature::None; }

private:
  struct State;
  xiiSharedPtr<State> m_pState;
};
