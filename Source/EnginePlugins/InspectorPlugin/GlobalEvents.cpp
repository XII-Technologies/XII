#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>

#include <Core/GameApplication/GameApplicationBase.h>

static xiiGlobalEvent::EventMap s_LastState;

static void SendGlobalEventTelemetry(xiiStringView sEvent, const xiiGlobalEvent::EventData& ed)
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  xiiTelemetryMessage msg;
  msg.SetMessageID('EVNT', 'DATA');
  msg.GetWriter() << sEvent;
  msg.GetWriter() << ed.m_uiNumTimesFired;
  msg.GetWriter() << ed.m_uiNumEventHandlersRegular;
  msg.GetWriter() << ed.m_uiNumEventHandlersOnce;

  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
}

static void SendAllGlobalEventTelemetry()
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    xiiTelemetryMessage msg;
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'EVNT', ' CLR', nullptr, 0);
  }

  xiiGlobalEvent::UpdateGlobalEventStatistics();

  s_LastState = xiiGlobalEvent::GetEventStatistics();

  for (xiiGlobalEvent::EventMap::ConstIterator it = s_LastState.GetIterator(); it.IsValid(); ++it)
  {
    SendGlobalEventTelemetry(it.Key(), it.Value());
  }
}

static void SendChangedGlobalEventTelemetry()
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  static xiiTime LastUpdate = xiiTime::Now();

  if ((xiiTime::Now() - LastUpdate).GetSeconds() < 0.5)
    return;

  LastUpdate = xiiTime::Now();

  xiiGlobalEvent::UpdateGlobalEventStatistics();

  const xiiGlobalEvent::EventMap& data = xiiGlobalEvent::GetEventStatistics();

  if (data.GetCount() != s_LastState.GetCount())
  {
    SendAllGlobalEventTelemetry();
    return;
  }

  for (xiiGlobalEvent::EventMap::ConstIterator it = data.GetIterator(); it.IsValid(); ++it)
  {
    const xiiGlobalEvent::EventData& currentEventData = it.Value();
    xiiGlobalEvent::EventData&       lastEventData    = s_LastState[it.Key()];

    if (xiiMemoryUtils::Compare(&currentEventData, &lastEventData) != 0)
    {
      SendGlobalEventTelemetry(it.Key().GetData(), it.Value());

      lastEventData = currentEventData;
    }
  }
}

namespace GlobalEventsDetail
{
  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }

  static void PerframeUpdateHandler(const xiiGameApplicationExecutionEvent& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case xiiGameApplicationExecutionEvent::Type::AfterPresent:
        SendChangedGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace GlobalEventsDetail

void AddGlobalEventHandler()
{
  xiiTelemetry::AddEventHandler(GlobalEventsDetail::TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using xiiTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the xiiStats and xiiTelemetry system.
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }
}

void RemoveGlobalEventHandler()
{
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }

  xiiTelemetry::RemoveEventHandler(GlobalEventsDetail::TelemetryEventsHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_GlobalEvents);
