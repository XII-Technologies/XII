#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/TriggerDelayModifierComponent.h>

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
    new xiiCategoryAttribute("Gameplay/Logic"), // Component menu group
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

void xiiTriggerDelayModifierComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_ActivationDelay;
  s << m_DeactivationDelay;
}

void xiiTriggerDelayModifierComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_ActivationDelay;
  s >> m_DeactivationDelay;
}

void xiiTriggerDelayModifierComponent::OnMsgTriggerTriggered(xiiMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == xiiTriggerState::Activated)
  {
    if (m_iElementsInside++ == 0) // Was 0 before the increment
    {
      // The first object entered the trigger

      if (!m_bIsActivated)
      {
        // The trigger is not active yet -> send an activation message with a new activation token

        ++m_iValidActivationToken;

        // Store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        xiiMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Activate");
        intMsg.m_iPayload = m_iValidActivationToken;

        PostMessage(intMsg, m_ActivationDelay, xiiObjectMsgQueueType::PostTransform);
      }
      else
      {
        // The trigger is already active -> there are pending deactivations (otherwise we wouldn't have had an element count of zero)
        // -> invalidate those pending deactivations
        ++m_iValidDeactivationToken;

        // No need to send an activation message
      }
    }

    return;
  }

  if (msg.m_TriggerState == xiiTriggerState::Deactivated)
  {
    if (--m_iElementsInside == 0) // 0 after the decrement
    {
      // The last object left the trigger

      if (m_bIsActivated)
      {
        // If the trigger is active, we need to send a deactivation message and we give it a new token

        ++m_iValidDeactivationToken;

        // Store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        xiiMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Deactivate");
        intMsg.m_iPayload = m_iValidDeactivationToken;

        PostMessage(intMsg, m_DeactivationDelay, xiiObjectMsgQueueType::PostTransform);
      }
      else
      {
        // When we are already inactive, all that's needed is to invalidate any pending activations
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

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), xiiTime::Zero(), xiiObjectMsgQueueType::PostTransform);
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

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), xiiTime::Zero(), xiiObjectMsgQueueType::PostTransform);
    }
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TriggerDelayModifierComponent);
