/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphAlgorithms.h>
#include <CoreTest/CoreTestPCH.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/CommonAllocators.h>

XII_CREATE_SIMPLE_TEST(Graph, Optimization)
{
  xiiGraphAlgorithmWorkspace workspace;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Optimization scratch and output allocation reuse")
  {
    xiiAllocatorWithPolicy<xiiAllocationPolicyProxy, xiiAllocatorTrackingMode::AllocationStats> allocator("GraphOptimizationTest", xiiFoundation::GetDefaultAllocator());
    xiiGraph                                                                                    graph(&allocator);
    auto                                                                                        a = graph.AddVertex();
    const auto                                                                                  b = graph.AddVertex();
    const auto                                                                                  c = graph.AddVertex();
    graph.AddEdge(a, b);
    graph.AddEdge(a, b);
    graph.AddEdge(b, c);
    graph.AddEdge(a, c);
    graph.AddEdge(b, a);
    xiiGraphAlgorithmWorkspace scratch(&allocator);
    xiiGraphPath               path(&allocator);
    xiiGraphFlow               flow(&allocator);
    xiiGraphSpanningForest     forest(&allocator);
    const auto                 run = [&]() {
      const auto weight = [](xiiGraphEdgeHandle) {
        return 3.0;
      };
      XII_TEST_RESULT(xiiGraphAlgorithms::FindShortestPath(graph, a, c, path, &scratch));
      XII_TEST_RESULT(xiiGraphAlgorithms::FindLowestCostPath(graph, a, c, weight, path, &scratch));
      XII_TEST_RESULT(xiiGraphAlgorithms::MaximumFlow(graph, a, c, weight, flow, &scratch));
      XII_TEST_RESULT(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, weight, &scratch));
    };
    run();
    const auto allocations        = allocator.GetStats().m_uiAllocationCount;
    const auto defaultAllocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
    for (xiiUInt32 i = 0; i < 10; ++i)
      run();
    XII_TEST_INT(allocator.GetStats().m_uiAllocationCount, allocations);
    XII_TEST_INT(xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount, defaultAllocations);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Filtered categories affect every traversal and connectivity algorithm")
  {
    xiiGraph   graph;
    const auto a  = graph.AddVertex();
    const auto b  = graph.AddVertex();
    const auto c  = graph.AddVertex();
    const auto d  = graph.AddVertex();
    const auto ab = graph.AddEdge(a, b);
    const auto bc = graph.AddEdge(b, c);
    graph.AddEdge(c, a);
    graph.AddEdge(a, d);
    xiiHashTable<xiiGraphEdgeHandle, bool> enabled;
    enabled.Insert(ab, true);
    enabled.Insert(bc, true);
    const auto filter = [&](xiiGraphEdgeHandle edge) {
      return enabled.Contains(edge);
    };
    XII_TEST_BOOL(xiiGraphAlgorithms::HasCycle(graph, &workspace));
    XII_TEST_BOOL(!xiiGraphAlgorithms::HasCycle(graph, &workspace, filter));
    XII_TEST_BOOL(xiiGraphAlgorithms::IsReachable(graph, a, c, &workspace, filter));
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, a, d, &workspace, filter));
    XII_TEST_BOOL(!xiiGraphAlgorithms::IsReachable(graph, c, a, &workspace, filter));
    xiiDynamicArray<xiiGraphVertexHandle> visited;
    const auto                            visitor = [&](xiiGraphVertexHandle vertex) {
      visited.PushBack(vertex);
      return true;
    };
    const xiiGraphVertexHandle expected[] = {a, b, c};
    XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, a, visitor, &workspace, filter));
    XII_TEST_BOOL(visited.GetArrayPtr() == xiiMakeArrayPtr(expected));
    visited.Clear();
    XII_TEST_RESULT(xiiGraphAlgorithms::DepthFirstTraversal(graph, a, visitor, &workspace, filter));
    XII_TEST_BOOL(visited.GetArrayPtr() == xiiMakeArrayPtr(expected));
    XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, visited, &workspace, filter));
    XII_TEST_BOOL(visited.IndexOf(a) < visited.IndexOf(b) && visited.IndexOf(b) < visited.IndexOf(c));
    xiiGraphComponents components;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace, filter);
    XII_TEST_INT(components.GetCount(), 4);
    xiiGraphAlgorithms::WeaklyConnectedComponents(graph, components, &workspace, filter);
    XII_TEST_INT(components.GetCount(), 2);
    xiiGraphPath path;
    XII_TEST_RESULT(xiiGraphAlgorithms::FindShortestPath(graph, a, c, path, &workspace, filter));
    XII_TEST_BOOL(path.m_Vertices.GetArrayPtr() == xiiMakeArrayPtr(expected));
    XII_TEST_BOOL(xiiGraphAlgorithms::FindShortestPath(graph, a, d, path, &workspace, filter).Failed());
    XII_TEST_BOOL(path.m_Vertices.IsEmpty() && path.m_Edges.IsEmpty());
    XII_TEST_RESULT(xiiGraphAlgorithms::FindLowestCostPath(graph, a, c, [](xiiGraphEdgeHandle) { return 2.0; }, path, &workspace, filter));
    XII_TEST_FLOAT(path.m_fCost, 4, 0.0001);
    xiiGraphFlow flow;
    XII_TEST_RESULT(xiiGraphAlgorithms::MaximumFlow(graph, a, d, [](xiiGraphEdgeHandle) { return 3.0; }, flow, &workspace, filter));
    XII_TEST_FLOAT(flow.m_fValue, 0, 0.0001);
    xiiGraphSpanningForest forest;
    XII_TEST_RESULT(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, {}, &workspace, filter));
    XII_TEST_INT(forest.m_uiComponentCount, 2);
    XII_TEST_INT(forest.m_Edges.GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Zero-length paths, invalid weights, empty/disconnected forests")
  {
    xiiGraph               graph;
    xiiGraphPath           path;
    xiiGraphFlow           flow;
    xiiGraphSpanningForest forest;
    XII_TEST_RESULT(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, {}, &workspace));
    XII_TEST_INT(forest.m_uiComponentCount, 0);
    XII_TEST_BOOL(xiiGraphAlgorithms::FindShortestPath(graph, {}, {}, path, &workspace).Failed());
    const auto a = graph.AddVertex();
    const auto b = graph.AddVertex();
    XII_TEST_RESULT(xiiGraphAlgorithms::FindShortestPath(graph, a, a, path, &workspace));
    XII_TEST_INT(path.m_Vertices.GetCount(), 1);
    XII_TEST_BOOL(path.m_Edges.IsEmpty());
    XII_TEST_RESULT(xiiGraphAlgorithms::FindLowestCostPath(graph, a, a, [](xiiGraphEdgeHandle) { return 0.0; }, path, &workspace));
    XII_TEST_BOOL(xiiGraphAlgorithms::FindLowestCostPath(graph, a, b, {}, path, &workspace).Failed());
    graph.AddEdge(a, b);
    const double badValues[] = {-1.0, xiiMath::Infinity<double>(), xiiMath::NaN<double>()};
    for (double value : badValues)
    {
      const auto weight = [value](xiiGraphEdgeHandle) {
        return value;
      };
      XII_TEST_BOOL(xiiGraphAlgorithms::FindLowestCostPath(graph, a, b, weight, path, &workspace).Failed());
      XII_TEST_BOOL(path.m_Vertices.IsEmpty());
      XII_TEST_BOOL(xiiGraphAlgorithms::MaximumFlow(graph, a, b, weight, flow, &workspace).Failed());
      XII_TEST_BOOL(flow.m_Edges.IsEmpty());
      if (value != -1.0)
        XII_TEST_BOOL(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, weight, &workspace).Failed());
    }
    XII_TEST_BOOL(xiiGraphAlgorithms::MaximumFlow(graph, a, a, [](xiiGraphEdgeHandle) { return 1.0; }, flow, &workspace).Failed());
    XII_TEST_RESULT(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, [](xiiGraphEdgeHandle) { return -2.0; }, &workspace));
    XII_TEST_FLOAT(forest.m_fWeight, -2, 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Random shortest paths versus Floyd-Warshall, flow versus all cuts, forest versus Prim")
  {
    constexpr xiiUInt32 count = 7;
    xiiRandom           random;
    random.Initialize(0x123456789);
    for (xiiUInt32 trial = 0; trial < 50; ++trial)
    {
      xiiGraph             graph;
      xiiGraphVertexHandle vertices[count];
      for (auto& vertex : vertices)
        vertex = graph.AddVertex();
      xiiHashTable<xiiGraphEdgeHandle, double> weights;
      double                                   distances[count][count];
      double                                   undirected[count][count];
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        for (xiiUInt32 j = 0; j < count; ++j)
        {
          distances[i][j]  = i == j ? 0 : 10000;
          undirected[i][j] = 10000;
        }
      }
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        for (xiiUInt32 j = 0; j < count; ++j)
        {
          if (random.UIntInRange(100) >= (trial % 3 == 0 ? 8U : 45U))
            continue;
          for (xiiUInt32 parallel = 0; parallel < 1 + trial % 2; ++parallel)
          {
            const auto   edge = graph.AddEdge(vertices[i], vertices[j]);
            const double cost = random.UIntInRange(10);
            weights.Insert(edge, cost);
            distances[i][j]  = xiiMath::Min(distances[i][j], cost);
            undirected[i][j] = undirected[j][i] = xiiMath::Min(undirected[i][j], cost);
          }
        }
      }
      const auto weight = [&](xiiGraphEdgeHandle edge) {
        return weights[edge];
      };
      for (xiiUInt32 k = 0; k < count; ++k)
        for (xiiUInt32 i = 0; i < count; ++i)
          for (xiiUInt32 j = 0; j < count; ++j)
            distances[i][j] = xiiMath::Min(distances[i][j], distances[i][k] + distances[k][j]);
      xiiGraphPath path;
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        for (xiiUInt32 j = 0; j < count; ++j)
        {
          const auto result = xiiGraphAlgorithms::FindLowestCostPath(graph, vertices[i], vertices[j], weight, path, &workspace);
          XII_TEST_BOOL(result.Succeeded() == (distances[i][j] < 10000));
          if (result.Failed())
            continue;
          XII_TEST_FLOAT(path.m_fCost, distances[i][j], 0.0001);
          XII_TEST_BOOL(path.m_Vertices[0] == vertices[i] && path.m_Vertices.PeekBack() == vertices[j]);
          XII_TEST_INT(path.m_Vertices.GetCount(), path.m_Edges.GetCount() + 1);
          double cost = 0;
          for (xiiUInt32 e = 0; e < path.m_Edges.GetCount(); ++e)
          {
            XII_TEST_BOOL(graph.GetSource(path.m_Edges[e]) == path.m_Vertices[e]);
            XII_TEST_BOOL(graph.GetTarget(path.m_Edges[e]) == path.m_Vertices[e + 1]);
            cost += weights[path.m_Edges[e]];
          }
          XII_TEST_FLOAT(cost, path.m_fCost, 0.0001);
        }
      }
      double minimumCut = 10000;
      for (xiiUInt32 cut = 0; cut < (1U << (count - 2)); ++cut)
      {
        const xiiUInt32 mask     = (cut << 1) | 1;
        double          capacity = 0;
        for (auto edge : graph.GetEdges())
        {
          const auto source = graph.GetVertexIndex(graph.GetSource(edge));
          const auto target = graph.GetVertexIndex(graph.GetTarget(edge));
          if ((mask & (1U << source)) != 0 && (mask & (1U << target)) == 0)
            capacity += weights[edge];
        }
        minimumCut = xiiMath::Min(minimumCut, capacity);
      }
      xiiGraphFlow flow;
      XII_TEST_RESULT(xiiGraphAlgorithms::MaximumFlow(graph, vertices[0], vertices[count - 1], weight, flow, &workspace));
      XII_TEST_FLOAT(flow.m_fValue, minimumCut, 0.0001);
      double balance[count] = {};
      for (xiiUInt32 e = 0; e < flow.m_Edges.GetCount(); ++e)
      {
        const auto edge = flow.m_Edges[e];
        XII_TEST_BOOL(flow.m_Flows[e] >= 0 && flow.m_Flows[e] <= weights[edge]);
        balance[graph.GetVertexIndex(graph.GetSource(edge))] -= flow.m_Flows[e];
        balance[graph.GetVertexIndex(graph.GetTarget(edge))] += flow.m_Flows[e];
      }
      XII_TEST_FLOAT(balance[0], -flow.m_fValue, 0.0001);
      XII_TEST_FLOAT(balance[count - 1], flow.m_fValue, 0.0001);
      for (xiiUInt32 i = 1; i < count - 1; ++i)
        XII_TEST_FLOAT(balance[i], 0, 0.0001);
      // Independent dense Prim reference, treating the directed input as undirected.
      bool      included[count] = {};
      xiiUInt32 componentCount  = 0;
      double    forestWeight    = 0;
      for (xiiUInt32 root = 0; root < count; ++root)
      {
        if (included[root])
          continue;
        included[root] = true;
        ++componentCount;
        while (true)
        {
          double    best = 10000;
          xiiUInt32 next = count;
          for (xiiUInt32 i = 0; i < count; ++i)
            for (xiiUInt32 j = 0; j < count; ++j)
              if (included[i] && !included[j] && undirected[i][j] < best)
              {
                best = undirected[i][j];
                next = j;
              }
          if (next == count)
            break;
          included[next] = true;
          forestWeight += best;
        }
      }
      xiiGraphSpanningForest forest;
      XII_TEST_RESULT(xiiGraphAlgorithms::MinimumSpanningForest(graph, forest, weight, &workspace));
      XII_TEST_FLOAT(forest.m_fWeight, forestWeight, 0.0001);
      XII_TEST_INT(forest.m_uiComponentCount, componentCount);
      XII_TEST_INT(forest.m_Edges.GetCount(), count - componentCount);
    }
  }
}
