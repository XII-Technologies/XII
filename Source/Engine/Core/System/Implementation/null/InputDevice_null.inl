#include <Core/System/Implementation/null/InputDevice_null.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber) {}
xiiStandardInputDevice::~xiiStandardInputDevice() = default;

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow) {}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) {}

xiiMouseCursorClipMode::Enum xiiStandardInputDevice::GetClipMouseCursor() const
{
  return xiiMouseCursorClipMode::Default;
}

void xiiStandardInputDevice::InitializeDevice() {}

void xiiStandardInputDevice::RegisterInputSlots() {}
