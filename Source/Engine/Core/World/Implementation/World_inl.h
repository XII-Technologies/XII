
XII_ALWAYS_INLINE xiiStringView xiiWorld::GetName() const
{
  return m_Data.m_sName;
}

XII_ALWAYS_INLINE xiiUInt32 xiiWorld::GetIndex() const
{
  return m_uiIndex;
}

XII_FORCE_INLINE xiiGameObjectHandle xiiWorld::CreateObject(const xiiGameObjectDesc& desc)
{
  xiiGameObject* pNewObject;
  return CreateObject(desc, pNewObject);
}

XII_ALWAYS_INLINE const xiiEvent<const xiiGameObject*>& xiiWorld::GetObjectDeletionEvent() const
{
  return m_Data.m_ObjectDeletionEvent;
}

XII_FORCE_INLINE bool xiiWorld::IsValidObject(const xiiGameObjectHandle& hObject) const
{
  CheckForReadAccess();
  XII_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
                 "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.Contains(hObject);
}

XII_FORCE_INLINE bool xiiWorld::TryGetObject(const xiiGameObjectHandle& hObject, xiiGameObject*& out_pObject)
{
  CheckForReadAccess();
  XII_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
                 "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.TryGetValue(hObject, out_pObject);
}

XII_FORCE_INLINE bool xiiWorld::TryGetObject(const xiiGameObjectHandle& hObject, const xiiGameObject*& out_pObject) const
{
  CheckForReadAccess();
  XII_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
                 "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  xiiGameObject* pObject = nullptr;
  bool           bResult = m_Data.m_Objects.TryGetValue(hObject, pObject);
  out_pObject            = pObject;
  return bResult;
}

XII_FORCE_INLINE bool xiiWorld::TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, xiiGameObject*& out_pObject)
{
  CheckForReadAccess();
  xiiGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

XII_FORCE_INLINE bool xiiWorld::TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, const xiiGameObject*& out_pObject) const
{
  CheckForReadAccess();
  xiiGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

XII_FORCE_INLINE xiiUInt32 xiiWorld::GetObjectCount() const
{
  CheckForReadAccess();
  // Subtract one to exclude dummy object with instance index 0
  return static_cast<xiiUInt32>(m_Data.m_Objects.GetCount() - 1);
}

XII_FORCE_INLINE xiiInternal::WorldData::ObjectIterator xiiWorld::GetObjects()
{
  CheckForWriteAccess();
  return xiiInternal::WorldData::ObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

XII_FORCE_INLINE xiiInternal::WorldData::ConstObjectIterator xiiWorld::GetObjects() const
{
  CheckForReadAccess();
  return xiiInternal::WorldData::ConstObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

XII_FORCE_INLINE void xiiWorld::Traverse(VisitorFunc visitorFunc, TraversalMethod method /*= DepthFirst*/)
{
  CheckForWriteAccess();

  if (method == DepthFirst)
  {
    m_Data.TraverseDepthFirst(visitorFunc);
  }
  else // method == BreadthFirst
  {
    m_Data.TraverseBreadthFirst(visitorFunc);
  }
}

template <typename ModuleType>
XII_ALWAYS_INLINE ModuleType* xiiWorld::GetOrCreateModule()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiWorldModule, ModuleType), "Not a valid module type");

  return xiiStaticCast<ModuleType*>(GetOrCreateModule(xiiGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
XII_ALWAYS_INLINE void xiiWorld::DeleteModule()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiWorldModule, ModuleType), "Not a valid module type");

  DeleteModule(xiiGetStaticRTTI<ModuleType>());
}

template <typename ModuleType>
XII_ALWAYS_INLINE ModuleType* xiiWorld::GetModule()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiWorldModule, ModuleType), "Not a valid module type");

  return xiiStaticCast<ModuleType*>(GetModule(xiiGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
XII_ALWAYS_INLINE const ModuleType* xiiWorld::GetModule() const
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiWorldModule, ModuleType), "Not a valid module type");

  return xiiStaticCast<const ModuleType*>(GetModule(xiiGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
XII_ALWAYS_INLINE const ModuleType* xiiWorld::GetModuleReadOnly() const
{
  return GetModule<ModuleType>();
}

template <typename ManagerType>
ManagerType* xiiWorld::GetOrCreateComponentManager()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  if (pModule == nullptr)
  {
    pModule = XII_NEW(&m_Data.m_Allocator, ManagerType, this);
    static_cast<xiiWorldModule*>(pModule)->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

XII_ALWAYS_INLINE xiiComponentManagerBase* xiiWorld::GetOrCreateManagerForComponentType(const xiiRTTI* pComponentRtti)
{
  XII_ASSERT_DEV(pComponentRtti->IsDerivedFrom<xiiComponent>(), "Invalid component type '%s'", pComponentRtti->GetTypeName());

  return xiiStaticCast<xiiComponentManagerBase*>(GetOrCreateModule(pComponentRtti));
}

template <typename ManagerType>
void xiiWorld::DeleteComponentManager()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]))
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      static_cast<xiiWorldModule*>(pModule)->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      XII_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

