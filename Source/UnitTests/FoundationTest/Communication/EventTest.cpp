/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Events/Event.h>

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subscription invalidation")
  {
    using TestEvent = xiiEvent<xiiInt32*>;
    Test                    test;
    TestEvent::Unsubscriber subscription;
    {
      TestEvent                event;
      const TestEvent::Handler handler(&Test::DoStuff, &test);

      event.AddEventHandler(handler, subscription);
      TestEvent::Unsubscriber moved(std::move(subscription));
      XII_TEST_BOOL(!subscription.IsSubscribed());
      XII_TEST_BOOL(moved.IsSubscribed());

      event.RemoveEventHandler(handler);
      XII_TEST_BOOL(!moved.IsSubscribed());

      event.AddEventHandler(handler, moved);
      subscription = std::move(moved);
      XII_TEST_BOOL(!moved.IsSubscribed());
      XII_TEST_BOOL(subscription.IsSubscribed());

      event.Clear();
      XII_TEST_BOOL(!subscription.IsSubscribed());
      event.AddEventHandler(handler, subscription);
    }

    XII_TEST_BOOL(!subscription.IsSubscribed());
    subscription.Unsubscribe();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reentrant default dispatch")
  {
    using TestEvent = xiiEvent<xiiUInt32, xiiMutex>;
    struct DispatchState
    {
      TestEvent               m_Event;
      TestEvent::Unsubscriber m_Subscription;
      xiiUInt32               m_uiOriginalCalls = 0;
      xiiUInt32               m_uiNewCalls      = 0;
    };

    DispatchState state;
    state.m_Event.AddEventHandler(
      [pState = &state](xiiUInt32 uiValue) {
        pState->m_Subscription.Unsubscribe();
        for (xiiUInt32 uiIndex = 0; uiIndex < 128; ++uiIndex)
        {
          pState->m_Event.AddEventHandler([pState](xiiUInt32 uiNestedValue) { pState->m_uiNewCalls += uiNestedValue; });
        }

        pState->m_uiOriginalCalls += uiValue;
        pState->m_Event.Broadcast(2, 1);
        pState->m_Event.Clear();
        XII_TEST_BOOL(pState->m_Event.IsEmpty());
        pState->m_uiOriginalCalls += uiValue;
      },
      state.m_Subscription);
    state.m_Event.AddEventHandler([](xiiUInt32 uiValue) { XII_TEST_INT(uiValue, 2); });

    state.m_Event.Broadcast(1);
    XII_TEST_BOOL(!state.m_Subscription.IsSubscribed());
    XII_TEST_INT(state.m_uiOriginalCalls, 2);
    XII_TEST_INT(state.m_uiNewCalls, 256);
    XII_TEST_BOOL(state.m_Event.IsEmpty());
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
