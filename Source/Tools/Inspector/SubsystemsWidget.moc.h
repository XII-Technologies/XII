/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_SubsystemsWidget.h>
#include <ads/DockWidget.h>

class xiiQtSubsystemsWidget : public ads::CDockWidget, public Ui_SubsystemsWidget
{
public:
  Q_OBJECT

public:
  xiiQtSubsystemsWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtSubsystemsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateSubSystems();

  struct SubsystemData
  {
    xiiString m_sPlugin;
    bool      m_bStartupDone[xiiStartupStage::ENUM_COUNT];
    xiiString m_sDependencies;
  };

  bool                             m_bUpdateSubsystems;
  xiiMap<xiiString, SubsystemData> m_Subsystems;
};
