/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

namespace
{
  void FrameCompleted(const xiiGameApplicationExecutionEvent& event)
  {
    if (event.m_Type != xiiGameApplicationExecutionEvent::Type::AfterPresent)
      return;
    auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService();
    if (!service || (!service->IsCollecting(xiiTelemetrySources::Memory) && !service->IsCollecting(xiiTelemetrySources::Counters)))
      return;
    xiiUInt64 allocations = 0, bytes = 0;
    xiiTime   allocationTime;
    for (auto it = xiiMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      allocations += it.Stats().m_uiAllocationCount;
      bytes += it.Stats().m_uiPerFrameAllocationSize;
      allocationTime += it.Stats().m_PerFrameAllocationTime;
    }
    static xiiUInt64 previous = 0;
    xiiStats::SetStat("App/Allocs Per Frame", allocations >= previous ? allocations - previous : 0);
    xiiStats::SetStat("App/Per Frame Alloc Size (byte)", bytes);
    xiiStats::SetStat("App/Per Frame Alloc Time", allocationTime);
    previous = allocations;
    xiiMemoryTracker::ResetPerFrameAllocatorStats();
  }

  void Collect(xiiTelemetryService& service, xiiUInt64 id)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = id;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();

    for (auto it = xiiMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      if (sample.m_Records.GetCount() == 4096)
      {
        ++sample.m_uiDropped;
        continue;
      }
      auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
      row.Insert("id", static_cast<xiiUInt32>(it.Id().m_Data));
      row.Insert("name", xiiString(it.Name()));
      row.Insert("parent", static_cast<xiiUInt32>(it.ParentId().IsInvalidated() ? xiiInvalidIndex : it.ParentId().m_Data));
      row.Insert("allocations", it.Stats().m_uiAllocationCount);
      row.Insert("deallocations", it.Stats().m_uiDeallocationCount);
      row.Insert("bytes", it.Stats().m_uiAllocationSize);
    }
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddMemoryEventHandler()
{
  if (auto* application = xiiGameApplicationBase::GetGameApplicationBaseInstance())
    application->m_ExecutionEvents.AddEventHandler(FrameCompleted);
  auto&                  service = xiiApplication::GetApplicationInstance()->CreateTelemetryService();
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Memory;
  info.m_sName                         = "Allocator statistics";
  info.m_sCategory                     = "Memory";
  info.m_uiMinimumIntervalMilliseconds = 200;
  info.m_bSnapshot                     = true;
  info.m_bLatestOnly                   = true;
  XII_VERIFY(service.RegisterSource(info, Collect).Succeeded(), "Duplicate diagnostics source.");
}

void RemoveMemoryEventHandler()
{
  if (auto* application = xiiGameApplicationBase::GetGameApplicationBaseInstance())
    application->m_ExecutionEvents.RemoveEventHandler(FrameCompleted);
  if (auto* service = xiiApplication::GetApplicationInstance()->GetTelemetryService())
    service->UnregisterSource(xiiTelemetrySources::Memory);
}
