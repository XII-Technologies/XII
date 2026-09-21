/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>
#include <Core/World/SpatialData.h>

// Avoid conflicts with windows.h
#ifdef SendMessage
#  undef SendMessage
#endif

/// Defines during re-parenting what transform is going to be preserved.
struct xiiTransformPreservation
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    PreserveLocal,
    PreserveGlobal,

    Default = PreserveLocal
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiTransformPreservation);

/// This class represents an object inside the world.
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
  /// Iterates over all children of one object.
  class XII_CORE_DLL ConstChildIterator
  {
  public:
    const xiiGameObject& operator*() const;
    const xiiGameObject* operator->() const;

    operator const xiiGameObject*() const;

    /// Advances the iterator to the next child object. The iterator will not be valid anymore, if the last child is reached.
    void Next();

    /// Checks whether this iterator points to a valid object.
    bool IsValid() const;

    /// Shorthand for 'Next'
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

  /// Returns a handle to this object.
  xiiGameObjectHandle GetHandle() const;

  /// Makes this object and all its children dynamic. Dynamic objects might move during runtime.
  void MakeDynamic();

  /// Makes this object static. Static objects don't move during runtime.
  void MakeStatic();

  /// Returns whether this object is dynamic.
  bool IsDynamic() const;

  /// Returns whether this object is static.
  bool IsStatic() const;

  /// Sets the 'active flag' of the game object, which affects its final 'active state'.
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

  /// Checks whether the 'active flag' is set on this game object. Note that this does not mean that the game object is also in an 'active
  /// state'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// Checks whether this game object is in an active state.
  ///
  /// The active state is determined by the active state of the parent game object and the 'active flag' of this game object.
  /// Only if the parent game object is active (and thus all of its parent objects as well) and this game object has the active flag set,
  /// will this game object be active.
  ///
  /// \sa xiiGameObject::SetActiveFlag(), xiiComponent::IsActive()
  bool IsActive() const;

  /// Adds xiiObjectFlags::CreatedByPrefab to the object. See the flag for details.
  void SetCreatedByPrefab() { m_Flags.Add(xiiObjectFlags::CreatedByPrefab); }

  /// Checks whether the xiiObjectFlags::CreatedByPrefab flag is set on this object.
  bool WasCreatedByPrefab() const { return m_Flags.IsSet(xiiObjectFlags::CreatedByPrefab); }

  /// Adds xiiObjectFlags::HideShapeIcon to the object. See the flag for details.
  void SetHideShapeIcon() { m_Flags.Add(xiiObjectFlags::HideShapeIcon); }

  /// Checks whether the xiiObjectFlags::HideShapeIcon flag is set on this object.
  bool IsShapeIconHidden() const { return m_Flags.IsSet(xiiObjectFlags::HideShapeIcon); }

  /// Sets the name to identify this object. Does not have to be a unique name.
  void                   SetName(xiiStringView sName);
  void                   SetName(const xiiHashedString& sName);
  xiiStringView          GetName() const;
  const xiiHashedString& GetNameHashed() const;
  bool                   HasName(const xiiTempHashedString& sName) const;

  /// Sets the global key to identify this object. Global keys must be unique within a world.
  ///
  /// If two objects use the same global key, the last one that registers it will be the referenced object.
  /// To prevent warnings about overwriting global keys, first clear the global key on the previous object.
  void          SetGlobalKey(xiiStringView sGlobalKey);
  void          SetGlobalKey(const xiiHashedString& sGlobalKey);
  xiiStringView GetGlobalKey() const;

  /// Enables or disabled notification message 'xiiMsgChildrenChanged' when children are added or removed. The message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

  /// Enables or disabled notification message 'xiiMsgParentChanged' when the parent changes. The message is sent to this object only.
  void EnableParentChangesNotifications();
  void DisableParentChangesNotifications();

  /// Sets the parent of this object to the given.
  void SetParent(const xiiGameObjectHandle& hParent, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::PreserveGlobal);

  /// Gets the parent of this object or nullptr if this is a top-level object.
  xiiGameObject* GetParent();

  /// Gets the parent of this object or nullptr if this is a top-level object.
  const xiiGameObject* GetParent() const;

  /// Adds the given object as a child object.
  void AddChild(const xiiGameObjectHandle& hChild, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::PreserveGlobal);

  /// Adds the given objects as child objects.
  void AddChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::PreserveGlobal);

  /// Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const xiiGameObjectHandle& hChild, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::PreserveGlobal);

  /// Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiTransformPreservation::Enum preserve = xiiTransformPreservation::PreserveGlobal);

  /// Returns the number of children.
  xiiUInt32 GetChildCount() const;

  /// Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  xiiGameObject* FindChildByName(const xiiTempHashedString& sName, bool bRecursive = true); // [tested]

  /// Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  const xiiGameObject* FindChildByName(const xiiTempHashedString& sName, bool bRecursive = true) const; // [tested]

  /// Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  xiiGameObject* FindChildByPath(xiiStringView sPath); // [tested]

  /// Const overload of FindChildByPath()
  const xiiGameObject* FindChildByPath(xiiStringView sPath) const; // [tested]

  /// Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  xiiGameObject* SearchForChildByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent = nullptr); // [tested]

  /// Const overload of SearchForChildByNameSequence()
  const xiiGameObject* SearchForChildByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent = nullptr) const; // [tested]

  /// Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent, xiiDynamicArray<xiiGameObject*>& out_objects);

  xiiWorld*       GetWorld();
  const xiiWorld* GetWorld() const;


  /// Defines update behavior for global transforms when changing the local transform on a static game object
  enum class UpdateBehaviorIfStatic
  {
    None,              ///< Only sets the local transform, does not update
    UpdateImmediately, ///< Updates the hierarchy underneath the object immediately
  };

  /// Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void    SetLocalPosition(xiiVec3 vPosition);
  xiiVec3 GetLocalPosition() const;

  void    SetLocalRotation(xiiQuat qRotation);
  xiiQuat GetLocalRotation() const;

  void    SetLocalScaling(xiiVec3 vScaling);
  xiiVec3 GetLocalScaling() const;

  void  SetLocalUniformScaling(float fScaling);
  float GetLocalUniformScaling() const;

  xiiTransform GetLocalTransform() const;

  void    SetGlobalPosition(const xiiVec3& vPosition);
  xiiVec3 GetGlobalPosition() const;

  void    SetGlobalRotation(const xiiQuat& qRotation);
  xiiQuat GetGlobalRotation() const;

  void    SetGlobalScaling(const xiiVec3& vScaling);
  xiiVec3 GetGlobalScaling() const;

  void         SetGlobalTransform(const xiiTransform& transform);
  xiiTransform GetGlobalTransform() const;

  /// Last frame's global transform (only valid if XII_GAMEOBJECT_VELOCITY is set, otherwise the same as GetGlobalTransform())
  xiiTransform GetLastGlobalTransform() const;

  // Simd variants of above methods
  void                SetLocalPosition(const xiiSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdVec4f& GetLocalPositionSimd() const;

  void               SetLocalRotation(const xiiSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdQuat& GetLocalRotationSimd() const;

  void                SetLocalScaling(const xiiSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const xiiSimdVec4f& GetLocalScalingSimd() const;

  void         SetLocalUniformScaling(const xiiSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  xiiSimdFloat GetLocalUniformScalingSimd() const;

  xiiSimdTransform GetLocalTransformSimd() const;

  void                SetGlobalPosition(const xiiSimdVec4f& vPosition);
  const xiiSimdVec4f& GetGlobalPositionSimd() const;

  void               SetGlobalRotation(const xiiSimdQuat& qRotation);
  const xiiSimdQuat& GetGlobalRotationSimd() const;

  void                SetGlobalScaling(const xiiSimdVec4f& vScaling);
  const xiiSimdVec4f& GetGlobalScalingSimd() const;

  void                    SetGlobalTransform(const xiiSimdTransform& transform);
  const xiiSimdTransform& GetGlobalTransformSimd() const;

  const xiiSimdTransform& GetLastGlobalTransformSimd() const;

  /// Returns the 'forwards' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirForwards() const;
  /// Returns the 'right' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirRight() const;
  /// Returns the 'up' direction of the world's xiiCoordinateSystem, rotated into the object's global space
  xiiVec3 GetGlobalDirUp() const;

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  /// The last global transform is used to calculate the object's velocity. By default this is set automatically to the global transform of the last frame.
  ///
  /// It might make sense to manually override the last global transform to e.g. indicate an object has been teleported instead of moved.
  void SetLastGlobalTransform(const xiiSimdTransform& transform);

  /// Returns the linear velocity of the object in units per second. This is only guaranteed to be correct in the PostTransform phase.
  xiiVec3 GetLinearVelocity() const;

  /// Returns the angular velocity of the object in radians per second. This is only guaranteed to be correct in the PostTransform phase.
  xiiVec3 GetAngularVelocity() const;
#endif

  /// Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();

  /// Enables or disabled notification message 'xiiMsgTransformChanged' when this object is static and its transform changes.
  /// The notification message is sent to this object and thus also to all its components.
  void EnableStaticTransformChangesNotifications();
  void DisableStaticTransformChangesNotifications();


  xiiBoundingBoxSphere GetLocalBounds() const;
  xiiBoundingBoxSphere GetGlobalBounds() const;

  const xiiSimdBBoxSphere& GetLocalBoundsSimd() const;
  const xiiSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// Updates the global bounds immediately. Usually this done during the world update after the "Post-async" phase.
  /// Note that this function does not ensure that the global transform is up-to-date. Use UpdateGlobalTransformAndBounds if you want to update both.
  void UpdateGlobalBounds();

  /// Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// Returns a handle to the internal spatial data.
  xiiSpatialDataHandle GetSpatialData() const;

  /// Enables or disabled notification message 'xiiMsgComponentsChanged' when components are added or removed. The message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const xiiRTTI* pType, xiiComponent*& out_pComponent);

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const xiiRTTI* pType, const xiiComponent*& out_pComponent) const;

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(xiiDynamicArray<T*>& out_components);

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(xiiDynamicArray<const T*>& out_components) const;

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<xiiComponent*>& out_components);

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<const xiiComponent*>& out_components) const;

  /// Returns a list of all components attached to this object.
  xiiArrayPtr<xiiComponent* const> GetComponents();

  /// Returns a list of all components attached to this object.
  xiiArrayPtr<const xiiComponent* const> GetComponents() const;

  /// Returns the current version of components attached to this object.
  /// This version is increased whenever components are added or removed and can be used for cache validation.
  xiiUInt16 GetComponentVersion() const;


  /// Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(xiiMessage& ref_msg);

  /// Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(xiiMessage& ref_msg) const;

  /// Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(xiiMessage& ref_msg);

  /// Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(xiiMessage& ref_msg) const;


  /// Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Delivers a xiiEventMessage to the closest (parent) object containing a xiiEventMessageHandlerComponent.
  ///
  /// Regular SendMessage() and PostMessage() send a message directly to the target object (and all attached components).
  /// SendMessageRecursive() and PostMessageRecursive() send a message 'down' the graph to the target object and all children.
  ///
  /// In contrast, SendEventMessage() / PostEventMessage() bubble the message 'up' the graph.
  /// They do so by inspecting the chain of parent objects for the existence of a xiiEventMessageHandlerComponent
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
  ///        For instance, a trigger component would pass through itself.
  ///        A projectile component sending a 'take damage event' to the hit object, would also pass through itself (the projectile)
  ///        such that the handling code can detect which object was responsible for the damage (and using the xiiGameObject's team-ID,
  ///        it can detect which player fired the projectile).
  bool SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent);

  /// \copydoc xiiGameObject::SendEventMessage()
  bool SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent) const;

  /// \copydoc xiiGameObject::SendEventMessage()
  ///
  /// \param queueType In which update phase to deliver the message.
  /// \param delay An optional delay before delivering the message.
  void PostEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent, xiiTime delay, xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;


  /// Returns the tag set associated with this object.
  const xiiTagSet& GetTags() const;

  /// Sets the tag set associated with this object.
  void SetTags(const xiiTagSet& tags);

  /// Adds the given tag to the object's tags.
  void SetTag(const xiiTag& tag);

  /// Removes the given tag from the object's tags.
  void RemoveTag(const xiiTag& tag);

  /// Checks whether this object has the given tag.
  bool HasTag(const xiiTempHashedString& sTagName) const;

  /// Returns the 'team ID' that was given during creation (/see xiiGameObjectDescription)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const xiiUInt16& GetTeamID() const { return m_uiTeamID; }

  /// Changes the team ID for this object and all children recursively.
  void SetTeamID(xiiUInt16 uiId);

  /// Returns a random value that is chosen once during object creation and remains stable even throughout serialization.
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

  /// Overwrites the object's random seed value.
  ///
  /// See \a GetStableRandomSeed() for details.
  ///
  /// It should not be necessary to manually change this value, unless you want to make the seed deterministic according to a custom rule.
  void SetStableRandomSeed(xiiUInt32 uiSeed);

  /// Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  /// This can be used to adjust the update logic of objects.
  /// An invisible object may stop updating entirely. An indirectly visible object may reduce its update rate.
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  xiiVisibilityState::Enum GetVisibilityState(xiiUInt32 uiNumFramesBeforeInvisible = 5) const;

