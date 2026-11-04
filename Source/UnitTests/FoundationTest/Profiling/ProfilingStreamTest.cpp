/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>

XII_CREATE_SIMPLE_TEST(Profiling, IncrementalStream)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingSystem::SetStreamingEnabled(true);
  XII_SCOPE_EXIT(xiiProfilingSystem::SetStreamingEnabled(false));
  xiiProfilingSystem::StreamData stream;
  xiiProfilingSystem::StartNewFrame();
  const auto now = xiiTime::Now();
  xiiProfilingSystem::AddCPUScope("Incremental outer", "IncrementalStream", now - xiiTime::MakeFromMilliseconds(2), now, {});
  xiiProfilingSystem::AddCPUScope("Incremental inner", "IncrementalStream", now - xiiTime::MakeFromMilliseconds(1), now, {});
  xiiProfilingSystem::InitializeGPUData();
  xiiProfilingSystem::AddGPUScope("Timestamp scope", now - xiiTime::MakeFromMilliseconds(1), now);
  xiiProfilingSystem::StartNewFrame();
  xiiProfilingSystem::DrainStream(stream);
  XII_TEST_INT(stream.m_Scopes.GetCount(), 2);
  XII_TEST_INT(stream.m_GpuScopes.GetCount(), 1);
  XII_TEST_STRING(stream.m_Scopes[0].m_szName, "Incremental outer");
  XII_TEST_STRING(stream.m_Scopes[0].m_szFunction, "IncrementalStream");
  XII_TEST_BOOL(stream.m_Scopes[1].m_uiSequence > stream.m_Scopes[0].m_uiSequence);
  XII_TEST_INT(stream.m_Frames.GetCount(), 1);
  XII_TEST_INT(stream.m_Frames[0].m_uiFrameIndex, xiiProfilingSystem::GetFrameCount() - 1);
  XII_TEST_BOOL(stream.m_Frames[0].m_EndTime >= stream.m_Frames[0].m_BeginTime);
  XII_TEST_BOOL(stream.m_Scopes[0].m_BeginTime == now - xiiTime::MakeFromMilliseconds(2));
  xiiProfilingSystem::DrainStream(stream);
  XII_TEST_BOOL(stream.m_Scopes.IsEmpty() && stream.m_Frames.IsEmpty());
  for (int i = 0; i < 9000; ++i)
    xiiProfilingSystem::AddCPUScope("Bounded stream", "IncrementalStream", now - xiiTime::MakeFromMilliseconds(1), now, {});
  for (int i = 0; i < 300; ++i)
    xiiProfilingSystem::StartNewFrame();
  xiiProfilingSystem::DrainStream(stream);
  XII_TEST_INT(stream.m_Scopes.GetCount(), 8192);
  XII_TEST_INT(stream.m_uiDroppedScopes, 808);
  XII_TEST_INT(stream.m_Frames.GetCount(), 256);
  XII_TEST_INT(stream.m_uiDroppedFrames, 44);
  xiiProfilingSystem::ProfilingData ordinary;
  xiiProfilingSystem::Capture(ordinary);
  XII_TEST_BOOL(!ordinary.m_AllEventBuffers.IsEmpty()); // Streaming never consumes ordinary captures.
  xiiProfilingSystem::SetStreamingEnabled(false);
  xiiProfilingSystem::AddCPUScope("Disabled stream", "IncrementalStream", now - xiiTime::MakeFromMilliseconds(1), now, {});
  xiiProfilingSystem::DrainStream(stream);
  XII_TEST_BOOL(stream.m_Scopes.IsEmpty() && stream.m_Frames.IsEmpty());
#endif
}

XII_CREATE_SIMPLE_TEST(Profiling, IncrementalWorkerStream)
{
#if XII_ENABLED(XII_USE_PROFILING)
  class Worker final : public xiiThread
  {
  public:
    Worker() : xiiThread("Profiling stream worker") {}

  private:
    xiiUInt32 Run() override
    {
      const auto end = xiiTime::Now();
      xiiProfilingSystem::AddCPUScope("Worker operation", "WorkerStream", end - xiiTime::MakeFromMilliseconds(1), end, {});
      return 0;
    }
  };
  xiiProfilingSystem::SetStreamingEnabled(true);
  XII_SCOPE_EXIT(xiiProfilingSystem::SetStreamingEnabled(false));
  Worker worker;
  worker.Start();
  worker.Join();
  xiiProfilingSystem::StreamData stream;
  xiiProfilingSystem::DrainStream(stream);
  bool      found = false, named = false;
  xiiUInt64 workerId = 0;
  for (const auto& scope : stream.m_Scopes)
    if (xiiStringUtils::IsEqual(scope.m_szName, "Worker operation"))
    {
      workerId = scope.m_uiThreadId;
      XII_TEST_BOOL(workerId != static_cast<xiiUInt64>(xiiThreadUtils::GetCurrentThreadID()));
      XII_TEST_BOOL(scope.m_EndTime > scope.m_BeginTime);
      XII_TEST_STRING(scope.m_szFunction, "WorkerStream");
      found = true;
    }
  for (const auto& thread : stream.m_Threads)
    if (thread.m_uiThreadId == workerId && thread.m_sName == "Profiling stream worker")
      named = true;
  XII_TEST_BOOL(found && named);
#endif
}
