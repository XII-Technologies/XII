#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#ifdef XII_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <QApplication>
#endif

#ifdef XII_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pLpCmdLine, int iCmdShow)
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
  pQtApplication->setApplicationName("xiiFileserve");
  pQtApplication->setOrganizationDomain("www.xiitechnologies.com");
  pQtApplication->setOrganizationName("XII Technologies");
  pQtApplication->setApplicationVersion("1.0.0");

  xiiRun_Startup(pApp).IgnoreResult();

  SetApplicationDarkTheme();
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
      ++m_uiConnections;
      m_TimeTillClosing.SetZero();
      break;
    case xiiFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = xiiTime::Now() + m_CloseAppTimeout;
      }

      break;
    default:
      break;
  }
}
