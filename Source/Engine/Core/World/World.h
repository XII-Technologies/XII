#pragma once

#include <Core/World/Implementation/WorldData.h>

struct xiiEventMessage;
class xiiEventMessageHandlerComponent;

/// \brief A world encapsulates a scene graph of game objects and various component managers and their components.
///
/// There can be multiple worlds active at a time, but only 64 at most. The world manages all object storage and might move objects around
/// in memory. Thus it is not allowed to store pointers to objects. They should be referenced by handles.\n The world has a multi-phase
/// update mechanism which is divided in the following phases:\n
/// * Pre-async phase: The corresponding component manager update functions are called synchronously in the order of their dependencies.
/// * Async phase: The update functions are called in batches asynchronously on multiple threads. There is absolutely no guarantee in which
/// order the functions are called.
///   Thus it is not allowed to access any data other than the components own data during that phase.
/// * Post-async phase: Another synchronous phase like the pre-async phase.
/// * Actual deletion of dead objects and components are done now.
/// * Transform update: The global transformation of dynamic objects is updated.
/// * Post-transform phase: Another synchronous phase like the pre-async phase after the transformation has been updated.
class XII_CORE_DLL xiiWorld final
{
public:
  /// \brief Creates a new world with the given name.
  xiiWorld(xiiWorldDesc& desc);
  ~xiiWorld();

  /// \brief Deletes all game objects in a world
  void Clear();

  /// \brief Returns the name of this world.
  const char* GetName() const;

  /// \brief Returns the index of this world.
  xiiUInt32 GetIndex() const;

  /// \name Object Functions
  ///@{

  /// \brief Create a new game object from the given description and returns a handle to it.
  xiiGameObjectHandle CreateObject(const xiiGameObjectDesc& desc);

  /// \brief Create a new game object from the given description, writes a pointer to it to out_pObject and returns a handle to it.
  xiiGameObjectHandle CreateObject(const xiiGameObjectDesc& desc, xiiGameObject*& out_pObject);

  /// \brief Deletes the given object, its children and all components.
  /// \note This function deletes the object immediately! It is unsafe to use this during a game update loop, as other objects
  /// may rely on this object staying valid for the rest of the frame.
  /// Use DeleteObjectDelayed() instead for safe removal at the end of the frame.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectNow(const xiiGameObjectHandle& object, bool bAlsoDeleteEmptyParents = true);

  /// \brief Deletes the given object at the beginning of the next world update. The object and its components and children stay completely
  /// valid until then.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectDelayed(const xiiGameObjectHandle& object, bool bAlsoDeleteEmptyParents = true);

  /// \brief Returns the event that is triggered before an object is deleted. This can be used for external systems to cleanup data
  /// which is associated with the deleted object.
  const xiiEvent<const xiiGameObject*>& GetObjectDeletionEvent() const;

  /// \brief Returns whether the given handle corresponds to a valid object.
  bool IsValidObject(const xiiGameObjectHandle& object) const;

  /// \brief Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const xiiGameObjectHandle& object, xiiGameObject*& out_pObject);

  /// \brief Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const xiiGameObjectHandle& object, const xiiGameObject*& out_pObject) const;

  /// \brief Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, xiiGameObject*& out_pObject);

  /// \brief Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, const xiiGameObject*& out_pObject) const;


  /// \brief Returns the total number of objects in this world.
  xiiUInt32 GetObjectCount() const;

  /// \brief Returns an iterator over all objects in this world in no specific order.
  xiiInternal::WorldData::ObjectIterator GetObjects();

  /// \brief Returns an iterator over all objects in this world in no specific order.
  xiiInternal::WorldData::ConstObjectIterator GetObjects() const;

  /// \brief Defines a visitor function that is called for every game-object when using the traverse method.
  /// The function takes a pointer to the game object as argument and returns a bool which indicates whether to continue (true) or abort
  /// (false) traversal.
  typedef xiiInternal::WorldData::VisitorFunc VisitorFunc;

  enum TraversalMethod
  {
    BreadthFirst,
    DepthFirst
  };

  /// \brief Traverses the game object tree starting at the top level objects and then recursively all children. The given callback function
  /// is called for every object.
  void Traverse(VisitorFunc visitorFunc, TraversalMethod method = DepthFirst);

