#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <TestFramework/Framework/TestFramework.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiRegisterSimpleTestHelper);

void xiiSimpleTestGroup::AddSimpleTest(const char* szName, SimpleTestFunc testFunc)
{
  SimpleTestEntry e;
  e.m_szName = szName;
  e.m_Func   = testFunc;

  for (xiiUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    if ((strcmp(m_SimpleTests[i].m_szName, e.m_szName) == 0) && (m_SimpleTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleTests.push_back(e);
}

void xiiSimpleTestGroup::SetupSubTests()
{
  for (xiiUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    AddSubTest(m_SimpleTests[i].m_szName, i);
  }
}

xiiTestAppRun xiiSimpleTestGroup::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  xiiTestFramework::s_szTestBlockName = m_SimpleTests[iIdentifier].m_szName;

  XII_PROFILE_SCOPE(m_SimpleTests[iIdentifier].m_szName);
  m_SimpleTests[iIdentifier].m_Func();

  xiiTestFramework::s_szTestBlockName = "";
  return xiiTestAppRun::Quit;
}

xiiResult xiiSimpleTestGroup::InitializeSubTest(xiiInt32 iIdentifier)
{
  // initialize everything up to 'core'
  xiiStartup::StartupCoreSystems();
  return XII_SUCCESS;
}

xiiResult xiiSimpleTestGroup::DeInitializeSubTest(xiiInt32 iIdentifier)
{
  // shut down completely
  xiiStartup::ShutdownCoreSystems();
  xiiMemoryTracker::DumpMemoryLeaks();
  return XII_SUCCESS;
}


XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_SimpleTest);
