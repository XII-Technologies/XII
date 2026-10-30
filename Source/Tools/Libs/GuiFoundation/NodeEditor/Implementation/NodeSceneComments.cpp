/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/CommentBlock.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QColorDialog>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMenu>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

void xiiQtNodeScene::SyncCommentBlocks()
{
  const bool ignored       = m_bIgnoreSelectionChange;
  m_bIgnoreSelectionChange = true;
  const auto& blocks       = m_pManager->GetCommentBlocks();
  for (auto it = m_CommentBlocks.GetIterator(); it.IsValid();)
  {
    if (!blocks.Contains(it.Key()))
    {
      delete it.Value();
      it = m_CommentBlocks.Remove(it);
    }
    else
      ++it;
  }
  for (const auto& block : blocks)
  {
    auto                    existing = m_CommentBlocks.Find(block.Key());
    xiiQtGraphCommentBlock* item     = existing.IsValid() ? existing.Value() : nullptr;
    if (item == nullptr)
    {
      item = new xiiQtGraphCommentBlock(*this);
      addItem(item);
      m_CommentBlocks.Insert(block.Key(), item);
    }
    item->SetBlock(block.Value());
  }
  m_bIgnoreSelectionChange = ignored;
}

xiiStatus xiiQtNodeScene::SetCommentBlock(const xiiGraphCommentBlock& block, bool bMoveContainedNodes)
{
  if (!m_pManager)
    return xiiStatus("The graph scene has no document.");
  auto* history = GetDocument()->GetCommandHistory();
  if (history->IsInTransaction() || history->IsInUndoRedo() || m_pTempConnection)
    return xiiStatus("Finish the current graph edit first.");
  const auto old = m_pManager->GetCommentBlocks().Find(block.m_Id);
  if (old.IsValid() && old.Value().ToVariant() == block.ToVariant())
    return XII_SUCCESS;
  xiiDynamicArray<const xiiDocumentObject*> contained;
  xiiVec2                                   delta = xiiVec2::MakeZero();
  if (old.IsValid() && bMoveContainedNodes)
  {
    const auto& before = old.Value();
    delta              = block.m_vPosition - before.m_vPosition;
    const QRectF bounds(before.m_vPosition.x, before.m_vPosition.y, before.m_vSize.x, before.m_vSize.y);
    for (const auto& node : m_Nodes)
      if (bounds.contains(node.Value()->sceneBoundingRect()))
        contained.PushBack(node.Key());
  }
  history->StartTransaction("Edit Comment Block");
  xiiSetGraphCommentBlockCommand command;
  command.m_Id    = block.m_Id;
  command.m_Value = block.ToVariant();
  auto status     = history->AddCommand(command);
  for (const auto* node : contained)
  {
    if (status.Failed())
      break;
    xiiMoveNodeCommand move;
    move.m_Object = node->GetGuid();
    move.m_NewPos = m_pManager->GetNodePos(node) + delta;
    status        = history->AddCommand(move);
  }
  if (status.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
  for (const auto* node : contained)
    m_Nodes.Find(node).Value()->ResetFlags();
  return status;
}

xiiStatus xiiQtNodeScene::RemoveCommentBlock(const xiiUuid& id)
{
  if (!m_pManager || !m_pManager->GetCommentBlocks().Contains(id))
    return xiiStatus("The graph comment block no longer exists.");
  auto* history = GetDocument()->GetCommandHistory();
  if (history->IsInTransaction() || history->IsInUndoRedo())
    return xiiStatus("Finish the current graph edit first.");
  history->StartTransaction("Remove Comment Block");
  xiiSetGraphCommentBlockCommand command;
  command.m_Id      = id;
  const auto status = history->AddCommand(command);
  if (status.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
  return status;
}

xiiStatus xiiQtNodeScene::CreateCommentBlock(xiiStringView sText)
{
  xiiGraphCommentBlock block;
  block.m_Id        = xiiUuid::MakeUuid();
  block.m_sText     = sText;
  block.m_vPosition = m_vMousePos;
  QRectF bounds;
  for (const auto& node : m_Nodes)
    if (node.Value()->isSelected())
      bounds = bounds.united(node.Value()->mapRectToScene(node.Value()->boundingRect().united(node.Value()->childrenBoundingRect())));
  if (!bounds.isEmpty())
  {
    bounds.adjust(-32, -72, 32, 32);
    block.m_vPosition = xiiVec2(bounds.x(), bounds.y());
    block.m_vSize     = xiiVec2(xiiMath::Max(160.0, bounds.width()), xiiMath::Max(80.0, bounds.height()));
  }
  return SetCommentBlock(block);
}

xiiStatus xiiQtNodeScene::FitCommentBlock(const xiiUuid& id)
{
  const auto existing = m_pManager->GetCommentBlocks().Find(id);
  if (!existing.IsValid())
    return xiiStatus("The graph comment block no longer exists.");
  auto         block = existing.Value();
  const QRectF old(block.m_vPosition.x, block.m_vPosition.y, block.m_vSize.x, block.m_vSize.y);
  QRectF       bounds;
  for (const auto& node : m_Nodes)
    if (old.contains(node.Value()->sceneBoundingRect()))
      bounds = bounds.united(node.Value()->mapRectToScene(node.Value()->boundingRect().united(node.Value()->childrenBoundingRect())));
  if (bounds.isEmpty())
    return XII_SUCCESS;
  bounds.adjust(-32, -72, 32, 32);
  block.m_vPosition = xiiVec2(bounds.x(), bounds.y());
  block.m_vSize     = xiiVec2(xiiMath::Max(160.0, bounds.width()), xiiMath::Max(80.0, bounds.height()));
  return SetCommentBlock(block);
}

void xiiQtNodeScene::AddCommentBlockActions(QMenu& menu, const xiiUuid& id)
{
  const bool idle = m_pManager && !GetDocument()->GetCommandHistory()->IsInTransaction() && !GetDocument()->GetCommandHistory()->IsInUndoRedo() && !m_pTempConnection;
  const auto add  = [&](const char* text, auto callback) {
    auto* action = menu.addAction(text);
    action->setEnabled(idle);
    connect(action, &QAction::triggered, this, callback);
  };
  if (!id.IsValid())
  {
    add("Add Organizational Comment Block...", [this]() {
      bool       accepted = false;
      const auto text     = QInputDialog::getMultiLineText(views().isEmpty() ? nullptr : views()[0], "Comment Block", "Comment", "Comment", &accepted);
      if (accepted)
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(CreateCommentBlock(xiiStringView(text.toUtf8().constData())), "Could not create comment block.");
    });
    return;
  }
  add("Edit Block Comment...", [this, id]() {
    const auto existing = m_pManager->GetCommentBlocks().Find(id);
    if (!existing.IsValid())
      return;
    auto       block    = existing.Value();
    bool       accepted = false;
    const auto text     = QInputDialog::getMultiLineText(views().isEmpty() ? nullptr : views()[0], "Comment Block", "Comment", xiiMakeQString(block.m_sText), &accepted);
    if (accepted)
    {
      block.m_sText = text.toUtf8().constData();
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(SetCommentBlock(block), "Could not edit comment block.");
    }
  });
  add("Block Color...", [this, id]() {
    const auto existing = m_pManager->GetCommentBlocks().Find(id);
    if (!existing.IsValid())
      return;
    auto                  block = existing.Value();
    const xiiColorGammaUB color(block.m_Color);
    const auto            selected = QColorDialog::getColor(QColor(color.r, color.g, color.b), views().isEmpty() ? nullptr : views()[0], "Comment Block Color");
    if (selected.isValid())
    {
      block.m_Color = xiiColorGammaUB(selected.red(), selected.green(), selected.blue());
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(SetCommentBlock(block), "Could not change block color.");
    }
  });
  add("Fit Block to Contents", [this, id]() { xiiQtUiServices::GetSingleton()->MessageBoxStatus(FitCommentBlock(id), "Could not fit comment block."); });
  add("Duplicate Comment Block", [this, id]() {
    const auto existing = m_pManager->GetCommentBlocks().Find(id);
    if (!existing.IsValid())
      return;
    auto block = existing.Value();
    block.m_Id = xiiUuid::MakeUuid();
    block.m_vPosition += xiiVec2(24);
    xiiQtUiServices::GetSingleton()->MessageBoxStatus(SetCommentBlock(block), "Could not duplicate comment block.");
  });
  add("Remove Comment Block", [this, id]() { xiiQtUiServices::GetSingleton()->MessageBoxStatus(RemoveCommentBlock(id), "Could not remove comment block."); });
}
