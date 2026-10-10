/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphMappings.h>
#include <Core/Graph/Implementation/GraphIdentity.h>
#include <Core/Graph/Implementation/GraphStorage.h>
#include <Foundation/Types/ScopeExit.h>

xiiGraph::xiiGraph(xiiAllocator* pAllocator) : m_pAllocator(pAllocator), m_Events(pAllocator) {}
xiiGraph::~xiiGraph() = default;
xiiGraph::xiiGraph(xiiGraph&& other) : xiiGraph(other.m_pAllocator)
{
  MoveFrom(std::move(other)).IgnoreResult();
}

xiiResult xiiGraph::TryInitialize()
{
  if (m_bDispatching)
    return XII_FAILURE;
  if (m_pStorage)
    return XII_SUCCESS;
  static xiiAtomicInteger<xiiUInt64> nextId;
  xiiUInt64                          identity;
  XII_SUCCEED_OR_RETURN(xiiInternal::AllocateGraphIdentity(nextId, identity));
  // Metadata allocation is checked directly; Foundation containers retain their normal allocator contract.
  void* memory = m_pAllocator->Allocate(sizeof(Storage), alignof(Storage));
  if (!memory)
    return XII_FAILURE;
  m_pStorage = xiiUniquePtr<Storage>(new (memory) Storage(m_pAllocator, identity), m_pAllocator);
  return XII_SUCCESS;
}

xiiResult xiiGraph::SetLimits(xiiUInt32 vertexLimit, xiiUInt32 edgeLimit)
{
  if (m_bDispatching || vertexLimit > MaxElementCount || edgeLimit > MaxElementCount ||
      (m_pStorage && (vertexLimit < m_pStorage->m_Vertices.GetCount() || edgeLimit < m_pStorage->m_Edges.GetCount())))
    return XII_FAILURE;
  m_uiVertexLimit = vertexLimit;
  m_uiEdgeLimit   = edgeLimit;
  return XII_SUCCESS;
}

xiiResult xiiGraph::Reserve(xiiUInt32 vertexCapacity, xiiUInt32 edgeCapacity)
{
  if (m_bDispatching || vertexCapacity > m_uiVertexLimit || edgeCapacity > m_uiEdgeLimit)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(TryInitialize());
  m_pStorage->m_Vertices.Reserve(vertexCapacity);
  m_pStorage->m_Edges.Reserve(edgeCapacity);
  m_pStorage->m_VertexHandles.Reserve(vertexCapacity);
  m_pStorage->m_EdgeHandles.Reserve(edgeCapacity);
  return XII_SUCCESS;
}

xiiUInt32                               xiiGraph::GetVertexCount() const { return m_pStorage ? m_pStorage->m_VertexHandles.GetCount() : 0; }
xiiUInt32                               xiiGraph::GetEdgeCount() const { return m_pStorage ? m_pStorage->m_EdgeHandles.GetCount() : 0; }
xiiArrayPtr<const xiiGraphVertexHandle> xiiGraph::GetVertices() const { return m_pStorage ? xiiArrayPtr<const xiiGraphVertexHandle>(m_pStorage->m_VertexHandles.GetArrayPtr()) : xiiArrayPtr<const xiiGraphVertexHandle>(); }
xiiArrayPtr<const xiiGraphEdgeHandle>   xiiGraph::GetEdges() const { return m_pStorage ? xiiArrayPtr<const xiiGraphEdgeHandle>(m_pStorage->m_EdgeHandles.GetArrayPtr()) : xiiArrayPtr<const xiiGraphEdgeHandle>(); }

void xiiGraph::Broadcast(xiiGraphEvent event)
{
  if (m_Events.IsEmpty())
    return;
  const bool wasDispatching = m_bDispatching;
  m_bDispatching            = true;
  XII_SCOPE_EXIT(m_bDispatching = wasDispatching);
  event.m_pGraph = this;
  m_Events.Broadcast(event);
}

