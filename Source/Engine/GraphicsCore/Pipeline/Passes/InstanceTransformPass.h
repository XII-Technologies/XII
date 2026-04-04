#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 5 â€” Instance Transform and Bounds Update.
///
/// Async Compute. Reads scene transform data and animation outputs (skinning results),
/// writes world matrices and updated axis-aligned bounds used by all culling passes.
struct XII_GRAPHICSCORE_DLL xiiInstanceTransformPass
{
  xiiSharedPtr<xiiGALBuffer> m_pWorldMatrixBuffer;  ///< RW: float4x3 per-instance world matrices.
  xiiSharedPtr<xiiGALBuffer> m_pBoundsBuffer;        ///< RW: float4 center + float4 extents per instance (AABB).
  xiiSharedPtr<xiiGALBuffer> m_pSceneTransformBuffer; ///< SRV: CPU-uploaded raw scene node transforms.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

  xiiString m_sName = "InstanceTransformPass";
  bool      m_bActive = true;
};

void xiiPopulateInstanceTransformPass(xiiInstanceTransformPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

