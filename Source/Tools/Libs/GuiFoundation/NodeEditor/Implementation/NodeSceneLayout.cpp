/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeLayout.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

void xiiQtNodeScene::AddArrangementActions(QMenu& menu)
{
  xiiUInt32 selected = 0;
  for (auto node : m_Nodes)
    if (node.Value()->isSelected())
      ++selected;
  const bool idle = m_pManager && !m_pTempConnection && !GetDocument()->GetCommandHistory()->IsInTransaction();
  auto       add  = [&](QMenu& target, const char* label, NodeArrangement operation, xiiUInt32 minimum, bool whole = false) {
    auto* action = target.addAction(label);
    action->setEnabled(idle && (whole ? m_Nodes.GetCount() : selected) >= minimum);
    connect(action, &QAction::triggered, this, [this, operation, whole]() {
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(ArrangeNodes(operation, whole), "Could not arrange nodes.");
    });
  };
  add(menu, "Format Graph\tCtrl+Shift+L", NodeArrangement::Format, 2, true);
  add(menu, "Format Selection\tCtrl+L", NodeArrangement::Format, 2);
  auto* align = menu.addMenu("Align Nodes");
  align->setEnabled(idle && selected >= 2);
  add(*align, "Left", NodeArrangement::AlignLeft, 2);
  add(*align, "Center", NodeArrangement::AlignHorizontalCenter, 2);
  add(*align, "Right", NodeArrangement::AlignRight, 2);
  align->addSeparator();
  add(*align, "Top", NodeArrangement::AlignTop, 2);
  add(*align, "Middle", NodeArrangement::AlignVerticalCenter, 2);
  add(*align, "Bottom", NodeArrangement::AlignBottom, 2);
  auto* distribute = menu.addMenu("Distribute Nodes");
  distribute->setEnabled(idle && selected >= 3);
  add(*distribute, "Horizontally", NodeArrangement::DistributeHorizontally, 3);
  add(*distribute, "Vertically", NodeArrangement::DistributeVertically, 3);
}

