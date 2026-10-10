/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Core/Graph/GraphHandles.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/IdTable.h>

/// Topology-only directed multigraph. Parallel edges, self-loops, cycles and isolated vertices are permitted.
///
/// Owns structural records only; associate payloads externally using the hashable handles. All handles are
/// specific to one graph lifetime. Foreign, removed and default handles are rejected, including after Clear().
/// Slots at their final generation are retired instead of allowing a stale handle to become valid again.
/// Identity and capacity exhaustion return failure through the Try operations.
///
/// Lookup, degree queries and edge removal are O(1); insertion is amortized O(1). Vertex removal is O(in + out).
/// Enumeration is O(V + E), independent of holes in ID storage. FindEdge is O(out-degree).
/// Clear is O(V + E), retains allocations and invalidates every handle. Reserve may relocate internal storage.
/// No pointers to structural records are exposed. All views, indices and iterators must be reacquired after
/// ANY mutation (including Reserve); unrelated element handles remain valid. Enumeration order is unspecified
/// after removal. Adjacency visits newest edges first. Identical mutation sequences yield identical orders.
///
/// Concurrent const access is allowed; mutation requires exclusive external synchronization. Algorithms and
/// callbacks must not mutate the graph. Reflection, events, payload ownership and serialization belong to consumers.
class XII_CORE_DLL xiiGraph
{
  using LocalId = xiiGraphElementId::LocalId;

public:
  explicit xiiGraph(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());
  ~xiiGraph();
  xiiGraph(const xiiGraph&)       = delete;
  void operator=(const xiiGraph&) = delete;

  /// Allocation-free, read-only adjacency cursor. Invalid vertices produce an empty cursor.
  class XII_CORE_DLL EdgeIterator
  {
  public:
    bool               IsValid() const { return !m_EdgeId.IsInvalidated(); }
    xiiGraphEdgeHandle GetHandle() const;
    void               Next();
    void               operator++() { Next(); }

  private:
    friend class xiiGraph;
    EdgeIterator(const xiiGraph* pGraph, LocalId edgeId, bool incoming);
    const xiiGraph* m_pGraph;
    LocalId         m_EdgeId;
    bool            m_bIncoming;
  };

  static constexpr xiiUInt32 MaxElementCount = 0xFFFFFFC0U;
  xiiResult                  TryInitialize();
  xiiResult                  Reserve(xiiUInt32 vertexCapacity, xiiUInt32 edgeCapacity);
  /// Limits provide predictable, recoverable capacity failures. Existing occupied slots must fit.
  xiiResult            SetLimits(xiiUInt32 vertexLimit, xiiUInt32 edgeLimit);
  xiiResult            TryAddVertex(xiiGraphVertexHandle& out_hVertex);
  xiiResult            TryAddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeHandle& out_hEdge);
  xiiGraphVertexHandle AddVertex();
  /// Returns an invalid handle without modifying the graph if either endpoint is invalid or foreign.
  xiiGraphEdgeHandle AddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget);
  /// Invalid, foreign or already removed handles return false without modifying the graph.
  bool RemoveVertex(xiiGraphVertexHandle hVertex);
  bool RemoveEdge(xiiGraphEdgeHandle hEdge);
  void Clear();

  bool                                    IsValid(xiiGraphVertexHandle hVertex) const;
  bool                                    IsValid(xiiGraphEdgeHandle hEdge) const;
  xiiUInt32                               GetVertexCount() const { return m_VertexHandles.GetCount(); }
  xiiUInt32                               GetEdgeCount() const { return m_EdgeHandles.GetCount(); }
  xiiArrayPtr<const xiiGraphVertexHandle> GetVertices() const { return m_VertexHandles; }
  xiiArrayPtr<const xiiGraphEdgeHandle>   GetEdges() const { return m_EdgeHandles; }

  /// Dense index into GetVertices(), useful for transient algorithm/payload arrays. Invalid returns xiiInvalidIndex.
  /// This is NOT a stable identity: any mutation may change it.
  xiiUInt32    GetVertexIndex(xiiGraphVertexHandle hVertex) const;
  xiiUInt32    GetInDegree(xiiGraphVertexHandle hVertex) const;
  xiiUInt32    GetOutDegree(xiiGraphVertexHandle hVertex) const;
  EdgeIterator GetIncomingEdges(xiiGraphVertexHandle hVertex) const;
  EdgeIterator GetOutgoingEdges(xiiGraphVertexHandle hVertex) const;
  /// Endpoint queries return invalid handles for an invalid edge. Use adjacency cursors plus these to visit neighbours.
  xiiGraphVertexHandle GetSource(xiiGraphEdgeHandle hEdge) const;
  xiiGraphVertexHandle GetTarget(xiiGraphEdgeHandle hEdge) const;
  /// Finds the newest surviving source-to-target edge, or returns an invalid handle.
  xiiGraphEdgeHandle FindEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget) const;

private:
  struct Vertex
  {
    LocalId   m_FirstIncoming;
    LocalId   m_FirstOutgoing;
    xiiUInt32 m_uiInDegree   = 0;
    xiiUInt32 m_uiOutDegree  = 0;
    xiiUInt32 m_uiDenseIndex = xiiInvalidIndex;
  };
  struct Edge
  {
    LocalId   m_Source;
    LocalId   m_Target;
    LocalId   m_PreviousIncoming;
    LocalId   m_NextIncoming;
    LocalId   m_PreviousOutgoing;
    LocalId   m_NextOutgoing;
    xiiUInt32 m_uiDenseIndex = xiiInvalidIndex;
  };

  xiiGraphVertexHandle                  MakeVertexHandle(LocalId id) const;
  xiiGraphEdgeHandle                    MakeEdgeHandle(LocalId id) const;
  xiiIdTable<LocalId, Vertex>           m_Vertices;
  xiiIdTable<LocalId, Edge>             m_Edges;
  xiiDynamicArray<xiiGraphVertexHandle> m_VertexHandles;
  xiiDynamicArray<xiiGraphEdgeHandle>   m_EdgeHandles;
  xiiUInt64                             m_uiGraphId     = 0;
  xiiUInt32                             m_uiVertexLimit = MaxElementCount;
  xiiUInt32                             m_uiEdgeLimit   = MaxElementCount;
};
