/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#if XII_DISABLED(XII_SUPPORTS_SDL)

#  include <Core/Platform/NoImpl/InputDevice_NoImpl.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber) {}
xiiStandardInputDevice::~xiiStandardInputDevice() = default;

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow) {}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return false;
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) {}

xiiMouseCursorClipMode::Enum xiiStandardInputDevice::GetClipMouseCursor() const
{
  return xiiMouseCursorClipMode::Default;
}

void xiiStandardInputDevice::InitializeDevice() {}

void xiiStandardInputDevice::RegisterInputSlots() {}

#endif
