/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace ResourceManagerDetail
{

  static void SendFullResourceInfo(const xiiResource* pRes)
  {
    xiiTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' SET');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << pRes->GetResourceID();
    Msg.GetWriter() << pRes->GetDynamicRTTI()->GetTypeName();
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;
    Msg.GetWriter() << pRes->GetResourceDescription();

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, Msg);
  }

  static void SendSmallResourceInfo(const xiiResource* pRes)
  {
    xiiTelemetryMessage Msg;

    Msg.SetMessageID('RESM', 'UPDT');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<xiiUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, Msg);
  }

  static void SendDeleteResourceInfo(const xiiResource* pRes)
  {
    xiiTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' DEL');

    Msg.GetWriter() << pRes->GetResourceIDHash();

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, Msg);
  }

  static void SendAllResourceTelemetry() { xiiResourceManager::BroadcastExistsEvent(); }

  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllResourceTelemetry();
        break;

      default:
        break;
    }
  }

  static void ResourceManagerEventHandler(const xiiResourceEvent& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case xiiResourceEvent::Type::ResourceCreated:
      case xiiResourceEvent::Type::ResourceExists:
        SendFullResourceInfo(e.m_pResource);
        return;

      case xiiResourceEvent::Type::ResourceDeleted:
        SendDeleteResourceInfo(e.m_pResource);
        return;

      case xiiResourceEvent::Type::ResourceContentUpdated:
      case xiiResourceEvent::Type::ResourceContentUnloading:
      case xiiResourceEvent::Type::ResourcePriorityChanged:
        SendSmallResourceInfo(e.m_pResource);
        return;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
} // namespace ResourceManagerDetail

void AddResourceManagerEventHandler()
{
  xiiTelemetry::AddEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
  xiiResourceManager::GetResourceEvents().AddEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
  xiiTelemetry::RemoveEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
}
