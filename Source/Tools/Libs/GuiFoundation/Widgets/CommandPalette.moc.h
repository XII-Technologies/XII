/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDialog>
#include <QList>
#include <QPointer>

class QAction;
class QLineEdit;
class QTreeWidget;

/// Searches borrowed live actions. Execution rechecks lifetime, visibility and enabled state.
class XII_GUIFOUNDATION_DLL xiiQtCommandPalette : public QDialog
{
  Q_OBJECT

public:
  xiiQtCommandPalette(QWidget* pParent, const QList<QAction*>& actions);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent) override;

private:
  void UpdateResults();
  void ExecuteCurrent();

  QList<QPointer<QAction>> m_Actions;
  QLineEdit*               m_pSearch  = nullptr;
  QTreeWidget*             m_pResults = nullptr;
};
