/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiInputDevice);

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiKeyState::Enum xiiKeyState::GetNewKeyState(xiiKeyState::Enum prevState, bool bKeyDown)
{
  switch (prevState)
  {
    case xiiKeyState::Down:
    case xiiKeyState::Pressed:
      return bKeyDown ? xiiKeyState::Down : xiiKeyState::Released;
    case xiiKeyState::Released:
    case xiiKeyState::Up:
      return bKeyDown ? xiiKeyState::Pressed : xiiKeyState::Up;
  }

  return xiiKeyState::Up;
}

xiiInputDevice::xiiInputDevice()
{
  m_bInitialized    = false;
  m_uiLastCharacter = '\0';
}

void xiiInputDevice::RegisterInputSlot(xiiStringView sName, xiiStringView sDefaultDisplayName, xiiBitflags<xiiInputSlotFlags> SlotFlags)
{
  xiiInputManager::RegisterInputSlot(sName, sDefaultDisplayName, SlotFlags);
}

void xiiInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  XII_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  xiiLog::Dev("Input Device Type: {0}, Device Name: {1}", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void xiiInputDevice::UpdateAllHardwareStates(xiiTime tTimeDifference)
{
  // tell each device to update its hardware
  for (xiiInputDevice* pDevice = xiiInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void xiiInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (xiiInputDevice* pDevice = xiiInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->Initialize();
    pDevice->UpdateInputSlotValues();
  }
}

void xiiInputDevice::ResetAllDevices()
{
  // tell all devices that the input update is through and they might need to reset some values now
  // this is especially important for device types that will get input messages at some undefined time after this call
  // but not during 'UpdateInputSlotValues'
  for (xiiInputDevice* pDevice = xiiInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

xiiUInt32 xiiInputDevice::RetrieveLastCharacter()
{
  xiiUInt32 Temp    = m_uiLastCharacter;
  m_uiLastCharacter = L'\0';
  return Temp;
}

xiiUInt32 xiiInputDevice::RetrieveLastCharacterFromAllDevices()
{
  for (xiiInputDevice* pDevice = xiiInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    const xiiUInt32 Char = pDevice->RetrieveLastCharacter();

    if (Char != L'\0')
      return Char;
  }

  return '\0';
}

float xiiInputDevice::GetInputSlotState(xiiStringView sSlot) const
{
  return m_InputSlotValues.GetValueOrDefault(sSlot, 0.f);
}

bool xiiInputDevice::HasDeviceBeenUsedLastFrame() const
{
  return m_bGeneratedInputRecently;
}

XII_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);