  ///@}
  /// \name Module Functions
  ///@{

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  template <typename ModuleType>
  ModuleType* GetOrCreateModule();

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  xiiWorldModule* GetOrCreateModule(const xiiRTTI* pRtti);

  /// \brief Deletes the module of the given type or derived types.
  template <typename ModuleType>
  void DeleteModule();

  /// \brief Deletes the module of the given type or derived types.
  void DeleteModule(const xiiRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  ModuleType* GetModule();

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModule() const;

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModuleReadOnly() const;

  /// \brief Returns the instance to the given module type or derived types.
  xiiWorldModule* GetModule(const xiiRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  const xiiWorldModule* GetModule(const xiiRTTI* pRtti) const;

  ///@}
  /// \name Component Functions
  ///@{

  /// \brief Creates an instance of the given component manager type or returns a pointer to an already existing instance.
  template <typename ManagerType>
  ManagerType* GetOrCreateComponentManager();

  /// \brief Returns the component manager that handles the given rtti component type.
  xiiComponentManagerBase* GetOrCreateManagerForComponentType(const xiiRTTI* pComponentRtti);

  /// \brief Deletes the component manager of the given type and all its components.
  template <typename ManagerType>
  void DeleteComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  ManagerType* GetComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  const ManagerType* GetComponentManager() const;

  /// \brief Returns the component manager that handles the given rtti component type.
  xiiComponentManagerBase* GetManagerForComponentType(const xiiRTTI* pComponentRtti);

  /// \brief Returns the component manager that handles the given rtti component type.
  const xiiComponentManagerBase* GetManagerForComponentType(const xiiRTTI* pComponentRtti) const;

  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const xiiComponentHandle& component) const;

  /// \brief Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const xiiComponentHandle& component, ComponentType*& out_pComponent);

  /// \brief Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const xiiComponentHandle& component, const ComponentType*& out_pComponent) const;

  /// \brief Creates a new component init batch.
  /// It is ensured that the Initialize function is called for all components in a batch before the OnSimulationStarted is called.
  /// If bMustFinishWithinOneFrame is set to false the processing of an init batch can be distributed over multiple frames if
  /// m_MaxComponentInitializationTimePerFrame in the world desc is set to a reasonable value.
  xiiComponentInitBatchHandle CreateComponentInitBatch(const char* szBatchName, bool bMustFinishWithinOneFrame = true);

  /// \brief Deletes a component init batch. It must be completely processed before it can be deleted.
  void DeleteComponentInitBatch(const xiiComponentInitBatchHandle& batch);

  /// \brief All components that are created between an BeginAddingComponentsToInitBatch/EndAddingComponentsToInitBatch scope are added to the
  /// given init batch.
  void BeginAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& batch);

