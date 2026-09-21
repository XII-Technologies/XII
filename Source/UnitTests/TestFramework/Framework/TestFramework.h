/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/TestResults.h>
#include <TestFramework/TestFrameworkDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

class xiiCommandLineUtils;

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class XII_TEST_DLL xiiTestFramework
{
public:
  xiiTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~xiiTestFramework();

  using OutputHandler = void (*)(xiiTestOutput::Enum, const char*);

  // Test management
  void        CreateOutputFolder();
  void        UpdateReferenceImages();
  const char* GetTestName() const;
  const char* GetAbsOutputPath() const;
  const char* GetRelTestDataPath() const;
  const char* GetAbsTestOrderFilePath() const;
  const char* GetAbsTestSettingsFilePath() const;
  void        RegisterOutputHandler(OutputHandler handler);
  void        GatherAllTests();
  void        LoadTestOrder();
  void        ApplyTestOrderFromCommandLine(const xiiCommandLineUtils& cmd);
  void        LoadTestSettings();
  void        AutoSaveTestOrder();
  void        SaveTestOrder(const char* const szFilePath);
  void        SaveTestSettings(const char* const szFilePath);
  void        SetAllTestsEnabledStatus(bool bEnable);
  void        SetAllFailedTestsEnabledStatus();
  // Each function on a test must not take longer than the given time or the test process will be terminated.
  void      SetTestTimeout(xiiUInt32 uiTestTimeoutMS);
  xiiUInt32 GetTestTimeout() const;
  void      GetTestSettingsFromCommandLine(const xiiCommandLineUtils& cmd);

  // Test execution
  void          ResetTests();
  xiiTestAppRun RunTestExecutionLoop();

  void StartTests();
  void ExecuteNextTest();
  void EndTests();
  void AbortTests();

  // Test queries
  xiiUInt32          GetTestCount() const;
  xiiUInt32          GetTestEnabledCount() const;
  xiiUInt32          GetSubTestEnabledCount(xiiUInt32 uiTestIndex) const;
  const std::string& IsTestAvailable(xiiUInt32 uiTestIndex) const;
  bool               IsTestEnabled(xiiUInt32 uiTestIndex) const;
  bool               IsSubTestEnabled(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex) const;
  void               SetTestEnabled(xiiUInt32 uiTestIndex, bool bEnabled);
  void               SetSubTestEnabled(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, bool bEnabled);

  xiiUInt32 GetCurrentTestIndex() const { return m_uiCurrentTestIndex; }
  xiiUInt32 GetCurrentSubTestIndex() const { return m_uiCurrentSubTestIndex; }
  xiiInt32  GetCurrentSubTestIdentifier() const;

  /// Returns the index of the sub-test with the given identifier.
  ///
  /// Only looks at the currently running test, assuming that the identifier is unique among its sub-tests.
  xiiUInt32 FindSubTestIndexForSubTestIdentifier(xiiInt32 iSubTestIdentifier) const;

  xiiTestEntry*       GetTest(xiiUInt32 uiTestIndex);
  const xiiTestEntry* GetTest(xiiUInt32 uiTestIndex) const;
  bool                GetTestsRunning() const { return m_bTestsRunning; }

  const xiiTestEntry*    GetCurrentTest() const;
  const xiiSubTestEntry* GetCurrentSubTest() const;

  // Global settings
  TestSettings GetSettings() const;
  void         SetSettings(const TestSettings& settings);

  // Test results
  xiiTestFrameworkResult& GetTestResult();
  xiiInt32                GetTotalErrorCount() const;
  xiiInt32                GetTestsPassedCount() const;
  xiiInt32                GetTestsFailedCount() const;
  double                  GetTotalTestDuration() const;

  // Image comparison
  void ScheduleImageComparison(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError);
  void ScheduleDepthImageComparison(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError);
  bool IsImageComparisonScheduled() const { return m_bImageComparisonScheduled; }
  bool IsDepthImageComparisonScheduled() const { return m_bDepthImageComparisonScheduled; }
  void GenerateComparisonImageName(xiiUInt32 uiImageNumber, xiiStringBuilder& ref_sImgName);
  void GetCurrentComparisonImageName(xiiStringBuilder& ref_sImgName);
  void SetImageReferenceFolderName(const char* szFolderName);
  void SetImageReferenceOverrideFolderName(const char* szFolderName);

  /// Writes an Html file that contains test information and an image diff view for failed image comparisons.
  void WriteImageDiffHtml(const char* szFileName, const xiiImage& referenceImgRgb, const xiiImage& referenceImgAlpha, const xiiImage& capturedImgRgb, const xiiImage& capturedImgAlpha, const xiiImage& diffImgRgb, const xiiImage& diffImgAlpha, xiiUInt32 uiError, xiiUInt32 uiThreshold, xiiUInt8 uiMinDiffRgb, xiiUInt8 uiMaxDiffRgb, xiiUInt8 uiMinDiffAlpha, xiiUInt8 uiMaxDiffAlpha);

  bool PerformImageComparison(xiiStringBuilder sImgName, const xiiImage& img, xiiUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg);
  bool CompareImages(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage = false, bool bIsLineImage = false);

  /// A function to be called to add extra info to image diff output, that is not available from here.
  /// E.g. device specific info like driver version.
  using ImageDiffExtraInfoCallback = std::function<xiiDynamicArray<std::pair<xiiString, xiiString>>()>;
  void SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider);

  using ImageComparisonCallback = std::function<void(bool)>; /// A function to be called after every image comparison with a bool
                                                             /// indicating if the images matched or not.
  void SetImageComparisonCallback(const ImageComparisonCallback& callback);

  static xiiResult CaptureRegressionStat(xiiStringView sTestName, xiiStringView sName, xiiStringView sUnit, float value, xiiInt32 iTestId = -1);

