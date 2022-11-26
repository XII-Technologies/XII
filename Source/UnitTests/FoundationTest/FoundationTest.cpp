#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <iostream>

xiiInt32 xiiConstructionCounter::s_iConstructions     = 0;
xiiInt32 xiiConstructionCounter::s_iDestructions      = 0;
xiiInt32 xiiConstructionCounter::s_iConstructionsLast = 0;
xiiInt32 xiiConstructionCounter::s_iDestructionsLast  = 0;

xiiInt32 xiiConstructionCounterRelocatable::s_iConstructions     = 0;
xiiInt32 xiiConstructionCounterRelocatable::s_iDestructions      = 0;
xiiInt32 xiiConstructionCounterRelocatable::s_iConstructionsLast = 0;
xiiInt32 xiiConstructionCounterRelocatable::s_iDestructionsLast  = 0;

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("FoundationTest", "Foundation Tests")
{
  xiiCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, xiiCommandLineUtils::PreferOsArgs);

  // if the -cmd switch is set, FoundationTest.exe will execute a couple of simple operations and then close
  // this is used to test process launching (e.g. xiiProcess)
  if (cmd.GetBoolOption("-cmd"))
  {
    // print something to stdout
    const char* szStdOut = cmd.GetStringOption("-stdout");
    if (!xiiStringUtils::IsNullOrEmpty(szStdOut))
    {
      std::cout << szStdOut;
    }

    const char* szStdErr = cmd.GetStringOption("-stderr");
    if (!xiiStringUtils::IsNullOrEmpty(szStdErr))
    {
      std::cerr << szStdErr;
    }

    // wait a little
    xiiThreadUtils::Sleep(xiiTime::Milliseconds(cmd.GetIntOption("-sleep")));

    // shutdown with exit code
    xiiTestSetup::DeInitTestFramework(true);
    return cmd.GetIntOption("-exitcode");
  }
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
