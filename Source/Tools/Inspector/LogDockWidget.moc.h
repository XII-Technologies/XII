#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_LogDockWidget.h>
#include <ads/DockWidget.h>

class xiiQtLogDockWidget : public ads::CDockWidget, public Ui_LogDockWidget
{
public:
  Q_OBJECT

public:
  xiiQtLogDockWidget(QWidget* parent = 0);

  void Log(const xiiFormatString& sText);

  static xiiQtLogDockWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
};
