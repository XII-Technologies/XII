/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

/// One frame-sliced derived vertex allocation; never owns authored component state.
struct xiiRayTracingDeformedVertices
{
  ~xiiRayTracingDeformedVertices();
  xiiSharedPtr<xiiGALBuffer>   m_Buffer, m_Source;
  xiiGALBindlessResourceHandle m_Srv;
  xiiDynamicArray<xiiMat4>     m_Pose;
  xiiDynamicArray<xiiVec4U32>  m_Morphs;
  xiiUInt32                    m_MeshRevision = 0;
  xiiUInt64                    m_Revision = 0, m_ReadyRevision = 0, m_LastUse = 0;
  xiiUInt64                    m_SetupSerial = 0;
  xiiRenderGraphBufferHandle   m_Handle;
};

/// Returns a typed dependency on the deformed native vertex stream. Reuses successful
/// unchanged frame slots, and only publishes readiness when the dispatch is recorded.
xiiResult xiiAddRayTracingDeformation(xiiRenderGraph& graph, xiiUInt64 frame, xiiStringView name,
                                      xiiRayTracingDeformedVertices& cache, xiiSharedPtr<xiiGALComputePipelineState>& pipeline,
                                      const xiiGpuInstanceData& instances, xiiUInt32 drawIndex, bool& dispatched);
