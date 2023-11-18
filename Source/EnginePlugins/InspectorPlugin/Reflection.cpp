#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Reflection/Reflection.h>

namespace ReflectionDetail
{

  static void SendBasicTypesGroup()
  {
    xiiTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << "Basic Types";
    msg.GetWriter() << "";
    msg.GetWriter() << 0;
    msg.GetWriter() << "";
    msg.GetWriter() << (xiiUInt32)0U;
    msg.GetWriter() << (xiiUInt32)0U;

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }

  static xiiStringView GetParentType(const xiiRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
    {
      return pRTTI->GetParentType()->GetTypeName();
    }

    if ((pRTTI->GetTypeName() == "bool") || (pRTTI->GetTypeName() == "float") ||
        (pRTTI->GetTypeName() == "double") || (pRTTI->GetTypeName() == "xiiInt8") ||
        (pRTTI->GetTypeName() == "xiiUInt8") || (pRTTI->GetTypeName() == "xiiInt16") ||
        (pRTTI->GetTypeName() == "xiiUInt16") || (pRTTI->GetTypeName() == "xiiInt32") ||
        (pRTTI->GetTypeName() == "xiiUInt32") || (pRTTI->GetTypeName() == "xiiInt64") ||
        (pRTTI->GetTypeName() == "xiiUInt64") || (pRTTI->GetTypeName() == "xiiConstCharPtr") ||

        (pRTTI->GetTypeName() == "xiiVec2") || (pRTTI->GetTypeName() == "xiiVec3") ||
        (pRTTI->GetTypeName() == "xiiVec4") || (pRTTI->GetTypeName() == "xiiMat3") ||
        (pRTTI->GetTypeName() == "xiiMat4") ||

        (pRTTI->GetTypeName() == "xiiVec2d") || (pRTTI->GetTypeName() == "xiiVec3d") ||
        (pRTTI->GetTypeName() == "xiiVec4d") || (pRTTI->GetTypeName() == "xiiMat3d") ||
        (pRTTI->GetTypeName() == "xiiMat4d") ||

        (pRTTI->GetTypeName() == "xiiVec2I") || (pRTTI->GetTypeName() == "xiiVec3I") ||
        (pRTTI->GetTypeName() == "xiiVec4I") ||

        (pRTTI->GetTypeName() == "xiiVec2U") || (pRTTI->GetTypeName() == "xiiVec3U") ||
        (pRTTI->GetTypeName() == "xiiVec4U") ||

        (pRTTI->GetTypeName() == "xiiVec2I64") || (pRTTI->GetTypeName() == "xiiVec3I64") ||
        (pRTTI->GetTypeName() == "xiiVec4I64") ||

        (pRTTI->GetTypeName() == "xiiVec2U64") || (pRTTI->GetTypeName() == "xiiVec3U64") ||
        (pRTTI->GetTypeName() == "xiiVec4U64") ||

        (pRTTI->GetTypeName() == "xiiTime") || (pRTTI->GetTypeName() == "xiiUuid") || (pRTTI->GetTypeName() == "xiiColor") ||
        (pRTTI->GetTypeName() == "xiiVariant") || (pRTTI->GetTypeName() == "xiiQuat") || (pRTTI->GetTypeName() == "xiiQuatd"))
    {
      return "Basic Types";
    }

    return {};
  }

  static void SendReflectionTelemetry(const xiiRTTI* pRTTI)
  {
    xiiTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << pRTTI->GetTypeName();
    msg.GetWriter() << GetParentType(pRTTI);
    msg.GetWriter() << pRTTI->GetTypeSize();
    msg.GetWriter() << pRTTI->GetPluginName();

    {
      auto properties = pRTTI->GetProperties();

      msg.GetWriter() << properties.GetCount();

      for (auto& prop : properties)
      {
        msg.GetWriter() << prop->GetPropertyName();
        msg.GetWriter() << (xiiInt8)prop->GetCategory();

        const xiiRTTI* pType = prop->GetSpecificType();
        msg.GetWriter() << (pType ? pType->GetTypeName() : "<Unknown Type>");
      }
    }

    {
      const xiiArrayPtr<xiiAbstractMessageHandler*>& Messages = pRTTI->GetMessageHandlers();

      msg.GetWriter() << Messages.GetCount();

      for (xiiUInt32 i = 0; i < Messages.GetCount(); ++i)
      {
        msg.GetWriter() << Messages[i]->GetMessageId();
      }
    }

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }

  static void SendAllReflectionTelemetry()
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    // clear
    {
      xiiTelemetryMessage msg;
      xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'RFLC', ' CLR', nullptr, 0);
    }

    SendBasicTypesGroup();

    xiiRTTI::ForEachType([](const xiiRTTI* pRtti) { SendReflectionTelemetry(pRtti); });
  }


  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllReflectionTelemetry();
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
        SendAllReflectionTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace ReflectionDetail

void AddReflectionEventHandler()
{
  xiiTelemetry::AddEventHandler(ReflectionDetail::TelemetryEventsHandler);

  xiiPlugin::Events().AddEventHandler(ReflectionDetail::PluginEventHandler);
}

void RemoveReflectionEventHandler()
{
  xiiPlugin::Events().RemoveEventHandler(ReflectionDetail::PluginEventHandler);

  xiiTelemetry::RemoveEventHandler(ReflectionDetail::TelemetryEventsHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Reflection);
