/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Math/Vec2.h>

/// Specifies how to restrict movement of the Operating System mouse
struct xiiMouseCursorClipMode
{
  enum Enum
  {
    NoClip,                ///< The mouse can move unrestricted and leave the application window
    ClipToWindow,          ///< The mouse cannot leave the window area anymore after the user started interacting with it (ie. clicks into the window).
    ClipToWindowImmediate, ///< The mouse gets restricted to the window area as soon as possible
    ClipToPosition,        ///< The mouse may not leave its current position. Can be used to keep the mouse in place while it is hidden. Note that you will still get mouse move deltas, just the OS cursor will stay in place.

    Default = NoClip,
  };
};

/// This is the base class for all input devices that handle mouse and keyboard input.
///
/// This class is derived from xiiInputDevice but adds interface functions to handle mouse and keyboard input.
class XII_CORE_DLL xiiInputDeviceMouseKeyboard : public xiiInputDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInputDeviceMouseKeyboard, xiiInputDevice);

public:
  xiiInputDeviceMouseKeyboard() { m_vMouseScale.Set(1.0f); }

  /// Shows or hides the mouse cursor inside the application window.
  virtual void SetShowMouseCursor(bool bShow) = 0;

  /// Returns whether the mouse cursor is shown.
  virtual bool GetShowMouseCursor() const = 0;

  /// Will trap the mouse inside the application window. Should usually be enabled, to prevent accidental task switches.
  ///
  /// Especially on multi-monitor systems, the mouse can easily leave the application window (even in fullscreen mode).
  /// Do NOT use this function when you have multiple windows and require absolute mouse positions.
  ///
  /// \sa xiiMouseCursorClipMode
  virtual void SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) = 0;

  /// Returns whether the mouse is confined to the application window or not.
  virtual xiiMouseCursorClipMode::Enum GetClipMouseCursor() const = 0;

  /// Sets the scaling factor that is applied on all (relative) mouse input.
  virtual void SetMouseSpeed(const xiiVec2& vScale) { m_vMouseScale = vScale; }

  /// Returns the scaling factor that is applied on all (relative) mouse input.
  xiiVec2 GetMouseSpeed() const { return m_vMouseScale; }

  /// Returns the number of the xiiWindow over which the mouse moved last.
  static xiiInt32 GetWindowNumberMouseIsOver() { return s_iMouseIsOverWindowNumber; }

  /// Returns if the associated xiiWindow has focus
  bool IsFocused() { return m_bIsFocused; }

protected:
  virtual void UpdateInputSlotValues() override;

  xiiTime         m_DoubleClickTime = xiiTime::MakeFromMilliseconds(500);
  static xiiInt32 s_iMouseIsOverWindowNumber;

private:
  xiiVec2 m_vMouseScale;

  bool m_bIsFocused = true;

  xiiTime m_LastMouseClick[3];
  bool    m_bMouseDown[3] = {false, false, false};
};
