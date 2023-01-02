#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage('SVAR', Msg) == XII_SUCCESS)
  {
    if (Msg.GetMessageID() == ' SET')
    {
      xiiString sCVar;
      xiiUInt8  uiType;

      float     fValue;
      double    dValue;
      xiiInt32  iValue;
      bool      bValue;
      xiiString sValue;

      Msg.GetReader() >> sCVar;
      Msg.GetReader() >> uiType;

      switch (uiType)
      {
        case xiiCVarType::Double:
          Msg.GetReader() >> dValue;
          break;
        case xiiCVarType::Float:
          Msg.GetReader() >> fValue;
          break;
        case xiiCVarType::Int:
          Msg.GetReader() >> iValue;
          break;
        case xiiCVarType::Bool:
          Msg.GetReader() >> bValue;
          break;
        case xiiCVarType::String:
          Msg.GetReader() >> sValue;
          break;
      }

      xiiCVar* pCVar = xiiCVar::GetFirstInstance();

      while (pCVar)
      {
        if (((xiiUInt8)pCVar->GetType() == uiType) && (pCVar->GetName() == sCVar))
        {
          switch (uiType)
          {
            case xiiCVarType::Double:
              *((xiiCVarDouble*)pCVar) = dValue;
              break;
            case xiiCVarType::Float:
              *((xiiCVarFloat*)pCVar) = fValue;
              break;
            case xiiCVarType::Int:
              *((xiiCVarInt*)pCVar) = iValue;
              break;
            case xiiCVarType::Bool:
              *((xiiCVarBool*)pCVar) = bValue;
              break;
            case xiiCVarType::String:
              *((xiiCVarString*)pCVar) = sValue;
              break;
          }
        }

        pCVar = pCVar->GetNextInstance();
      }
    }
  }
}

static void SendCVarTelemetry(xiiCVar* pCVar)
{
  xiiTelemetryMessage msg;
  msg.SetMessageID('CVAR', 'DATA');
  msg.GetWriter() << pCVar->GetName();
  msg.GetWriter() << pCVar->GetPluginName();
  // msg.GetWriter() << (xiiUInt8) pCVar->GetFlags().GetValue(); // currently not used
  msg.GetWriter() << (xiiUInt8)pCVar->GetType();
  msg.GetWriter() << pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case xiiCVarType::Double:
    {
      const double val = ((xiiCVarDouble*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case xiiCVarType::Float:
    {
      const float val = ((xiiCVarFloat*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case xiiCVarType::Int:
    {
      const int val = ((xiiCVarInt*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case xiiCVarType::Bool:
    {
      const bool val = ((xiiCVarBool*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case xiiCVarType::String:
    {
      const char* val = ((xiiCVarString*)pCVar)->GetValue().GetData();
      msg.GetWriter() << val;
    }
    break;

    case xiiCVarType::ENUM_COUNT:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
}

static void SendAllCVarTelemetry()
{
  if (!xiiTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    xiiTelemetryMessage msg;
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'CVAR', ' CLR', nullptr, 0);
  }

  xiiCVar* pCVar = xiiCVar::GetFirstInstance();

  while (pCVar)
  {
    SendCVarTelemetry(pCVar);

    pCVar = pCVar->GetNextInstance();
  }

  {
    xiiTelemetryMessage msg;
    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, 'CVAR', 'SYNC', nullptr, 0);
  }
}

namespace CVarsDetail
{

  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }

  static void CVarEventHandler(const xiiCVarEvent& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case xiiCVarEvent::ValueChanged:
        SendCVarTelemetry(e.m_pCVar);
        break;

      case xiiCVarEvent::ListOfVarsChanged:
        SendAllCVarTelemetry();
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
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace CVarsDetail

void AddCVarEventHandler()
{
  xiiTelemetry::AddEventHandler(CVarsDetail::TelemetryEventsHandler);
  xiiTelemetry::AcceptMessagesForSystem('SVAR', true, TelemetryMessage, nullptr);

  xiiCVar::s_AllCVarEvents.AddEventHandler(CVarsDetail::CVarEventHandler);
  xiiPlugin::Events().AddEventHandler(CVarsDetail::PluginEventHandler);
}

void RemoveCVarEventHandler()
{
  xiiPlugin::Events().RemoveEventHandler(CVarsDetail::PluginEventHandler);
  xiiCVar::s_AllCVarEvents.RemoveEventHandler(CVarsDetail::CVarEventHandler);

  xiiTelemetry::RemoveEventHandler(CVarsDetail::TelemetryEventsHandler);
  xiiTelemetry::AcceptMessagesForSystem('SVAR', false);
}



XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_CVars);
