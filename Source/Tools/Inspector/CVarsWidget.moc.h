/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Inspector/ui_CVarsWidget.h>
#include <ads/DockWidget.h>

class xiiQtCVarsWidget : public ads::CDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  xiiQtCVarsWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static xiiQtCVarsWidget* s_pWidget;

private Q_SLOTS:
  void BoolChanged(xiiStringView sCVar, bool newValue);
  void FloatChanged(xiiStringView sCVar, float newValue);
  void DoubleChanged(xiiStringView sCVar, double newValue);
  void IntChanged(xiiStringView sCVar, int newValue);
  void StringChanged(xiiStringView sCVar, xiiStringView sNewValue);

public:
  static void ReceiveDiagnostics(const xiiTelemetrySample& sample);
  static void ReceiveCommandResult(const xiiNetworkRpcResult& result, const xiiTelemetryCommandResponse& response);

  void ResetStats();

private:
  // void UpdateCVarsTable(bool bRecreate);

  void SendCVarUpdateToServer(xiiStringView sName, const xiiCVarWidgetData& cvd);

  xiiMap<xiiString, xiiCVarWidgetData> m_CVars;
};
