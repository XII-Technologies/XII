/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/Implementation/GraphStorage.h>

xiiGraphVertexHandle xiiGraph::AddVertex()
{
  xiiGraphVertexHandle result;
  TryAddVertex(result).IgnoreResult();
  return result;
}
xiiGraphEdgeHandle xiiGraph::AddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget)
{
  xiiGraphEdgeHandle result;
  TryAddEdge(hSource, hTarget, result).IgnoreResult();
  return result;
}
bool xiiGraph::RemoveVertex(xiiGraphVertexHandle hVertex) { return TryRemoveVertex(hVertex).Succeeded(); }
bool xiiGraph::RemoveEdge(xiiGraphEdgeHandle hEdge) { return TryRemoveEdge(hEdge).Succeeded(); }
void xiiGraph::Clear() { TryClear().IgnoreResult(); }

xiiResult xiiGraph::TryAddVertex(xiiGraphVertexHandle& out_hVertex)
{
  out_hVertex.Invalidate();
  if (m_bDispatching || m_uiVertexLimit == 0 || (m_pStorage && m_pStorage->m_Vertices.GetCount() >= m_uiVertexLimit))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(TryInitialize());
  Storage::Vertex vertex;
  vertex.m_uiDenseIndex = GetVertexCount();
  out_hVertex           = MakeVertexHandle(m_pStorage->m_Vertices.Insert(vertex));
  m_pStorage->m_VertexHandles.PushBack(out_hVertex);
  Broadcast({xiiGraphEvent::Type::VertexAdded, nullptr, out_hVertex});
  return XII_SUCCESS;
}

xiiResult xiiGraph::TryAddEdge(xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeHandle& out_hEdge)
{
  out_hEdge.Invalidate();
  XII_SUCCEED_OR_RETURN(ValidateConnection(hSource, hTarget));
  if (m_pStorage->m_Edges.GetCount() >= m_uiEdgeLimit)
    return XII_FAILURE;
  Storage::Edge edge;
  edge.m_Source       = hSource.GetInternalID().m_LocalId;
  edge.m_Target       = hTarget.GetInternalID().m_LocalId;
  edge.m_uiDenseIndex = GetEdgeCount();
  const auto id       = m_pStorage->m_Edges.Insert(edge);
  LinkEdge(id);
  out_hEdge = MakeEdgeHandle(id);
  m_pStorage->m_EdgeHandles.PushBack(out_hEdge);
  Broadcast({xiiGraphEvent::Type::EdgeAdded, nullptr, {}, out_hEdge, hSource, hTarget});
  return XII_SUCCESS;
}

void xiiGraph::LinkEdge(LocalId id)
{
  auto& edge   = m_pStorage->m_Edges[id];
  auto& source = m_pStorage->m_Vertices[edge.m_Source];
  auto& target = m_pStorage->m_Vertices[edge.m_Target];
  edge.m_PreviousOutgoing.Invalidate();
  edge.m_PreviousIncoming.Invalidate();
  edge.m_NextOutgoing = source.m_FirstOutgoing;
  edge.m_NextIncoming = target.m_FirstIncoming;
  if (!edge.m_NextOutgoing.IsInvalidated())
    m_pStorage->m_Edges[edge.m_NextOutgoing].m_PreviousOutgoing = id;
  if (!edge.m_NextIncoming.IsInvalidated())
    m_pStorage->m_Edges[edge.m_NextIncoming].m_PreviousIncoming = id;
  source.m_FirstOutgoing = target.m_FirstIncoming = id;
  ++source.m_uiOutDegree;
  ++target.m_uiInDegree;
}

void xiiGraph::UnlinkEdge(LocalId id)
{
  const auto& edge   = m_pStorage->m_Edges[id];
  auto&       source = m_pStorage->m_Vertices[edge.m_Source];
  auto&       target = m_pStorage->m_Vertices[edge.m_Target];
  if (edge.m_PreviousOutgoing.IsInvalidated())
    source.m_FirstOutgoing = edge.m_NextOutgoing;
  else
    m_pStorage->m_Edges[edge.m_PreviousOutgoing].m_NextOutgoing = edge.m_NextOutgoing;
  if (!edge.m_NextOutgoing.IsInvalidated())
    m_pStorage->m_Edges[edge.m_NextOutgoing].m_PreviousOutgoing = edge.m_PreviousOutgoing;
  if (edge.m_PreviousIncoming.IsInvalidated())
    target.m_FirstIncoming = edge.m_NextIncoming;
  else
    m_pStorage->m_Edges[edge.m_PreviousIncoming].m_NextIncoming = edge.m_NextIncoming;
  if (!edge.m_NextIncoming.IsInvalidated())
    m_pStorage->m_Edges[edge.m_NextIncoming].m_PreviousIncoming = edge.m_PreviousIncoming;
  --source.m_uiOutDegree;
  --target.m_uiInDegree;
}

