/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

#include <QApplication>
#include <QSettings>
#include <qstylefactory.h>

class xiiInspectorApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiInspectorApp() :
    xiiApplication("xiiInspector")
  {
  }

  void SetStyleSheet()
  {
    QApplication::setStyle(new xiiQtDarkEditorStyle);
  }

  virtual xiiResult BeforeCoreSystemsStartup() override
  {
    xiiStartup::AddApplicationTag("tool");
    xiiStartup::AddApplicationTag("inspector");

    return xiiApplication::BeforeCoreSystemsStartup();
  }

  virtual Execution Run() override
  {
    int    iArgs = GetArgumentCount();
    char** cArgs = (char**)GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    QCoreApplication::setOrganizationDomain("www.xiitechnologies.com");
    QCoreApplication::setOrganizationName("XII Technologies");
    QCoreApplication::setApplicationName("xiiInspector");
    QCoreApplication::setApplicationVersion("1.0.0");

    SetStyleSheet();

    xiiQtMainWindow MainWindow;

    xiiTelemetry::AcceptMessagesForSystem('CVAR', true, xiiQtCVarsWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('CMD', true, xiiQtCVarsWidget::ProcessTelemetryConsole, nullptr);
    xiiTelemetry::AcceptMessagesForSystem(' LOG', true, xiiQtLogDockWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem(' MEM', true, xiiQtMemoryWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('TIME', true, xiiQtTimeWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem(' APP', true, xiiQtMainWindow::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('FILE', true, xiiQtFileWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('INPT', true, xiiQtInputWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('STRT', true, xiiQtSubsystemsWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('STAT', true, xiiQtMainWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('PLUG', true, xiiQtPluginsWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('EVNT', true, xiiQtGlobalEventsWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('RFLC', true, xiiQtReflectionWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('TRAN', true, xiiQtDataWidget::ProcessTelemetry, nullptr);
    xiiTelemetry::AcceptMessagesForSystem('RESM', true, xiiQtResourceWidget::ProcessTelemetry, nullptr);

    QSettings     Settings;
    const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

    xiiTelemetry::ConnectToServer(sServer.toUtf8().data()).IgnoreResult();

    MainWindow.show();
    SetReturnCode(app.exec());

    xiiTelemetry::CloseConnection();

    return xiiApplication::Execution::Quit;
  }
};

XII_APPLICATION_ENTRY_POINT(xiiInspectorApp);
