/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>

/// A struct that defines how to register an input action.
struct XII_CORE_DLL xiiInputActionConfig
{
  /// Change this value to adjust how many input slots may trigger the same action.
  enum
  {
    MaxInputSlotAlternatives = 3
  };

  xiiInputActionConfig();

  /// If this is set to true, the value of the action is scaled by the time difference since the last input update. Default is true.
  ///
  /// You should enable this, if the value of the triggered action is used to modify how much to e.g. move or rotate something.
  /// For example, if an action 'RotateLeft' will rotate the player to the left, then he should rotate each frame an amount that is
  /// dependent on how much time has passed since the last update and by how much the button is pressed (e.g. a thumb-stick can be pressed
  /// only slightly). Mouse input, however, should not get scaled, because when the user moved the mouse one centimeter in the last frame,
  /// then that is an absolute movement and it does not depend on how much time elapsed. Therefore the xiiInputManager will take care NOT to
  /// scale input from such devices, whereas input from a thumb-stick or a keyboard key would be scaled by the elapsed time.
  ///
  /// However, if you for example use a thumb-stick to position something on screen (e.g. a cursor), then that action should NEVER be scaled
  /// by the elapsed time, because you are actually interested in the absolute value of the thumb-stick (and thus the action). In such cases
  /// you should disable time scaling.
  ///
  /// When you have an action where your are not interested in the value, only in whether it is triggered, at all, you can ignore time
  /// scaling altogether.
  bool m_bApplyTimeScaling;

  /// Which input slots will trigger this action.
  xiiString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  /// This scale is applied to the input slot value (before time scaling). Positive values mean a linear scaling, negative values an
  /// exponential scaling (i.e SlotValue = xiiMath::Pow(SlotValue, -ScaleValue)). Default is 1.0f.
  float m_fInputSlotScale[MaxInputSlotAlternatives];

  /// For Input Areas: If this is set, the input slot with the given name must have a value between m_fFilterXMinValue and
  /// m_fFilterXMaxValue. Otherwise this action will not be triggered.
  xiiString m_sFilterByInputSlotX[MaxInputSlotAlternatives];

  /// For Input Areas: If this is set, the input slot with the given name must have a value between m_fFilterYMinValue and
  /// m_fFilterYMaxValue. Otherwise this action will not be triggered.
  xiiString m_sFilterByInputSlotY[MaxInputSlotAlternatives];

  float m_fFilterXMinValue; ///< =0; see m_sFilterByInputSlotX
  float m_fFilterXMaxValue; ///< =1; see m_sFilterByInputSlotX
  float m_fFilterYMinValue; ///< =0; see m_sFilterByInputSlotY
  float m_fFilterYMaxValue; ///< =1; see m_sFilterByInputSlotY

  float m_fFilteredPriority; ///< =large negative value; For Input Areas: If two input actions overlap and they have different priorities,
                             ///< the one with the larger priority will be triggered. Otherwise both are triggered.

  /// For Input Areas: Describes what happens when an action is currently triggered, but the input slots used for filtering leave
  /// their min/max values.
  enum OnLeaveArea
  {
    LoseFocus, ///< The input action will lose focus and thus get 'deactivated' immediately. Ie. it will return xiiKeyState::Released.
    KeepFocus, ///< The input action will keep focus and continue to return xiiKeyState::Down, until all trigger slots are actually released.
  };

  /// For Input Areas: Describes what happens when any trigger slot is already active will the input slots that are used for
  /// filtering enter the valid ranges.
  enum OnEnterArea
  {
    ActivateImmediately, ///< The input action will immediately get activated and return xiiKeyState::Pressed, even though the input slots
                         ///< are already pressed for some time.
    RequireKeyUp,        ///< The input action will not get triggered, unless some trigger input slot is actually pressed while the input slots
                         ///< that are used for filtering are actually within valid ranges.
  };

  OnLeaveArea m_OnLeaveArea; ///< =LoseFocus
  OnEnterArea m_OnEnterArea; ///< =ActivateImmediately
};

/// The central class to set up and query the state of all input.
///
/// The xiiInputManager is the central hub through which you can configure which keys will trigger which actions. You can query in which
/// state an action is (inactive (up), active (down), just recently activated (pressed) or just recently deactivated (released)). You can
/// query their values (e.g. how much a thumb-stick or the mouse was moved). Additionally you can localize buttons and actions. The internal
/// data will always use English names and the US keyboard layout, but what with which names those keys are presented to the user can be
/// changed. Although the input manager allows to query the state of each key, button, axis, etc. directly, this is not advised. Instead the
/// user should set up 'actions' and define which keys will trigger those actions. At runtime the user should only query the state of
/// actions. In the best case, an application allows the player to change the mapping which keys are used to trigger which actions.
class XII_CORE_DLL xiiInputManager
{
public:
  /// Updates the state of the input manager. This should be called exactly once each frame.
  ///
  /// \param tTimeDifference The time elapsed since the last update. This will affect the value scaling of actions that
  /// use frame time scaling and is necessary to update controller vibration tracks.
  static void Update(xiiTime timeDifference); // [tested]

