/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Utilities/Stats.h>

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
  if (auto* application = xiiApplication::GetApplicationInstance())
  {
    xiiTelemetrySourceInfo source;
    source.m_uiId                          = xiiTelemetrySources::Counters;
    source.m_sName                         = "Runtime statistics";
    source.m_sCategory                     = "Application";
    source.m_bSnapshot                     = true;
    source.m_bLatestOnly                   = true;
    source.m_uiMinimumIntervalMilliseconds = 33;
    application->CreateTelemetryService().RegisterSource(source, [](xiiTelemetryService& service, xiiUInt64 id) {
                                           xiiStats::MapType  values;
                                           const auto         omitted = xiiStats::Capture(values, 4000);
                                           xiiTelemetrySample sample;
                                           sample.m_uiSource               = id;
                                           sample.m_uiDropped              = omitted;
                                           sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
                                           for (auto it = values.GetIterator(); it.IsValid(); ++it)
                                           {
                                             auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
                                             row.Insert("name", it.Key());
                                             row.Insert("value", it.Value());
                                           }
                                           const auto diagnostics = service.GetStatistics();
                                           const auto network     = service.GetNetworkStatistics();
                                           // Direct observation avoids recursively publishing measurements through xiiStats.
                                           sample.m_Fields.Insert("Diagnostics/Clients", diagnostics.m_uiClients);
                                           sample.m_Fields.Insert("Diagnostics/Subscriptions", diagnostics.m_uiSubscriptions);
                                           sample.m_Fields.Insert("Diagnostics/Queued Samples", diagnostics.m_uiQueuedSamples);
                                           sample.m_Fields.Insert("Diagnostics/Queued Bytes", diagnostics.m_uiQueuedBytes);
                                           sample.m_Fields.Insert("Diagnostics/Collected", diagnostics.m_uiSamplesCollected);
                                           sample.m_Fields.Insert("Diagnostics/Dropped", diagnostics.m_uiSamplesDropped);
                                           sample.m_Fields.Insert("Diagnostics/Published Bytes", diagnostics.m_uiPublishedBytes);
                                           sample.m_Fields.Insert("Diagnostics/Backpressure", diagnostics.m_uiBackpressureEvents);
                                           sample.m_Fields.Insert("Diagnostics/Collection Time", diagnostics.m_CollectionTime);
                                           sample.m_Fields.Insert("Diagnostics/Serialization Time", diagnostics.m_SerializationTime);
                                           sample.m_Fields.Insert("Diagnostics/Publication Time", diagnostics.m_PublicationTime);
                                           sample.m_Fields.Insert("Diagnostics/Maximum Publication Delay", diagnostics.m_MaximumPublicationDelay);
                                           sample.m_Fields.Insert("Diagnostics/Pool Committed Bytes", network.m_IO.m_BufferPool.m_uiCommittedBytes);
                                           sample.m_Fields.Insert("Diagnostics/Pool Leased Bytes", network.m_IO.m_BufferPool.m_uiLeasedBytes);
                                           sample.m_Fields.Insert("Diagnostics/Pool Backpressure", network.m_IO.m_BufferPool.m_uiBackpressureEvents);
                                           service.Publish(std::move(sample)).IgnoreResult();
                                         })
      .AssertSuccess("Could not register runtime statistics.");
  }
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

  if (auto* application = xiiApplication::GetApplicationInstance())
    if (auto* service = application->GetTelemetryService())
      service->UnregisterSource(xiiTelemetrySources::Counters);
}
