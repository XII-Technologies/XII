#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 4 — Skinning and Morph Compute.
///
/// Runs on Async Compute. Reads raw vertex streams and the bone palette / morph weights uploaded
/// by PerFrameBufferUploadPass, then writes deformed vertex streams consumed by the instance
/// transform pass and the depth prepass.
class XII_GRAPHICSCORE_DLL xiiSkinningAndMorphPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinningAndMorphPass, xiiRenderPipelinePass);

public:
  xiiSkinningAndMorphPass();
  virtual ~xiiSkinningAndMorphPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

private:
  xiiSharedPtr<xiiGALBuffer> m_pSkinnedVertexBuffer;   ///< RW output: deformed vertex positions (float4 per vertex).
  xiiSharedPtr<xiiGALBuffer> m_pSkinningInputBuffer;    ///< SRV input: raw vertex positions + skin weights.
  xiiSharedPtr<xiiGALBuffer> m_pBonePaletteBuffer;      ///< SRV input: per-skeleton bone palette (float4x3 matrices).
  xiiSharedPtr<xiiGALBuffer> m_pMorphWeightsBuffer;     ///< SRV input: morph target weights + delta streams.
};
