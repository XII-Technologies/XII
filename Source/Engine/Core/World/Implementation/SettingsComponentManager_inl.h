/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename ComponentType>
xiiSettingsComponentManager<ComponentType>::xiiSettingsComponentManager(xiiWorld* pWorld) :
  xiiComponentManagerBase(pWorld)
{
}

template <typename ComponentType>
xiiSettingsComponentManager<ComponentType>::~xiiSettingsComponentManager()
{
  for (auto& component : m_Components)
  {
    DeinitializeComponent(component.Borrow());
  }
}

template <typename ComponentType>
XII_ALWAYS_INLINE ComponentType* xiiSettingsComponentManager<ComponentType>::GetSingletonComponent()
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

template <typename ComponentType>
XII_ALWAYS_INLINE const ComponentType* xiiSettingsComponentManager<ComponentType>::GetSingletonComponent() const
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

// static
template <typename ComponentType>
XII_ALWAYS_INLINE xiiWorldModuleTypeId xiiSettingsComponentManager<ComponentType>::TypeId()
{
  return ComponentType::TypeId();
}

template <typename ComponentType>
void xiiSettingsComponentManager<ComponentType>::CollectAllComponents(xiiDynamicArray<xiiComponentHandle>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component->GetHandle());
    }
  }
}

template <typename ComponentType>
void xiiSettingsComponentManager<ComponentType>::CollectAllComponents(xiiDynamicArray<xiiComponent*>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component.Borrow());
    }
  }
}

template <typename ComponentType>
xiiComponent* xiiSettingsComponentManager<ComponentType>::CreateComponentStorage()
{
  if (!m_Components.IsEmpty())
  {
    xiiLog::Warning("A component of type '{0}' is already present in this world. Having more than one is not allowed.", xiiGetStaticRTTI<ComponentType>()->GetTypeName());
  }

  m_Components.PushBack(XII_NEW(GetAllocator(), ComponentType));
  return m_Components.PeekBack().Borrow();
}

template <typename ComponentType>
void xiiSettingsComponentManager<ComponentType>::DeleteComponentStorage(xiiComponent* pComponent, xiiComponent*& out_pMovedComponent)
{
  out_pMovedComponent = pComponent;

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    if (m_Components[i].Borrow() == pComponent)
    {
      m_Components.RemoveAtAndCopy(i);
      break;
    }
  }
}
