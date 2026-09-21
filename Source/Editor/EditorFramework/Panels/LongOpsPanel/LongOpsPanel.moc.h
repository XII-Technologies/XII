/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_LongOpsPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <QTimer>

struct xiiLongOpControllerEvent;

/// This panel listens to events from xiiLongOpControllerManager and displays all currently known long operations
class XII_EDITORFRAMEWORK_DLL xiiQtLongOpsPanel : public xiiQtApplicationPanel, public Ui_LongOpsPanel
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtLongOpsPanel);

public:
  xiiQtLongOpsPanel(ads::CDockManager* pDockManager);
  ~xiiQtLongOpsPanel();

private:
  void LongOpsEventHandler(const xiiLongOpControllerEvent& e);
  void RebuildTable();
  void UpdateTable();

  bool                             m_bUpdateTimerRunning = false;
  bool                             m_bRebuildTable       = true;
  bool                             m_bUpdateTable        = false;
  xiiHashTable<xiiUuid, xiiUInt32> m_LongOpGuidToRow;

private Q_SLOTS:
  void StartUpdateTimer();
  void UpdateUI();
  void OnClickButton(bool);
  void OnCellDoubleClicked(int row, int column);
};
