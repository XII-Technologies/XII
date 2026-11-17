/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Diagnostics/TelemetryService.h>

namespace
{
  void Collect(xiiTelemetryService& service, xiiUInt64 source)
  {
    xiiTelemetrySample sample;
    sample.m_uiSource               = source;
    sample.m_fTimestampMicroseconds = xiiTime::Now().GetMicroseconds();
    {
      auto resources     = xiiResourceManager::GetAllResourcesOfType<xiiResource>(4096);
      sample.m_uiDropped = resources->GetCount() == 4096 ? 1 : 0;
      for (const auto* resource : *resources)
      {
        xiiResource::ContentStats content;
        if (!resource->TryCaptureContentStats(content))
        {
          ++sample.m_uiDropped;
          continue;
        }
        auto& row = sample.m_Records.ExpandAndGetRef().m_Fields;
        row.Insert("id", resource->GetResourceIDHash());
        row.Insert("name", xiiString(resource->GetResourceID()));
        row.Insert("type", xiiString(resource->GetDynamicRTTI()->GetTypeName()));
        row.Insert("priority", static_cast<xiiUInt8>(resource->GetPriority()));
        row.Insert("flags", static_cast<xiiUInt8>(resource->GetBaseResourceFlags().GetValue()));
        row.Insert("state", static_cast<xiiUInt8>(content.m_State));
        row.Insert("discardable", content.m_uiDiscardable);
        row.Insert("loadable", content.m_uiLoadable);
        row.Insert("cpuBytes", content.m_Memory.m_uiMemoryCPU);
        row.Insert("gpuBytes", content.m_Memory.m_uiMemoryGPU);
        row.Insert("description", content.m_sDescription);
      }
    }
    sample.m_Fields.Insert("complete", sample.m_uiDropped == 0);
    service.Publish(std::move(sample)).IgnoreResult();
  }
} // namespace

void AddResourceManagerEventHandler()
{
  xiiTelemetrySourceInfo info;
  info.m_uiId                          = xiiTelemetrySources::Resources;
  info.m_sName                         = "Resources";
  info.m_sCategory                     = "Resources";
  info.m_uiMinimumIntervalMilliseconds = 500;
  info.m_bSnapshot = info.m_bLatestOnly = true;
  XII_VERIFY(xiiApplication::GetApplicationInstance()->CreateTelemetryService().RegisterSource(info, Collect).Succeeded(), "Duplicate resource source.");
}
void RemoveResourceManagerEventHandler()
{
  xiiApplication::GetApplicationInstance()->GetTelemetryService()->UnregisterSource(xiiTelemetrySources::Resources);
}
