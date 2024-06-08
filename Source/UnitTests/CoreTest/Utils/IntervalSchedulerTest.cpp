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

      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr xiiUInt32 uiNumIterations = 60;
    constexpr xiiTime   timeStep        = xiiTime::MakeFromMilliseconds(10);

    xiiUInt32 wrongDelta = 0;
    for (xiiUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, xiiTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = pWork->m_IntervalMs * 0.3;
          const double midValue = pWork->m_IntervalMs + 1.0 - variance;
          if (xiiMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      XII_TEST_FLOAT(fNumWorks, 2.5f, 0.5f);

      for (auto& work : works)
      {
        XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~120 scheduled works is ok
    XII_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / xiiMath::Max(work.m_IntervalMs, 10.0f);

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

      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr xiiUInt32 uiNumIterations = 60;
    constexpr xiiTime   timeStep        = xiiTime::MakeFromMilliseconds(20);

    xiiUInt32 wrongDelta = 0;
    for (xiiUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, xiiTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = xiiMath::Max(pWork->m_IntervalMs, 20.0f) * 0.3;
          const double midValue = xiiMath::Max(pWork->m_IntervalMs, 20.0f) + 1.0 - variance;
          if (xiiMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      XII_TEST_FLOAT(fNumWorks, 3.5f, 0.5f);

      for (auto& work : works)
      {
        XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~150 scheduled works is ok
    XII_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / xiiMath::Max(work.m_IntervalMs, 20.0f);

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
      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(i));
    }

    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(xiiTime::MakeFromMilliseconds(10), [&](TestWork* pWork, xiiTime deltaTime) {
        pWork->Run();
        ++fNumWorks;
      });

      XII_TEST_FLOAT(fNumWorks, 15.5f, 0.5f);
    }

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(20 + i));
    }

    float fPrevNumWorks = 15.5f;
    for (xiiUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0.0f;
      scheduler.Update(xiiTime::MakeFromMilliseconds(10), [&](TestWork* pWork, xiiTime deltaTime) {
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

    scheduler.Update(xiiTime::MakeFromMilliseconds(10), xiiIntervalScheduler<TestWork*>::RunWorkCallback());

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::MakeFromMilliseconds(20 + i));

      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(100 + i));
    }

    scheduler.Update(xiiTime::MakeFromMilliseconds(10), xiiIntervalScheduler<TestWork*>::RunWorkCallback());

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      XII_TEST_BOOL(scheduler.GetInterval(&work) == xiiTime::MakeFromMilliseconds(100 + i));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Update/Remove during schedule")
  {
    xiiHybridArray<TestWork, 32> works;

    xiiIntervalScheduler<TestWork*> scheduler;

    for (xiiUInt32 i = 0; i < 32; ++i)
    {
      auto& work        = works.ExpandAndGetRef();
      work.m_IntervalMs = static_cast<float>((i & 1u));

      scheduler.AddOrUpdateWork(&work, xiiTime::MakeFromMilliseconds(i));
    }

    xiiUInt32 uiNumWorks = 0;
    scheduler.Update(xiiTime::MakeFromMilliseconds(33),
                     [&](TestWork* pWork, xiiTime deltaTime) {
                       pWork->Run();
                       ++uiNumWorks;

                       if (pWork->m_IntervalMs == 0.0f)
                       {
                         scheduler.RemoveWork(pWork);
                       }
                       else
                       {
                         scheduler.AddOrUpdateWork(pWork, xiiTime::MakeFromMilliseconds(50));
                       }
                     });

    XII_TEST_INT(uiNumWorks, 32);
    for (xiiUInt32 i = 0; i < 32; ++i)
    {
      const xiiUInt32 uiExpectedCounter = 1;
      XII_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }

    uiNumWorks = 0;
    scheduler.Update(xiiTime::MakeFromMilliseconds(100),
                     [&](TestWork* pWork, xiiTime deltaTime) {
                       XII_TEST_FLOAT(pWork->m_IntervalMs, 1.0f, xiiMath::DefaultEpsilon<float>());

                       pWork->Run();
                       ++uiNumWorks;
                     });

    XII_TEST_INT(uiNumWorks, 16);
    for (xiiUInt32 i = 0; i < 32; ++i)
    {
      const xiiUInt32 uiExpectedCounter = 1 + (i & 1);
      XII_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }
  }
}
