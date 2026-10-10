/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graph/GraphData.h>
#include <Core/Graph/GraphMappings.h>
#include <Core/Graph/GraphSerialization.h>
#include <Core/Graph/GraphSynchronization.h>
#include <CoreTest/CoreTestPCH.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Threading/Thread.h>

namespace GraphFacilitiesTestDetail
{
  class RejectingAllocator : public xiiAllocator
  {
  public:
    void*          Allocate(size_t, size_t, xiiMemoryUtils::DestructorFunction = nullptr) override { return nullptr; }
    void           Deallocate(void*) override {}
    size_t         AllocatedSize(const void*) override { return 0; }
    xiiAllocatorId GetId() const override { return {}; }
    Stats          GetStats() const override { return {}; }
  };
  class WriterThread : public xiiThread
  {
  public:
    explicit WriterThread(xiiSynchronizedGraph& graph) : xiiThread("GraphWriter"), m_Graph(graph) {}

  private:
    xiiUInt32 Run() override
    {
      for (xiiUInt32 i = 0; i < 200; ++i)
      {
        auto       access = m_Graph.Write();
        const auto vertex = access->AddVertex();
        access->AddEdge(vertex, vertex);
      }
      return 0;
    }
    xiiSynchronizedGraph& m_Graph;
  };

  class FailingWriter : public xiiStreamWriter
  {
    xiiResult WriteBytes(const void*, xiiUInt64) override { return XII_FAILURE; }
  };
} // namespace GraphFacilitiesTestDetail

