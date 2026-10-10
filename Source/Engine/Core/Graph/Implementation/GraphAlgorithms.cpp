/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Graph/GraphAlgorithms.h>

xiiGraphAlgorithmWorkspace::xiiGraphAlgorithmWorkspace(xiiAllocator* pAllocator) :
  m_State(pAllocator), m_Work(pAllocator), m_Order(pAllocator), m_Frames(pAllocator)
{
}

void xiiGraphAlgorithmWorkspace::Reset(xiiUInt32 vertexCount)
{
  m_State.SetCount(vertexCount);
  for (auto& state : m_State)
    state = 0;
  m_Work.Clear();
  m_Order.Clear();
  m_Frames.Clear();
}

xiiGraphComponents::xiiGraphComponents(xiiAllocator* pAllocator) :
  m_Vertices(pAllocator), m_Offsets(pAllocator)
{
}

xiiArrayPtr<const xiiGraphVertexHandle> xiiGraphComponents::GetComponent(xiiUInt32 index) const
{
  XII_ASSERT_DEV(index < GetCount(), "Invalid component index.");
  return m_Vertices.GetArrayPtr().GetSubArray(m_Offsets[index], m_Offsets[index + 1] - m_Offsets[index]);
}

xiiResult xiiGraphAlgorithms::BreadthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace)
{
  if (!graph.IsValid(hStart) || !visitor.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  const auto vertices    = graph.GetVertices();
  const auto start       = graph.GetVertexIndex(hStart);
  scratch.m_State[start] = 1;
  scratch.m_Work.PushBack(start);
  for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
  {
    const auto vertex = vertices[scratch.m_Work[head]];
    if (!visitor(vertex))
      break;
    for (auto it = graph.GetOutgoingEdges(vertex); it.IsValid(); ++it)
    {
      const auto next = graph.GetVertexIndex(graph.GetTarget(it.GetHandle()));
      if (scratch.m_State[next] == 0)
      {
        scratch.m_State[next] = 1;
        scratch.m_Work.PushBack(next);
      }
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::DepthFirstTraversal(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace* pWorkspace)
{
  if (!graph.IsValid(hStart) || !visitor.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  WalkDepthFirst(graph, hStart, visitor, scratch);
  return XII_SUCCESS;
}

void xiiGraphAlgorithms::WalkDepthFirst(const xiiGraph& graph, xiiGraphVertexHandle hStart, Visitor visitor, xiiGraphAlgorithmWorkspace& scratch)
{
  const auto vertices    = graph.GetVertices();
  const auto start       = graph.GetVertexIndex(hStart);
  scratch.m_State[start] = 1;
  if (!visitor(hStart))
    return;
  scratch.m_Frames.PushBack({start, graph.GetOutgoingEdges(hStart)});
  while (!scratch.m_Frames.IsEmpty())
  {
    auto& frame = scratch.m_Frames.PeekBack();
    if (!frame.m_Edges.IsValid())
    {
      scratch.m_Order.PushBack(frame.m_uiVertex);
      scratch.m_Frames.PopBack();
      continue;
    }
    const auto next = graph.GetVertexIndex(graph.GetTarget(frame.m_Edges.GetHandle()));
    frame.m_Edges.Next();
    if (scratch.m_State[next] == 0)
    {
      scratch.m_State[next] = 1;
      if (!visitor(vertices[next]))
        break;
      scratch.m_Frames.PushBack({next, graph.GetOutgoingEdges(vertices[next])});
    }
  }
}

bool xiiGraphAlgorithms::IsReachable(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphAlgorithmWorkspace* pWorkspace)
{
  if (!graph.IsValid(hSource) || !graph.IsValid(hTarget))
    return false;
  bool found = false;
  BreadthFirstTraversal(graph, hSource, [&](xiiGraphVertexHandle vertex) { found = vertex == hTarget; return !found; }, pWorkspace).IgnoreResult();
  return found;
}

xiiResult xiiGraphAlgorithms::TopologicalSort(const xiiGraph& graph, xiiDynamicArray<xiiGraphVertexHandle>& out_order, xiiGraphAlgorithmWorkspace* pWorkspace)
{
  out_order.Clear();
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  XII_SUCCEED_OR_RETURN(SortTopologically(graph, scratch));
  const auto vertices = graph.GetVertices();
  out_order.Reserve(vertices.GetCount());
  for (auto index : scratch.m_Work)
    out_order.PushBack(vertices[index]);
  return XII_SUCCESS;
}

xiiResult xiiGraphAlgorithms::SortTopologically(const xiiGraph& graph, xiiGraphAlgorithmWorkspace& scratch)
{
  scratch.Reset(graph.GetVertexCount());
  const auto vertices = graph.GetVertices();
  for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
  {
    scratch.m_State[i] = graph.GetInDegree(vertices[i]);
    if (scratch.m_State[i] == 0)
      scratch.m_Work.PushBack(i);
  }
  for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount(); ++head)
  {
    const auto vertex = vertices[scratch.m_Work[head]];
    for (auto it = graph.GetOutgoingEdges(vertex); it.IsValid(); ++it)
    {
      const auto next = graph.GetVertexIndex(graph.GetTarget(it.GetHandle()));
      if (--scratch.m_State[next] == 0)
        scratch.m_Work.PushBack(next);
    }
  }
  return scratch.m_Work.GetCount() == vertices.GetCount() ? XII_SUCCESS : XII_FAILURE;
}

bool xiiGraphAlgorithms::HasCycle(const xiiGraph& graph, xiiGraphAlgorithmWorkspace* pWorkspace)
{
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  return SortTopologically(graph, scratch).Failed();
}

void xiiGraphAlgorithms::StronglyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace)
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
    WalkDepthFirst(graph, vertices[root], [](xiiGraphVertexHandle) { return true; }, scratch);
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

void xiiGraphAlgorithms::WeaklyConnectedComponents(const xiiGraph& graph, xiiGraphComponents& out_components, xiiGraphAlgorithmWorkspace* pWorkspace)
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
        visit(graph.GetTarget(it.GetHandle()));
      for (auto it = graph.GetIncomingEdges(vertex); it.IsValid(); ++it)
        visit(graph.GetSource(it.GetHandle()));
    }
    out_components.m_Offsets.PushBack(out_components.m_Vertices.GetCount());
  }
}
