#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>

xiiRttiMappedObjectFactory<xiiQtNode>       xiiQtNodeScene::s_NodeFactory;
xiiRttiMappedObjectFactory<xiiQtPin>        xiiQtNodeScene::s_PinFactory;
xiiRttiMappedObjectFactory<xiiQtConnection> xiiQtNodeScene::s_ConnectionFactory;
xiiVec2                                     xiiQtNodeScene::s_vLastMouseInteraction(0);

xiiQtNodeScene::xiiQtNodeScene(QObject* pParent) :
  QGraphicsScene(pParent)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(this, &QGraphicsScene::selectionChanged, this, &xiiQtNodeScene::OnSelectionChanged);
}

xiiQtNodeScene::~xiiQtNodeScene()
{
  disconnect(this, &QGraphicsScene::selectionChanged, this, &xiiQtNodeScene::OnSelectionChanged);

  Clear();

  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtNodeScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtNodeScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtNodeScene::PropertyEventsHandler, this));
  }
}

void xiiQtNodeScene::InitScene(const xiiDocumentNodeManager* pManager)
{
  XII_ASSERT_DEV(pManager != nullptr, "Invalid node manager.");

  m_pManager = pManager;

  m_pManager->m_NodeEvents.AddEventHandler(xiiMakeDelegate(&xiiQtNodeScene::NodeEventsHandler, this));
  m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtNodeScene::SelectionEventsHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtNodeScene::PropertyEventsHandler, this));

  // Create Nodes
  const auto& rootObjects = pManager->GetRootObject()->GetChildren();
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsNode(pObject))
    {
      CreateQtNode(pObject);
    }
  }
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsConnection(pObject))
    {
      CreateQtConnection(pObject);
    }
  }
}

const xiiDocument* xiiQtNodeScene::GetDocument() const
{
  return m_pManager->GetDocument();
}

const xiiDocumentNodeManager* xiiQtNodeScene::GetDocumentNodeManager() const
{
  return m_pManager;
}

xiiRttiMappedObjectFactory<xiiQtNode>& xiiQtNodeScene::GetNodeFactory()
{
  return s_NodeFactory;
}

xiiRttiMappedObjectFactory<xiiQtPin>& xiiQtNodeScene::GetPinFactory()
{
  return s_PinFactory;
}

xiiRttiMappedObjectFactory<xiiQtConnection>& xiiQtNodeScene::GetConnectionFactory()
{
  return s_ConnectionFactory;
}

void xiiQtNodeScene::SetConnectionStyle(xiiEnum<ConnectionStyle> style)
{
  m_ConnectionStyle = style;
  invalidate();
}

void xiiQtNodeScene::SetConnectionDecorationFlags(xiiBitflags<ConnectionDecorationFlags> flags)
{
  m_ConnectionDecorationFlags = flags;
  invalidate();
}

void xiiQtNodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  m_vMousePos             = xiiVec2(event->scenePos().x(), event->scenePos().y());
  s_vLastMouseInteraction = m_vMousePos;

  if (m_pTempConnection)
  {
    event->accept();

    xiiVec2 bestPos = m_vMousePos;

    // snap to the closest pin that we can connect to
    if (!m_ConnectablePins.IsEmpty())
    {
      const float fPinSize = m_ConnectablePins[0]->sceneBoundingRect().height();

      // this is also the threshold at which we snap to another position
      float fDistToBest = xiiMath::Square(fPinSize * 2.5f);

      for (auto pin : m_ConnectablePins)
      {
        const QPointF center = pin->sceneBoundingRect().center();
        const xiiVec2 pt     = xiiVec2(center.x(), center.y());
        const float   lenSqr = (pt - s_vLastMouseInteraction).GetLengthSquared();

        if (lenSqr < fDistToBest)
        {
          fDistToBest = lenSqr;
          bestPos     = pt;
        }
      }
    }

    if (m_pStartPin->GetPin()->GetType() == xiiPin::Type::Input)
    {
      m_pTempConnection->SetPosOut(QPointF(bestPos.x, bestPos.y));
    }
    else
    {
      m_pTempConnection->SetPosIn(QPointF(bestPos.x, bestPos.y));
    }
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);
}

void xiiQtNodeScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  switch (event->button())
  {
    case Qt::LeftButton:
    {
      QList<QGraphicsItem*> itemList = items(event->scenePos(), Qt::IntersectsItemBoundingRect);
      for (QGraphicsItem* item : itemList)
      {
        if (item->type() != Type::Pin)
          continue;

        event->accept();
        xiiQtPin* pPin    = static_cast<xiiQtPin*>(item);
        m_pStartPin       = pPin;
        m_pTempConnection = new xiiQtConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());

        if (pPin->GetPin()->GetType() == xiiPin::Type::Input)
        {
          m_pTempConnection->SetDirIn(pPin->GetPinDir());
          m_pTempConnection->SetDirOut(-pPin->GetPinDir());
        }
        else
        {
          m_pTempConnection->SetDirIn(-pPin->GetPinDir());
          m_pTempConnection->SetDirOut(pPin->GetPinDir());
        }

        MarkupConnectablePins(pPin);
        return;
      }
    }
    break;
    case Qt::RightButton:
    {
      event->accept();
      return;
    }

    default:
      break;
  }

  QGraphicsScene::mousePressEvent(event);
}

void xiiQtNodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();

    const bool    startWasInput = m_pStartPin->GetPin()->GetType() == xiiPin::Type::Input;
    const QPointF releasePos    = startWasInput ? m_pTempConnection->GetOutPos() : m_pTempConnection->GetInPos();

    QList<QGraphicsItem*> itemList = items(releasePos, Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem* item : itemList)
    {
      if (item->type() != Type::Pin)
        continue;

      xiiQtPin* pPin = static_cast<xiiQtPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const xiiPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const xiiPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ConnectPinsAction(*pSourcePin, *pTargetPin);
        break;
      }
    }

    delete m_pTempConnection;
    m_pTempConnection = nullptr;
    m_pStartPin       = nullptr;

    ResetConnectablePinMarkup();
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  xiiSet<const xiiDocumentObject*> moved;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetFlags().IsSet(xiiNodeFlags::Moved))
    {
      moved.Insert(it.Key());
      it.Value()->ResetFlags();
    }
  }

  if (!moved.IsEmpty())
  {
    xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Move Node");

    xiiStatus res;
    for (auto pObject : moved)
    {
      xiiMoveNodeCommand move;
      move.m_Object = pObject->GetGuid();
      auto pos      = m_Nodes[pObject]->pos();
      move.m_NewPos = xiiVec2(pos.x(), pos.y());
      res           = history->AddCommand(move);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Move node failed");
  }
}

void xiiQtNodeScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
{
  QTransform id;

  QGraphicsItem* pItem = itemAt(contextMenuEvent->scenePos(), id);
  int            iType = pItem != nullptr ? pItem->type() : -1;
  while (pItem && !(iType >= Type::Node && iType <= Type::Connection))
  {
    pItem = pItem->parentItem();
    iType = pItem != nullptr ? pItem->type() : -1;
  }

  QMenu menu;
  if (iType == Type::Pin)
  {
    xiiQtPin* pPin    = static_cast<xiiQtPin*>(pItem);
    QAction*  pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked) { DisconnectPinsAction(pPin); });

    pPin->ExtendContextMenu(menu);
  }
  else if (iType == Type::Node)
  {
    xiiQtNode* pNode = static_cast<xiiQtNode*>(pItem);

    // if we clicked on an unselected item, make it the only selected item
    if (!pNode->isSelected())
    {
      clearSelection();
      pNode->setSelected(true);
    }

    // Delete Node
    {
      QAction* pAction = new QAction("Remove", &menu);
      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked) { RemoveSelectedNodesAction(); });
    }

    pNode->ExtendContextMenu(menu);
  }
  else if (iType == Type::Connection)
  {
    xiiQtConnection* pConnection = static_cast<xiiQtConnection*>(pItem);
    QAction*         pAction     = new QAction("Delete Connection", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pConnection](bool bChecked) { DisconnectPinsAction(pConnection); });

    pConnection->ExtendContextMenu(menu);
  }
  else
  {
    OpenSearchMenu(contextMenuEvent->screenPos());
    return;
  }

  menu.exec(contextMenuEvent->screenPos());
}

