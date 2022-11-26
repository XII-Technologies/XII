#include <FoundationTest/FoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Utilities/CommandLineUtils.h>

XII_CREATE_SIMPLE_TEST(System, ProcessGroup)
{
  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const xiiStringBuilder pathToSelf = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "WaitToFinish")
  {
    xiiProcessGroup  pgroup;
    xiiStringBuilder out;

    xiiMutex mutex;

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      xiiProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_onStdOut = [&out, &mutex](xiiStringView view) {
        XII_LOCK(mutex);
        out.Append(view);
      };

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("1000");
      opt.m_Arguments.PushBack("-stdout");
      opt.m_Arguments.PushBack("Na");

      XII_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    // in a debugger with child debugging enabled etc. even 10 seconds can lead to timeouts due to long delays in the IDE
    XII_TEST_BOOL(pgroup.WaitToFinish(xiiTime::Seconds(60)).Succeeded());
    XII_TEST_STRING(out, "NaNaNaNaNaNaNaNa"); // BATMAN!
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TerminateAll")
  {
    xiiProcessGroup pgroup;

    xiiHybridArray<xiiProcess, 8> procs;

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      xiiProcessOptions opt;
      opt.m_sProcess = pathToSelf;

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("60000");

      XII_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    const xiiTime tStart = xiiTime::Now();
    XII_TEST_BOOL(pgroup.TerminateAll().Succeeded());
    const xiiTime tDiff = xiiTime::Now() - tStart;

    XII_TEST_BOOL(tDiff < xiiTime::Seconds(10));
  }
}
#endif
