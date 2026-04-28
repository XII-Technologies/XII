/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Clock.h>

class xiiSimpleTimeStepSmoother : public xiiTimeStepSmoothing
{
public:
  virtual xiiTime GetSmoothedTimeStep(xiiTime rawTimeStep, const xiiClock* pClock) override { return xiiTime::MakeFromSeconds(0.42); }

  virtual void Reset(const xiiClock* pClock) override {}
};

XII_CREATE_SIMPLE_TEST(Time, Clock)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor / Reset")
  {
    xiiClock c("Test"); // calls 'Reset' internally

    XII_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor

    XII_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 0.0, 0.0);
    XII_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);
    XII_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);
    XII_TEST_BOOL(c.GetPaused() == false);
    XII_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants
    XII_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0);   // to ensure the tests fail if somebody changes these constants
    XII_TEST_BOOL(c.GetTimeDiff() > xiiTime::MakeFromSeconds(0.0));

    xiiSimpleTimeStepSmoother s;

    c.SetTimeStepSmoothing(&s);

    XII_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(false);

    // does NOT reset which time step smoother to use
    XII_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(true);
    XII_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetPaused / GetPaused")
  {
    xiiClock c("Test");
    XII_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    XII_TEST_BOOL(c.GetPaused());

    c.SetPaused(false);
    XII_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    XII_TEST_BOOL(c.GetPaused());

    c.Reset(false);
    XII_TEST_BOOL(!c.GetPaused());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Updates while Paused / Unpaused")
  {
    xiiClock c("Test");

    c.SetPaused(false);

    const xiiTime t0 = c.GetAccumulatedTime();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    c.Update();

    const xiiTime t1 = c.GetAccumulatedTime();
    XII_TEST_BOOL(t0 < t1);

    c.SetPaused(true);

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    c.Update();

    const xiiTime t2 = c.GetAccumulatedTime();
    XII_TEST_BOOL(t1 == t2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFixedTimeStep / GetFixedTimeStep")
  {
    xiiClock c("Test");

    XII_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);

    c.SetFixedTimeStep(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Updates with fixed time step")
  {
    xiiClock c("Test");
    c.SetFixedTimeStep(xiiTime::MakeFromSeconds(1.0 / 60.0));
    c.Update();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

    c.Update();
    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(50));

    c.Update();
    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    c.Update();
    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetAccumulatedTime / GetAccumulatedTime")
  {
    xiiClock c("Test");

    c.SetAccumulatedTime(xiiTime::MakeFromSeconds(23.42));

    XII_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 23.42, 0.000001);

    c.Update(); // by default after a SetAccumulatedTime the time diff should always be > 0

    XII_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);

    const xiiTime t0 = c.GetAccumulatedTime();

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
    c.Update();

    const xiiTime t1 = c.GetAccumulatedTime();

    XII_TEST_BOOL(t1 > t0);
    XII_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetSpeed / GetSpeed / GetTimeDiff")
  {
    xiiClock c("Test");
    XII_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);

    c.SetFixedTimeStep(xiiTime::MakeFromSeconds(0.01));

    c.SetSpeed(10.0);
    XII_TEST_DOUBLE(c.GetSpeed(), 10.0, 0.000001);

    c.Update();
    const xiiTime t0 = c.GetTimeDiff();
    XII_TEST_DOUBLE(t0.GetSeconds(), 0.1, 0.00001);

    c.SetSpeed(0.1);

    c.Update();
    const xiiTime t1 = c.GetTimeDiff();
    XII_TEST_DOUBLE(t1.GetSeconds(), 0.001, 0.00001);

    c.Reset(false);

    c.Update();
    const xiiTime t2 = c.GetTimeDiff();
    XII_TEST_DOUBLE(t2.GetSeconds(), 0.01, 0.00001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetMinimumTimeStep / GetMinimumTimeStep")
  {
    xiiClock c("Test");
    XII_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants

    c.Update();
    c.Update();

    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMinimumTimeStep(xiiTime::MakeFromSeconds(0.1));
    c.SetMaximumTimeStep(xiiTime::MakeFromSeconds(1.0));

    XII_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.1, 0.0);

    c.Update();
    c.Update();

    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetMaximumTimeStep / GetMaximumTimeStep")
  {
    xiiClock c("Test");
    XII_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0); // to ensure the tests fail if somebody changes these constants

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(200));
    c.Update();

    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMaximumTimeStep(xiiTime::MakeFromSeconds(0.2));

    XII_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.2, 0.0);

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(400));
    c.Update();

    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetTimeStepSmoothing / GetTimeStepSmoothing")
  {
    xiiClock c("Test");

    XII_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr);

    xiiSimpleTimeStepSmoother s;
    c.SetTimeStepSmoothing(&s);

    XII_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.SetMaximumTimeStep(xiiTime::MakeFromSeconds(10.0)); // this would limit the time step even after smoothing
    c.Update();

    XII_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 0.42, 0.0);
  }
}
