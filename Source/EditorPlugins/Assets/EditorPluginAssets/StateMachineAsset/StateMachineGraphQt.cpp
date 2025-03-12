#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <Foundation/Math/ColorScheme.h>

xiiQtStateMachinePin::xiiQtStateMachinePin() = default;

void xiiQtStateMachinePin::SetPin(const xiiPin& pin)
{
  xiiQtPin::SetPin(pin);

  constexpr int padding = 3;

  m_pLabel->setPlainText("     +     ");
  m_pLabel->setPos(0, 0);

  auto bounds = m_pLabel->boundingRect();
  m_PinCenter = bounds.center();

  QPainterPath p;
  p.addRect(bounds);
  setPath(p);

  if (pin.GetType() == xiiPin::Type::Input)
  {
    m_pLabel->setPlainText("");
  }
  else
  {
    m_pLabel->setToolTip("Add Transition");
  }
}

QRectF xiiQtStateMachinePin::GetPinRect() const
{
  auto rect = path().boundingRect();
  rect.translate(pos());
  return rect;
}

//////////////////////////////////////////////////////////////////////////

xiiQtStateMachineConnection::xiiQtStateMachineConnection()
{
  setFlag(QGraphicsItem::ItemIsSelectable);
}

//////////////////////////////////////////////////////////////////////////

xiiQtStateMachineNode::xiiQtStateMachineNode() = default;

void xiiQtStateMachineNode::InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject)
{
  xiiQtNode::InitNode(pManager, pObject);

  UpdateHeaderColor();
}

void xiiQtStateMachineNode::UpdateGeometry()
{
  prepareGeometryChange();

  auto labelRect = m_pTitleLabel->boundingRect();

  constexpr int padding      = 5;
  const int     headerWidth  = labelRect.width();
  const int     headerHeight = labelRect.height() + padding * 2;

  int h = headerHeight;
  int w = headerWidth;

  for (xiiQtPin* pQtPin : GetInputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w            = xiiMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
  }

  for (xiiQtPin* pQtPin : GetOutputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w            = xiiMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
    h += rectPin.height();
  }

  w += padding * 2;
  h += padding * 2;

  m_HeaderRect = QRectF(-padding, -padding, w, headerHeight);

  {
    QPainterPath p;
    p.addRoundedRect(-padding, -padding, w, h, padding, padding);
    setPath(p);
  }
}

void xiiQtStateMachineNode::UpdateState()
{
  UpdateHeaderColor();

  if (IsAnyState())
  {
    m_pTitleLabel->setPlainText("Any State");
  }
  else
  {
    xiiStringBuilder sName;

    auto& typeAccessor = GetObject()->GetTypeAccessor();

    xiiVariant name = typeAccessor.GetValue("Name");
    if (name.IsA<xiiString>() && name.Get<xiiString>().IsEmpty() == false)
    {
      sName = name.Get<xiiString>();
    }
    else
    {
      sName = typeAccessor.GetType()->GetTypeName();
    }

    if (IsInitialState())
    {
      sName.Append(" [Initial State]");
    }

    m_pTitleLabel->setPlainText(sName.GetData());
  }
}

void xiiQtStateMachineNode::ExtendContextMenu(QMenu& ref_menu)
{
  if (IsAnyState())
    return;

  QAction* pAction = new QAction("Set as Initial State", &ref_menu);
  pAction->setEnabled(IsInitialState() == false);
  pAction->connect(pAction, &QAction::triggered,
                   [this]() {
                     auto pScene = static_cast<xiiQtStateMachineAssetScene*>(scene());
                     pScene->SetInitialState(this);
                   });

  ref_menu.addAction(pAction);
}

bool xiiQtStateMachineNode::IsInitialState() const
{
  auto pManager = static_cast<const xiiStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsInitialState(GetObject());
}

bool xiiQtStateMachineNode::IsAnyState() const
{
  auto pManager = static_cast<const xiiStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsAnyState(GetObject());
}

void xiiQtStateMachineNode::UpdateHeaderColor()
{
  xiiColorScheme::Enum schemeColor = xiiColorScheme::Gray;

  if (IsAnyState())
  {
    schemeColor = xiiColorScheme::Violet;
  }
  else if (IsInitialState())
  {
    schemeColor = xiiColorScheme::Teal;
  }

  m_HeaderColor = xiiToQtColor(xiiColorScheme::DarkUI(schemeColor));

  update();
}

//////////////////////////////////////////////////////////////////////////

xiiQtStateMachineAssetScene::xiiQtStateMachineAssetScene(QObject* pParent /*= nullptr*/) :
  xiiQtNodeScene(pParent)
{
  SetConnectionStyle(xiiQtNodeScene::ConnectionStyle::StraightLine);
  SetConnectionDecorationFlags(xiiQtNodeScene::ConnectionDecorationFlags::DirectionArrows);
}

xiiQtStateMachineAssetScene::~xiiQtStateMachineAssetScene() = default;

void xiiQtStateMachineAssetScene::SetInitialState(xiiQtStateMachineNode* pNode)
{
  xiiCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Set Initial State");

  xiiStateMachine_SetInitialStateCommand cmd;
  cmd.m_NewInitialStateObject = pNode->GetObject()->GetGuid();

  xiiStatus res = history->AddCommand(cmd);

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

xiiStatus xiiQtStateMachineAssetScene::RemoveNode(xiiQtNode* pNode)
{
  auto       pManager         = static_cast<const xiiStateMachineNodeManager*>(GetDocumentNodeManager());
  const bool bWasInitialState = pManager->IsInitialState(pNode->GetObject());

  auto res = xiiQtNodeScene::RemoveNode(pNode);
  if (res.Succeeded() && bWasInitialState)
  {
    // Find another node
    xiiUuid newInitialStateObject;
    for (auto it : m_Nodes)
    {
      if (it.Value() != pNode && pManager->IsAnyState(it.Key()) == false)
      {
        newInitialStateObject = it.Key()->GetGuid();
      }
    }

    if (newInitialStateObject.IsValid())
    {
      xiiCommandHistory* history = pManager->GetDocument()->GetCommandHistory();

      xiiStateMachine_SetInitialStateCommand cmd;
      cmd.m_NewInitialStateObject = newInitialStateObject;

      res = history->AddCommand(cmd);
    }
  }

  return res;
}
