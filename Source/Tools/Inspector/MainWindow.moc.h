/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Inspector/StatVisWidget.moc.h>
#include <Inspector/ui_MainWindow.h>
#include <QMainWindow>
#include <ads/DockManager.h>

class xiiQtMainWindow : public QMainWindow, public Ui_MainWindow
{
  enum OnTopMode
  {
    Never,
    Always,
    WhenConnected
  };

public:
  Q_OBJECT

public:
  xiiQtMainWindow();
  ~xiiQtMainWindow();

  static xiiQtMainWindow* s_pWidget;

  static void ReceiveDiagnostics(const xiiTelemetrySample& sample);

  xiiResult           ConnectDiagnostics(xiiStringView sAddress, xiiUuid expectedInstance = {});
  xiiTelemetryClient& GetDiagnosticsClient() { return m_DiagnosticsClient; }

  virtual void closeEvent(QCloseEvent* pEvent);

public Q_SLOTS:
  void DockWidgetVisibilityChanged(bool bVisible);
  void UpdateNetworkTimeOut();

private Q_SLOTS:
  void on_ActionShowWindowLog_triggered();
  void on_ActionShowWindowMemory_triggered();
  void on_ActionShowWindowTime_triggered();
  void on_ActionShowWindowInput_triggered();
  void on_ActionShowWindowCVar_triggered();
  void on_ActionShowWindowReflection_triggered();
  void on_ActionShowWindowSubsystems_triggered();
  void on_ActionShowWindowPlugins_triggered();
  void on_ActionShowWindowFile_triggered();
  void on_ActionShowWindowGlobalEvents_triggered();
  void on_ActionShowWindowResource_triggered();

  void on_ActionOnTopWhenConnected_triggered();
  void on_ActionAlwaysOnTop_triggered();
  void on_ActionNeverOnTop_triggered();

private:
  xiiTelemetryClient  m_DiagnosticsClient;
  xiiUuid             m_LastInstance;
  xiiUuid             m_ExpectedInstance;
  xiiNetworkErrorCode m_LastDiagnosticsError = xiiNetworkErrorCode::Success;
  bool                m_bDiagnosticsReady    = false;
  xiiSet<xiiUInt64>   m_DiagnosticsSubscribed;
  xiiUInt64           m_uiDiagnosticsRequest    = 0;
  xiiUInt64           m_uiCatalogRevision       = 0;
  xiiUInt64           m_uiDiagnosticsGeneration = 0;
  void                SetAlwaysOnTop(OnTopMode Mode);
  void                UpdateAlwaysOnTop();
  void                SetupNetworkTimer();
  void                UpdateNetwork();

private:
  OnTopMode m_OnTopMode;
  QTimer*   m_pNetworkTimer;

public:
  ads::CDockManager*  m_pDockManager = nullptr;
  QAction*            m_pActionShowStatIn[10];
  xiiQtStatVisWidget* m_pStatHistoryWidgets[10];
};
