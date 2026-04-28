/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread : public xiiThread
  {
  public:
    TestThread() :
      xiiThread("Test Thread")
    {
    }

    xiiConditionVariable* m_pCV      = nullptr;
    xiiAtomicInteger32*   m_pCounter = nullptr;

    virtual xiiUInt32 Run()
    {
      XII_LOCK(*m_pCV);

      m_pCounter->Decrement();

      m_pCV->UnlockWaitForSignalAndLock();

      m_pCounter->Increment();
      return 0;
    }
  };

  class TestThreadTimeout : public xiiThread
  {
  public:
    TestThreadTimeout() :
      xiiThread("Test Thread Timeout")
    {
    }

    xiiConditionVariable* m_pCV        = nullptr;
    xiiConditionVariable* m_pCVTimeout = nullptr;
    xiiAtomicInteger32*   m_pCounter   = nullptr;

    virtual xiiUInt32 Run()
    {
      // make sure all threads are put to sleep first
      {
        XII_LOCK(*m_pCV);
        m_pCounter->Decrement();
        m_pCV->UnlockWaitForSignalAndLock();
      }

      // this condition will never be met during the test
      // it should always run into the timeout
      XII_LOCK(*m_pCVTimeout);
      m_pCVTimeout->UnlockWaitForSignalAndLock(xiiTime::MakeFromSeconds(0.5));

      m_pCounter->Increment();
      return 0;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Threading, ConditionalVariable)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr xiiUInt32 uiNumThreads = 32;

    xiiUniquePtr<TestThread> pTestThreads[uiNumThreads];
    xiiAtomicInteger32       iCounter = uiNumThreads;
    xiiConditionVariable     cv;

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]             = XII_DEFAULT_NEW(TestThread);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV      = &cv;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      XII_LOCK(cv);
      if (iCounter == 0)
        break;

      xiiThreadUtils::YieldTimeSlice();
    }

    for (xiiUInt32 t = 0; t < uiNumThreads / 2; ++t)
    {
      const xiiInt32 iExpected = iCounter + 1;

      cv.SignalOne();

      for (xiiUInt32 a = 0; a < 1000; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // Theoretically this could fail, if the OS doesn't wake up any other thread in time but with 1000 tries that is very unlikely.
      // On some platforms like posix it is not guaranteed that exactly one thread is woken up, so we check that at least one thread was woken up.
      XII_TEST_BOOL(iCounter >= iExpected);
    }

    // wake up the rest
    {
      cv.SignalAll();

      for (xiiUInt32 a = 0; a < 1000; ++a)
      {
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

        if (iCounter >= (xiiInt32)uiNumThreads)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      XII_TEST_INT(iCounter, (xiiInt32)uiNumThreads);
      XII_TEST_BOOL(iCounter <= (xiiInt32)uiNumThreads); // THIS test must never fail!
    }

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Wait With timeout")
  {
    constexpr xiiUInt32 uiNumThreads = 16;

    xiiUniquePtr<TestThreadTimeout> pTestThreads[uiNumThreads];
    xiiAtomicInteger32              iCounter = uiNumThreads;
    xiiConditionVariable            cv;
    xiiConditionVariable            cvt;

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]               = XII_DEFAULT_NEW(TestThreadTimeout);
      pTestThreads[i]->m_pCounter   = &iCounter;
      pTestThreads[i]->m_pCV        = &cv;
      pTestThreads[i]->m_pCVTimeout = &cvt;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      XII_LOCK(cv);
      if (iCounter == 0)
        break;

      xiiThreadUtils::YieldTimeSlice();
    }

    // open the flood gates
    cv.SignalAll();

    // all threads should run into their timeout now
    for (xiiUInt32 a = 0; a < 100; ++a)
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(50));

      if (iCounter >= (xiiInt32)uiNumThreads)
        break;
    }

    // theoretically this could fail, if the OS doesn't wake up any other thread in time
    // but with 100 tries that is very unlikely
    XII_TEST_INT(iCounter, (xiiInt32)uiNumThreads);
    XII_TEST_BOOL(iCounter <= (xiiInt32)uiNumThreads); // THIS test must never fail!

    for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }
}
