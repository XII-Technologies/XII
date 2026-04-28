/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TestFramework/TestFrameworkPCH.h>

#include <Texture/Image/Formats/ImageFileFormat.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <TestFramework/Utilities/TestOrder.h>

#include <cstdlib>
#include <stdexcept>
#include <stdlib.h>

#ifdef XII_TESTFRAMEWORK_USE_FILESERVE
#  include <FileservePlugin/Client/FileserveClient.h>
#  include <FileservePlugin/Client/FileserveDataDir.h>
#  include <FileservePlugin/FileservePluginDLL.h>
#endif

xiiTestFramework* xiiTestFramework::s_pInstance = nullptr;

const char*           xiiTestFramework::s_szTestBlockName    = "";
int                   xiiTestFramework::s_iAssertCounter     = 0;
bool                  xiiTestFramework::s_bCallstackOnAssert = false;
xiiLog::TimestampMode xiiTestFramework::s_LogTimestampMode   = xiiLog::TimestampMode::None;

xiiCommandLineOptionPath   opt_OrderFile("_TestFramework", "-order", "Path to a file that defines which tests to run.", "");
xiiCommandLineOptionPath   opt_SettingsFile("_TestFramework", "-settings", "Path to a file containing the test settings.", "");
xiiCommandLineOptionBool   opt_Run("_TestFramework", "-run", "Makes the tests execute right away.", false);
xiiCommandLineOptionBool   opt_Close("_TestFramework", "-close", "Makes the application close automatically after the tests are finished.", false);
xiiCommandLineOptionBool   opt_NoGui("_TestFramework", "-noGui", "Never show a GUI.", false);
xiiCommandLineOptionBool   opt_HTML("_TestFramework", "-html", "Open summary HTML on error.", false);
xiiCommandLineOptionBool   opt_Console("_TestFramework", "-console", "Keep the console open.", false);
xiiCommandLineOptionBool   opt_Timestamps("_TestFramework", "-timestamps", "Show timestamps in logs.", false);
xiiCommandLineOptionBool   opt_MsgBox("_TestFramework", "-msgbox", "Show message box after tests.", false);
xiiCommandLineOptionBool   opt_DisableSuccessful("_TestFramework", "-disableSuccessful", "Disable tests that ran successfully.", false);
xiiCommandLineOptionBool   opt_EnableAllTests("_TestFramework", "-all", "Enable all tests.", false);
xiiCommandLineOptionBool   opt_NoSave("_TestFramework", "-noSave", "Disables saving of any state.", false);
xiiCommandLineOptionInt    opt_Revision("_TestFramework", "-rev", "Revision number to pass through to JSON output.", -1);
xiiCommandLineOptionInt    opt_Passes("_TestFramework", "-passes", "Number of passes to execute.", 1);
xiiCommandLineOptionInt    opt_Assert("_TestFramework", "-assert", "Whether to assert when a test fails.", (int)AssertOnTestFail::AssertIfDebuggerAttached);
xiiCommandLineOptionString opt_Filter("_TestFramework", "-filter", "Filter to execute only certain tests.", "");
xiiCommandLineOptionPath   opt_Json("_TestFramework", "-json", "JSON file to write.", "");
xiiCommandLineOptionPath   opt_OutputDir("_TestFramework", "-outputDir", "Output directory", "");

constexpr int s_iMaxErrorMessageLength = 512;

static bool TestAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (xiiTestFramework::s_bCallstackOnAssert)
  {
    void*              pBuffer[64];
    xiiArrayPtr<void*> tempTrace(pBuffer);
    const xiiUInt32    uiNumTraces = xiiStackTracer::GetStackTrace(tempTrace, nullptr);
    xiiStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &xiiLog::Print);
  }

  xiiTestFramework::Error(szExpression, szSourceFile, (xiiInt32)uiLine, szFunction, szAssertMsg);

  // if a debugger is attached, one typically always wants to know about asserts
  if (xiiSystemInformation::IsDebuggerAttached())
    return true;

  xiiTestFramework::GetInstance()->AbortTests();

  return xiiTestFramework::GetAssertOnTestFail();
}

////////////////////////////////////////////////////////////////////////
// xiiTestFramework public functions
////////////////////////////////////////////////////////////////////////

xiiTestFramework::xiiTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv) :
  m_sTestName(szTestName), m_sAbsTestOutputDir(szAbsTestOutputDir), m_sRelTestDataDir(szRelTestDataDir)
{
  s_pInstance = this;

  xiiCommandLineUtils::GetGlobalInstance()->SetCommandLine(iArgc, pArgv, xiiCommandLineUtils::PreferOsArgs);

  GetTestSettingsFromCommandLine(*xiiCommandLineUtils::GetGlobalInstance());
}

xiiTestFramework::~xiiTestFramework()
{
  if (m_bIsInitialized)
    DeInitialize();
  s_pInstance = nullptr;
}

void xiiTestFramework::Initialize()
{
  {
    xiiStringBuilder cmdHelp;
    if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_TestFramework;cvar"))
    {
      // make sure the console stays open
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-console");
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("true");

      xiiLog::Print(cmdHelp);
    }
  }

  if (m_Settings.m_bNoGUI)
  {
    // if the UI is run with GUI disabled, set the environment variable XII_SILENT_ASSERTS
    // to make sure that no child process that the tests launch shows an assert dialog in case of a crash
    if (xiiEnvironmentVariableUtils::SetValueInt("XII_SILENT_ASSERTS", 1).Failed())
    {
      xiiLog::Print("Failed to set 'XII_SILENT_ASSERTS' environment variable!");
    }
  }

  if (m_Settings.m_bShowTimestampsInLog)
  {
    xiiTestFramework::s_LogTimestampMode = xiiLog::TimestampMode::TimeOnly;
    xiiLogWriter::Console::SetTimestampMode(xiiLog::TimestampMode::TimeOnly);
  }

  // Don't do this, it will spam the log with sub-system messages
  // xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  // xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  xiiStartup::AddApplicationTag("testframework");
  xiiStartup::StartupCoreSystems();
  XII_SCOPE_EXIT(xiiStartup::ShutdownCoreSystems());

  // if tests need to write data back through Fileserve (e.g. image comparison results), they can do that through a data dir mounted with
  // this path
  xiiFileSystem::SetSpecialDirectory("xiitest", xiiTestFramework::GetInstance()->GetAbsOutputPath());

  // Setting XII assert handler
  m_PreviousAssertHandler = xiiGetAssertHandler();
  xiiSetAssertHandler(TestAssertHandler);

  CreateOutputFolder();
  xiiFileSystem::DetectSdkRootDirectory().IgnoreResult();

  xiiCommandLineUtils& cmd = *xiiCommandLineUtils::GetGlobalInstance();
  // figure out which tests exist
  GatherAllTests();

  if (!m_Settings.m_bNoGUI || opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    // load the test order from file, if that file does not exist, the array is not modified.
    LoadTestOrder();
  }
  ApplyTestOrderFromCommandLine(cmd);

  if (!m_Settings.m_bNoGUI || opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    // Load the test settings from file, if that file does not exist, the settings are not modified.
    LoadTestSettings();
    // Overwrite loaded test settings with command line
    GetTestSettingsFromCommandLine(cmd);
  }

  // save the current order back to the same file
  AutoSaveTestOrder();

  m_bIsInitialized = true;
}

void xiiTestFramework::DeInitialize()
{
  m_bIsInitialized = false;

  xiiSetAssertHandler(m_PreviousAssertHandler);
  m_PreviousAssertHandler = nullptr;
}

const char* xiiTestFramework::GetTestName() const
{
  return m_sTestName.c_str();
}