protected:
  void Initialize();
  void DeInitialize();

  /// Will be called for test failures to record the location of the failure and forward the error to OutputImpl.
  virtual void ErrorImpl(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg);
  /// Receives xiiLog messages (via LogWriter) as well as test-framework internal logging. Any xiiTestOutput::Error will
  /// cause the test to fail.
  virtual void OutputImpl(xiiTestOutput::Enum Type, const char* szMsg);
  virtual void TestResultImpl(xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  virtual void SetSubTestStatusImpl(xiiUInt32 uiSubTestIndex, const char* szStatus);
  void         FlushAsserts();
  void         TimeoutThread();
  void         UpdateTestTimeout();

  // ignore this for now
public:
  static const char*           s_szTestBlockName;
  static int                   s_iAssertCounter;
  static bool                  s_bCallstackOnAssert;
  static xiiLog::TimestampMode s_LogTimestampMode;

  // static functions
public:
  static XII_ALWAYS_INLINE xiiTestFramework* GetInstance() { return s_pInstance; }

  /// Returns whether to assert on test failure.
  static bool GetAssertOnTestFail();

  static void Output(xiiTestOutput::Enum type, const char* szMsg, ...);
  static void OutputArgs(xiiTestOutput::Enum type, const char* szMsg, va_list szArgs);
  static void Error(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, xiiStringView sMsg, ...);
  static void Error(const char* szError, const char* szFile, xiiInt32 iLine, const char* szFunction, xiiStringView sMsg, va_list szArgs);
  static void TestResult(xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  static void SetSubTestStatus(xiiUInt32 uiSubTestIndex, const char* szStatus);

  // static members
private:
  static xiiTestFramework* s_pInstance;

private:
  std::string                m_sTestName;                ///< The name of the tests being done
  std::string                m_sAbsTestOutputDir;        ///< Absolute path to the output folder where results and temp data is stored
  std::string                m_sRelTestDataDir;          ///< Relative path from the SDK to where the unit test data is located
  std::string                m_sAbsTestOrderFilePath;    ///< Absolute path to the test order file
  std::string                m_sAbsTestSettingsFilePath; ///< Absolute path to the test settings file
  xiiInt32                   m_iErrorCount  = 0;
  xiiInt32                   m_iTestsFailed = 0;
  xiiInt32                   m_iTestsPassed = 0;
  TestSettings               m_Settings;
  std::recursive_mutex       m_OutputMutex;
  std::deque<OutputHandler>  m_OutputHandlers;
  std::deque<xiiTestEntry>   m_TestEntries;
  xiiTestFrameworkResult     m_Result;
  xiiAssertHandler           m_PreviousAssertHandler = nullptr;
  ImageDiffExtraInfoCallback m_ImageDiffExtraInfoCallback;
  ImageComparisonCallback    m_ImageComparisonCallback;

  std::mutex              m_TimeoutLock;
  xiiUInt32               m_uiTimeoutMS = 5 * 60 * 1000; // 5 min default timeout
  bool                    m_bUseTimeout = false;
  bool                    m_bArm        = false;
  std::condition_variable m_TimeoutCV;
  std::thread             m_TimeoutThread;

  xiiUInt32 m_uiExecutingTest          = 0;
  xiiUInt32 m_uiExecutingSubTest       = 0;
  bool      m_bSubTestInitialized      = false;
  bool      m_bAbortTests              = false;
  xiiUInt8  m_uiPassesLeft             = 0;
  double    m_fTotalTestDuration       = 0.0;
  double    m_fTotalSubTestDuration    = 0.0;
  xiiInt32  m_iErrorCountBeforeTest    = 0;
  xiiUInt32 m_uiSubTestInvocationCount = 0;

  bool m_bIsInitialized = false;

  // image comparisons
  bool      m_bImageComparisonScheduled = false;
  xiiUInt32 m_uiMaxImageComparisonError = 0;
  xiiUInt32 m_uiComparisonImageNumber   = 0;

  bool      m_bDepthImageComparisonScheduled = false;
  xiiUInt32 m_uiMaxDepthImageComparisonError = 0;
  xiiUInt32 m_uiComparisonDepthImageNumber   = 0;

  std::string m_sImageReferenceFolderName = "Images_Reference";
  std::string m_sImageReferenceOverrideFolderName;

protected:
  xiiUInt32 m_uiCurrentTestIndex    = xiiInvalidIndex;
  xiiUInt32 m_uiCurrentSubTestIndex = xiiInvalidIndex;
  bool      m_bTestsRunning         = false;
};

#ifdef XII_NV_OPTIMUS
#  undef XII_NV_OPTIMUS
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/MinWindows.h>
#  define XII_NV_OPTIMUS                                                                           \
    extern "C"                                                                                     \
    {                                                                                              \
      _declspec(dllexport) xiiMinWindows::DWORD NvOptimusEnablement                  = 0x00000001; \
      _declspec(dllexport) xiiMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
    }
#else
#  define XII_NV_OPTIMUS
#endif

/// Macro to define the application entry point for all test applications
#define XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */            \
  XII_NV_OPTIMUS                                                                           \
  XII_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
  int main(int argc, char** argv)                                                          \
  {                                                                                        \
    xiiTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define XII_TESTFRAMEWORK_ENTRY_POINT_END()                         \
  while (xiiTestSetup::RunTests() == xiiTestAppRun::Continue)       \
  {                                                                 \
  }                                                                 \
  const xiiInt32 iFailedTests = xiiTestSetup::GetFailedTestCount(); \
  xiiTestSetup::DeInitTestFramework();                              \
  return iFailedTests;                                              \
  }

#define XII_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)            \
  XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)            \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  XII_TESTFRAMEWORK_ENTRY_POINT_END()

