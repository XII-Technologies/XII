/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Types/Bitflags.h>

/// This struct defines the different states a key can be in.
///        All keys always go through the states 'Pressed' and 'Released', even if they are active for only one frame.
///        A key is 'Down' when it is pressed for at least two frames. It is 'Up' when it is not pressed for at least two frames.
struct XII_CORE_DLL xiiKeyState
{
  enum Enum
  {
    Up,       ///< Key is not pressed at all.
    Released, ///< Key has just been released this frame.
    Pressed,  ///< Key has just been pressed down this frame.
    Down      ///< Key is pressed down for longer than one frame now.
  };

  /// Computes the new key state from a previous key state and whether it is currently pressed or not.
  static xiiKeyState::Enum GetNewKeyState(xiiKeyState::Enum prevState, bool bKeyDown);
};

// clang-format off
// off for the entire file

/// These flags are specified when registering an input slot (by a device), to define some capabilities and restrictions of the hardware.
///
/// By default you do not need to use these flags at all. However, when presenting the user with a list of 'possible' buttons to press to map to an
/// action, these flags can be used to filter out unwanted slots.
/// For example you can filter out mouse movements by requiring that the input slot must be pressable or may not represent any axis.
/// You an additionally also use the prefix of the input slot name, to filter out all touch input slots etc. if necessary.
struct xiiInputSlotFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None                      = 0,

    ReportsRelativeValues     = XII_BIT(0),  ///< The input slot reports delta values (e.g. a mouse move), instead of absolute values.
    ValueBinaryZeroOrOne      = XII_BIT(1),  ///< The input slot will either be zero or one. Used for all buttons and keys.
    ValueRangeZeroToOne       = XII_BIT(2),  ///< The input slot has analog values between zero and one. Used for analog axis like the xbox triggers or thumb-sticks.
    ValueRangeZeroToInf       = XII_BIT(3),  ///< The input slot has unbounded values larger than zero. Used for all absolute positions, such as the mouse position.
    Pressable                 = XII_BIT(4),  ///< The slot can be pressed (e.g. a key). This is not possible for an axis, such as the mouse or an analog stick.
    Holdable                  = XII_BIT(5),  ///< The user can hold down the key. Possible for buttons, but not for axes or for wheels such as the mouse wheel.
    HalfAxis                  = XII_BIT(6),  ///< The input slot represents one half of the actually possible data. Used for all axes (pos / neg mouse movement, thumb-sticks).
    FullAxis                  = XII_BIT(7),  ///< The input slot represents one full axis. Mostly used for devices that report absolute values, such as the mouse position or touch input positions (values between zero and one)
    RequiresDeadZone          = XII_BIT(8),  ///< The input slot represents hardware that should use a dead zone, otherwise it might fire prematurely. Mostly used on thumb-sticks and trigger buttons.
    ValuesAreNonContinuous    = XII_BIT(9),  ///< The values of the slot can jump around randomly, ie. the user can input arbitrary values, like the position on a touchpad
    ActivationDependsOnOthers = XII_BIT(10), ///< Whether this slot can be activated depends on whether certain other slots are active. This is the case for touch-points which are numbered depending on how many other touch-points are already active.
    NeverTimeScale            = XII_BIT(11), ///< When this flag is specified, data from the input slot will never be scaled by the input update time difference. Important for mouse deltas and such.


    // Some predefined sets of flags for the most common use cases
    IsButton                  =                         ValueBinaryZeroOrOne | Pressable | Holdable,
    IsMouseWheel              = ReportsRelativeValues | ValueRangeZeroToInf  | Pressable |            HalfAxis |                    NeverTimeScale,
    IsAnalogTrigger           =                         ValueRangeZeroToOne  | Pressable | Holdable | FullAxis | RequiresDeadZone,
    IsMouseAxisPosition       =                         ValueRangeZeroToOne  |                        FullAxis |                    NeverTimeScale,
    IsMouseAxisMove           = ReportsRelativeValues | ValueRangeZeroToInf  |                        HalfAxis |                    NeverTimeScale,
    IsAnalogStick             =                         ValueRangeZeroToOne  |             Holdable | HalfAxis | RequiresDeadZone,
    IsDoubleClick             =                         ValueBinaryZeroOrOne | Pressable |                                          NeverTimeScale,
    IsTouchPosition           =                         ValueRangeZeroToOne  |                        FullAxis |                    NeverTimeScale | ValuesAreNonContinuous,
    IsTouchPoint              =                         ValueBinaryZeroOrOne | Pressable | Holdable |                                                ActivationDependsOnOthers,
    IsDPad                    =                         ValueBinaryZeroOrOne | Pressable | Holdable | HalfAxis,
    IsTrackedValue            =                         ValueRangeZeroToInf  |                        HalfAxis |                    NeverTimeScale | ValuesAreNonContinuous,

    Default                   = None
  };

  struct Bits
  {
    StorageType ReportsRelativeValues     : 1;
    StorageType ValueBinaryZeroOrOne      : 1;
    StorageType ValueRangeZeroToOne       : 1;
    StorageType ValueRangeZeroToInf       : 1;
    StorageType Pressable                 : 1;
    StorageType Holdable                  : 1;
    StorageType HalfAxis                  : 1;
    StorageType FullAxis                  : 1;
    StorageType RequiresDeadZone          : 1;
    StorageType ValuesAreNonContinuous    : 1;
    StorageType ActivationDependsOnOthers : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiInputSlotFlags);

