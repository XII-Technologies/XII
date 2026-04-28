/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Stopwatch.h>

XII_CREATE_SIMPLE_TEST(Time, Stopwatch)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "General Functionality")
  {
    xiiStopwatch sw;

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(50));

    sw.StopAndReset();
    sw.Resume();

    const xiiTime t0 = sw.Checkpoint();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

    const xiiTime t1 = sw.Checkpoint();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(20));

    const xiiTime t2 = sw.Checkpoint();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(30));

    const xiiTime t3 = sw.Checkpoint();

    const xiiTime tTotal1 = sw.GetRunningTotal();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

    sw.Pause(); // freeze the current running total

    const xiiTime tTotal2 = sw.GetRunningTotal();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10)); // should not affect the running total anymore

    const xiiTime tTotal3 = sw.GetRunningTotal();


    // these tests are deliberately written such that they cannot fail,
    // even when the OS is under heavy load

    XII_TEST_BOOL(t0 > xiiTime::MakeFromMilliseconds(5));
    XII_TEST_BOOL(t1 > xiiTime::MakeFromMilliseconds(5));
    XII_TEST_BOOL(t2 > xiiTime::MakeFromMilliseconds(5));
    XII_TEST_BOOL(t3 > xiiTime::MakeFromMilliseconds(5));


    XII_TEST_BOOL(t1 + t2 + t3 <= tTotal1);
    XII_TEST_BOOL(t0 + t1 + t2 + t3 > tTotal1);

    XII_TEST_BOOL(tTotal1 < tTotal2);
    XII_TEST_BOOL(tTotal1 < tTotal3);
    XII_TEST_BOOL(tTotal2 == tTotal3);
  }
}