XII_CREATE_SIMPLE_TEST(Graph, Facilities)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Recoverable metadata allocation and full empty compaction")
  {
    GraphFacilitiesTestDetail::RejectingAllocator failingAllocator;
    xiiGraph                                      failingGraph(&failingAllocator);
    xiiGraphVertexHandle                          rejected;
    XII_TEST_BOOL(failingGraph.TryInitialize().Failed());
    XII_TEST_BOOL(failingGraph.TryAddVertex(rejected).Failed());
    XII_TEST_BOOL(failingGraph.Reserve(1, 1).Failed());
    XII_TEST_BOOL(rejected.IsInvalidated());
    XII_TEST_INT(failingGraph.GetVertexCount(), 0);
    xiiAllocatorWithPolicy<xiiAllocationPolicyProxy, xiiAllocatorTrackingMode::AllocationStats> allocator("GraphCompactTest", xiiFoundation::GetDefaultAllocator());
    xiiGraph                                                                                    graph(&allocator);
    const auto                                                                                  old = graph.AddVertex();
    for (xiiUInt32 i = 0; i < 1024; ++i)
      graph.AddEdge(old, graph.AddVertex());
    graph.Clear();
    XII_TEST_BOOL(allocator.GetStats().m_uiAllocationSize > 0);
    xiiGraphHandleMap mapping;
    XII_TEST_RESULT(graph.Compact(mapping));
    XII_TEST_INT(allocator.GetStats().m_uiAllocationSize, 0);
    const auto fresh = graph.AddVertex();
    XII_TEST_BOOL(fresh != old && !graph.IsValid(old));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Validation, events, reentrant mutation and reconnection")
  {
    xiiGraph   graph;
    const auto a = graph.AddVertex();
    const auto b = graph.AddVertex();
    const auto c = graph.AddVertex();
    XII_TEST_RESULT(graph.SetConnectionValidator([](const xiiGraph& g, xiiGraphVertexHandle source, xiiGraphVertexHandle target, xiiGraphEdgeHandle existing) {
      return source != target && (g.FindEdge(source, target).IsInvalidated() || g.FindEdge(source, target) == existing) ? XII_SUCCESS : XII_FAILURE;
    }));
    XII_TEST_BOOL(graph.AddEdge(a, a).IsInvalidated());
    const auto edge = graph.AddEdge(a, b);
    XII_TEST_BOOL(graph.AddEdge(a, b).IsInvalidated());
    XII_TEST_BOOL(graph.ReconnectEdge(edge, b, b).Failed());
    XII_TEST_BOOL(graph.GetSource(edge) == a && graph.GetTarget(edge) == b);
    xiiDynamicArray<xiiGraphEvent::Type> events;
    xiiGraph::Event::Unsubscriber        subscription;
    graph.GetEvents().AddEventHandler([&](const xiiGraphEvent& event) {
      events.PushBack(event.m_Type);
      XII_TEST_BOOL(event.m_pGraph == &graph);
      xiiGraphVertexHandle rejected;
      XII_TEST_BOOL(graph.TryAddVertex(rejected).Failed());
      XII_TEST_BOOL(graph.TryClear().Failed());
      XII_TEST_BOOL(graph.Reserve(8, 8).Failed());
      if (event.m_Type == xiiGraphEvent::Type::BeforeEdgeRemoved)
        XII_TEST_BOOL(graph.IsValid(event.m_hEdge));
      if (event.m_Type == xiiGraphEvent::Type::EdgeRemoved)
        XII_TEST_BOOL(!graph.IsValid(event.m_hEdge));
    },
                                      subscription);
    XII_TEST_RESULT(graph.ReconnectEdge(edge, c, a));
    XII_TEST_BOOL(graph.IsValid(edge));
    XII_TEST_INT(graph.GetOutDegree(a), 0);
    XII_TEST_INT(graph.GetInDegree(b), 0);
    XII_TEST_INT(graph.GetOutDegree(c), 1);
    XII_TEST_INT(graph.GetInDegree(a), 1);
    XII_TEST_RESULT(graph.TryRemoveVertex(a));
    const xiiGraphEvent::Type expected[] = {xiiGraphEvent::Type::EdgeReconnected, xiiGraphEvent::Type::BeforeVertexRemoved,
                                            xiiGraphEvent::Type::BeforeEdgeRemoved, xiiGraphEvent::Type::EdgeRemoved, xiiGraphEvent::Type::VertexRemoved};
    XII_TEST_BOOL(events.GetArrayPtr() == xiiMakeArrayPtr(expected));
    subscription.Unsubscribe();
    XII_TEST_RESULT(graph.SetConnectionValidator({}));
    const auto loop = graph.AddEdge(c, c);
    XII_TEST_RESULT(graph.ReconnectEdge(loop, b, c));
    XII_TEST_INT(graph.GetOutDegree(c), 0);
    XII_TEST_INT(graph.GetInDegree(c), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy, move, compaction and transactional limits")
  {
    xiiGraph   source;
    const auto a    = source.AddVertex();
    const auto b    = source.AddVertex();
    const auto edge = source.AddEdge(a, b);
    source.AddEdge(a, b);
    source.AddEdge(b, b);
    xiiGraph          destination;
    const auto        replaced = destination.AddVertex();
    xiiGraphHandleMap map;
    XII_TEST_RESULT(destination.CopyFrom(source, &map));
    XII_TEST_BOOL(source.IsValid(a) && source.IsValid(edge));
    XII_TEST_BOOL(!destination.IsValid(replaced) && !destination.IsValid(a));
    XII_TEST_BOOL(destination.IsValid(map.m_Vertices[a]));
    XII_TEST_BOOL(destination.GetSource(map.m_Edges[edge]) == map.m_Vertices[a]);
    XII_TEST_BOOL(destination.GetTarget(map.m_Edges[edge]) == map.m_Vertices[b]);
    const auto copiedA = map.m_Vertices[a];
    xiiGraph   moved(std::move(destination));
    XII_TEST_BOOL(moved.IsValid(copiedA) && !destination.IsValid(copiedA));
    XII_TEST_INT(destination.GetVertexCount(), 0);
    XII_TEST_BOOL(destination.IsValid(destination.AddVertex()));
    XII_TEST_RESULT(moved.Compact(map));
    XII_TEST_BOOL(!moved.IsValid(copiedA));
    XII_TEST_BOOL(moved.IsValid(map.m_Vertices[copiedA]));
    XII_TEST_INT(moved.GetEdgeCount(), 3);
    xiiGraph limited;
    XII_TEST_RESULT(limited.SetLimits(1, 1));
    const auto survivor = limited.AddVertex();
    XII_TEST_BOOL(limited.CopyFrom(source).Failed());
    XII_TEST_BOOL(limited.MoveFrom(std::move(source)).Failed());
    XII_TEST_BOOL(limited.IsValid(survivor) && source.IsValid(a));
    XII_TEST_RESULT(limited.SetConnectionValidator([](const xiiGraph&, xiiGraphVertexHandle, xiiGraphVertexHandle, xiiGraphEdgeHandle) { return XII_FAILURE; }));
    XII_TEST_RESULT(limited.SetLimits(8, 8));
    XII_TEST_BOOL(limited.CopyFrom(source).Failed());
    XII_TEST_BOOL(limited.IsValid(survivor));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Portable reflected topology and failed stream transactions")
  {
    xiiGraph   graph;
    const auto a = graph.AddVertex();
    const auto b = graph.AddVertex();
    graph.AddVertex();
    graph.AddEdge(a, b);
    graph.AddEdge(a, b);
    graph.AddEdge(b, b);
    graph.AddEdge(b, a);
    xiiGraphSnapshot snapshot;
    snapshot.Capture(graph);
    XII_TEST_RESULT(snapshot.Validate());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGraphSnapshot>()->FindPropertyByName("EdgeSources") != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGraph>()->FindPropertyByName("VertexCount") != nullptr);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(snapshot.Save(writer));
    xiiMemoryStreamReader reader(&storage);
    xiiGraphSnapshot      loaded;
    XII_TEST_RESULT(loaded.Load(reader));
    XII_TEST_INT(loaded.m_uiVertexCount, 3);
    XII_TEST_BOOL(loaded.m_EdgeSources == snapshot.m_EdgeSources && loaded.m_EdgeTargets == snapshot.m_EdgeTargets);
    xiiGraph              replacement;
    xiiGraphLoadedHandles handles;
    XII_TEST_RESULT(loaded.Apply(replacement, &handles));
    XII_TEST_INT(replacement.GetVertexCount(), 3);
    XII_TEST_INT(replacement.GetEdgeCount(), 4);
    for (xiiUInt32 i = 0; i < handles.m_Edges.GetCount(); ++i)
    {
      XII_TEST_BOOL(replacement.GetSource(handles.m_Edges[i]) == handles.m_Vertices[loaded.m_EdgeSources[i]]);
      XII_TEST_BOOL(replacement.GetTarget(handles.m_Edges[i]) == handles.m_Vertices[loaded.m_EdgeTargets[i]]);
    }
    GraphFacilitiesTestDetail::FailingWriter failedWriter;
    XII_TEST_BOOL(loaded.Save(failedWriter).Failed());
    const auto bytes = storage.GetContiguousMemoryRange(0);
    for (xiiUInt32 length = 0; length < bytes.GetCount(); ++length)
    {
      xiiRawMemoryStreamReader truncated(bytes.GetPtr(), length);
      XII_TEST_BOOL(loaded.Load(truncated).Failed());
      XII_TEST_INT(loaded.m_uiVertexCount, 3);
      XII_TEST_BOOL(loaded.m_EdgeSources == snapshot.m_EdgeSources);
    }
    xiiMemoryStreamReader bounded(&storage);
    XII_TEST_BOOL(loaded.Load(bounded, 2, 100).Failed());
    loaded.m_EdgeTargets[0] = 99;
    XII_TEST_BOOL(loaded.Apply(replacement).Failed());
    XII_TEST_BOOL(replacement.IsValid(handles.m_Vertices[0]));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Typed ownership and automatic incident-data cleanup")
  {
    xiiGraphWithData<xiiUniquePtr<xiiUInt32>, xiiUniquePtr<xiiUInt32>> graph;
    const auto                                                         a    = graph.m_Graph.AddVertex();
    const auto                                                         b    = graph.m_Graph.AddVertex();
    const auto                                                         edge = graph.m_Graph.AddEdge(a, b);
    XII_TEST_RESULT(graph.m_Data.SetVertexData(a, xiiUniquePtr<xiiUInt32>(XII_DEFAULT_NEW(xiiUInt32, 42))));
    XII_TEST_RESULT(graph.m_Data.SetEdgeData(edge, xiiUniquePtr<xiiUInt32>(XII_DEFAULT_NEW(xiiUInt32, 7))));
    XII_TEST_INT(**graph.m_Data.GetVertexData(a), 42);
    XII_TEST_INT(**graph.m_Data.GetEdgeData(edge), 7);
    XII_TEST_RESULT(graph.m_Graph.TryRemoveVertex(a));
    XII_TEST_BOOL(graph.m_Data.GetVertexData(a) == nullptr && graph.m_Data.GetEdgeData(edge) == nullptr);
    XII_TEST_BOOL(graph.m_Data.SetVertexData(a, xiiUniquePtr<xiiUInt32>()).Failed());
    XII_TEST_RESULT(graph.m_Data.SetVertexData(b, xiiUniquePtr<xiiUInt32>(XII_DEFAULT_NEW(xiiUInt32, 3))));
    xiiGraphHandleMap mapping;
    XII_TEST_RESULT(graph.m_Graph.Compact(mapping));
    XII_TEST_INT(**graph.m_Data.GetVertexData(mapping.m_Vertices[b]), 3);
    XII_TEST_BOOL(!graph.m_Graph.IsValid(b));
    XII_TEST_RESULT(graph.m_Graph.TryClear());
    XII_TEST_BOOL(graph.m_Data.GetVertexData(b) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Concurrent mutation through Foundation scoped locking")
  {
    xiiSynchronizedGraph                    graph;
    GraphFacilitiesTestDetail::WriterThread a(graph), b(graph), c(graph), d(graph);
    a.Start();
    b.Start();
    c.Start();
    d.Start();
    a.Join();
    b.Join();
    c.Join();
    d.Join();
    auto access = graph.Read();
    XII_TEST_INT(access->GetVertexCount(), 800);
    XII_TEST_INT(access->GetEdgeCount(), 800);
    for (auto vertex : access->GetVertices())
    {
      XII_TEST_INT(access->GetInDegree(vertex), 1);
      XII_TEST_INT(access->GetOutDegree(vertex), 1);
    }
  }
}
