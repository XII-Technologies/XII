/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

xiiInputManager::xiiEventInput xiiInputManager::s_InputEvents;
xiiInputManager::InternalData* xiiInputManager::s_pData                   = nullptr;
xiiUInt32                      xiiInputManager::s_uiLastCharacter         = '\0';
bool                           xiiInputManager::s_bInputSlotResetRequired = true;
xiiString                      xiiInputManager::s_sExclusiveInputSet;

xiiInputManager::InternalData& xiiInputManager::GetInternals()
{
  if (s_pData == nullptr)
    s_pData = XII_DEFAULT_NEW(InternalData);

  return *s_pData;
}

void xiiInputManager::DeallocateInternals()
{
  XII_DEFAULT_DELETE(s_pData);
}

xiiInputManager::xiiInputSlot::xiiInputSlot()
{
  m_fValue    = 0.0f;
  m_State     = xiiKeyState::Up;
  m_fDeadZone = 0.0f;
}

void xiiInputManager::RegisterInputSlot(xiiStringView sInputSlot, xiiStringView sDefaultDisplayName, xiiBitflags<xiiInputSlotFlags> SlotFlags)
{
  xiiMap<xiiString, xiiInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != xiiInputSlotFlags::Default) && (SlotFlags != xiiInputSlotFlags::Default))
      {
        xiiStringBuilder tmp, tmp2;
        tmp.SetPrintf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b", sInputSlot.GetData(tmp2), it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());

        xiiLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // xiiLog::Debug("Registered Input Slot: '{0}'", sInputSlot);

  xiiInputSlot& sm = GetInternals().s_InputSlots[sInputSlot];

  sm.m_sDisplayName = sDefaultDisplayName;
  sm.m_SlotFlags    = SlotFlags;

  InputEventData e;
  e.m_EventType  = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

xiiBitflags<xiiInputSlotFlags> xiiInputManager::GetInputSlotFlags(xiiStringView sInputSlot)
{
  xiiMap<xiiString, xiiInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  xiiLog::Warning("xiiInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", sInputSlot);

  return xiiInputSlotFlags::Default;
}

void xiiInputManager::SetInputSlotDisplayName(xiiStringView sInputSlot, xiiStringView sDefaultDisplayName)
{
  RegisterInputSlot(sInputSlot, sDefaultDisplayName, xiiInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_sDisplayName = sDefaultDisplayName;

  InputEventData e;
  e.m_EventType  = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

xiiStringView xiiInputManager::GetInputSlotDisplayName(xiiStringView sInputSlot)
{
  xiiMap<xiiString, xiiInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  xiiLog::Warning("xiiInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", sInputSlot);
  return sInputSlot;
}

xiiStringView xiiInputManager::GetInputSlotDisplayName(xiiStringView sInputSet, xiiStringView sAction, xiiInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(sInputSet, sAction);

  if (iTrigger < 0)
  {
    for (iTrigger = 0; iTrigger < xiiInputActionConfig::MaxInputSlotAlternatives; ++iTrigger)
    {
      if (!cfg.m_sInputSlotTrigger[iTrigger].IsEmpty())
        break;
    }
  }

  if (iTrigger >= xiiInputActionConfig::MaxInputSlotAlternatives)
    return nullptr;

  return GetInputSlotDisplayName(cfg.m_sInputSlotTrigger[iTrigger]);
}

void xiiInputManager::SetInputSlotDeadZone(xiiStringView sInputSlot, float fDeadZone)
{
  RegisterInputSlot(sInputSlot, sInputSlot, xiiInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_fDeadZone = xiiMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType  = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

float xiiInputManager::GetInputSlotDeadZone(xiiStringView sInputSlot)
{
  xiiMap<xiiString, xiiInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  xiiLog::Warning("xiiInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", sInputSlot);

  xiiInputSlot s;
  return s.m_fDeadZone; // return the default value
}

xiiKeyState::Enum xiiInputManager::GetInputSlotState(xiiStringView sInputSlot, float* pValue)
{
  xiiMap<xiiString, xiiInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (pValue)
    {
      *pValue = s_bInputSlotResetRequired ? it.Value().m_fValue : it.Value().m_fValueOld;
    }
    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  xiiLog::Warning("xiiInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                  "xiiInputManager::Update before querying device states, or at least call xiiInputManager::PollHardware.",
                  sInputSlot);

  RegisterInputSlot(sInputSlot, sInputSlot, xiiInputSlotFlags::None);

  return xiiKeyState::Up;
}

void xiiInputManager::PollHardware()
{
  if (s_bInputSlotResetRequired)
  {
    s_bInputSlotResetRequired = false;
    ResetInputSlotValues();
  }

  xiiInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
}

void xiiInputManager::Update(xiiTime timeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_uiLastCharacter = xiiInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions(timeDifference);

  xiiInputDevice::ResetAllDevices();

  xiiInputDevice::UpdateAllHardwareStates(timeDifference);

  s_bInputSlotResetRequired = true;
}

void xiiInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (xiiInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValueOld = it.Value().m_fValue;
    it.Value().m_fValue    = 0.0f;
  }
}

void xiiInputManager::GatherDeviceInputSlotValues()
{
  for (xiiInputDevice* pDevice = xiiInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->m_bGeneratedInputRecently = false;

    // iterate over all the input slots that this device provides
    for (auto it = pDevice->m_InputSlotValues.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        xiiInputManager::xiiInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the dead-zone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          Slot.m_fValue = xiiMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices

          pDevice->m_bGeneratedInputRecently = true;
        }
      }
    }
  }

  xiiMap<xiiString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for (; it.IsValid(); ++it)
  {
    xiiInputManager::xiiInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the dead-zone threshold
    if (it.Value() > Slot.m_fDeadZone)
      Slot.m_fValue = xiiMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
  }

  GetInternals().s_InjectedInputSlots.Clear();
}

void xiiInputManager::UpdateInputSlotStates()
{
  for (xiiInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its dead-zone value
    const xiiKeyState::Enum NewState = xiiKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);

    if ((it.Value().m_State != NewState) || (NewState != xiiKeyState::Up))
    {
      it.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType  = InputEventData::InputSlotChanged;
      e.m_sInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
  }
}

void xiiInputManager::RetrieveAllKnownInputSlots(xiiDynamicArray<xiiStringView>& out_inputSlots)
{
  out_inputSlots.Clear();
  out_inputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (xiiInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_inputSlots.PushBack(it.Key().GetData());
  }
}

xiiUInt32 xiiInputManager::RetrieveLastCharacter(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_uiLastCharacter;

  xiiUInt32 Temp    = s_uiLastCharacter;
  s_uiLastCharacter = L'\0';
  return Temp;
}

void xiiInputManager::InjectInputSlotValue(xiiStringView sInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[sInputSlot] = xiiMath::Max(GetInternals().s_InjectedInputSlots[sInputSlot], fValue);
}

xiiStringView xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::Enum mustHaveFlags, xiiInputSlotFlags::Enum mustNotHaveFlags)
{
  for (xiiInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != xiiKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(mustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(mustHaveFlags))
      return it.Key().GetData();
  }

  return xiiInputSlot_None;
}

xiiStringView xiiInputManager::GetInputSlotTouchPoint(xiiUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return xiiInputSlot_TouchPoint0;
    case 1:
      return xiiInputSlot_TouchPoint1;
    case 2:
      return xiiInputSlot_TouchPoint2;
    case 3:
      return xiiInputSlot_TouchPoint3;
    case 4:
      return xiiInputSlot_TouchPoint4;
    case 5:
      return xiiInputSlot_TouchPoint5;
    case 6:
      return xiiInputSlot_TouchPoint6;
    case 7:
      return xiiInputSlot_TouchPoint7;
    case 8:
      return xiiInputSlot_TouchPoint8;
    case 9:
      return xiiInputSlot_TouchPoint9;
    default:
      XII_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

xiiStringView xiiInputManager::GetInputSlotTouchPointPositionX(xiiUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return xiiInputSlot_TouchPoint0_PositionX;
    case 1:
      return xiiInputSlot_TouchPoint1_PositionX;
    case 2:
      return xiiInputSlot_TouchPoint2_PositionX;
    case 3:
      return xiiInputSlot_TouchPoint3_PositionX;
    case 4:
      return xiiInputSlot_TouchPoint4_PositionX;
    case 5:
      return xiiInputSlot_TouchPoint5_PositionX;
    case 6:
      return xiiInputSlot_TouchPoint6_PositionX;
    case 7:
      return xiiInputSlot_TouchPoint7_PositionX;
    case 8:
      return xiiInputSlot_TouchPoint8_PositionX;
    case 9:
      return xiiInputSlot_TouchPoint9_PositionX;
    default:
      XII_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

xiiStringView xiiInputManager::GetInputSlotTouchPointPositionY(xiiUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return xiiInputSlot_TouchPoint0_PositionY;
    case 1:
      return xiiInputSlot_TouchPoint1_PositionY;
    case 2:
      return xiiInputSlot_TouchPoint2_PositionY;
    case 3:
      return xiiInputSlot_TouchPoint3_PositionY;
    case 4:
      return xiiInputSlot_TouchPoint4_PositionY;
    case 5:
      return xiiInputSlot_TouchPoint5_PositionY;
    case 6:
      return xiiInputSlot_TouchPoint6_PositionY;
    case 7:
      return xiiInputSlot_TouchPoint7_PositionY;
    case 8:
      return xiiInputSlot_TouchPoint8_PositionY;
    case 9:
      return xiiInputSlot_TouchPoint9_PositionY;
    default:
      XII_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

XII_STATICLINK_FILE(Core, Core_Input_Implementation_InputManager);
