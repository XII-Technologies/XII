/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <DearImguiPlugin/DearImguiInput.h>
#include <DearImguiPlugin/Implementation/DearImguiContextData.h>

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
  io.AddMouseWheelEvent(0, device.GetInputSlotState(xiiInputSlot_MouseWheelUp) - device.GetInputSlotState(xiiInputSlot_MouseWheelDown));
  struct KeyBinding
  {
    const char* m_szSlot;
    ImGuiKey    m_Key;
  };
  static constexpr KeyBinding keys[] = {
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
  };
  for (const auto& key : keys)
    io.AddKeyEvent(key.m_Key, device.GetInputSlotState(key.m_szSlot) > 0);
  // These XII slot families and ImGui key families are both contiguous in spelling/order.
  xiiStringBuilder slot;
  for (int i = 0; i < 26; ++i)
  {
    slot.SetFormat("keyboard_{0}", char('a' + i));
    io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + i), device.GetInputSlotState(slot) > 0);
  }
  for (int i = 0; i < 10; ++i)
  {
    slot.SetFormat("keyboard_{0}", i);
    io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_0 + i), device.GetInputSlotState(slot) > 0);
    slot.SetFormat("keyboard_numpad_{0}", i);
    io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + i), device.GetInputSlotState(slot) > 0);
  }
  for (int i = 0; i < 12; ++i)
  {
    slot.SetFormat("keyboard_f{0}", i + 1);
    io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_F1 + i), device.GetInputSlotState(slot) > 0);
  }
  io.AddKeyEvent(ImGuiMod_Ctrl, device.GetInputSlotState(xiiInputSlot_KeyLeftCtrl) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightCtrl) > 0);
  io.AddKeyEvent(ImGuiMod_Shift, device.GetInputSlotState(xiiInputSlot_KeyLeftShift) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightShift) > 0);
  io.AddKeyEvent(ImGuiMod_Alt, device.GetInputSlotState(xiiInputSlot_KeyLeftAlt) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightAlt) > 0);
  io.AddKeyEvent(ImGuiMod_Super, device.GetInputSlotState(xiiInputSlot_KeyLeftWin) > 0 || device.GetInputSlotState(xiiInputSlot_KeyRightWin) > 0);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiInput);
