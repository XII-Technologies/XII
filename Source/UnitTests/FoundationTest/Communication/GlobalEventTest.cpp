#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>

static xiiInt32 iTestData1 = 0;
static xiiInt32 iTestData2 = 0;

// The following event handlers are automatically registered, nothing else needs to be done here

XII_ON_GLOBAL_EVENT(TestGlobalEvent1)
{
  iTestData1 += param0.Get<xiiInt32>();
}

XII_ON_GLOBAL_EVENT(TestGlobalEvent2)
{
  iTestData2 += param0.Get<xiiInt32>();
}

XII_ON_GLOBAL_EVENT_ONCE(TestGlobalEvent3)
{
  // this handler will be executed only once, even if the event is broadcast multiple times
  iTestData2 += 42;
}

static bool g_bFirstRun = true;

XII_CREATE_SIMPLE_TEST(Communication, GlobalEvent)
{
  iTestData1 = 0;
  iTestData2 = 0;

  XII_TEST_INT(iTestData1, 0);
  XII_TEST_INT(iTestData2, 0);

  xiiGlobalEvent::Broadcast("TestGlobalEvent1", 1);

  XII_TEST_INT(iTestData1, 1);
  XII_TEST_INT(iTestData2, 0);

  xiiGlobalEvent::Broadcast("TestGlobalEvent1", 2);

  XII_TEST_INT(iTestData1, 3);
  XII_TEST_INT(iTestData2, 0);

  xiiGlobalEvent::Broadcast("TestGlobalEvent1", 3);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 0);

  xiiGlobalEvent::Broadcast("TestGlobalEvent2", 4);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 4);

  xiiGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  XII_TEST_INT(iTestData1, 6);

  if (g_bFirstRun)
  {
    g_bFirstRun = false;
    XII_TEST_INT(iTestData2, 46);
  }
  else
  {
    XII_TEST_INT(iTestData2, 4);
    iTestData2 += 42;
  }

  xiiGlobalEvent::Broadcast("TestGlobalEvent2", 5);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 51);

  xiiGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 51);

  xiiGlobalEvent::Broadcast("TestGlobalEvent2", 6);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 57);

  xiiGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  XII_TEST_INT(iTestData1, 6);
  XII_TEST_INT(iTestData2, 57);

  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);

  xiiGlobalEvent::PrintGlobalEventStatistics();

  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
}
