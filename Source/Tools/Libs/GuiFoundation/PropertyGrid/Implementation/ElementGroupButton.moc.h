#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QToolButton>

class XII_GUIFOUNDATION_DLL xiiQtElementGroupButton : public QToolButton
{
  Q_OBJECT
public:
  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
    Help,
  };

  explicit xiiQtElementGroupButton(QWidget* pParent, ElementAction action, xiiQtPropertyWidget* pGroupWidget);
  ElementAction        GetAction() const { return m_Action; }
  xiiQtPropertyWidget* GetGroupWidget() const { return m_pGroupWidget; }

private:
  ElementAction        m_Action;
  xiiQtPropertyWidget* m_pGroupWidget;
};
