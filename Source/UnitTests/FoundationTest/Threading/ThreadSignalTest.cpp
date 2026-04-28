/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread2 : public xiiThread
  {
  public:
    TestThread2() :
      xiiThread("Test Thread")
    {
    }

    xiiThreadSignal*    m_pSignalAuto   = nullptr;
    xiiThreadSignal*    m_pSignalManual = nullptr;
    xiiAtomicInteger32* m_pCounter      = nullptr;
    bool                m_bTimeout      = false;

    virtual xiiUInt32 Run()
    {
      m_pCounter->Decrement();

      m_pSignalAuto->WaitForSignal();

      m_pCounter->Increment();

      if (m_bTimeout)
      {
        m_pSignalManual->WaitForSignal(xiiTime::MakeFromSeconds(0.5));
      }
      else
      {
        m_pSignalManual->WaitForSignal();
      }

      m_pCounter->Increment();

      return 0;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Threading, ThreadSignal)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr xiiUInt32 uiNumThreads = 32;

    xiiUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    xiiAtomicInteger32        iCounter = uiNumThreads;
    xiiThreadSignal           sigAuto(xiiThreadSignal::Mode::AutoReset);
    xiiThreadSignal           sigManual(xiiThreadSignal::Mode::ManualReset);

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]                  = XII_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter      = &iCounter;
      pTestThread2s[i]->m_pSignalAuto   = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      xiiThreadUtils::YieldTimeSlice();
    }

    for (xiiUInt32 t = 0; t < uiNumThreads; ++t)
    {
      const xiiInt32 iExpected = t + 1;

      sigAuto.RaiseSignal();

      for (xiiUInt32 a = 0; a < 1000; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      XII_TEST_INT(iCounter, iExpected);
      XII_TEST_BOOL(iCounter <= iExpected); // THIS test must never fail!
    }

    // wake up the rest
    {
      sigManual.RaiseSignal();

      for (xiiUInt32 a = 0; a < 1000; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

        if (iCounter >= (xiiInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      XII_TEST_INT(iCounter, (xiiInt32)uiNumThreads * 2);
      XII_TEST_BOOL(iCounter <= (xiiInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Wait With Timeout")
  {
    constexpr xiiUInt32 uiNumThreads = 16;

    xiiUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    xiiAtomicInteger32        iCounter = uiNumThreads;
    xiiThreadSignal           sigAuto(xiiThreadSignal::Mode::AutoReset);
    xiiThreadSignal           sigManual(xiiThreadSignal::Mode::ManualReset);

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]                  = XII_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter      = &iCounter;
      pTestThread2s[i]->m_pSignalAuto   = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->m_bTimeout      = true;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      xiiThreadUtils::YieldTimeSlice();
    }

    // raise the signal N times
    for (xiiUInt32 t = 0; t < uiNumThreads; ++t)
    {
      sigAuto.RaiseSignal();

      for (xiiUInt32 a = 0; a < 1000; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

        if (iCounter >= (xiiInt32)t + 1)
          break;
      }
    }

    // due to the wait timeout in the thread, testing this exact value here would be unreliable
    // XII_TEST_INT(iCounter, (xiiInt32)uiNumThreads);

    // just wait for the rest
    {
      for (xiiUInt32 a = 0; a < 100; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(50));

        if (iCounter >= (xiiInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      XII_TEST_INT(iCounter, (xiiInt32)uiNumThreads * 2);
      XII_TEST_BOOL(iCounter <= (xiiInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }
}
