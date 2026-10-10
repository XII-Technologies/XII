/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Core/Graph/GraphHandles.h>

class xiiGraph;

/// Handlers observe consistent topology. Before-removal handles are valid; after-removal handles are stale.
/// Mutating the emitting graph from a handler or validator is rejected.
struct xiiGraphEvent
{
  XII_DECLARE_POD_TYPE();
  enum class Type
  {
    VertexAdded,
    BeforeVertexRemoved,
    VertexRemoved,
    EdgeAdded,
    BeforeEdgeRemoved,
    EdgeRemoved,
    EdgeReconnected,
    BeforeReset,
    Reset
  };
  Type                 m_Type;
  const xiiGraph*      m_pGraph = nullptr;
  xiiGraphVertexHandle m_hVertex;
  xiiGraphEdgeHandle   m_hEdge;
  xiiGraphVertexHandle m_hSource;
  xiiGraphVertexHandle m_hTarget;
};

XII_DEFINE_AS_POD_TYPE(xiiGraphEvent::Type);
