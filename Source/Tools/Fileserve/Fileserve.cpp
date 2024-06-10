#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#ifdef XII_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <QApplication>
#  include <qstylefactory.h>
#endif

#ifdef XII_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int iCmdShow)
{
#else
int main(int argc, const char** argv)
{
#endif
  xiiFileserverApp* pApp = new xiiFileserverApp();

#ifdef XII_USE_QT
  xiiCommandLineUtils::GetGlobalInstance()->SetCommandLine();

  int           argc           = 0;
  char**        argv           = nullptr;
  QApplication* pQtApplication = new QApplication(argc, const_cast<char**>(argv));
  pQtApplication->setOrganizationDomain("www.xiitechnologies.com");
  pQtApplication->setOrganizationName("XII Technologies");
  pQtApplication->setApplicationName("xiiFileserve");
  pQtApplication->setApplicationVersion("1.0.0");

  pApp->SetStyleSheet();

  xiiRun_Startup(pApp).IgnoreResult();

  CreateFileserveMainWindow(pApp);
  pQtApplication->exec();
  xiiRun_Shutdown(pApp);
#else
  pApp->SetCommandLineArguments((xiiUInt32)argc, argv);
  xiiRun(pApp);
#endif

  const int iReturnCode = pApp->GetReturnCode();
  if (iReturnCode != 0)
  {

    std::string text = pApp->TranslateReturnCode();
    if (!text.empty())
      printf("Return Code: '%s'\n", text.c_str());
  }

#ifdef XII_USE_QT
  delete pQtApplication;
#endif

  delete pApp;


  return iReturnCode;
}

xiiResult xiiFileserverApp::BeforeCoreSystemsStartup()
{
  xiiStartup::AddApplicationTag("tool");
  xiiStartup::AddApplicationTag("fileserve");

  return SUPER::BeforeCoreSystemsStartup();
}

void xiiFileserverApp::FileserverEventHandler(const xiiFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case xiiFileserverEvent::Type::ClientConnected:
    case xiiFileserverEvent::Type::ClientReconnected:
    {
      ++m_uiConnections;
      m_TimeTillClosing = xiiTime::MakeZero();
    }
    break;

    case xiiFileserverEvent::Type::ClientDisconnected:
    {
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = xiiTime::Now() + m_CloseAppTimeout;
      }
    }
    break;

    default:
      break;
  }
}

void xiiFileserverApp::SetStyleSheet()
{
#ifdef XII_USE_QT
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
#endif
}