#define xiiInputSlot_None                  ""

//
// Touchpads
//

#define xiiInputSlot_TouchPoint0           "touchpoint_0"
#define xiiInputSlot_TouchPoint0_PositionX "touchpoint_0_position_x"
#define xiiInputSlot_TouchPoint0_PositionY "touchpoint_0_position_y"

#define xiiInputSlot_TouchPoint1           "touchpoint_1"
#define xiiInputSlot_TouchPoint1_PositionX "touchpoint_1_position_x"
#define xiiInputSlot_TouchPoint1_PositionY "touchpoint_1_position_y"

#define xiiInputSlot_TouchPoint2           "touchpoint_2"
#define xiiInputSlot_TouchPoint2_PositionX "touchpoint_2_position_x"
#define xiiInputSlot_TouchPoint2_PositionY "touchpoint_2_position_y"

#define xiiInputSlot_TouchPoint3           "touchpoint_3"
#define xiiInputSlot_TouchPoint3_PositionX "touchpoint_3_position_x"
#define xiiInputSlot_TouchPoint3_PositionY "touchpoint_3_position_y"

#define xiiInputSlot_TouchPoint4           "touchpoint_4"
#define xiiInputSlot_TouchPoint4_PositionX "touchpoint_4_position_x"
#define xiiInputSlot_TouchPoint4_PositionY "touchpoint_4_position_y"

#define xiiInputSlot_TouchPoint5           "touchpoint_5"
#define xiiInputSlot_TouchPoint5_PositionX "touchpoint_5_position_x"
#define xiiInputSlot_TouchPoint5_PositionY "touchpoint_5_position_y"

#define xiiInputSlot_TouchPoint6           "touchpoint_6"
#define xiiInputSlot_TouchPoint6_PositionX "touchpoint_6_position_x"
#define xiiInputSlot_TouchPoint6_PositionY "touchpoint_6_position_y"

#define xiiInputSlot_TouchPoint7           "touchpoint_7"
#define xiiInputSlot_TouchPoint7_PositionX "touchpoint_7_position_x"
#define xiiInputSlot_TouchPoint7_PositionY "touchpoint_7_position_y"

#define xiiInputSlot_TouchPoint8           "touchpoint_8"
#define xiiInputSlot_TouchPoint8_PositionX "touchpoint_8_position_x"
#define xiiInputSlot_TouchPoint8_PositionY "touchpoint_8_position_y"

#define xiiInputSlot_TouchPoint9           "touchpoint_9"
#define xiiInputSlot_TouchPoint9_PositionX "touchpoint_9_position_x"
#define xiiInputSlot_TouchPoint9_PositionY "touchpoint_9_position_y"

//
// Standard Controllers
//

