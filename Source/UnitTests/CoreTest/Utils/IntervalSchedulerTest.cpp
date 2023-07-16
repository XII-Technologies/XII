#include <CoreTest/CoreTestPCH.h>

#include <Core/Utils/IntervalScheduler.h>

XII_CREATE_SIMPLE_TEST_GROUP(Utils);

namespace
{
  struct TestWork
  {
    float     m_IntervalMs = 0.0f;
    xiiUInt32 m_Counter    = 0;

    void Run()
    {
      ++m_Counter;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Utils, IntervalScheduler)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant workload")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    xiiHybridArray<TestWork, 32>    works;
    xiiIntervalScheduler<TestWork*> scheduler;

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(intervals); ++i)
    {
      auto& work        = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, xiiTime::Milliseconds(work.m_IntervalMs));
    }

    xiiUInt32 wrongDelta = 0;
    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(xiiTime::Milliseconds(10), [&](TestWork* pWork, xiiTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs  = deltaTime.GetMilliseconds();
          const double variance = pWork->m_IntervalMs * 0.3;
          const double midValue = pWork->m_IntervalMs + 1.0 - variance;
          if (xiiMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        if (pWork->m_IntervalMs <= 10.0f)
        {
          XII_TEST_INT(static_cast<xiiUInt32>(pWork->m_Counter), i);
        }

        pWork->Run();
        ++fNumWorks;
      });

      XII_TEST_FLOAT(fNumWorks, 2.5f, 0.5f);

      for (auto& work : works)
      {
        XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::Milliseconds(work.m_IntervalMs));
      }
    }

    // 2 wrong deltas for ~120 scheduled works is ok
    XII_TEST_BOOL(wrongDelta <= 2);

    for (auto& work : works)
    {
      const float expectedCounter = 600.0f / xiiMath::Max(work.m_IntervalMs, 10.0f);

      // check for roughly expected or a little bit more
      XII_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 3.0f, 4.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constant workload (bigger delta)")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    xiiHybridArray<TestWork, 32>    works;
    xiiIntervalScheduler<TestWork*> scheduler;

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(intervals); ++i)
    {
      auto& work        = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, xiiTime::Milliseconds(work.m_IntervalMs));
    }

    xiiUInt32 wrongDelta = 0;
    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(xiiTime::Milliseconds(20), [&](TestWork* pWork, xiiTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs  = deltaTime.GetMilliseconds();
          const double variance = xiiMath::Max(pWork->m_IntervalMs, 20.0f) * 0.3;
          const double midValue = xiiMath::Max(pWork->m_IntervalMs, 20.0f) + 1.0 - variance;
          if (xiiMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        if (pWork->m_IntervalMs <= 20.0f)
        {
          XII_TEST_INT(static_cast<xiiUInt32>(pWork->m_Counter), i);
        }

        pWork->Run();
        ++fNumWorks;
      });

      XII_TEST_FLOAT(fNumWorks, 3.5f, 0.5f);

      for (auto& work : works)
      {
        XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::Milliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~150 scheduled works is ok
    XII_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = 1200.0f / xiiMath::Max(work.m_IntervalMs, 20.0f);

      // check for roughly expected or a little bit more
      XII_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 2.0f, 3.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dynamic workload")
  {
    xiiHybridArray<TestWork, 32> works;

    xiiIntervalScheduler<TestWork*> scheduler;

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, xiiTime::Milliseconds(i));
    }

    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(xiiTime::Milliseconds(10), [&](TestWork* pWork, xiiTime deltaTime) {
        pWork->Run();
        ++fNumWorks;
      });

      XII_TEST_FLOAT(fNumWorks, 15.5f, 0.5f);
    }

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, xiiTime::Milliseconds(20 + i));
    }

    float fPrevNumWorks = 15.5f;
    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0.0f;
      scheduler.Update(xiiTime::Milliseconds(10), [&](TestWork* pWork, xiiTime deltaTime) {
        pWork->Run();
        ++fNumWorks;
      });

      // fNumWork will slowly ramp up until it reaches the new workload of 22 or 23 per update
      XII_TEST_BOOL(fNumWorks + 1.0f >= fPrevNumWorks);
      XII_TEST_BOOL(fNumWorks <= 23.0f);

      fPrevNumWorks = fNumWorks;
    }

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i];
      scheduler.RemoveWork(&work);
    }

    scheduler.Update(xiiTime::Milliseconds(10), xiiIntervalScheduler<TestWork*>::RunWorkCallback());

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::Milliseconds(20 + i));

      scheduler.AddOrUpdateWork(&work, xiiTime::Milliseconds(100 + i));
    }

    scheduler.Update(xiiTime::Milliseconds(10), xiiIntervalScheduler<TestWork*>::RunWorkCallback());

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::Milliseconds(100 + i));
    }
  }
}
