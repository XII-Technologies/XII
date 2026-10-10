/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Core/Graph/GraphAlgorithms.h>

xiiResult xiiGraphAlgorithms::MaximumFlow(const xiiGraph& graph, xiiGraphVertexHandle hSource, xiiGraphVertexHandle hTarget, xiiGraphEdgeWeight capacity, xiiGraphFlow& out_flow, xiiGraphAlgorithmWorkspace* pWorkspace, xiiGraphEdgeFilter filter)
{
  out_flow.Clear();
  if (!graph.IsValid(hSource) || !graph.IsValid(hTarget) || hSource == hTarget || !capacity.IsValid())
    return XII_FAILURE;
  xiiGraphAlgorithmWorkspace local;
  auto&                      scratch = pWorkspace ? *pWorkspace : local;
  scratch.Reset(graph.GetVertexCount());
  scratch.m_EdgeValues.SetCount(graph.GetEdgeCount());
  scratch.m_EdgeFlows.SetCount(graph.GetEdgeCount());
  scratch.m_PreviousEdges.SetCount(graph.GetVertexCount());
  scratch.m_Order.SetCount(graph.GetVertexCount());
  const auto vertices = graph.GetVertices();
  const auto edges    = graph.GetEdges();
  for (xiiUInt32 i = 0; i < edges.GetCount(); ++i)
  {
    const double value = !filter.IsValid() || filter(edges[i]) ? capacity(edges[i]) : 0;
    if (!xiiMath::IsFinite(value) || value < 0)
      return XII_FAILURE;
    scratch.m_EdgeValues[i] = value;
    scratch.m_EdgeFlows[i]  = 0;
  }
  const auto source = graph.GetVertexIndex(hSource);
  const auto target = graph.GetVertexIndex(hTarget);
  double     total  = 0;
  while (true)
  {
    for (auto& state : scratch.m_State)
      state = 0;
    scratch.m_Work.Clear();
    scratch.m_Work.PushBack(source);
    scratch.m_State[source] = 1;
    for (xiiUInt32 head = 0; head < scratch.m_Work.GetCount() && scratch.m_State[target] == 0; ++head)
    {
      const auto current = scratch.m_Work[head];
      const auto visit   = [&](xiiGraphEdgeHandle edge, bool forward) {
        const auto   index    = graph.GetEdgeIndex(edge);
        const double residual = forward ? scratch.m_EdgeValues[index] - scratch.m_EdgeFlows[index] : scratch.m_EdgeFlows[index];
        const auto   next     = graph.GetVertexIndex(forward ? graph.GetTarget(edge) : graph.GetSource(edge));
        if (residual > 0 && scratch.m_State[next] == 0)
        {
          scratch.m_State[next]         = 1;
          scratch.m_Order[next]         = current;
          scratch.m_PreviousEdges[next] = edge;
          scratch.m_Work.PushBack(next);
        }
      };
      for (auto it = graph.GetOutgoingEdges(vertices[current]); it.IsValid(); ++it)
        visit(it.GetHandle(), true);
      for (auto it = graph.GetIncomingEdges(vertices[current]); it.IsValid(); ++it)
        visit(it.GetHandle(), false);
    }
    if (scratch.m_State[target] == 0)
      break;
    double amount = xiiMath::Infinity<double>();
    for (auto current = target; current != source; current = scratch.m_Order[current])
    {
      const auto edge    = scratch.m_PreviousEdges[current];
      const auto index   = graph.GetEdgeIndex(edge);
      const bool forward = graph.GetTarget(edge) == vertices[current];
      amount             = xiiMath::Min(amount, forward ? scratch.m_EdgeValues[index] - scratch.m_EdgeFlows[index] : scratch.m_EdgeFlows[index]);
    }
    if (!xiiMath::IsFinite(total + amount))
      return XII_FAILURE;
    total += amount;
    for (auto current = target; current != source; current = scratch.m_Order[current])
    {
      const auto edge    = scratch.m_PreviousEdges[current];
      const auto index   = graph.GetEdgeIndex(edge);
      const bool forward = graph.GetTarget(edge) == vertices[current];
      auto&      flow    = scratch.m_EdgeFlows[index];
      if (forward)
        flow = amount == scratch.m_EdgeValues[index] - flow ? scratch.m_EdgeValues[index] : flow + amount;
      else
        flow = amount == flow ? 0 : flow - amount;
    }
  }
  for (xiiUInt32 i = 0; i < edges.GetCount(); ++i)
  {
    out_flow.m_Edges.PushBack(edges[i]);
    out_flow.m_Flows.PushBack(scratch.m_EdgeFlows[i]);
  }
  out_flow.m_fValue = total;
  return XII_SUCCESS;
}
