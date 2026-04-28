/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Inspector/ui_MainWidget.h>
#include <QMainWindow>
#include <ads/DockManager.h>

class QTreeWidgetItem;

class xiiQtMainWidget : public ads::CDockWidget, public Ui_MainWidget
{
  Q_OBJECT
public:
  static xiiQtMainWidget* s_pWidget;

  xiiQtMainWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);
  ~xiiQtMainWidget();

  void         ResetStats();
  void         UpdateStats();
  virtual void closeEvent(QCloseEvent* pEvent) override;

  static void ProcessTelemetry(void* pUnused);

public Q_SLOTS:
  void ShowStatIn(bool);

private Q_SLOTS:
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:
  void SaveFavorites();
  void LoadFavorites();

  QTreeWidgetItem* CreateStat(xiiStringView sPath, bool bParent);
  void             SetFavorite(const xiiString& sStat, bool bFavorite);

  xiiUInt32 m_uiMaxStatSamples;
  xiiTime   m_MaxGlobalTime;

  struct StatSample
  {
    xiiTime m_AtGlobalTime;
    double  m_Value;
  };

  struct StatData
  {
    xiiDeque<StatSample> m_History;

    xiiVariant       m_Value;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavorite;

    StatData()
    {
      m_pItem         = nullptr;
      m_pItemFavorite = nullptr;
    }
  };

  friend class xiiQtStatVisWidget;
  xiiMap<xiiString, StatData> m_Stats;
  xiiSet<xiiString>           m_Favorites;
};
