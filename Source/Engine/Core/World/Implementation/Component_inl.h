/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Logging/Log.h>

XII_ALWAYS_INLINE xiiComponent::xiiComponent() = default;

XII_ALWAYS_INLINE xiiComponent::~xiiComponent()
{
  m_pMessageDispatchType = nullptr;
  m_pManager             = nullptr;
  m_pOwner               = nullptr;
  m_InternalId.Invalidate();
}

XII_ALWAYS_INLINE bool xiiComponent::IsDynamic() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::Dynamic);
}

XII_ALWAYS_INLINE bool xiiComponent::GetActiveFlag() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::ActiveFlag);
}

XII_ALWAYS_INLINE bool xiiComponent::IsActive() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::ActiveState);
}

XII_ALWAYS_INLINE bool xiiComponent::IsActiveAndInitialized() const
{
  return m_ComponentFlags.AreAllSet(xiiObjectFlags::ActiveState | xiiObjectFlags::Initialized);
}

XII_ALWAYS_INLINE xiiComponentManagerBase* xiiComponent::GetOwningManager()
{
  return m_pManager;
}

XII_ALWAYS_INLINE const xiiComponentManagerBase* xiiComponent::GetOwningManager() const
{
  return m_pManager;
}

XII_ALWAYS_INLINE xiiGameObject* xiiComponent::GetOwner()
{
  return m_pOwner;
}

XII_ALWAYS_INLINE const xiiGameObject* xiiComponent::GetOwner() const
{
  return m_pOwner;
}

XII_ALWAYS_INLINE xiiComponentHandle xiiComponent::GetHandle() const
{
  return xiiComponentHandle(m_InternalId);
}

XII_ALWAYS_INLINE xiiUInt32 xiiComponent::GetUniqueID() const
{
  return m_uiUniqueID;
}

XII_ALWAYS_INLINE void xiiComponent::SetUniqueID(xiiUInt32 uiUniqueID)
{
  m_uiUniqueID = uiUniqueID;
}

XII_ALWAYS_INLINE bool xiiComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::Initialized);
}

XII_ALWAYS_INLINE bool xiiComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::Initializing);
}

XII_ALWAYS_INLINE bool xiiComponent::IsSimulationStarted() const
{
  return m_ComponentFlags.IsSet(xiiObjectFlags::SimulationStarted);
}

XII_ALWAYS_INLINE bool xiiComponent::IsActiveAndSimulating() const
{
  return m_ComponentFlags.AreAllSet(xiiObjectFlags::Initialized | xiiObjectFlags::ActiveState) &&
    m_ComponentFlags.IsAnySet(xiiObjectFlags::SimulationStarting | xiiObjectFlags::SimulationStarted);
}
