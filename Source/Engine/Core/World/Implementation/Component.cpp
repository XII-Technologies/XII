#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiComponent, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiComponent::SetActiveFlag(bool bEnabled)
{
  if (m_ComponentFlags.IsSet(xiiObjectFlags::ActiveFlag) != bEnabled)
  {
    m_ComponentFlags.AddOrRemove(xiiObjectFlags::ActiveFlag, bEnabled);

    UpdateActiveState(GetOwner() == nullptr ? true : GetOwner()->IsActive());
  }
}

void xiiComponent::UpdateActiveState(bool bOwnerActive)
{
  const bool bSelfActive = bOwnerActive && m_ComponentFlags.IsSet(xiiObjectFlags::ActiveFlag);

  if (m_ComponentFlags.IsSet(xiiObjectFlags::ActiveState) != bSelfActive)
  {
    m_ComponentFlags.AddOrRemove(xiiObjectFlags::ActiveState, bSelfActive);

    if (IsInitialized())
    {
      if (bSelfActive)
      {
        // Don't call OnActivated & EnsureSimulationStarted here since there might be other components
        // that are needed in the OnSimulation callback but are activated right after this component.
        // Instead add the component to the initialization batch again.
        // There initialization will be skipped since the component is already initialized.
        GetWorld()->AddComponentToInitialize(GetHandle());
      }
      else
      {
        OnDeactivated();

        m_ComponentFlags.Remove(xiiObjectFlags::SimulationStarted);
      }
    }
  }
}

xiiWorld* xiiComponent::GetWorld()
{
  return m_pManager->GetWorld();
}

const xiiWorld* xiiComponent::GetWorld() const
{
  return m_pManager->GetWorld();
}

void xiiComponent::SerializeComponent(xiiWorldWriter& inout_stream) const {}

void xiiComponent::DeserializeComponent(xiiWorldReader& inout_stream) {}

void xiiComponent::EnsureInitialized()
{
  XII_ASSERT_DEV(m_pOwner != nullptr, "Owner must not be null");

  if (IsInitializing())
  {
    xiiLog::Error("Recursive initialize call is ignored.");
    return;
  }

  if (!IsInitialized())
  {
    m_pMessageDispatchType = GetDynamicRTTI();

    m_ComponentFlags.Add(xiiObjectFlags::Initializing);

    Initialize();

    m_ComponentFlags.Remove(xiiObjectFlags::Initializing);
    m_ComponentFlags.Add(xiiObjectFlags::Initialized);
  }
}

void xiiComponent::EnsureSimulationStarted()
{
  XII_ASSERT_DEV(IsActiveAndInitialized(), "Must not be called on uninitialized or inactive components.");
  XII_ASSERT_DEV(GetWorld()->GetWorldSimulationEnabled(), "Must not be called when the world is not simulated.");

  if (m_ComponentFlags.IsSet(xiiObjectFlags::SimulationStarting))
  {
    xiiLog::Error("Recursive simulation started call is ignored.");
    return;
  }

  if (!IsSimulationStarted())
  {
    m_ComponentFlags.Add(xiiObjectFlags::SimulationStarting);

    OnSimulationStarted();

    m_ComponentFlags.Remove(xiiObjectFlags::SimulationStarting);
    m_ComponentFlags.Add(xiiObjectFlags::SimulationStarted);
  }
}

bool xiiComponent::SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      xiiLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
                      GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(xiiObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    xiiLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

bool xiiComponent::SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      xiiLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
                      GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(xiiObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    xiiLog::Warning(
      "(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

void xiiComponent::PostMessage(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

bool xiiComponent::HandlesMessage(const xiiMessage& msg) const
{
  return m_pMessageDispatchType->CanHandleMessage(msg.GetId());
}

void xiiComponent::Initialize() {}

void xiiComponent::Deinitialize()
{
  XII_ASSERT_DEV(m_pOwner != nullptr, "Owner must still be valid");

  SetActiveFlag(false);
}

void xiiComponent::OnActivated() {}

void xiiComponent::OnDeactivated() {}

void xiiComponent::OnSimulationStarted() {}

void xiiComponent::EnableUnhandledMessageHandler(bool enable)
{
  m_ComponentFlags.AddOrRemove(xiiObjectFlags::UnhandledMessageHandler, enable);
}

bool xiiComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg)
{
  return false;
}

bool xiiComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const
{
  return false;
}

void xiiComponent::SetUserFlag(xiiUInt8 uiFlagIndex, bool bSet)
{
  XII_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  m_ComponentFlags.AddOrRemove(static_cast<xiiObjectFlags::Enum>(xiiObjectFlags::UserFlag0 << uiFlagIndex), bSet);
}

bool xiiComponent::GetUserFlag(xiiUInt8 uiFlagIndex) const
{
  XII_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  return m_ComponentFlags.IsSet(static_cast<xiiObjectFlags::Enum>(xiiObjectFlags::UserFlag0 << uiFlagIndex));
}


XII_STATICLINK_FILE(Core, Core_World_Implementation_Component);
