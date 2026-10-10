/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <memory>

namespace GraphTestDetail
{
  struct ReferenceVertex
  {
    xiiGraphVertexHandle m_Handle;
    bool                 m_bActive = false;
  };
  struct ReferenceEdge
  {
    xiiGraphEdgeHandle   m_Handle;
    xiiGraphVertexHandle m_Source;
    xiiGraphVertexHandle m_Target;
    bool                 m_bActive = true;
  };

  void CheckStructure(const xiiGraph& graph, xiiArrayPtr<const ReferenceVertex> vertices,
                      xiiArrayPtr<const ReferenceEdge> edges, const xiiHashTable<xiiGraphEdgeHandle, xiiUInt32>& edgeIndices)
  {
    xiiUInt32 vertexCount = 0;
    xiiUInt32 edgeCount   = 0;
    for (const auto& edge : edges)
    {
      XII_TEST_BOOL(graph.IsValid(edge.m_Handle) == edge.m_bActive);
      if (edge.m_bActive)
      {
        ++edgeCount;
        XII_TEST_BOOL(graph.GetSource(edge.m_Handle) == edge.m_Source);
        XII_TEST_BOOL(graph.GetTarget(edge.m_Handle) == edge.m_Target);
      }
    }
    XII_TEST_INT(graph.GetEdgeCount(), edgeCount);
    xiiDynamicArray<xiiUInt8> seen;
    seen.SetCount(edges.GetCount());
    for (auto& value : seen)
      value = 0;
    for (const auto handle : graph.GetEdges())
    {
      xiiUInt32 index;
      if (!XII_TEST_BOOL(edgeIndices.TryGetValue(handle, index)))
        return;
      XII_TEST_BOOL(edges[index].m_bActive);
      XII_TEST_INT(seen[index]++, 0);
    }
    for (const auto& vertex : vertices)
    {
      XII_TEST_BOOL(graph.IsValid(vertex.m_Handle) == vertex.m_bActive);
      if (!vertex.m_bActive)
        continue;
      ++vertexCount;
      xiiUInt32 inDegree  = 0;
      xiiUInt32 outDegree = 0;
      for (const auto& edge : edges)
      {
        if (edge.m_bActive)
        {
          inDegree += edge.m_Target == vertex.m_Handle ? 1 : 0;
          outDegree += edge.m_Source == vertex.m_Handle ? 1 : 0;
        }
      }
      XII_TEST_INT(graph.GetInDegree(vertex.m_Handle), inDegree);
      XII_TEST_INT(graph.GetOutDegree(vertex.m_Handle), outDegree);
      xiiUInt32 count = 0;
      for (auto it = graph.GetOutgoingEdges(vertex.m_Handle); it.IsValid() && count <= edgeCount; ++it, ++count)
      {
        xiiUInt32 index;
        if (!XII_TEST_BOOL(edgeIndices.TryGetValue(it.GetHandle(), index)))
          return;
        XII_TEST_BOOL(edges[index].m_bActive && edges[index].m_Source == vertex.m_Handle);
        XII_TEST_INT(seen[index] & 2, 0);
        seen[index] |= 2;
      }
      XII_TEST_INT(count, outDegree);
      count = 0;
      for (auto it = graph.GetIncomingEdges(vertex.m_Handle); it.IsValid() && count <= edgeCount; ++it, ++count)
      {
        xiiUInt32 index;
        if (!XII_TEST_BOOL(edgeIndices.TryGetValue(it.GetHandle(), index)))
          return;
        XII_TEST_BOOL(edges[index].m_bActive && edges[index].m_Target == vertex.m_Handle);
        XII_TEST_INT(seen[index] & 4, 0);
        seen[index] |= 4;
      }
      XII_TEST_INT(count, inDegree);
    }
    XII_TEST_INT(graph.GetVertexCount(), vertexCount);
    for (xiiUInt32 i = 0; i < edges.GetCount(); ++i)
      XII_TEST_INT(seen[i], edges[i].m_bActive ? 7 : 0);
    for (xiiUInt32 i = 0; i < graph.GetVertexCount(); ++i)
    {
      const auto handle = graph.GetVertices()[i];
      XII_TEST_BOOL(graph.IsValid(handle));
      XII_TEST_INT(graph.GetVertexIndex(handle), i);
      xiiUInt32 occurrences = 0;
      for (const auto& vertex : vertices)
        occurrences += vertex.m_bActive && vertex.m_Handle == handle ? 1 : 0;
      XII_TEST_INT(occurrences, 1);
    }
  }

