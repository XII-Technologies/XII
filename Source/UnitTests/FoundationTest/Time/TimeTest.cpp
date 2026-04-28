/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Time/Time.h>

XII_CREATE_SIMPLE_TEST_GROUP(Time);

XII_CREATE_SIMPLE_TEST(Time, Timer)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
    xiiTime TestTime = xiiTime::Now();

    XII_TEST_BOOL(TestTime.GetMicroseconds() > 0.0);

    volatile xiiUInt32 testValue = 0;
    for (xiiUInt32 i = 0; i < 42000; ++i)
    {
      testValue += 23;
    }

    xiiTime TestTime2 = xiiTime::Now();

    XII_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);

    TestTime2 -= TestTime;

    XII_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);
  }
}
