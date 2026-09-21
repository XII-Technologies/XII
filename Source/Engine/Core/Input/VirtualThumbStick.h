/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Vec2.h>

/// A Virtual Thumb-stick is an input device that transforms certain types of input (mouse / touch) into input similar to a
/// thumb-stick on a controller.
///
/// A virtual thumb-stick can be used to provide an 'input device' on a touch screen, that acts like a controller thumb-stick and thus
/// allows easier control over a game. The virtual thumb-stick takes input inside a certain screen area. It tracks the users finger
/// movements inside this area and translates those into input from a controller thumb-stick, which it then feeds back into the input
/// system. That makes it then possible to be mapped to input actions again. This way a game controller type of input is emulated.
class XII_CORE_DLL xiiVirtualThumbStick final : public xiiInputDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualThumbStick, xiiInputDevice);

public:
  /// Constructor.
  xiiVirtualThumbStick();

  /// Destructor.
  ~xiiVirtualThumbStick();

  /// This enum allows to select either some default input mapping or to select 'Custom'.
  struct Input
  {
    enum Enum
    {
      Touchpoint,    ///< The Virtual Thumb-stick will be triggered by touch input events.
      MousePosition, ///< The Virtual Thumb-stick will be triggered by mouse input.
      Custom         ///< The Thumb-stick triggers are specified manually.
    };
  };

  /// Specifies which type of output the thumb-stick shall generate.
  struct Output
  {
    enum Enum
    {
      Controller0_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 0.
      Controller0_RightStick, ///< The Thumb-stick acts like the right stick of controller 0.
      Controller1_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 1.
      Controller1_RightStick, ///< The Thumb-stick acts like the right stick of controller 1.
      Controller2_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 2.
      Controller2_RightStick, ///< The Thumb-stick acts like the right stick of controller 2.
      Controller3_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 3.
      Controller3_RightStick, ///< The Thumb-stick acts like the right stick of controller 3.
      Custom                  ///< The thumb-stick output is specified manually.
    };
  };

  /// Defines whether the thumb-stick center position is locked or relative to where the user started touching it.
  struct CenterMode
  {
    enum Enum
    {
      InputArea,       ///< The center of the thumb-stick is always at the center of the input area.
      ActivationPoint, ///< The center of the thumb-stick is always where the user activates the thumb-stick (first touch-point)
      Swipe,           ///< The center follows the touch-point with a short time delay, thus a swipe at any position is a temporary direction input.
    };
  };

  struct Flags
  {
    using StorageType = xiiUInt16;

    enum Enum
    {
      None        = 0,
      OnlyMaxAxis = XII_BIT(0), ///< If set, only the output axis that has the strongest value will be set. Thus the stick acts more like a DPAD with 4 distinct directions where only one will be active at any one time.

      Default = None,
    };

    struct Bits
    {
      StorageType OnlyMaxAxis : 1;
    };
  };

  /// Defines the area on screen where the thumb-stick is located and accepts input.
  ///
  /// \param vLowerLeft
  ///   The lower left corner of the input area. Coordinates are in [0; 1] range (normalized screen coordinates).
  ///
  /// \param vUpperRight
  ///   The upper right corner of the input area. Coordinates are in [0; 1] range (normalized screen coordinates).
  ///
  /// \param fThumbstickRadius
  ///   The distance to move the touch point to create a maximum input value (1.0).
  ///   With a larger radius, users have to move the finger farther for full input strength.
  ///   Note that the radius is also in [0; 1] range (normalized screen coordinates).
  ///
  /// \param fPriority
  ///   The priority of the input area. Defines which thumb-stick or other input action gets priority, if they overlap.
  ///
  /// \param center
  ///   \sa CenterMode.
  void SetInputArea(const xiiVec2& vLowerLeft, const xiiVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center = CenterMode::ActivationPoint);

  /// See the Flags struct for details.
  void SetFlags(xiiBitflags<Flags> flags);

  /// See the Flags struct for details.
  xiiBitflags<Flags> GetFlags() const { return m_Flags; }

  /// Sets the aspect ratio of the screen on which the input happens.
  ///
  /// Mouse and touch input coordinates are in normalized [0; 1] coordinate space.
  /// To calculate correct input values, the aspect ratio of the screen is needed (width divided by height).
  /// Call this when the screen resolution is known. Without the correct aspect ratio, moving the finger left/right a given distance
  /// won't have the same influence as moving it up/down the same distance.
  void SetInputCoordinateAspectRatio(float fWidthDivHeight);

  /// Returns the screen aspect ratio that was set. See SetInputCoordinateAspectRatio().
  float GetInputCoordinateAspectRatio() const { return m_fAspectRatio; }

  /// Returns the input area of the virtual thumb-stick.
  void GetInputArea(xiiVec2& out_vLowerLeft, xiiVec2& out_vUpperRight) const;

  /// Specifies from which input slots the thumb-stick is activated.
  ///
  /// If \a Input is 'Custom' the remaining parameters define the filter axes and up to three input slots that trigger the thumb-stick.
  /// Otherwise the remaining parameters are ignored.
  void SetTriggerInputSlot(Input::Enum input, const xiiInputActionConfig* pCustomConfig = nullptr);

  /// Specifies which output the thumb-stick generates.
  ///
  /// If \a Output is 'Custom' the remaining parameters define which input slots the thumb-stick triggers for which direction.
  /// Otherwise the remaining parameters are ignored.
  void SetThumbstickOutput(Output::Enum output, xiiStringView sOutputLeft = {}, xiiStringView sOutputRight = {}, xiiStringView sOutputUp = {}, xiiStringView sOutputDown = {});

  /// Specifies what happens when the input slots that trigger the thumb-stick are active while entering or leaving the input area.
  void SetAreaFocusMode(xiiInputActionConfig::OnEnterArea onEnter, xiiInputActionConfig::OnLeaveArea onLeave);

  /// Allows to enable or disable the entire thumb-stick temporarily.
  void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }

  /// Returns whether the thumb-stick is currently enabled.
  bool IsEnabled() const { return m_bEnabled; }

  /// Returns whether the thumb-stick is currently active (ie. triggered) and generates output.
  bool IsActive() const { return m_bIsActive; }

  /// Returns the (normalized screen) coordinate where the current input center is. Depends on CenterMode.
  xiiVec2 GetCurrentCenter() const { return m_vCenter; }

  /// See SetInputArea() for details.
  float GetThumbstickRadius() const { return m_fRadius; }

  /// Returns the (normalized screen) coordinate where the current touch point is.
  xiiVec2 GetCurrentTouchPos() const { return m_vTouchPos; }

  /// Returns the total strength of input.
  float GetInputStrength() const { return m_fInputStrength; }

  /// Returns the normalized direction of the input.
  xiiVec2 GetInputDirection() const { return m_vInputDirection; }

protected:
  void UpdateActionMapping();

  xiiVec2 m_vLowerLeft  = xiiVec2::MakeZero();
  xiiVec2 m_vUpperRight = xiiVec2::MakeZero();
  float   m_fRadius     = 0.0f;

  xiiInputActionConfig m_ActionConfig;
  xiiStringView        m_sOutputLeft;
  xiiStringView        m_sOutputRight;
  xiiStringView        m_sOutputUp;
  xiiStringView        m_sOutputDown;

  xiiBitflags<Flags> m_Flags;
  bool               m_bEnabled       = false;
  bool               m_bConfigChanged = false;
  bool               m_bIsActive      = false;
  xiiString          m_sName;
  xiiVec2            m_vCenter         = xiiVec2::MakeZero();
  xiiVec2            m_vTouchPos       = xiiVec2::MakeZero();
  xiiVec2            m_vInputDirection = xiiVec2::MakeZero();
  float              m_fInputStrength  = 0.0f;
  float              m_fAspectRatio    = 1.0f;
  CenterMode::Enum   m_CenterMode;

  static xiiInt32 s_iThumbsticks;

private:
  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
};
