/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
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
