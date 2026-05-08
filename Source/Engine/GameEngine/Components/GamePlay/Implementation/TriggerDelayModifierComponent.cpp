/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/Gameplay/TriggerDelayModifierComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTriggerDelayModifierComponent, 1 /* version */, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ActivationDelay", m_ActivationDelay),
    XII_MEMBER_PROPERTY("DeactivationDelay", m_DeactivationDelay),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
    XII_MESSAGE_HANDLER(xiiMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  XII_END_MESSAGEHANDLERS;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiTriggerDelayModifierComponent::xiiTriggerDelayModifierComponent()  = default;
xiiTriggerDelayModifierComponent::~xiiTriggerDelayModifierComponent() = default;

void xiiTriggerDelayModifierComponent::Initialize()
{
  SUPER::Initialize();
}

void xiiTriggerDelayModifierComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_ActivationDelay;
  s << m_DeactivationDelay;
}

void xiiTriggerDelayModifierComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_ActivationDelay;
  s >> m_DeactivationDelay;
}

void xiiTriggerDelayModifierComponent::OnMsgTriggerTriggered(xiiMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == xiiTriggerState::Activated)
  {
    if (m_iElementsInside++ == 0) // was 0 before the increment
    {
      // the first object entered the trigger

      if (!m_bIsActivated)
      {
        // the trigger is not active yet -> send an activation message with a new activation token

        ++m_iValidActivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        xiiMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Activate");
        intMsg.m_iPayload = m_iValidActivationToken;

        PostMessage(intMsg, m_ActivationDelay, xiiObjectMsgQueueType::PostTransform);
      }
      else
      {
        // the trigger is already active -> there are pending deactivations (otherwise we wouldn't have had an element count of zero)
        // -> invalidate those pending deactivations
        ++m_iValidDeactivationToken;

        // no need to send an activation message
      }
    }

    return;
  }

  if (msg.m_TriggerState == xiiTriggerState::Deactivated)
  {
    if (--m_iElementsInside == 0) // 0 after the decrement
    {
      // the last object left the trigger

      if (m_bIsActivated)
      {
        // if the trigger is active, we need to send a deactivation message and we give it a new token

        ++m_iValidDeactivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        xiiMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Deactivate");
        intMsg.m_iPayload = m_iValidDeactivationToken;

        PostMessage(intMsg, m_DeactivationDelay, xiiObjectMsgQueueType::PostTransform);
      }
      else
      {
        // when we are already inactive, all that's needed is to invalidate any pending activations
        ++m_iValidActivationToken;
      }
    }

    return;
  }
}

void xiiTriggerDelayModifierComponent::OnMsgComponentInternalTrigger(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == xiiTempHashedString("Activate"))
  {
    if (msg.m_iPayload == m_iValidActivationToken && !m_bIsActivated)
    {
      m_bIsActivated = true;

      xiiMsgTriggerTriggered newMsg;
      newMsg.m_sMessage     = m_sMessage;
      newMsg.m_TriggerState = xiiTriggerState::Activated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), xiiTime::MakeZero(), xiiObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_sMessage == xiiTempHashedString("Deactivate"))
  {
    if (msg.m_iPayload == m_iValidDeactivationToken && m_bIsActivated)
    {
      m_bIsActivated = false;

      xiiMsgTriggerTriggered newMsg;
      newMsg.m_sMessage     = m_sMessage;
      newMsg.m_TriggerState = xiiTriggerState::Deactivated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), xiiTime::MakeZero(), xiiObjectMsgQueueType::PostTransform);
    }
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TriggerDelayModifierComponent);
