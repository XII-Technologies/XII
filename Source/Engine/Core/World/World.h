/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Implementation/WorldData.h>

struct xiiEventMessage;
class xiiEventMessageHandlerComponent;

/// A world encapsulates a scene graph of game objects and various component managers and their components.
///
/// There can be multiple worlds active at a time, but only 256 at most. The world manages all object storage and might move objects around in memory.
/// Thus it is not allowed to store pointers to objects. They should be referenced by handles.\n The world has a multi-phase update mechanism which is divided in the following phases:\n
/// * Pre-async phase: The corresponding component manager update functions are called synchronously in the order of their dependencies.
/// * Async phase: The update functions are called in batches asynchronously on multiple threads. There is absolutely no guarantee in which order the functions are called. Thus it is not allowed to access any data other than the components own data during that phase.
/// * Post-async phase: Another synchronous phase like the pre-async phase.
/// * Actual deletion of dead objects and components are done now.
/// * Transform update: The global transformation of dynamic objects is updated.
/// * Post-transform phase: Another synchronous phase like the pre-async phase after the transformation has been updated.
class XII_CORE_DLL xiiWorld final
{
public:
  /// Creates a new world with the given name.
  xiiWorld(xiiWorldDescription& ref_description);
  ~xiiWorld();

  /// Deletes all game objects in a world
  void Clear();

  /// Returns the name of this world.
  xiiStringView GetName() const;

  /// Returns the index of this world.
  xiiUInt32 GetIndex() const;

  /// \name Object Functions
  ///@{

  /// Create a new game object from the given description and returns a handle to it.
  xiiGameObjectHandle CreateObject(const xiiGameObjectDescription& desc); // [tested]

  /// Create a new game object from the given description, writes a pointer to it to out_pObject and returns a handle to it.
  xiiGameObjectHandle CreateObject(const xiiGameObjectDescription& desc, xiiGameObject*& out_pObject); // [tested]

  /// Deletes the given object, its children and all components.
  /// \note This function deletes the object immediately! It is unsafe to use this during a game update loop, as other objects
  /// may rely on this object staying valid for the rest of the frame.
  /// Use DeleteObjectDelayed() instead for safe removal at the end of the frame.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectNow(const xiiGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents = true); // [tested]

  /// Deletes the given object at the beginning of the next world update. The object and its components and children stay completely
  /// valid until then.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectDelayed(const xiiGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents = true); // [tested]

  /// Returns the event that is triggered before an object is deleted. This can be used for external systems to cleanup data
  /// which is associated with the deleted object.
  const xiiEvent<const xiiGameObject*>& GetObjectDeletionEvent() const;

  /// Returns whether the given handle corresponds to a valid object.
  bool IsValidObject(const xiiGameObjectHandle& hObject) const; // [tested]

  /// Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const xiiGameObjectHandle& hObject, xiiGameObject*& out_pObject); // [tested]

  /// Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const xiiGameObjectHandle& hObject, const xiiGameObject*& out_pObject) const; // [tested]

  /// Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, xiiGameObject*& out_pObject); // [tested]

  /// Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const xiiTempHashedString& sGlobalKey, const xiiGameObject*& out_pObject) const; // [tested]

