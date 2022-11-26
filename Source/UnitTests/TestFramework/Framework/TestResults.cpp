#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Framework/TestResults.h>

////////////////////////////////////////////////////////////////////////
// xiiTestOutput public functions
////////////////////////////////////////////////////////////////////////

const char* const xiiTestOutput::s_Names[] = {
  "StartOutput", "BeginBlock", "EndBlock", "ImportantInfo", "Details", "Success", "Message", "Warning", "Error", "Duration", "FinalResult"};

const char* xiiTestOutput::ToString(Enum type)
{
  return s_Names[type];
}

xiiTestOutput::Enum xiiTestOutput::FromString(const char* szName)
{
  for (xiiUInt32 i = 0; i < AllOutputTypes; ++i)
  {
    if (strcmp(szName, s_Names[i]) == 0)
      return (xiiTestOutput::Enum)i;
  }
  return InvalidType;
}


////////////////////////////////////////////////////////////////////////
// xiiTestResultData public functions
////////////////////////////////////////////////////////////////////////

void xiiTestResultData::Reset()
{
  m_bExecuted     = false;
  m_bSuccess      = false;
  m_iTestAsserts  = 0;
  m_fTestDuration = 0.0;
  m_iFirstOutput  = -1;
  m_iLastOutput   = -1;
}

void xiiTestResultData::AddOutput(xiiInt32 iOutputIndex)
{
  if (m_iFirstOutput == -1)
  {
    m_iFirstOutput = iOutputIndex;
    m_iLastOutput  = iOutputIndex;
  }
  else
  {
    m_iLastOutput = iOutputIndex;
  }
}


////////////////////////////////////////////////////////////////////////
// xiiTestResultData public functions
////////////////////////////////////////////////////////////////////////

xiiTestConfiguration::xiiTestConfiguration() :
  m_uiInstalledMainMemory(0), m_uiMemoryPageSize(0), m_uiCPUCoreCount(0), m_b64BitOS(false), m_b64BitApplication(false), m_iDateTime(0), m_iRCSRevision(-1)
{
}


////////////////////////////////////////////////////////////////////////
// xiiTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void xiiTestFrameworkResult::Clear()
{
  m_Tests.clear();
  m_Errors.clear();
  m_TestOutput.clear();
}

void xiiTestFrameworkResult::SetupTests(const std::deque<xiiTestEntry>& tests, const xiiTestConfiguration& config)
{
  m_Config = config;
  Clear();

  const xiiUInt32 uiTestCount = (xiiUInt32)tests.size();
  for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests.push_back(xiiTestResult(tests[uiTestIdx].m_szTestName));

    const xiiUInt32 uiSubTestCount = (xiiUInt32)tests[uiTestIdx].m_SubTests.size();
    for (xiiUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTestCount; ++uiSubTestIdx)
    {
      m_Tests[uiTestIdx].m_SubTests.push_back(xiiSubTestResult(tests[uiTestIdx].m_SubTests[uiSubTestIdx].m_szSubTestName));
    }
  }
}

void ::xiiTestFrameworkResult::Reset()
{
  const xiiUInt32 uiTestCount = (xiiUInt32)m_Tests.size();
  for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests[uiTestIdx].Reset();
  }
  m_Errors.clear();
  m_TestOutput.clear();
}