void xiiQtNodeScene::keyPressEvent(QKeyEvent* event)
{
  QTransform     id;
  QGraphicsItem* pItem = itemAt(QPointF(m_vMousePos.x, m_vMousePos.y), id);
  if (pItem && pItem->type() == Type::Pin)
  {
    xiiQtPin* pin = static_cast<xiiQtPin*>(pItem);
    if (event->key() == Qt::Key_Delete)
    {
      DisconnectPinsAction(pin);
    }

    pin->keyPressEvent(event);
  }

  if (event->key() == Qt::Key_Delete)
  {
    RemoveSelectedNodesAction();
  }
  else if (event->key() == Qt::Key_Space)
  {
    OpenSearchMenu(QCursor::pos());
  }

  // Pass Shortcuts/KeyPresses up the chain again, so e.g. Ctrl+S work even if inside a window.
  if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
  {
    event->ignore();
  }
}

void xiiQtNodeScene::Clear()
{
  while (!m_Connections.IsEmpty())
  {
    DeleteQtConnection(m_Connections.GetIterator().Key());
  }

  while (!m_Nodes.IsEmpty())
  {
    DeleteQtNode(m_Nodes.GetIterator().Key());
  }
}

void xiiQtNodeScene::CreateQtNode(const xiiDocumentObject* pObject)
{
  xiiVec2 vPos = m_pManager->GetNodePos(pObject);

  xiiQtNode* pNode = s_NodeFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pNode == nullptr)
  {
    pNode = new xiiQtNode();
  }
  m_Nodes[pObject] = pNode;
  addItem(pNode);
  pNode->InitNode(m_pManager, pObject);
  pNode->setPos(vPos.x, vPos.y);

  pNode->ResetFlags();
}

