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

  static const char* GetParentType(xiiRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
      return pRTTI->GetParentType()->GetTypeName();

    if ((xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "bool")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "float")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "double")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiInt8")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiUInt8")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiInt16")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiUInt16")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiInt32")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiUInt32")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiInt64")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiUInt64")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiConstCharPtr")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiVec2")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiVec3")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiVec4")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiMat3")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiMat4")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiTime")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiUuid")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiColor")) ||
        (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiVariant")) || (xiiStringUtils::IsEqual(pRTTI->GetTypeName(), "xiiQuat")))
      return "Basic Types";

    return "";
  }

  static void SendReflectionTelemetry(xiiRTTI* pRTTI)
  {
    xiiTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << pRTTI->GetTypeName();
    msg.GetWriter() << GetParentType(pRTTI);
    msg.GetWriter() << pRTTI->GetTypeSize();
    msg.GetWriter() << pRTTI->GetPluginName();

    {
      const xiiArrayPtr<xiiAbstractProperty*>& properties = pRTTI->GetProperties();

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

    xiiRTTI* pRTTI = xiiRTTI::GetFirstInstance();

    while (pRTTI)
    {
      SendReflectionTelemetry(pRTTI);

      pRTTI = pRTTI->GetNextInstance();
    }
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
