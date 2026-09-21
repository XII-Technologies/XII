/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiTriggerState
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Activated,   ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,  ///< The trigger is active for more than one frame now.
    Deactivated, ///< The trigger was just deactivated (left area, key released, etc.)

    Default = Activated
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiTriggerState);

/// For internal use by components to trigger some known behavior. Usually components will post this message to themselves with a
/// delay, e.g. to trigger self destruction.
struct XII_CORE_DLL xiiMsgComponentInternalTrigger : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgComponentInternalTrigger, xiiMessage);

  /// Identifies what the message should trigger.
  xiiHashedString m_sMessage;

  xiiInt32 m_iPayload = 0;
};

/// Sent when something enters or leaves a trigger
struct XII_CORE_DLL xiiMsgTriggerTriggered : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgTriggerTriggered, xiiEventMessage);

  /// Identifies what the message should trigger.
  xiiHashedString m_sMessage;

  /// Messages are only sent for 'entered' ('Activated') and 'left' ('Deactivated')
  xiiEnum<xiiTriggerState> m_TriggerState;

  /// The object that entered the trigger volume.
  xiiGameObjectHandle m_hTriggeringObject;
};