private:
  friend class xiiComponentManagerBase;
  friend class xiiGameObjectTest;

  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg) const;
  bool SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg);
  bool SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg) const;

  XII_ALLOW_PRIVATE_PROPERTIES(xiiGameObject);

  void Reflection_SetTag(xiiStringView sTagName);
  void Reflection_RemoveTag(xiiStringView sTagName);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void                                                  Reflection_AddChild(xiiGameObject* pChild);
  void                                                  Reflection_DetachChild(xiiGameObject* pChild);
  xiiHybridArray<xiiGameObject*, 8>                     Reflection_GetChildren() const;
  void                                                  Reflection_AddComponent(xiiComponent* pComponent);
  void                                                  Reflection_RemoveComponent(xiiComponent* pComponent);
  xiiHybridArray<xiiComponent*, NUM_INPLACE_COMPONENTS> Reflection_GetComponents() const;

  xiiGameObject* Reflection_FindChildByName(const xiiTempHashedString& sName, bool bRecursive) { return FindChildByName(sName, bRecursive); }
  xiiGameObject* Reflection_FindChildByPath(xiiStringView sPath) { return FindChildByPath(sPath); }

  xiiObjectMode::Enum Reflection_GetMode() const;
  void                Reflection_SetMode(xiiObjectMode::Enum mode);

  xiiGameObject* Reflection_GetParent() const;
  void           Reflection_SetGlobalPosition(const xiiVec3& vPosition);
  void           Reflection_SetGlobalRotation(const xiiQuat& qRotation);
  void           Reflection_SetGlobalScaling(const xiiVec3& vScaling);
  void           Reflection_SetGlobalTransform(const xiiTransform& transform);

  bool DetermineDynamicMode(xiiComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(xiiComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();
  void UpdateLastGlobalTransform();

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
    xiiSimdTransform m_lastGlobalTransform;
#endif

    xiiSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    xiiSimdBBoxSphere m_globalBounds;

    xiiSpatialDataHandle m_hSpatialData;
    xiiUInt32            m_uiSpatialDataCategoryBitmask;

    xiiUInt32 m_uiStableRandomSeed = 0;

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
    xiiUInt32 m_uiLastGlobalTransformUpdateCounter = 0;
#else
    xiiUInt32 m_uiPadding2[1];
#endif

    /// Recomputes the local transform from this object's global transform and, if available, the parent's global transform.
    void UpdateLocalTransform();

    /// Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// In case there is a parent transform it also recursively calls itself on the parent transform to ensure everything is up-to-date.
    void UpdateGlobalTransformRecursive(xiiUInt32 uiUpdateCounter);

    /// Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformNonRecursive(xiiUInt32 uiUpdateCounter);

    /// Updates the global transform by copying the object's local transform into the global transform.
    /// This is for objects that have no parent.
    void UpdateGlobalTransformWithoutParent(xiiUInt32 uiUpdateCounter);

    /// Updates the global transform by combining the parents global transform with this object's local transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformWithParent(xiiUInt32 uiUpdateCounter);

    void UpdateGlobalBounds(xiiSpatialSystem* pSpatialSystem);
    void UpdateGlobalBounds();
    void UpdateGlobalBoundsAndSpatialData(xiiSpatialSystem& ref_spatialSystem);

    void UpdateLastGlobalTransform(xiiUInt32 uiUpdateCounter);

    void RecreateSpatialData(xiiSpatialSystem& ref_spatialSystem);
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