const char* xiiTestFramework::GetAbsOutputPath() const
{
  return m_sAbsTestOutputDir.c_str();
}


const char* xiiTestFramework::GetRelTestDataPath() const
{
  return m_sRelTestDataDir.c_str();
}

const char* xiiTestFramework::GetAbsTestOrderFilePath() const
{
  return m_sAbsTestOrderFilePath.c_str();
}

const char* xiiTestFramework::GetAbsTestSettingsFilePath() const
{
  return m_sAbsTestSettingsFilePath.c_str();
}

void xiiTestFramework::RegisterOutputHandler(OutputHandler handler)
{
  // do not register a handler twice
  for (xiiUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    if (m_OutputHandlers[i] == handler)
      return;
  }

  m_OutputHandlers.push_back(handler);
}


void xiiTestFramework::SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider)
{
  m_ImageDiffExtraInfoCallback = provider;
}

bool xiiTestFramework::GetAssertOnTestFail()
{
  switch (s_pInstance->m_Settings.m_AssertOnTestFail)
  {
    case AssertOnTestFail::DoNotAssert:
      return false;
    case AssertOnTestFail::AssertIfDebuggerAttached:
      return xiiSystemInformation::IsDebuggerAttached();
    case AssertOnTestFail::AlwaysAssert:
      return true;
  }
  return false;
}

void xiiTestFramework::GatherAllTests()
{
  m_TestEntries.clear();

  m_iErrorCount         = 0;
  m_iTestsFailed        = 0;
  m_iTestsPassed        = 0;
  m_uiExecutingTest     = xiiInvalidIndex;
  m_uiExecutingSubTest  = xiiInvalidIndex;
  m_bSubTestInitialized = false;

  // first let all simple tests register themselves
  {
    xiiRegisterSimpleTestHelper* pHelper = xiiRegisterSimpleTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  xiiTestConfiguration config;
  xiiTestBaseClass*    pTestClass = xiiTestBaseClass::GetFirstInstance();

  while (pTestClass)
  {
    pTestClass->ClearSubTests();
    pTestClass->SetupSubTests();
    pTestClass->UpdateConfiguration(config);

    xiiTestEntry e;
    e.m_pTest               = pTestClass;
    e.m_szTestName          = pTestClass->GetTestName();
    e.m_sNotAvailableReason = pTestClass->IsTestAvailable();

    for (xiiUInt32 i = 0; i < pTestClass->m_Entries.size(); ++i)
    {
      xiiSubTestEntry st;
      st.m_szSubTestName      = pTestClass->m_Entries[i].m_szName;
      st.m_iSubTestIdentifier = pTestClass->m_Entries[i].m_iIdentifier;

      e.m_SubTests.push_back(st);
    }

    m_TestEntries.push_back(e);

    pTestClass = pTestClass->GetNextInstance();
  }
  ::SortTestsAlphabetically(m_TestEntries);

  m_Result.SetupTests(m_TestEntries, config);
}

void xiiTestFramework::GetTestSettingsFromCommandLine(const xiiCommandLineUtils& cmd)
{
  // use a local instance of xiiCommandLineUtils as global instance is not guaranteed to have been set up
  // for all call sites of this method.

  m_Settings.m_bRunTests       = opt_Run.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bCloseOnSuccess = opt_Close.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bNoGUI          = opt_NoGui.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  if (opt_Assert.IsOptionSpecified(nullptr, &cmd))
  {
    const int assertOnTestFailure = opt_Assert.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
    switch (assertOnTestFailure)
    {
      case 0:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::DoNotAssert;
        break;
      case 1:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AssertIfDebuggerAttached;
        break;
      case 2:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AlwaysAssert;
        break;
    }
  }

  xiiStringBuilder tmp;

  opt_HTML.SetDefaultValue(m_Settings.m_bOpenHtmlOutputOnError);
  m_Settings.m_bOpenHtmlOutputOnError = opt_HTML.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_Console.SetDefaultValue(m_Settings.m_bKeepConsoleOpen);
  m_Settings.m_bKeepConsoleOpen = opt_Console.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_Timestamps.SetDefaultValue(m_Settings.m_bShowTimestampsInLog);
  m_Settings.m_bShowTimestampsInLog = opt_Timestamps.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_MsgBox.SetDefaultValue(m_Settings.m_bShowMessageBox);
  m_Settings.m_bShowMessageBox = opt_MsgBox.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_DisableSuccessful.SetDefaultValue(m_Settings.m_bAutoDisableSuccessfulTests);
  m_Settings.m_bAutoDisableSuccessfulTests = opt_DisableSuccessful.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  m_Settings.m_iRevision       = opt_Revision.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bEnableAllTests = opt_EnableAllTests.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_uiFullPasses    = static_cast<xiiUInt8>(opt_Passes.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd));
  m_Settings.m_sTestFilter     = opt_Filter.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd).GetData(tmp);

  if (opt_Json.IsOptionSpecified(nullptr, &cmd))
  {
    m_Settings.m_sJsonOutput = opt_Json.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  if (opt_OutputDir.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOutputDir = opt_OutputDir.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  bool bNoAutoSave = false;
  if (opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOrderFilePath = opt_OrderFile.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    // If a custom order file was provided, default to -nosave as to not overwrite that file with additional
    // parameters from command line. Use "-nosave false" to explicitly enable auto save in this case.
    bNoAutoSave = true;
  }
  else
  {
    m_sAbsTestOrderFilePath = m_sAbsTestOutputDir + std::string("/TestOrder.txt");
  }

  if (opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestSettingsFilePath = opt_SettingsFile.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    // If a custom settings file was provided, default to -nosave as to not overwrite that file with additional
    // parameters from command line. Use "-nosave false" to explicitly enable auto save in this case.
    bNoAutoSave = true;
  }
  else
  {
    m_sAbsTestSettingsFilePath = m_sAbsTestOutputDir + std::string("/TestSettings.txt");
  }
  opt_NoSave.SetDefaultValue(bNoAutoSave);
  m_Settings.m_bNoAutomaticSaving = opt_NoSave.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  m_uiPassesLeft = m_Settings.m_uiFullPasses;
}

void xiiTestFramework::LoadTestOrder()
{
  ::LoadTestOrder(m_sAbsTestOrderFilePath.c_str(), m_TestEntries);
}

void xiiTestFramework::ApplyTestOrderFromCommandLine(const xiiCommandLineUtils& cmd)
{
  if (m_Settings.m_bEnableAllTests)
    SetAllTestsEnabledStatus(true);
  if (!m_Settings.m_sTestFilter.empty())
  {
    const xiiUInt32 uiTestCount = GetTestCount();
    for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
    {
      const bool bEnable                     = xiiStringUtils::FindSubString_NoCase(m_TestEntries[uiTestIdx].m_szTestName, m_Settings.m_sTestFilter.c_str()) != nullptr;
      m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
      const xiiUInt32 uiSubTestCount         = (xiiUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
      for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
      {
        m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
      }
    }
  }
}

void xiiTestFramework::LoadTestSettings()
{
  ::LoadTestSettings(m_sAbsTestSettingsFilePath.c_str(), m_Settings);
}

void xiiTestFramework::CreateOutputFolder()
{
  xiiOSFile::CreateDirectoryStructure(m_sAbsTestOutputDir.c_str()).IgnoreResult();

  XII_ASSERT_RELEASE(xiiOSFile::ExistsDirectory(m_sAbsTestOutputDir.c_str()), "Failed to create output directory '{0}'", m_sAbsTestOutputDir.c_str());
}

void xiiTestFramework::UpdateReferenceImages()
{
  xiiStringBuilder sDir;
  if (xiiFileSystem::ResolveSpecialDirectory(">sdk", sDir).Failed())
    return;

  sDir.AppendPath(GetRelTestDataPath());

  const xiiStringBuilder sNewFiles(m_sAbsTestOutputDir.c_str(), "/Images_Result");
  const xiiStringBuilder sRefFiles(sDir, "/", m_sImageReferenceFolderName.c_str());

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS)


#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiStringBuilder sOptiPng = xiiFileSystem::GetSdkRootDirectory();
  sOptiPng.AppendPath("Data/Tools/Precompiled/optipng/optipng.exe");

  if (xiiOSFile::ExistsFile(sOptiPng))
  {
    xiiStringBuilder sPath;

    xiiFileSystemIterator it;
    it.StartSearch(sNewFiles, xiiFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sPath);

      xiiProcessOptions opt;
      opt.m_sProcess = sOptiPng;
      opt.m_Arguments.PushBack(sPath);
      xiiProcess::Execute(opt).IgnoreResult();
    }
  }
#  endif

  // If some target files already exist somewhere (ie. custom folders for the tests), overwrite the existing files in their location
  {
    xiiHybridArray<xiiString, 32> targetFolders;
    xiiStringBuilder              sFullPath, sTargetPath;

    {
      xiiFileSystemIterator it;
      it.StartSearch(sDir, xiiFileSystemIteratorFlags::ReportFoldersRecursive);
      for (; it.IsValid(); it.Next())
      {
        if (it.GetStats().m_sName == m_sImageReferenceFolderName.c_str())
        {
          it.GetStats().GetFullPath(sFullPath);

          targetFolders.PushBack(sFullPath);
        }
      }
    }

    xiiFileSystemIterator it;
    it.StartSearch(sNewFiles, xiiFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sFullPath);

      for (xiiUInt32 i = 0; i < targetFolders.GetCount(); ++i)
      {
        sTargetPath = targetFolders[i];
        sTargetPath.AppendPath(it.GetStats().m_sName);

        if (xiiOSFile::ExistsFile(sTargetPath))
        {
          xiiOSFile::DeleteFile(sTargetPath).IgnoreResult();
          xiiOSFile::MoveFileOrDirectory(sFullPath, sTargetPath).IgnoreResult();
          break;
        }
      }
    }
  }

  // Copy the remaining files to the default directory.
  xiiOSFile::CopyFolder(sNewFiles, sRefFiles).IgnoreResult();
  xiiOSFile::DeleteFolder(sNewFiles).IgnoreResult();
