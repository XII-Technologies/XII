/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

xiiResult xiiGraphAlgorithms::BreadthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  if (!graph.IsValid(hStart) || !visitor.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  const auto vertices    = graph.GetVertices();
  const auto start       = graph.GetVertexIndex(hStart);
  scratch.m_State[start] = 1;
  scratch.m_Work.PushBack(start);
  for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
  {
    const auto vertex = vertices[scratch.m_Work[head]];
    if (!visitor(vertex))
      break;
    for (auto it = graph.GetOutgoingEdges(vertex); it.IsValid(); ++it)
    {
      if (filter.IsValid() && !filter(it.GetHandle()))
        continue;
      const auto next = graph.GetVertexIndex(graph.GetTarget(it.GetHandle()));
      if (scratch.m_State[next] == 0)
      {
        scratch.m_State[next] = 1;
        scratch.m_Work.PushBack(next);
      }
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::DepthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  if (!graph.IsValid(hStart) || !visitor.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  WalkDepthFirst(graph, hStart, visitor, scratch, filter);
  return XII_SUCCESS;
}

void xiiGraphAlgorithms::WalkDepthFirst(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace& scratch, xiiGraphEdgeFilter filter)
{
  const auto vertices    = graph.GetVertices();
  const auto start       = graph.GetVertexIndex(hStart);
  scratch.m_State[start] = 1;
  if (!visitor(hStart))
    return;
  scratch.m_Frames.PushBack({start, graph.GetOutgoingEdges(hStart)});
  while (!scratch.m_Frames.IsEmpty())
  {
    auto& frame = scratch.m_Frames.PeekBack();
    if (!frame.m_Edges.IsValid())
    {
      scratch.m_Order.PushBack(frame.m_uiVertex);
      scratch.m_Frames.PopBack();
      continue;
    }
    const auto edge = frame.m_Edges.GetHandle();
    frame.m_Edges.Next();
    if (filter.IsValid() && !filter(edge))
      continue;
    const auto next = graph.GetVertexIndex(graph.GetTarget(edge));
    if (scratch.m_State[next] == 0)
    {
      scratch.m_State[next] = 1;
      if (!visitor(vertices[next]))
        break;
      scratch.m_Frames.PushBack({next, graph.GetOutgoingEdges(vertices[next])});
    }
  }
  // Early visitor termination must not retain cursors referencing the graph in reusable scratch storage.
  scratch.m_Frames.Clear();
}

bool xiiGraphAlgorithms::IsReachable(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  if (!graph.IsValid(hSource) || !graph.IsValid(hTarget))
    return false;
  struct
  {
    xiiGraphVertexHandle m_Target;
    bool                 m_bFound = false;
  } query{hTarget};
  // One pointer capture fits Foundation's delegate inline storage, including its lambda wrapper.
  BreadthFirstTraversal(graph, hSource, [&query](xiiGraphVertexHandle vertex) { query.m_bFound = vertex == query.m_Target; return !query.m_bFound; }, pWorkspace, filter).IgnoreResult();
  return query.m_bFound;
}
