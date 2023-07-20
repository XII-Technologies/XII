#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace InputDetail
{
  static void SendInputSlotData(xiiStringView sInputSlot)
  {
    float fValue = 0.0f;

    xiiTelemetryMessage msg;
    msg.SetMessageID('INPT', 'SLOT');
    msg.GetWriter() << sInputSlot;
    msg.GetWriter() << xiiInputManager::GetInputSlotFlags(sInputSlot).GetValue();
    msg.GetWriter() << (xiiUInt8)xiiInputManager::GetInputSlotState(sInputSlot, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << xiiInputManager::GetInputSlotDeadZone(sInputSlot);

    xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
  }

  static void SendInputActionData(xiiStringView sInputSet, xiiStringView sInputAction)
  {
    float fValue = 0.0f;

    const xiiInputActionConfig cfg = xiiInputManager::GetInputActionConfig(sInputSet, sInputAction);

    xiiTelemetryMessage msg;
    msg.SetMessageID('INPT', 'ACTN');
    msg.GetWriter() << sInputSet;
    msg.GetWriter() << sInputAction;
    msg.GetWriter() << (xiiUInt8)xiiInputManager::GetInputActionState(sInputSet, sInputAction, &fValue);
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
    xiiDynamicArray<xiiStringView> InputSlots;
    xiiInputManager::RetrieveAllKnownInputSlots(InputSlots);

    for (xiiUInt32 i = 0; i < InputSlots.GetCount(); ++i)
    {
      SendInputSlotData(InputSlots[i]);
    }
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
      {
        SendInputActionData(InputSetNames[s].GetData(), InputActions[a].GetData());
      }
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
        SendInputActionData(e.m_sInputSet, e.m_sInputAction);
        break;
      case xiiInputManager::InputEventData::InputSlotChanged:
        SendInputSlotData(e.m_sInputSlot);
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