  /// \brief End adding components to the given batch. Components created after this call are added to the default init batch.
  void EndAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& batch);

  /// \brief After all components have been added to the init batch call submit to start processing the batch.
  void SubmitComponentInitBatch(const xiiComponentInitBatchHandle& batch);

  /// \brief Returns whether the init batch has been completely processed and all corresponding components are initialized
  /// and their OnSimulationStarted function was called.
  bool IsComponentInitBatchCompleted(const xiiComponentInitBatchHandle& batch, double* pCompletionFactor = nullptr);

  /// \brief Cancel the init batch if it is still active. This might leave outstanding components in an inconsistent state,
  /// so this function has be used with care.
  void CancelComponentInitBatch(const xiiComponentInitBatchHandle& batch);

  ///@}
  /// \name Message Functions
  ///@{

  /// \brief Sends a message to all components of the receiverObject.
  void SendMessage(const xiiGameObjectHandle& receiverObject, xiiMessage& msg);

  /// \brief Sends a message to all components of the receiverObject and all its children.
  void SendMessageRecursive(const xiiGameObjectHandle& receiverObject, xiiMessage& msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverObject after the given delay in the corresponding phase.
  void PostMessage(const xiiGameObjectHandle& receiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// \brief Queues the message for the given phase. The message is send to the receiverObject and all its children after the given delay in
  /// the corresponding phase.
  void PostMessageRecursive(const xiiGameObjectHandle& receiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// \brief Sends a message to the component.
  void SendMessage(const xiiComponentHandle& receiverComponent, xiiMessage& msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverComponent after the given delay in the corresponding phase.
  void PostMessage(const xiiComponentHandle& receiverComponent, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// \brief Finds the closest (parent) object, starting at pSearchObject, which has an xiiEventMessageHandlerComponent and returns all
  /// xiiEventMessageHandlerComponents owned by that object and that handle messages of the given type.
  ///
  /// If any such parent object exists, the search is stopped there, even if that component does not handle messages of the given type.
  /// If no such parent object exists, it searches for all xiiEventMessageHandlerComponent instances that are set to 'handle global events'
  /// that handle messages of the given type.
  void FindEventMsgHandlers(const xiiEventMessage& msg, xiiGameObject* pSearchObject, xiiDynamicArray<xiiComponent*>& out_components);

  /// \copydoc xiiWorld::FindEventMsgHandlers()
  void FindEventMsgHandlers(const xiiEventMessage& msg, const xiiGameObject* pSearchObject, xiiDynamicArray<const xiiComponent*>& out_components) const;

  ///@}

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  void SetWorldSimulationEnabled(bool bEnable);

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  bool GetWorldSimulationEnabled() const;

  /// \brief Updates the world by calling the various update methods on the component managers and also updates the transformation data of
  /// the game objects. See xiiWorld for a detailed description of the update phases.
  void Update();

  /// \brief Returns a task implementation that calls Update on this world.
  const xiiSharedPtr<xiiTask>& GetUpdateTask();


  /// \brief Returns the spatial system that is associated with this world.
  xiiSpatialSystem* GetSpatialSystem();

  /// \brief Returns the spatial system that is associated with this world.
  const xiiSpatialSystem* GetSpatialSystem() const;


  /// \brief Returns the coordinate system for the given position.
  /// By default this always returns a coordinate system with forward = +X, right = +Y and up = +Z.
  /// This can be customized by setting a different coordinate system provider.
  void GetCoordinateSystem(const xiiVec3& vGlobalPosition, xiiCoordinateSystem& out_CoordinateSystem) const;

  /// \brief Sets the coordinate system provider that should be used in this world.
  void SetCoordinateSystemProvider(const xiiSharedPtr<xiiCoordinateSystemProvider>& pProvider);

  /// \brief Returns the coordinate system provider that is associated with this world.
  xiiCoordinateSystemProvider& GetCoordinateSystemProvider();

  /// \brief Returns the coordinate system provider that is associated with this world.
  const xiiCoordinateSystemProvider& GetCoordinateSystemProvider() const;


  /// \brief Returns the clock that is used for all updates in this game world
  xiiClock& GetClock();

  /// \brief Returns the clock that is used for all updates in this game world
  const xiiClock& GetClock() const;

  /// \brief Accesses the default random number generator.
  /// If more control is desired, individual components should use their own RNG.
  xiiRandom& GetRandomNumberGenerator();


  /// \brief Returns the allocator used by this world.
  xiiAllocatorBase* GetAllocator();

  /// \brief Returns the block allocator used by this world.
  xiiInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns the stack allocator used by this world.
  xiiDoubleBufferedStackAllocator* GetStackAllocator();

  /// \brief Mark the world for reading by using XII_LOCK(world.GetReadMarker()). Multiple threads can read simultaneously if none is
  /// writing.
  xiiInternal::WorldData::ReadMarker& GetReadMarker() const;

  /// \brief Mark the world for writing by using XII_LOCK(world.GetWriteMarker()). Only one thread can write at a time.
  xiiInternal::WorldData::WriteMarker& GetWriteMarker();

  /// \brief Allows re-setting the maximum time that is spent on component initialization per frame, which is first configured on construction.
  void SetMaxInitializationTimePerFrame(xiiTime maxInitTime);

  /// \brief Associates the given user data with the world. The user is responsible for the life time of user data.
  void SetUserData(void* pUserData);

  /// \brief Returns the associated user data.
  void* GetUserData() const;

  using ReferenceResolver = xiiDelegate<xiiGameObjectHandle(const void*, xiiComponentHandle hThis, const char* szProperty)>;

  /// \brief If set, this delegate can be used to map some data (GUID or string) to an xiiGameObjectHandle.
  ///
  /// Currently only used in editor settings, to create a runtime handle from a unique editor reference.
  void SetGameObjectReferenceResolver(const ReferenceResolver& resolver);

  /// \sa SetGameObjectReferenceResolver()
  const ReferenceResolver& GetGameObjectReferenceResolver() const;

  /// \name Helper methods to query xiiWorld limits
  ///@{
  static constexpr xiiUInt64 GetMaxNumGameObjects();
  static constexpr xiiUInt64 GetMaxNumHierarchyLevels();
  static constexpr xiiUInt64 GetMaxNumComponentsPerType();
  static constexpr xiiUInt64 GetMaxNumWorldModules();
  static constexpr xiiUInt64 GetMaxNumComponentTypes();
  static constexpr xiiUInt64 GetMaxNumWorlds();
  ///@}

public:
  /// \brief Returns the number of active worlds.
  static xiiUInt32 GetWorldCount();

  /// \brief Returns the world with the given index.
  static xiiWorld* GetWorld(xiiUInt32 uiIndex);

  /// \brief Returns the world for the given game object handle.
  static xiiWorld* GetWorld(const xiiGameObjectHandle& object);

  /// \brief Returns the world for the given component handle.
  static xiiWorld* GetWorld(const xiiComponentHandle& component);

private:
  friend class xiiGameObject;
  friend class xiiWorldModule;
  friend class xiiComponentManagerBase;
  friend class xiiComponent;

  void CheckForReadAccess() const;
  void CheckForWriteAccess() const;

  xiiGameObject* GetObjectUnchecked(xiiUInt32 uiIndex) const;

  void SetParent(xiiGameObject* pObject, xiiGameObject* pNewParent, xiiGameObject::TransformPreservation preserve = xiiGameObject::TransformPreservation::PreserveGlobal);
  void LinkToParent(xiiGameObject* pObject);
  void UnlinkFromParent(xiiGameObject* pObject);

  void        SetObjectGlobalKey(xiiGameObject* pObject, const xiiHashedString& sGlobalKey);
  const char* GetObjectGlobalKey(const xiiGameObject* pObject) const;

  void PostMessage(const xiiGameObjectHandle& receiverObject, const xiiMessage& msg, xiiObjectMsgQueueType::Enum queueType, xiiTime delay, bool bRecursive) const;
  void ProcessQueuedMessage(const xiiInternal::WorldData::MessageQueue::Entry& entry);
  void ProcessQueuedMessages(xiiObjectMsgQueueType::Enum queueType);

  template <typename World, typename GameObject, typename Component>
  static void FindEventMsgHandlers(World& world, const xiiEventMessage& msg, GameObject pSearchObject, xiiDynamicArray<Component>& out_components);

  void RegisterUpdateFunction(const xiiWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const xiiWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunctions(xiiWorldModule* pModule);

  /// \brief Used by component managers to queue a new component for initialization during the next update
  void AddComponentToInitialize(xiiComponentHandle hComponent);

  void UpdateFromThread();
  void UpdateSynchronous(const xiiArrayPtr<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();

  // returns if the batch was completely initialized
  bool      ProcessInitializationBatch(xiiInternal::WorldData::InitBatch& batch, xiiTime endTime);
  void      ProcessComponentsToInitialize();
  void      ProcessUpdateFunctionsToRegister();
  xiiResult RegisterUpdateFunctionInternal(const xiiWorldModule::UpdateFunctionDesc& desc);

  void DeleteDeadObjects();
  void DeleteDeadComponents();

  void PatchHierarchyData(xiiGameObject* pObject, xiiGameObject::TransformPreservation preserve);
  void RecreateHierarchyData(xiiGameObject* pObject, bool bWasDynamic);

  bool ReportErrorWhenStaticObjectMoves() const;

  xiiSharedPtr<xiiTask> m_pUpdateTask;

  xiiInternal::WorldData m_Data;

  typedef xiiInternal::WorldData::QueuedMsgMetaData QueuedMsgMetaData;

  xiiUInt32                                        m_uiIndex;
  static xiiStaticArray<xiiWorld*, XII_MAX_WORLDS> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>