void xiiQtNodeScene::DeleteQtNode(const xiiDocumentObject* pObject)
{
  xiiQtNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void xiiQtNodeScene::CreateQtConnection(const xiiDocumentObject* pObject)
{
  const xiiConnection& connection = m_pManager->GetConnection(pObject);
  const xiiPin&        pinSource  = connection.GetSourcePin();
  const xiiPin&        pinTarget  = connection.GetTargetPin();

  xiiQtNode* pSource = m_Nodes[pinSource.GetParent()];
  xiiQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  xiiQtPin*  pOutput = pSource->GetOutputPin(pinSource);
  xiiQtPin*  pInput  = pTarget->GetInputPin(pinTarget);
  XII_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  xiiQtConnection* pQtConnection = s_ConnectionFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new xiiQtConnection(nullptr);
  }

  addItem(pQtConnection);
  pQtConnection->InitConnection(pObject, &connection);
  pOutput->AddConnection(pQtConnection);
  pInput->AddConnection(pQtConnection);
  m_Connections[pObject] = pQtConnection;

  // Reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void xiiQtNodeScene::DeleteQtConnection(const xiiDocumentObject* pObject)
{
  xiiQtConnection* pQtConnection = m_Connections[pObject];
  m_Connections.Remove(pObject);

  const xiiConnection* pConnection = pQtConnection->GetConnection();
  XII_ASSERT_DEV(pConnection != nullptr, "No connection");

  const xiiPin& pinSource = pConnection->GetSourcePin();
  const xiiPin& pinTarget = pConnection->GetTargetPin();

  xiiQtNode* pSource = m_Nodes[pinSource.GetParent()];
  xiiQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  xiiQtPin*  pOutput = pSource->GetOutputPin(pinSource);
  xiiQtPin*  pInput  = pTarget->GetInputPin(pinTarget);
  XII_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);

  removeItem(pQtConnection);
  delete pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void xiiQtNodeScene::RecreateQtPins(const xiiDocumentObject* pObject)
{
  xiiQtNode* pNode = m_Nodes[pObject];
  pNode->CreatePins();
  pNode->UpdateState();
  pNode->UpdateGeometry();
}

void xiiQtNodeScene::CreateNodeObject(const xiiRTTI* pRtti)
{
  xiiCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Node");

  xiiStatus res;
  {
    xiiAddObjectCommand cmd;
    cmd.m_pType         = pRtti;
    cmd.m_NewObjectGuid = xiiUuid::MakeUuid();
    cmd.m_Index         = -1;

    res = history->AddCommand(cmd);
    if (res.m_Result.Succeeded())
    {
      xiiMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = m_vMousePos;
      res           = history->AddCommand(move);
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void xiiQtNodeScene::NodeEventsHandler(const xiiDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentNodeManagerEvent::Type::NodeMoved:
    {
      xiiVec2    vPos  = m_pManager->GetNodePos(e.m_pObject);
      xiiQtNode* pNode = m_Nodes[e.m_pObject];
      pNode->setPos(vPos.x, vPos.y);
    }
    break;
    case xiiDocumentNodeManagerEvent::Type::AfterPinsConnected:
      CreateQtConnection(e.m_pObject);
      break;

    case xiiDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
      DeleteQtConnection(e.m_pObject);
      break;

    case xiiDocumentNodeManagerEvent::Type::BeforePinsChanged:
      break;

    case xiiDocumentNodeManagerEvent::Type::AfterPinsChanged:
      RecreateQtPins(e.m_pObject);
      break;

    case xiiDocumentNodeManagerEvent::Type::AfterNodeAdded:
      CreateQtNode(e.m_pObject);
      break;

    case xiiDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
      DeleteQtNode(e.m_pObject);
      break;

    default:
      break;
  }
}

void xiiQtNodeScene::PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);
  if (it.IsValid())
  {
    it.Value()->ResetFlags();
    it.Value()->update();
  }
}

void xiiQtNodeScene::SelectionEventsHandler(const xiiSelectionManagerEvent& e)
{
  const xiiDeque<const xiiDocumentObject*>& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;

    clearSelection();

    QList<QGraphicsItem*> qSelection;
    for (const xiiDocumentObject* pObject : selection)
    {
      auto it = m_Nodes.Find(pObject);
      if (!it.IsValid())
        continue;

      it.Value()->setSelected(true);
    }
    m_bIgnoreSelectionChange = false;
  }

  bool bAnyPaintChanges = false;

  for (auto itCon : m_Connections)
  {
    auto pQtCon = itCon.Value();
    auto pCon   = pQtCon->GetConnection();

    const bool prev = pQtCon->m_bAdjacentNodeSelected;

    pQtCon->m_bAdjacentNodeSelected = false;

    for (const xiiDocumentObject* pObject : selection)
    {
      if (pCon->GetSourcePin().GetParent() == pObject || pCon->GetTargetPin().GetParent() == pObject)
      {
        pQtCon->m_bAdjacentNodeSelected = true;
        break;
      }
    }

    if (prev != pQtCon->m_bAdjacentNodeSelected)
    {
      bAnyPaintChanges = true;
    }
  }

  if (bAnyPaintChanges)
  {
    invalidate();
  }
}

void xiiQtNodeScene::GetSelectedNodes(xiiDeque<xiiQtNode*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == xiiQtNodeScene::Node)
    {
      xiiQtNode* pNode = static_cast<xiiQtNode*>(pItem);
      selection.PushBack(pNode);
    }
  }
}

