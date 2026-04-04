#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Pass 60 â€” Tone Mapping.
///
/// Graphics/Compute. Converts HDR scene colour to LDR using a selectable operator.
/// Default is ACES filmics. Exposure from the eye-adaptation pass is applied here.
struct XII_GRAPHICSCORE_DLL xiiToneMappingPass
{
  enum class EOperator : xiiUInt8
  {
    ACES    = 0,
    AgX     = 1,
    Reinhard = 2,
    Uncharted2 = 3,
  };

  EOperator m_Operator      = EOperator::ACES;
  float     m_fExposureBias = 0.0f; ///< Manual exposure bias in EV stops.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ToneMappingPass";
  bool      m_bActive = true;
};

void xiiPopulateToneMappingPass(xiiToneMappingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