#define xiiInputSlot_Controller0_ButtonA         "controller0_button_a"
#define xiiInputSlot_Controller0_ButtonB         "controller0_button_b"
#define xiiInputSlot_Controller0_ButtonX         "controller0_button_x"
#define xiiInputSlot_Controller0_ButtonY         "controller0_button_y"
#define xiiInputSlot_Controller0_ButtonStart     "controller0_button_start"
#define xiiInputSlot_Controller0_ButtonBack      "controller0_button_back"
#define xiiInputSlot_Controller0_LeftShoulder    "controller0_left_shoulder"
#define xiiInputSlot_Controller0_RightShoulder   "controller0_right_shoulder"
#define xiiInputSlot_Controller0_LeftTrigger     "controller0_left_trigger"
#define xiiInputSlot_Controller0_RightTrigger    "controller0_right_trigger"
#define xiiInputSlot_Controller0_PadUp           "controller0_pad_up"
#define xiiInputSlot_Controller0_PadDown         "controller0_pad_down"
#define xiiInputSlot_Controller0_PadLeft         "controller0_pad_left"
#define xiiInputSlot_Controller0_PadRight        "controller0_pad_right"
#define xiiInputSlot_Controller0_LeftStick       "controller0_left_stick"
#define xiiInputSlot_Controller0_RightStick      "controller0_right_stick"
#define xiiInputSlot_Controller0_LeftStick_NegX  "controller0_leftstick_negx"
#define xiiInputSlot_Controller0_LeftStick_PosX  "controller0_leftstick_posx"
#define xiiInputSlot_Controller0_LeftStick_NegY  "controller0_leftstick_negy"
#define xiiInputSlot_Controller0_LeftStick_PosY  "controller0_leftstick_posy"
#define xiiInputSlot_Controller0_RightStick_NegX "controller0_rightstick_negx"
#define xiiInputSlot_Controller0_RightStick_PosX "controller0_rightstick_posx"
#define xiiInputSlot_Controller0_RightStick_NegY "controller0_rightstick_negy"
#define xiiInputSlot_Controller0_RightStick_PosY "controller0_rightstick_posy"

#define xiiInputSlot_Controller1_ButtonA         "controller1_button_a"
#define xiiInputSlot_Controller1_ButtonB         "controller1_button_b"
#define xiiInputSlot_Controller1_ButtonX         "controller1_button_x"
#define xiiInputSlot_Controller1_ButtonY         "controller1_button_y"
#define xiiInputSlot_Controller1_ButtonStart     "controller1_button_start"
#define xiiInputSlot_Controller1_ButtonBack      "controller1_button_back"
#define xiiInputSlot_Controller1_LeftShoulder    "controller1_left_shoulder"
#define xiiInputSlot_Controller1_RightShoulder   "controller1_right_shoulder"
#define xiiInputSlot_Controller1_LeftTrigger     "controller1_left_trigger"
#define xiiInputSlot_Controller1_RightTrigger    "controller1_right_trigger"
#define xiiInputSlot_Controller1_PadUp           "controller1_pad_up"
#define xiiInputSlot_Controller1_PadDown         "controller1_pad_down"
#define xiiInputSlot_Controller1_PadLeft         "controller1_pad_left"
#define xiiInputSlot_Controller1_PadRight        "controller1_pad_right"
#define xiiInputSlot_Controller1_LeftStick       "controller1_left_stick"
#define xiiInputSlot_Controller1_RightStick      "controller1_right_stick"
#define xiiInputSlot_Controller1_LeftStick_NegX  "controller1_leftstick_negx"
#define xiiInputSlot_Controller1_LeftStick_PosX  "controller1_leftstick_posx"
#define xiiInputSlot_Controller1_LeftStick_NegY  "controller1_leftstick_negy"
#define xiiInputSlot_Controller1_LeftStick_PosY  "controller1_leftstick_posy"
#define xiiInputSlot_Controller1_RightStick_NegX "controller1_rightstick_negx"
#define xiiInputSlot_Controller1_RightStick_PosX "controller1_rightstick_posx"
#define xiiInputSlot_Controller1_RightStick_NegY "controller1_rightstick_negy"
#define xiiInputSlot_Controller1_RightStick_PosY "controller1_rightstick_posy"

