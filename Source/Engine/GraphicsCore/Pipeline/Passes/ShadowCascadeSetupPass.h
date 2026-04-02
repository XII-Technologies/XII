#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 15 — Directional Cascade Setup.
///
/// CPU/Graphics. Computes stable cascade splits and projection matrices for the directional
/// (sun) shadow. Uses camera frustum + sun direction to derive N cascade volumes.
/// Stable texel-snapping is applied to prevent shadow shimmer. Cascade matrices are
/// uploaded to GPU as a structured buffer.
class XII_GRAPHICSCORE_DLL xiiShadowCascadeSetupPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCascadeSetupPass, xiiRenderPipelinePass);

public:
  xiiShadowCascadeSetupPass();
  virtual ~xiiShadowCascadeSetupPass();
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  xiiUInt32 m_uiCascadeCount     = 4u;     ///< Number of CSM cascades.
  float     m_fSplitLambda       = 0.85f;  ///< PSSM split lambda [0=uniform, 1=logarithmic].
  float     m_fShadowMapSize     = 2048.0f;
  float     m_fMaxShadowDistance = 300.0f;

private:
  xiiSharedPtr<xiiGALBuffer> m_pCascadeMatrixBuffer; ///< float4x4 * cascadeCount view-proj matrices.
};
