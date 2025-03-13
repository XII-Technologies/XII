#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_GlobalEventsWidget.h>
#include <ads/DockWidget.h>

class xiiQtGlobalEventsWidget : public ads::CDockWidget, public Ui_GlobalEventsWidget
{
public:
  Q_OBJECT

public:
  xiiQtGlobalEventsWidget(QWidget* pParent = nullptr);

  static xiiQtGlobalEventsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void UpdateTable(bool bRecreate);

  struct GlobalEventsData
  {
    xiiInt32  m_iTableRow;
    xiiUInt32 m_uiTimesFired;
    xiiUInt16 m_uiNumHandlers;
    xiiUInt16 m_uiNumHandlersOnce;

    GlobalEventsData()
    {
      m_iTableRow = -1;

      m_uiTimesFired      = 0;
      m_uiNumHandlers     = 0;
      m_uiNumHandlersOnce = 0;
    }
  };

  xiiMap<xiiString, GlobalEventsData> m_Events;
};