#define xiiInputSlot_Controller2_ButtonA         "controller2_button_a"
#define xiiInputSlot_Controller2_ButtonB         "controller2_button_b"
#define xiiInputSlot_Controller2_ButtonX         "controller2_button_x"
#define xiiInputSlot_Controller2_ButtonY         "controller2_button_y"
#define xiiInputSlot_Controller2_ButtonStart     "controller2_button_start"
#define xiiInputSlot_Controller2_ButtonBack      "controller2_button_back"
#define xiiInputSlot_Controller2_LeftShoulder    "controller2_left_shoulder"
#define xiiInputSlot_Controller2_RightShoulder   "controller2_right_shoulder"
#define xiiInputSlot_Controller2_LeftTrigger     "controller2_left_trigger"
#define xiiInputSlot_Controller2_RightTrigger    "controller2_right_trigger"
#define xiiInputSlot_Controller2_PadUp           "controller2_pad_up"
#define xiiInputSlot_Controller2_PadDown         "controller2_pad_down"
#define xiiInputSlot_Controller2_PadLeft         "controller2_pad_left"
#define xiiInputSlot_Controller2_PadRight        "controller2_pad_right"
#define xiiInputSlot_Controller2_LeftStick       "controller2_left_stick"
#define xiiInputSlot_Controller2_RightStick      "controller2_right_stick"
#define xiiInputSlot_Controller2_LeftStick_NegX  "controller2_leftstick_negx"
#define xiiInputSlot_Controller2_LeftStick_PosX  "controller2_leftstick_posx"
#define xiiInputSlot_Controller2_LeftStick_NegY  "controller2_leftstick_negy"
#define xiiInputSlot_Controller2_LeftStick_PosY  "controller2_leftstick_posy"
#define xiiInputSlot_Controller2_RightStick_NegX "controller2_rightstick_negx"
#define xiiInputSlot_Controller2_RightStick_PosX "controller2_rightstick_posx"
#define xiiInputSlot_Controller2_RightStick_NegY "controller2_rightstick_negy"
#define xiiInputSlot_Controller2_RightStick_PosY "controller2_rightstick_posy"

#define xiiInputSlot_Controller3_ButtonA         "controller3_button_a"
#define xiiInputSlot_Controller3_ButtonB         "controller3_button_b"
#define xiiInputSlot_Controller3_ButtonX         "controller3_button_x"
#define xiiInputSlot_Controller3_ButtonY         "controller3_button_y"
#define xiiInputSlot_Controller3_ButtonStart     "controller3_button_start"
#define xiiInputSlot_Controller3_ButtonBack      "controller3_button_back"
#define xiiInputSlot_Controller3_LeftShoulder    "controller3_left_shoulder"
#define xiiInputSlot_Controller3_RightShoulder   "controller3_right_shoulder"
#define xiiInputSlot_Controller3_LeftTrigger     "controller3_left_trigger"
#define xiiInputSlot_Controller3_RightTrigger    "controller3_right_trigger"
#define xiiInputSlot_Controller3_PadUp           "controller3_pad_up"
#define xiiInputSlot_Controller3_PadDown         "controller3_pad_down"
#define xiiInputSlot_Controller3_PadLeft         "controller3_pad_left"
#define xiiInputSlot_Controller3_PadRight        "controller3_pad_right"
#define xiiInputSlot_Controller3_LeftStick       "controller3_left_stick"
#define xiiInputSlot_Controller3_RightStick      "controller3_right_stick"
#define xiiInputSlot_Controller3_LeftStick_NegX  "controller3_leftstick_negx"
#define xiiInputSlot_Controller3_LeftStick_PosX  "controller3_leftstick_posx"
#define xiiInputSlot_Controller3_LeftStick_NegY  "controller3_leftstick_negy"
#define xiiInputSlot_Controller3_LeftStick_PosY  "controller3_leftstick_posy"
#define xiiInputSlot_Controller3_RightStick_NegX "controller3_rightstick_negx"
#define xiiInputSlot_Controller3_RightStick_PosX "controller3_rightstick_posx"
#define xiiInputSlot_Controller3_RightStick_NegY "controller3_rightstick_negy"
#define xiiInputSlot_Controller3_RightStick_PosY "controller3_rightstick_posy"

//
// Keyboard
//

