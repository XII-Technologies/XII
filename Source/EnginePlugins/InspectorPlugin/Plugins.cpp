/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>

namespace PluginsDetail
{
  static void SendPluginTelemetry()
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'PLUG', ' CLR', nullptr, 0);

    xiiHybridArray<xiiPlugin::PluginInfo, 16> infos;
    xiiPlugin::GetAllPluginInfos(infos);

    for (const auto& pi : infos)
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID('PLUG', 'DATA');
      msg.GetWriter() << pi.m_sName;
      msg.GetWriter() << false; // deprecated 'IsReloadable' flag

      xiiStringBuilder s;

      for (const auto& dep : pi.m_sDependencies)
      {
        s.AppendWithSeparator(" | ", dep);
      }

      msg.GetWriter() << s;

      xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
    }
  }

  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }

  static void PluginEventHandler(const xiiPluginEvent& e)
  {
    switch (e.m_EventType)
    {
      case xiiPluginEvent::AfterPluginChanges:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace PluginsDetail

void AddPluginEventHandler()
{
  xiiTelemetry::AddEventHandler(PluginsDetail::TelemetryEventsHandler);
  xiiPlugin::Events().AddEventHandler(PluginsDetail::PluginEventHandler);
}

void RemovePluginEventHandler()
{
  xiiPlugin::Events().RemoveEventHandler(PluginsDetail::PluginEventHandler);
  xiiTelemetry::RemoveEventHandler(PluginsDetail::TelemetryEventsHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Plugins);
