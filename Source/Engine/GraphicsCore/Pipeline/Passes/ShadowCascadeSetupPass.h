#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 15 â€” Directional Cascade Setup.
///
/// CPU/Graphics. Computes stable cascade splits and projection matrices for the directional
/// (sun) shadow. Uses camera frustum + sun direction to derive N cascade volumes.
/// Stable texel-snapping is applied to prevent shadow shimmer. Cascade matrices are
/// uploaded to GPU as a structured buffer.
struct XII_GRAPHICSCORE_DLL xiiShadowCascadeSetupPass
{
  xiiUInt32 m_uiCascadeCount     = 4u;     ///< Number of CSM cascades.
  float     m_fSplitLambda       = 0.85f;  ///< PSSM split lambda [0=uniform, 1=logarithmic].
  float     m_fShadowMapSize     = 2048.0f;
  float     m_fMaxShadowDistance = 300.0f;
  xiiSharedPtr<xiiGALBuffer> m_pCascadeMatrixBuffer; ///< float4x4 * cascadeCount view-proj matrices.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "ShadowCascadeSetupPass";
  bool      m_bActive = true;
};

void xiiPopulateShadowCascadeSetupPass(xiiShadowCascadeSetupPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