#define xiiInputSlot_KeyLeft           "keyboard_left"
#define xiiInputSlot_KeyRight          "keyboard_right"
#define xiiInputSlot_KeyUp             "keyboard_up"
#define xiiInputSlot_KeyDown           "keyboard_down"
#define xiiInputSlot_KeyEscape         "keyboard_escape"
#define xiiInputSlot_KeySpace          "keyboard_space"
#define xiiInputSlot_KeyBackspace      "keyboard_backspace"
#define xiiInputSlot_KeyReturn         "keyboard_return"
#define xiiInputSlot_KeyTab            "keyboard_tab"
#define xiiInputSlot_KeyLeftShift      "keyboard_left_shift"
#define xiiInputSlot_KeyRightShift     "keyboard_right_shift"
#define xiiInputSlot_KeyLeftCtrl       "keyboard_left_ctrl"
#define xiiInputSlot_KeyRightCtrl      "keyboard_right_ctrl"
#define xiiInputSlot_KeyLeftAlt        "keyboard_left_alt"
#define xiiInputSlot_KeyRightAlt       "keyboard_right_alt"
#define xiiInputSlot_KeyLeftWin        "keyboard_left_win"
#define xiiInputSlot_KeyRightWin       "keyboard_right_win"
#define xiiInputSlot_KeyBracketOpen    "keyboard_bracket_open"
#define xiiInputSlot_KeyBracketClose   "keyboard_bracket_close"
#define xiiInputSlot_KeySemicolon      "keyboard_semicolon"
#define xiiInputSlot_KeyApostrophe     "keyboard_apostrophe"
#define xiiInputSlot_KeySlash          "keyboard_slash"
#define xiiInputSlot_KeyEquals         "keyboard_equals"
#define xiiInputSlot_KeyTilde          "keyboard_tilde"
#define xiiInputSlot_KeyHyphen         "keyboard_hyphen"
#define xiiInputSlot_KeyComma          "keyboard_comma"
#define xiiInputSlot_KeyPeriod         "keyboard_period"
#define xiiInputSlot_KeyBackslash      "keyboard_backslash"
#define xiiInputSlot_KeyPipe           "keyboard_pipe"
#define xiiInputSlot_Key1              "keyboard_1"
#define xiiInputSlot_Key2              "keyboard_2"
#define xiiInputSlot_Key3              "keyboard_3"
#define xiiInputSlot_Key4              "keyboard_4"
#define xiiInputSlot_Key5              "keyboard_5"
#define xiiInputSlot_Key6              "keyboard_6"
#define xiiInputSlot_Key7              "keyboard_7"
#define xiiInputSlot_Key8              "keyboard_8"
#define xiiInputSlot_Key9              "keyboard_9"
#define xiiInputSlot_Key0              "keyboard_0"
#define xiiInputSlot_KeyNumpad1        "keyboard_numpad_1"
#define xiiInputSlot_KeyNumpad2        "keyboard_numpad_2"
#define xiiInputSlot_KeyNumpad3        "keyboard_numpad_3"
#define xiiInputSlot_KeyNumpad4        "keyboard_numpad_4"
#define xiiInputSlot_KeyNumpad5        "keyboard_numpad_5"
#define xiiInputSlot_KeyNumpad6        "keyboard_numpad_6"
#define xiiInputSlot_KeyNumpad7        "keyboard_numpad_7"
#define xiiInputSlot_KeyNumpad8        "keyboard_numpad_8"
#define xiiInputSlot_KeyNumpad9        "keyboard_numpad_9"
#define xiiInputSlot_KeyNumpad0        "keyboard_numpad_0"
#define xiiInputSlot_KeyA              "keyboard_a"
#define xiiInputSlot_KeyB              "keyboard_b"
#define xiiInputSlot_KeyC              "keyboard_c"
#define xiiInputSlot_KeyD              "keyboard_d"
#define xiiInputSlot_KeyE              "keyboard_e"
#define xiiInputSlot_KeyF              "keyboard_f"
#define xiiInputSlot_KeyG              "keyboard_g"
#define xiiInputSlot_KeyH              "keyboard_h"
#define xiiInputSlot_KeyI              "keyboard_i"
#define xiiInputSlot_KeyJ              "keyboard_j"
#define xiiInputSlot_KeyK              "keyboard_k"
#define xiiInputSlot_KeyL              "keyboard_l"
#define xiiInputSlot_KeyM              "keyboard_m"
#define xiiInputSlot_KeyN              "keyboard_n"
#define xiiInputSlot_KeyO              "keyboard_o"
#define xiiInputSlot_KeyP              "keyboard_p"
#define xiiInputSlot_KeyQ              "keyboard_q"
#define xiiInputSlot_KeyR              "keyboard_r"
#define xiiInputSlot_KeyS              "keyboard_s"
#define xiiInputSlot_KeyT              "keyboard_t"
#define xiiInputSlot_KeyU              "keyboard_u"
#define xiiInputSlot_KeyV              "keyboard_v"
#define xiiInputSlot_KeyW              "keyboard_w"
#define xiiInputSlot_KeyX              "keyboard_x"
#define xiiInputSlot_KeyY              "keyboard_y"
#define xiiInputSlot_KeyZ              "keyboard_z"
#define xiiInputSlot_KeyF1             "keyboard_f1"
#define xiiInputSlot_KeyF2             "keyboard_f2"
#define xiiInputSlot_KeyF3             "keyboard_f3"
#define xiiInputSlot_KeyF4             "keyboard_f4"
#define xiiInputSlot_KeyF5             "keyboard_f5"
#define xiiInputSlot_KeyF6             "keyboard_f6"
#define xiiInputSlot_KeyF7             "keyboard_f7"
#define xiiInputSlot_KeyF8             "keyboard_f8"
#define xiiInputSlot_KeyF9             "keyboard_f9"
#define xiiInputSlot_KeyF10            "keyboard_f10"
#define xiiInputSlot_KeyF11            "keyboard_f11"
#define xiiInputSlot_KeyF12            "keyboard_f12"
#define xiiInputSlot_KeyHome           "keyboard_home"
#define xiiInputSlot_KeyEnd            "keyboard_end"
#define xiiInputSlot_KeyDelete         "keyboard_delete"
#define xiiInputSlot_KeyInsert         "keyboard_insert"
#define xiiInputSlot_KeyPageUp         "keyboard_page_up"
#define xiiInputSlot_KeyPageDown       "keyboard_page_down"
#define xiiInputSlot_KeyNumLock        "keyboard_numlock"
#define xiiInputSlot_KeyNumpadPlus     "keyboard_numpad_plus"
#define xiiInputSlot_KeyNumpadMinus    "keyboard_numpad_minus"
#define xiiInputSlot_KeyNumpadStar     "keyboard_numpad_star"
#define xiiInputSlot_KeyNumpadSlash    "keyboard_numpad_slash"
#define xiiInputSlot_KeyNumpadPeriod   "keyboard_numpad_period"
#define xiiInputSlot_KeyNumpadEnter    "keyboard_numpad_enter"
#define xiiInputSlot_KeyCapsLock       "keyboard_capslock"
#define xiiInputSlot_KeyPrint          "keyboard_print"
#define xiiInputSlot_KeyScroll         "keyboard_scroll"
#define xiiInputSlot_KeyPause          "keyboard_pause"
#define xiiInputSlot_KeyApps           "keyboard_apps"
#define xiiInputSlot_KeyPrevTrack      "keyboard_prev_track"
#define xiiInputSlot_KeyNextTrack      "keyboard_next_track"
#define xiiInputSlot_KeyPlayPause      "keyboard_play_pause"
#define xiiInputSlot_KeyStop           "keyboard_stop"
#define xiiInputSlot_KeyVolumeUp       "keyboard_volume_up"
#define xiiInputSlot_KeyVolumeDown     "keyboard_volume_down"
#define xiiInputSlot_KeyMute           "keyboard_mute"

