#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief Pass 6 — LOD Selection and Meshlet Classification.
///
/// Async Compute. Uses updated bounds and camera data to select per-instance LOD levels
/// and classify geometry into draw metadata bins. Kept branchless and cache-friendly:
/// one thread per instance, outputs compacted into a tightly-packed structured buffer.
class XII_GRAPHICSCORE_DLL xiiLodAndMeshletPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLodAndMeshletPass, xiiRenderPipelinePass);

public:
  xiiLodAndMeshletPass();
  virtual ~xiiLodAndMeshletPass();

  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard) override;

  float m_fLOD0Distance = 10.0f;
  float m_fLOD1Distance = 30.0f;
  float m_fLOD2Distance = 80.0f;
  float m_fLOD3Distance = 200.0f;

private:
  xiiSharedPtr<xiiGALBuffer> m_pLODMetadataBuffer; ///< RW: per-instance (lodLevel:8, materialBin:8, flags:16) packed uint.
};