template <typename ManagerType>
XII_FORCE_INLINE ManagerType* xiiWorld::GetComponentManager()
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const xiiWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return xiiStaticCast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

template <typename ManagerType>
XII_FORCE_INLINE const ManagerType* xiiWorld::GetComponentManager() const
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForReadAccess();

  const xiiWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return xiiStaticCast<const ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

XII_ALWAYS_INLINE xiiComponentManagerBase* xiiWorld::GetManagerForComponentType(const xiiRTTI* pComponentRtti)
{
  XII_ASSERT_DEV(pComponentRtti->IsDerivedFrom<xiiComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return xiiStaticCast<xiiComponentManagerBase*>(GetModule(pComponentRtti));
}

XII_ALWAYS_INLINE const xiiComponentManagerBase* xiiWorld::GetManagerForComponentType(const xiiRTTI* pComponentRtti) const
{
  XII_ASSERT_DEV(pComponentRtti->IsDerivedFrom<xiiComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return xiiStaticCast<const xiiComponentManagerBase*>(GetModule(pComponentRtti));
}

inline bool xiiWorld::IsValidComponent(const xiiComponentHandle& hComponent) const
{
  CheckForReadAccess();
  const xiiWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const xiiWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      return static_cast<const xiiComponentManagerBase*>(pModule)->IsValidComponent(hComponent);
    }
  }

  return false;
}

template <typename ComponentType>
inline bool xiiWorld::TryGetComponent(const xiiComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  CheckForWriteAccess();
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponent, ComponentType), "Not a valid component type");

  const xiiWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (xiiWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      xiiComponent* pComponent = nullptr;
      bool          bResult    = static_cast<xiiComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent           = xiiDynamicCast<ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

template <typename ComponentType>
inline bool xiiWorld::TryGetComponent(const xiiComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  CheckForReadAccess();
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiComponent, ComponentType), "Not a valid component type");

  const xiiWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const xiiWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      const xiiComponent* pComponent = nullptr;
      bool                bResult    = static_cast<const xiiComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent                 = xiiDynamicCast<const ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

XII_FORCE_INLINE void xiiWorld::SendMessage(const xiiGameObjectHandle& hReceiverObject, xiiMessage& ref_msg)
{
  CheckForWriteAccess();

  xiiGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessage(ref_msg);
  }
  else
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiWorld::SendMessage: The receiver xiiGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

XII_FORCE_INLINE void xiiWorld::SendMessageRecursive(const xiiGameObjectHandle& hReceiverObject, xiiMessage& ref_msg)
{
  CheckForWriteAccess();

  xiiGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessageRecursive(ref_msg);
  }
  else
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiWorld::SendMessageRecursive: The receiver xiiGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

XII_ALWAYS_INLINE void xiiWorld::PostMessage(const xiiGameObjectHandle& hReceiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, false);
}

XII_ALWAYS_INLINE void xiiWorld::PostMessageRecursive(const xiiGameObjectHandle& hReceiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, true);
}