  xiiUInt32 FindComponent(const xiiGraphComponents& components, xiiGraphVertexHandle vertex)
  {
    xiiUInt32 found = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < components.GetCount(); ++i)
    {
      XII_TEST_BOOL(!components.GetComponent(i).IsEmpty());
      for (const auto member : components.GetComponent(i))
      {
        if (member == vertex)
        {
          XII_TEST_INT(found, xiiInvalidIndex);
          found = i;
        }
      }
    }
    XII_TEST_BOOL(found != xiiInvalidIndex);
    return found;
  }
} // namespace GraphTestDetail

XII_CREATE_SIMPLE_TEST(Graph, Stress)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Randomized mutations compared with independent edge records")
  {
    for (xiiUInt32 seed = 1; seed <= 4; ++seed)
    {
      xiiRandom random;
      random.Initialize(seed * 0x1234567ULL);
      xiiGraph                                        graph;
      GraphTestDetail::ReferenceVertex                vertices[24];
      xiiDynamicArray<GraphTestDetail::ReferenceEdge> edges;
      xiiHashTable<xiiGraphEdgeHandle, xiiUInt32>     edgeIndices;
      for (xiiUInt32 step = 0; step < 2000; ++step)
      {
        const auto slot   = random.UIntInRange(24);
        const auto action = random.UIntInRange(10);
        if (action < 2 && !vertices[slot].m_bActive)
        {
          const auto old = vertices[slot].m_Handle;
          vertices[slot] = {graph.AddVertex(), true};
          XII_TEST_BOOL(!graph.IsValid(old));
        }
        else if (action < (seed % 2 == 0 ? 8U : 5U))
        {
          const auto target = random.UIntInRange(24);
          if (vertices[slot].m_bActive && vertices[target].m_bActive)
          {
            const auto sourceHandle = vertices[slot].m_Handle;
            const auto targetHandle = vertices[target].m_Handle;
            const auto handle       = graph.AddEdge(sourceHandle, targetHandle);
            edgeIndices.Insert(handle, edges.GetCount());
            edges.PushBack({handle, sourceHandle, targetHandle});
            XII_TEST_BOOL(graph.FindEdge(sourceHandle, targetHandle) == handle);
          }
        }
        else if (action == 8 && !edges.IsEmpty())
        {
          auto&      edge   = edges[random.UIntInRange(edges.GetCount())];
          const auto target = random.UIntInRange(24);
          if (edge.m_bActive && vertices[slot].m_bActive && vertices[target].m_bActive && random.Bool())
          {
            XII_TEST_RESULT(graph.ReconnectEdge(edge.m_Handle, vertices[slot].m_Handle, vertices[target].m_Handle));
            edge.m_Source = vertices[slot].m_Handle;
            edge.m_Target = vertices[target].m_Handle;
          }
          else
          {
            XII_TEST_BOOL(graph.RemoveEdge(edge.m_Handle) == edge.m_bActive);
            edge.m_bActive = false;
          }
        }
        else
        {
          auto& vertex = vertices[slot];
          XII_TEST_BOOL(graph.RemoveVertex(vertex.m_Handle) == vertex.m_bActive);
          vertex.m_bActive = false;
          for (auto& edge : edges)
          {
            if (edge.m_Source == vertex.m_Handle || edge.m_Target == vertex.m_Handle)
              edge.m_bActive = false;
          }
        }
        if (step % 257 == 256)
        {
          graph.Clear();
          for (auto& vertex : vertices)
            vertex.m_bActive = false;
          for (auto& edge : edges)
            edge.m_bActive = false;
        }
        GraphTestDetail::CheckStructure(graph, xiiMakeArrayPtr(vertices), edges, edgeIndices);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Algorithms compared with independent transitive closure")
  {
    constexpr xiiUInt32 count = 12;
    xiiRandom           random;
    random.Initialize(0xC0DEF00D);
    xiiGraphAlgorithmWorkspace workspace;
    for (xiiUInt32 trial = 0; trial < 80; ++trial)
    {
      xiiGraph             graph;
      xiiGraphVertexHandle vertices[count];
      // Leave holes in the ID tables and reorder dense storage before running algorithms.
      for (xiiUInt32 i = 0; i < count * 4; ++i)
      {
        const auto vertex = graph.AddVertex();
        if (i % 4 == 0)
          vertices[i / 4] = vertex;
        else
          graph.RemoveVertex(vertex);
      }
      bool            reach[count][count] = {};
      bool            weak[count][count]  = {};
      xiiUInt32       distance[count][count];
      const xiiUInt32 densities[] = {0, 8, 35, 100};
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        for (xiiUInt32 j = 0; j < count; ++j)
        {
          distance[i][j] = i == j ? 0 : 10000;
          // Alternate arbitrary directed graphs with guaranteed DAGs.
          if (random.UIntInRange(100) < densities[(trial / 2) % 4] && (trial % 2 == 0 || i < j))
          {
            graph.AddEdge(vertices[i], vertices[j]);
            if (random.Bool())
              graph.AddEdge(vertices[i], vertices[j]);
            reach[i][j] = true;
            weak[i][j] = weak[j][i] = true;
            distance[i][j]          = i == j ? 0 : 1;
          }
        }
      }
      // Floyd-Warshall is intentionally different from the production algorithms.
      for (xiiUInt32 k = 0; k < count; ++k)
      {
        for (xiiUInt32 i = 0; i < count; ++i)
        {
          for (xiiUInt32 j = 0; j < count; ++j)
          {
            reach[i][j]    = reach[i][j] || (reach[i][k] && reach[k][j]);
            weak[i][j]     = weak[i][j] || (weak[i][k] && weak[k][j]);
            distance[i][j] = xiiMath::Min(distance[i][j], distance[i][k] + distance[k][j]);
          }
        }
      }
      bool cyclic = false;
      for (xiiUInt32 i = 0; i < count; ++i)
        cyclic |= reach[i][i];
      XII_TEST_BOOL(xiiGraphAlgorithms::HasCycle(graph, &workspace) == cyclic);
      xiiDynamicArray<xiiGraphVertexHandle> order;
      XII_TEST_BOOL(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace).Failed() == cyclic);
      XII_TEST_INT(order.GetCount(), cyclic ? 0 : count);
      if (!cyclic)
      {
        for (const auto edge : graph.GetEdges())
          XII_TEST_BOOL(order.IndexOf(graph.GetSource(edge)) < order.IndexOf(graph.GetTarget(edge)));
      }
      xiiGraphComponents strongComponents;
      xiiGraphComponents weakComponents;
      xiiGraphAlgorithms::StronglyConnectedComponents(graph, strongComponents, &workspace);
      xiiGraphAlgorithms::WeaklyConnectedComponents(graph, weakComponents, &workspace);
      XII_TEST_INT(strongComponents.m_Vertices.GetCount(), count);
      XII_TEST_INT(weakComponents.m_Vertices.GetCount(), count);
      xiiUInt32 strongIds[count];
      xiiUInt32 weakIds[count];
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        strongIds[i] = GraphTestDetail::FindComponent(strongComponents, vertices[i]);
        weakIds[i]   = GraphTestDetail::FindComponent(weakComponents, vertices[i]);
      }
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        for (xiiUInt32 j = 0; j < count; ++j)
        {
          XII_TEST_BOOL(xiiGraphAlgorithms::IsReachable(graph, vertices[i], vertices[j], &workspace) == (i == j || reach[i][j]));
          XII_TEST_BOOL((strongIds[i] == strongIds[j]) == (i == j || (reach[i][j] && reach[j][i])));
          XII_TEST_BOOL((weakIds[i] == weakIds[j]) == (i == j || weak[i][j]));
        }
        bool       visited[count]   = {};
        xiiUInt32  previousDistance = 0;
        const auto visitor          = [&](xiiGraphVertexHandle vertex) {
          xiiUInt32 index = 0;
          while (index < count && vertices[index] != vertex)
            ++index;
          if (!XII_TEST_BOOL(index < count))
            return false;
          XII_TEST_BOOL(!visited[index]);
          visited[index] = true;
          XII_TEST_BOOL(distance[i][index] >= previousDistance);
          previousDistance = distance[i][index];
          return true;
        };
        XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, vertices[i], visitor, &workspace));
        for (xiiUInt32 j = 0; j < count; ++j)
          XII_TEST_BOOL(visited[j] == (i == j || reach[i][j]));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Allocator ownership, reserve, scratch reuse and destruction")
  {
    xiiAllocatorWithPolicy<xiiAllocationPolicyProxy, xiiAllocatorTrackingMode::AllocationStats> allocator("GraphTest", xiiFoundation::GetDefaultAllocator());
    {
      xiiGraph                              graph(&allocator);
      xiiGraphAlgorithmWorkspace            workspace(&allocator);
      xiiGraphComponents                    components(&allocator);
      xiiDynamicArray<xiiGraphVertexHandle> order(&allocator);
      XII_TEST_RESULT(graph.Reserve(512, 512));
      const auto allocations = allocator.GetStats().m_uiAllocationCount;
      auto       first       = graph.AddVertex();
      auto       last        = first;
      for (xiiUInt32 i = 1; i < 256; ++i)
      {
        const auto vertex = graph.AddVertex();
        graph.AddEdge(last, vertex);
        last = vertex;
      }
      for (auto it = graph.GetOutgoingEdges(first); it.IsValid(); ++it)
        XII_TEST_BOOL(graph.IsValid(it.GetHandle()));
      XII_TEST_INT(allocator.GetStats().m_uiAllocationCount, allocations);
      const auto run = [&]() {
        const auto visitor = [](xiiGraphVertexHandle) {
          return true;
        };
        XII_TEST_RESULT(xiiGraphAlgorithms::BreadthFirstTraversal(graph, first, visitor, &workspace));
        XII_TEST_RESULT(xiiGraphAlgorithms::DepthFirstTraversal(graph, first, visitor, &workspace));
        XII_TEST_RESULT(xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace));
        XII_TEST_BOOL(!xiiGraphAlgorithms::HasCycle(graph, &workspace));
        XII_TEST_BOOL(xiiGraphAlgorithms::IsReachable(graph, first, last, &workspace));
        xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace);
        xiiGraphAlgorithms::WeaklyConnectedComponents(graph, components, &workspace);
      };
      run();
      const auto warmedAllocations  = allocator.GetStats().m_uiAllocationCount;
      const auto defaultAllocations = xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount;
      for (xiiUInt32 i = 0; i < 10; ++i)
        run();
      XII_TEST_INT(xiiFoundation::GetDefaultAllocator()->GetStats().m_uiAllocationCount, defaultAllocations);
      XII_TEST_INT(allocator.GetStats().m_uiAllocationCount, warmedAllocations);
      graph.Clear();
      XII_TEST_INT(allocator.GetStats().m_uiAllocationCount, warmedAllocations);
    }
    XII_TEST_INT(allocator.GetStats().m_uiAllocationCount, allocator.GetStats().m_uiDeallocationCount);
    XII_TEST_INT(allocator.GetStats().m_uiAllocationSize, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Destroyed graphs never regain ownership at the same address")
  {
    alignas(xiiGraph) unsigned char storage[sizeof(xiiGraph)];
    auto*                           graph     = std::construct_at(reinterpret_cast<xiiGraph*>(storage));
    const auto                      oldVertex = graph->AddVertex();
    const auto                      oldEdge   = graph->AddEdge(oldVertex, oldVertex);
    std::destroy_at(graph);
    graph             = std::construct_at(reinterpret_cast<xiiGraph*>(storage));
    const auto vertex = graph->AddVertex();
    const auto edge   = graph->AddEdge(vertex, vertex);
    XII_TEST_BOOL(vertex != oldVertex && edge != oldEdge);
    XII_TEST_BOOL(!graph->IsValid(oldVertex) && !graph->IsValid(oldEdge));
    std::destroy_at(graph);
  }
}
