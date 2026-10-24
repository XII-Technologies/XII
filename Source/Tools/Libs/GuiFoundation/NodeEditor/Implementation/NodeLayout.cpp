/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Core/Graph/GraphAlgorithms.h>
#include <GuiFoundation/NodeEditor/NodeLayout.h>

namespace
{
  constexpr double s_fColumnGap    = 120.0;
  constexpr double s_fRowGap       = 60.0;
  constexpr double s_fComponentGap = 120.0;

  struct LayoutNode
  {
    xiiUInt32                  m_uiRank = 0;
    double                     m_fY     = 0;
    double                     m_fOrder = 0;
    xiiDynamicArray<xiiUInt32> m_Incoming;
    xiiDynamicArray<xiiUInt32> m_Outgoing;
  };

  // Project desired positions onto non-overlapping intervals, keeping their order. Pooling
  // adjacent violations avoids the downward drift of a greedy collision-resolution pass.
  void CompactLayer(xiiArrayPtr<const xiiUInt32> layer, xiiArrayPtr<const QRectF> bounds, xiiDynamicArray<LayoutNode>& nodes)
  {
    struct Block
    {
      XII_DECLARE_POD_TYPE();
      xiiUInt32 m_uiBegin;
      xiiUInt32 m_uiEnd;
      double    m_fSum;
      double    Mean() const { return m_fSum / (m_uiEnd - m_uiBegin + 1); }
    };
    xiiDynamicArray<Block>  blocks;
    xiiDynamicArray<double> offsets;
    double                  offset = 0;
    for (xiiUInt32 i = 0; i < layer.GetCount(); ++i)
    {
      offsets.PushBack(offset);
      blocks.PushBack({i, i, nodes[layer[i]].m_fY - offset});
      while (blocks.GetCount() > 1 && blocks[blocks.GetCount() - 2].Mean() > blocks.PeekBack().Mean())
      {
        const auto last = blocks.PeekBack();
        blocks.PopBack();
        blocks.PeekBack().m_uiEnd = last.m_uiEnd;
        blocks.PeekBack().m_fSum += last.m_fSum;
      }
      offset += bounds[layer[i]].height() + s_fRowGap;
    }
    for (const auto& block : blocks)
      for (xiiUInt32 i = block.m_uiBegin; i <= block.m_uiEnd; ++i)
        nodes[layer[i]].m_fY = block.Mean() + offsets[i];
  }
} // namespace