/// Enum for usage in XII_TEST_BLOCK to enable or disable the block.
struct xiiTestBlock
{
  /// Enum for usage in XII_TEST_BLOCK to enable or disable the block.
  enum Enum
  {
    Enabled,           ///< The test block is enabled.
    Disabled,          ///< The test block will be skipped. The test framework will print a warning message, that some block is deactivated.
    DisabledNoWarning, ///< The test block will be skipped, but no warning printed. Used to deactivate 'on demand/optional' tests.
  };
};

#define safeprintf xiiStringUtils::snprintf

/// Starts a small test block inside a larger test.
///
/// First parameter allows to quickly disable a block depending on a condition (e.g. platform).
/// Second parameter just gives it a name for better error reporting.
/// Also skipped tests are highlighted in the output, such that people can quickly see when a test is currently deactivated.
#define XII_TEST_BLOCK(enable, name)                                                   \
  xiiTestFramework::s_szTestBlockName = name;                                          \
  if (enable == xiiTestBlock::Disabled)                                                \
  {                                                                                    \
    xiiTestFramework::s_szTestBlockName = "";                                          \
    xiiTestFramework::Output(xiiTestOutput::Warning, "Skipped Test Block '%s'", name); \
  }                                                                                    \
  else if (enable == xiiTestBlock::DisabledNoWarning)                                  \
  {                                                                                    \
    xiiTestFramework::s_szTestBlockName = "";                                          \
  }                                                                                    \
  else