//
// Mouse
//

#define xiiInputSlot_MouseWheelUp      "mouse_wheel_up"
#define xiiInputSlot_MouseWheelDown    "mouse_wheel_down"
#define xiiInputSlot_MouseMoveNegX     "mouse_move_negx"
#define xiiInputSlot_MouseMovePosX     "mouse_move_posx"
#define xiiInputSlot_MouseMoveNegY     "mouse_move_negy"
#define xiiInputSlot_MouseMovePosY     "mouse_move_posy"
#define xiiInputSlot_MouseButton0      "mouse_button_0"
#define xiiInputSlot_MouseButton1      "mouse_button_1"
#define xiiInputSlot_MouseButton2      "mouse_button_2"
#define xiiInputSlot_MouseButton3      "mouse_button_3"
#define xiiInputSlot_MouseButton4      "mouse_button_4"
#define xiiInputSlot_MouseDblClick0    "mouse_button_0_doubleclick"
#define xiiInputSlot_MouseDblClick1    "mouse_button_1_doubleclick"
#define xiiInputSlot_MouseDblClick2    "mouse_button_2_doubleclick"
#define xiiInputSlot_MousePositionX    "mouse_position_x"
#define xiiInputSlot_MousePositionY    "mouse_position_y"

//
// Spatial Input Data (Tracked Hands or Controllers)
//

