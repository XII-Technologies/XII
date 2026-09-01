/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

#include <Core/GameApplication/GameApplicationBase.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    xiiUInt64 uiTotalAllocations            = 0;
    xiiUInt64 uiTotalPerFrameAllocationSize = 0;
    xiiTime   TotalPerFrameAllocationTime;

    {
      xiiTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'BGN');
      xiiTelemetry::Broadcast(xiiTelemetry::Unreliable, msg);
    }

    for (auto it = xiiMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'STAT');
      msg.GetWriter() << it.Id().m_Data;
      msg.GetWriter() << it.Name();
      msg.GetWriter() << (it.ParentId().IsInvalidated() ? xiiInvalidIndex : it.ParentId().m_Data);
      msg.GetWriter() << it.Stats();

      uiTotalAllocations += it.Stats().m_uiAllocationCount;
      uiTotalPerFrameAllocationSize += it.Stats().m_uiPerFrameAllocationSize;
      TotalPerFrameAllocationTime += it.Stats().m_PerFrameAllocationTime;

      xiiTelemetry::Broadcast(xiiTelemetry::Unreliable, msg);
    }

    {
      xiiTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'END');
      xiiTelemetry::Broadcast(xiiTelemetry::Unreliable, msg);
    }

    static xiiUInt64 uiLastTotalAllocations = 0;

    xiiStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);
    xiiStats::SetStat("App/Per Frame Alloc Size (byte)", uiTotalPerFrameAllocationSize);
    xiiStats::SetStat("App/Per Frame Alloc Time", TotalPerFrameAllocationTime);

    uiLastTotalAllocations = uiTotalAllocations;

    xiiMemoryTracker::ResetPerFrameAllocatorStats();
  }

  static void PerframeUpdateHandler(const xiiGameApplicationExecutionEvent& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case xiiGameApplicationExecutionEvent::Type::AfterPresent:
        BroadcastMemoryStats();
        break;

      default:
        break;
    }
  }
} // namespace MemoryDetail


void AddMemoryEventHandler()
{
  // We're handling the per frame update by a different event since
  // using xiiTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the xiiStats and xiiTelemetry system.
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}

void RemoveMemoryEventHandler()
{
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Memory);
