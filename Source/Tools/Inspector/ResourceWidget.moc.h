/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_ResourceWidget.h>
#include <ads/DockWidget.h>

class xiiQtResourceWidget : public ads::CDockWidget, public Ui_ResourceWidget
{
public:
  Q_OBJECT

public:
  xiiQtResourceWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtResourceWidget* s_pWidget;

private Q_SLOTS:

  void on_LineFilterByName_textChanged();
  void on_ComboResourceTypes_currentIndexChanged(int state);
  void on_CheckShowDeleted_toggled(bool checked);
  void on_ButtonSave_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  void UpdateAll();

  struct ResourceData
  {
    ResourceData()
    {
      m_pMainItem = nullptr;
      m_bUpdate   = true;
    }

    bool                          m_bUpdate;
    QTableWidgetItem*             m_pMainItem;
    xiiString                     m_sResourceID;
    xiiString                     m_sResourceType;
    xiiResourcePriority           m_Priority;
    xiiBitflags<xiiResourceFlags> m_Flags;
    xiiResourceLoadDescription    m_LoadingState;
    xiiResource::MemoryUsage      m_Memory;
    xiiString                     m_sResourceDescription;
  };

  bool      m_bShowDeleted;
  xiiString m_sTypeFilter;
  xiiString m_sNameFilter;
  xiiTime   m_LastTableUpdate;
  bool      m_bUpdateTable;

  bool                                  m_bUpdateTypeBox;
  xiiSet<xiiString>                     m_ResourceTypes;
  xiiHashTable<xiiUInt64, ResourceData> m_Resources;
};
