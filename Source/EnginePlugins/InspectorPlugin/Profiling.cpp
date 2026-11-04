/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <atomic>

namespace
{
  using Sample = xiiRenderGraphTimestampProfiler::TimingSample;
  struct GraphName
  {
    xiiString m_sName, m_sView;
    bool      m_bDirty = true;
  };
  xiiMutex                          s_Mutex;
  std::atomic<bool>                 s_bSubscribed{false};
  std::atomic<xiiUInt64>            s_uiDroppedGpu{0};
  xiiStaticRingBuffer<Sample, 4096> s_Gpu;
  xiiMap<xiiUInt64, GraphName>      s_Graphs;
  xiiMap<xiiString, xiiUInt32>      s_Strings;
  xiiDynamicArray<xiiString>        s_NewStrings;
  xiiMap<xiiUInt64, xiiString>      s_Threads;
  xiiProfilingSystem::StreamData    s_Cpu;
  xiiTime                           s_LastSend, s_LastConsumer;
  xiiUInt64                         s_uiSequence = 0, s_uiAcknowledged = 0;
  xiiUInt64                         s_uiSession = 0;

  void Stop()
  {
    s_bSubscribed = false;
    xiiProfilingSystem::SetStreamingEnabled(false);
    XII_LOCK(s_Mutex);
    s_Gpu.Clear();
    s_Graphs.Clear();
    s_Strings.Clear();
    s_Threads.Clear();
    s_NewStrings.Clear();
    s_uiDroppedGpu = 0;
  }

  xiiUInt32 Intern(xiiStringView name)
  {
    // Debug names are bounded independently of sample count. Keep UTF-8 character boundaries.
    if (name.GetElementCount() > 512)
      name = name.GetSubString(0, 512);
    auto it = s_Strings.Find(name);
    if (it.IsValid())
      return it.Value();
    // A new subscription resets the dictionary. The reserved empty entry makes overflow explicit.
    if (s_Strings.GetCount() >= 32768)
      return 0;
    const xiiUInt32 id   = s_Strings.GetCount();
    xiiString       text = name;
    s_Strings.Insert(text, id);
    s_NewStrings.PushBack(std::move(text));
    return id;
  }

  void Timing(const Sample& sample)
  {
    if (!s_bSubscribed.load(std::memory_order_relaxed))
      return;
    if (s_Mutex.TryLock().Failed())
    {
      ++s_uiDroppedGpu;
      return;
    }
    XII_SCOPE_EXIT(s_Mutex.Unlock());
    if (!s_Gpu.CanAppend())
    {
      ++s_uiDroppedGpu;
      return;
    }
    s_Gpu.PushBack(sample);
  }

  void GraphExecuted(const xiiRenderGraphDebug::ExecutionEvent& event)
  {
    if (!s_bSubscribed.load(std::memory_order_relaxed))
      return;
    if (s_Mutex.TryLock().Failed())
      return; // Retry metadata on the next execution; never stall a rendering thread.
    XII_SCOPE_EXIT(s_Mutex.Unlock());
    const auto id = event.m_Graph.GetInstanceId();
    if (!s_Graphs.Contains(id) && s_Graphs.GetCount() >= 64)
      s_Graphs.Remove(s_Graphs.GetIterator());
    auto& name = s_Graphs[id];
    if (name.m_sName != event.m_Graph.GetName() || name.m_sView != event.m_sViewName)
    {
      name.m_sName  = event.m_Graph.GetName();
      name.m_sView  = event.m_sViewName;
      name.m_bDirty = true;
    }
  }

  void Request(void*)
  {
    xiiTelemetryMessage message;
    while (xiiTelemetry::RetrieveMessage('PSTR', message).Succeeded())
    {
      if (message.GetMessageID() == 'SUBS')
      {
        xiiUInt64 session = 0;
        message.GetReader() >> session;
        if (session == 0)
          continue;
        if (!s_bSubscribed || session != s_uiSession)
        {
          Stop();
          s_uiSession  = session;
          s_uiSequence = s_uiAcknowledged = 0;
          s_LastSend                      = {};
          Intern("");
          xiiProfilingSystem::SetStreamingEnabled(true);
          s_bSubscribed = true;
        }
        s_LastConsumer = xiiTime::Now();
      }
      else if (message.GetMessageID() == 'ACKN')
      {
        xiiUInt64 session = 0, sequence = 0;
        message.GetReader() >> session >> sequence;
        if (session == s_uiSession && sequence == s_uiSequence)
        {
          s_uiAcknowledged = sequence;
          s_LastConsumer   = xiiTime::Now();
        }
      }
      else if (message.GetMessageID() == 'STOP')
        Stop();
    }
  }

