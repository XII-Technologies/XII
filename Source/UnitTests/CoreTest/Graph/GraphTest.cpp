/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Graph/Graph.h>
#include <Foundation/Containers/HashTable.h>

XII_CREATE_SIMPLE_TEST_GROUP(Graph);

XII_CREATE_SIMPLE_TEST(Graph, Structure)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty and invalid operations")
  {
    xiiGraph graph;
    XII_TEST_INT(graph.GetVertexCount(), 0);
    XII_TEST_INT(graph.GetEdgeCount(), 0);
    XII_TEST_BOOL(graph.GetVertices().IsEmpty());
    XII_TEST_BOOL(graph.GetEdges().IsEmpty());
    XII_TEST_BOOL(!graph.IsValid(xiiGraphVertexHandle()));
    XII_TEST_BOOL(!graph.IsValid(xiiGraphEdgeHandle()));
    XII_TEST_BOOL(!graph.RemoveVertex({}));
    XII_TEST_BOOL(!graph.RemoveEdge({}));
    XII_TEST_BOOL(graph.AddEdge({}, {}).IsInvalidated());
    XII_TEST_BOOL(graph.GetSource({}).IsInvalidated());
    XII_TEST_BOOL(graph.GetTarget({}).IsInvalidated());
    XII_TEST_BOOL(!graph.GetIncomingEdges({}).IsValid());
    XII_TEST_BOOL(!graph.GetOutgoingEdges({}).IsValid());
    XII_TEST_INT(graph.GetInDegree({}), 0);
    XII_TEST_INT(graph.GetOutDegree({}), 0);
    XII_TEST_INT(graph.GetVertexIndex({}), xiiInvalidIndex);
    graph.Clear();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Parallel edges, loops and unlinking")
  {
    xiiGraph   graph;
    const auto a        = graph.AddVertex();
    const auto b        = graph.AddVertex();
    const auto isolated = graph.AddVertex();
    const auto ab1      = graph.AddEdge(a, b);
    const auto ab2      = graph.AddEdge(a, b);
    const auto ab3      = graph.AddEdge(a, b);
    const auto ba       = graph.AddEdge(b, a);
    const auto loop     = graph.AddEdge(a, a);
    XII_TEST_INT(graph.GetOutDegree(a), 4);
    XII_TEST_INT(graph.GetInDegree(a), 2);
    XII_TEST_INT(graph.GetInDegree(b), 3);
    XII_TEST_BOOL(graph.FindEdge(a, b) == ab3);
    XII_TEST_BOOL(graph.FindEdge(a, isolated).IsInvalidated());
    XII_TEST_BOOL(graph.GetSource(loop) == a && graph.GetTarget(loop) == a);
    XII_TEST_BOOL(graph.RemoveEdge(ab2)); // middle of both adjacency lists
    XII_TEST_BOOL(graph.RemoveEdge(ab3)); // head
    XII_TEST_BOOL(graph.FindEdge(a, b) == ab1);
    XII_TEST_BOOL(!graph.RemoveEdge(ab2));
    XII_TEST_BOOL(graph.RemoveVertex(a));
    XII_TEST_INT(graph.GetEdgeCount(), 0);
    XII_TEST_INT(graph.GetVertexCount(), 2);
    XII_TEST_BOOL(!graph.IsValid(ab1) && !graph.IsValid(loop) && !graph.IsValid(ba));
    XII_TEST_BOOL(graph.IsValid(b) && graph.IsValid(isolated));
    XII_TEST_INT(graph.GetInDegree(b), 0);
    XII_TEST_INT(graph.GetOutDegree(b), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Growth, stale handles, clear and graph ownership")
  {
    xiiGraph   graph;
    xiiGraph   other;
    const auto a           = graph.AddVertex();
    const auto foreign     = other.AddVertex();
    const auto edge        = graph.AddEdge(a, a);
    const auto foreignEdge = other.AddEdge(foreign, foreign);
    XII_TEST_BOOL(!other.IsValid(a) && !graph.IsValid(foreign));
    XII_TEST_BOOL(!graph.IsValid(foreignEdge) && !other.IsValid(edge));
    XII_TEST_BOOL(!graph.RemoveVertex(foreign) && !graph.RemoveEdge(foreignEdge));
    XII_TEST_BOOL(graph.AddEdge(a, foreign).IsInvalidated());
    XII_TEST_BOOL(graph.AddEdge(foreign, a).IsInvalidated());
    XII_TEST_BOOL(graph.FindEdge(a, foreign).IsInvalidated());
    XII_TEST_BOOL(graph.GetSource(foreignEdge).IsInvalidated());
    for (xiiUInt32 i = 0; i < 2048; ++i)
    {
      const auto v = graph.AddVertex();
      graph.AddEdge(a, v);
    }
    graph.Reserve(4096, 4096);
    XII_TEST_BOOL(graph.IsValid(a) && graph.IsValid(edge));
    XII_TEST_BOOL(graph.GetSource(edge) == a && graph.GetTarget(edge) == a);
    XII_TEST_INT(graph.GetOutDegree(a), 2049);
    XII_TEST_BOOL(graph.RemoveVertex(a));
    XII_TEST_INT(graph.GetEdgeCount(), 0);
    graph.Clear();
    for (xiiUInt32 i = 0; i < 4096; ++i)
    {
      const auto v = graph.AddVertex();
      const auto e = graph.AddEdge(v, v);
      XII_TEST_BOOL(v != a && e != edge);
      XII_TEST_BOOL(!graph.IsValid(a) && !graph.IsValid(edge));
      XII_TEST_BOOL(graph.RemoveVertex(v));
      XII_TEST_BOOL(!graph.IsValid(v) && !graph.IsValid(e));
    }
    const auto v = graph.AddVertex();
    const auto e = graph.AddEdge(v, v);
    graph.Clear();
    graph.AddVertex();
    XII_TEST_BOOL(!graph.IsValid(v) && !graph.IsValid(e));
    // External payload ownership uses ordinary engine containers, without polymorphic nodes.
    xiiHashTable<xiiGraphVertexHandle, xiiUInt32> payloads;
    payloads.Insert(foreign, 42);
    XII_TEST_INT(payloads[foreign], 42);
    xiiHashTable<xiiGraphEdgeHandle, xiiUInt32> edgePayloads;
    edgePayloads.Insert(foreignEdge, 7);
    XII_TEST_INT(edgePayloads[foreignEdge], 7);
  }
}
