/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Graph/GraphAlgorithms.h>

namespace
{
  void CheckTopologicalOrder(const xiiGraph& graph, xiiArrayPtr<const xiiGraphVertexHandle> order)
  {
    XII_TEST_INT(order.GetCount(), graph.GetVertexCount());
    xiiDynamicArray<xiiUInt32> positions;
    positions.SetCount(graph.GetVertexCount());
    for (auto& position : positions)
      position = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < order.GetCount(); ++i)
    {
      const auto index = graph.GetVertexIndex(order[i]);
      XII_TEST_BOOL(index != xiiInvalidIndex);
      if (index == xiiInvalidIndex)
        return;
      XII_TEST_INT(positions[index], xiiInvalidIndex);
      positions[index] = i;
    }
    for (const auto edge : graph.GetEdges())
      XII_TEST_BOOL(positions[graph.GetVertexIndex(graph.GetSource(edge))] < positions[graph.GetVertexIndex(graph.GetTarget(edge))]);
  }

  xiiUInt32 FindComponent(const xiiGraphComponents& components, xiiGraphVertexHandle vertex)
  {
    for (xiiUInt32 i = 0; i < components.GetCount(); ++i)
    {
      for (const auto member : components.GetComponent(i))
      {
        if (member == vertex)
          return i;
      }
    }
    return xiiInvalidIndex;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Graph, Algorithms)
{
  xiiGraphAlgorithmWorkspace workspace;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty graphs and invalid traversal")
  {
    xiiGraph                              graph;
    xiiDynamicArray<xiiGraphVertexHandle> order;
    xiiGraphComponents                    components;
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
    XII_TEST_BOOL(order.IsEmpty());
    XII_TEST_BOOL(!xiiGraphAlgorithms::HasCycle(graph, &workspace));
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace);
    XII_TEST_INT(components.GetCount(), 0);
    xiiGraphAlgorithms::WeaklyConnectedComponents(graph, components, &workspace);
    XII_TEST_INT(components.GetCount(), 0);
    XII_TEST_INT(components.m_Offsets.GetCount(), 1);
    bool       called  = false;
    const auto visitor = [&](xiiGraphVertexHandle) {
      called = true;
      return true;
    };
    XII_TEST_BOOL(xiiGraphAlgorithms::BreadthFirstTraversal(graph, {}, visitor, &workspace).Failed());
    XII_TEST_BOOL(xiiGraphAlgorithms::DepthFirstTraversal(graph, {}, visitor, &workspace).Failed());
    XII_TEST_BOOL(!called);
    const auto vertex = graph.AddVertex();
    XII_TEST_BOOL(xiiGraphAlgorithms::BreadthFirstTraversal(graph, vertex, {}, &workspace).Failed());
    XII_TEST_BOOL(xiiGraphAlgorithms::DepthFirstTraversal(graph, vertex, {}, &workspace).Failed());
    XII_TEST_BOOL(xiiGraphAlgorithms::IsReachable(graph, vertex, vertex, &workspace));
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, vertex, {}, &workspace));
    xiiGraph   other;
    const auto foreign = other.AddVertex();
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, vertex, foreign, &workspace));
    XII_TEST_BOOL(xiiGraphAlgorithms::DepthFirstTraversal(graph, foreign, visitor, &workspace).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Traversal order, parallel edges, loops, reachability and early stop")
  {
    xiiGraph   graph;
    const auto a        = graph.AddVertex();
    const auto b        = graph.AddVertex();
    const auto c        = graph.AddVertex();
    const auto d        = graph.AddVertex();
    const auto isolated = graph.AddVertex();
    graph.AddEdge(a, b);
    graph.AddEdge(a, c);
    graph.AddEdge(c, d);
    graph.AddEdge(b, d);
    graph.AddEdge(a, c);
    graph.AddEdge(d, a);
    graph.AddEdge(c, c);
    xiiDynamicArray<xiiGraphVertexHandle> visited;
    const auto                            visitor = [&](xiiGraphVertexHandle v) {
      visited.PushBack(v);
      return true;
    };
    XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, a, visitor, &workspace));
    const xiiGraphVertexHandle bfs[] = {a, c, b, d};
    XII_TEST_BOOL(visited.GetArrayPtr() == xiiMakeArrayPtr(bfs));
    visited.Clear();
    XII_TEST_RESULT(xiiGraphAlgorithms::DepthFirstTraversal(graph, a, visitor, &workspace));
    const xiiGraphVertexHandle dfs[] = {a, c, d, b};
    XII_TEST_BOOL(visited.GetArrayPtr() == xiiMakeArrayPtr(dfs));
    XII_TEST_BOOL(xiiGraphAlgorithms::IsReachable(graph, b, c, &workspace));
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, a, isolated, &workspace));
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, isolated, a, &workspace));
    visited.Clear();
    XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, isolated, visitor));
    XII_TEST_INT(visited.GetCount(), 1);
    xiiUInt32  count = 0;
    const auto stop  = [&](xiiGraphVertexHandle) {
      return ++count < 2;
    };
    XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, a, stop, &workspace));
    XII_TEST_INT(count, 2);
    count = 0;
    XII_TEST_RESULT(xiiGraphAlgorithms::DepthFirstTraversal(graph, a, stop, &workspace));
    XII_TEST_INT(count, 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Topological ordering and cycle failure")
  {
    xiiGraph   graph;
    const auto a = graph.AddVertex();
    const auto b = graph.AddVertex();
    const auto c = graph.AddVertex();
    const auto d = graph.AddVertex();
    graph.AddVertex();
    graph.AddEdge(a, c);
    graph.AddEdge(a, c);
    graph.AddEdge(b, c);
    graph.AddEdge(c, d);
    xiiDynamicArray<xiiGraphVertexHandle> order;
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
    CheckTopologicalOrder(graph, order);
    const auto firstOrder = order;
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
    XII_TEST_BOOL(order == firstOrder);
    XII_TEST_BOOL(!xiiGraphAlgorithms::HasCycle(graph, &workspace));
    const auto cycle = graph.AddEdge(d, a);
    XII_TEST_BOOL(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace).Failed());
    XII_TEST_BOOL(order.IsEmpty());
    XII_TEST_BOOL(xiiGraphAlgorithms::HasCycle(graph, &workspace));
    graph.RemoveEdge(cycle);
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
    graph.AddEdge(b, b);
    XII_TEST_BOOL(xiiGraphAlgorithms::HasCycle(graph, &workspace));
    XII_TEST_BOOL(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace).Failed());
    XII_TEST_BOOL(order.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Strong and weak components distinguish directed connectivity")
  {
    xiiGraph   graph;
    const auto a        = graph.AddVertex();
    const auto b        = graph.AddVertex();
    const auto c        = graph.AddVertex();
    const auto d        = graph.AddVertex();
    const auto e        = graph.AddVertex();
    const auto isolated = graph.AddVertex();
    graph.AddEdge(a, b);
    graph.AddEdge(a, b);
    graph.AddEdge(b, a);
    graph.AddEdge(b, c);
    graph.AddEdge(c, d);
    graph.AddEdge(d, c);
    graph.AddEdge(d, e);
    graph.AddEdge(e, e);
    xiiGraphComponents strong;
    xiiGraphComponents weak;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, strong, &workspace);
    XII_TEST_INT(strong.GetCount(), 4);
    XII_TEST_INT(strong.m_Vertices.GetCount(), 6);
    XII_TEST_INT(FindComponent(strong, a), FindComponent(strong, b));
    XII_TEST_INT(FindComponent(strong, c), FindComponent(strong, d));
    XII_TEST_BOOL(FindComponent(strong, a) != FindComponent(strong, c));
    XII_TEST_BOOL(FindComponent(strong, e) != FindComponent(strong, d));
    XII_TEST_INT(strong.GetComponent(FindComponent(strong, e)).GetCount(), 1);
    XII_TEST_INT(strong.GetComponent(FindComponent(strong, isolated)).GetCount(), 1);
    xiiGraphAlgorithms::WeaklyConnectedComponents(graph, weak, &workspace);
    XII_TEST_INT(weak.GetCount(), 2);
    XII_TEST_INT(weak.GetComponent(FindComponent(weak, a)).GetCount(), 5);
    XII_TEST_INT(weak.GetComponent(FindComponent(weak, isolated)).GetCount(), 1);
    const auto firstVertices = strong.m_Vertices;
    const auto firstOffsets  = strong.m_Offsets;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, strong, &workspace);
    XII_TEST_BOOL(strong.m_Vertices == firstVertices && strong.m_Offsets == firstOffsets);
    graph.Clear();
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, strong, &workspace);
    XII_TEST_INT(strong.GetCount(), 0);
    XII_TEST_BOOL(strong.m_Vertices.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Long chains do not consume the call stack")
  {
    constexpr xiiUInt32 count = 30000;
    xiiGraph            graph;
    XII_TEST_RESULT(graph.Reserve(count, count));
    auto first = graph.AddVertex();
    auto last  = first;
    for (xiiUInt32 i = 1; i < count; ++i)
    {
      const auto next = graph.AddVertex();
      graph.AddEdge(last, next);
      last = next;
    }
    xiiUInt32 visited = 0;
    XII_TEST_RESULT(xiiGraphAlgorithms::DepthFirstTraversal(graph, first, [&](xiiGraphVertexHandle) { ++visited; return true; }, &workspace));
    XII_TEST_INT(visited, count);
    xiiDynamicArray<xiiGraphVertexHandle> order;
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
    CheckTopologicalOrder(graph, order);
    xiiGraphComponents components;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace);
    XII_TEST_INT(components.GetCount(), count);
    graph.AddEdge(last, first);
    XII_TEST_BOOL(xiiGraphAlgorithms::HasCycle(graph, &workspace));
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace);
    XII_TEST_INT(components.GetCount(), 1);
    XII_TEST_INT(components.GetComponent(0).GetCount(), count);
  }
}