  /// Changes the display name of an input slot.
  static void SetInputSlotDisplayName(xiiStringView sInputSlot, xiiStringView sDefaultDisplayName); // [tested]

  /// Returns the display name that was assigned to the given input slot.
  static xiiStringView GetInputSlotDisplayName(xiiStringView sInputSlot); // [tested]

  /// A shortcut to get the display name of the input slot bound to a given action
  ///
  /// If iTrigger is set, the name of that trigger (0 .. xiiInputActionConfig::MaxInputSlotAlternatives) will be used.
  /// If iTrigger is less than 0, the first valid trigger is used.
  /// If iTrigger is outside the valid range or no valid trigger is bound, nullptr is returned.
  static xiiStringView GetInputSlotDisplayName(xiiStringView sInputSet, xiiStringView sAction, xiiInt32 iTrigger = -1);

  /// Sets the dead zone for the given input slot. As long as the hardware reports values lower than this, the input slot will report
  /// a value of zero.
  static void SetInputSlotDeadZone(xiiStringView sInputSlot, float fDeadZone); // [tested]

  /// Returns the dead zone value for the given input slot.
  static float GetInputSlotDeadZone(xiiStringView sInputSlot); // [tested]

  /// Returns the flags for the given input slot.
  static xiiBitflags<xiiInputSlotFlags> GetInputSlotFlags(xiiStringView sInputSlot); // [tested]

  /// Returns the current key state of the given input slot and optionally also returns its full value.
  ///
  /// Do not use this function, unless you really, really need the value of exactly this key.
  /// Prefer to map your key to an action and then use GetInputActionState(). That method is more robust and extensible.
  static xiiKeyState::Enum GetInputSlotState(xiiStringView sInputSlot, float* pValue = nullptr); // [tested]

  /// Returns an array that contains all the names of all currently known input slots.
  static void RetrieveAllKnownInputSlots(xiiDynamicArray<xiiStringView>& out_inputSlots);

  /// Returns the last typed character as the OS has reported it. Thus supports Unicode etc.
  ///
  /// If \a bResetCurrent is true, the internal last character will be reset to '\0'.
  /// If it is false, the internal state will not be changed. This should only be used, if the calling code does not do anything meaningful
  /// with the value.
  static xiiUInt32 RetrieveLastCharacter(bool bResetCurrent = true); // [tested]

  /// Makes sure that hardware input is processed at this moment, which allows to do this more often than Update() is called.
  ///
  /// When you have a game where you are doing relatively few game updates (including processing input), for example only 20 times
  /// per second, it is possible to 'miss' input. PollHardware() allows to introduce sampling the hardware state more often to prevent this.
  /// E.g. when your renderer renders at 60 Hz, you can poll input also at 60 Hz, even though you really only process it at 20 Hz.
  /// In typical usage scenarios this is not required to do and can be ignored.
  /// Note that you can call PollHardware() as often as you like and at irregular intervals, it will not have a negative effect
  /// on the input states.
  static void PollHardware();

  /// If \a szInputSlot is used in any action in \a szInputSet, it will be removed from all of them.
  ///
  /// This should be used to reset the usage of an input slot before it is bound to another input action.
  static void ClearInputMapping(xiiStringView sInputSet, xiiStringView sInputSlot); // [tested]

  /// This is the one function to set up which input actions are available and by which input slots (keys) they are triggered.
  ///
  /// \param szInputSet
  ///   'Input Sets' are sets of actions that are disjunct from each other. That means the same input slot (key, mouse button, etc.) can
  ///   trigger multiple different actions from different input sets. For example In the input set 'Game' the left mouse button may trigger
  ///   the action 'Shoot', but in the input set 'UI' the left mouse button may trigger the action 'Click'. All input sets are always
  ///   evaluated and update their state simultaneously. The user only has to decide which actions to react to, ie. whether the game is
  ///   currently running and thus the 'Game' input set is queried or whether a menu is shown and thus the 'UI' input set is queried.
  /// \param szAction
  ///   The action that is supposed to be triggered. The same action name may be reused in multiple input sets, they will have nothing in
  ///   common. The action name should describe WHAT is to be done, not which key the user pressed. For example an action could be
  ///   'player_forwards'. Which key is set to trigger that action should be irrelevant at run-time.
  /// \param Config
  ///   This struct defines exactly which input slots (keys, buttons etc.) will trigger this action. The configuration allows to scale key
  ///   values by the frame time, to get smooth movement when the frame-rate varies. It allows to only accept input from a slot if two other
  ///   slots have certain values. This makes it possible to react to mouse or touch input only if that input is done inside a certain input
  ///   area. The action can be triggered by multiple keys, if desired. In the most common cases, one will only set one or two input slots
  ///   as triggers (Config.m_sInputSlotTrigger) and possibly decide whether frame time scaling is required. It makes sense to let the
  ///   xiiInputManager do the frame time scaling, because it should not be applied to all input, e.g. mouse delta values should never be
  ///   scaled by the frame time.
  /// \param bClearPreviousInputMappings
  ///   If set to true it is ensured that all the input slots that are used by this action are not mapped to any other action.
  ///   That means no other action can be triggered by this key within this input set.
  ///   For most actions this should be set to true. However, if you have several actions that can be triggered by the same slot
  ///   (for example touch input) but only in different areas of the screen, this should be set to false.
  static void SetInputActionConfig(xiiStringView sInputSet, xiiStringView sAction, const xiiInputActionConfig& config, bool bClearPreviousInputMappings); // [tested]

