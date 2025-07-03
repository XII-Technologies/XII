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
  using SUPER = xiiApplication;

  xiiInspectorApp() :
    xiiApplication("xiiInspector")
  {
  }

  void SetStyleSheet()
  {
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;

    // Base surfaces
    palette.setColor(QPalette::Window, QColor(28, 28, 30));        // Main window background
    palette.setColor(QPalette::Base, QColor(18, 18, 20));          // Input fields, scene graph
    palette.setColor(QPalette::AlternateBase, QColor(36, 36, 38)); // Alternating rows
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));           // Property grid arrays

    // Text & foreground
    palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    palette.setColor(QPalette::Text, QColor(220, 220, 220));
    palette.setColor(QPalette::BrightText, QColor(255, 85, 85)); // Alerts or emphasis
    palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
    palette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));

    // Buttons & controls
    palette.setColor(QPalette::Button, QColor(40, 40, 42)); // Toolbuttons, dashboard
    palette.setColor(QPalette::Light, QColor(60, 60, 60));  // Tab lines, gradients
    palette.setColor(QPalette::Midlight, QColor(55, 55, 55));
    palette.setColor(QPalette::Dark, QColor(35, 35, 35)); // Underlines, separators
    palette.setColor(QPalette::Mid, QColor(45, 45, 45));  // Group box outlines

    // Highlights & links
    palette.setColor(QPalette::Highlight, QColor(0, 122, 204)); // Selection blue
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, QColor(0, 122, 204));
    palette.setColor(QPalette::LinkVisited, QColor(128, 100, 162));

    // Tooltips
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
    palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));

    // Disabled state
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(35, 35, 35));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(70, 90, 110));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));

    // NoRole fallback
    palette.setBrush(QPalette::NoRole, QBrush(QColor(0, 0, 0), Qt::NoBrush));

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
