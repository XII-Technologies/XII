/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/GraphEvents.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>

struct xiiGraphHandleMap;

/// General directed multigraph, with stable, graph-owned handles and no domain-specific semantics.
/// Parallel edges, loops, cycles and isolated vertices are supported. Lookup/degree/edge removal are O(1),
/// insertion amortized O(1), vertex removal O(in + out), enumeration O(V + E). Adjacency allocates nothing.
///
/// Unrelated handles survive mutation and Reserve. Reacquire all views, dense indices and cursors after any
/// mutation. Adjacency visits newest edges first; dense enumeration uses swap removal. Removed/foreign handles
/// are rejected. Generations never wrap: exhausted slots retire. Compact explicitly remaps all live handles.
///
/// Concurrent const access is supported; use xiiSynchronizedGraph for synchronized mutation. Validators,
/// event handlers and algorithm visitors must not mutate the graph. Instance subscriptions/validators remain
/// with the graph object when topology is moved. CopyFrom provides explicit, recoverable copying and remapping.
class XII_CORE_DLL xiiGraph
{
  using LocalId = xiiGraphElementId::LocalId;

public:
  explicit xiiGraph(xiiAllocator* pAllocator = xiiDefaultAllocatorWrapper::GetAllocator());
  ~xiiGraph();
  xiiGraph(xiiGraph&& other);
  xiiGraph(const xiiGraph&)       = delete;
  void operator=(const xiiGraph&) = delete;

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
  /// Limits bound occupied slots (including retired slots), and fail before mutation when exceeded.
  xiiResult SetLimits(xiiUInt32 vertexLimit, xiiUInt32 edgeLimit);
  xiiResult TryAddVertex(xiiGraphVertexHandle& out_hVertex);
  xiiResult TryAddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeHandle& out_hEdge);
  xiiResult TryRemoveVertex(xiiGraphVertexHandle hVertex);
  xiiResult TryRemoveEdge(xiiGraphEdgeHandle hEdge);
  xiiResult TryClear();
  /// Convenience forms return invalid handles/false on failure. Prefer Try forms when failure must propagate.
  xiiGraphVertexHandle AddVertex();
  xiiGraphEdgeHandle   AddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget);
  bool                 RemoveVertex(xiiGraphVertexHandle hVertex);
  bool                 RemoveEdge(xiiGraphEdgeHandle hEdge);
  void                 Clear();

  /// Reconnect without changing edge identity. Failure leaves the original connectivity unchanged.
  xiiResult ReconnectEdge(xiiGraphEdgeHandle hEdge, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget);
  using ConnectionValidator = xiiDelegate<xiiResult(const xiiGraph&, xiiGraphVertexHandle, xiiGraphVertexHandle, xiiGraphEdgeHandle)>;
  xiiResult SetConnectionValidator(ConnectionValidator validator);
  xiiResult ValidateConnection(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeHandle hExistingEdge = {}) const;
  using Event = xiiEvent<const xiiGraphEvent&>;
  const Event& GetEvents() const { return m_Events; }

  /// Transactional replacement. Copy creates new handles; move transfers source handles and empties source.
  /// Limits and validation of the destination apply. Subscriptions and validators are never copied/moved.
  xiiResult CopyFrom(const xiiGraph& source, xiiGraphHandleMap* out_pMapping = nullptr);
  xiiResult MoveFrom(xiiGraph&& source);
  /// Rebuild only live storage. Old handles become stale; output maps them to the new handles on success.
  xiiResult     Compact(xiiGraphHandleMap& out_mapping);
  xiiAllocator* GetAllocator() const { return m_pAllocator; }
  xiiUInt32     GetVertexLimit() const { return m_uiVertexLimit; }
  xiiUInt32     GetEdgeLimit() const { return m_uiEdgeLimit; }

  bool                                    IsValid(xiiGraphVertexHandle hVertex) const;
  bool                                    IsValid(xiiGraphEdgeHandle hEdge) const;
  xiiUInt32                               GetVertexCount() const;
  xiiUInt32                               GetEdgeCount() const;
  xiiArrayPtr<const xiiGraphVertexHandle> GetVertices() const;
  xiiArrayPtr<const xiiGraphEdgeHandle>   GetEdges() const;
  /// Transient indices into GetVertices()/GetEdges(). Invalid handles return xiiInvalidIndex.
  xiiUInt32 GetVertexIndex(xiiGraphVertexHandle hVertex) const;
  xiiUInt32 GetEdgeIndex(xiiGraphEdgeHandle hEdge) const;
  /// Invalid vertices return zero degrees and empty adjacency cursors.
  xiiUInt32    GetInDegree(xiiGraphVertexHandle hVertex) const;
  xiiUInt32    GetOutDegree(xiiGraphVertexHandle hVertex) const;
  EdgeIterator GetIncomingEdges(xiiGraphVertexHandle hVertex) const;
  EdgeIterator GetOutgoingEdges(xiiGraphVertexHandle hVertex) const;
  /// Invalid edges return invalid endpoints.
  xiiGraphVertexHandle GetSource(xiiGraphEdgeHandle hEdge) const;
  xiiGraphVertexHandle GetTarget(xiiGraphEdgeHandle hEdge) const;
  xiiGraphEdgeHandle   FindEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget) const;

private:
  xiiResult MoveFromInternal(xiiGraph&& source, const xiiGraphHandleMap* pMapping);
  struct Storage;
  xiiGraphVertexHandle  MakeVertexHandle(LocalId id) const;
  xiiGraphEdgeHandle    MakeEdgeHandle(LocalId id) const;
  void                  Broadcast(xiiGraphEvent event);
  void                  LinkEdge(LocalId edge);
  void                  UnlinkEdge(LocalId edge);
  xiiUniquePtr<Storage> m_pStorage;
  xiiAllocator*         m_pAllocator;
  Event                 m_Events;
  ConnectionValidator   m_Validator;
  mutable bool          m_bDispatching  = false;
  xiiUInt32             m_uiVertexLimit = MaxElementCount;
  xiiUInt32             m_uiEdgeLimit   = MaxElementCount;
};
