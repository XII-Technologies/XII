/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QToolButton;

/// Integrated editor caption and workspace controls, using native window movement.
class XII_GUIFOUNDATION_DLL xiiQtWindowTitleBar : public QWidget
{
public:
  explicit xiiQtWindowTitleBar(QWidget* pWindow);
  void AddWindowControls();

  /// Coordinates are local logical pixels. Interactive children, including menu whitespace, are excluded.
  bool IsCaptionPosition(const QPoint& position) const;

protected:
  void changeEvent(QEvent* pEvent) override;
  bool eventFilter(QObject* pObject, QEvent* pEvent) override;
  void mousePressEvent(QMouseEvent* pEvent) override;
  void mouseDoubleClickEvent(QMouseEvent* pEvent) override;

private:
  QToolButton* m_pMaximizeButton = nullptr;
};
