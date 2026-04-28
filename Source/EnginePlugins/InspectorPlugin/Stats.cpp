/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>

static void StatsEventHandler(const xiiStats::StatsEventData& e)
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  xiiTelemetry::TransmitMode Mode = xiiTelemetry::Reliable;

  switch (e.m_EventType)
  {
    case xiiStats::StatsEventData::Set:
      Mode = xiiTelemetry::Unreliable;
      // fall-through
    case xiiStats::StatsEventData::Add:
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID('STAT', ' SET');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << e.m_NewStatValue;
      msg.GetWriter() << xiiTime::Now();

      xiiTelemetry::Broadcast(Mode, msg);
    }
    break;
    case xiiStats::StatsEventData::Remove:
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID('STAT', ' DEL');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << xiiTime::Now();

      xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
    }
    break;
  }
}


static void SendAllStatsTelemetry()
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  for (xiiStats::MapType::ConstIterator it = xiiStats::GetAllStats().GetIterator(); it.IsValid(); ++it)
  {
    xiiTelemetryMessage msg;
    msg.SetMessageID('STAT', ' SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value();

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }
}

static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
    case xiiTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllStatsTelemetry();
      break;

    default:
      break;
  }
}

static void PerFrameUpdateHandler(const xiiGameApplicationExecutionEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameApplicationExecutionEvent::Type::AfterPresent:
    {
      xiiTime FrameTime;

      if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
      {
        FrameTime = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetFrameTime();
      }

      xiiStringBuilder s;
      xiiStats::SetStat("App/FrameTime[ms]", FrameTime.GetMilliseconds());
      xiiStats::SetStat("App/FPS", 1.0 / FrameTime.GetSeconds());

      xiiStats::SetStat("App/Active Threads", xiiOSThread::GetThreadCount());

      // Tasksystem Thread Utilization
      {
        for (xiiUInt32 t = 0; t < xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks); ++t)
        {
          xiiUInt32    uiNumTasks  = 0;
          const double Utilization = xiiTaskSystem::GetThreadUtilization(xiiWorkerThreadType::ShortTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Short{0}_Load[%%]", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Short{0}_Tasks", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (xiiUInt32 t = 0; t < xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::LongTasks); ++t)
        {
          xiiUInt32    uiNumTasks  = 0;
          const double Utilization = xiiTaskSystem::GetThreadUtilization(xiiWorkerThreadType::LongTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Long{0}_Load[%%]", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Long{0}_Tasks", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (xiiUInt32 t = 0; t < xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::FileAccess); ++t)
        {
          xiiUInt32    uiNumTasks  = 0;
          const double Utilization = xiiTaskSystem::GetThreadUtilization(xiiWorkerThreadType::FileAccess, t, &uiNumTasks);

          s.SetFormat("Utilization/File{0}_Load[%%]", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/File{0}_Tasks", xiiArgI(t, 2, true));
          xiiStats::SetStat(s.GetData(), uiNumTasks);
        }
      }
    }
    break;

    default:
      break;
  }
}

void AddStatsEventHandler()
{
  xiiStats::AddEventHandler(StatsEventHandler);

  xiiTelemetry::AddEventHandler(TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using xiiTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the xiiStats and xiiTelemetry system.
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(PerFrameUpdateHandler);
  }
}

void RemoveStatsEventHandler()
{
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(PerFrameUpdateHandler);
  }

  xiiTelemetry::RemoveEventHandler(TelemetryEventsHandler);

  xiiStats::RemoveEventHandler(StatsEventHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Stats);
