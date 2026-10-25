/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <DearImguiPlugin/DearImguiInput.h>
#include <DearImguiPlugin/Implementation/DearImguiContextData.h>

namespace
{
  struct KeyBinding
  {
    const char* m_szSlot;
    ImGuiKey    m_Key;
  };
  static constexpr KeyBinding s_KeyBindings[] = {
    {xiiInputSlot_KeyLeft, ImGuiKey_LeftArrow},
    {xiiInputSlot_KeyRight, ImGuiKey_RightArrow},
    {xiiInputSlot_KeyUp, ImGuiKey_UpArrow},
    {xiiInputSlot_KeyDown, ImGuiKey_DownArrow},
    {xiiInputSlot_KeyTab, ImGuiKey_Tab},
    {xiiInputSlot_KeySpace, ImGuiKey_Space},
    {xiiInputSlot_KeyEscape, ImGuiKey_Escape},
    {xiiInputSlot_KeyBackspace, ImGuiKey_Backspace},
    {xiiInputSlot_KeyReturn, ImGuiKey_Enter},
    {xiiInputSlot_KeyInsert, ImGuiKey_Insert},
    {xiiInputSlot_KeyDelete, ImGuiKey_Delete},
    {xiiInputSlot_KeyHome, ImGuiKey_Home},
    {xiiInputSlot_KeyEnd, ImGuiKey_End},
    {xiiInputSlot_KeyPageUp, ImGuiKey_PageUp},
    {xiiInputSlot_KeyPageDown, ImGuiKey_PageDown},
    {xiiInputSlot_KeyLeftCtrl, ImGuiKey_LeftCtrl},
    {xiiInputSlot_KeyRightCtrl, ImGuiKey_RightCtrl},
    {xiiInputSlot_KeyLeftShift, ImGuiKey_LeftShift},
    {xiiInputSlot_KeyRightShift, ImGuiKey_RightShift},
    {xiiInputSlot_KeyLeftAlt, ImGuiKey_LeftAlt},
    {xiiInputSlot_KeyRightAlt, ImGuiKey_RightAlt},
    {xiiInputSlot_KeyLeftWin, ImGuiKey_LeftSuper},
    {xiiInputSlot_KeyRightWin, ImGuiKey_RightSuper},
    {xiiInputSlot_KeyBracketOpen, ImGuiKey_LeftBracket},
    {xiiInputSlot_KeyBracketClose, ImGuiKey_RightBracket},
    {xiiInputSlot_KeySemicolon, ImGuiKey_Semicolon},
    {xiiInputSlot_KeyApostrophe, ImGuiKey_Apostrophe},
    {xiiInputSlot_KeySlash, ImGuiKey_Slash},
    {xiiInputSlot_KeyBackslash, ImGuiKey_Backslash},
    {xiiInputSlot_KeyEquals, ImGuiKey_Equal},
    {xiiInputSlot_KeyHyphen, ImGuiKey_Minus},
    {xiiInputSlot_KeyComma, ImGuiKey_Comma},
    {xiiInputSlot_KeyPeriod, ImGuiKey_Period},
    {xiiInputSlot_KeyTilde, ImGuiKey_GraveAccent},
    {xiiInputSlot_KeyCapsLock, ImGuiKey_CapsLock},
    {xiiInputSlot_KeyNumLock, ImGuiKey_NumLock},
    {xiiInputSlot_KeyScroll, ImGuiKey_ScrollLock},
    {xiiInputSlot_KeyPrint, ImGuiKey_PrintScreen},
    {xiiInputSlot_KeyPause, ImGuiKey_Pause},
    {xiiInputSlot_KeyApps, ImGuiKey_Menu},
    {xiiInputSlot_KeyNumpadPlus, ImGuiKey_KeypadAdd},
    {xiiInputSlot_KeyNumpadMinus, ImGuiKey_KeypadSubtract},
    {xiiInputSlot_KeyNumpadStar, ImGuiKey_KeypadMultiply},
    {xiiInputSlot_KeyNumpadSlash, ImGuiKey_KeypadDivide},
    {xiiInputSlot_KeyNumpadPeriod, ImGuiKey_KeypadDecimal},
    {xiiInputSlot_KeyNumpadEnter, ImGuiKey_KeypadEnter},
    {xiiInputSlot_KeyA, ImGuiKey_A},
    {xiiInputSlot_KeyB, ImGuiKey_B},
    {xiiInputSlot_KeyC, ImGuiKey_C},
    {xiiInputSlot_KeyD, ImGuiKey_D},
    {xiiInputSlot_KeyE, ImGuiKey_E},
    {xiiInputSlot_KeyF, ImGuiKey_F},
    {xiiInputSlot_KeyG, ImGuiKey_G},
    {xiiInputSlot_KeyH, ImGuiKey_H},
    {xiiInputSlot_KeyI, ImGuiKey_I},
    {xiiInputSlot_KeyJ, ImGuiKey_J},
    {xiiInputSlot_KeyK, ImGuiKey_K},
    {xiiInputSlot_KeyL, ImGuiKey_L},
    {xiiInputSlot_KeyM, ImGuiKey_M},
    {xiiInputSlot_KeyN, ImGuiKey_N},
    {xiiInputSlot_KeyO, ImGuiKey_O},
    {xiiInputSlot_KeyP, ImGuiKey_P},
    {xiiInputSlot_KeyQ, ImGuiKey_Q},
    {xiiInputSlot_KeyR, ImGuiKey_R},
    {xiiInputSlot_KeyS, ImGuiKey_S},
    {xiiInputSlot_KeyT, ImGuiKey_T},
    {xiiInputSlot_KeyU, ImGuiKey_U},
    {xiiInputSlot_KeyV, ImGuiKey_V},
    {xiiInputSlot_KeyW, ImGuiKey_W},
    {xiiInputSlot_KeyX, ImGuiKey_X},
    {xiiInputSlot_KeyY, ImGuiKey_Y},
    {xiiInputSlot_KeyZ, ImGuiKey_Z},
    {xiiInputSlot_Key0, ImGuiKey_0},
    {xiiInputSlot_KeyNumpad0, ImGuiKey_Keypad0},
    {xiiInputSlot_Key1, ImGuiKey_1},
    {xiiInputSlot_KeyNumpad1, ImGuiKey_Keypad1},
    {xiiInputSlot_Key2, ImGuiKey_2},
    {xiiInputSlot_KeyNumpad2, ImGuiKey_Keypad2},
    {xiiInputSlot_Key3, ImGuiKey_3},
    {xiiInputSlot_KeyNumpad3, ImGuiKey_Keypad3},
    {xiiInputSlot_Key4, ImGuiKey_4},
    {xiiInputSlot_KeyNumpad4, ImGuiKey_Keypad4},
    {xiiInputSlot_Key5, ImGuiKey_5},
    {xiiInputSlot_KeyNumpad5, ImGuiKey_Keypad5},
    {xiiInputSlot_Key6, ImGuiKey_6},
    {xiiInputSlot_KeyNumpad6, ImGuiKey_Keypad6},
    {xiiInputSlot_Key7, ImGuiKey_7},
    {xiiInputSlot_KeyNumpad7, ImGuiKey_Keypad7},
    {xiiInputSlot_Key8, ImGuiKey_8},
    {xiiInputSlot_KeyNumpad8, ImGuiKey_Keypad8},
    {xiiInputSlot_Key9, ImGuiKey_9},
    {xiiInputSlot_KeyNumpad9, ImGuiKey_Keypad9},
    {xiiInputSlot_KeyF1, ImGuiKey_F1},
    {xiiInputSlot_KeyF2, ImGuiKey_F2},
    {xiiInputSlot_KeyF3, ImGuiKey_F3},
    {xiiInputSlot_KeyF4, ImGuiKey_F4},
    {xiiInputSlot_KeyF5, ImGuiKey_F5},
    {xiiInputSlot_KeyF6, ImGuiKey_F6},
    {xiiInputSlot_KeyF7, ImGuiKey_F7},
    {xiiInputSlot_KeyF8, ImGuiKey_F8},
    {xiiInputSlot_KeyF9, ImGuiKey_F9},
    {xiiInputSlot_KeyF10, ImGuiKey_F10},
    {xiiInputSlot_KeyF11, ImGuiKey_F11},
    {xiiInputSlot_KeyF12, ImGuiKey_F12},
  };
} // namespace

