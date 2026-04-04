#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Pass 60 — Tone Mapping.
///
/// Graphics/Compute. Converts HDR scene colour to LDR using a selectable operator.
/// Default is ACES filmics. Exposure from the eye-adaptation pass is applied here.
class XII_GRAPHICSCORE_DLL xiiToneMappingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiToneMappingPass, xiiRenderPipelinePass);

public:
  enum class EOperator : xiiUInt8
  {
    ACES    = 0,
    AgX     = 1,
    Reinhard = 2,
    Uncharted2 = 3,
  };

  xiiToneMappingPass();
  virtual ~xiiToneMappingPass();
  void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  EOperator m_Operator      = EOperator::ACES;
  float     m_fExposureBias = 0.0f; ///< Manual exposure bias in EV stops.
};
