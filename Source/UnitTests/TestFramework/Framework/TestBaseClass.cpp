#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Framework/TestFramework.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiTestBaseClass);

const char* xiiTestBaseClass::GetSubTestName(xiiInt32 iIdentifier) const
{
  if (iIdentifier < 0 || static_cast<std::size_t>(iIdentifier) > m_Entries.size())
  {
    xiiLog::Error("Tried to access retrieve sub-test name using invalid identifier.");
    return "";
  }

  return m_Entries[iIdentifier].m_szName;
}

void xiiTestBaseClass::UpdateConfiguration(xiiTestConfiguration& ref_config) const
{
  // If the configuration hasn't been set yet this is the first instance of xiiTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of xiiTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (ref_config.m_uiInstalledMainMemory == 0)
  {
    const xiiSystemInformation& pSysInfo = xiiSystemInformation::Get();
    ref_config.m_uiInstalledMainMemory   = pSysInfo.GetInstalledMainMemory();
    ref_config.m_uiMemoryPageSize        = pSysInfo.GetMemoryPageSize();
    ref_config.m_uiCPUCoreCount          = pSysInfo.GetCPUCoreCount();
    ref_config.m_sPlatformName           = pSysInfo.GetPlatformName();
    ref_config.m_b64BitOS                = pSysInfo.Is64BitOS();
    ref_config.m_b64BitApplication       = XII_ENABLED(XII_PLATFORM_64BIT);
    ref_config.m_sBuildConfiguration     = pSysInfo.GetBuildConfiguration();
    ref_config.m_iDateTime               = xiiTimestamp::CurrentTimestamp().GetInt64(xiiSIUnitOfTime::Second);
    ref_config.m_iRCSRevision            = xiiTestFramework::GetInstance()->GetSettings().m_iRevision;
    ref_config.m_sHostName               = pSysInfo.GetHostName();
  }
}

void xiiTestBaseClass::MapImageNumberToString(const char* szTestName, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber, xiiStringBuilder& out_sString) const
{
  out_sString.Format("{0}_{1}_{2}", szTestName, subTest.m_szSubTestName, xiiArgI(uiImageNumber, 3, true));
  out_sString.ReplaceAll(" ", "_");
}

void xiiTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
}

void xiiTestBaseClass::AddSubTest(const char* szName, xiiInt32 iIdentifier)
{
  XII_ASSERT_DEV(szName != nullptr, "Sub test name must not be nullptr");

  TestEntry e;
  e.m_szName      = szName;
  e.m_iIdentifier = iIdentifier;

  m_Entries.push_back(e);
}

xiiResult xiiTestBaseClass::DoTestInitialization()
{
  try
  {
    if (InitializeTest() == XII_FAILURE)
    {
      xiiTestFramework::Output(xiiTestOutput::Error, "Test Initialization failed.");
      return XII_FAILURE;
    }
  }
  catch (...)
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "Exception during test initialization.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiTestBaseClass::DoTestDeInitialization()
{
  try

  {
    if (DeInitializeTest() == XII_FAILURE)
      xiiTestFramework::Output(xiiTestOutput::Error, "Test DeInitialization failed.");
  }
  catch (...)
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "Exception during test de-initialization.");
  }
}

xiiResult xiiTestBaseClass::DoSubTestInitialization(xiiInt32 iIdentifier)
{
  try
  {
    if (InitializeSubTest(iIdentifier) == XII_FAILURE)
    {
      xiiTestFramework::Output(xiiTestOutput::Error, "Sub-Test Initialization failed, skipping Test.");
      return XII_FAILURE;
    }
  }
  catch (...)
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "Exception during sub-test initialization.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiTestBaseClass::DoSubTestDeInitialization(xiiInt32 iIdentifier)
{
  try
  {
    if (DeInitializeSubTest(iIdentifier) == XII_FAILURE)
      xiiTestFramework::Output(xiiTestOutput::Error, "Sub-Test De-Initialization failed.");
  }
  catch (...)
  {
    xiiTestFramework::Output(xiiTestOutput::Error, "Exception during sub-test de-initialization.");
  }
}

xiiTestAppRun xiiTestBaseClass::DoSubTestRun(xiiInt32 iIdentifier, double& fDuration, xiiUInt32 uiInvocationCount)
{
  fDuration = 0.0;

  xiiTestAppRun ret = xiiTestAppRun::Quit;

  try
  {
    xiiTime StartTime = xiiTime::Now();

    ret = RunSubTest(iIdentifier, uiInvocationCount);

    fDuration = (xiiTime::Now() - StartTime).GetMilliseconds();
  }
  catch (...)
  {
    xiiInt32 iEntry = -1;

    for (xiiInt32 i = 0; i < (xiiInt32)m_Entries.size(); ++i)
    {
      if (m_Entries[i].m_iIdentifier == iIdentifier)
      {
        iEntry = i;
        break;
      }
    }

    if (iEntry >= 0)
      xiiTestFramework::Output(xiiTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iEntry].m_szName);
    else
      xiiTestFramework::Output(xiiTestOutput::Error, "Exception during unknown sub-test.");
  }

  return ret;
}


XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestBaseClass);
