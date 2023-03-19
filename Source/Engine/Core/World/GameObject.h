#pragma once

/// \file

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

// Avoid conflicts with windows.h
#ifdef SendMessage
#  undef SendMessage
#endif

/// \brief This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// xiiGameObjectHandle instead.
///
/// \see xiiWorld
/// \see xiiComponent
/// \see xiiGameObjectHandle
class XII_CORE_DLL xiiGameObject final
{
private:
  enum
  {
#if XII_ENABLED(XII_PLATFORM_32BIT)
    NUM_INPLACE_COMPONENTS = 12
#else
    NUM_INPLACE_COMPONENTS = 6
#endif
  };

  friend class xiiWorld;
  friend class xiiInternal::WorldData;
  friend class xiiMemoryUtils;

  xiiGameObject();
  xiiGameObject(const xiiGameObject& other);
  ~xiiGameObject();

  void operator=(const xiiGameObject& other);

public:
  /// \brief Iterates over all children of one object.
  class XII_CORE_DLL ConstChildIterator
  {
  public:
    const xiiGameObject& operator*() const;
    const xiiGameObject* operator->() const;

    operator const xiiGameObject*() const;

    /// \brief Advances the iterator to the next child object. The iterator will not be valid anymore, if the last child is reached.
    void Next();

    /// \brief Checks whether this iterator points to a valid object.
    bool IsValid() const;

    /// \brief Shorthand for 'Next'
    void operator++();

  private:
    friend class xiiGameObject;

    ConstChildIterator(xiiGameObject* pObject, const xiiWorld* pWorld);

    xiiGameObject*  m_pObject = nullptr;
    const xiiWorld* m_pWorld  = nullptr;
  };

  class XII_CORE_DLL ChildIterator : public ConstChildIterator
  {
  public:
    xiiGameObject& operator*();
    xiiGameObject* operator->();

    operator xiiGameObject*();

  private:
    friend class xiiGameObject;

    ChildIterator(xiiGameObject* pObject, const xiiWorld* pWorld);
  };

  /// \brief Returns a handle to this object.
  xiiGameObjectHandle GetHandle() const;

  /// \brief Makes this object and all its children dynamic. Dynamic objects might move during runtime.
  void MakeDynamic();

  /// \brief Makes this object static. Static objects don't move during runtime.
  void MakeStatic();

  /// \brief Returns whether this object is dynamic.
  bool IsDynamic() const;

  /// \brief Returns whether this object is static.
  bool IsStatic() const;

  /// \brief Sets the 'active flag' of the game object, which affects its final 'active state'.
  ///
  /// The active flag affects the 'active state' of the game object and all its children and attached components.
  /// When a game object does not have the active flag, it is switched to 'inactive'. The same happens for all its children and
  /// all components attached to those game objects.
  /// Thus removing the active flag from a game object recursively deactivates the entire sub-tree of objects and components.
  ///
  /// When the active flag is set on a game object, and all of its parent nodes have the flag set as well, then the active state
  /// will be set to true on it and all its children and attached components.
  ///
  /// \sa IsActive(), xiiComponent::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// \brief Checks whether the 'active flag' is set on this game object. Note that this does not mean that the game object is also in an 'active
  /// state'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// \brief Checks whether this game object is in an active state.
  ///
  /// The active state is determined by the active state of the parent game object and the 'active flag' of this game object.
  /// Only if the parent game object is active (and thus all of its parent objects as well) and this game object has the active flag set,
  /// will this game object be active.
  ///
  /// \sa xiiGameObject::SetActiveFlag(), xiiComponent::IsActive()
  bool IsActive() const;

  /// \brief Sets the name to identify this object. Does not have to be a unique name.
  void          SetName(xiiStringView sName);
  void          SetName(const xiiHashedString& sName);
  xiiStringView GetName() const;
  bool          HasName(const xiiTempHashedString& name) const;