#define xiiInputSlot_Spatial_Hand0_Tracked "spatial_hand0_tracked"
#define xiiInputSlot_Spatial_Hand0_Pressed "spatial_hand0_pressed"
#define xiiInputSlot_Spatial_Hand0_PositionPosX "spatial_hand0_position_posx"
#define xiiInputSlot_Spatial_Hand0_PositionPosY "spatial_hand0_position_posy"
#define xiiInputSlot_Spatial_Hand0_PositionPosZ "spatial_hand0_position_posz"
#define xiiInputSlot_Spatial_Hand0_PositionNegX "spatial_hand0_position_negx"
#define xiiInputSlot_Spatial_Hand0_PositionNegY "spatial_hand0_position_negy"
#define xiiInputSlot_Spatial_Hand0_PositionNegZ "spatial_hand0_position_negz"

#define xiiInputSlot_Spatial_Hand1_Tracked "spatial_hand1_tracked"
#define xiiInputSlot_Spatial_Hand1_Pressed "spatial_hand1_pressed"
#define xiiInputSlot_Spatial_Hand1_PositionPosX "spatial_hand1_position_posx"
#define xiiInputSlot_Spatial_Hand1_PositionPosY "spatial_hand1_position_posy"
#define xiiInputSlot_Spatial_Hand1_PositionPosZ "spatial_hand1_position_posz"
#define xiiInputSlot_Spatial_Hand1_PositionNegX "spatial_hand1_position_negx"
#define xiiInputSlot_Spatial_Hand1_PositionNegY "spatial_hand1_position_negy"
#define xiiInputSlot_Spatial_Hand1_PositionNegZ "spatial_hand1_position_negz"

#define xiiInputSlot_Spatial_Head_PositionPosX "spatial_head_position_posx"
#define xiiInputSlot_Spatial_Head_PositionPosY "spatial_head_position_posy"
#define xiiInputSlot_Spatial_Head_PositionPosZ "spatial_head_position_posz"
#define xiiInputSlot_Spatial_Head_PositionNegX "spatial_head_position_negx"
#define xiiInputSlot_Spatial_Head_PositionNegY "spatial_head_position_negy"
#define xiiInputSlot_Spatial_Head_PositionNegZ "spatial_head_position_negz"

#define xiiInputSlot_Spatial_Head_ForwardPosX "spatial_head_forward_posx"
#define xiiInputSlot_Spatial_Head_ForwardPosY "spatial_head_forward_posy"
#define xiiInputSlot_Spatial_Head_ForwardPosZ "spatial_head_forward_posz"
#define xiiInputSlot_Spatial_Head_ForwardNegX "spatial_head_forward_negx"
#define xiiInputSlot_Spatial_Head_ForwardNegY "spatial_head_forward_negy"
#define xiiInputSlot_Spatial_Head_ForwardNegZ "spatial_head_forward_negz"

#define xiiInputSlot_Spatial_Head_UpPosX "spatial_head_up_posx"
#define xiiInputSlot_Spatial_Head_UpPosY "spatial_head_up_posy"
#define xiiInputSlot_Spatial_Head_UpPosZ "spatial_head_up_posz"
#define xiiInputSlot_Spatial_Head_UpNegX "spatial_head_up_negx"
#define xiiInputSlot_Spatial_Head_UpNegY "spatial_head_up_negy"
#define xiiInputSlot_Spatial_Head_UpNegZ "spatial_head_up_negz"