  /// Searches for an object by path. Can find objects through a global key and/or relative to an object. May also search for an object that has a certain component.
  ///
  /// The syntax for \a sSearchPath is as follows:
  /// * All pieces of the path must be separated by slashes (/)
  /// * If it starts with "G:" then it will search for an object via global key.
  ///   Ie "G:keyname" will search for an object with the global key "keyname".
  ///   If no such object exists, the search fails. Otherwise this object becomes the reference object for the remainder.
  ///   If pReferenceObject was nullptr to begin with, sSearchPath MUST start with a global key search.
  /// * If it starts (or continues) with "P:" it will search "upwards" from the reference object to find the closest parent object that has the requested name.
  ///   Ie "P:parentname" will check the reference object's name for "parentname", if it doesn't match, it checks the parent object, and so on, until it either fails, because no such named parent exists, or it finds an object.
  /// * If the path starts (or continues) with "../" it will go to the parent object of the current object.
  ///   This is repeated for every occurrance of "../".
  ///   The search fails, if there are not enough parent objects.
  /// * If the path starts (or continues) with a relative path (e.g. "a/b") it then searches for any direct or indirect child called "a" and below that a direct or indirect object with name "b".
  ///   If that is found, and pExpectedComponent is not nullptr, it is checked whether the found object has that component type attached. If so, the search terminates successfully. Otherwise the search continues for all other combinations of objects called "a" with decendants called "b" until it finds an object with that component type, or runs out of objects.
  ///
  /// Example paths:
  /// * "G:player" -> search for the object with the global key "player"
  /// * "P:root" -> starting at the reference object searches for the parent object called "root"
  /// * "../.." -> starting at the reference object, goes two levels up
  /// * "head/camera" -> starting at the reference object searches for a child object called "head" and from there searches for a child object called "camera"
  /// * "G:door1/P:frame" -> uses a global key to find the "door1" object and from there gets the parent object called "frame"
  /// * "G:door1/.." -> uses a global key to find the "door1" object and returns its parent object
  /// * "P:root/obj2" -> starting at the reference object searches for a parent called "root" and from there searches for a child object called "obj2"
  ///
  /// Invalid paths:
  /// * "P:name/G:key" -> "G:" must be the first part of the string.
  /// * "obj/../" -> ".." can only appear at the beginning of the relative path
  /// * "obj/P:name" -> "P:" must be at the very beginning or directly after "G:"
  [[nodiscard]] xiiGameObject* SearchForObject(xiiStringView sSearchPath, xiiGameObject* pReferenceObject = nullptr, const xiiRTTI* pExpectedComponent = nullptr); // [tested]

  /// const overload of SearchForObject()
  [[nodiscard]] const xiiGameObject* SearchForObject(xiiStringView sSearchPath, const xiiGameObject* pReferenceObject = nullptr, const xiiRTTI* pExpectedComponent = nullptr) const; // [tested]

  /// Returns the total number of objects in this world.
  xiiUInt32 GetObjectCount() const; // [tested]

  /// Returns an iterator over all objects in this world in no specific order.
  xiiInternal::WorldData::ObjectIterator GetObjects(); // [tested]

  /// Returns an iterator over all objects in this world in no specific order.
  xiiInternal::WorldData::ConstObjectIterator GetObjects() const; // [tested]

  /// Defines a visitor function that is called for every game-object when using the traverse method.
  /// The function takes a pointer to the game object as argument and returns a bool which indicates whether to continue (true) or abort
  /// (false) traversal.
  using VisitorFunc = xiiInternal::WorldData::VisitorFunc;

  enum TraversalMethod
  {
    BreadthFirst,
    DepthFirst
  };

  /// Traverses the game object tree starting at the top level objects and then recursively all children. The given callback function
  /// is called for every object.
  void Traverse(VisitorFunc visitorFunc, TraversalMethod method = DepthFirst); // [tested]

  ///@}
  /// \name Module Functions
  ///@{

  /// Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  template <typename ModuleType>
  ModuleType* GetOrCreateModule(); // [tested]

  /// Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  xiiWorldModule* GetOrCreateModule(const xiiRTTI* pRtti); // [tested]

  /// Deletes the module of the given type or derived types.
  template <typename ModuleType>
  void DeleteModule();

  /// Deletes the module of the given type or derived types.
  void DeleteModule(const xiiRTTI* pRtti);

  /// Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  ModuleType* GetModule();

  /// Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModule() const;

  /// Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModuleReadOnly() const;

  /// Returns the instance to the given module type or derived types.
  xiiWorldModule* GetModule(const xiiRTTI* pRtti);

  /// Returns the instance to the given module type or derived types.
  const xiiWorldModule* GetModule(const xiiRTTI* pRtti) const;

  ///@}
  /// \name Component Functions
  ///@{

  /// Creates an instance of the given component manager type or returns a pointer to an already existing instance.
  template <typename ManagerType>
  ManagerType* GetOrCreateComponentManager();

  /// Returns the component manager that handles the given rtti component type.
  xiiComponentManagerBase* GetOrCreateManagerForComponentType(const xiiRTTI* pComponentRtti);

  /// Deletes the component manager of the given type and all its components.
  template <typename ManagerType>
  void DeleteComponentManager();

  /// Returns the instance to the given component manager type.
  template <typename ManagerType>
  ManagerType* GetComponentManager();

  /// Returns the instance to the given component manager type.
  template <typename ManagerType>
  const ManagerType* GetComponentManager() const;

  /// Returns the component manager that handles the given rtti component type.
  xiiComponentManagerBase* GetManagerForComponentType(const xiiRTTI* pComponentRtti);