#endif
}

void xiiTestFramework::AutoSaveTestOrder()
{
  if (m_Settings.m_bNoAutomaticSaving)
    return;

  SaveTestOrder(m_sAbsTestOrderFilePath.c_str());
  SaveTestSettings(m_sAbsTestSettingsFilePath.c_str());
}

void xiiTestFramework::SaveTestOrder(const char* const szFilePath)
{
  ::SaveTestOrder(szFilePath, m_TestEntries);
}

void xiiTestFramework::SaveTestSettings(const char* const szFilePath)
{
  ::SaveTestSettings(szFilePath, m_Settings);
}

void xiiTestFramework::SetAllTestsEnabledStatus(bool bEnable)
{
  const xiiUInt32 uiTestCount = GetTestCount();
  for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
    const xiiUInt32 uiSubTestCount         = (xiiUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
    }
  }
}

void xiiTestFramework::SetAllFailedTestsEnabledStatus()
{
  const auto& LastResult = GetTestResult();

  const xiiUInt32 uiTestCount = GetTestCount();
  for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    const auto& TestRes                    = LastResult.GetTestResultData(uiTestIdx, -1);
    m_TestEntries[uiTestIdx].m_bEnableTest = TestRes.m_bExecuted && !TestRes.m_bSuccess;

    const xiiUInt32 uiSubTestCount = (xiiUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      const auto& SubTestRes                                       = LastResult.GetTestResultData(uiTestIdx, uiSubTest);
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = SubTestRes.m_bExecuted && !SubTestRes.m_bSuccess;
    }
  }
}

void xiiTestFramework::SetTestTimeout(xiiUInt32 uiTestTimeoutMS)
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    m_uiTimeoutMS = uiTestTimeoutMS;
  }
  UpdateTestTimeout();
}

xiiUInt32 xiiTestFramework::GetTestTimeout() const
{
  return m_uiTimeoutMS;
}

void xiiTestFramework::TimeoutThread()
{
  std::unique_lock<std::mutex> lock(m_TimeoutLock);
  while (m_bUseTimeout)
  {
    if (m_uiTimeoutMS == 0)
    {
      // If no timeout is set, we simply put the thread to sleep.
      m_TimeoutCV.wait(lock, [this] { return !m_bUseTimeout; });
    }
    // We want to be notified when we reach the timeout and not when we are spuriously woken up.
    // Thus we continue waiting via the predicate if we are still using a timeout until we are either
    // woken up via the CV or reach the timeout.
    else if (!m_TimeoutCV.wait_for(lock, std::chrono::milliseconds(m_uiTimeoutMS), [this] { return !m_bUseTimeout || m_bArm; }))
    {
      if (xiiSystemInformation::IsDebuggerAttached())
      {
        // Should we attach a debugger mid run and reach the timeout we obviously do not want to terminate.
        continue;
      }

      // CV was not signaled until the timeout was reached.
      xiiTestFramework::Output(xiiTestOutput::Error, "Timeout reached, terminating app.");
      // The top level exception handler takes care of all the shutdown logic already (app specific logic, crash dump, callstack etc)
      // which we do not want to duplicate here so we simply throw an unhandled exception.
      throw std::runtime_error("Timeout reached, terminating app.");
    }
    m_bArm = false;
  }
}


void xiiTestFramework::UpdateTestTimeout()
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    if (!m_bUseTimeout)
    {
      return;
    }
    m_bArm = true;
  }
  m_TimeoutCV.notify_one();
}

void xiiTestFramework::ResetTests()
{
  m_iErrorCount         = 0;
  m_iTestsFailed        = 0;
  m_iTestsPassed        = 0;
  m_uiExecutingTest     = xiiInvalidIndex;
  m_uiExecutingSubTest  = xiiInvalidIndex;
  m_bSubTestInitialized = false;
  m_bAbortTests         = false;

  m_Result.Reset();
}

xiiTestAppRun xiiTestFramework::RunTestExecutionLoop()
{
  if (!m_bIsInitialized)
  {
    Initialize();

#ifdef XII_TESTFRAMEWORK_USE_FILESERVE
    if (xiiFileserveClient::GetSingleton() == nullptr)
    {
      XII_DEFAULT_NEW(xiiFileserveClient);

      if (xiiFileserveClient::GetSingleton()->SearchForServerAddress().Failed())
      {
        xiiFileserveClient::GetSingleton()->WaitForServerInfo().IgnoreResult();
      }
    }

    if (xiiFileserveClient::GetSingleton()->EnsureConnected(xiiTime::MakeFromSeconds(-30)).Failed())
    {
      Error("Failed to establish a Fileserve connection", "", 0, "xiiTestFramework::RunTestExecutionLoop", "");
      return xiiTestAppRun::Quit;
    }
#endif
  }

#ifdef XII_TESTFRAMEWORK_USE_FILESERVE
  xiiFileserveClient::GetSingleton()->UpdateClient();
#endif


  if (m_uiExecutingTest == xiiInvalidIndex)
  {
    StartTests();
    m_uiExecutingTest = 0;
    XII_ASSERT_DEV(m_uiExecutingSubTest == xiiInvalidIndex, "Invalid test framework state");
    XII_ASSERT_DEV(!m_bSubTestInitialized, "Invalid test framework state");
  }

  ExecuteNextTest();

  if (m_uiExecutingTest >= (xiiUInt32)m_TestEntries.size())
  {
    EndTests();

    if (m_uiPassesLeft > 1 && !m_bAbortTests)
    {
      --m_uiPassesLeft;

      m_uiExecutingTest    = xiiInvalidIndex;
      m_uiExecutingSubTest = xiiInvalidIndex;

      return xiiTestAppRun::Continue;
    }

#ifdef XII_TESTFRAMEWORK_USE_FILESERVE
    if (xiiFileserveClient* pClient = xiiFileserveClient::GetSingleton())
    {
      // shutdown the fileserve client
      XII_DEFAULT_DELETE(pClient);
    }
#endif

    return xiiTestAppRun::Quit;
  }

  return xiiTestAppRun::Continue;
}

void xiiTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;
  xiiTestFramework::Output(xiiTestOutput::StartOutput, "");

  // Start timeout thread.
  std::scoped_lock lock(m_TimeoutLock);
  m_bUseTimeout   = true;
  m_bArm          = false;
  m_TimeoutThread = std::thread(&xiiTestFramework::TimeoutThread, this);
}

// Redirects engine warnings / errors to test-framework output
static void LogWriter(const xiiLoggingEventData& e)
{
  const xiiStringBuilder sText = e.m_sText;

  switch (e.m_EventType)
  {
    case xiiLogMsgType::ErrorMsg:
      xiiTestFramework::Output(xiiTestOutput::Error, "xiiLog Error: %s", sText.GetData());
      break;
    case xiiLogMsgType::SeriousWarningMsg:
      xiiTestFramework::Output(xiiTestOutput::Error, "xiiLog Serious Warning: %s", sText.GetData());
      break;
    case xiiLogMsgType::WarningMsg:
      xiiTestFramework::Output(xiiTestOutput::Warning, "xiiLog Warning: %s", sText.GetData());
      break;
    case xiiLogMsgType::InfoMsg:
    case xiiLogMsgType::DevMsg:
    case xiiLogMsgType::DebugMsg:
    {
      if (e.m_sTag.IsEqual_NoCase("test"))
        xiiTestFramework::Output(xiiTestOutput::Details, sText.GetData());
    }
    break;

    default:
      return;
  }
}

void xiiTestFramework::ExecuteNextTest()
{
  XII_ASSERT_DEV(m_uiExecutingTest >= 0, "Invalid current test.");

  if (m_uiExecutingTest == (xiiUInt32)GetTestCount())
    return;

  if (!m_TestEntries[m_uiExecutingTest].m_bEnableTest)
  {
    // next time run the next test and start with the first subtest
    m_uiExecutingTest++;
    m_uiExecutingSubTest = xiiInvalidIndex;
    return;
  }

  xiiTestEntry&     TestEntry  = m_TestEntries[m_uiExecutingTest];
  xiiTestBaseClass* pTestClass = m_TestEntries[m_uiExecutingTest].m_pTest;

  // Execute test
  {
    if (m_uiExecutingSubTest == xiiInvalidIndex) // no subtest has run yet, so initialize the test first
    {
      if (m_bAbortTests)
      {
        m_uiExecutingTest    = (xiiUInt32)m_TestEntries.size(); // skip to the end of all tests
        m_uiExecutingSubTest = xiiInvalidIndex;
        return;
      }

      m_uiExecutingSubTest = 0;
      m_fTotalTestDuration = 0.0;

      // Reset assert counter. This variable is used to reduce the overhead of counting millions of asserts.
      s_iAssertCounter     = 0;
      m_uiCurrentTestIndex = m_uiExecutingTest;
      // Log writer translates engine warnings / errors into test framework error messages.
      xiiGlobalLog::AddLogWriter(LogWriter);

      m_iErrorCountBeforeTest = GetTotalErrorCount();

      xiiTestFramework::Output(xiiTestOutput::BeginBlock, "Executing Test: '%s'", TestEntry.m_szTestName);

      // *** Test Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        UpdateTestTimeout();
        if (pTestClass->DoTestInitialization().Failed())
        {
          m_uiExecutingSubTest = (xiiUInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
        }
      }
      else
      {
        xiiTestFramework::Output(xiiTestOutput::ImportantInfo, "Test not available: %s", TestEntry.m_sNotAvailableReason.c_str());
        m_uiExecutingSubTest = (xiiUInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
      }
    }

    if (m_uiExecutingSubTest < (xiiUInt32)TestEntry.m_SubTests.size())
    {
      xiiSubTestEntry& subTest            = TestEntry.m_SubTests[m_uiExecutingSubTest];
      xiiInt32         iSubTestIdentifier = subTest.m_iSubTestIdentifier;

      if (!subTest.m_bEnableTest)
      {
        ++m_uiExecutingSubTest;
        return;
      }

      if (!m_bSubTestInitialized)
      {
        if (m_bAbortTests)
        {
          // tests shall be aborted, so do not start a new one

          m_uiExecutingTest    = (xiiInt32)m_TestEntries.size(); // skip to the end of all tests
          m_uiExecutingSubTest = xiiInvalidIndex;
          return;
        }

        m_fTotalSubTestDuration    = 0.0;
        m_uiSubTestInvocationCount = 0;

        // First flush of assert counter, these are all asserts during test init.
        FlushAsserts();
        m_uiCurrentSubTestIndex = m_uiExecutingSubTest;
        xiiTestFramework::Output(xiiTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);

        // *** Sub-Test Initialization ***
        UpdateTestTimeout();
        m_bSubTestInitialized = pTestClass->DoSubTestInitialization(iSubTestIdentifier).Succeeded();
      }

      xiiTestAppRun subTestResult = xiiTestAppRun::Quit;

      if (m_bSubTestInitialized)
      {
        // *** Run Sub-Test ***
        double fDuration = 0.0;

        // start with 1
        ++m_uiSubTestInvocationCount;

        UpdateTestTimeout();
        subTestResult     = pTestClass->DoSubTestRun(iSubTestIdentifier, fDuration, m_uiSubTestInvocationCount);
        s_szTestBlockName = "";

        if (m_bImageComparisonScheduled)
        {
          XII_TEST_IMAGE(m_uiComparisonImageNumber, m_uiMaxImageComparisonError);
          m_bImageComparisonScheduled = false;
        }


        if (m_bDepthImageComparisonScheduled)
        {
          XII_TEST_DEPTH_IMAGE(m_uiComparisonDepthImageNumber, m_uiMaxDepthImageComparisonError);
          m_bDepthImageComparisonScheduled = false;
        }

        // I guess we can require that tests are written in a way that they can be interrupted
        if (m_bAbortTests)
          subTestResult = xiiTestAppRun::Quit;

        m_fTotalSubTestDuration += fDuration;
      }

      // this is executed when sub-test initialization failed or the sub-test reached its end
      if (subTestResult == xiiTestAppRun::Quit)
      {
        // *** Sub-Test De-Initialization ***
        UpdateTestTimeout();
        pTestClass->DoSubTestDeInitialization(iSubTestIdentifier);

        bool bSubTestSuccess = m_bSubTestInitialized && (m_Result.GetErrorMessageCount(m_uiExecutingTest, m_uiExecutingSubTest) == 0);
        xiiTestFramework::TestResult(m_uiExecutingSubTest, bSubTestSuccess, m_fTotalSubTestDuration);

        m_fTotalTestDuration += m_fTotalSubTestDuration;

        // advance to the next (sub) test
        m_bSubTestInitialized = false;
        ++m_uiExecutingSubTest;

        // Second flush of assert counter, these are all asserts for the current subtest.
        FlushAsserts();
        xiiTestFramework::Output(xiiTestOutput::EndBlock, "");
        m_uiCurrentSubTestIndex = xiiInvalidIndex;
      }
    }

    if (m_bAbortTests || m_uiExecutingSubTest >= (xiiInt32)TestEntry.m_SubTests.size())
    {
      // *** Test De-Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        // We only call DoTestInitialization under this condition so DoTestDeInitialization must be guarded by the same.
        UpdateTestTimeout();
        pTestClass->DoTestDeInitialization();
      }
      // Third and last flush of assert counter, these are all asserts for the test de-init.
      FlushAsserts();

      xiiGlobalLog::RemoveLogWriter(LogWriter);

      bool bTestSuccess = m_iErrorCountBeforeTest == GetTotalErrorCount();
      xiiTestFramework::TestResult(-1, bTestSuccess, m_fTotalTestDuration);
      xiiTestFramework::Output(xiiTestOutput::EndBlock, "");
      m_uiCurrentTestIndex = xiiInvalidIndex;

      // advance to the next test
      m_uiExecutingTest++;
      m_uiExecutingSubTest = xiiInvalidIndex;
    }
  }
}

