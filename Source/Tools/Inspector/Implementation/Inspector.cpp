/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Foundation/Application/Application.h>
#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/RenderGraphWidget.h>
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


    QSettings     Settings;
    const QString sServer = Settings.value("LastConnection", QLatin1String("local:")).toString();

    const auto diagnostics = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-DiagnosticsConnect");
    if (!diagnostics.IsEmpty())
      MainWindow.ConnectDiagnostics(diagnostics).IgnoreResult();
    else if (sServer.startsWith("local:") || sServer.startsWith("tls://") || sServer.startsWith("quic://"))
      MainWindow.ConnectDiagnostics(sServer.toUtf8().constData()).IgnoreResult();


    MainWindow.show();
    SetReturnCode(app.exec());


    return xiiApplication::Execution::Quit;
  }
};

XII_APPLICATION_ENTRY_POINT(xiiInspectorApp);
