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
    xiiStringView sStdOut = cmd.GetStringOption("-stdout");
    if (!sStdOut.IsEmpty())
    {
      xiiStringBuilder tmp;
      std::cout << sStdOut.GetData(tmp);
    }

    xiiStringView sStdErr = cmd.GetStringOption("-stderr");
    if (!sStdErr.IsEmpty())
    {
      xiiStringBuilder tmp;
      std::cerr << sStdErr.GetData(tmp);
    }

    // wait a little
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(cmd.GetIntOption("-sleep")));

    // shutdown with exit code
    xiiTestSetup::DeInitTestFramework(true);
    return cmd.GetIntOption("-exitcode");
  }
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