  /// Returns the component manager that handles the given rtti component type.
  const xiiComponentManagerBase* GetManagerForComponentType(const xiiRTTI* pComponentRtti) const;

  /// Checks whether the given handle references a valid component.
  bool IsValidComponent(const xiiComponentHandle& hComponent) const;

  /// Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const xiiComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const xiiComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// Explicitly delete TryGetComponent overload when handle type is not related to a pointer type given by out_pComponent.
  template <typename T, typename U, std::enable_if_t<!std::disjunction_v<std::is_base_of<U, T>, std::is_base_of<T, U>>, bool> = true>
  [[nodiscard]] bool TryGetComponent(const xiiTypedComponentHandle<T>& hComponent, U*& out_pComponent) = delete;

  /// Explicitly delete TryGetComponent overload when handle type is not related to a pointer type given by out_pComponent.
  template <typename T, typename U, std::enable_if_t<!std::disjunction_v<std::is_base_of<U, T>, std::is_base_of<T, U>>, bool> = true>
  [[nodiscard]] bool TryGetComponent(const xiiTypedComponentHandle<T>& hComponent, const U*& out_pComponent) const = delete;

  /// Creates a new component init batch.
  /// It is ensured that the Initialize function is called for all components in a batch before the OnSimulationStarted is called.
  /// If bMustFinishWithinOneFrame is set to false the processing of an init batch can be distributed over multiple frames if
  /// m_MaxComponentInitializationTimePerFrame in the world desc is set to a reasonable value.
  xiiComponentInitBatchHandle CreateComponentInitBatch(xiiStringView sBatchName, bool bMustFinishWithinOneFrame = true);

  /// Deletes a component init batch. It must be completely processed before it can be deleted.
  void DeleteComponentInitBatch(const xiiComponentInitBatchHandle& hBatch);

