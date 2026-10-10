/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

xiiResult xiiGraphAlgorithms::TopologicalSort(const xiiGraph& graph, xiiDynamicArray<xiiGraphVertexHandle>& out_order, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_order.Clear();
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  XII_SUCCEED_OR_RETURN(SortTopologically(graph, scratch, filter));
  const auto vertices = graph.GetVertices();
  out_order.Reserve(vertices.GetCount());
  for (auto index : scratch.m_Work)
    out_order.PushBack(vertices[index]);
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::SortTopologically(const xiiGraph& graph, xiiGraphAlgorithmWorkspace& scratch, xiiGraphEdgeFilter filter)
{
  scratch.Reset(graph.GetVertexCount());
  const auto vertices = graph.GetVertices();
  for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
  {
    scratch.m_State[i] = 0;
    for (auto it = graph.GetIncomingEdges(vertices[i]); it.IsValid(); ++it)
      if (!filter.IsValid() || filter(it.GetHandle()))
        ++scratch.m_State[i];
    if (scratch.m_State[i] == 0)
      scratch.m_Work.PushBack(i);
  }
  for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
  {
    const auto vertex = vertices[scratch.m_Work[head]];
    for (auto it = graph.GetOutgoingEdges(vertex); it.IsValid(); ++it)
    {
      if (filter.IsValid() && !filter(it.GetHandle()))
        continue;
      const auto next = graph.GetVertexIndex(graph.GetTarget(it.GetHandle()));
      if (--scratch.m_State[next] == 0)
        scratch.m_Work.PushBack(next);
    }
  }
  return scratch.m_Work.GetCount() == vertices.GetCount() ? XII_SUCCESS : XII_FAILURE;
}

bool xiiGraphAlgorithms::HasCycle(const xiiGraph& graph, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  return SortTopologically(graph, scratch, filter).Failed();
}
