/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Graph/Graph.h>
#include <Foundation/Threading/AtomicInteger.h>

namespace
{
  xiiUInt64 AllocateGraphId()
  {
    static xiiAtomicInteger<xiiUInt64> nextId;
    const xiiUInt64                    id = nextId.Increment();
    XII_ASSERT_ALWAYS(id != 0, "Graph identity space exhausted.");
    return id;
  }
} // namespace

xiiGraph::xiiGraph(xiiAllocator* pAllocator) :
  m_Vertices(pAllocator), m_Edges(pAllocator), m_VertexHandles(pAllocator), m_EdgeHandles(pAllocator), m_uiGraphId(AllocateGraphId())
{
}

xiiGraph::~xiiGraph() = default;

void xiiGraph::Reserve(xiiUInt32 vertexCapacity, xiiUInt32 edgeCapacity)
{
  m_Vertices.Reserve(vertexCapacity);
  m_Edges.Reserve(edgeCapacity);
  m_VertexHandles.Reserve(vertexCapacity);
  m_EdgeHandles.Reserve(edgeCapacity);
}

xiiGraphVertexHandle xiiGraph::MakeVertexHandle(LocalId id) const
{
  return id.IsInvalidated() ? xiiGraphVertexHandle() : xiiGraphVertexHandle(xiiGraphElementId{id, m_uiGraphId});
}

xiiGraphEdgeHandle xiiGraph::MakeEdgeHandle(LocalId id) const
{
  return id.IsInvalidated() ? xiiGraphEdgeHandle() : xiiGraphEdgeHandle(xiiGraphElementId{id, m_uiGraphId});
}

bool xiiGraph::IsValid(xiiGraphVertexHandle hVertex) const
{
  const auto id = hVertex.GetInternalID();
  return id.m_uiGraphId == m_uiGraphId && m_Vertices.Contains(id.m_LocalId) && m_Vertices[id.m_LocalId].m_uiDenseIndex != xiiInvalidIndex;
}

bool xiiGraph::IsValid(xiiGraphEdgeHandle hEdge) const
{
  const auto id = hEdge.GetInternalID();
  return id.m_uiGraphId == m_uiGraphId && m_Edges.Contains(id.m_LocalId) && m_Edges[id.m_LocalId].m_uiDenseIndex != xiiInvalidIndex;
}

xiiGraphVertexHandle xiiGraph::AddVertex()
{
  Vertex vertex;
  vertex.m_uiDenseIndex = GetVertexCount();
  const auto handle     = MakeVertexHandle(m_Vertices.Insert(vertex));
  m_VertexHandles.PushBack(handle);
  return handle;
}

xiiGraphEdgeHandle xiiGraph::AddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget)
{
  if (!IsValid(hSource) || !IsValid(hTarget))
    return {};

  const auto sourceId = hSource.GetInternalID().m_LocalId;
  const auto targetId = hTarget.GetInternalID().m_LocalId;
  auto&      source   = m_Vertices[sourceId];
  auto&      target   = m_Vertices[targetId];
  Edge       edge;
  edge.m_Source       = sourceId;
  edge.m_Target       = targetId;
  edge.m_NextOutgoing = source.m_FirstOutgoing;
  edge.m_NextIncoming = target.m_FirstIncoming;
  edge.m_uiDenseIndex = GetEdgeCount();
  const auto id       = m_Edges.Insert(edge);

  if (!edge.m_NextOutgoing.IsInvalidated())
    m_Edges[edge.m_NextOutgoing].m_PreviousOutgoing = id;
  if (!edge.m_NextIncoming.IsInvalidated())
    m_Edges[edge.m_NextIncoming].m_PreviousIncoming = id;
  source.m_FirstOutgoing = id;
  target.m_FirstIncoming = id;
  ++source.m_uiOutDegree;
  ++target.m_uiInDegree;
  const auto handle = MakeEdgeHandle(id);
  m_EdgeHandles.PushBack(handle);
  return handle;
}

