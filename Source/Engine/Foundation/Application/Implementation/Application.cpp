#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

xiiApplication::xiiApplication(const char* szAppName) :
  m_iReturnCode(0), m_uiArgumentCount(0), m_pArguments(nullptr), m_bReportMemoryLeaks(true), m_sAppName(szAppName)
{
}

xiiApplication::~xiiApplication() = default;

void xiiApplication::SetApplicationName(const char* szAppName)
{
  m_sAppName = szAppName;
}

xiiCommandLineOptionBool opt_WaitForDebugger("app", "-WaitForDebugger", "If specified, the application will wait at startup until a debugger is attached.", false);

xiiResult xiiApplication::BeforeCoreSystemsStartup()
{
  if (xiiFileSystem::DetectSdkRootDirectory().Failed())
  {
    xiiLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  xiiRTTI::VerifyCorrectnessForAllTypes();
#endif

  if (opt_WaitForDebugger.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    while (!xiiSystemInformation::IsDebuggerAttached())
    {
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(1));
    }

    XII_DEBUG_BREAK;
  }

  return XII_SUCCESS;
}


void xiiApplication::SetCommandLineArguments(xiiUInt32 uiArgumentCount, const char** ppArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_pArguments      = ppArguments;

  xiiCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, ppArguments, xiiCommandLineUtils::PreferOsArgs);
}


const char* xiiApplication::GetArgument(xiiUInt32 uiArgument) const
{
  XII_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_pArguments[uiArgument];
}


void xiiApplication::RequestQuit()
{
  m_bWasQuitRequested = true;
}


xiiApplication* xiiApplication::s_pApplicationInstance = nullptr;



XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Application);