XII_FORCE_INLINE void xiiWorld::SendMessage(const xiiComponentHandle& hReceiverComponent, xiiMessage& ref_msg)
{
  CheckForWriteAccess();

  xiiComponent* pReceiverComponent = nullptr;
  if (TryGetComponent(hReceiverComponent, pReceiverComponent))
  {
    pReceiverComponent->SendMessage(ref_msg);
  }
  else
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      xiiLog::Warning("xiiWorld::SendMessage: The receiver xiiComponent for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

XII_ALWAYS_INLINE void xiiWorld::SetWorldSimulationEnabled(bool bEnable)
{
  m_Data.m_bSimulateWorld = bEnable;
}

XII_ALWAYS_INLINE bool xiiWorld::GetWorldSimulationEnabled() const
{
  return m_Data.m_bSimulateWorld;
}

XII_ALWAYS_INLINE const xiiSharedPtr<xiiTask>& xiiWorld::GetUpdateTask()
{
  return m_pUpdateTask;
}

XII_ALWAYS_INLINE xiiUInt32 xiiWorld::GetUpdateCounter() const
{
  return m_Data.m_uiUpdateCounter;
}

XII_FORCE_INLINE xiiSpatialSystem* xiiWorld::GetSpatialSystem()
{
  CheckForWriteAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

XII_FORCE_INLINE const xiiSpatialSystem* xiiWorld::GetSpatialSystem() const
{
  CheckForReadAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

XII_ALWAYS_INLINE void xiiWorld::GetCoordinateSystem(const xiiVec3& vGlobalPosition, xiiCoordinateSystem& out_coordinateSystem) const
{
  m_Data.m_pCoordinateSystemProvider->GetCoordinateSystem(vGlobalPosition, out_coordinateSystem);
}

XII_ALWAYS_INLINE xiiCoordinateSystemProvider& xiiWorld::GetCoordinateSystemProvider()
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

XII_ALWAYS_INLINE const xiiCoordinateSystemProvider& xiiWorld::GetCoordinateSystemProvider() const
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

XII_ALWAYS_INLINE xiiClock& xiiWorld::GetClock()
{
  return m_Data.m_Clock;
}

XII_ALWAYS_INLINE const xiiClock& xiiWorld::GetClock() const
{
  return m_Data.m_Clock;
}

XII_ALWAYS_INLINE xiiRandom& xiiWorld::GetRandomNumberGenerator()
{
  return m_Data.m_Random;
}

XII_ALWAYS_INLINE xiiAllocatorBase* xiiWorld::GetAllocator()
{
  return &m_Data.m_Allocator;
}

XII_ALWAYS_INLINE xiiInternal::WorldLargeBlockAllocator* xiiWorld::GetBlockAllocator()
{
  return &m_Data.m_BlockAllocator;
}

XII_ALWAYS_INLINE xiiDoubleBufferedStackAllocator* xiiWorld::GetStackAllocator()
{
  return &m_Data.m_StackAllocator;
}

XII_ALWAYS_INLINE xiiInternal::WorldData::ReadMarker& xiiWorld::GetReadMarker() const
{
  return m_Data.m_ReadMarker;
}

XII_ALWAYS_INLINE xiiInternal::WorldData::WriteMarker& xiiWorld::GetWriteMarker()
{
  return m_Data.m_WriteMarker;
}

XII_FORCE_INLINE void xiiWorld::SetUserData(void* pUserData)
{
  CheckForWriteAccess();

  m_Data.m_pUserData = pUserData;
}

XII_FORCE_INLINE void* xiiWorld::GetUserData() const
{
  CheckForReadAccess();

  return m_Data.m_pUserData;
}

constexpr xiiUInt64 xiiWorld::GetMaxNumGameObjects()
{
  return xiiGameObjectId::MAX_INSTANCES - 2;
}

constexpr xiiUInt64 xiiWorld::GetMaxNumHierarchyLevels()
{
  return 1 << (sizeof(xiiGameObject::m_uiHierarchyLevel) * 8);
}

constexpr xiiUInt64 xiiWorld::GetMaxNumComponentsPerType()
{
  return xiiComponentId::MAX_INSTANCES - 1;
}

constexpr xiiUInt64 xiiWorld::GetMaxNumWorldModules()
{
  return XII_MAX_WORLD_MODULE_TYPES;
}

constexpr xiiUInt64 xiiWorld::GetMaxNumComponentTypes()
{
  return XII_MAX_COMPONENT_TYPES;
}

constexpr xiiUInt64 xiiWorld::GetMaxNumWorlds()
{
  return XII_MAX_WORLDS;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiWorld::GetWorldCount()
{
  return s_Worlds.GetCount();
}

// static
XII_ALWAYS_INLINE xiiWorld* xiiWorld::GetWorld(xiiUInt32 uiIndex)
{
  return s_Worlds[uiIndex];
}

// static
XII_ALWAYS_INLINE xiiWorld* xiiWorld::GetWorld(const xiiGameObjectHandle& hObject)
{
  return s_Worlds[hObject.GetInternalID().m_WorldIndex];
}

// static
XII_ALWAYS_INLINE xiiWorld* xiiWorld::GetWorld(const xiiComponentHandle& hComponent)
{
  return s_Worlds[hComponent.GetInternalID().m_WorldIndex];
}

XII_ALWAYS_INLINE void xiiWorld::CheckForReadAccess() const
{
  XII_ASSERT_DEV(m_Data.m_iReadCounter > 0, "Trying to read from World '{0}', but it is not marked for reading.", GetName());
}

XII_ALWAYS_INLINE void xiiWorld::CheckForWriteAccess() const
{
  XII_ASSERT_DEV(m_Data.m_WriteThreadID == xiiThreadUtils::GetCurrentThreadID(), "Trying to write to World '{0}', but it is not marked for writing.", GetName());
}

XII_ALWAYS_INLINE xiiGameObject* xiiWorld::GetObjectUnchecked(xiiUInt32 uiIndex) const
{
  return m_Data.m_Objects.GetValueUnchecked(uiIndex);
}

XII_ALWAYS_INLINE bool xiiWorld::ReportErrorWhenStaticObjectMoves() const
{
  return m_Data.m_bReportErrorWhenStaticObjectMoves;
}

XII_ALWAYS_INLINE float xiiWorld::GetInvDeltaSeconds() const
{
  const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
  if (fDelta > 0.0f)
  {
    return 1.0f / fDelta;
  }

  // when the clock is paused just use zero
  return 0.0f;
}