xiiResult xiiGraph::SetConnectionValidator(ConnectionValidator validator)
{
  if (m_bDispatching)
    return XII_FAILURE;
  m_Validator = std::move(validator);
  return XII_SUCCESS;
}

xiiResult xiiGraph::ValidateConnection(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeHandle hExistingEdge) const
{
  if (m_bDispatching || !IsValid(hSource) || !IsValid(hTarget) || (!hExistingEdge.IsInvalidated() && !IsValid(hExistingEdge)))
    return XII_FAILURE;
  if (!m_Validator.IsValid())
    return XII_SUCCESS;
  m_bDispatching = true;
  XII_SCOPE_EXIT(m_bDispatching = false);
  return m_Validator(*this, hSource, hTarget, hExistingEdge);
}

xiiResult xiiGraph::MoveFrom(xiiGraph&& source)
{
  if (&source == this)
    return m_bDispatching ? XII_FAILURE : XII_SUCCESS;
  if (m_bDispatching || source.m_bDispatching ||
      (source.m_pStorage && (source.m_pStorage->m_Vertices.GetCount() > m_uiVertexLimit || source.m_pStorage->m_Edges.GetCount() > m_uiEdgeLimit)))
    return XII_FAILURE;
  source.m_bDispatching = true;
  m_bDispatching        = true;
  XII_SCOPE_EXIT(source.m_bDispatching = false; m_bDispatching = false);
  // Validate a snapshot on the source before transferring any ownership.
  if (m_Validator.IsValid())
  {
    for (auto edge : source.GetEdges())
      XII_SUCCEED_OR_RETURN(m_Validator(source, source.GetSource(edge), source.GetTarget(edge), edge));
  }
  Broadcast({xiiGraphEvent::Type::BeforeReset});
  source.Broadcast({xiiGraphEvent::Type::BeforeReset});
  m_pStorage = std::move(source.m_pStorage);
  Broadcast({xiiGraphEvent::Type::Reset});
  source.Broadcast({xiiGraphEvent::Type::Reset});
  return XII_SUCCESS;
}

xiiResult xiiGraph::CopyFrom(const xiiGraph& source, xiiGraphHandleMap* out_pMapping)
{
  if (m_bDispatching || source.GetVertexCount() > m_uiVertexLimit || source.GetEdgeCount() > m_uiEdgeLimit)
    return XII_FAILURE;
  xiiGraph copy(m_pAllocator);
  XII_SUCCEED_OR_RETURN(copy.SetLimits(m_uiVertexLimit, m_uiEdgeLimit));
  XII_SUCCEED_OR_RETURN(copy.Reserve(source.GetVertexCount(), source.GetEdgeCount()));
  xiiGraphHandleMap mapping(m_pAllocator);
  for (auto vertex : source.GetVertices())
  {
    xiiGraphVertexHandle cloned;
    XII_SUCCEED_OR_RETURN(copy.TryAddVertex(cloned));
    mapping.m_Vertices.Insert(vertex, cloned);
  }
  for (auto edge : source.GetEdges())
  {
    xiiGraphEdgeHandle cloned;
    XII_SUCCEED_OR_RETURN(copy.TryAddEdge(mapping.m_Vertices[source.GetSource(edge)], mapping.m_Vertices[source.GetTarget(edge)], cloned));
    mapping.m_Edges.Insert(edge, cloned);
  }
  XII_SUCCEED_OR_RETURN(MoveFrom(std::move(copy)));
  if (out_pMapping)
  {
    out_pMapping->m_Vertices = std::move(mapping.m_Vertices);
    out_pMapping->m_Edges    = std::move(mapping.m_Edges);
  }
  return XII_SUCCESS;
}

xiiResult xiiGraph::Compact(xiiGraphHandleMap& out_mapping)
{
  return CopyFrom(*this, &out_mapping);
}