  /// Returns the configuration for the given input action in the given input set. Returns a default configuration, if the action
  /// does not exist.
  static xiiInputActionConfig GetInputActionConfig(xiiStringView sInputSet, xiiStringView sAction); // [tested]

  /// Deletes all state associated with the given input action.
  ///
  /// It is not necessary to call this function for cleanup.
  static void RemoveInputAction(xiiStringView sInputSet, xiiStringView sAction); // [tested]

  /// Returns the current state and value of the given input action.
  ///
  /// This is the one function that is called repeatedly at runtime to figure out which actions are active and thus which game-play
  /// functions to execute. You can (and should) use the /a pValue to scale game play features (e.g. how fast to drive).
  static xiiKeyState::Enum GetInputActionState(xiiStringView sInputSet, xiiStringView sAction, float* pValue = nullptr, xiiInt8* pTriggeredSlot = nullptr); // [tested]

  /// Sets the display name for the given action.
  static void SetActionDisplayName(xiiStringView sAction, xiiStringView sDisplayName); // [tested]

  /// Returns the display name for the given action, or the action name itself, if no special display name was specified yet.
  static const xiiString GetActionDisplayName(xiiStringView sAction); // [tested]

  /// Returns the names of all currently registered input sets.
  static void GetAllInputSets(xiiDynamicArray<xiiString>& out_inputSetNames); // [tested]

  /// Returns the names of all input actions in the given input set.
  static void GetAllInputActions(xiiStringView sInputSetName, xiiDynamicArray<xiiString>& out_inputActions); // [tested]

  /// This can be used to pass input exclusively to this input set and no others.
  ///
  /// Querying input from other input sets will always return 'key up'.
  static void SetExclusiveInputSet(xiiStringView sExclusiveSet) { s_sExclusiveInputSet = sExclusiveSet; }

  /// Returns whether any input set gets input exclusively.
  static xiiStringView GetExclusiveInputSet() { return s_sExclusiveInputSet; }

  /// This function allows to 'inject' input state for one frame.
  ///
  /// This can be useful to emulate certain keys, e.g. for virtual devices.
  /// Note that it usually makes more sense to actually have another input device, however this can be used to
  /// get data into the system quickly for when a full blown input device might be overkill.
  /// The injected input state is cleared immediately after it has been processed, so to keep a virtual input slot active,
  /// the input needs to be injected every frame.
  ///
  /// Note that when the input is injected after xiiInputManager::Update was called, its effect will be delayed by one frame.
  static void InjectInputSlotValue(xiiStringView sInputSlot, float fValue); // [tested]

  /// Checks whether any input slot has been triggered in this frame, which has all \a MustHaveFlags and has none of the \a
  /// MustNotHaveFlags.
  ///
  /// This function can be used in a UI to wait for user input and then assign that input to a certain action.
  static xiiStringView GetPressedInputSlot(xiiInputSlotFlags::Enum mustHaveFlags, xiiInputSlotFlags::Enum mustNotHaveFlags); // [tested]

  /// Mostly for internal use. Converts a scan-code value to the string that is used inside the engine for that key.
  static xiiStringView ConvertScanCodeToEngineName(xiiUInt8 uiScanCode, bool bIsExtendedKey);

  /// Helper for retrieving the input slot string for touch point with a given index.
  static xiiStringView GetInputSlotTouchPoint(xiiUInt32 uiIndex);

  /// Helper for retrieving the input slot string for touch point x position with a given index.
  static xiiStringView GetInputSlotTouchPointPositionX(xiiUInt32 uiIndex);

  /// Helper for retrieving the input slot string for touch point y position with a given index.
  static xiiStringView GetInputSlotTouchPointPositionY(xiiUInt32 uiIndex);

