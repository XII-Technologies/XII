#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiForwardEventsToGameStateComponent, 1 /* version */, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiForwardEventsToGameStateComponent::xiiForwardEventsToGameStateComponent()  = default;
xiiForwardEventsToGameStateComponent::~xiiForwardEventsToGameStateComponent() = default;

bool xiiForwardEventsToGameStateComponent::HandlesMessage(const xiiMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (xiiGameStateBase* pGameState = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool xiiForwardEventsToGameStateComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg)
{
  XII_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (xiiGameStateBase* pGameState = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool xiiForwardEventsToGameStateComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const
{
  XII_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (const xiiGameStateBase* pGameState = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void xiiForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}


XII_STATICLINK_FILE(Core, Core_GameState_Implementation_ForwardEventsToGameStateComponent);
