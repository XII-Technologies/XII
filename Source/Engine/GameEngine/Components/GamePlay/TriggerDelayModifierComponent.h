/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <GameEngine/GameEngineDLL.h>

struct xiiMsgTriggerTriggered;
struct xiiMsgComponentInternalTrigger;

using xiiTriggerDelayModifierComponentManager = xiiComponentManager<class xiiTriggerDelayModifierComponent, xiiBlockStorageType::Compact>;

/// Handles xiiMsgTriggerTriggered events and sends new messages after a delay.
///
/// The 'enter' and 'leave' messages are sent only when an empty trigger is entered or when the last object leaves the trigger.
/// While any object is already inside the trigger, no change event is sent.
/// Therefore this component can't be used to keep track of all the objects inside the trigger.
///
/// The 'enter' and 'leave' events can be sent with a delay. The 'enter' event is only sent, if the trigger had at least one object
/// inside it for the full duration of the delay. Which exact object may change, but once the trigger contains no object, the timer is reset.
///
/// The sent xiiMsgTriggerTriggered does not contain a reference to the 'triggering' object, since there may be multiple and they may change randomly.
class XII_GAMEENGINE_DLL xiiTriggerDelayModifierComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTriggerDelayModifierComponent, xiiComponent, xiiTriggerDelayModifierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiTriggerDelayModifierComponent

public:
  xiiTriggerDelayModifierComponent();
  ~xiiTriggerDelayModifierComponent();

protected:
  virtual void Initialize() override;

  void OnMsgTriggerTriggered(xiiMsgTriggerTriggered& msg);
  void OnMsgComponentInternalTrigger(xiiMsgComponentInternalTrigger& msg);

  bool     m_bIsActivated            = false;
  xiiInt32 m_iElementsInside         = 0;
  xiiInt32 m_iValidActivationToken   = 0;
  xiiInt32 m_iValidDeactivationToken = 0;

  xiiTime         m_ActivationDelay;
  xiiTime         m_DeactivationDelay;
  xiiHashedString m_sMessage;

  xiiEventMessageSender<xiiMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