void xiiQtNodeScene::MarkupConnectablePins(xiiQtPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const xiiRTTI* pConnectionType = m_pManager->GetConnectionType();

  const xiiPin* pSourcePin      = pQtSourcePin->GetPin();
  const bool    bConnectForward = pSourcePin->GetType() == xiiPin::Type::Output;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const xiiDocumentObject* pDocObject  = it.Key();
    xiiQtNode*               pTargetNode = it.Value();

    {
      auto pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        xiiQtPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);

        xiiDocumentNodeManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pConnectionType, *pSourcePin, *pin, res).IgnoreResult();
        else
          m_pManager->CanConnect(pConnectionType, *pin, *pSourcePin, res).IgnoreResult();

        if (res == xiiDocumentNodeManager::CanConnectResult::ConnectNever)
        {
          pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::CannotConnect);
        }
        else
        {
          m_ConnectablePins.PushBack(pQtTargetPin);

          if (res == xiiDocumentNodeManager::CanConnectResult::Connect1toN || res == xiiDocumentNodeManager::CanConnectResult::ConnectNtoN)
          {
            pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::CanAddConnection);
          }
          else
          {
            pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::CanReplaceConnection);
          }
        }
      }
    }

    {
      auto pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        xiiQtPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);
        pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::CannotConnectSameDirection);
      }
    }
  }
}

void xiiQtNodeScene::ResetConnectablePinMarkup()
{
  m_ConnectablePins.Clear();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const xiiDocumentObject* pDocObject  = it.Key();
    xiiQtNode*               pTargetNode = it.Value();

    for (auto& pin : m_pManager->GetInputPins(pDocObject))
    {
      xiiQtPin* pQtTargetPin = pTargetNode->GetInputPin(*pin);
      pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::None);
    }

    for (auto& pin : m_pManager->GetOutputPins(pDocObject))
    {
      xiiQtPin* pQtTargetPin = pTargetNode->GetOutputPin(*pin);
      pQtTargetPin->SetHighlightState(xiiQtPinHighlightState::None);
    }
  }
}

