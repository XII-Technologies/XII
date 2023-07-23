#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_StatVisWidget.h>
#include <QAction>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class xiiQtStatVisWidget : public ads::CDockWidget, public Ui_StatVisWidget
{
public:
  Q_OBJECT

public:
  static const xiiUInt8 s_uiMaxColors = 9;

  xiiQtStatVisWidget(QWidget* pParent, xiiInt32 iWindowNumber);
  ~xiiQtStatVisWidget();

  void UpdateStats();

  static xiiQtStatVisWidget* s_pWidget;

  void AddStat(const xiiString& sStatPath, bool bEnabled = true, bool bRaiseWindow = true);

  void Save();
  void Load();

private Q_SLOTS:

  void on_ComboTimeframe_currentIndexChanged(int index);
  void on_LineName_textChanged(const QString& text);
  void on_SpinMin_valueChanged(double val);
  void on_SpinMax_valueChanged(double val);
  void on_ToggleVisible();
  void on_ButtonRemove_clicked();
  void on_ListStats_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

public:
  QAction m_ShowWindowAction;

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene     m_Scene;

  static xiiInt32 s_iCurColor;

  xiiTime m_DisplayInterval;

  xiiInt32 m_iWindowNumber;

  struct StatsData
  {
    QListWidgetItem* m_pListItem = nullptr;
    xiiUInt8         m_uiColor   = 0;
  };

  xiiMap<xiiString, StatsData> m_Stats;
};
