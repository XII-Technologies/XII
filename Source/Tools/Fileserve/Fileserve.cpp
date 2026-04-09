#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

#ifdef XII_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
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

void xiiFileserverApp::ShaderMessageHandler(xiiFileserveClientContext& ref_ctxt, xiiRemoteMessage& ref_msg, xiiRemoteInterface& ref_clientChannel, xiiDelegate<void(const char*)> logActivity)
{
  if (ref_msg.GetMessageID() == 'CMPL')
  {
    for (auto& dd : ref_ctxt.m_MountedDataDirs)
    {
      xiiFileSystem::AddDataDirectory(dd.m_sPathOnServer, "FileServe", dd.m_sRootName, xiiDataDirUsage::AllowWrites).IgnoreResult();
    }

    auto& r = ref_msg.GetReader();

    xiiStringBuilder                              tmp;
    xiiStringBuilder                              sFile, sPlatform;
    xiiUInt32                                     uiPermutationVariableCount;
    xiiHybridArray<xiiGALPermutationVariable, 16> permutationVariables;

    r >> sFile;
    r >> sPlatform;
    r >> uiPermutationVariableCount;
    permutationVariables.SetCount(uiPermutationVariableCount);

    tmp.SetFormat("Compiling Shader '{}' - '{}'", sFile, sPlatform);

    for (auto& pv : permutationVariables)
    {
      r >> pv.m_sName;
      r >> pv.m_sValue;

      tmp.AppendWithSeparator(" | ", pv.m_sName, "=", pv.m_sValue);
    }

    logActivity(tmp);

    // enable runtime shader compilation and set the shader cache directories (this only works, if the user doesn't change the default values)
    // the 'active platform' value should never be used during shader compilation, because there it is passed in
    xiiGALShaderManager::Configure("FILESERVE_UNUSED", true);

    xiiLogSystemToBuffer log;
    xiiLogSystemScope    ls(&log);

    xiiGALShaderCompiler shaderCompiler;
    xiiResult            result = shaderCompiler.CompileShaderPermutationForPlatforms(sFile, permutationVariables, xiiLog::GetThreadLocalLogSystem(), sPlatform);

    xiiFileSystem::RemoveDataDirectoryGroup("FileServe");

    if (result.Succeeded())
    {
      // invalidate read cache to not short-circuit the next file read operation
      xiiRemoteMessage msg2('FSRV', 'INVC');
      ref_clientChannel.Send(xiiRemoteTransmitMode::Reliable, msg2);
    }
    else
    {
      logActivity("[ERROR] Shader Compilation failed:");

      xiiHybridArray<xiiStringView, 32> lines;
      log.m_sBuffer.Split(false, lines, "\n", "\r");

      for (auto line : lines)
      {
        tmp.Set(">   ", line);
        logActivity(tmp);
      }
    }

    {
      xiiRemoteMessage msg2('SHDR', 'CRES');
      msg2.GetWriter() << (result == XII_SUCCESS);
      msg2.GetWriter() << log.m_sBuffer;

      ref_clientChannel.Send(xiiRemoteTransmitMode::Reliable, msg2);
    }
  }
}

void xiiFileserverApp::SetStyleSheet()
{
#ifdef XII_USE_QT
  QApplication::setStyle(QStyleFactory::create("fusion"));

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
#endif
}