xiiStatus xiiQtNodeScene::ArrangeNodes(NodeArrangement arrangement, bool bWholeGraph)
{
  if (!m_pManager)
    return xiiStatus("The node scene has no document.");
  auto* history = GetDocument()->GetCommandHistory();
  if (history->IsInTransaction() || history->IsInUndoRedo() || m_pTempConnection)
    return xiiStatus("Finish the current graph edit before arranging nodes.");
  if (bWholeGraph && arrangement != NodeArrangement::Format)
    return xiiStatus("Alignment and distribution require a node selection.");

  xiiDynamicArray<xiiQtNode*> nodes;
  for (auto node : m_Nodes)
    if (bWholeGraph || node.Value()->isSelected())
      nodes.PushBack(node.Value());
  nodes.Sort([](const auto* a, const auto* b) { return a->GetObject()->GetGuid() < b->GetObject()->GetGuid(); });
  const bool distribute = arrangement == NodeArrangement::DistributeHorizontally || arrangement == NodeArrangement::DistributeVertically;
  if (nodes.GetCount() < (distribute ? 3U : 2U))
    return xiiStatus(XII_SUCCESS);

  // Titles may be pending a paint after a property/connection change. Layout must not
  // depend on whether the viewport has already repainted those nodes.
  for (auto node : m_Nodes)
  {
    node.Value()->UpdateState();
    node.Value()->UpdateGeometry();
  }

  xiiDynamicArray<QRectF>  bounds;
  xiiDynamicArray<xiiVec2> positions;
  QRectF                   total;
  for (auto* node : nodes)
  {
    // Include titles and pin labels, but not the cosmetic drop shadow.
    auto rect = node->mapRectToScene(node->boundingRect().united(node->childrenBoundingRect()));
    bounds.PushBack(rect);
    total = total.united(rect);
    positions.PushBack(xiiVec2(static_cast<float>(rect.x()), static_cast<float>(rect.y())));
  }
  const char* description = "Align Nodes";
  if (arrangement == NodeArrangement::Format)
  {
    description = bWholeGraph ? "Format Graph" : "Format Selected Nodes";
    xiiMap<const xiiDocumentObject*, xiiUInt32> indices;
    for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
      indices.Insert(nodes[i]->GetObject(), i);
    xiiDynamicArray<xiiNodeLayoutConnection> connections;
    for (auto connection : m_Connections)
    {
      const auto* native = connection.Value()->GetConnection();
      xiiUInt32   source, target;
      if (indices.TryGetValue(native->GetSourcePin().GetParent(), source) && indices.TryGetValue(native->GetTargetPin().GetParent(), target))
        connections.PushBack({source, target, connection.Value()->GetOutPos().y() - bounds[source].top(), connection.Value()->GetInPos().y() - bounds[target].top()});
    }
    // Stable edge summation also keeps repeated formatting independent of allocation order.
    connections.Sort([](const auto& a, const auto& b) {
      if (a.m_uiSource != b.m_uiSource)
        return a.m_uiSource < b.m_uiSource;
      if (a.m_uiTarget != b.m_uiTarget)
        return a.m_uiTarget < b.m_uiTarget;
      if (a.m_fSourceY != b.m_fSourceY)
        return a.m_fSourceY < b.m_fSourceY;
      return a.m_fTargetY < b.m_fTargetY;
    });
    if (xiiNodeLayout::Compute(bounds, connections, positions).Failed())
      return xiiStatus("Could not compute node layout: invalid node geometry or connectivity.");
    if (!bWholeGraph)
    {
      // Keep boundary wires coherent without allowing a far-away neighbor to drag the
      // selection across the canvas. Median offsets balance several external consumers.
      xiiDynamicArray<double> xOffsets, yOffsets;
      for (auto connection : m_Connections)
      {
        const auto* native = connection.Value()->GetConnection();
        xiiUInt32   source, target;
        const bool  hasSource = indices.TryGetValue(native->GetSourcePin().GetParent(), source);
        const bool  hasTarget = indices.TryGetValue(native->GetTargetPin().GetParent(), target);
        if (hasSource == hasTarget)
          continue;
        if (hasTarget)
        {
          xOffsets.PushBack(connection.Value()->GetOutPos().x() + xiiNodeLayout::HorizontalSpacing - positions[target].x);
          yOffsets.PushBack(connection.Value()->GetOutPos().y() - positions[target].y - (connection.Value()->GetInPos().y() - bounds[target].top()));
        }
        else
        {
          xOffsets.PushBack(connection.Value()->GetInPos().x() - xiiNodeLayout::HorizontalSpacing - positions[source].x - bounds[source].width());
          yOffsets.PushBack(connection.Value()->GetInPos().y() - positions[source].y - (connection.Value()->GetOutPos().y() - bounds[source].top()));
        }
      }
      if (!xOffsets.IsEmpty())
      {
        xOffsets.Sort();
        yOffsets.Sort();
        const double x = xOffsets[xOffsets.GetCount() / 2];
        const double y = yOffsets[yOffsets.GetCount() / 2];
        // Ignore distant anchors instead of clamping a step toward them: clamped steps
        // would make each repeated formatting invocation creep across the canvas.
        const xiiVec2 offset(static_cast<float>(xiiMath::Abs(x) <= 240 ? x : 0), static_cast<float>(xiiMath::Abs(y) <= 180 ? y : 0));
        for (auto& position : positions)
          position += offset;
      }

      // Test actual rectangles, including gaps inside the group. Candidate translations
      // clear an obstacle on each of its four sides; choose the nearest valid placement.
      QRectF formatted;
      for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
        formatted = formatted.united(QRectF(QPointF(positions[i].x, positions[i].y), bounds[i].size()));
      xiiDynamicArray<QRectF>  obstacles;
      xiiDynamicArray<QPointF> candidates;
      candidates.PushBack(QPointF());
      for (auto node : m_Nodes)
      {
        if (node.Value()->isSelected())
          continue;
        const auto fixed = node.Value()->mapRectToScene(node.Value()->boundingRect().united(node.Value()->childrenBoundingRect())).adjusted(-30, -30, 30, 30);
        obstacles.PushBack(fixed);
        candidates.PushBack(QPointF(fixed.right() - formatted.left() + 1, 0));
        candidates.PushBack(QPointF(fixed.left() - formatted.right() - 1, 0));
        candidates.PushBack(QPointF(0, fixed.bottom() - formatted.top() + 1));
        candidates.PushBack(QPointF(0, fixed.top() - formatted.bottom() - 1));
      }
      candidates.Sort([](const QPointF& a, const QPointF& b) {
        const double first = QPointF::dotProduct(a, a), second = QPointF::dotProduct(b, b);
        if (first != second)
          return first < second;
        if (a.y() != b.y())
          return a.y() < b.y();
        return a.x() < b.x();
      });
      for (const auto& offset : candidates)
      {
        bool collision = false;
        for (xiiUInt32 i = 0; i < nodes.GetCount() && !collision; ++i)
        {
          const QRectF rect(QPointF(positions[i].x, positions[i].y) + offset, bounds[i].size());
          for (const auto& fixed : obstacles)
            if (rect.intersects(fixed))
            {
              collision = true;
              break;
            }
        }
        if (!collision)
        {
          for (auto& position : positions)
            position += xiiVec2(static_cast<float>(offset.x()), static_cast<float>(offset.y()));
          break;
        }
      }
    }
  }
  else if (distribute)
  {
    description                           = "Distribute Nodes";
    const bool                 horizontal = arrangement == NodeArrangement::DistributeHorizontally;
    xiiDynamicArray<xiiUInt32> order;
    double                     occupied = 0;
    for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
    {
      order.PushBack(i);
      occupied += horizontal ? bounds[i].width() : bounds[i].height();
    }
    order.Sort([&](xiiUInt32 a, xiiUInt32 b) {
      const double first  = horizontal ? bounds[a].left() : bounds[a].top();
      const double second = horizontal ? bounds[b].left() : bounds[b].top();
      return first == second ? a < b : first < second;
    });
    const double gap    = xiiMath::Max(xiiNodeLayout::VerticalSpacing, ((horizontal ? total.width() : total.height()) - occupied) / (nodes.GetCount() - 1));
    double       cursor = horizontal ? total.left() : total.top();
    for (auto i : order)
    {
      if (horizontal)
        positions[i].x = static_cast<float>(cursor);
      else
        positions[i].y = static_cast<float>(cursor);
      cursor += (horizontal ? bounds[i].width() : bounds[i].height()) + gap;
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
    {
      switch (arrangement)
      {
        case NodeArrangement::AlignLeft:
          positions[i].x = static_cast<float>(total.left());
          break;
        case NodeArrangement::AlignHorizontalCenter:
          positions[i].x = static_cast<float>(total.center().x() - bounds[i].width() / 2);
          break;
        case NodeArrangement::AlignRight:
          positions[i].x = static_cast<float>(total.right() - bounds[i].width());
          break;
        case NodeArrangement::AlignTop:
          positions[i].y = static_cast<float>(total.top());
          break;
        case NodeArrangement::AlignVerticalCenter:
          positions[i].y = static_cast<float>(total.center().y() - bounds[i].height() / 2);
          break;
        case NodeArrangement::AlignBottom:
          positions[i].y = static_cast<float>(total.bottom() - bounds[i].height());
          break;
        default:
          return xiiStatus("Unknown node arrangement.");
      }
    }
  }

  xiiDynamicArray<xiiVec2> destinations;
  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    const auto    point = nodes[i]->pos() + QPointF(positions[i].x, positions[i].y) - bounds[i].topLeft();
    const xiiVec2 destination(static_cast<float>(point.x()), static_cast<float>(point.y()));
    if (!destination.IsValid())
      return xiiStatus("Node layout produced an invalid position.");
    if (!destination.IsEqual(m_pManager->GetNodePos(nodes[i]->GetObject()), 0.01f))
    {
      XII_SUCCEED_OR_RETURN(m_pManager->CanMoveNode(nodes[i]->GetObject(), destination));
    }
    destinations.PushBack(destination);
  }
  bool started = false;
  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    xiiMoveNodeCommand command;
    command.m_Object = nodes[i]->GetObject()->GetGuid();
    command.m_NewPos = destinations[i];
    if (command.m_NewPos.IsEqual(m_pManager->GetNodePos(nodes[i]->GetObject()), 0.01f))
      continue;
    if (!started)
    {
      history->StartTransaction(description);
      started = true;
    }
    const auto result = history->AddCommand(command);
    if (result.Failed())
    {
      history->CancelTransaction();
      return result;
    }
  }
  if (started)
    history->FinishTransaction();
  if (bWholeGraph)
  {
    QRectF formatted;
    for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
      formatted = formatted.united(QRectF(QPointF(positions[i].x, positions[i].y), bounds[i].size()));
    Q_EMIT GraphFormatted(formatted);
  }
  return xiiStatus(XII_SUCCESS);
}