void xiiTestFramework::EndTests()
{
  m_bTestsRunning = false;

  if (GetTestsPassedCount() + GetTestsFailedCount() == 0)
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "No tests were run. The -filter option may not have matched any tests.");
    ++m_iTestsFailed;
  }

  if (GetTestsFailedCount() == 0)
  {
    xiiTestFramework::Output(xiiTestOutput::FinalResult, "All tests passed.");
  }
  else
  {
    xiiTestFramework::Output(xiiTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", GetTestsFailedCount(), GetTestsPassedCount());
  }

  if (!m_Settings.m_sJsonOutput.empty())
  {
    m_Result.WriteJsonToFile(m_Settings.m_sJsonOutput.c_str());
  }

  m_uiExecutingTest    = xiiInvalidIndex;
  m_uiExecutingSubTest = xiiInvalidIndex;
  m_bAbortTests        = false;

  // Stop timeout thread.
  {
    std::scoped_lock lock(m_TimeoutLock);
    m_bUseTimeout = false;
    m_TimeoutCV.notify_one();
  }
  m_TimeoutThread.join();
}

void xiiTestFramework::AbortTests()
{
  m_bAbortTests = true;
}

xiiUInt32 xiiTestFramework::GetTestCount() const
{
  return (xiiUInt32)m_TestEntries.size();
}