xiiResult xiiGraph::ReconnectEdge(xiiGraphEdgeHandle hEdge, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget)
{
  if (!IsValid(hEdge))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(ValidateConnection(hSource, hTarget, hEdge));
  const auto id = hEdge.GetInternalID().m_LocalId;
  UnlinkEdge(id);
  auto& edge    = m_pStorage->m_Edges[id];
  edge.m_Source = hSource.GetInternalID().m_LocalId;
  edge.m_Target = hTarget.GetInternalID().m_LocalId;
  LinkEdge(id);
  Broadcast({xiiGraphEvent::Type::EdgeReconnected, nullptr, {}, hEdge, hSource, hTarget});
  return XII_SUCCESS;
}

xiiResult xiiGraph::TryRemoveEdge(xiiGraphEdgeHandle hEdge)
{
  if (m_bDispatching || !IsValid(hEdge))
    return XII_FAILURE;
  const auto    id   = hEdge.GetInternalID().m_LocalId;
  const auto    edge = m_pStorage->m_Edges[id];
  xiiGraphEvent event{xiiGraphEvent::Type::BeforeEdgeRemoved, nullptr, {}, hEdge, MakeVertexHandle(edge.m_Source), MakeVertexHandle(edge.m_Target)};
  Broadcast(event);
  UnlinkEdge(id);
  const auto last                                                    = m_pStorage->m_EdgeHandles.PeekBack();
  m_pStorage->m_EdgeHandles[edge.m_uiDenseIndex]                     = last;
  m_pStorage->m_Edges[last.GetInternalID().m_LocalId].m_uiDenseIndex = edge.m_uiDenseIndex;
  m_pStorage->m_EdgeHandles.PopBack();
  if (id.m_Generation == 0x7FFFFFFFULL)
    m_pStorage->m_Edges[id].m_uiDenseIndex = xiiInvalidIndex;
  else
    m_pStorage->m_Edges.Remove(id);
  event.m_Type = xiiGraphEvent::Type::EdgeRemoved;
  Broadcast(event);
  return XII_SUCCESS;
}

xiiResult xiiGraph::TryRemoveVertex(xiiGraphVertexHandle hVertex)
{
  if (m_bDispatching || !IsValid(hVertex))
    return XII_FAILURE;
  Broadcast({xiiGraphEvent::Type::BeforeVertexRemoved, nullptr, hVertex});
  const auto id     = hVertex.GetInternalID().m_LocalId;
  auto&      vertex = m_pStorage->m_Vertices[id];
  while (!vertex.m_FirstOutgoing.IsInvalidated())
    XII_SUCCEED_OR_RETURN(TryRemoveEdge(MakeEdgeHandle(vertex.m_FirstOutgoing)));
  while (!vertex.m_FirstIncoming.IsInvalidated())
    XII_SUCCEED_OR_RETURN(TryRemoveEdge(MakeEdgeHandle(vertex.m_FirstIncoming)));
  const auto last                                                       = m_pStorage->m_VertexHandles.PeekBack();
  m_pStorage->m_VertexHandles[vertex.m_uiDenseIndex]                    = last;
  m_pStorage->m_Vertices[last.GetInternalID().m_LocalId].m_uiDenseIndex = vertex.m_uiDenseIndex;
  m_pStorage->m_VertexHandles.PopBack();
  if (id.m_Generation == 0x7FFFFFFFULL)
    vertex.m_uiDenseIndex = xiiInvalidIndex;
  else
    m_pStorage->m_Vertices.Remove(id);
  Broadcast({xiiGraphEvent::Type::VertexRemoved, nullptr, hVertex});
  return XII_SUCCESS;
}

xiiResult xiiGraph::TryClear()
{
  if (m_bDispatching)
    return XII_FAILURE;
  while (GetVertexCount() > 0)
    XII_SUCCEED_OR_RETURN(TryRemoveVertex(m_pStorage->m_VertexHandles.PeekBack()));
  return XII_SUCCESS;
}
