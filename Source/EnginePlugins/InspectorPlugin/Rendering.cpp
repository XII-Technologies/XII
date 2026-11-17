/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>

namespace
{
  constexpr xiiUInt32 MaximumCaptureBytes = 16 * 1024 * 1024;
  struct Request
  {
    xiiTelemetrySubscriber m_Subscriber;
    xiiTime                m_Expires;
    xiiHashSet<xiiUInt64>  m_Graphs;
  };
  xiiMutex                 s_RequestMutex;
  xiiDynamicArray<Request> s_Requests;
  xiiTelemetryService*     s_pService = nullptr; // Borrowed until the execution handler is removed.

  class CaptureStream : public xiiStreamWriter
  {
  public:
    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamWriter            m_Writer{&m_Storage};
    xiiResult                        WriteBytes(const void* pData, xiiUInt64 uiSize) override
    {
      if (uiSize > MaximumCaptureBytes - m_Storage.GetStorageSize32())
        return XII_FAILURE;
      return m_Writer.WriteBytes(pData, uiSize);
    }
  };

  bool PublishCapture(xiiTelemetryService& service, xiiUInt64 source, CaptureStream& stream, bool succeeded, xiiArrayPtr<const xiiTelemetrySubscriber> recipients = {})
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = source;
    sample.m_uiKind                 = succeeded ? 0 : 1;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    if (succeeded)
      sample.m_Data = xiiMakeArrayPtr(stream.m_Storage.GetData(), stream.m_Storage.GetStorageSize32());
    else
    {
      const xiiStringView error = "Capture exceeded the 16 MiB limit or serialization failed.";
      sample.m_Data             = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(error.GetStartPointer()), error.GetElementCount());
    }
    return service.Publish(std::move(sample), recipients).Succeeded();
  }

  void GraphExecuted(const xiiRenderGraphDebug::ExecutionEvent& event)
  {
    if (s_RequestMutex.TryLock().Failed())
      return;
    xiiDynamicArray<xiiTelemetrySubscriber> recipients;
    for (auto& request : s_Requests)
    {
      if (xiiTime::Now() >= request.m_Expires || request.m_Graphs.GetCount() >= 64 || request.m_Graphs.Contains(event.m_Graph.GetInstanceId()))
        continue;
      request.m_Graphs.Insert(event.m_Graph.GetInstanceId());
      recipients.PushBack(request.m_Subscriber);
    }
    s_RequestMutex.Unlock();
    if (recipients.IsEmpty())
      return;
    CaptureStream stream;
    const bool    succeeded = xiiRenderGraphDebug::WriteSnapshot(event, stream).Succeeded();
    if (!PublishCapture(*s_pService, xiiTelemetrySources::RenderGraphs, stream, succeeded, recipients))
    {
      // Retry admission on another execution while this request's collection window is open.
      XII_LOCK(s_RequestMutex);
      for (auto& request : s_Requests)
        for (const auto& recipient : recipients)
          if (request.m_Subscriber.m_Channel == recipient.m_Channel && request.m_Subscriber.m_uiGeneration == recipient.m_uiGeneration)
            request.m_Graphs.Remove(event.m_Graph.GetInstanceId());
    }
  }

  void CollectGraphs(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiDynamicArray<xiiTelemetrySubscriber> subscribers;
    service.GetSubscribers(source, subscribers);
    XII_LOCK(s_RequestMutex);
    for (xiiUInt32 i = s_Requests.GetCount(); i-- > 0;)
    {
      bool retained = false;
      for (const auto& subscriber : subscribers)
        retained |= subscriber.m_Channel == s_Requests[i].m_Subscriber.m_Channel && subscriber.m_uiGeneration == s_Requests[i].m_Subscriber.m_uiGeneration;
      if (!retained)
        s_Requests.RemoveAtAndCopy(i);
    }
    for (const auto& subscriber : subscribers)
    {
      bool found = false;
      for (const auto& request : s_Requests)
        found |= request.m_Subscriber.m_Channel == subscriber.m_Channel && request.m_Subscriber.m_uiGeneration == subscriber.m_uiGeneration;
      if (!found)
      {
        auto& request        = s_Requests.ExpandAndGetRef();
        request.m_Subscriber = subscriber;
        request.m_Expires    = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
      }
    }
  }

  void CollectHistory(xiiTelemetryService& service, xiiUInt64 source)
  {
    // Capture is non-destructive and also supports the explicit offline collection API.
    xiiProfilingSystem::ProfilingData capture;
    xiiProfilingSystem::Capture(capture);
    CaptureStream stream;
    (void)PublishCapture(service, source, stream, capture.Write(stream).Succeeded());
  }
} // namespace

void AddRenderingEventHandler()
{
  auto& service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  s_pService    = &service;
  xiiTelemetrySourceInfo info;
  info.m_uiId                           = xiiTelemetrySources::RenderGraphs;
  info.m_sName                          = "Render graph capture";
  info.m_sCategory                      = "Rendering";
  info.m_uiMinimumIntervalMilliseconds  = 100;
  info.m_uiPermission                   = static_cast<xiiUInt8>(xiiTelemetryPermission::Capture);
  info.m_uiCollectionWindowMilliseconds = 2000;
  XII_VERIFY(service.RegisterSource(info, CollectGraphs, [](bool active) {
                      if (!active)
                      {
                        XII_LOCK(s_RequestMutex);
                        s_Requests.Clear();
                      }
                    })
               .Succeeded(),
             "Duplicate render graph source.");
  info.m_uiId                           = xiiTelemetrySources::ProfileHistory;
  info.m_uiCollectionWindowMilliseconds = 0;
  info.m_sName                          = "Profiling history snapshot";
  info.m_bSnapshot                      = true;
  info.m_uiMinimumIntervalMilliseconds  = 1000;
  XII_VERIFY(service.RegisterSource(info, CollectHistory).Succeeded(), "Duplicate profiling history source.");
  xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(GraphExecuted);
}

void RemoveRenderingEventHandler()
{
  xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(GraphExecuted);
  s_pService->UnregisterSource(xiiTelemetrySources::RenderGraphs);
  s_pService->UnregisterSource(xiiTelemetrySources::ProfileHistory);
  s_pService = nullptr;
  XII_LOCK(s_RequestMutex);
  s_Requests.Clear();
}

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Rendering);
