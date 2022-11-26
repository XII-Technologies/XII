#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>
#include <vector>

struct xiiTestOutput
{
  /// \brief Defines the type of output message for xiiTestOutputMessage.
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

/// \brief A message of type xiiTestOutput::Enum, stored in xiiResult.
struct xiiTestErrorMessage
{
  xiiTestErrorMessage() :
    m_iLine(-1)
  {
  }

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  xiiInt32    m_iLine;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// \brief A message of type xiiTestOutput::Enum, stored in xiiResult.
struct xiiTestOutputMessage
{
  xiiTestOutputMessage() :
    m_Type(xiiTestOutput::ImportantInfo), m_iErrorIndex(-1)
  {
  }

  xiiTestOutput::Enum m_Type;
  std::string         m_sMessage;
  xiiInt32            m_iErrorIndex;
};

struct xiiTestResultQuery
{
  /// \brief Defines what information should be accumulated over the sub-tests in xiiTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Executed,
    Success,
    Errors,
  };
};

/// \brief Stores the results of a test run. Used by both xiiTestEntry and xiiSubTestEntry.
struct xiiTestResultData
{
  xiiTestResultData() :
    m_bExecuted(false), m_bSuccess(false), m_iTestAsserts(0), m_fTestDuration(0.0), m_iFirstOutput(-1), m_iLastOutput(-1)
  {
  }
  void Reset();
  void AddOutput(xiiInt32 iOutputIndex);

  std::string m_sName;
  bool        m_bExecuted;  ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before
                            ///< executing it.
  bool     m_bSuccess;      ///< Whether the test succeeded or not.
  int      m_iTestAsserts;  ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double   m_fTestDuration; ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  xiiInt32 m_iFirstOutput;  ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  xiiInt32 m_iLastOutput;   ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
};

struct xiiTestConfiguration
{
  xiiTestConfiguration();

  xiiUInt64   m_uiInstalledMainMemory;
  xiiUInt32   m_uiMemoryPageSize;
  xiiUInt32   m_uiCPUCoreCount;
  bool        m_b64BitOS;
  bool        m_b64BitApplication;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  xiiInt64    m_iDateTime;           ///< in seconds since Linux epoch
  xiiInt32    m_iRCSRevision;
  std::string m_sHostName;
};

class xiiTestFrameworkResult
{
public:
  xiiTestFrameworkResult() {}

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<xiiTestEntry>& tests, const xiiTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szFileName) const;

  // Result access
  xiiUInt32                GetTestCount(xiiTestResultQuery::Enum countQuery = xiiTestResultQuery::Count) const;
  xiiUInt32                GetSubTestCount(xiiUInt32 uiTestIndex, xiiTestResultQuery::Enum countQuery = xiiTestResultQuery::Count) const;
  xiiInt32                 GetTestIndexByName(const char* szTestName) const;
  xiiInt32                 GetSubTestIndexByName(xiiUInt32 uiTestIndex, const char* szSubTestName) const;
  double                   GetTotalTestDuration() const;
  const xiiTestResultData& GetTestResultData(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex) const;

  // Test output
  void TestOutput(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, xiiTestOutput::Enum Type, const char* szMsg);
  void TestError(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, const char* szError, const char* szBlock, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg);
  void TestResult(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, int iCount);

  // Messages / Errors
  xiiUInt32                   GetOutputMessageCount(xiiInt32 iTestIndex = -1, xiiInt32 iSubTestIndex = -1, xiiTestOutput::Enum Type = xiiTestOutput::AllOutputTypes) const;
  const xiiTestOutputMessage* GetOutputMessage(xiiUInt32 uiOutputMessageIdx) const;

  xiiUInt32                  GetErrorMessageCount(xiiInt32 iTestIndex = -1, xiiInt32 iSubTestIndex = -1) const;
  const xiiTestErrorMessage* GetErrorMessage(xiiUInt32 uiErrorMessageIdx) const;

private:
  struct xiiSubTestResult
  {
    xiiSubTestResult() {}
    xiiSubTestResult(const char* szName) { m_Result.m_sName = szName; }

    xiiTestResultData m_Result;
  };

  struct xiiTestResult
  {
    xiiTestResult() {}
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