  /// \brief Sets the global key to identify this object. Global keys must be unique within a world.
  void          SetGlobalKey(xiiStringView sGlobalKey);
  void          SetGlobalKey(const xiiHashedString& sGlobalKey);
  xiiStringView GetGlobalKey() const;

  /// \brief Enables or disabled notification message 'xiiMsgChildrenChanged' when children are added or removed. The message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

  /// \brief Enables or disabled notification message 'xiiMsgParentChanged' when the parent changes. The message is sent to this object only.
  void EnableParentChangesNotifications();
  void DisableParentChangesNotifications();

  /// \brief Defines during re-parenting what transform is going to be preserved.
  enum class TransformPreservation
  {
    PreserveLocal,
    PreserveGlobal
  };

  /// \brief Sets the parent of this object to the given.
  void SetParent(const xiiGameObjectHandle& parent, xiiGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  xiiGameObject* GetParent();

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  const xiiGameObject* GetParent() const;

  /// \brief Adds the given object as a child object.
  void AddChild(const xiiGameObjectHandle& child, xiiGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Adds the given objects as child objects.
  void AddChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const xiiGameObjectHandle& child, xiiGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Returns the number of children.
  xiiUInt32 GetChildCount() const;

  /// \brief Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// \brief Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// \brief Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  xiiGameObject* FindChildByName(const xiiTempHashedString& name, bool bRecursive = true);

  /// \brief Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  xiiGameObject* FindChildByPath(xiiStringView sPath);

  /// \brief Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  xiiGameObject* SearchForChildByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent = nullptr);

  /// \brief Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent, xiiHybridArray<xiiGameObject*, 8>& out_Objects);

  xiiWorld*       GetWorld();
  const xiiWorld* GetWorld() const;


  /// \brief Defines update behavior for global transforms when changing the local transform on a static game object
  enum class UpdateBehaviorIfStatic
  {
    None,              ///< Only sets the local transform, does not update
    UpdateImmediately, ///< Updates the hierarchy underneath the object immediately
  };

  /// \brief Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void    SetLocalPosition(xiiVec3 position);
  xiiVec3 GetLocalPosition() const;

  void    SetLocalRotation(xiiQuat rotation);
  xiiQuat GetLocalRotation() const;

  void    SetLocalScaling(xiiVec3 scaling);
  xiiVec3 GetLocalScaling() const;

  void  SetLocalUniformScaling(float scaling);
  float GetLocalUniformScaling() const;

  xiiTransform GetLocalTransform() const;

  void    SetGlobalPosition(const xiiVec3& position);
  xiiVec3 GetGlobalPosition() const;

  void    SetGlobalRotation(const xiiQuat rotation);
  xiiQuat GetGlobalRotation() const;

  void    SetGlobalScaling(const xiiVec3 scaling);
  xiiVec3 GetGlobalScaling() const;

  void         SetGlobalTransform(const xiiTransform& transform);
  xiiTransform GetGlobalTransform() const;

  // Simd variants of above methods
  void                SetLocalPosition(const xiiSimdVec4f& position, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdVec4f& GetLocalPositionSimd() const;

  void               SetLocalRotation(const xiiSimdQuat& rotation, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdQuat& GetLocalRotationSimd() const;

  void                SetLocalScaling(const xiiSimdVec4f& scaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdVec4f& GetLocalScalingSimd() const;

  void         SetLocalUniformScaling(const xiiSimdFloat& scaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  xiiSimdFloat GetLocalUniformScalingSimd() const;

  xiiSimdTransform GetLocalTransformSimd() const;

  void                SetGlobalPosition(const xiiSimdVec4f& position);
  const xiiSimdVec4f& GetGlobalPositionSimd() const;

  void               SetGlobalRotation(const xiiSimdQuat& rotation);
  const xiiSimdQuat& GetGlobalRotationSimd() const;

  void                SetGlobalScaling(const xiiSimdVec4f& scaling);
  const xiiSimdVec4f& GetGlobalScalingSimd() const;

  void                    SetGlobalTransform(const xiiSimdTransform& transform);
  const xiiSimdTransform& GetGlobalTransformSimd() const;

  /// \brief Returns the 'forwards' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirForwards() const;
  /// \brief Returns the 'right' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirRight() const;
  /// \brief Returns the 'up' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirUp() const;

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  /// \brief Sets the object's velocity.
  ///
  /// This is used for some rendering techniques or for the computation of sound Doppler effect.
  /// It has no effect on the object's subsequent position.
  void SetVelocity(const xiiVec3& vVelocity);

  /// \brief Returns the velocity of the object in units per second. This is not only the diff between last frame's position and this
  /// frame's position, but
  ///        also the time difference is divided out.
  xiiVec3 GetVelocity() const;
#endif

  /// \brief Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();

  /// \brief Enables or disabled notification message 'xiiMsgTransformChanged' when this object is static and its transform changes.
  /// The notification message is sent to this object and thus also to all its components.
  void EnableStaticTransformChangesNotifications();
  void DisableStaticTransformChangesNotifications();


  xiiBoundingBoxSphere GetLocalBounds() const;
  xiiBoundingBoxSphere GetGlobalBounds() const;

  const xiiSimdBBoxSphere& GetLocalBoundsSimd() const;
  const xiiSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// \brief Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// \brief Updates the global bounds immediately. Usually this done during the world update after the "Post-async" phase.
  /// Note that this function does not ensure that the global transform is up-to-date. Use UpdateGlobalTransformAndBounds if you want to update both.
  void UpdateGlobalBounds();

  /// \brief Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// \brief Returns a handle to the internal spatial data.
  xiiSpatialDataHandle GetSpatialData() const;

  /// \brief Enables or disabled notification message 'xiiMsgComponentsChanged' when components are added or removed. The message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const xiiRTTI* pType, xiiComponent*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const xiiRTTI* pType, const xiiComponent*& out_pComponent) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(xiiDynamicArray<T*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(xiiDynamicArray<const T*>& out_components) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<xiiComponent*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<const xiiComponent*>& out_components) const;

  /// \brief Returns a list of all components attached to this object.
  xiiArrayPtr<xiiComponent* const> GetComponents();

  /// \brief Returns a list of all components attached to this object.
  xiiArrayPtr<const xiiComponent* const> GetComponents() const;

  /// \brief Returns the current version of components attached to this object.
  /// This version is increased whenever components are added or removed and can be used for cache validation.
  xiiUInt16 GetComponentVersion() const;


  /// \brief Sends a message to all components of this object.
  bool SendMessage(xiiMessage& msg);

  /// \brief Sends a message to all components of this object.
  bool SendMessage(xiiMessage& msg) const;

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(xiiMessage& msg);

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(xiiMessage& msg) const;


  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// \brief Delivers an xiiEventMessage to the closest (parent) object containing an xiiEventMessageHandlerComponent.
  ///
  /// Regular SendMessage() and PostMessage() send a message directly to the target object (and all attached components).
  /// SendMessageRecursive() and PostMessageRecursive() send a message 'down' the graph to the target object and all children.
  ///
  /// In contrast, SendEventMessage() / PostEventMessage() bubble the message 'up' the graph.
  /// They do so by inspecting the chain of parent objects for the existence of an xiiEventMessageHandlerComponent
  /// (typically a script component). If such a component is found, the message is delivered to it directly, and no other component.
  /// If it is found, but does not handle this type of message, the message is discarded and NOT tried to be delivered
  /// to anyone else.
  ///
  /// If no such component is found in all parent objects, the message is delivered to one xiiEventMessageHandlerComponent
  /// instances that is set to 'handle global events' (typically used for level-logic scripts), no matter where in the graph it resides.
  /// If multiple global event handler component exist that handle the same message type, the result is non-deterministic.
  ///
  /// \param msg The message to deliver.
  /// \param senderComponent The component that triggered the event in the first place. May be nullptr.
  ///        If not null, this information is stored in \a msg as xiiEventMessage::m_hSenderObject and xiiEventMessage::m_hSenderComponent.
  ///        This information is used to pass through more contextual information for the event handler.
  ///        For instance, a trigger would pass through which object entered the trigger.
  ///        A projectile component sending a 'take damage event' to the hit object, would pass through itself (the projectile)
  ///        such that the handling code can detect which object was responsible for the damage (and using the xiiGameObject's team-ID,
  ///        it can detect which player fired the projectile).
  void SendEventMessage(xiiMessage& msg, const xiiComponent* senderComponent);

  /// \copydoc xiiGameObject::SendEventMessage()
  void SendEventMessage(xiiMessage& msg, const xiiComponent* senderComponent) const;

  /// \copydoc xiiGameObject::SendEventMessage()
  ///
  /// \param queueType In which update phase to deliver the message.
  /// \param delay An optional delay before delivering the message.
  void PostEventMessage(xiiMessage& msg, const xiiComponent* pSenderComponent, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;


  /// \brief Returns the tag set associated with this object.
  const xiiTagSet& GetTags() const;

  /// \brief Sets the tag set associated with this object.
  void SetTags(const xiiTagSet& tags);

  /// \brief Adds the given tag to the object's tags.
  void SetTag(const xiiTag& tag);

  /// \brief Removes the given tag from the object's tags.
  void RemoveTag(const xiiTag& tag);

  /// \brief Returns the 'team ID' that was given during creation (/see xiiGameObjectDesc)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const xiiUInt16& GetTeamID() const { return m_uiTeamID; }

  /// \brief Changes the team ID for this object and all children recursively.
  void SetTeamID(xiiUInt16 id);

  /// \brief Returns a random value that is chosen once during object creation and remains stable even throughout serialization.
  ///
  /// This value is intended to be used for choosing random variations of components. For instance, if a component has two
  /// different meshes it can use for variation, this seed should be used to decide which one to use.
  ///
  /// The stable random seed can also be set from the outside, which is what the editor does, to assign a truly stable seed value.
  /// Therefore, each object placed in the editor will always have the same seed value, and objects won't change their appearance
  /// on every run of the game.
  ///
  /// The stable seed is also propagated through prefab instances, such that every prefab instance gets a different value, but
  /// in a deterministic fashion.
  xiiUInt32 GetStableRandomSeed() const;

  /// \brief Overwrites the object's random seed value.
  ///
  /// See \a GetStableRandomSeed() for details.
  ///
  /// It should not be necessary to manually change this value, unless you want to make the seed deterministic according to a custom rule.
  void SetStableRandomSeed(xiiUInt32 seed);

  /// \brief Returns the number of frames since this object was last visible in any view.
  ///
  /// This value can be used to skip update logic of invisible objects.
  xiiUInt64 GetNumFramesSinceVisible() const;

private:
  friend class xiiComponentManagerBase;
  friend class xiiGameObjectTest;

  // Only needed until reflection can deal with xiiStringView
  void        SetNameInternal(const char* szName);
  const char* GetNameInternal() const;

  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg) const;
  bool SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg);
  bool SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg) const;

  XII_ALLOW_PRIVATE_PROPERTIES(xiiGameObject);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void                                                  Reflection_AddChild(xiiGameObject* pChild);
  void                                                  Reflection_DetachChild(xiiGameObject* pChild);
  xiiHybridArray<xiiGameObject*, 8>                     Reflection_GetChildren() const;
  void                                                  Reflection_AddComponent(xiiComponent* pComponent);
  void                                                  Reflection_RemoveComponent(xiiComponent* pComponent);
  xiiHybridArray<xiiComponent*, NUM_INPLACE_COMPONENTS> Reflection_GetComponents() const;

  xiiObjectMode::Enum Reflection_GetMode() const;
  void                Reflection_SetMode(xiiObjectMode::Enum mode);

  bool DetermineDynamicMode(xiiComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(xiiComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();

  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);

  void AddComponent(xiiComponent* pComponent);
  void RemoveComponent(xiiComponent* pComponent);
  void FixComponentPointer(xiiComponent* pOldPtr, xiiComponent* pNewPtr);

  // Updates the active state of this object and all children and attached components recursively, depending on the enabled states.
  void UpdateActiveState(bool bParentActive);

  void SendNotificationMessage(xiiMessage& msg);

  struct XII_CORE_DLL alignas(16) TransformationData
  {
    XII_DECLARE_POD_TYPE();

    xiiGameObject*      m_pObject;
    TransformationData* m_pParentData;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt64 m_uiPadding;
#endif

    xiiSimdVec4f m_localPosition;
    xiiSimdQuat  m_localRotation;
    xiiSimdVec4f m_localScaling; // x,y,z = non-uniform scaling, w = uniform scaling

    xiiSimdTransform m_globalTransform;

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
    xiiSimdVec4f m_lastGlobalPosition;
    xiiSimdVec4f m_velocity; // w != 0 indicates custom velocity
#endif

    xiiSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    xiiSimdBBoxSphere m_globalBounds;

    xiiSpatialDataHandle m_hSpatialData;
    xiiUInt32            m_uiSpatialDataCategoryBitmask;

    xiiUInt32 m_uiStableRandomSeed = 0;

    xiiUInt32 m_uiPadding2[1];

    /// \brief Recomputes the local transform from this object's global transform and, if available, the parent's global transform.
    void UpdateLocalTransform();

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// In case there is a parent transform it also recursively calls itself on the parent transform to ensure everything is up-to-date.
    void UpdateGlobalTransformRecursive();

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformNonRecursive();

    /// \brief Updates the global transform by copying the object's local transform into the global transform.
    /// This is for objects that have no parent.
    void UpdateGlobalTransformWithoutParent();

    /// \brief Updates the global transform by combining the parents global transform with this object's local transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformWithParent();

    void UpdateGlobalBounds(xiiSpatialSystem* pSpatialSystem);
    void UpdateGlobalBounds();
    void UpdateGlobalBoundsAndSpatialData(xiiSpatialSystem& spatialSystem);

    void UpdateVelocity(const xiiSimdFloat& fInvDeltaSeconds);

    void RecreateSpatialData(xiiSpatialSystem& spatialSystem);
  };

  xiiGameObjectId m_InternalId;
  xiiHashedString m_sName;

#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiUInt32 m_uiNamePadding;
#endif

  xiiBitflags<xiiObjectFlags> m_Flags;

  xiiUInt32 m_uiParentIndex     = 0;
  xiiUInt32 m_uiFirstChildIndex = 0;
  xiiUInt32 m_uiLastChildIndex  = 0;

  xiiUInt32 m_uiNextSiblingIndex = 0;
  xiiUInt32 m_uiPrevSiblingIndex = 0;
  xiiUInt32 m_uiChildCount       = 0;

  xiiUInt16 m_uiHierarchyLevel = 0;

  /// An int that will be passed on to objects spawned from this one, which allows to identify which team or player it belongs to.
  xiiUInt16 m_uiTeamID = 0;

  TransformationData* m_pTransformationData = nullptr;

#if XII_ENABLED(XII_PLATFORM_32BIT)
  xiiUInt32 m_uiPadding = 0;
#endif

  xiiSmallArrayBase<xiiComponent*, NUM_INPLACE_COMPONENTS> m_Components;

  struct ComponentUserData
  {
    xiiUInt16 m_uiVersion;
    xiiUInt16 m_uiUnused;
  };

  xiiTagSet m_Tags;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGameObject);

#include <Core/World/Implementation/GameObject_inl.h>