  void Update(const xiiTelemetry::TelemetryEventData& event)
  {
    if (event.m_EventType == xiiTelemetry::TelemetryEventData::DisconnectedFromClient)
    {
      Stop();
      return;
    }
    if (event.m_EventType != xiiTelemetry::TelemetryEventData::PerFrameUpdate || !s_bSubscribed)
      return;
    const auto now = xiiTime::Now();
    if (now - s_LastConsumer > xiiTime::MakeFromSeconds(3))
    {
      Stop();
      return;
    }
    // One reliable batch in flight bounds ENet's backlog, independently of engine frame rate.
    if (s_uiSequence != s_uiAcknowledged || now - s_LastSend < xiiTime::MakeFromMilliseconds(33))
      return;
    s_LastSend = now;
    xiiProfilingSystem::DrainStream(s_Cpu);

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         body(&storage);
    auto*                         application = xiiApplication::GetApplicationInstance();
    body << Intern(application ? application->GetApplicationName().GetView() : xiiStringView("XII application"));
#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
    body << static_cast<xiiUInt64>(xiiProcess::GetCurrentProcessID());
#else
    body << xiiUInt64(0);
#endif
    xiiUInt32 threadCount = 0;
    for (const auto& thread : s_Cpu.m_Threads)
      if (!s_Threads.Contains(thread.m_uiThreadId) || s_Threads[thread.m_uiThreadId] != thread.m_sName)
        ++threadCount;
    body << threadCount;
    for (const auto& thread : s_Cpu.m_Threads)
    {
      if (s_Threads.Contains(thread.m_uiThreadId) && s_Threads[thread.m_uiThreadId] == thread.m_sName)
        continue;
      s_Threads[thread.m_uiThreadId] = thread.m_sName;
      body << thread.m_uiThreadId << Intern(thread.m_sName);
    }
    body << s_Cpu.m_Frames.GetCount();
    for (const auto& frame : s_Cpu.m_Frames)
      body << frame.m_uiFrameIndex << frame.m_BeginTime.GetMicroseconds() << frame.m_EndTime.GetMicroseconds();
    body << s_Cpu.m_Scopes.GetCount();
    for (const auto& scope : s_Cpu.m_Scopes)
    {
      const auto name     = Intern(static_cast<const char*>(scope.m_szName));
      const auto function = Intern(static_cast<const char*>(scope.m_szFunction));
      body << scope.m_uiSequence << scope.m_uiThreadId << name << function << scope.m_BeginTime.GetMicroseconds() << scope.m_EndTime.GetMicroseconds();
    }

    body << s_Cpu.m_GpuScopes.GetCount();
    for (const auto& sample : s_Cpu.m_GpuScopes)
      body << sample.m_uiGpuIndex << Intern(static_cast<const char*>(sample.m_Scope.m_szName)) << sample.m_Scope.m_BeginTime.GetMicroseconds() << sample.m_Scope.m_EndTime.GetMicroseconds();
    {
      XII_LOCK(s_Mutex);
      xiiUInt32 graphCount = 0;
      for (auto it = s_Graphs.GetIterator(); it.IsValid(); ++it)
        graphCount += it.Value().m_bDirty;
      body << graphCount;
      for (auto it = s_Graphs.GetIterator(); it.IsValid(); ++it)
      {
        auto& name = it.Value();
        if (!name.m_bDirty)
          continue;
        body << it.Key() << Intern(name.m_sName) << Intern(name.m_sView);
        name.m_bDirty = false;
      }
      body << s_Gpu.GetCount();
      while (!s_Gpu.IsEmpty())
      {
        const auto& sample = s_Gpu.PeekFront();
        body << sample.m_uiGraphId << sample.m_uiFrameIndex << sample.m_uiPassIndex << sample.m_uiQueueIndex
             << sample.m_bSubmission << sample.m_bAvailable << sample.m_fDurationMs << sample.m_RecordedAt.GetMicroseconds() << Intern(sample.m_sName.GetView());
        s_Gpu.PopFront();
      }
    }
    xiiTelemetryMessage batch;
    batch.SetMessageID('PSTR', 'BTCH');
    auto& writer = batch.GetWriter();
    writer << xiiUInt32(1) << s_uiSession << ++s_uiSequence << now.GetMicroseconds()
           << s_Cpu.m_uiDroppedScopes << s_Cpu.m_uiDroppedFrames << (s_uiDroppedGpu.load() + s_Cpu.m_uiDroppedGpuScopes);
    writer << s_NewStrings.GetCount();
    for (const auto& name : s_NewStrings)
      writer << name;
    s_NewStrings.Clear();
    storage.CopyToStream(writer).IgnoreResult();
    writer << xiiUInt32(0x50535452);
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, batch);
  }
} // namespace

void AddProfilingStream()
{
  xiiTelemetry::SetOutgoingQueueSize('PSTR', 0);
  xiiTelemetry::AcceptMessagesForSystem('PSTR', true, Request);
  xiiTelemetry::AddEventHandler(Update);
  xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(GraphExecuted);
  xiiRenderGraphTimestampProfiler::s_TimingEvents.AddEventHandler(Timing);
}

void RemoveProfilingStream()
{
  xiiRenderGraphTimestampProfiler::s_TimingEvents.RemoveEventHandler(Timing);
  xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(GraphExecuted);
  xiiTelemetry::RemoveEventHandler(Update);
  xiiTelemetry::AcceptMessagesForSystem('PSTR', false);
  Stop();
}

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Profiling);
