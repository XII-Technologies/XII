#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>

namespace StartupDetail
{
  static void     SendSubsystemTelemetry();
  static xiiInt32 s_iSendSubSystemTelemetry = 0;
} // namespace StartupDetail

XII_ON_GLOBAL_EVENT(xiiStartup_StartupCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

XII_ON_GLOBAL_EVENT(xiiStartup_StartupHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

XII_ON_GLOBAL_EVENT(xiiStartup_ShutdownCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

XII_ON_GLOBAL_EVENT(xiiStartup_ShutdownHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

namespace StartupDetail
{
  static void SendSubsystemTelemetry()
  {
    if (s_iSendSubSystemTelemetry <= 0)
      return;

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'STRT', ' CLR', nullptr, 0);

    xiiSubSystem* pSub = xiiSubSystem::GetFirstInstance();

    while (pSub)
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID('STRT', 'SYST');
      msg.GetWriter() << pSub->GetGroupName();
      msg.GetWriter() << pSub->GetSubSystemName();
      msg.GetWriter() << pSub->GetPluginName();

      for (xiiUInt32 i = 0; i < xiiStartupStage::ENUM_COUNT; ++i)
        msg.GetWriter() << pSub->IsStartupPhaseDone((xiiStartupStage::Enum)i);

      xiiUInt8 uiDependencies = 0;
      while (pSub->GetDependency(uiDependencies) != nullptr)
        ++uiDependencies;

      msg.GetWriter() << uiDependencies;

      for (xiiUInt8 i = 0; i < uiDependencies; ++i)
        msg.GetWriter() << pSub->GetDependency(i);

      xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);

      pSub = pSub->GetNextInstance();
    }
  }

  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendSubsystemTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace StartupDetail

void AddStartupEventHandler()
{
  ++StartupDetail::s_iSendSubSystemTelemetry;
  xiiTelemetry::AddEventHandler(StartupDetail::TelemetryEventsHandler);
}

void RemoveStartupEventHandler()
{
  --StartupDetail::s_iSendSubSystemTelemetry;
  xiiTelemetry::RemoveEventHandler(StartupDetail::TelemetryEventsHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Startup);
