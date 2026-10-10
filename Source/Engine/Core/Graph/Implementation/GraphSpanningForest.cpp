/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

xiiResult xiiGraphAlgorithms::MinimumSpanningForest(const xiiGraph& graph, xiiGraphSpanningForest& out_forest, xiiGraphEdgeWeight weight, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_forest.Clear();
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  scratch.m_Order.SetCount(graph.GetVertexCount());
  scratch.m_WeightedEdges.Clear();
  for (xiiUInt32 i = 0; i < graph.GetVertexCount(); ++i)
  {
    scratch.m_State[i] = i;
    scratch.m_Order[i] = 0;
  }
  const auto edges = graph.GetEdges();
  for (xiiUInt32 i = 0; i < edges.GetCount(); ++i)
  {
    if ((filter.IsValid() && !filter(edges[i])) || graph.GetSource(edges[i]) == graph.GetTarget(edges[i]))
      continue;
    const double cost = weight.IsValid() ? weight(edges[i]) : 1;
    if (!xiiMath::IsFinite(cost))
      return XII_FAILURE;
    scratch.m_WeightedEdges.PushBack({edges[i], cost, i});
  }
  scratch.m_WeightedEdges.Sort([](const auto& a, const auto& b) {
    return a.m_fWeight < b.m_fWeight || (a.m_fWeight == b.m_fWeight && a.m_uiOrder < b.m_uiOrder);
  });
  const auto root = [&](xiiUInt32 vertex) {
    while (scratch.m_State[vertex] != vertex)
    {
      scratch.m_State[vertex] = scratch.m_State[scratch.m_State[vertex]];
      vertex                  = scratch.m_State[vertex];
    }
    return vertex;
  };
  xiiUInt32 componentCount = graph.GetVertexCount();
  double    total          = 0;
  for (const auto& weighted : scratch.m_WeightedEdges)
  {
    auto a = root(graph.GetVertexIndex(graph.GetSource(weighted.m_hEdge)));
    auto b = root(graph.GetVertexIndex(graph.GetTarget(weighted.m_hEdge)));
    if (a == b)
      continue;
    if (scratch.m_Order[a] < scratch.m_Order[b])
      xiiMath::Swap(a, b);
    scratch.m_State[b] = a;
    if (scratch.m_Order[a] == scratch.m_Order[b])
      ++scratch.m_Order[a];
    total += weighted.m_fWeight;
    if (!xiiMath::IsFinite(total))
    {
      out_forest.Clear();
      return XII_FAILURE;
    }
    out_forest.m_Edges.PushBack(weighted.m_hEdge);
    --componentCount;
  }
  out_forest.m_fWeight          = total;
  out_forest.m_uiComponentCount = componentCount;
  return XII_SUCCESS;
}