  /// The data that is broadcast when certain events occur.
  struct InputEventData
  {
    enum EventType
    {
      InputSlotChanged,   ///< An input slot has been registered or its state changed.
      InputActionChanged, ///< An input action has been registered or its state changed.
    };

    EventType     m_EventType = InputSlotChanged;
    xiiStringView m_sInputSlot;
    xiiStringView m_sInputSet;
    xiiStringView m_sInputAction;
  };

  using xiiEventInput = xiiEvent<const InputEventData&>;

  /// Adds an event handler that is called for input events.
  static xiiEventSubscriptionID AddEventHandler(xiiEventInput::Handler handler) { return s_InputEvents.AddEventHandler(handler); }

  /// Removes a previously added event handler.
  static void RemoveEventHandler(xiiEventInput::Handler handler) { s_InputEvents.RemoveEventHandler(handler); }
  static void RemoveEventHandler(xiiEventSubscriptionID id) { s_InputEvents.RemoveEventHandler(id); }

private:
  friend class xiiInputDevice;

  /// Registers an input slot with the given name and a default display name.
  ///
  /// If the input slot was already registered before, the display name is only changed, it the previous registration did not
  /// specify a display name different from the input slot name.
  static void RegisterInputSlot(xiiStringView sName, xiiStringView sDefaultDisplayName, xiiBitflags<xiiInputSlotFlags> SlotFlags);

private:
  /// Stores the current state for one input slot.
  struct xiiInputSlot
  {
    xiiInputSlot();

    xiiString                      m_sDisplayName;     ///< The display name. Use this to present input slots in UIs.
    float                          m_fValue;           ///< The current value.
    float                          m_fValueOld = 0.0f; ///< The previous value. Needed so that GetInputSlotState can be called during input update phase.
    xiiKeyState::Enum              m_State;            ///< The current state.
    float                          m_fDeadZone;        ///< The dead zone. Unless the value exceeds this, it reports a zero value.
    xiiBitflags<xiiInputSlotFlags> m_SlotFlags;        ///< Describes the capabilities of the slot.
  };

  /// The data that is stored for each action.
  struct xiiActionData
  {
    xiiActionData();

    xiiInputActionConfig m_Config; ///< The configuration that was specified by the user.

    float             m_fValue; ///< The current value. This is the maximum of all input slot values that trigger this action.
    xiiKeyState::Enum m_State;  ///< The current state. Derived from m_fValue.

    xiiInt8 m_iTriggeredViaAlternative;
  };

  using xiiActionMap     = xiiMap<xiiString, xiiActionData>; ///< Maps input action names to their data.
  using xiiInputSetMap   = xiiMap<xiiString, xiiActionMap>;  ///< Maps input set names to their data.
  using xiiInputSlotsMap = xiiMap<xiiString, xiiInputSlot>;  ///< Maps input slot names to their data.

  /// The internal data of the xiiInputManager. Not allocated until it is actually required.
  struct InternalData
  {
    xiiInputSetMap               s_ActionMapping;      ///< Maps input set names to their data.
    xiiInputSlotsMap             s_InputSlots;         ///< Maps input slot names to their data.
    xiiMap<xiiString, xiiString> s_ActionDisplayNames; ///< Stores a display name for each input action.
    xiiMap<xiiString, float>     s_InjectedInputSlots;
  };

  /// The last (Unicode) character that was typed by the user, as reported by the OS (on Windows: WM_CHAR).
  static xiiUInt32 s_uiLastCharacter;

  static bool s_bInputSlotResetRequired;

  /// If not empty, all input for other input sets is returned as inactive ('key up')
  static xiiString s_sExclusiveInputSet;

  /// Resets all input slot value to zero.
  static void ResetInputSlotValues();

  /// Queries all known devices for their input slot values and stores the maximum values inside the input manager.
  static void GatherDeviceInputSlotValues();

  /// Uses the previously queried input slot values to update their overall state.
  static void UpdateInputSlotStates();

  /// Uses the previously queried input slot values to update the state (and value) of all input actions.
  static void UpdateInputActions(xiiTime tTimeDifference);

  /// Updates the state of all input actions in the given input set.
  static void UpdateInputActions(xiiStringView sInputSet, xiiActionMap& Actions, xiiTime tTimeDifference);

  /// Returns an iterator to the (next) action that should get triggered by the given slot.
  ///
  /// This may return several actions (when called repeatedly) when their are several actions with the same priority.
  static xiiActionMap::Iterator GetBestAction(xiiActionMap& Actions, const xiiString& sSlot, const xiiActionMap::Iterator& itFirst);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, InputManager);

  static void          DeallocateInternals();
  static InternalData& GetInternals();

  static xiiEventInput s_InputEvents;

  static InternalData* s_pData;
};
