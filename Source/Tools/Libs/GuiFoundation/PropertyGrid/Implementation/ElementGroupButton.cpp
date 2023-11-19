#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>

xiiQtElementGroupButton::xiiQtElementGroupButton(QWidget* pParent, xiiQtElementGroupButton::ElementAction action, xiiQtPropertyWidget* pGroupWidget) :
  QToolButton(pParent)
{
  m_Action       = action;
  m_pGroupWidget = pGroupWidget;

  setAutoRaise(true);

  setIconSize(QSize(16, 16));

  switch (action)
  {
    case xiiQtElementGroupButton::ElementAction::MoveElementUp:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveUp.svg")));
      break;
    case xiiQtElementGroupButton::ElementAction::MoveElementDown:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/MoveDown.svg")));
      break;
    case xiiQtElementGroupButton::ElementAction::DeleteElement:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Delete.svg")));
      break;
    case xiiQtElementGroupButton::ElementAction::Help:
      setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Log.svg")));
      break;
  }
}