void xiiQtNodeScene::OpenSearchMenu(QPoint screenPos)
{
  QMenu                menu;
  xiiQtSearchableMenu* pSearchMenu = new xiiQtSearchableMenu(&menu);
  menu.addAction(pSearchMenu);

  connect(pSearchMenu, &xiiQtSearchableMenu::MenuItemTriggered, this, &xiiQtNodeScene::OnMenuItemTriggered);
  connect(pSearchMenu, &xiiQtSearchableMenu::MenuItemTriggered, this, [&menu]() { menu.close(); });

  xiiStringBuilder tmp;
  xiiStringBuilder sFullPath;

  xiiHybridArray<const xiiRTTI*, 32> types;
  m_pManager->GetCreateableTypes(types);

  for (const xiiRTTI* pRtti : types)
  {
    xiiStringView sCleanName = pRtti->GetTypeName();

    if (const char* szUnderscore = sCleanName.FindLastSubString("_"))
    {
      sCleanName.SetStartPosition(szUnderscore + 1);
    }

    if (const char* szBracket = sCleanName.FindLastSubString("<"))
    {
      sCleanName = xiiStringView(sCleanName.GetStartPointer(), szBracket);
    }

    sFullPath = m_pManager->GetTypeCategory(pRtti);

    if (sFullPath.IsEmpty())
    {
      if (auto pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
      {
        sFullPath = pAttr->GetCategory();
      }
    }

    sFullPath.AppendPath(sCleanName);

    pSearchMenu->AddItem(xiiTranslate(sCleanName.GetData(tmp)), sFullPath, QVariant::fromValue((void*)pRtti));
  }

  pSearchMenu->Finalize(m_sContextMenuSearchText);

  menu.exec(screenPos);

  m_sContextMenuSearchText = pSearchMenu->GetSearchText();
}

xiiStatus xiiQtNodeScene::RemoveNode(xiiQtNode* pNode)
{
  XII_SUCCEED_OR_RETURN(m_pManager->CanRemove(pNode->GetObject()));

  xiiRemoveNodeCommand cmd;
  cmd.m_Object = pNode->GetObject()->GetGuid();

  xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  return history->AddCommand(cmd);
}

void xiiQtNodeScene::RemoveSelectedNodesAction()
{
  xiiDeque<xiiQtNode*> selection;
  GetSelectedNodes(selection);

  if (selection.IsEmpty())
    return;

  xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Nodes");

  for (xiiQtNode* pNode : selection)
  {
    xiiStatus res = RemoveNode(pNode);

    if (res.m_Result.Failed())
    {
      history->CancelTransaction();

      xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to remove node");
      return;
    }
  }

  history->FinishTransaction();
}

void xiiQtNodeScene::ConnectPinsAction(const xiiPin& sourcePin, const xiiPin& targetPin)
{
  xiiDocumentNodeManager::CanConnectResult connect;
  xiiStatus                                res = m_pManager->CanConnect(m_pManager->GetConnectionType(), sourcePin, targetPin, connect);

  if (connect == xiiDocumentNodeManager::CanConnectResult::ConnectNever)
  {
    xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
    return;
  }

  xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Connect Pins");

  // disconnect everything from the source pin
  if (connect == xiiDocumentNodeManager::CanConnectResult::Connect1to1 || connect == xiiDocumentNodeManager::CanConnectResult::Connect1toN)
  {
    const xiiArrayPtr<const xiiConnection* const> connections = m_pManager->GetConnections(sourcePin);
    for (const xiiConnection* pConnection : connections)
    {
      res = xiiNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // disconnect everything from the target pin
  if (connect == xiiDocumentNodeManager::CanConnectResult::Connect1to1 || connect == xiiDocumentNodeManager::CanConnectResult::ConnectNto1)
  {
    const xiiArrayPtr<const xiiConnection* const> connections = m_pManager->GetConnections(targetPin);
    for (const xiiConnection* pConnection : connections)
    {
      res = xiiNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // connect the two pins
  {
    res = xiiNodeCommands::AddAndConnectCommand(history, m_pManager->GetConnectionType(), sourcePin, targetPin);
    if (res.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void xiiQtNodeScene::DisconnectPinsAction(xiiQtConnection* pConnection)
{
  xiiStatus res = m_pManager->CanDisconnect(pConnection->GetConnection());
  if (res.m_Result.Succeeded())
  {
    xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Disconnect Pins");

    res = xiiNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetConnection()->GetParent()->GetGuid());
    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
}

void xiiQtNodeScene::DisconnectPinsAction(xiiQtPin* pPin)
{
  xiiCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  xiiStatus res = xiiStatus(XII_SUCCESS);
  for (xiiQtConnection* pConnection : pPin->GetConnections())
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void xiiQtNodeScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  const xiiRTTI* pRtti = static_cast<const xiiRTTI*>(variant.value<void*>());

  CreateNodeObject(pRtti);
}

void xiiQtNodeScene::OnSelectionChanged()
{
  xiiCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  if (pHistory->IsInUndoRedo() || pHistory->IsInTransaction())
    return;

  m_Selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == xiiQtNodeScene::Node)
    {
      xiiQtNode* pNode = static_cast<xiiQtNode*>(pItem);
      m_Selection.PushBack(pNode->GetObject());
    }
    else if (pItem->type() == xiiQtNodeScene::Connection)
    {
      xiiQtConnection* pConnection = static_cast<xiiQtConnection*>(pItem);
      m_Selection.PushBack(pConnection->GetObject());
    }
  }

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;
    m_pManager->GetDocument()->GetSelectionManager()->SetSelection(m_Selection);
    m_bIgnoreSelectionChange = false;
  }
}