bool xiiTestFrameworkResult::WriteJsonToFile(const char* szFileName) const
{
  xiiStartup::StartupCoreSystems();
  XII_SCOPE_EXIT(xiiStartup::ShutdownCoreSystems());

  {
    xiiStringBuilder jsonFilename;
    if (xiiPathUtils::IsAbsolutePath(szFileName))
    {
      // Make sure we can access raw absolute file paths
      if (xiiFileSystem::AddDataDirectory("", "jsonoutput", ":", xiiFileSystem::AllowWrites).Failed())
        return false;

      jsonFilename = szFileName;
    }
    else
    {
      // If this is a relative path, we use the xiitest/ data directory to make sure that this works properly with the fileserver.
      if (xiiFileSystem::AddDataDirectory(">xiitest/", "jsonoutput", ":", xiiFileSystem::AllowWrites).Failed())
        return false;

      jsonFilename = ":";
      jsonFilename.AppendPath(szFileName);
    }

    xiiFileWriter file;
    if (file.Open(jsonFilename).Failed())
    {
      return false;
    }
    xiiStandardJSONWriter js;
    js.SetOutputStream(&file);

    js.BeginObject();
    {
      js.BeginObject("configuration");
      {
        js.AddVariableUInt64("m_uiInstalledMainMemory", m_Config.m_uiInstalledMainMemory);
        js.AddVariableUInt32("m_uiMemoryPageSize", m_Config.m_uiMemoryPageSize);
        js.AddVariableUInt32("m_uiCPUCoreCount", m_Config.m_uiCPUCoreCount);
        js.AddVariableBool("m_b64BitOS", m_Config.m_b64BitOS);
        js.AddVariableBool("m_b64BitApplication", m_Config.m_b64BitApplication);
        js.AddVariableString("m_sPlatformName", m_Config.m_sPlatformName.c_str());
        js.AddVariableString("m_sBuildConfiguration", m_Config.m_sBuildConfiguration.c_str());
        js.AddVariableInt64("m_iDateTime", m_Config.m_iDateTime);
        js.AddVariableInt32("m_iRCSRevision", m_Config.m_iRCSRevision);
        js.AddVariableString("m_sHostName", m_Config.m_sHostName.c_str());
      }
      js.EndObject();

      // Output Messages
      js.BeginArray("messages");
      {
        xiiUInt32 uiMessages = GetOutputMessageCount();
        for (xiiUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const xiiTestOutputMessage* pMessage = GetOutputMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_Type", xiiTestOutput::ToString(pMessage->m_Type));
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
            if (pMessage->m_iErrorIndex != -1)
              js.AddVariableInt32("m_iErrorIndex", pMessage->m_iErrorIndex);
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Error Messages
      js.BeginArray("errors");
      {
        xiiUInt32 uiMessages = GetErrorMessageCount();
        for (xiiUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const xiiTestErrorMessage* pMessage = GetErrorMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_sError", pMessage->m_sError.c_str());
            js.AddVariableString("m_sBlock", pMessage->m_sBlock.c_str());
            js.AddVariableString("m_sFile", pMessage->m_sFile.c_str());
            js.AddVariableString("m_sFunction", pMessage->m_sFunction.c_str());
            js.AddVariableInt32("m_iLine", pMessage->m_iLine);
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Tests
      js.BeginArray("tests");
      {
        xiiUInt32 uiTests = GetTestCount();
        for (xiiUInt32 uiTestIdx = 0; uiTestIdx < uiTests; ++uiTestIdx)
        {
          const xiiTestResultData& testResult = GetTestResultData(uiTestIdx, -1);
          js.BeginObject();
          {
            js.AddVariableString("m_sName", testResult.m_sName.c_str());
            js.AddVariableBool("m_bExecuted", testResult.m_bExecuted);
            js.AddVariableBool("m_bSuccess", testResult.m_bSuccess);
            js.AddVariableInt32("m_iTestAsserts", testResult.m_iTestAsserts);
            js.AddVariableDouble("m_fTestDuration", testResult.m_fTestDuration);
            js.AddVariableInt32("m_iFirstOutput", testResult.m_iFirstOutput);
            js.AddVariableInt32("m_iLastOutput", testResult.m_iLastOutput);

            // Sub Tests
            js.BeginArray("subTests");
            {
              xiiUInt32 uiSubTests = GetSubTestCount(uiTestIdx);
              for (xiiUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTests; ++uiSubTestIdx)
              {
                const xiiTestResultData& subTestResult = GetTestResultData(uiTestIdx, uiSubTestIdx);
                js.BeginObject();
                {
                  js.AddVariableString("m_sName", subTestResult.m_sName.c_str());
                  js.AddVariableBool("m_bExecuted", subTestResult.m_bExecuted);
                  js.AddVariableBool("m_bSuccess", subTestResult.m_bSuccess);
                  js.AddVariableInt32("m_iTestAsserts", subTestResult.m_iTestAsserts);
                  js.AddVariableDouble("m_fTestDuration", subTestResult.m_fTestDuration);
                  js.AddVariableInt32("m_iFirstOutput", subTestResult.m_iFirstOutput);
                  js.AddVariableInt32("m_iLastOutput", subTestResult.m_iLastOutput);
                }
                js.EndObject();
              }
            }
            js.EndArray(); // subTests
          }
          js.EndObject();
        }
      }
      js.EndArray(); // tests
    }
    js.EndObject();
  }

  return true;
}

xiiUInt32 xiiTestFrameworkResult::GetTestCount(xiiTestResultQuery::Enum countQuery) const
{
  xiiUInt32       uiAccumulator = 0;
  const xiiUInt32 uiTests       = (xiiUInt32)m_Tests.size();

  if (countQuery == xiiTestResultQuery::Count)
    return uiTests;

  if (countQuery == xiiTestResultQuery::Errors)
    return (xiiUInt32)m_Errors.size();

  for (xiiUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    switch (countQuery)
    {
      case xiiTestResultQuery::Executed:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case xiiTestResultQuery::Success:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

xiiUInt32 xiiTestFrameworkResult::GetSubTestCount(xiiUInt32 uiTestIndex, xiiTestResultQuery::Enum countQuery) const
{
  if (uiTestIndex >= (xiiUInt32)m_Tests.size())
    return 0;

  const xiiTestResult& test          = m_Tests[uiTestIndex];
  xiiUInt32            uiAccumulator = 0;
  const xiiUInt32      uiSubTests    = (xiiUInt32)test.m_SubTests.size();

  if (countQuery == xiiTestResultQuery::Count)
    return uiSubTests;

  if (countQuery == xiiTestResultQuery::Errors)
  {
    for (xiiInt32 iOutputIdx = test.m_Result.m_iFirstOutput; iOutputIdx <= test.m_Result.m_iLastOutput && iOutputIdx != -1; ++iOutputIdx)
    {
      if (m_TestOutput[iOutputIdx].m_Type == xiiTestOutput::Error)
        uiAccumulator++;
    }
    return uiAccumulator;
  }

  for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    switch (countQuery)
    {
      case xiiTestResultQuery::Executed:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case xiiTestResultQuery::Success:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

xiiInt32 xiiTestFrameworkResult::GetTestIndexByName(const char* szTestName) const
{
  xiiInt32 iTestCount = (xiiInt32)GetTestCount();
  for (xiiInt32 i = 0; i < iTestCount; ++i)
  {
    if (m_Tests[i].m_Result.m_sName.compare(szTestName) == 0)
      return i;
  }
  return -1;
}

xiiInt32 xiiTestFrameworkResult::GetSubTestIndexByName(xiiUInt32 uiTestIndex, const char* szSubTestName) const
{
  if (uiTestIndex >= GetTestCount())
    return -1;

  xiiInt32 iSubTestCount = (xiiInt32)GetSubTestCount(uiTestIndex);
  for (xiiInt32 i = 0; i < iSubTestCount; ++i)
  {
    if (m_Tests[uiTestIndex].m_SubTests[i].m_Result.m_sName.compare(szSubTestName) == 0)
      return i;
  }
  return -1;
}

double xiiTestFrameworkResult::GetTotalTestDuration() const
{
  double          fTotalTestDuration = 0.0;
  const xiiUInt32 uiTests            = (xiiUInt32)m_Tests.size();
  for (xiiUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    fTotalTestDuration += m_Tests[uiTest].m_Result.m_fTestDuration;
  }
  return fTotalTestDuration;
}

const xiiTestResultData& xiiTestFrameworkResult::GetTestResultData(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex) const
{
  return (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;
}

void xiiTestFrameworkResult::TestOutput(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, xiiTestOutput::Enum Type, const char* szMsg)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.AddOutput((xiiInt32)m_TestOutput.size());
    if (iSubTestIndex != -1)
    {
      m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.AddOutput((xiiInt32)m_TestOutput.size());
    }
  }

  m_TestOutput.push_back(xiiTestOutputMessage());
  xiiTestOutputMessage& outputMessage = *m_TestOutput.rbegin();
  outputMessage.m_Type                = Type;
  outputMessage.m_sMessage.assign(szMsg);
}

void xiiTestFrameworkResult::TestError(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, const char* szError, const char* szBlock, const char* szFile, xiiInt32 iLine, const char* szFunction, const char* szMsg)
{
  // In case there is no message set, we use the error as the message.
  TestOutput(uiTestIndex, iSubTestIndex, xiiTestOutput::Error, szError);
  m_TestOutput.rbegin()->m_iErrorIndex = (xiiInt32)m_Errors.size();

  m_Errors.push_back(xiiTestErrorMessage());
  xiiTestErrorMessage& errorMessage = *m_Errors.rbegin();
  errorMessage.m_sError.assign(szError);
  errorMessage.m_sBlock.assign(szBlock);
  errorMessage.m_sFile.assign(szFile);
  errorMessage.m_iLine = iLine;
  errorMessage.m_sFunction.assign(szFunction);
  errorMessage.m_sMessage.assign(szMsg);
}

void xiiTestFrameworkResult::TestResult(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  xiiTestResultData& Result = (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;

  Result.m_bExecuted     = true;
  Result.m_bSuccess      = bSuccess;
  Result.m_fTestDuration = fDuration;

  // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
  // Final time will be set again once the entire test finishes and currently these times are identical as
  // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_fTestDuration += fDuration;
  }
}

void xiiTestFrameworkResult::AddAsserts(xiiUInt32 uiTestIndex, xiiInt32 iSubTestIndex, int iCount)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_iTestAsserts += iCount;
  }

  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.m_iTestAsserts += iCount;
  }
}

xiiUInt32 xiiTestFrameworkResult::GetOutputMessageCount(xiiInt32 iTestIndex, xiiInt32 iSubTestIndex, xiiTestOutput::Enum Type) const
{
  if (iTestIndex == -1 && Type == xiiTestOutput::AllOutputTypes)
    return (xiiUInt32)m_TestOutput.size();

  xiiInt32 iStartIdx = 0;
  xiiInt32 iEndIdx   = (xiiInt32)m_TestOutput.size() - 1;

  if (iTestIndex != -1)
  {
    const xiiTestResultData& result = GetTestResultData(iTestIndex, iSubTestIndex);
    iStartIdx                       = result.m_iFirstOutput;
    iEndIdx                         = result.m_iLastOutput;

    // If no messages have been output (yet) for the given test we early-out here.
    if (iStartIdx == -1)
      return 0;

    // If all message types should be counted we can simply return the range.
    if (Type == xiiTestOutput::AllOutputTypes)
      return iEndIdx - iStartIdx + 1;
  }

  xiiUInt32 uiAccumulator = 0;
  for (xiiInt32 uiOutputMessageIdx = iStartIdx; uiOutputMessageIdx <= iEndIdx; ++uiOutputMessageIdx)
  {
    if (m_TestOutput[uiOutputMessageIdx].m_Type == Type)
      uiAccumulator++;
  }
  return uiAccumulator;
}

const xiiTestOutputMessage* xiiTestFrameworkResult::GetOutputMessage(xiiUInt32 uiOutputMessageIdx) const
{
  return &m_TestOutput[uiOutputMessageIdx];
}

xiiUInt32 xiiTestFrameworkResult::GetErrorMessageCount(xiiInt32 iTestIndex, xiiInt32 iSubTestIndex) const
{
  // If no test is given we can simply return the total error count.
  if (iTestIndex == -1)
  {
    return (xiiUInt32)m_Errors.size();
  }

  return GetOutputMessageCount(iTestIndex, iSubTestIndex, xiiTestOutput::Error);
}

const xiiTestErrorMessage* xiiTestFrameworkResult::GetErrorMessage(xiiUInt32 uiErrorMessageIdx) const
{
  return &m_Errors[uiErrorMessageIdx];
}


////////////////////////////////////////////////////////////////////////
// xiiTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void xiiTestFrameworkResult::xiiTestResult::Reset()
{
  m_Result.Reset();
  const xiiUInt32 uiSubTestCount = (xiiUInt32)m_SubTests.size();
  for (xiiUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
  {
    m_SubTests[uiSubTest].m_Result.Reset();
  }
}



XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestResults);
