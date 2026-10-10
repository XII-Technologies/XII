/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Core/Graph/Graph.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>

/// Index-to-runtime-handle mapping for attaching consumer payloads after loading a topology snapshot.
struct xiiGraphLoadedHandles
{
  explicit xiiGraphLoadedHandles(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator()) :
    m_Vertices(pAllocator), m_Edges(pAllocator) {}
  xiiDynamicArray<xiiGraphVertexHandle> m_Vertices;
  xiiDynamicArray<xiiGraphEdgeHandle>   m_Edges;
};

/// Reflected, portable topology data. Array indices identify serialized vertices/edges, including parallel edges.
/// Runtime handles and algorithm state are deliberately excluded. Consumer payload formats can use these indices.
struct XII_CORE_DLL xiiGraphSnapshot
{
  explicit xiiGraphSnapshot(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());
  void      Capture(const xiiGraph& graph);
  xiiResult Validate() const;
  /// Transactional: failed validation/loading leaves the destination and optional handle mapping unchanged.
  xiiResult Apply(xiiGraph& graph, xiiGraphLoadedHandles* out_pHandles = nullptr) const;
  xiiResult Save(xiiStreamWriter& stream) const;
  /// Bounded, versioned, endian-independent reads. Incomplete/unsupported input leaves this snapshot unchanged.
  xiiResult Load(xiiStreamReader& stream, xiiUInt32 vertexLimit = 1024 * 1024, xiiUInt32 edgeLimit = 4 * 1024 * 1024);

  xiiUInt32                  m_uiVertexCount = 0;
  xiiDynamicArray<xiiUInt32> m_EdgeSources;
  xiiDynamicArray<xiiUInt32> m_EdgeTargets;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraphSnapshot);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGraph);
