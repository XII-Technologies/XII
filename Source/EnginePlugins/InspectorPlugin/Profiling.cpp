/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Diagnostics/TelemetryService.h>
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
  struct Consumer
  {
    xiiNetworkChannel            m_Channel;
    xiiUInt64                    m_uiGeneration    = 0;
    xiiUInt64                    m_uiSequence      = 0;
    xiiUInt64                    m_uiStringBytes   = 0;
    xiiUInt64                    m_uiDroppedScopes = 0, m_uiDroppedFrames = 0, m_uiDroppedGpu = 0, m_uiOmittedNames = 0;
    xiiMap<xiiString, xiiUInt32> m_Strings;
    xiiDynamicArray<xiiString>   m_NewStrings;
    xiiMap<xiiUInt64, xiiString> m_Threads;
    xiiMap<xiiUInt64, GraphName> m_Graphs;

    xiiUInt32 Intern(xiiStringView name)
    {
      if (name.GetElementCount() > 512)
      {
        const char* end = name.GetStartPointer() + 512;
        while (end > name.GetStartPointer() && (static_cast<xiiUInt8>(*end) & 0xC0) == 0x80)
          --end;
        name = xiiStringView(name.GetStartPointer(), end);
      }
      auto it = m_Strings.Find(name);
      if (it.IsValid())
        return it.Value();
      if (m_Strings.GetCount() >= 32768 || m_uiStringBytes + name.GetElementCount() + 32 > 4 * 1024 * 1024)
      {
        ++m_uiOmittedNames;
        return 0;
      }
      m_uiStringBytes += name.GetElementCount() + 32;
      const xiiUInt32 id   = m_Strings.GetCount();
      xiiString       text = name;
      m_Strings.Insert(text, id);
      m_NewStrings.PushBack(std::move(text));
      return id;
    }
  };
  xiiDynamicArray<Consumer>      s_Consumers;
  xiiProfilingSystem::StreamData s_Cpu;

  void Demand(bool active)
  {
    s_bSubscribed = active;
    xiiProfilingSystem::SetStreamingEnabled(active);
    if (!active)
    {
      XII_LOCK(s_Mutex);
      s_Gpu.Clear();
      s_Graphs.Clear();
      s_Consumers.Clear();
      s_uiDroppedGpu = 0;
    }
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

  void Collect(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiDynamicArray<xiiTelemetrySubscriber> subscribers;
    service.GetSubscribers(source, subscribers);
    for (xiiUInt32 i = s_Consumers.GetCount(); i-- > 0;)
    {
      bool retained = false;
      for (const auto& subscriber : subscribers)
        retained |= subscriber.m_Channel == s_Consumers[i].m_Channel && subscriber.m_uiGeneration == s_Consumers[i].m_uiGeneration;
      if (!retained)
        s_Consumers.RemoveAtAndCopy(i);
    }
    for (const auto& subscriber : subscribers)
    {
      bool found = false;
      for (const auto& consumer : s_Consumers)
        found |= consumer.m_Channel == subscriber.m_Channel && consumer.m_uiGeneration == subscriber.m_uiGeneration;
      if (!found)
      {
        auto& consumer          = s_Consumers.ExpandAndGetRef();
        consumer.m_Channel      = subscriber.m_Channel;
        consumer.m_uiGeneration = subscriber.m_uiGeneration;
        consumer.Intern("");
      }
    }
    xiiProfilingSystem::DrainStream(s_Cpu);
    xiiSet<xiiUInt64> activeThreads;
    for (const auto& thread : s_Cpu.m_Threads)
      activeThreads.Insert(thread.m_uiThreadId);
    xiiDynamicArray<Sample>      gpu;
    xiiMap<xiiUInt64, GraphName> graphs;
    {
      XII_LOCK(s_Mutex);
      graphs = s_Graphs;
      while (!s_Gpu.IsEmpty())
      {
        gpu.PushBack(s_Gpu.PeekFront());
        s_Gpu.PopFront();
      }
    }
    const auto now = xiiTime::Now();
    for (auto& consumer : s_Consumers)
    {
      for (auto it = consumer.m_Threads.GetIterator(); it.IsValid();)
      {
        if (!activeThreads.Contains(it.Key()))
          it = consumer.m_Threads.Remove(it);
        else
          ++it;
      }
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         body(&storage);
      auto*                         application = xiiApplication::GetApplicationInstance();
      body << consumer.Intern(application ? application->GetApplicationName().GetView() : xiiStringView("XII application"));
#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
      body << static_cast<xiiUInt64>(xiiProcess::GetCurrentProcessID());
#else
      body << xiiUInt64(0);
#endif
      xiiUInt32 threadCount = 0;
      for (const auto& thread : s_Cpu.m_Threads)
        if (!consumer.m_Threads.Contains(thread.m_uiThreadId) || consumer.m_Threads[thread.m_uiThreadId] != thread.m_sName)
          ++threadCount;
      body << threadCount;
      for (const auto& thread : s_Cpu.m_Threads)
      {
        if (consumer.m_Threads.Contains(thread.m_uiThreadId) && consumer.m_Threads[thread.m_uiThreadId] == thread.m_sName)
          continue;
        consumer.m_Threads[thread.m_uiThreadId] = thread.m_sName;
        body << thread.m_uiThreadId << consumer.Intern(thread.m_sName);
      }
      body << s_Cpu.m_Frames.GetCount();
      for (const auto& frame : s_Cpu.m_Frames)
        body << frame.m_uiFrameIndex << frame.m_BeginTime.GetMicroseconds() << frame.m_EndTime.GetMicroseconds();
      body << s_Cpu.m_Scopes.GetCount();
      for (const auto& scope : s_Cpu.m_Scopes)
      {
        const auto name     = consumer.Intern(static_cast<const char*>(scope.m_szName));
        const auto function = consumer.Intern(static_cast<const char*>(scope.m_szFunction));
        body << scope.m_uiSequence << scope.m_uiThreadId << name << function << scope.m_BeginTime.GetMicroseconds() << scope.m_EndTime.GetMicroseconds();
      }

      body << s_Cpu.m_GpuScopes.GetCount();
      for (const auto& sample : s_Cpu.m_GpuScopes)
        body << sample.m_uiGpuIndex << consumer.Intern(static_cast<const char*>(sample.m_Scope.m_szName)) << sample.m_Scope.m_BeginTime.GetMicroseconds() << sample.m_Scope.m_EndTime.GetMicroseconds();
      xiiUInt32 graphCount = 0;
      for (auto it = graphs.GetIterator(); it.IsValid(); ++it)
      {
        const auto old = consumer.m_Graphs.Find(it.Key());
        graphCount += !old.IsValid() || old.Value().m_sName != it.Value().m_sName || old.Value().m_sView != it.Value().m_sView;
      }
      body << graphCount;
      for (auto it = graphs.GetIterator(); it.IsValid(); ++it)
      {
        const auto old = consumer.m_Graphs.Find(it.Key());
        if (old.IsValid() && old.Value().m_sName == it.Value().m_sName && old.Value().m_sView == it.Value().m_sView)
          continue;
        body << it.Key() << consumer.Intern(it.Value().m_sName) << consumer.Intern(it.Value().m_sView);
      }
      consumer.m_Graphs = graphs;
      body << gpu.GetCount();
      for (const auto& sample : gpu)
        body << sample.m_uiGraphId << sample.m_uiFrameIndex << sample.m_uiPassIndex << sample.m_uiQueueIndex
             << sample.m_bSubmission << sample.m_bAvailable << sample.m_fDurationMs << sample.m_RecordedAt.GetMicroseconds() << consumer.Intern(sample.m_sName.GetView());

      xiiContiguousMemoryStreamStorage encoded;
      xiiMemoryStreamWriter            writer(&encoded);
      writer << xiiUInt32(1) << consumer.m_uiGeneration << (consumer.m_uiSequence + 1) << now.GetMicroseconds()
             << (s_Cpu.m_uiDroppedScopes + consumer.m_uiDroppedScopes) << (s_Cpu.m_uiDroppedFrames + consumer.m_uiDroppedFrames)
             << (s_uiDroppedGpu.load() + s_Cpu.m_uiDroppedGpuScopes + consumer.m_uiDroppedGpu);
      writer << consumer.m_NewStrings.GetCount();
      for (const auto& name : consumer.m_NewStrings)
        writer << name;
      storage.CopyToStream(writer).IgnoreResult();
      writer << xiiUInt32(0x50535452);
      xiiTelemetrySample sample;
      sample.m_uiSource               = source;
      sample.m_uiGeneration           = consumer.m_uiGeneration;
      sample.m_uiDropped              = consumer.m_uiOmittedNames;
      sample.m_fTimestampMicroseconds = now.GetMicroseconds();
      sample.m_Data                   = xiiMakeArrayPtr(encoded.GetData(), encoded.GetStorageSize32());
      if (service.Publish(std::move(sample), consumer.m_Channel).Succeeded())
      {
        ++consumer.m_uiSequence;
        consumer.m_NewStrings.Clear();
      }
      else
      {
        consumer.m_uiDroppedScopes += s_Cpu.m_Scopes.GetCount();
        consumer.m_uiDroppedFrames += s_Cpu.m_Frames.GetCount();
        consumer.m_uiDroppedGpu += gpu.GetCount() + s_Cpu.m_GpuScopes.GetCount();
        consumer.m_Threads.Clear();
        consumer.m_Graphs.Clear();
      }
      // Rejected admission retains dictionary additions for the next accepted batch.
    }
  }
} // namespace

void AddProfilingStream()
{
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Profiling;
  info.m_sName                         = "CPU and GPU profiling";
  info.m_sCategory                     = "Profiling";
  info.m_uiMinimumIntervalMilliseconds = 33;
  XII_VERIFY(service.RegisterSource(info, Collect, Demand).Succeeded(), "Duplicate profiling source.");
  xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(GraphExecuted);
  xiiRenderGraphTimestampProfiler::s_TimingEvents.AddEventHandler(Timing);
}

void RemoveProfilingStream()
{
  xiiRenderGraphTimestampProfiler::s_TimingEvents.RemoveEventHandler(Timing);
  xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(GraphExecuted);
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::Profiling);
  Demand(false);
}

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Profiling);
