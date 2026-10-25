/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiContext.h>
#include <DearImguiPlugin/DearImguiPluginDLL.h>
#include <Foundation/Strings/StringView.h>

class xiiInputDeviceMouseKeyboard;
class xiiInputDevice;

/// Route only events from the context's own host window, before BeginFrame. Never reads merged global input.
/// Event methods support editor/Qt hosts; UpdateFromDevice supports XII windows. Text is explicitly supplied
/// by subscribing to the source device's GetTextInputEvents(), preserving Unicode bursts and source identity.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiInput
{
public:
  static xiiResult MousePosition(xiiImguiContextHandle hContext, const xiiVec2& position);
  static xiiResult MouseButton(xiiImguiContextHandle hContext, xiiUInt32 uiButton, bool bDown);
  static xiiResult MouseWheel(xiiImguiContextHandle hContext, const xiiVec2& delta);
  static xiiResult Key(xiiImguiContextHandle hContext, ImGuiKey key, bool bDown);
  static xiiResult Text(xiiImguiContextHandle hContext, xiiUInt32 uiCodepoint);
  static xiiResult Focus(xiiImguiContextHandle hContext, bool bFocused);
  /// Subscribe to the source's GetInputSlotEvents() for lossless key/button/wheel transitions.
  /// Absolute mouse position is sampled before button events using the context's last display size.
  static xiiResult InputSlot(xiiImguiContextHandle hContext, const xiiInputDevice& source, xiiStringView sSlot, float fValue);

  /// Call after PollHardware and before InputManager::Update resets transient wheel/device values.
  /// The host must keep the device assigned to one context until it sends focus loss.
  static xiiResult UpdateFromDevice(xiiImguiContextHandle hContext, xiiInputDeviceMouseKeyboard& device, const xiiVec2& vLogicalSize);

private:
  static bool CanSubmit(const xiiImguiScopedContext& scope);
};
