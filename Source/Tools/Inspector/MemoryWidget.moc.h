#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_MemoryWidget.h>
#include <QAction>
#include <QGraphicsView>
#include <QPointer>
#include <ads/DockWidget.h>

class QTreeWidgetItem;

class xiiQtMemoryWidget : public ads::CDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  static const xiiUInt8 s_uiMaxColors = 9;

  xiiQtMemoryWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtMemoryWidget* s_pWidget;

private Q_SLOTS:

  void on_ListAllocators_itemChanged(QTreeWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);
  void on_actionEnableOnlyThis_triggered(bool);
  void on_actionEnableAll_triggered(bool);
  void on_actionDisableAll_triggered(bool);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void CustomContextMenuRequested(const QPoint& pos);

  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene     m_Scene;

  xiiTime m_LastUsedMemoryStored;
  xiiTime m_LastUpdatedAllocatorList;

  xiiUInt32 m_uiMaxSamples;
  xiiUInt32 m_uiDisplaySamples;

  xiiUInt8 m_uiColorsUsed;
  bool     m_bAllocatorsChanged;

  struct AllocatorData
  {
    xiiDeque<xiiUInt64> m_UsedMemory;
    xiiString           m_sName;

    bool             m_bStillInUse             = true;
    bool             m_bReceivedData           = false;
    bool             m_bDisplay                = true;
    xiiUInt8         m_uiColor                 = 0xFF;
    xiiUInt32        m_uiParentId              = xiiInvalidIndex;
    xiiUInt64        m_uiAllocs                = 0;
    xiiUInt64        m_uiDeallocs              = 0;
    xiiUInt64        m_uiLiveAllocs            = 0;
    xiiUInt64        m_uiMaxUsedMemoryRecently = 0;
    xiiUInt64        m_uiMaxUsedMemory         = 0;
    QTreeWidgetItem* m_pTreeItem               = nullptr;
  };

  AllocatorData m_Accu;

  xiiMap<xiiUInt32, AllocatorData> m_AllocatorData;
};
