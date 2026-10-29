/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QToolBar>

class QToolButton;

/// Workspace controls in the application title area, using Qt's native system move/resize support.
class XII_GUIFOUNDATION_DLL xiiQtWindowTitleBar : public QToolBar
{
public:
  explicit xiiQtWindowTitleBar(QWidget* pWindow);
  void AddWindowControls();

protected:
  void resizeEvent(QResizeEvent* pEvent) override;
  void changeEvent(QEvent* pEvent) override;
  bool eventFilter(QObject* pObject, QEvent* pEvent) override;
  void mousePressEvent(QMouseEvent* pEvent) override;
  void mouseDoubleClickEvent(QMouseEvent* pEvent) override;

private:
  QToolButton* m_pMaximizeButton = nullptr;
};