xiiResult xiiImguiInput::InputSlot(xiiImguiContextHandle hContext, const xiiInputDevice& source, xiiStringView sSlot, float fValue)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || !xiiMath::IsFinite(fValue))
    return XII_FAILURE;
  if (!scope.m_pContext->m_bInputEnabled)
    return XII_SUCCESS;
  auto&                 io        = ImGui::GetIO();
  constexpr const char* buttons[] = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2, xiiInputSlot_MouseButton3, xiiInputSlot_MouseButton4};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(buttons); ++i)
    if (sSlot == buttons[i])
    {
      io.AddMousePosEvent(source.GetInputSlotState(xiiInputSlot_MousePositionX) * io.DisplaySize.x, source.GetInputSlotState(xiiInputSlot_MousePositionY) * io.DisplaySize.y);
      io.AddMouseButtonEvent(static_cast<int>(i), fValue > 0);
      return XII_SUCCESS;
    }
  if (sSlot == xiiInputSlot_MouseWheelUp || sSlot == xiiInputSlot_MouseWheelDown)
  {
    io.AddMouseWheelEvent(0, sSlot == xiiInputSlot_MouseWheelUp ? fValue : -fValue);
    return XII_SUCCESS;
  }
  for (const auto& key : s_KeyBindings)
    if (sSlot == key.m_szSlot)
    {
      io.AddKeyEvent(key.m_Key, fValue > 0);
      io.AddKeyEvent(ImGuiMod_Ctrl, source.GetInputSlotState(xiiInputSlot_KeyLeftCtrl) > 0 || source.GetInputSlotState(xiiInputSlot_KeyRightCtrl) > 0);
      io.AddKeyEvent(ImGuiMod_Shift, source.GetInputSlotState(xiiInputSlot_KeyLeftShift) > 0 || source.GetInputSlotState(xiiInputSlot_KeyRightShift) > 0);
      io.AddKeyEvent(ImGuiMod_Alt, source.GetInputSlotState(xiiInputSlot_KeyLeftAlt) > 0 || source.GetInputSlotState(xiiInputSlot_KeyRightAlt) > 0);
      io.AddKeyEvent(ImGuiMod_Super, source.GetInputSlotState(xiiInputSlot_KeyLeftWin) > 0 || source.GetInputSlotState(xiiInputSlot_KeyRightWin) > 0);
      break;
    }
  return XII_SUCCESS;
}

