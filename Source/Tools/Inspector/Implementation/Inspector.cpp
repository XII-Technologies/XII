#include <Inspector/InspectorPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
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
  typedef xiiApplication SUPER;

  xiiInspectorApp() :
    xiiApplication("xiiInspector")
  {
  }

  void SetStyleSheet()
  {
    QApplication::setStyle(QStyleFactory::create("fusion"));
    QPalette palette;

    palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::Button, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Light, QColor(60, 60, 60, 255));
    palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
    palette.setColor(QPalette::Dark, QColor(45, 45, 45, 255));
    palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
    palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
    palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
    palette.setColor(QPalette::Base, QColor(20, 20, 20, 255));
    palette.setColor(QPalette::AlternateBase, QColor(20, 20, 20, 255));
    palette.setColor(QPalette::Window, QColor(30, 30, 30, 255));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
    palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
    palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
    QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
    palette.setBrush(QPalette::NoRole, NoRoleBrush);
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
    palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::PlaceholderText, QColor(200, 200, 200, 255).darker());

    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(40, 40, 40, 255));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

    QApplication::setPalette(palette);
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
