/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

xiiResult xiiGraphAlgorithms::BuildPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace& scratch, xiiGraphPath& out_path)
{
  auto current = hTarget;
  while (current != hSource)
  {
    const auto edge = scratch.m_PreviousEdges[graph.GetVertexIndex(current)];
    if (edge.IsInvalidated())
    {
      out_path.Clear();
      return XII_FAILURE;
    }
    out_path.m_Vertices.PushBack(current);
    out_path.m_Edges.PushBack(edge);
    current = graph.GetSource(edge);
  }
  out_path.m_Vertices.PushBack(hSource);
  for (xiiUInt32 i = 0; i < out_path.m_Vertices.GetCount() / 2; ++i)
    xiiMath::Swap(out_path.m_Vertices[i], out_path.m_Vertices[out_path.m_Vertices.GetCount() - 1 - i]);
  for (xiiUInt32 i = 0; i < out_path.m_Edges.GetCount() / 2; ++i)
    xiiMath::Swap(out_path.m_Edges[i], out_path.m_Edges[out_path.m_Edges.GetCount() - 1 - i]);
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::FindShortestPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphPath& out_path, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_path.Clear();
  if (!graph.IsValid(hSource) || !graph.IsValid(hTarget))
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  scratch.m_PreviousEdges.SetCount(graph.GetVertexCount());
  for (auto& edge : scratch.m_PreviousEdges)
    edge.Invalidate();
  const auto vertices = graph.GetVertices();
  const auto source   = graph.GetVertexIndex(hSource);
  const auto target   = graph.GetVertexIndex(hTarget);
  scratch.m_Work.PushBack(source);
  scratch.m_State[source] = 1;
  for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount() && scratch.m_State[target] == 0; ++head)
  {
    for (auto it = graph.GetOutgoingEdges(vertices[scratch.m_Work[head]]); it.IsValid(); ++it)
    {
      const auto edge = it.GetHandle();
      if (filter.IsValid() && !filter(edge))
        continue;
      const auto next = graph.GetVertexIndex(graph.GetTarget(edge));
      if (scratch.m_State[next] == 0)
      {
        scratch.m_State[next]         = 1;
        scratch.m_PreviousEdges[next] = edge;
        scratch.m_Work.PushBack(next);
      }
    }
  }
  if (scratch.m_State[target] == 0)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(BuildPath(graph, hSource, hTarget, scratch, out_path));
  out_path.m_fCost = out_path.m_Edges.GetCount();
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::FindLowestCostPath(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeWeight weight, xiiGraphPath& out_path, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_path.Clear();
  if (!graph.IsValid(hSource) || !graph.IsValid(hTarget) || !weight.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  scratch.m_PreviousEdges.SetCount(graph.GetVertexCount());
  scratch.m_Distances.SetCount(graph.GetVertexCount());
  for (xiiUInt32 i = 0; i < graph.GetVertexCount(); ++i)
  {
    scratch.m_State[i] = xiiInvalidIndex;
    scratch.m_PreviousEdges[i].Invalidate();
    scratch.m_Distances[i] = xiiMath::Infinity<double>();
  }
  const auto vertices = graph.GetVertices();
  const auto source   = graph.GetVertexIndex(hSource);
  const auto target   = graph.GetVertexIndex(hTarget);
  auto&      heap     = scratch.m_Work;
  const auto swapHeap = [&](xiiUInt32 a, xiiUInt32 b) {
    xiiMath::Swap(heap[a], heap[b]);
    scratch.m_State[heap[a]] = a;
    scratch.m_State[heap[b]] = b;
  };
  const auto less = [&](xiiUInt32 a, xiiUInt32 b) {
    return scratch.m_Distances[a] < scratch.m_Distances[b] || (scratch.m_Distances[a] == scratch.m_Distances[b] && a < b);
  };
  scratch.m_Distances[source] = 0;
  heap.PushBack(source);
  scratch.m_State[source]     = 0;
  constexpr xiiUInt32 settled = xiiInvalidIndex - 1;
  while (!heap.IsEmpty())
  {
    const auto current = heap[0];
    swapHeap(0, heap.GetCount() - 1);
    heap.PopBack();
    scratch.m_State[current] = settled;
    for (xiiUInt32 position = 0; position < heap.GetCount();)
    {
      if (position >= heap.GetCount() / 2)
        break;
      const auto left  = position * 2 + 1;
      const auto right = left + 1;
      const auto best  = right < heap.GetCount() && less(heap[right], heap[left]) ? right : left;
      if (!less(heap[best], heap[position]))
        break;
      swapHeap(position, best);
      position = best;
    }
    if (current == target)
    {
      XII_SUCCEED_OR_RETURN(BuildPath(graph, hSource, hTarget, scratch, out_path));
      out_path.m_fCost = scratch.m_Distances[target];
      return XII_SUCCESS;
    }
    for (auto it = graph.GetOutgoingEdges(vertices[current]); it.IsValid(); ++it)
    {
      const auto edge = it.GetHandle();
      if (filter.IsValid() && !filter(edge))
        continue;
      const double cost = weight(edge);
      if (!xiiMath::IsFinite(cost) || cost < 0)
        return XII_FAILURE;
      const auto next = graph.GetVertexIndex(graph.GetTarget(edge));
      if (scratch.m_State[next] == settled)
        continue;
      const auto distance = scratch.m_Distances[current] + cost;
      if (!xiiMath::IsFinite(distance))
        return XII_FAILURE;
      if (distance >= scratch.m_Distances[next])
        continue;
      scratch.m_Distances[next]     = distance;
      scratch.m_PreviousEdges[next] = edge;
      if (scratch.m_State[next] == xiiInvalidIndex)
      {
        scratch.m_State[next] = heap.GetCount();
        heap.PushBack(next);
      }
      for (auto position = scratch.m_State[next]; position > 0;)
      {
        const auto parent = (position - 1) / 2;
        if (!less(heap[position], heap[parent]))
          break;
        swapHeap(position, parent);
        position = parent;
      }
    }
  }
  return XII_FAILURE;
}
