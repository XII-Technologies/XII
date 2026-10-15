/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/Graph.h>
#include <Core/Graph/GraphMappings.h>
#include <Foundation/Containers/HashSet.h>

/// Non-owning subset of one graph object. The source must outlive the view. No subscriptions or
/// membership bookkeeping are added to the source, so source mutation costs are unchanged.
/// Queries validate generations and current endpoints. Induced views include newly inserted edges
/// between selected vertices; selected views include only explicitly selected edges. Enumeration
/// follows source dense order and costs O(V + E). Reacquire enumerations after source mutation.
/// Views stay with their source object when topology moves, just like graph subscriptions.
class XII_CORE_DLL xiiGraphView
{
public:
  enum class Mode
  {
    Selected,
    Induced
  };
  explicit xiiGraphView(const xiiGraph& graph) : m_Graph(graph) {}

  /// Transactional selection. Every edge must currently connect two selected vertices.
  xiiResult Select(xiiArrayPtr<const xiiGraphVertexHandle> vertices, Mode mode = Mode::Induced, xiiArrayPtr<const xiiGraphEdgeHandle> edges = {});
  bool      Contains(xiiGraphVertexHandle vertex) const;
  bool      Contains(xiiGraphEdgeHandle edge) const;
  void      Enumerate(xiiDynamicArray<xiiGraphVertexHandle>& out_vertices, xiiDynamicArray<xiiGraphEdgeHandle>& out_edges) const;
  /// Explicitly discard stale memberships, without affecting graph topology.
  void Refresh();
  /// Follow an explicit CopyFrom/Compact map into this source. Missing entries are dropped.
  /// Call before Refresh after compaction. No automatic update or source ownership transfer occurs.
  xiiResult Remap(const xiiGraphHandleMap& mapping);
  /// Copy the current view into an independent graph using the existing snapshot machinery.
  /// Failure leaves destination and mapping unchanged. Graph-owned identities are never reused.
  xiiResult       CopyTo(xiiGraph& destination, xiiGraphHandleMap& out_mapping) const;
  const xiiGraph& GetGraph() const { return m_Graph; }

private:
  const xiiGraph&                  m_Graph;
  Mode                             m_Mode = Mode::Induced;
  xiiHashSet<xiiGraphVertexHandle> m_Vertices;
  xiiHashSet<xiiGraphEdgeHandle>   m_Edges;
};
