#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 4 â€” Skinning and Morph Compute.
///
/// Runs on Async Compute. Reads raw vertex streams and the bone palette / morph weights uploaded
/// by PerFrameBufferUploadPass, then writes deformed vertex streams consumed by the instance
/// transform pass and the depth prepass.
struct XII_GRAPHICSCORE_DLL xiiSkinningAndMorphPass
{
  xiiSharedPtr<xiiGALBuffer> m_pSkinnedVertexBuffer;   ///< RW output: deformed vertex positions (float4 per vertex).
  xiiSharedPtr<xiiGALBuffer> m_pSkinningInputBuffer;    ///< SRV input: raw vertex positions + skin weights.
  xiiSharedPtr<xiiGALBuffer> m_pBonePaletteBuffer;      ///< SRV input: per-skeleton bone palette (float4x3 matrices).
  xiiSharedPtr<xiiGALBuffer> m_pMorphWeightsBuffer;     ///< SRV input: morph target weights + delta streams.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "SkinningAndMorphPass";
  bool      m_bActive = true;
};

void xiiPopulateSkinningAndMorphPass(xiiSkinningAndMorphPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