xiiResult xiiNodeLayout::Compute(xiiArrayPtr<const QRectF> bounds, xiiArrayPtr<const xiiNodeLayoutConnection> connections, xiiDynamicArray<xiiVec2>& out_positions)
{
  out_positions.Clear();
  if (bounds.IsEmpty())
    return XII_SUCCESS;

  QRectF                                original;
  xiiGraph                              graph;
  xiiDynamicArray<xiiGraphVertexHandle> vertices;
  xiiDynamicArray<LayoutNode>           nodes;
  nodes.SetCount(bounds.GetCount());
  for (const auto& rect : bounds)
  {
    if (!xiiMath::IsFinite(rect.x()) || !xiiMath::IsFinite(rect.y()) || !xiiMath::IsFinite(rect.width()) || !xiiMath::IsFinite(rect.height()) || rect.width() <= 0 || rect.height() <= 0)
      return XII_FAILURE;
    original = original.united(rect);
    xiiGraphVertexHandle vertex;
    XII_SUCCEED_OR_RETURN(graph.TryAddVertex(vertex));
    vertices.PushBack(vertex);
  }
  for (xiiUInt32 i = 0; i < connections.GetCount(); ++i)
  {
    const auto& edge = connections[i];
    if (edge.m_uiSource >= bounds.GetCount() || edge.m_uiTarget >= bounds.GetCount() || !xiiMath::IsFinite(edge.m_fSourceY) || !xiiMath::IsFinite(edge.m_fTargetY))
      return XII_FAILURE;
    xiiGraphEdgeHandle handle;
    XII_SUCCEED_OR_RETURN(graph.TryAddEdge(vertices[edge.m_uiSource], vertices[edge.m_uiTarget], handle));
    nodes[edge.m_uiSource].m_Outgoing.PushBack(i);
    nodes[edge.m_uiTarget].m_Incoming.PushBack(i);
  }

  // Collapse feedback loops before assigning longest-path ranks. All other connections
  // advance at least one column; a cycle cannot strand the rest of the graph.
  xiiGraphComponents strong, weak;
  xiiGraphAlgorithms::StronglyConnectedComponents(graph, strong);
  xiiGraphAlgorithms::WeaklyConnectedComponents(graph, weak);
  xiiDynamicArray<xiiUInt32> component;
  component.SetCount(bounds.GetCount());
  xiiGraph                              dag;
  xiiDynamicArray<xiiGraphVertexHandle> groups;
  for (xiiUInt32 c = 0; c < strong.GetCount(); ++c)
  {
    xiiGraphVertexHandle group;
    XII_SUCCEED_OR_RETURN(dag.TryAddVertex(group));
    groups.PushBack(group);
    for (auto vertex : strong.GetComponent(c))
      component[graph.GetVertexIndex(vertex)] = c;
  }
  for (const auto& edge : connections)
    if (component[edge.m_uiSource] != component[edge.m_uiTarget])
    {
      xiiGraphEdgeHandle handle;
      XII_SUCCEED_OR_RETURN(dag.TryAddEdge(groups[component[edge.m_uiSource]], groups[component[edge.m_uiTarget]], handle));
    }
  xiiDynamicArray<xiiGraphVertexHandle> order;
  XII_SUCCEED_OR_RETURN(xiiGraphAlgorithms::TopologicalSort(dag, order));
  xiiDynamicArray<xiiUInt32> ranks;
  ranks.SetCount(strong.GetCount(), 0);
  for (auto group : order)
    for (auto edge = dag.GetOutgoingEdges(group); edge.IsValid(); ++edge)
    {
      auto& rank = ranks[dag.GetVertexIndex(dag.GetTarget(edge.GetHandle()))];
      rank       = xiiMath::Max(rank, ranks[dag.GetVertexIndex(group)] + 1);
    }
  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
    nodes[i].m_uiRank = ranks[component[i]];

  // Component order is based on persistent input identity, not traversal or current positions.
  xiiDynamicArray<xiiDynamicArray<xiiUInt32>> components;
  for (xiiUInt32 c = 0; c < weak.GetCount(); ++c)
  {
    auto& members = components.ExpandAndGetRef();
    for (auto vertex : weak.GetComponent(c))
      members.PushBack(graph.GetVertexIndex(vertex));
    members.Sort();
  }
  components.Sort([](const auto& a, const auto& b) { return a[0] < b[0]; });
  xiiDynamicArray<xiiVec2> positions;
  positions.SetCount(bounds.GetCount());
  xiiDynamicArray<QSizeF> componentSizes;
  for (const auto& members : components)
  {
    xiiUInt32 maxRank = 0;
    for (auto index : members)
      maxRank = xiiMath::Max(maxRank, nodes[index].m_uiRank);
    xiiDynamicArray<xiiDynamicArray<xiiUInt32>> layers;
    layers.SetCount(maxRank + 1);
    for (auto index : members)
      layers[nodes[index].m_uiRank].PushBack(index);
    for (const auto& layer : layers)
    {
      double y = 0;
      for (auto index : layer)
      {
        nodes[index].m_fY = y;
        y += bounds[index].height() + s_fRowGap;
      }
    }

    // Alternating barycentric sweeps untangle branches. Pin offsets, rather than node
    // centers, align connected rows even when nodes have very different pin counts.
    auto sweep = [&](bool forward, bool reorder) {
      for (xiiUInt32 step = 0; step < layers.GetCount(); ++step)
      {
        auto& layer = layers[forward ? step : layers.GetCount() - 1 - step];
        for (auto index : layer)
        {
          double      desired = 0;
          xiiUInt32   count   = 0;
          const auto& edges   = forward ? nodes[index].m_Incoming : nodes[index].m_Outgoing;
          for (auto edgeIndex : edges)
          {
            const auto& edge     = connections[edgeIndex];
            auto        neighbor = forward ? edge.m_uiSource : edge.m_uiTarget;
            if (nodes[neighbor].m_uiRank == nodes[index].m_uiRank)
              continue;
            desired += nodes[neighbor].m_fY + (forward ? edge.m_fSourceY - edge.m_fTargetY : edge.m_fTargetY - edge.m_fSourceY);
            ++count;
          }
          nodes[index].m_fOrder = count ? desired / count : nodes[index].m_fY;
        }
        if (reorder)
          layer.Sort([&](xiiUInt32 a, xiiUInt32 b) {
            return nodes[a].m_fOrder == nodes[b].m_fOrder ? a < b : nodes[a].m_fOrder < nodes[b].m_fOrder;
          });
        for (auto index : layer)
          nodes[index].m_fY = nodes[index].m_fOrder;
        CompactLayer(layer, bounds, nodes);
      }
    };
    for (xiiUInt32 iteration = 0; iteration < 8; ++iteration)
    {
      sweep(true, true);
      sweep(false, true);
    }
    for (xiiUInt32 iteration = 0; iteration < 4; ++iteration)
    {
      sweep(true, false);
      sweep(false, false);
    }
    double top    = xiiMath::MaxValue<double>();
    double bottom = -xiiMath::MaxValue<double>();
    for (auto index : members)
    {
      top    = xiiMath::Min(top, nodes[index].m_fY);
      bottom = xiiMath::Max(bottom, nodes[index].m_fY + bounds[index].height());
    }
    double x = 0;
    for (const auto& layer : layers)
    {
      double width = 0;
      for (auto index : layer)
        width = xiiMath::Max(width, bounds[index].width());
      for (auto index : layer)
        positions[index] = xiiVec2(static_cast<float>(x + (width - bounds[index].width()) * 0.5), static_cast<float>(nodes[index].m_fY - top));
      x += width + s_fColumnGap;
    }
    componentSizes.PushBack(QSizeF(x - s_fColumnGap, bottom - top));
  }

  // Shelf-pack disconnected groups, including isolated nodes, rather than making one
  // arbitrarily tall column. Large groups establish the visual hierarchy first.
  double                     area = 0, widest = 0;
  xiiDynamicArray<xiiUInt32> packingOrder;
  for (xiiUInt32 i = 0; i < components.GetCount(); ++i)
  {
    const auto size = componentSizes[i];
    area += (size.width() + s_fComponentGap) * (size.height() + s_fComponentGap);
    widest = xiiMath::Max(widest, size.width());
    packingOrder.PushBack(i);
  }
  packingOrder.Sort([&](xiiUInt32 a, xiiUInt32 b) {
    const double first  = componentSizes[a].width() * componentSizes[a].height();
    const double second = componentSizes[b].width() * componentSizes[b].height();
    return first == second ? a < b : first > second;
  });
  const double shelfWidth = xiiMath::Max(widest, xiiMath::Sqrt(area) * 1.6);
  double       x = 0, y = 0, shelfHeight = 0;
  for (auto group : packingOrder)
  {
    const auto size = componentSizes[group];
    if (x > 0 && x + size.width() > shelfWidth)
    {
      x = 0;
      y += shelfHeight + s_fComponentGap;
      shelfHeight = 0;
    }
    const xiiVec2 offset(static_cast<float>(original.left() + x), static_cast<float>(original.top() + y));
    for (auto index : components[group])
      positions[index] += offset;
    shelfHeight = xiiMath::Max(shelfHeight, size.height());
    x += size.width() + s_fComponentGap;
  }
  out_positions = std::move(positions);
  return XII_SUCCESS;
}
