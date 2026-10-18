/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <atomic>

namespace
{
  // Capture work is opt-in. Bound both each reliable packet and the number of graphs per request.
  constexpr xiiUInt32 s_uiMaxCaptureBytes = 16 * 1024 * 1024;
  std::atomic<xiiUInt64> s_uiRequest = 0;
  std::atomic<xiiInt64> s_iExpires = 0;
  xiiUInt64 s_uiHandledRequest = 0;
  xiiHashSet<xiiUInt64> s_CapturedGraphs; // Used only inside the serialized execution event.

  class CaptureStream : public xiiStreamWriter
  {
  public:
    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamWriter m_Writer{&m_Storage};
    xiiResult WriteBytes(const void* data, xiiUInt64 size) override
    {
      if (size > s_uiMaxCaptureBytes - m_Storage.GetStorageSize32())
        return XII_FAILURE;
      return m_Writer.WriteBytes(data, size);
    }
  };

  void SendCapture(xiiUInt32 system, CaptureStream& stream, bool succeeded)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;
    xiiTelemetryMessage message;
    message.SetMessageID(system, succeeded ? 'DATA' : 'ERRO');
    const xiiStringView error = "Capture exceeded the 16 MiB limit or serialization failed.";
    const xiiUInt32 size = succeeded ? stream.m_Storage.GetStorageSize32() : error.GetElementCount();
    message.GetWriter() << size;
    message.GetWriter().WriteBytes(succeeded ? stream.m_Storage.GetData() : reinterpret_cast<const xiiUInt8*>(error.GetStartPointer()), size).IgnoreResult();
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, message);
  }

  void GraphExecuted(const xiiRenderGraphDebug::ExecutionEvent& event)
  {
    if (static_cast<xiiInt64>(xiiTime::Now().GetMicroseconds()) > s_iExpires.load())
      return;
    const auto request = s_uiRequest.load();
    if (request != s_uiHandledRequest)
    {
      s_uiHandledRequest = request;
      s_CapturedGraphs.Clear();
    }
    if (request == 0 || s_CapturedGraphs.GetCount() >= 64 || s_CapturedGraphs.Contains(event.m_Graph.GetId().m_uiValue))
      return;
    s_CapturedGraphs.Insert(event.m_Graph.GetId().m_uiValue);
    CaptureStream stream;
    SendCapture('RGRF', stream, xiiRenderGraphDebug::WriteSnapshot(event, stream).Succeeded());
  }

  void RequestGraph(void*)
  {
    xiiTelemetryMessage message;
    while (xiiTelemetry::RetrieveMessage('RGRF', message).Succeeded())
    {
      if (message.GetMessageID() != 'CAPT')
        continue;
      s_uiRequest.fetch_add(1);
      s_iExpires.store(static_cast<xiiInt64>((xiiTime::Now() + xiiTime::MakeFromSeconds(2)).GetMicroseconds()));
    }
  }

  void RequestProfile(void*)
  {
    xiiTelemetryMessage message;
    bool requested = false;
    while (xiiTelemetry::RetrieveMessage('PROF', message).Succeeded())
      requested |= message.GetMessageID() == 'CAPT';
    if (!requested || !xiiTelemetry::IsConnectedToClient())
      return;

    xiiProfilingSystem::ProfilingData capture;
    xiiProfilingSystem::Capture(capture); // Preserve the application's own profiling history.
    const xiiTime cutoff = xiiTime::Now() - xiiTime::MakeFromSeconds(1);
    for (auto& buffer : capture.m_AllEventBuffers)
    {
      for (xiiUInt32 i = buffer.m_Data.GetCount(); i-- > 0;)
        if (buffer.m_Data[i].m_EndTime < cutoff)
          buffer.m_Data.RemoveAtAndSwap(i);
    }
    for (auto& buffer : capture.m_GPUScopes)
    {
      for (xiiUInt32 i = buffer.GetCount(); i-- > 0;)
        if (buffer[i].m_EndTime < cutoff)
          buffer.RemoveAtAndSwap(i);
    }
    // Keep frame numbering intact: only remove the old prefix.
    xiiUInt32 first = 0;
    while (first + 1 < capture.m_FrameStartTimes.GetCount() && capture.m_FrameStartTimes[first + 1] < cutoff)
      ++first;
    capture.m_FrameStartTimes.RemoveAtAndCopy(0, first);
    CaptureStream stream;
    SendCapture('PROF', stream, capture.Write(stream).Succeeded());
  }

  void ConnectionEvent(const xiiTelemetry::TelemetryEventData& event)
  {
    if (event.m_EventType == xiiTelemetry::TelemetryEventData::DisconnectedFromClient)
      s_iExpires.store(0);
  }
}

void AddRenderingEventHandler()
{
  xiiTelemetry::SetOutgoingQueueSize('RGRF', 0);
  xiiTelemetry::SetOutgoingQueueSize('PROF', 0);
  xiiTelemetry::AcceptMessagesForSystem('RGRF', true, RequestGraph);
  xiiTelemetry::AcceptMessagesForSystem('PROF', true, RequestProfile);
  xiiTelemetry::AddEventHandler(ConnectionEvent);
  xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(GraphExecuted);
}

void RemoveRenderingEventHandler()
{
  xiiTelemetry::AcceptMessagesForSystem('RGRF', false);
  xiiTelemetry::AcceptMessagesForSystem('PROF', false);
  xiiTelemetry::RemoveEventHandler(ConnectionEvent);
  xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(GraphExecuted);
  s_iExpires.store(0);
  s_CapturedGraphs.Clear();
}

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Rendering);
