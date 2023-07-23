#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Time/Clock.h>

static void TimeEventHandler(const xiiClock::EventData& e)
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  xiiTelemetryMessage Msg;
  Msg.SetMessageID('TIME', 'UPDT');
  Msg.GetWriter() << e.m_sClockName;
  Msg.GetWriter() << xiiTime::Now();
  Msg.GetWriter() << e.m_RawTimeStep;
  Msg.GetWriter() << e.m_SmoothedTimeStep;

  xiiTelemetry::Broadcast(xiiTelemetry::Unreliable, Msg);
}

void AddTimeEventHandler()
{
  xiiClock::AddEventHandler(TimeEventHandler);
}

void RemoveTimeEventHandler()
{
  xiiClock::RemoveEventHandler(TimeEventHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Time);