bool xiiImguiInput::CanSubmit(const xiiImguiScopedContext& scope)
{
  return scope.m_pContext && !scope.m_pContext->m_bFrameActive;
}

xiiResult xiiImguiInput::MousePosition(xiiImguiContextHandle hContext, const xiiVec2& position)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || !position.IsValid())
    return XII_FAILURE;
  if (scope.m_pContext->m_bInputEnabled)
    ImGui::GetIO().AddMousePosEvent(position.x, position.y);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::MouseButton(xiiImguiContextHandle hContext, xiiUInt32 uiButton, bool bDown)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || uiButton >= 5)
    return XII_FAILURE;
  if (scope.m_pContext->m_bInputEnabled)
    ImGui::GetIO().AddMouseButtonEvent(static_cast<int>(uiButton), bDown);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::MouseWheel(xiiImguiContextHandle hContext, const xiiVec2& delta)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || !delta.IsValid())
    return XII_FAILURE;
  if (scope.m_pContext->m_bInputEnabled)
    ImGui::GetIO().AddMouseWheelEvent(delta.x, delta.y);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::Key(xiiImguiContextHandle hContext, ImGuiKey key, bool bDown)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || !((key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END) || key == ImGuiMod_Ctrl || key == ImGuiMod_Shift || key == ImGuiMod_Alt || key == ImGuiMod_Super))
    return XII_FAILURE;
  if (scope.m_pContext->m_bInputEnabled)
    ImGui::GetIO().AddKeyEvent(key, bDown);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::Text(xiiImguiContextHandle hContext, xiiUInt32 uiCodepoint)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || uiCodepoint > 0x10FFFF || (uiCodepoint >= 0xD800 && uiCodepoint <= 0xDFFF))
    return XII_FAILURE;
  if (scope.m_pContext->m_bInputEnabled && uiCodepoint)
    ImGui::GetIO().AddInputCharacter(uiCodepoint);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::Focus(xiiImguiContextHandle hContext, bool bFocused)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope))
    return XII_FAILURE;
  ImGui::GetIO().AddFocusEvent(bFocused);
  return XII_SUCCESS;
}

xiiResult xiiImguiInput::UpdateFromDevice(xiiImguiContextHandle hContext, xiiInputDeviceMouseKeyboard& device, const xiiVec2& vLogicalSize)
{
  xiiImguiScopedContext scope(hContext);
  if (!CanSubmit(scope) || !vLogicalSize.IsValid() || vLogicalSize.x < 0 || vLogicalSize.y < 0)
    return XII_FAILURE;
  auto& io = ImGui::GetIO();
  io.AddFocusEvent(device.IsFocused());
  if (!scope.m_pContext->m_bInputEnabled || !device.IsFocused())
    return XII_SUCCESS;
  io.AddMousePosEvent(device.GetInputSlotState(xiiInputSlot_MousePositionX) * vLogicalSize.x, device.GetInputSlotState(xiiInputSlot_MousePositionY) * vLogicalSize.y);
  constexpr const char* buttons[] = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2, xiiInputSlot_MouseButton3, xiiInputSlot_MouseButton4};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(buttons); ++i)
    io.AddMouseButtonEvent(static_cast<int>(i), device.GetInputSlotState(buttons[i]) > 0);
  for (const auto& key : s_KeyBindings)
    io.AddKeyEvent(key.m_Key, device.GetInputSlotState(key.m_szSlot) > 0);
  io.AddKeyEvent(ImGuiMod_Ctrl, device.GetInputSlotState(xiiInputSlot_KeyLeftCtrl) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightCtrl) > 0);
  io.AddKeyEvent(ImGuiMod_Shift, device.GetInputSlotState(xiiInputSlot_KeyLeftShift) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightShift) > 0);
  io.AddKeyEvent(ImGuiMod_Alt, device.GetInputSlotState(xiiInputSlot_KeyLeftAlt) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightAlt) > 0);
  io.AddKeyEvent(ImGuiMod_Super, device.GetInputSlotState(xiiInputSlot_KeyLeftWin) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightWin) > 0);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiInput);
