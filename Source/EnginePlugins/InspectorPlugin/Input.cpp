#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace InputDetail
{

  static void SendInputSlotData(const char* szInputSlot)
  {
    float fValue = 0.0f;

    xiiTelemetryMessage msg;
    msg.SetMessageID('INPT', 'SLOT');
    msg.GetWriter() << szInputSlot;
    msg.GetWriter() << xiiInputManager::GetInputSlotFlags(szInputSlot).GetValue();
    msg.GetWriter() << (xiiUInt8)xiiInputManager::GetInputSlotState(szInputSlot, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << xiiInputManager::GetInputSlotDeadZone(szInputSlot);

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }

  static void SendInputActionData(const char* szInputSet, const char* szInputAction)
  {
    float fValue = 0.0f;

    const xiiInputActionConfig cfg = xiiInputManager::GetInputActionConfig(szInputSet, szInputAction);

    xiiTelemetryMessage msg;
    msg.SetMessageID('INPT', 'ACTN');
    msg.GetWriter() << szInputSet;
    msg.GetWriter() << szInputAction;
    msg.GetWriter() << (xiiUInt8)xiiInputManager::GetInputActionState(szInputSet, szInputAction, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << cfg.m_bApplyTimeScaling;

    for (xiiUInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
    {
      msg.GetWriter() << cfg.m_sInputSlotTrigger[i];
      msg.GetWriter() << cfg.m_fInputSlotScale[i];
    }

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }

  static void SendAllInputSlots()
  {
    xiiDynamicArray<const char*> InputSlots;
    xiiInputManager::RetrieveAllKnownInputSlots(InputSlots);

    for (xiiUInt32 i = 0; i < InputSlots.GetCount(); ++i)
      SendInputSlotData(InputSlots[i]);
  }

  static void SendAllInputActions()
  {
    xiiDynamicArray<xiiString> InputSetNames;
    xiiInputManager::GetAllInputSets(InputSetNames);

    for (xiiUInt32 s = 0; s < InputSetNames.GetCount(); ++s)
    {
      xiiHybridArray<xiiString, 24> InputActions;

      xiiInputManager::GetAllInputActions(InputSetNames[s].GetData(), InputActions);

      for (xiiUInt32 a = 0; a < InputActions.GetCount(); ++a)
        SendInputActionData(InputSetNames[s].GetData(), InputActions[a].GetData());
    }
  }

  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case xiiTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllInputSlots();
        SendAllInputActions();
        break;

      default:
        break;
    }
  }

  static void InputManagerEventHandler(const xiiInputManager::InputEventData& e)
  {
    if (!xiiTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case xiiInputManager::InputEventData::InputActionChanged:
        SendInputActionData(e.m_szInputSet, e.m_szInputAction);
        break;
      case xiiInputManager::InputEventData::InputSlotChanged:
        SendInputSlotData(e.m_szInputSlot);
        break;

      default:
        break;
    }
  }
} // namespace InputDetail

void AddInputEventHandler()
{
  xiiTelemetry::AddEventHandler(InputDetail::TelemetryEventsHandler);
  xiiInputManager::AddEventHandler(InputDetail::InputManagerEventHandler);
}

void RemoveInputEventHandler()
{
  xiiInputManager::RemoveEventHandler(InputDetail::InputManagerEventHandler);
  xiiTelemetry::RemoveEventHandler(InputDetail::TelemetryEventsHandler);
}



XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Input);
