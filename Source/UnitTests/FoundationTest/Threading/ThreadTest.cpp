#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>

namespace
{
  xiiInt32        g_iCrossThreadVariable = 0;
  const xiiUInt32 g_uiIncrementSteps     = 160000;

  class TestThread3 : public xiiThread
  {
  public:
    TestThread3() :
      xiiThread("Test Thread")
    {
    }

    xiiMutex* m_pWaitMutex    = nullptr;
    xiiMutex* m_pBlockedMutex = nullptr;

    virtual xiiUInt32 Run()
    {
      // test TryLock on a locked mutex
      XII_TEST_BOOL(m_pBlockedMutex->TryLock().Failed());

      {
        // enter and leave the mutex once
        XII_LOCK(*m_pWaitMutex);
      }

      XII_PROFILE_SCOPE("Test Thread::Run");

      for (xiiUInt32 i = 0; i < g_uiIncrementSteps; i++)
      {
        xiiAtomicUtils::Increment(g_iCrossThreadVariable);

        xiiTime::Now();
        xiiThreadUtils::YieldTimeSlice();
        xiiTime::Now();
      }

      return 0;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(Threading);

XII_CREATE_SIMPLE_TEST(Threading, Thread)
{
  g_iCrossThreadVariable = 0;


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Thread")
  {
    TestThread3* pTestThread31 = nullptr;
    TestThread3* pTestThread32 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread31 = new TestThread3;
      pTestThread32 = new TestThread3;
    }
    catch (...)
    {
    }

    XII_TEST_BOOL(pTestThread31 != nullptr);
    XII_TEST_BOOL(pTestThread32 != nullptr);

    xiiMutex waitMutex, blockedMutex;
    pTestThread31->m_pWaitMutex = &waitMutex;
    pTestThread32->m_pWaitMutex = &waitMutex;

    pTestThread31->m_pBlockedMutex = &blockedMutex;
    pTestThread32->m_pBlockedMutex = &blockedMutex;

    // no one holds these mutexes yet, must succeed
    XII_TEST_BOOL(blockedMutex.TryLock().Succeeded());
    XII_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Both thread will increment the global variable via atomic operations
    pTestThread31->Start();
    pTestThread32->Start();

    // give the threads a bit of time to start
    xiiThreadUtils::Sleep(xiiTime::Milliseconds(50));

    // allow the threads to run now
    waitMutex.Unlock();

    // Main thread will also increment the test variable
    xiiAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread31->Join();
    pTestThread32->Join();

    // we are holding the mutex, another TryLock should work
    XII_TEST_BOOL(blockedMutex.TryLock().Succeeded());

    // The threads should have finished, no one holds the lock
    XII_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Test deletion
    delete pTestThread31;
    delete pTestThread32;

    XII_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Thread Sleeping")
  {
    const xiiTime start = xiiTime::Now();

    xiiTime sleepTime(xiiTime::Seconds(0.3));

    xiiThreadUtils::Sleep(sleepTime);

    const xiiTime stop = xiiTime::Now();

    const xiiTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    XII_TEST_BOOL(duration.GetSeconds() > 0.25);
    XII_TEST_BOOL_MSG(duration.GetSeconds() < 1.0, "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
