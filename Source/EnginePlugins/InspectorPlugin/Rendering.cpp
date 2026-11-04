/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <atomic>

namespace
{
  // Capture work is opt-in. Bound both each reliable packet and the number of graphs per request.
  constexpr xiiUInt32   s_uiMaxCaptureBytes = 16 * 1024 * 1024;
  xiiAtomicIntegerU64   s_uiRequest         = 0;
  xiiAtomicInteger64    s_iExpires          = 0;
  xiiUInt64             s_uiHandledRequest  = 0;
  xiiHashSet<xiiUInt64> s_CapturedGraphs; // Used only inside the serialized execution event.

  class CaptureStream : public xiiStreamWriter
  {
  public:
    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamWriter            m_Writer{&m_Storage};
    xiiResult                        WriteBytes(const void* data, xiiUInt64 size) override
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
    const xiiUInt32     size  = succeeded ? stream.m_Storage.GetStorageSize32() : error.GetElementCount();
    message.GetWriter() << size;
    message.GetWriter().WriteBytes(succeeded ? stream.m_Storage.GetData() : reinterpret_cast<const xiiUInt8*>(error.GetStartPointer()), size).IgnoreResult();
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, message);
  }

  void GraphExecuted(const xiiRenderGraphDebug::ExecutionEvent& event)
  {
    if (static_cast<xiiInt64>(xiiTime::Now().GetMicroseconds()) > s_iExpires)
      return;
    const auto request = s_uiRequest;
    if (request != s_uiHandledRequest)
    {
      s_uiHandledRequest = request;
      s_CapturedGraphs.Clear();
    }
    if (request == 0 || s_CapturedGraphs.GetCount() >= 64 || s_CapturedGraphs.Contains(event.m_Graph.GetInstanceId()))
      return;
    s_CapturedGraphs.Insert(event.m_Graph.GetInstanceId());
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
      s_uiRequest.PostIncrement();
      s_iExpires.Set(static_cast<xiiInt64>((xiiTime::Now() + xiiTime::MakeFromSeconds(2)).GetMicroseconds()));
    }
  }

  void ConnectionEvent(const xiiTelemetry::TelemetryEventData& event)
  {
    if (event.m_EventType == xiiTelemetry::TelemetryEventData::DisconnectedFromClient)
    {
      s_iExpires.Set(0);
    }
  }
} // namespace

void AddRenderingEventHandler()
{
  xiiTelemetry::SetOutgoingQueueSize('RGRF', 0);
  xiiTelemetry::AcceptMessagesForSystem('RGRF', true, RequestGraph);
  xiiTelemetry::AddEventHandler(ConnectionEvent);

  xiiRenderGraphDebug::s_ExecutionEvents.AddEventHandler(GraphExecuted);
}

void RemoveRenderingEventHandler()
{
  xiiTelemetry::AcceptMessagesForSystem('RGRF', false);
  xiiTelemetry::RemoveEventHandler(ConnectionEvent);

  xiiRenderGraphDebug::s_ExecutionEvents.RemoveEventHandler(GraphExecuted);

  s_iExpires.Set(0);

  s_CapturedGraphs.Clear();
}

XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Rendering);
