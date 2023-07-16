#include <TestFramework/TestFrameworkPCH.h>

#ifdef XII_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

////////////////////////////////////////////////////////////////////////
// xiiQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

xiiQtTestFramework::xiiQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv) :
  xiiTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
  Q_INIT_RESOURCE(resources);
  Initialize();
}

xiiQtTestFramework::~xiiQtTestFramework() = default;


////////////////////////////////////////////////////////////////////////
// xiiQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void xiiQtTestFramework::OutputImpl(xiiTestOutput::Enum Type, const char* szMsg)
{
  xiiTestFramework::OutputImpl(Type, szMsg);
}

void xiiQtTestFramework::TestResultImpl(xiiInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  xiiTestFramework::TestResultImpl(iSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_iCurrentTestIndex, iSubTestIndex);
}

#endif

XII_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestFramework);
