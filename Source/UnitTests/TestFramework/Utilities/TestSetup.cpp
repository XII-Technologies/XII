#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/SystemInformation.h>

#ifdef XII_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <TestFramework/Framework/Uwp/uwpTestFramework.h>
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <conio.h>
#endif

int          xiiTestSetup::s_iArgc = 0;
const char** xiiTestSetup::s_pArgv = nullptr;

xiiTestFramework* xiiTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int iArgc, const char** pArgv)
{
  s_iArgc = iArgc;
  s_pArgv = pArgv;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
  {
    std::cout << "Failed to init WinRT." << std::endl;
  }
#endif

  // without a proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(xiiOSFile::GetUserDataFolder());
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append("XII/UnitTests/");
  sTestFolder.append(szTestName);

  std::string sTestDataSubFolder = "Data/UnitTests/";
  sTestDataSubFolder.append(szTestName);

#ifdef XII_USE_QT
  xiiTestFramework* pTestFramework = new xiiQtTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Command line args in UWP are handled differently and can't be retrieved from the main function.
  xiiTestFramework* pTestFramework = new xiiUwpTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), 0, nullptr);
#else
  xiiTestFramework* pTestFramework = new xiiTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(xiiOutputToHTML::OutputToHTML);

  xiiCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(pTestFramework->GetAbsOutputPath(), szTestName);
  xiiCrashHandler::SetCrashHandler(&xiiCrashHandler_WriteMiniDump::g_Instance);

  return pTestFramework;
}

xiiTestAppRun xiiTestSetup::RunTests()
{
  xiiTestFramework* pTestFramework = xiiTestFramework::GetInstance();

  // Todo: Incorporate all the below in a virtual call of testFramework?
#ifdef XII_USE_QT
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bNoGUI)
  {
    return pTestFramework->RunTestExecutionLoop();
  }

  // Setup Qt Application

  int    argc = s_iArgc;
  char** argv = const_cast<char**>(s_pArgv);

  if (qApp != nullptr)
  {
    bool ok     = false;
    int  iCount = qApp->property("Shared").toInt(&ok);
    XII_ASSERT_DEV(ok, "Existing QApplication was not constructed by XII!");
    qApp->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    new QApplication(argc, argv);
    qApp->setProperty("Shared", QVariant::fromValue((int)1));
    qApp->setOrganizationDomain("www.xiitechnologies.com");
    qApp->setOrganizationName("XII Technologies");
    qApp->setApplicationName(pTestFramework->GetTestName());
    qApp->setApplicationVersion("1.0.0");

    xiiQtTestGUI::SetDarkTheme();
  }

  // Create main window
  {
    xiiQtTestGUI mainWindow(*static_cast<xiiQtTestFramework*>(pTestFramework));
    mainWindow.show();

    qApp->exec();
  }
  {
    const int iCount = qApp->property("Shared").toInt();
    if (iCount == 1)
    {
      delete qApp;
    }
    else
    {
      qApp->setProperty("Shared", QVariant::fromValue(iCount - 1));
    }
  }

  return xiiTestAppRun::Quit;
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  static_cast<xiiUwpTestFramework*>(pTestFramework)->Run();
  return xiiTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTestExecutionLoop();
#endif
}

void xiiTestSetup::DeInitTestFramework(bool bSilent /*= false*/)
{
  xiiTestFramework* pTestFramework = xiiTestFramework::GetInstance();

  xiiStartup::ShutdownCoreSystems();

  // In the UWP case we never initialized this thread for XII so we can't do log output now.
#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  if (!bSilent)
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  }
#endif

  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bKeepConsoleOpen && !bSilent)
  {
    if (xiiSystemInformation::IsDebuggerAttached())
    {
      std::cout << "Press the any key to continue...\n";
      fflush(stdin);
      [[maybe_unused]] xiiInt32 c = getchar();
    }
  }

  // This is needed as at least windows can't be bothered to write anything
  // to the output streams at all if it's not enough or the app is too fast.
  fflush(stdout);
  fflush(stderr);
  delete pTestFramework;
}

xiiInt32 xiiTestSetup::GetFailedTestCount()
{
  return xiiTestFramework::GetInstance()->GetTestsFailedCount();
}


XII_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestSetup);