xiiUInt32 xiiTestFramework::GetTestEnabledCount() const
{
  xiiUInt32       uiEnabledCount = 0;
  const xiiUInt32 uiTests        = GetTestCount();
  for (xiiUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    uiEnabledCount += m_TestEntries[uiTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

xiiUInt32 xiiTestFramework::GetSubTestEnabledCount(xiiUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return 0;

  xiiUInt32       uiEnabledCount = 0;
  const xiiUInt32 uiSubTests     = (xiiUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    uiEnabledCount += m_TestEntries[uiTestIndex].m_SubTests[uiSubTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

const std::string& xiiTestFramework::IsTestAvailable(xiiUInt32 uiTestIndex) const
{
  XII_ASSERT_DEV(uiTestIndex < GetTestCount(), "Test index {0} is larger than number of tests {1}.", uiTestIndex, GetTestCount());
  return m_TestEntries[uiTestIndex].m_sNotAvailableReason;
}

bool xiiTestFramework::IsTestEnabled(xiiUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  return m_TestEntries[uiTestIndex].m_bEnableTest;
}

bool xiiTestFramework::IsSubTestEnabled(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  const xiiUInt32 uiSubTests = (xiiUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return false;

  return m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest;
}

void xiiTestFramework::SetTestEnabled(xiiUInt32 uiTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  m_TestEntries[uiTestIndex].m_bEnableTest = bEnabled;
}

void xiiTestFramework::SetSubTestEnabled(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  const xiiUInt32 uiSubTests = (xiiUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return;

  m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = bEnabled;
}

xiiInt32 xiiTestFramework::GetCurrentSubTestIdentifier() const
{
  return GetCurrentSubTest()->m_iSubTestIdentifier;
}

xiiUInt32 xiiTestFramework::FindSubTestIndexForSubTestIdentifier(xiiInt32 iSubTestIdentifier) const
{
  const xiiTestEntry* pTest = GetCurrentTest();

  const xiiUInt32 uiSubTests = (xiiUInt32)pTest->m_SubTests.size();
  for (xiiUInt32 i = 0; i < uiSubTests; ++i)
  {
    if (pTest->m_SubTests[i].m_iSubTestIdentifier == iSubTestIdentifier)
      return i;
  }

  return xiiInvalidIndex;
}

xiiTestEntry* xiiTestFramework::GetTest(xiiUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const xiiTestEntry* xiiTestFramework::GetTest(xiiUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const xiiTestEntry* xiiTestFramework::GetCurrentTest() const
{
  return GetTest(GetCurrentTestIndex());
}

const xiiSubTestEntry* xiiTestFramework::GetCurrentSubTest() const
{
  if (auto pTest = GetCurrentTest())
  {
    if (m_uiCurrentSubTestIndex >= (xiiInt32)pTest->m_SubTests.size())
      return nullptr;

    return &pTest->m_SubTests[m_uiCurrentSubTestIndex];
  }

  return nullptr;
}

TestSettings xiiTestFramework::GetSettings() const
{
  return m_Settings;
}

void xiiTestFramework::SetSettings(const TestSettings& settings)
{
  m_Settings = settings;
}

xiiTestFrameworkResult& xiiTestFramework::GetTestResult()
{
  return m_Result;
}

xiiInt32 xiiTestFramework::GetTotalErrorCount() const
{
  return m_iErrorCount;
}

xiiInt32 xiiTestFramework::GetTestsPassedCount() const
{
  return m_iTestsPassed;
}

xiiInt32 xiiTestFramework::GetTestsFailedCount() const
{
  return m_iTestsFailed;
}

double xiiTestFramework::GetTotalTestDuration() const
{
  return m_Result.GetTotalTestDuration();
}

////////////////////////////////////////////////////////////////////////
// xiiTestFramework protected functions
////////////////////////////////////////////////////////////////////////

static bool g_bBlockOutput = false;

void xiiTestFramework::OutputImpl(xiiTestOutput::Enum Type, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  if (Type == xiiTestOutput::Error)
  {
    m_iErrorCount++;
  }
  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (xiiUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    m_OutputHandlers[i](Type, szMsg);
  }

  if (g_bBlockOutput)
    return;

  m_Result.TestOutput(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, Type, szMsg);
}

void xiiTestFramework::ErrorImpl(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestError(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, szError, xiiTestFramework::s_szTestBlockName, szFile, iLine, szFunction, szMsg);

  g_bBlockOutput = true;
  xiiTestFramework::Output(xiiTestOutput::Error, "%s", szError); // This will also increase the global error count.
  xiiTestFramework::Output(xiiTestOutput::BeginBlock, "");
  {
    if ((xiiTestFramework::s_szTestBlockName != nullptr) && (xiiTestFramework::s_szTestBlockName[0] != '\0'))
      xiiTestFramework::Output(xiiTestOutput::Message, "Block: '%s'", xiiTestFramework::s_szTestBlockName);

    xiiTestFramework::Output(xiiTestOutput::ImportantInfo, "File: %s", szFile);
    xiiTestFramework::Output(xiiTestOutput::ImportantInfo, "Line: %i", iLine);
    xiiTestFramework::Output(xiiTestOutput::ImportantInfo, "Function: %s", szFunction);

    if ((szMsg != nullptr) && (szMsg[0] != '\0'))
      xiiTestFramework::Output(xiiTestOutput::Message, "Error: %s", szMsg);
  }
  xiiTestFramework::Output(xiiTestOutput::EndBlock, "");
  g_bBlockOutput = false;
}

void xiiTestFramework::TestResultImpl(xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestResult(m_uiCurrentTestIndex, uiSubTestIndex, bSuccess, fDuration);

  const xiiUInt32 uiMin = (xiiUInt32)(fDuration / 1000.0 / 60.0);
  const xiiUInt32 uiSec = (xiiUInt32)(fDuration / 1000.0 - uiMin * 60.0);
  const xiiUInt32 uiMS  = (xiiUInt32)(fDuration - uiSec * 1000.0);

  xiiTestFramework::Output(xiiTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  if (uiSubTestIndex == xiiInvalidIndex)
  {
    const char* szTestName = m_TestEntries[m_uiCurrentTestIndex].m_szTestName;
    if (bSuccess)
    {
      m_iTestsPassed++;
      xiiTestFramework::Output(xiiTestOutput::Success, "Test '%s' succeeded (%.2f sec).", szTestName, m_fTotalTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_uiCurrentTestIndex].m_bEnableTest = false;
        xiiTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      m_iTestsFailed++;
      xiiTestFramework::Output(xiiTestOutput::Error, "Test '%s' failed: %i Errors (%.2f sec).", szTestName, (xiiUInt32)m_Result.GetErrorMessageCount(m_uiCurrentTestIndex, uiSubTestIndex), m_fTotalTestDuration / 1000.0f);
    }
  }
  else
  {
    const char* szSubTestName = m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex].m_szSubTestName;
    if (bSuccess)
    {
      xiiTestFramework::Output(xiiTestOutput::Success, "Sub-Test '%s' succeeded (%.2f sec).", szSubTestName, m_fTotalSubTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = false;
        xiiTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      xiiTestFramework::Output(xiiTestOutput::Error, "Sub-Test '%s' failed: %i Errors (%.2f sec).", szSubTestName, (xiiUInt32)m_Result.GetErrorMessageCount(m_uiCurrentTestIndex, uiSubTestIndex), m_fTotalSubTestDuration / 1000.0f);
    }
  }
}

void xiiTestFramework::SetSubTestStatusImpl(xiiUInt32 uiSubTestIndex, const char* szStatus)
{
  std::scoped_lock _(m_OutputMutex);

  if (m_uiCurrentTestIndex != xiiInvalidIndex && uiSubTestIndex != xiiInvalidIndex)
  {
    const xiiSubTestEntry& subtest = m_TestEntries[m_uiCurrentTestIndex].m_SubTests[uiSubTestIndex];

    m_Result.SetCustomStatus(m_uiCurrentTestIndex, uiSubTestIndex, szStatus);

    if (!xiiStringUtils::IsNullOrEmpty(szStatus))
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Status of sub-test '%s': %s.", subtest.m_szSubTestName, szStatus);
    }
  }
}

void xiiTestFramework::FlushAsserts()
{
  std::scoped_lock _(m_OutputMutex);
  m_Result.AddAsserts(m_uiCurrentTestIndex, m_uiCurrentSubTestIndex, s_iAssertCounter);
  s_iAssertCounter = 0;
}

void xiiTestFramework::ScheduleImageComparison(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError)
{
  m_bImageComparisonScheduled = true;
  m_uiMaxImageComparisonError = uiMaxError;
  m_uiComparisonImageNumber   = uiImageNumber;
}

void xiiTestFramework::ScheduleDepthImageComparison(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError)
{
  m_bDepthImageComparisonScheduled = true;
  m_uiMaxDepthImageComparisonError = uiMaxError;
  m_uiComparisonDepthImageNumber   = uiImageNumber;
}

void xiiTestFramework::GenerateComparisonImageName(xiiUInt32 uiImageNumber, xiiStringBuilder& ref_sImgName)
{
  xiiTestEntry* pMainTest = GetTest(GetCurrentTestIndex());

  const char*            szTestName = pMainTest->m_szTestName;
  const xiiSubTestEntry& subTest    = pMainTest->m_SubTests[GetCurrentSubTestIndex()];
  pMainTest->m_pTest->MapImageNumberToString(szTestName, subTest, uiImageNumber, ref_sImgName);
}

void xiiTestFramework::GetCurrentComparisonImageName(xiiStringBuilder& ref_sImgName)
{
  GenerateComparisonImageName(m_uiComparisonImageNumber, ref_sImgName);
}

void xiiTestFramework::SetImageReferenceFolderName(const char* szFolderName)
{
  m_sImageReferenceFolderName = szFolderName;
}

void xiiTestFramework::SetImageReferenceOverrideFolderName(const char* szFolderName)
{
  m_sImageReferenceOverrideFolderName = szFolderName;

  if (!m_sImageReferenceOverrideFolderName.empty())
  {
    Output(xiiTestOutput::Message, "Using ImageReference override folder '%s'", szFolderName);
  }
}

void xiiTestFramework::WriteImageDiffHtml(const char* szFileName, const xiiImage& referenceImgRgb, const xiiImage& referenceImgAlpha, const xiiImage& capturedImgRgb, const xiiImage& capturedImgAlpha, const xiiImage& diffImgRgb, const xiiImage& diffImgAlpha, xiiUInt32 uiError, xiiUInt32 uiThreshold, xiiUInt8 uiMinDiffRgb, xiiUInt8 uiMaxDiffRgb, xiiUInt8 uiMinDiffAlpha, xiiUInt8 uiMaxDiffAlpha)
{
  xiiFileWriter outputFile;
  if (outputFile.Open(szFileName).Failed())
  {
    xiiTestFramework::Output(xiiTestOutput::Warning, "Could not open HTML diff file \"%s\" for writing.", szFileName);
    return;
  }

  const char* szTestName    = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;

  xiiStringBuilder tmp(szTestName, " - ", szSubTestName);

  xiiStringBuilder output;
  xiiImageUtils::CreateImageDiffHtml(output, tmp, referenceImgRgb, referenceImgAlpha, capturedImgRgb, capturedImgAlpha, diffImgRgb, diffImgAlpha, uiError, uiThreshold, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

  if (m_ImageDiffExtraInfoCallback)
  {
    tmp.Clear();

    xiiDynamicArray<std::pair<xiiString, xiiString>> extraInfo = m_ImageDiffExtraInfoCallback();

    for (const auto& labelValuePair : extraInfo)
    {
      tmp.AppendFormat("<tr>\n"
                       "<td>{}:</td>\n"
                       "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                       "</tr>\n",
                       labelValuePair.first, labelValuePair.second);
    }

    output.ReplaceFirst("<!-- STATS-TABLE-START -->", tmp);
  }

  outputFile.WriteBytes(output.GetData(), output.GetElementCount()).AssertSuccess();
  outputFile.Close();
}

bool xiiTestFramework::PerformImageComparison(xiiStringBuilder sImgName, const xiiImage& img, xiiUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg)
{
  xiiImage imgRgba;
  if (xiiImageConversion::Convert(img, imgRgba, xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Captured Image '%s' could not be converted to RGBA8", sImgName.GetData());
    return false;
  }

  xiiStringBuilder sImgPathReference, sImgPathResult;

  if (!m_sImageReferenceOverrideFolderName.empty())
  {
    sImgPathReference = m_sImageReferenceOverrideFolderName.c_str();
    sImgPathReference.AppendPath(sImgName);
    sImgPathReference.ChangeFileExtension(".png");

    if (!xiiFileSystem::ExistsFile(sImgPathReference))
    {
      // try the regular path
      sImgPathReference.Clear();
    }
  }

  if (sImgPathReference.IsEmpty())
  {
    sImgPathReference = m_sImageReferenceFolderName.c_str();
    sImgPathReference.AppendPath(sImgName);
    sImgPathReference.ChangeFileExtension(".png");
  }

  sImgPathResult = ":imgout/Images_Result";
  sImgPathResult.AppendPath(sImgName);
  sImgPathResult.ChangeFileExtension(".png");

  auto SaveResultImage = [&]() {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    xiiStringBuilder sAbsPath;
    if (xiiFileSystem::ResolvePath(sImgPathResult, &sAbsPath, nullptr).Failed())
    {
      xiiLog::Warning("Failed to resolve absolute path of '{}'. Image will not be compressed with optipng.", sImgPathResult);
      return;
    }

    xiiStringBuilder sOptiPng = xiiFileSystem::GetSdkRootDirectory();
    sOptiPng.AppendPath("Data/Tools/Precompiled/optipng/optipng.exe");

    if (xiiOSFile::ExistsFile(sOptiPng))
    {
      xiiProcessOptions opt;
      opt.m_sProcess = sOptiPng;
      opt.m_Arguments.PushBack(sAbsPath);
      xiiInt32 iReturnCode = 0;
      if (xiiProcess::Execute(opt, &iReturnCode).Failed() || iReturnCode != 0)
      {
        xiiLog::Warning("Failed to run optipng with return code {}. Image will not be compressed with optipng.", iReturnCode);
      }
    }
#endif
  };

  // if a previous output image exists, get rid of it
  xiiFileSystem::DeleteFile(sImgPathResult);

  xiiImage imgExp, imgExpRgba;
  if (imgExp.LoadFrom(sImgPathReference).Failed())
  {
    SaveResultImage();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be read", sImgPathReference.GetData());
    return false;
  }

  if (xiiImageConversion::Convert(imgExp, imgExpRgba, xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    SaveResultImage();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be converted to RGBA8", sImgPathReference.GetData());
    return false;
  }

  if (imgRgba.GetWidth() != imgExpRgba.GetWidth() || imgRgba.GetHeight() != imgExpRgba.GetHeight())
  {
    SaveResultImage();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' size (%ix%i) does not match captured image size (%ix%i)", sImgPathReference.GetData(), imgExpRgba.GetWidth(), imgExpRgba.GetHeight(), imgRgba.GetWidth(), imgRgba.GetHeight());
    return false;
  }

  xiiImage imgDiffRgba;
  if (bIsLineImage)
    xiiImageUtils::ComputeImageDifferenceABSRelaxed(imgExpRgba, imgRgba, imgDiffRgba);
  else
    xiiImageUtils::ComputeImageDifferenceABS(imgExpRgba, imgRgba, imgDiffRgba);

  const xiiUInt32 uiMeanError = xiiImageUtils::ComputeMeanSquareError(imgDiffRgba, 32);

  if (uiMeanError > uiMaxError)
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

    xiiUInt8 uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha;
    xiiImageUtils::Normalize(imgDiffRgba, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    xiiImage imgDiffRgb;
    xiiImageConversion::Convert(imgDiffRgba, imgDiffRgb, xiiImageFormat::R8G8B8_UNORM).IgnoreResult();

    xiiStringBuilder sImgDiffName;
    sImgDiffName.SetFormat(":imgout/Images_Diff/{0}.png", sImgName);
    imgDiffRgb.SaveTo(sImgDiffName).IgnoreResult();

    xiiImage imgDiffAlpha;
    xiiImageUtils::ExtractAlphaChannel(imgDiffRgba, imgDiffAlpha);

    xiiStringBuilder sImgDiffAlphaName;
    sImgDiffAlphaName.SetFormat(":imgout/Images_Diff/{0}_alpha.png", sImgName);
    imgDiffAlpha.SaveTo(sImgDiffAlphaName).IgnoreResult();

    xiiImage imgExpRgb;
    xiiImageConversion::Convert(imgExpRgba, imgExpRgb, xiiImageFormat::R8G8B8_UNORM).IgnoreResult();
    xiiImage imgExpAlpha;
    xiiImageUtils::ExtractAlphaChannel(imgExpRgba, imgExpAlpha);

    xiiImage imgRgb;
    xiiImageConversion::Convert(imgRgba, imgRgb, xiiImageFormat::R8G8B8_UNORM).IgnoreResult();
    xiiImage imgAlpha;
    xiiImageUtils::ExtractAlphaChannel(imgRgba, imgAlpha);

    xiiStringBuilder sDiffHtmlPath;
    sDiffHtmlPath.SetFormat(":imgout/Html_Diff/{0}.html", sImgName);
    WriteImageDiffHtml(sDiffHtmlPath, imgExpRgb, imgExpAlpha, imgRgb, imgAlpha, imgDiffRgb, imgDiffAlpha, uiMeanError, uiMaxError, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Error: Image Comparison Failed: MSE of %u exceeds threshold of %u for image '%s'.", uiMeanError, uiMaxError, sImgName.GetData());

    xiiStringBuilder sDataDirRelativePath;
    xiiFileSystem::ResolvePath(sDiffHtmlPath, nullptr, &sDataDirRelativePath).IgnoreResult();
    xiiTestFramework::Output(xiiTestOutput::ImageDiffFile, sDataDirRelativePath);
    return false;
  }
  return true;
}

bool xiiTestFramework::CompareImages(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage, bool bIsLineImage)
{
  xiiStringBuilder sImgName;
  GenerateComparisonImageName(uiImageNumber, sImgName);

  xiiImage img;
  if (bIsDepthImage)
  {
    sImgName.Append("-depth");
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetDepthImage(img, *GetCurrentSubTest(), uiImageNumber).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Depth image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }
  else
  {
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetImage(img, *GetCurrentSubTest(), uiImageNumber).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }

  bool bImagesMatch = true;
  if (img.GetNumArrayIndices() <= 1)
  {
    bImagesMatch = PerformImageComparison(sImgName, img, uiMaxError, bIsLineImage, szErrorMsg);
  }
  else
  {
    xiiStringBuilder lastError;
    for (xiiUInt32 i = 0; i < img.GetNumArrayIndices(); ++i)
    {
      xiiStringBuilder subImageName;
      subImageName.AppendFormat("{0}_{1}", sImgName, i);
      if (!PerformImageComparison(subImageName, img.GetSubImageView(0, 0, i), uiMaxError, bIsLineImage, szErrorMsg))
      {
        bImagesMatch = false;
        if (!lastError.IsEmpty())
        {
          xiiTestFramework::Output(xiiTestOutput::Error, "%s", lastError.GetData());
        }
        lastError = szErrorMsg;
      }
    }
  }

  if (m_ImageComparisonCallback)
  {
    m_ImageComparisonCallback(bImagesMatch);
  }

  return bImagesMatch;
}

void xiiTestFramework::SetImageComparisonCallback(const ImageComparisonCallback& callback)
{
  m_ImageComparisonCallback = callback;
}

xiiResult xiiTestFramework::CaptureRegressionStat(xiiStringView sTestName, xiiStringView sName, xiiStringView sUnit, float value, xiiInt32 iTestId)
{
  xiiStringBuilder strippedTestName = sTestName;
  strippedTestName.ReplaceAll(" ", "");

  xiiStringBuilder perTestName;
  if (iTestId < 0)
  {
    perTestName.SetFormat("{}_{}", strippedTestName, sName);
  }
  else
  {
    perTestName.SetFormat("{}_{}_{}", strippedTestName, sName, iTestId);
  }

  {
    xiiStringBuilder regression;
    // The 6 floating point digits are forced as per a requirement of the CI
    // feature that parses these values.
    regression.SetFormat("[test][REGRESSION:{}:{}:{}]", perTestName, sUnit, xiiArgF(value, 6));
    xiiLog::Info(regression);
  }

  return XII_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// xiiTestFramework static functions
////////////////////////////////////////////////////////////////////////

void xiiTestFramework::Output(xiiTestOutput::Enum type, const char* szMsg, ...)
{
  va_list args;
  va_start(args, szMsg);

  OutputArgs(type, szMsg, args);

  va_end(args);
}

void xiiTestFramework::OutputArgs(xiiTestOutput::Enum type, const char* szMsg, va_list szArgs)
{
  // format the output text
  char     szBuffer[1024 * 10];
  xiiInt32 pos = 0;

  if (xiiTestFramework::s_LogTimestampMode != xiiLog::TimestampMode::None)
  {
    if (type == xiiTestOutput::BeginBlock || type == xiiTestOutput::EndBlock || type == xiiTestOutput::ImportantInfo || type == xiiTestOutput::Details || type == xiiTestOutput::Success || type == xiiTestOutput::Message || type == xiiTestOutput::Warning || type == xiiTestOutput::Error ||
        type == xiiTestOutput::FinalResult)
    {
      xiiStringBuilder timestamp;

      xiiLog::GenerateFormattedTimestamp(xiiTestFramework::s_LogTimestampMode, timestamp);
      pos = xiiStringUtils::snprintf(szBuffer, XII_ARRAY_SIZE(szBuffer), "%s", timestamp.GetData());
    }
  }
  xiiStringUtils::vsnprintf(szBuffer + pos, XII_ARRAY_SIZE(szBuffer) - pos, szMsg, szArgs);

  GetInstance()->OutputImpl(type, szBuffer);
}

void xiiTestFramework::Error(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, xiiStringView sMsg, ...)
{
  va_list args;
  va_start(args, sMsg);

  Error(szError, szFile, iLine, szFunction, sMsg, args);

  va_end(args);
}

void xiiTestFramework::Error(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, xiiStringView sMsg, va_list szArgs)
{
  // format the output text
  char szBuffer[1024 * 10];
  xiiStringUtils::vsnprintf(szBuffer, XII_ARRAY_SIZE(szBuffer), xiiString(sMsg).GetData(), szArgs);

  GetInstance()->ErrorImpl(szError, szFile, iLine, szFunction, szBuffer);
}

void xiiTestFramework::TestResult(xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(uiSubTestIndex, bSuccess, fDuration);
}

void xiiTestFramework::SetSubTestStatus(xiiUInt32 uiSubTestIndex, const char* szStatus)
{
  GetInstance()->SetSubTestStatusImpl(uiSubTestIndex, szStatus);
}

////////////////////////////////////////////////////////////////////////
// XII_TEST_... macro functions
////////////////////////////////////////////////////////////////////////

#define OUTPUT_TEST_ERROR                                                         \
  {                                                                               \
    va_list args;                                                                 \
    va_start(args, szMsg);                                                        \
    xiiTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args); \
    XII_TEST_DEBUG_BREAK                                                          \
    va_end(args);                                                                 \
    return XII_FAILURE;                                                           \
  }

#define OUTPUT_TEST_ERROR_NO_BREAK                                                \
  {                                                                               \
    va_list args;                                                                 \
    va_start(args, szMsg);                                                        \
    xiiTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args); \
    va_end(args);                                                                 \
    return XII_FAILURE;                                                           \
  }

bool xiiTestBool(bool bCondition, const char* szErrorText, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  if (!bCondition)
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestResult(xiiResult condition, const char* szErrorText, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  if (condition.Failed())
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  const double fD = f1 - f2;

  if (fD < -fEps || fD > +fEps)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%.8f) does not equal '%s' (%.8f) within an epsilon of %.8f", szF1, f1, szF2, f2, fEps);

    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestInt(xiiInt64 i1, xiiInt64 i2, const char* szI1, const char* szI2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  if (i1 != i2)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%lli) does not equal '%s' (%lli)", szI1, i1, szI2, i2);

    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestWString(std::wstring s1, std::wstring s2, const char* szWString1, const char* szWString2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szWString1, xiiStringUtf8(s1.c_str()).GetData(), szWString2, xiiStringUtf8(s2.c_str()).GetData());

    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestString(xiiStringView s1, xiiStringView s2, const char* szString1, const char* szString2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    xiiStringBuilder ss1 = s1;
    xiiStringBuilder ss2 = s2;

    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szString1, ss1.GetData(), szString2, ss2.GetData());

    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestVector(xiiVec4d v1, xiiVec4d v2, double fEps, const char* szCondition, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  char szErrorText[256];

  if (!xiiMath::IsEqual(v1.x, v2.x, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.x (%.8f) does not equal v2.x (%.8f) within an epsilon of %.8f", szCondition, v1.x, v2.x, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!xiiMath::IsEqual(v1.y, v2.y, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.y (%.8f) does not equal v2.y (%.8f) within an epsilon of %.8f", szCondition, v1.y, v2.y, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!xiiMath::IsEqual(v1.z, v2.z, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.z (%.8f) does not equal v2.z (%.8f) within an epsilon of %.8f", szCondition, v1.z, v2.z, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!xiiMath::IsEqual(v1.w, v2.w, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.w (%.8f) does not equal v2.w (%.8f) within an epsilon of %.8f", szCondition, v1.w, v2.w, fEps);

    OUTPUT_TEST_ERROR
  }

  return XII_SUCCESS;
}

bool xiiTestFiles(const char* szFile1, const char* szFile2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  xiiFileReader ReadFile1;
  xiiFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == XII_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == XII_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }

  else if (ReadFile1.GetFileSize() != ReadFile2.GetFileSize())
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File sizes do not match: '%s' (%llu Bytes) and '%s' (%llu Bytes)", szFile1, ReadFile1.GetFileSize(), szFile2, ReadFile2.GetFileSize());

    OUTPUT_TEST_ERROR
  }
  else
  {
    while (true)
    {
      xiiUInt8        uiTemp1[512];
      xiiUInt8        uiTemp2[512];
      const xiiUInt64 uiRead1 = ReadFile1.ReadBytes(uiTemp1, 512);
      const xiiUInt64 uiRead2 = ReadFile2.ReadBytes(uiTemp2, 512);

      if (uiRead1 != uiRead2)
      {
        safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files could not read same amount of data: '%s' and '%s'", szFile1, szFile2);

        OUTPUT_TEST_ERROR
      }
      else
      {
        if (uiRead1 == 0)
          break;

        if (memcmp(uiTemp1, uiTemp2, (size_t)uiRead1) != 0)
        {
          safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files contents do not match: '%s' and '%s'", szFile1, szFile2);

          OUTPUT_TEST_ERROR
        }
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiTestTextFiles(const char* szFile1, const char* szFile2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  xiiTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  xiiFileReader ReadFile1;
  xiiFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == XII_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == XII_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }
  else
  {
    xiiStringBuilder sFile1;
    sFile1.ReadAll(ReadFile1);
    sFile1.ReplaceAll("\r\n", "\n");

    xiiStringBuilder sFile2;
    sFile2.ReadAll(ReadFile2);
    sFile2.ReplaceAll("\r\n", "\n");

    if (sFile1 != sFile2)
    {
      safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Text files contents do not match: '%s' and '%s'", szFile1, szFile2);

      OUTPUT_TEST_ERROR
    }
  }

  return XII_SUCCESS;
}

bool xiiTestImage(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  char szErrorText[s_iMaxErrorMessageLength] = "";

  if (!xiiTestFramework::GetInstance()->CompareImages(uiImageNumber, uiMaxError, szErrorText, bIsDepthImage, bIsLineImage))
  {
    OUTPUT_TEST_ERROR_NO_BREAK
  }

  return XII_SUCCESS;
}