  /// All components that are created between an BeginAddingComponentsToInitBatch/EndAddingComponentsToInitBatch scope are added to the
  /// given init batch.
  void BeginAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& hBatch);

  /// End adding components to the given batch. Components created after this call are added to the default init batch.
  void EndAddingComponentsToInitBatch(const xiiComponentInitBatchHandle& hBatch);

  /// After all components have been added to the init batch call submit to start processing the batch.
  void SubmitComponentInitBatch(const xiiComponentInitBatchHandle& hBatch);

  /// Returns whether the init batch has been completely processed and all corresponding components are initialized
  /// and their OnSimulationStarted function was called.
  bool IsComponentInitBatchCompleted(const xiiComponentInitBatchHandle& hBatch, double* pCompletionFactor = nullptr);

  /// Cancel the init batch if it is still active. This might leave outstanding components in an inconsistent state,
  /// so this function has be used with care.
  void CancelComponentInitBatch(const xiiComponentInitBatchHandle& hBatch);

  ///@}
  /// \name Message Functions
  ///@{

  /// Sends a message to all components of the receiverObject.
  void SendMessage(const xiiGameObjectHandle& hReceiverObject, xiiMessage& ref_msg);

  /// Sends a message to all components of the receiverObject and all its children.
  void SendMessageRecursive(const xiiGameObjectHandle& hReceiverObject, xiiMessage& ref_msg);

  /// Queues the message for the given phase. The message is send to the receiverObject after the given delay in the corresponding phase.
  void PostMessage(const xiiGameObjectHandle& hReceiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Queues the message for the given phase. The message is send to the receiverObject and all its children after the given delay in
  /// the corresponding phase.
  void PostMessageRecursive(const xiiGameObjectHandle& hReceiverObject, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Sends a message to the component.
  void SendMessage(const xiiComponentHandle& hReceiverComponent, xiiMessage& ref_msg);

  /// Queues the message for the given phase. The message is send to the receiverComponent after the given delay in the corresponding phase.
  void PostMessage(const xiiComponentHandle& hReceiverComponent, const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Finds the closest (parent) object, starting at pSearchObject, which has a xiiComponent that handles the given message and returns all
  /// matching components owned by that object. If a xiiEventMessageHandlerComponent is found the search is stopped even if it doesn't handle the given message.
  ///
  /// If no such parent object exists, it searches for all xiiEventMessageHandlerComponent instances that are set to 'handle global events'
  /// that handle messages of the given type.
  void FindEventMsgHandlers(const xiiMessage& msg, xiiGameObject* pSearchObject, xiiDynamicArray<xiiComponent*>& out_components);

  /// \copydoc xiiWorld::FindEventMsgHandlers()
  void FindEventMsgHandlers(const xiiMessage& msg, const xiiGameObject* pSearchObject, xiiDynamicArray<const xiiComponent*>& out_components) const;

  ///@}

  /// If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  void SetWorldSimulationEnabled(bool bEnable);

  /// If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  bool GetWorldSimulationEnabled() const;

  /// Updates the world by calling the various update methods on the component managers and also updates the transformation data of
  /// the game objects. See xiiWorld for a detailed description of the update phases.
  void Update(); // [tested]

  /// Returns a task implementation that calls Update on this world.
  const xiiSharedPtr<xiiTask>& GetUpdateTask();

  /// Returns the number of update calls. Can be used to determine whether an operation has already been done during a frame.
  xiiUInt32 GetUpdateCounter() const;

  /// Returns the spatial system that is associated with this world.
  xiiSpatialSystem* GetSpatialSystem();

  /// Returns the spatial system that is associated with this world.
  const xiiSpatialSystem* GetSpatialSystem() const;


  /// Returns the coordinate system for the given position.
  /// By default this always returns a coordinate system with forward = +X, right = +Y and up = +Z.
  /// This can be customized by setting a different coordinate system provider.
  void GetCoordinateSystem(const xiiVec3& vGlobalPosition, xiiCoordinateSystem& out_coordinateSystem) const; // [tested]

  /// Sets the coordinate system provider that should be used in this world.
  void SetCoordinateSystemProvider(const xiiSharedPtr<xiiCoordinateSystemProvider>& pProvider); // [tested]

  /// Returns the coordinate system provider that is associated with this world.
  xiiCoordinateSystemProvider& GetCoordinateSystemProvider(); // [tested]

  /// Returns the coordinate system provider that is associated with this world.
  const xiiCoordinateSystemProvider& GetCoordinateSystemProvider() const; // [tested]


  /// Returns the clock that is used for all updates in this game world
  xiiClock& GetClock(); // [tested]

  /// Returns the clock that is used for all updates in this game world
  const xiiClock& GetClock() const; // [tested]

  /// Accesses the default random number generator.
  /// If more control is desired, individual components should use their own RNG.
  xiiRandom& GetRandomNumberGenerator();


  /// Returns the allocator used by this world.
  xiiAllocator* GetAllocator();

  /// Returns the block allocator used by this world.
  xiiInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// Returns the stack allocator used by this world.
  xiiDoubleBufferedLinearAllocator* GetStackAllocator();

  /// Mark the world for reading by using XII_LOCK(world.GetReadMarker()). Multiple threads can read simultaneously if none is
  /// writing.
  xiiInternal::WorldData::ReadMarker& GetReadMarker() const; // [tested]

  /// Mark the world for writing by using XII_LOCK(world.GetWriteMarker()). Only one thread can write at a time.
  xiiInternal::WorldData::WriteMarker& GetWriteMarker(); // [tested]

  /// Allows re-setting the maximum time that is spent on component initialization per frame, which is first configured on construction.
  void SetMaxInitializationTimePerFrame(xiiTime maxInitTime);

  /// Associates the given user data with the world. The user is responsible for the life time of user data.
  void SetUserData(void* pUserData);

  /// Returns the associated user data.
  void* GetUserData() const;

  using ReferenceResolver = xiiDelegate<xiiGameObjectHandle(const void*, xiiComponentHandle hThis, xiiStringView sProperty)>;

  /// If set, this delegate can be used to map some data (GUID or string) to a xiiGameObjectHandle.
  ///
  /// Currently only used in editor settings, to create a runtime handle from a unique editor reference.
  void SetGameObjectReferenceResolver(const ReferenceResolver& resolver);

  /// \sa SetGameObjectReferenceResolver()
  const ReferenceResolver& GetGameObjectReferenceResolver() const;

  using ResourceReloadContext = xiiInternal::WorldData::ResourceReloadContext;
  using ResourceReloadFunc    = xiiInternal::WorldData::ResourceReloadFunc;

  /// Add a function that is called when the given resource has been reloaded.
  void AddResourceReloadFunction(xiiTypelessResourceHandle hResource, xiiComponentHandle hComponent, void* pUserData, ResourceReloadFunc function);
  void RemoveResourceReloadFunction(xiiTypelessResourceHandle hResource, xiiComponentHandle hComponent, void* pUserData);

  /// \name Helper methods to query xiiWorld limits.
  ///@{
  static constexpr xiiUInt64 GetMaxNumGameObjects();
  static constexpr xiiUInt64 GetMaxNumHierarchyLevels();
  static constexpr xiiUInt64 GetMaxNumComponentsPerType();
  static constexpr xiiUInt64 GetMaxNumWorldModules();
  static constexpr xiiUInt64 GetMaxNumComponentTypes();
  static constexpr xiiUInt64 GetMaxNumWorlds();
  ///@}

public:
  /// Returns the number of active worlds.
  static xiiUInt32 GetWorldCount();

  /// Returns the world with the given index.
  static xiiWorld* GetWorld(xiiUInt32 uiIndex);

  /// Returns the world for the given game object handle.
  static xiiWorld* GetWorld(const xiiGameObjectHandle& hObject);

  /// Returns the world for the given component handle.
  static xiiWorld* GetWorld(const xiiComponentHandle& hComponent);

private:
  friend class xiiGameObject;
  friend class xiiWorldModule;
  friend class xiiComponentManagerBase;
  friend class xiiComponent;

  XII_ALLOW_PRIVATE_PROPERTIES(xiiWorld);

  xiiGameObject* Reflection_TryGetObjectWithGlobalKey(xiiTempHashedString sGlobalKey);
  xiiClock*      Reflection_GetClock();
  xiiGameObject* Reflection_SearchForObject(xiiStringView sSearchPath, xiiGameObject* pReferenceObject) { return SearchForObject(sSearchPath, pReferenceObject, nullptr); }

  void CheckForReadAccess() const;
  void CheckForWriteAccess() const;

  xiiGameObject* GetObjectUnchecked(xiiUInt32 uiIndex) const;

  void SetParent(xiiGameObject* pObject, xiiGameObject* pNewParent, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::Enum::PreserveGlobal);
  void LinkToParent(xiiGameObject* pObject);
  void UnlinkFromParent(xiiGameObject* pObject);

  void          SetObjectGlobalKey(xiiGameObject* pObject, const xiiHashedString& sGlobalKey);
  xiiStringView GetObjectGlobalKey(const xiiGameObject* pObject) const;

  void PostMessage(const xiiGameObjectHandle& receiverObject, const xiiMessage& msg, xiiObjectMsgQueueType::Enum queueType, xiiTime delay, bool bRecursive) const;
  void ProcessQueuedMessage(const xiiInternal::WorldData::MessageQueue::Entry& entry);
  void ProcessQueuedMessages(xiiObjectMsgQueueType::Enum queueType);

  template <typename World, typename GameObject, typename Component>
  static void FindEventMsgHandlers(World& world, const xiiMessage& msg, GameObject pSearchObject, xiiDynamicArray<Component>& out_components);

  void RegisterUpdateFunction(const xiiWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const xiiWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunctions(xiiWorldModule* pModule);

  /// Used by component managers to queue a new component for initialization during the next update
  void AddComponentToInitialize(xiiComponentHandle hComponent);

  void UpdateFromThread();
  void UpdateSynchronous(const xiiArrayPtr<xiiInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();

  // Returns if the batch was completely initialized.
  bool      ProcessInitializationBatch(xiiInternal::WorldData::InitBatch& batch, xiiTime endTime);
  void      ProcessComponentsToInitialize();
  void      ProcessUpdateFunctionsToRegister();
  xiiResult RegisterUpdateFunctionInternal(const xiiWorldModule::UpdateFunctionDesc& desc);

  void DeleteDeadObjects();
  void DeleteDeadComponents();

  void PatchHierarchyData(xiiGameObject* pObject, xiiTransformPreservation::Enum preserve);
  void RecreateHierarchyData(xiiGameObject* pObject, bool bWasDynamic);

  void ProcessResourceReloadFunctions();

  bool ReportErrorWhenStaticObjectMoves() const;

  float GetInvDeltaSeconds() const;

  xiiSharedPtr<xiiTask> m_pUpdateTask;

  xiiInternal::WorldData m_Data;

  using QueuedMsgMetaData = xiiInternal::WorldData::QueuedMsgMetaData;

  xiiUInt32                                        m_uiIndex;
  static xiiStaticArray<xiiWorld*, XII_MAX_WORLDS> s_Worlds;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiWorld);

#include <Core/World/Implementation/World_inl.h>
