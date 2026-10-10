/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/IdTable.h>

/// Internal identity shared by the two strongly typed graph handles. Not a persistent asset identifier.
struct xiiGraphElementId
{
  XII_DECLARE_POD_TYPE();

  using LocalId = xiiGenericId<32, 31>;
  LocalId   m_LocalId;
  xiiUInt64 m_uiGraphId = 0;

  bool operator==(const xiiGraphElementId& other) const = default;
  bool operator<(const xiiGraphElementId& other) const
  {
    return m_uiGraphId < other.m_uiGraphId || (m_uiGraphId == other.m_uiGraphId && m_LocalId < other.m_LocalId);
  }
  void Invalidate()
  {
    m_LocalId.Invalidate();
    m_uiGraphId = 0;
  }
  bool IsInvalidated() const { return m_uiGraphId == 0 || m_LocalId.IsInvalidated(); }
};

struct xiiGraphVertexHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGraphVertexHandle, xiiGraphElementId);
};

struct xiiGraphEdgeHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGraphEdgeHandle, xiiGraphElementId);
};

template <>
struct xiiHashHelper<xiiGraphVertexHandle>
{
  static xiiUInt32 Hash(xiiGraphVertexHandle value)
  {
    const auto      id     = value.GetInternalID();
    const xiiUInt64 data[] = {id.m_LocalId.m_Data, id.m_uiGraphId};
    return xiiHashingUtils::xxHash32(data, sizeof(data));
  }
  static bool Equal(xiiGraphVertexHandle a, xiiGraphVertexHandle b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiGraphEdgeHandle>
{
  static xiiUInt32 Hash(xiiGraphEdgeHandle value)
  {
    const auto      id     = value.GetInternalID();
    const xiiUInt64 data[] = {id.m_LocalId.m_Data, id.m_uiGraphId};
    return xiiHashingUtils::xxHash32(data, sizeof(data));
  }
  static bool Equal(xiiGraphEdgeHandle a, xiiGraphEdgeHandle b) { return a == b; }
};

/// Topology-only directed multigraph. Parallel edges, self-loops, cycles and isolated vertices are permitted.
///
/// Owns structural records only; associate payloads externally using the hashable handles. All handles are
/// specific to one graph lifetime. Foreign, removed and default handles are rejected, including after Clear().
/// Slots at their final generation are retired instead of allowing a stale handle to become valid again.
/// Graphs cannot be copied or moved. Process-wide graph identity exhaustion is a fatal error.
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

  void                 Reserve(xiiUInt32 vertexCapacity, xiiUInt32 edgeCapacity);
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
  const xiiUInt64                       m_uiGraphId;
};