/// Will trigger a debug break, if the test framework is configured to do so on test failure
#define XII_TEST_DEBUG_BREAK                   \
  if (xiiTestFramework::GetAssertOnTestFail()) \
  XII_DEBUG_BREAK

#define XII_TEST_FAILURE(erroroutput, msg, ...)                                                                      \
  {                                                                                                                  \
    xiiTestFramework::Error(erroroutput, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__); \
    XII_TEST_DEBUG_BREAK                                                                                             \
  }

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestBool(bool bCondition, const char* szErrorText, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define XII_TEST_BOOL(condition) XII_TEST_BOOL_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define XII_TEST_BOOL_MSG(condition, msg, ...) \
  xiiTestBool(condition, "Test failed: " XII_PP_STRINGIFY(condition), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestResult(xiiResult condition, const char* szErrorText, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define XII_TEST_RESULT(condition) XII_TEST_RESULT_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define XII_TEST_RESULT_MSG(condition, msg, ...) \
  xiiTestResult(condition, "Test failed: " XII_PP_STRINGIFY(condition), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestResult(xiiResult condition, const char* szErrorText, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define XII_TEST_RESULT(condition) XII_TEST_RESULT_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define XII_TEST_RESULT_MSG(condition, msg, ...) \
  xiiTestResult(condition, "Test failed: " XII_PP_STRINGIFY(condition), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests for a xiiStatus condition, outputs xiiStatus message on failure
#define XII_TEST_STATUS(condition)                     \
  auto XII_PP_CONCAT(l_, XII_SOURCE_LINE) = condition; \
  xiiTestResult(XII_PP_CONCAT(l_, XII_SOURCE_LINE).GetResult(), "Test failed: " XII_PP_STRINGIFY(condition), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, XII_PP_CONCAT(l_, XII_SOURCE_LINE).GetMessageString())

inline double ToFloat(int f)
{
  return static_cast<double>(f);
}

inline double ToFloat(float f)
{
  return static_cast<double>(f);
}

inline double ToFloat(double f)
{
  return static_cast<double>(f);
}

XII_TEST_DLL bool xiiTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output.
#define XII_TEST_FLOAT(f1, f2, epsilon) XII_TEST_FLOAT_MSG(f1, f2, epsilon, "")

/// Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define XII_TEST_FLOAT_MSG(f1, f2, epsilon, msg, ...) \
  xiiTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), XII_PP_STRINGIFY(f1), XII_PP_STRINGIFY(f2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////

/// Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output.
#define XII_TEST_DOUBLE(f1, f2, epsilon) XII_TEST_DOUBLE_MSG(f1, f2, epsilon, "")

/// Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define XII_TEST_DOUBLE_MSG(f1, f2, epsilon, msg, ...) \
  xiiTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), XII_PP_STRINGIFY(f1), XII_PP_STRINGIFY(f2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestInt(xiiInt64 i1, xiiInt64 i2, const char* szI1, const char* szI2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two ints for equality. On failure both actual and expected values are output.
#define XII_TEST_INT(i1, i2) XII_TEST_INT_MSG(i1, i2, "")

/// Tests two ints for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_INT_MSG(i1, i2, msg, ...) \
  xiiTestInt(i1, i2, XII_PP_STRINGIFY(i1), XII_PP_STRINGIFY(i2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestString(xiiStringView s1, xiiStringView s2, const char* szString1, const char* szString2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two strings for equality. On failure both actual and expected values are output.
#define XII_TEST_STRING(i1, i2) XII_TEST_STRING_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_STRING_MSG(s1, s2, msg, ...) \
  xiiTestString(static_cast<xiiStringView>(s1), static_cast<xiiStringView>(s2), XII_PP_STRINGIFY(s1), XII_PP_STRINGIFY(s2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestWString(std::wstring s1, std::wstring s2, const char* szString1, const char* szString2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two strings for equality. On failure both actual and expected values are output.
#define XII_TEST_WSTRING(i1, i2) XII_TEST_WSTRING_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_WSTRING_MSG(s1, s2, msg, ...) \
  xiiTestWString(static_cast<const wchar_t*>(s1), static_cast<const wchar_t*>(s2), XII_PP_STRINGIFY(s1), XII_PP_STRINGIFY(s2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two strings for equality. On failure both actual and expected values are output. Does not embed the original expression to
/// work around issues with the current code page and unicode literals.
#define XII_TEST_STRING_UNICODE(i1, i2) XII_TEST_STRING_UNICODE_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed. Does not
/// embed the original expression to work around issues with the current code page and unicode literals.
#define XII_TEST_STRING_UNICODE_MSG(s1, s2, msg, ...) \
  xiiTestString(static_cast<xiiStringView>(s1), static_cast<xiiStringView>(s2), "", "", XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestVector(xiiVec4d v1, xiiVec4d v2, double fEps, const char* szCondition, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two xiiVec2's for equality, using some epsilon. On failure both actual and expected values are output.
#define XII_TEST_VEC2(i1, i2, epsilon) XII_TEST_VEC2_MSG(i1, i2, epsilon, "")

/// Tests two xiiVec2's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_VEC2_MSG(r1, r2, epsilon, msg, ...)                                                                                  \
  xiiTestVector(xiiVec4d(ToFloat((r1).x), ToFloat((r1).y), 0, 0), xiiVec4d(ToFloat((r2).x), ToFloat((r2).y), 0, 0), ToFloat(epsilon), \
                XII_PP_STRINGIFY(r1) " == " XII_PP_STRINGIFY(r2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two xiiVec3's for equality, using some epsilon. On failure both actual and expected values are output.
#define XII_TEST_VEC3(i1, i2, epsilon) XII_TEST_VEC3_MSG(i1, i2, epsilon, "")

/// Tests two xiiVec3's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_VEC3_MSG(r1, r2, epsilon, msg, ...)                                                                                            \
  xiiTestVector(xiiVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), 0), xiiVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), 0), \
                ToFloat(epsilon), XII_PP_STRINGIFY(r1) " == " XII_PP_STRINGIFY(r2), XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two xiiVec4's for equality, using some epsilon. On failure both actual and expected values are output.
#define XII_TEST_VEC4(i1, i2, epsilon) XII_TEST_VEC4_MSG(i1, i2, epsilon, "")

/// Tests two xiiVec4's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define XII_TEST_VEC4_MSG(r1, r2, epsilon, msg, ...)                                                                                                              \
  xiiTestVector(xiiVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), ToFloat((r1).w)),                                                                     \
                xiiVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), ToFloat((r2).w)), ToFloat(epsilon), XII_PP_STRINGIFY(r1) " == " XII_PP_STRINGIFY(r2), \
                XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestFiles(const char* szFile1, const char* szFile2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define XII_TEST_FILES(szFile1, szFile2, msg, ...) \
  xiiTestFiles(szFile1, szFile2, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

XII_TEST_DLL bool xiiTestTextFiles(const char* szFile1, const char* szFile2, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define XII_TEST_TEXT_FILES(szFile1, szFile2, msg, ...) \
  xiiTestTextFiles(szFile1, szFile2, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

XII_TEST_DLL bool xiiTestImage(xiiUInt32 uiImageNumber, xiiUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Same as XII_TEST_IMAGE_MSG but uses an empty error message.
#define XII_TEST_IMAGE(ImageNumber, MaxError) XII_TEST_IMAGE_MSG(ImageNumber, MaxError, "")

/// Same as XII_TEST_DEPTH_IMAGE_MSG but uses an empty error message.
#define XII_TEST_DEPTH_IMAGE(ImageNumber, MaxError) XII_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, "")

/// Same as XII_TEST_LINE_IMAGE_MSG but uses an empty error message.
#define XII_TEST_LINE_IMAGE(ImageNumber, MaxError) XII_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, "")

/// Executes an image comparison right now.
///
/// The reference image is read from disk.
/// The path to the reference image is constructed from the test and sub-test name and the 'ImageNumber'.
/// One can, for instance, use the 'invocation count' that is passed to xiiTestBaseClass::RunSubTest() as the ImageNumber,
/// but any other integer is fine as well.
///
/// The current image to compare is taken from xiiTestBaseClass::GetImage().
/// Rendering tests typically override this function to return the result of the currently rendered frame.
///
/// 'MaxError' specifies the maximum mean-square error that is still considered acceptable
/// between the reference image and the current image.
///
/// Use the * DEPTH * variant if a depth buffer comparison should be requested.
///
/// \note Some tests need to know at the start, whether an image comparison will be done at the end, so they
/// can capture the image first. For such use cases, use XII_SCHEDULE_IMAGE_TEST at the start of a sub-test instead.
#define XII_TEST_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  xiiTestImage(ImageNumber, MaxError, false, false, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

#define XII_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  xiiTestImage(ImageNumber, MaxError, true, false, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// Same as XII_TEST_IMAGE_MSG, but allows for pixels to shift in a 1-pixel radius to account for different line rasterization of GPU vendors.
#define XII_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  xiiTestImage(ImageNumber, MaxError, false, true, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// Schedules an XII_TEST_IMAGE to be executed after the current sub-test execution finishes.
///
/// Call this at the beginning of a sub-test, to automatically execute an image comparison when it is finished.
/// Calling xiiTestFramework::IsImageComparisonScheduled() will now return true.
///
/// To support image comparisons, tests derived from xiiTestBaseClass need to provide the current image through xiiTestBaseClass::GetImage().
/// To support 'scheduled' image comparisons, the class should poll xiiTestFramework::IsImageComparisonScheduled() every step and capture the
/// image when needed.
///
/// Use the * DEPTH * variant if a depth buffer comparison is intended.
///
/// \note Scheduling image comparisons is an optimization to only capture data when necessary, instead of capturing it every single frame.
#define XII_SCHEDULE_IMAGE_TEST(ImageNumber, MaxError) xiiTestFramework::GetInstance()->ScheduleImageComparison(ImageNumber, MaxError);

#define XII_SCHEDULE_DEPTH_IMAGE_TEST(ImageNumber, MaxError) xiiTestFramework::GetInstance()->ScheduleDepthImageComparison(ImageNumber, MaxError);
