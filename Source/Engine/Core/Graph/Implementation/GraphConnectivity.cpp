/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/Implementation/GraphStorage.h>

xiiUInt32 xiiGraph::GetVertexIndex(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_pStorage->m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiDenseIndex : xiiInvalidIndex;
}

xiiUInt32 xiiGraph::GetEdgeIndex(xiiGraphEdgeHandle hEdge) const
{
  return IsValid(hEdge) ? m_pStorage->m_Edges[hEdge.GetInternalID().m_LocalId].m_uiDenseIndex : xiiInvalidIndex;
}

xiiUInt32 xiiGraph::GetInDegree(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_pStorage->m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiInDegree : 0;
}

xiiUInt32 xiiGraph::GetOutDegree(xiiGraphVertexHandle hVertex) const
{
  return IsValid(hVertex) ? m_pStorage->m_Vertices[hVertex.GetInternalID().m_LocalId].m_uiOutDegree : 0;
}

xiiGraph::EdgeIterator xiiGraph::GetIncomingEdges(xiiGraphVertexHandle hVertex) const
{
  return EdgeIterator(this, IsValid(hVertex) ? m_pStorage->m_Vertices[hVertex.GetInternalID().m_LocalId].m_FirstIncoming : LocalId(), true);
}

xiiGraph::EdgeIterator xiiGraph::GetOutgoingEdges(xiiGraphVertexHandle hVertex) const
{
  return EdgeIterator(this, IsValid(hVertex) ? m_pStorage->m_Vertices[hVertex.GetInternalID().m_LocalId].m_FirstOutgoing : LocalId(), false);
}

xiiGraphVertexHandle xiiGraph::GetSource(xiiGraphEdgeHandle hEdge) const
{
  return IsValid(hEdge) ? MakeVertexHandle(m_pStorage->m_Edges[hEdge.GetInternalID().m_LocalId].m_Source) : xiiGraphVertexHandle();
}

xiiGraphVertexHandle xiiGraph::GetTarget(xiiGraphEdgeHandle hEdge) const
{
  return IsValid(hEdge) ? MakeVertexHandle(m_pStorage->m_Edges[hEdge.GetInternalID().m_LocalId].m_Target) : xiiGraphVertexHandle();
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
    const auto& edge = m_pGraph->m_pStorage->m_Edges[m_EdgeId];
    m_EdgeId         = m_bIncoming ? edge.m_NextIncoming : edge.m_NextOutgoing;
  }
}

xiiGraphVertexHandle xiiGraph::MakeVertexHandle(LocalId id) const
{
  return !m_pStorage || id.IsInvalidated() ? xiiGraphVertexHandle() : xiiGraphVertexHandle(xiiGraphElementId{id, m_pStorage->m_uiGraphId});
}

xiiGraphEdgeHandle xiiGraph::MakeEdgeHandle(LocalId id) const
{
  return !m_pStorage || id.IsInvalidated() ? xiiGraphEdgeHandle() : xiiGraphEdgeHandle(xiiGraphElementId{id, m_pStorage->m_uiGraphId});
}

bool xiiGraph::IsValid(xiiGraphVertexHandle hVertex) const
{
  const auto id = hVertex.GetInternalID();
  return m_pStorage && m_pStorage->m_uiGraphId != 0 && id.m_uiGraphId == m_pStorage->m_uiGraphId && m_pStorage->m_Vertices.Contains(id.m_LocalId) && m_pStorage->m_Vertices[id.m_LocalId].m_uiDenseIndex != xiiInvalidIndex;
}

bool xiiGraph::IsValid(xiiGraphEdgeHandle hEdge) const
{
  const auto id = hEdge.GetInternalID();
  return m_pStorage && m_pStorage->m_uiGraphId != 0 && id.m_uiGraphId == m_pStorage->m_uiGraphId && m_pStorage->m_Edges.Contains(id.m_LocalId) && m_pStorage->m_Edges[id.m_LocalId].m_uiDenseIndex != xiiInvalidIndex;
}
