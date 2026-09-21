/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Input/Declarations.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using xiiInputComponentManager = xiiComponentManagerSimple<class xiiInputComponent, xiiComponentUpdateType::WhenSimulating>;

/// Which types of input events are broadcast
struct XII_GAMEENGINE_DLL xiiInputMessageGranularity
{
  using StorageType = xiiInt8;

  /// Which types of input events are broadcast
  enum Enum
  {
    PressOnly,           ///< Key pressed events are sent, but nothing else
    PressAndRelease,     ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well

    Default = PressOnly
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiInputMessageGranularity);

/// xiiInputComponent raises this event when it detects input
struct XII_GAMEENGINE_DLL xiiMsgInputActionTriggered : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgInputActionTriggered, xiiEventMessage);

  /// The input action string.
  xiiHashedString m_sInputAction;

  /// The 'trigger state', depending on the key state and the configuration on the xiiInputComponent
  xiiEnum<xiiTriggerState> m_TriggerState;

  /// For analog keys, how much they are pressed. Typically between 0 and 1.
  float m_fKeyPressValue;

private:
  const char* GetInputAction() const { return m_sInputAction; }
  void        SetInputAction(const char* szInputAction) { m_sInputAction.Assign(szInputAction); }
};

/// This component polls all input events from the given input set every frame and broadcasts the information to components on the same game
/// object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type xiiMsgInputActionTriggered.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released or down) messages are sent.
class XII_GAMEENGINE_DLL xiiInputComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiInputComponent, xiiComponent, xiiInputComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiInputComponent

public:
  xiiInputComponent();
  ~xiiInputComponent();

  /// Returns the amount to which szInputAction is active (0 to 1).
  ///
  /// If bOnlyKeyPressed is set to true, only key press events return a non-zero value,
  /// ie key down and key released events are ignored.
  float GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed = false) const; // [ scriptable ]

  xiiString                           m_sInputSet;                    // [ property ]
  xiiEnum<xiiInputMessageGranularity> m_Granularity;                  // [ property ]
  bool                                m_bForwardToBlackboard = false; // [ property ]

protected:
  void Update();

  xiiEventMessageSender<xiiMsgInputActionTriggered> m_InputEventSender; // [ event ]
};
