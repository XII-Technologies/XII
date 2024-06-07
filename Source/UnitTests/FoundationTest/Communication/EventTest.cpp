#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Event.h>

XII_CREATE_SIMPLE_TEST_GROUP(Communication);

namespace
{
  struct Test
  {
    void DoStuff(xiiInt32* pEventData) { *pEventData += m_iData; }

    xiiInt32 m_iData;
  };

  struct TestRecursion
  {
    TestRecursion() { m_uiRecursionCount = 0; }
    void DoStuff(xiiUInt32 uiRecursions)
    {
      if (m_uiRecursionCount < uiRecursions)
      {
        m_uiRecursionCount++;
        m_Event.Broadcast(uiRecursions, 10);
      }
    }

    using Event = xiiEvent<xiiUInt32>;
    Event     m_Event;
    xiiUInt32 m_uiRecursionCount;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Communication, Event)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
    using TestEvent = xiiEvent<xiiInt32*>;
    TestEvent e;

    Test test1;
    test1.m_iData = 3;

    Test test2;
    test2.m_iData = 5;

    xiiInt32 iResult = 0;

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    XII_TEST_INT(iResult, 3);

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    XII_TEST_INT(iResult, 8);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    XII_TEST_INT(iResult, 5);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    XII_TEST_INT(iResult, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unsubscribing via ID")
  {
    using TestEvent = xiiEvent<xiiInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    auto subId1 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    auto subId2 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    e.RemoveEventHandler(subId1);
    XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    e.RemoveEventHandler(subId2);
    XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unsubscribing via Unsubscriber")
  {
    using TestEvent = xiiEvent<xiiInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    {
      TestEvent::Unsubscriber unsub1;

      {
        TestEvent::Unsubscriber unsub2;

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1), unsub1);
        XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2), unsub2);
        XII_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
      }

      XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
    }

    XII_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Recursion")
  {
    for (xiiUInt32 i = 0; i < 10; i++)
    {
      TestRecursion test;
      test.m_Event.AddEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
      test.m_Event.Broadcast(i, 10);
      XII_TEST_INT(test.m_uiRecursionCount, i);
      test.m_Event.RemoveEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove while iterate")
  {
    using TestEvent = xiiEvent<int, xiiMutex, xiiDefaultAllocatorWrapper, xiiEventType::CopyOnBroadcast>;
    TestEvent e;

    xiiUInt32 callMap = 0;

    xiiEventSubscriptionID subscriptions[4] = {};

    subscriptions[0] = e.AddEventHandler(TestEvent::Handler([&](int i) { callMap |= XII_BIT(0); }));

    subscriptions[1] = e.AddEventHandler(TestEvent::Handler([&](int i) {
      callMap |= XII_BIT(1);
      e.RemoveEventHandler(subscriptions[1]);
    }));

    subscriptions[2] = e.AddEventHandler(TestEvent::Handler([&](int i) {
      callMap |= XII_BIT(2);
      e.RemoveEventHandler(subscriptions[2]);
      e.RemoveEventHandler(subscriptions[3]);
    }));

    subscriptions[3] = e.AddEventHandler(TestEvent::Handler([&](int i) { callMap |= XII_BIT(3); }));

    e.Broadcast(0);

    XII_TEST_BOOL(callMap == (XII_BIT(0) | XII_BIT(1) | XII_BIT(2) | XII_BIT(3)));

    callMap = 0;
    e.Broadcast(0);
    XII_TEST_BOOL(callMap == XII_BIT(0));

    e.RemoveEventHandler(subscriptions[0]);
  }
}
