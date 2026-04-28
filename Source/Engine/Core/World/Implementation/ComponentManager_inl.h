/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_FORCE_INLINE bool xiiComponentManagerBase::IsValidComponent(const xiiComponentHandle& hComponent) const
{
  return m_Components.Contains(hComponent);
}

XII_FORCE_INLINE bool xiiComponentManagerBase::TryGetComponent(const xiiComponentHandle& hComponent, xiiComponent*& out_pComponent)
{
  return m_Components.TryGetValue(hComponent, out_pComponent);
}

XII_FORCE_INLINE bool xiiComponentManagerBase::TryGetComponent(const xiiComponentHandle& hComponent, const xiiComponent*& out_pComponent) const
{
  xiiComponent* pComponent = nullptr;
  bool          res        = m_Components.TryGetValue(hComponent, pComponent);
  out_pComponent           = pComponent;
  return res;
}

XII_ALWAYS_INLINE xiiUInt32 xiiComponentManagerBase::GetComponentCount() const
{
  return static_cast<xiiUInt32>(m_Components.GetCount());
}

template <typename ComponentType>
XII_ALWAYS_INLINE xiiTypedComponentHandle<ComponentType> xiiComponentManagerBase::CreateComponent(xiiGameObject* pOwnerObject, ComponentType*& out_pComponent)
{
  xiiComponent*      pComponent = nullptr;
  xiiComponentHandle hComponent = CreateComponentNoInit(pOwnerObject, pComponent);

  if (pComponent != nullptr)
  {
    InitializeComponent(pComponent);
  }

  out_pComponent = xiiStaticCast<ComponentType*>(pComponent);
  return xiiTypedComponentHandle<ComponentType>(hComponent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, xiiBlockStorageType::Enum StorageType>
xiiComponentManager<T, StorageType>::xiiComponentManager(xiiWorld* pWorld) :
  xiiComponentManagerBase(pWorld), m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponent, ComponentType), "Not a valid component type");
}

template <typename T, xiiBlockStorageType::Enum StorageType>
xiiComponentManager<T, StorageType>::~xiiComponentManager() = default;

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE bool xiiComponentManager<T, StorageType>::TryGetComponent(const xiiComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  XII_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId, "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(), hComponent.GetInternalID().m_TypeId);
  XII_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(), "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  xiiComponent* pComponent = nullptr;
  bool          bResult    = xiiComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent           = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE bool xiiComponentManager<T, StorageType>::TryGetComponent(const xiiComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  XII_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId, "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(), hComponent.GetInternalID().m_TypeId);
  XII_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(), "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  const xiiComponent* pComponent = nullptr;
  bool                bResult    = xiiComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent                 = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE typename xiiBlockStorage<T, xiiInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator xiiComponentManager<T, StorageType>::GetComponents(xiiUInt32 uiStartIndex /*= 0*/)
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE typename xiiBlockStorage<T, xiiInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator
xiiComponentManager<T, StorageType>::GetComponents(xiiUInt32 uiStartIndex /*= 0*/) const
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

// static
template <typename T, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE xiiWorldModuleTypeId xiiComponentManager<T, StorageType>::TypeId()
{
  return T::TypeId();
}

template <typename T, xiiBlockStorageType::Enum StorageType>
void xiiComponentManager<T, StorageType>::CollectAllComponents(xiiDynamicArray<xiiComponentHandle>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it->GetHandle());
    }
  }
}

template <typename T, xiiBlockStorageType::Enum StorageType>
void xiiComponentManager<T, StorageType>::CollectAllComponents(xiiDynamicArray<xiiComponent*>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it);
    }
  }
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE xiiComponent* xiiComponentManager<T, StorageType>::CreateComponentStorage()
{
  return m_ComponentStorage.Create();
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiComponentManager<T, StorageType>::DeleteComponentStorage(xiiComponent* pComponent, xiiComponent*& out_pMovedComponent)
{
  T* pMovedComponent = nullptr;
  m_ComponentStorage.Delete(static_cast<T*>(pComponent), pMovedComponent);
  out_pMovedComponent = pMovedComponent;
}

template <typename T, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiComponentManager<T, StorageType>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiAsyncPhaseBatchSize != 0)
  {
    desc.m_uiAsyncPhaseBatchSize = static_cast<xiiUInt16>(xiiMath::RoundUp(static_cast<xiiInt32>(desc.m_uiAsyncPhaseBatchSize), xiiDataBlock<ComponentType, xiiInternal::DEFAULT_BLOCK_SIZE>::CAPACITY));
  }

  xiiComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, xiiComponentUpdateType::Enum UpdateType, xiiBlockStorageType::Enum StorageType, xiiWorldUpdatePhase::Enum UpdatePhase>
xiiComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::xiiComponentManagerSimple(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, StorageType>(pWorld)
{
}

template <typename ComponentType, xiiComponentUpdateType::Enum UpdateType, xiiBlockStorageType::Enum StorageType, xiiWorldUpdatePhase::Enum UpdatePhase>
void xiiComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::Initialize()
{
  using OwnType = xiiComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>;

  xiiStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc                        = xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_Phase                     = UpdatePhase;
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == xiiComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, xiiComponentUpdateType::Enum UpdateType, xiiBlockStorageType::Enum StorageType, xiiWorldUpdatePhase::Enum UpdatePhase>
void xiiComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdate(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// static
template <typename ComponentType, xiiComponentUpdateType::Enum UpdateType, xiiBlockStorageType::Enum StorageType, xiiWorldUpdatePhase::Enum UpdatePhase>
void xiiComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdateName(xiiStringBuilder& out_sName)
{
  xiiStringView sName(XII_SOURCE_FUNCTION);
  const char*   szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("xiiComponentManagerSimple<class "))
  {
    xiiStringView sChoppedName(sName.GetStartPointer() + xiiStringUtils::GetStringElementCount("xiiComponentManagerSimple<class "), szEnd);

    XII_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::SimpleUpdate");
  }
  else
  {
    out_sName = sName;
  }
}
