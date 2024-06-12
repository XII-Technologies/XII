#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_TimeWidget.h>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class xiiQtTimeWidget : public ads::CDockWidget, public Ui_TimeWidget
{
public:
  Q_OBJECT

public:
  static const xiiUInt8 s_uiMaxColors = 9;

  xiiQtTimeWidget(QWidget* pParent = 0);

  static xiiQtTimeWidget* s_pWidget;

private Q_SLOTS:

  void on_ListClocks_itemChanged(QListWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene     m_Scene;

  xiiUInt32 m_uiMaxSamples;

  xiiUInt8 m_uiColorsUsed;
  bool     m_bClocksChanged;

  xiiTime m_MaxGlobalTime;
  xiiTime m_DisplayInterval;
  xiiTime m_LastUpdatedClockList;

  struct TimeSample
  {
    xiiTime m_AtGlobalTime;
    xiiTime m_Timestep;
  };

  struct ClockData
  {
    xiiDeque<TimeSample> m_TimeSamples;

    bool             m_bDisplay    = true;
    xiiUInt8         m_uiColor     = 0xFF;
    xiiTime          m_MinTimestep = xiiTime::MakeFromSeconds(60.0);
    xiiTime          m_MaxTimestep;
    QListWidgetItem* m_pListItem = nullptr;
  };

  xiiMap<xiiString, ClockData> m_ClockData;
};
