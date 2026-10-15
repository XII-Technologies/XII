/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphView.h>
#include <CoreTest/CoreTestPCH.h>

XII_CREATE_SIMPLE_TEST(Graph, Views)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selected and induced membership follows current topology")
  {
    xiiGraph                   graph;
    const auto                 a = graph.AddVertex(), b = graph.AddVertex(), c = graph.AddVertex();
    const auto                 ab         = graph.AddEdge(a, b);
    const xiiGraphVertexHandle vertices[] = {a, b};
    const xiiGraphEdgeHandle   edges[]    = {ab};
    xiiGraphView               induced(graph), selected(graph);
    XII_TEST_RESULT(induced.Select(vertices));
    XII_TEST_RESULT(selected.Select(vertices, xiiGraphView::Mode::Selected, edges));
    const auto parallel = graph.AddEdge(a, b);
    XII_TEST_BOOL(induced.Contains(parallel) && !selected.Contains(parallel));
    XII_TEST_RESULT(graph.ReconnectEdge(ab, a, c));
    XII_TEST_BOOL(!induced.Contains(ab) && !selected.Contains(ab));
    XII_TEST_RESULT(graph.ReconnectEdge(ab, b, a));
    XII_TEST_BOOL(selected.Contains(ab));
    xiiGraph          copy;
    xiiGraphHandleMap copyMap;
    XII_TEST_RESULT(induced.CopyTo(copy, copyMap));
    XII_TEST_INT(copy.GetVertexCount(), 2);
    XII_TEST_INT(copy.GetEdgeCount(), 2);
    XII_TEST_BOOL(!copy.IsValid(a));
    xiiGraphHandleMap compactMap;
    XII_TEST_RESULT(graph.Compact(compactMap));
    XII_TEST_BOOL(!induced.Contains(a));
    XII_TEST_RESULT(induced.Remap(compactMap));
    XII_TEST_BOOL(induced.Contains(compactMap.m_Vertices[a]));
    graph.RemoveVertex(compactMap.m_Vertices[b]);
    induced.Refresh();
    xiiDynamicArray<xiiGraphVertexHandle> liveVertices;
    xiiDynamicArray<xiiGraphEdgeHandle>   liveEdges;
    induced.Enumerate(liveVertices, liveEdges);
    XII_TEST_INT(liveVertices.GetCount(), 1);
    XII_TEST_INT(liveEdges.GetCount(), 0);
    xiiGraph moved(std::move(graph));
    induced.Enumerate(liveVertices, liveEdges);
    XII_TEST_BOOL(liveVertices.IsEmpty());
  }
}
