/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>
#include <vector>

struct xiiTestOutput
{
  /// Defines the type of output message for xiiTestOutputMessage.
  enum Enum
  {
    InvalidType = -1,
    StartOutput = 0,
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Warning,
    Error,
    ImageDiffFile,
    Duration,
    FinalResult,
    AllOutputTypes
  };

  static const char* const s_Names[];
  static const char*       ToString(Enum type);
  static Enum              FromString(const char* szName);
};

/// A message of type xiiTestOutput::Enum, stored in xiiResult.
struct xiiTestErrorMessage
{
  xiiTestErrorMessage() = default;

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  xiiInt32    m_iLine = -1;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// A message of type xiiTestOutput::Enum, stored in xiiResult.
struct xiiTestOutputMessage
{
  xiiTestOutputMessage() = default;

  xiiTestOutput::Enum m_Type = xiiTestOutput::ImportantInfo;
  std::string         m_sMessage;
  xiiInt32            m_iErrorIndex = -1;
};

struct xiiTestResultQuery
{
  /// Defines what information should be accumulated over the sub-tests in xiiTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Executed,
    Success,
    Errors,
  };
};

/// Stores the results of a test run. Used by both xiiTestEntry and xiiSubTestEntry.
struct xiiTestResultData
{
  xiiTestResultData() = default;

  void Reset();
  void AddOutput(xiiInt32 iOutputIndex);

  std::string m_sName;
  bool        m_bExecuted = false;     ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before
                                       ///< executing it.
  bool        m_bSuccess      = false; ///< Whether the test succeeded or not.
  int         m_iTestAsserts  = 0;     ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double      m_fTestDuration = 0.0;   ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  xiiInt32    m_iFirstOutput  = -1;    ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  xiiInt32    m_iLastOutput   = -1;    ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
  std::string m_sCustomStatus;         ///< If this is not empty, the UI will display this instead of "Pending"
};

struct xiiTestConfiguration
{
  xiiTestConfiguration();

  xiiUInt64   m_uiInstalledMainMemory = 0;
  xiiUInt32   m_uiMemoryPageSize      = 0;
  xiiUInt32   m_uiCPUCoreCount        = 0;
  bool        m_b64BitOS              = false;
  bool        m_b64BitApplication     = false;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  xiiInt64    m_iDateTime    = 0;    ///< in seconds since Linux epoch
  xiiInt32    m_iRCSRevision = -1;
  std::string m_sHostName;
};

class xiiTestFrameworkResult
{
public:
  xiiTestFrameworkResult() = default;

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<xiiTestEntry>& tests, const xiiTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szFileName) const;

  // Result access
  xiiUInt32                GetTestCount(xiiTestResultQuery::Enum countQuery = xiiTestResultQuery::Count) const;
  xiiUInt32                GetSubTestCount(xiiUInt32 uiTestIndex, xiiTestResultQuery::Enum countQuery = xiiTestResultQuery::Count) const;
  xiiUInt32                GetTestIndexByName(const char* szTestName) const;
  xiiUInt32                GetSubTestIndexByName(xiiUInt32 uiTestIndex, const char* szSubTestName) const;
  double                   GetTotalTestDuration() const;
  const xiiTestResultData& GetTestResultData(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex) const;

  // Test output
  void TestOutput(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, xiiTestOutput::Enum type, const char* szMsg);
  void TestError(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, const char* szError, const char* szBlock, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg);
  void TestResult(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, int iCount);
  void SetCustomStatus(xiiUInt32 uiTestIndex, xiiUInt32 uiSubTestIndex, const char* szCustomStatus);

  // Messages / Errors
  xiiUInt32                   GetOutputMessageCount(xiiUInt32 uiTestIndex = xiiInvalidIndex, xiiUInt32 uiSubTestIndex = xiiInvalidIndex, xiiTestOutput::Enum type = xiiTestOutput::AllOutputTypes) const;
  const xiiTestOutputMessage* GetOutputMessage(xiiUInt32 uiOutputMessageIdx) const;

  xiiUInt32                  GetErrorMessageCount(xiiUInt32 uiTestIndex = xiiInvalidIndex, xiiUInt32 uiSubTestIndex = xiiInvalidIndex) const;
  const xiiTestErrorMessage* GetErrorMessage(xiiUInt32 uiErrorMessageIdx) const;

private:
  struct xiiSubTestResult
  {
    xiiSubTestResult() = default;
    xiiSubTestResult(const char* szName) { m_Result.m_sName = szName; }

    xiiTestResultData m_Result;
  };

  struct xiiTestResult
  {
    xiiTestResult() = default;
    xiiTestResult(const char* szName) { m_Result.m_sName = szName; }

    void Reset();

    xiiTestResultData            m_Result;
    std::deque<xiiSubTestResult> m_SubTests;
  };

private:
  xiiTestConfiguration             m_Config;
  std::deque<xiiTestResult>        m_Tests;
  std::deque<xiiTestErrorMessage>  m_Errors;
  std::deque<xiiTestOutputMessage> m_TestOutput;
};
