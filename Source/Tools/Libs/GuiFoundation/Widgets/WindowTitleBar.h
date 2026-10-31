/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QToolBar>

/// Workspace controls below the native caption, with additional noninteractive system-move regions.
class XII_GUIFOUNDATION_DLL xiiQtWindowTitleBar : public QToolBar
{
public:
  explicit xiiQtWindowTitleBar(QWidget* pWindow);

  /// Coordinates are local logical pixels. Interactive children, including menu whitespace, are excluded.
  bool IsCaptionPosition(const QPoint& position) const;

protected:
  void changeEvent(QEvent* pEvent) override;
  void mousePressEvent(QMouseEvent* pEvent) override;
  void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
};
