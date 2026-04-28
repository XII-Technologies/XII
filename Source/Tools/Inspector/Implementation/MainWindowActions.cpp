/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

void xiiQtMainWindow::on_ActionShowWindowLog_triggered()
{
  xiiQtLogDockWidget::s_pWidget->toggleView(ActionShowWindowLog->isChecked());
  xiiQtLogDockWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowMemory_triggered()
{
  xiiQtMemoryWidget::s_pWidget->toggleView(ActionShowWindowMemory->isChecked());
  xiiQtMemoryWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowTime_triggered()
{
  xiiQtTimeWidget::s_pWidget->toggleView(ActionShowWindowTime->isChecked());
  xiiQtTimeWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowInput_triggered()
{
  xiiQtInputWidget::s_pWidget->toggleView(ActionShowWindowInput->isChecked());
  xiiQtInputWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowCVar_triggered()
{
  xiiQtCVarsWidget::s_pWidget->toggleView(ActionShowWindowCVar->isChecked());
  xiiQtCVarsWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowReflection_triggered()
{
  xiiQtReflectionWidget::s_pWidget->toggleView(ActionShowWindowReflection->isChecked());
  xiiQtReflectionWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  xiiQtSubsystemsWidget::s_pWidget->toggleView(ActionShowWindowSubsystems->isChecked());
  xiiQtSubsystemsWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowPlugins_triggered()
{
  xiiQtPluginsWidget::s_pWidget->toggleView(ActionShowWindowPlugins->isChecked());
  xiiQtPluginsWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowFile_triggered()
{
  xiiQtFileWidget::s_pWidget->toggleView(ActionShowWindowFile->isChecked());
  xiiQtFileWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  xiiQtGlobalEventsWidget::s_pWidget->toggleView(ActionShowWindowGlobalEvents->isChecked());
  xiiQtGlobalEventsWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowData_triggered()
{
  xiiQtDataWidget::s_pWidget->toggleView(ActionShowWindowData->isChecked());
  xiiQtDataWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionShowWindowResource_triggered()
{
  xiiQtResourceWidget::s_pWidget->toggleView(ActionShowWindowResource->isChecked());
  xiiQtResourceWidget::s_pWidget->raise();
}

void xiiQtMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void xiiQtMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void xiiQtMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}
