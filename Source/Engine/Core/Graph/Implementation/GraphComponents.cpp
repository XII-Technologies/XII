/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

void xiiGraphAlgorithms::StronglyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_components.m_Vertices.Clear();
  out_components.m_Offsets.Clear();
  out_components.m_Offsets.PushBack(0);
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  const auto vertices = graph.GetVertices();
  // First pass: explicit DFS frames preserve finishing order, without using the C++ call stack.
  for (xiiUInt32 root = 0; root < vertices.GetCount(); ++root)
  {
    if (scratch.m_State[root] != 0)
      continue;
    WalkDepthFirst(graph, vertices[root], [](xiiGraphVertexHandle) { return true; }, scratch, filter);
  }
  // Second pass: traverse incoming edges in reverse finishing order. No transposed topology allocation.
  for (xiiUInt32 i = scratch.m_Order.GetCount(); i > 0; --i)
  {
    const auto root = scratch.m_Order[i - 1];
    if (scratch.m_State[root] != 1)
      continue;
    scratch.m_Work.Clear();
    scratch.m_Work.PushBack(root);
    scratch.m_State[root] = 2;
    for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
    {
      const auto vertex = vertices[scratch.m_Work[head]];
      out_components.m_Vertices.PushBack(vertex);
      for (auto it = graph.GetIncomingEdges(vertex); it.IsValid(); ++it)
      {
        if (filter.IsValid() && !filter(it.GetHandle()))
          continue;
        const auto next = graph.GetVertexIndex(graph.GetSource(it.GetHandle()));
        if (scratch.m_State[next] == 1)
        {
          scratch.m_State[next] = 2;
          scratch.m_Work.PushBack(next);
        }
      }
    }
    out_components.m_Offsets.PushBack(out_components.m_Vertices.GetCount());
  }
}

void xiiGraphAlgorithms::WeaklyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_components.m_Vertices.Clear();
  out_components.m_Offsets.Clear();
  out_components.m_Offsets.PushBack(0);
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  const auto vertices = graph.GetVertices();
  for (xiiUInt32 root = 0; root < vertices.GetCount(); ++root)
  {
    if (scratch.m_State[root] != 0)
      continue;
    scratch.m_Work.Clear();
    scratch.m_Work.PushBack(root);
    scratch.m_State[root] = 1;
    for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
    {
      const auto vertex = vertices[scratch.m_Work[head]];
      out_components.m_Vertices.PushBack(vertex);
      const auto visit = [&](xiiGraphVertexHandle neighbour) {
        const auto next = graph.GetVertexIndex(neighbour);
        if (scratch.m_State[next] == 0)
        {
          scratch.m_State[next] = 1;
          scratch.m_Work.PushBack(next);
        }
      };
      for (auto it = graph.GetOutgoingEdges(vertex); it.IsValid(); ++it)
        if (!filter.IsValid() || filter(it.GetHandle())) visit(graph.GetTarget(it.GetHandle()));
      for (auto it = graph.GetIncomingEdges(vertex); it.IsValid(); ++it)
        if (!filter.IsValid() || filter(it.GetHandle())) visit(graph.GetSource(it.GetHandle()));
    }
    out_components.m_Offsets.PushBack(out_components.m_Vertices.GetCount());
  }
}