bool xiiGraph::RemoveEdge(xiiGraphEdgeHandle hEdge)
{
  if (!IsValid(hEdge))
    return false;

  const auto id     = hEdge.GetInternalID().m_LocalId;
  const Edge edge   = m_Edges[id];
  auto&      source = m_Vertices[edge.m_Source];
  auto&      target = m_Vertices[edge.m_Target];
  if (edge.m_PreviousOutgoing.IsInvalidated())
    source.m_FirstOutgoing = edge.m_NextOutgoing;
  else
    m_Edges[edge.m_PreviousOutgoing].m_NextOutgoing = edge.m_NextOutgoing;
  if (!edge.m_NextOutgoing.IsInvalidated())
    m_Edges[edge.m_NextOutgoing].m_PreviousOutgoing = edge.m_PreviousOutgoing;

  if (edge.m_PreviousIncoming.IsInvalidated())
    target.m_FirstIncoming = edge.m_NextIncoming;
  else
    m_Edges[edge.m_PreviousIncoming].m_NextIncoming = edge.m_NextIncoming;
  if (!edge.m_NextIncoming.IsInvalidated())
    m_Edges[edge.m_NextIncoming].m_PreviousIncoming = edge.m_PreviousIncoming;
  --source.m_uiOutDegree;
  --target.m_uiInDegree;

  const auto last                                        = m_EdgeHandles.PeekBack();
  m_EdgeHandles[edge.m_uiDenseIndex]                     = last;
  m_Edges[last.GetInternalID().m_LocalId].m_uiDenseIndex = edge.m_uiDenseIndex;
  m_EdgeHandles.PopBack();
  // Keep exhausted slots occupied but inactive, so the Foundation free list never wraps their generation.
  if (id.m_Generation == 0x7FFFFFFFULL)
    m_Edges[id].m_uiDenseIndex = xiiInvalidIndex;
  else
    m_Edges.Remove(id);
  return true;
}

bool xiiGraph::RemoveVertex(xiiGraphVertexHandle hVertex)
{
  if (!IsValid(hVertex))
    return false;

  const auto id     = hVertex.GetInternalID().m_LocalId;
  auto&      vertex = m_Vertices[id];
  while (!vertex.m_FirstOutgoing.IsInvalidated())
    RemoveEdge(MakeEdgeHandle(vertex.m_FirstOutgoing));
  while (!vertex.m_FirstIncoming.IsInvalidated())
    RemoveEdge(MakeEdgeHandle(vertex.m_FirstIncoming));

  const auto last                                           = m_VertexHandles.PeekBack();
  m_VertexHandles[vertex.m_uiDenseIndex]                    = last;
  m_Vertices[last.GetInternalID().m_LocalId].m_uiDenseIndex = vertex.m_uiDenseIndex;
  m_VertexHandles.PopBack();
  if (id.m_Generation == 0x7FFFFFFFULL)
    vertex.m_uiDenseIndex = xiiInvalidIndex;
  else
    m_Vertices.Remove(id);
  return true;
}

void xiiGraph::Clear()
{
  while (!m_VertexHandles.IsEmpty())
    RemoveVertex(m_VertexHandles.PeekBack());
}

xiiUInt32 xiiGraph::GetVertexIndex(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiDenseIndex : xiiInvalidIndex;
}

xiiUInt32 xiiGraph::GetInDegree(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiInDegree : 0;
}

xiiUInt32 xiiGraph::GetOutDegree(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiOutDegree : 0;
}

xiiGraph::EdgeIterator xiiGraph::GetIncomingEdges(xiiGraphVertexHandle hVertex) const
{
  return EdgeIterator(this, IsValid(hVertex) ? m_Vertices[hVertex.GetInternalID().m_LocalId].m_FirstIncoming : LocalId(), true);
}

xiiGraph::EdgeIterator xiiGraph::GetOutgoingEdges(xiiGraphVertexHandle hVertex) const
{
  return EdgeIterator(this, IsValid(hVertex) ? m_Vertices[hVertex.GetInternalID().m_LocalId].m_FirstOutgoing : LocalId(), false);
}

xiiGraphVertexHandle xiiGraph::GetSource(xiiGraphEdgeHandle hEdge) const
{
  return IsValid(hEdge) ? MakeVertexHandle(m_Edges[hEdge.GetInternalID().m_LocalId].m_Source) : xiiGraphVertexHandle();
}

xiiGraphVertexHandle xiiGraph::GetTarget(xiiGraphEdgeHandle hEdge) const
{
  return IsValid(hEdge) ? MakeVertexHandle(m_Edges[hEdge.GetInternalID().m_LocalId].m_Target) : xiiGraphVertexHandle();
}

xiiGraphEdgeHandle xiiGraph::FindEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget) const
{
  if (!IsValid(hTarget))
    return {};
  for (auto it = GetOutgoingEdges(hSource); it.IsValid(); ++it)
  {
    if (GetTarget(it.GetHandle()) == hTarget)
      return it.GetHandle();
  }
  return {};
}

xiiGraph::EdgeIterator::EdgeIterator(const xiiGraph* pGraph, LocalId edgeId, bool incoming) :
  m_pGraph(pGraph), m_EdgeId(edgeId), m_bIncoming(incoming)
{
}

xiiGraphEdgeHandle xiiGraph::EdgeIterator::GetHandle() const
{
  return m_pGraph->MakeEdgeHandle(m_EdgeId);
}

void xiiGraph::EdgeIterator::Next()
{
  if (IsValid())
  {
    const auto& edge = m_pGraph->m_Edges[m_EdgeId];
    m_EdgeId         = m_bIncoming ? edge.m_NextIncoming : edge.m_NextOutgoing;
  }
}
