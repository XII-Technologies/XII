/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

class xiiMessage;
class xiiWorldWriter;
class xiiWorldReader;

// TODO: windows.h workaround
#ifdef SendMessage
#  undef SendMessage
#endif

/// Base class of all component types.
///
/// Derive from this class to implement custom component types. Also add the XII_DECLARE_COMPONENT_TYPE macro to your class declaration.
/// Also add a XII_BEGIN_COMPONENT_TYPE/XII_END_COMPONENT_TYPE block to a cpp file. In that block you can add reflected members or message
/// handlers. Note that every component type needs a corresponding manager type. Take a look at xiiComponentManagerSimple for a simple
/// manager implementation that calls an update method on its components every frame. To create a component instance call CreateComponent on
/// the corresponding manager. Never store a direct pointer to a component but store a xiiComponentHandle instead.
class XII_CORE_DLL xiiComponent : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiComponent, xiiReflectedClass);

protected:
  /// Keep the constructor private or protected in derived classes, so it cannot be called manually.
  xiiComponent();
  virtual ~xiiComponent();

public:
  /// Sets the active flag of the component, which affects its active state.
  ///
  /// The active flag affects the 'active state' of the component. Ie. a component without the active flag will always be inactive.
  /// However, the active state is also affected by the active state of the owning game object. Thus a component attached to an inactive
  /// game object, will also be inactive.
  ///
  /// Note that it is up to the component manager though, whether it differentiates between active and inactive components.
  ///
  /// \sa xiiGameObject::IsActive(), xiiGameObject::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// Checks whether the 'active flag' is set on this component. Note that this does not mean that the component is also 'active'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// Checks whether this component is in an active state.
  ///
  /// The active state is determined by the active state of the owning game object and the 'active flag' of this component.
  /// Only if the owning game object is active (and thus all of its parent objects as well) and the component has the active flag set,
  /// will this component be active.
  ///
  /// \sa xiiGameObject::IsActive(), xiiGameObject::SetActiveFlag()
  bool IsActive() const;

  /// Returns whether this component is active and initialized.
  ///
  /// \sa IsActive()
  bool IsActiveAndInitialized() const;

  /// Whether the component is currently active and simulation has been started as well.
  ///
  /// \sa IsActive()
  bool IsActiveAndSimulating() const;

  /// Returns the corresponding manager for this component.
  xiiComponentManagerBase* GetOwningManager();

  /// Returns the corresponding manager for this component.
  const xiiComponentManagerBase* GetOwningManager() const;

  /// Returns the owner game object if the component is attached to one or nullptr.
  xiiGameObject* GetOwner();

  /// Returns the owner game object if the component is attached to one or nullptr.
  const xiiGameObject* GetOwner() const;

  /// Returns the corresponding world for this component.
  xiiWorld* GetWorld();

  /// Returns the corresponding world for this component.
  const xiiWorld* GetWorld() const;


  /// Returns a handle to this component.
  xiiComponentHandle GetHandle() const;

  /// Returns the unique id for this component.
  xiiUInt32 GetUniqueID() const;

  /// Sets the unique id for this component.
  void SetUniqueID(xiiUInt32 uiUniqueID);


  /// Override this to save the current state of the component to the given stream.
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const;

  /// Override this to load the current state of the component from the given stream.
  ///
  /// The active state will be automatically serialized. The 'initialized' state is not serialized, all components
  /// will be initialized after creation, even if they were already in an initialized state when they were serialized.
  virtual void DeserializeComponent(xiiWorldReader& ref_stream);


  /// Ensures that the component is initialized. Must only be called from another component's Initialize callback.
  void EnsureInitialized();

  /// Ensures that the OnSimulationStarted method has been called. Must only be called from another component's OnSimulationStarted
  /// callback.
  void EnsureSimulationStarted();


  /// Sends a message to this component.
  XII_ALWAYS_INLINE bool SendMessage(xiiMessage& ref_msg) { return SendMessageInternal(ref_msg, false); }
  XII_ALWAYS_INLINE bool SendMessage(xiiMessage& ref_msg) const { return SendMessageInternal(ref_msg, false); }

  /// Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const xiiMessage& msg, xiiTime delay = xiiTime::MakeZero(), xiiObjectMsgQueueType::Enum queueType = xiiObjectMsgQueueType::NextFrame) const;

  /// Returns whether the given Message is handled by this component.
  virtual bool HandlesMessage(const xiiMessage& msg) const;

  /// Be careful to check which flags may already be in use by base classes.
  void SetUserFlag(xiiUInt8 uiFlagIndex, bool bSet);

  /// Retrieves a custom flag. Index must be between 0 and 7.
  bool GetUserFlag(xiiUInt8 uiFlagIndex) const;

  /// Adds xiiObjectFlags::CreatedByPrefab to the component. See the flag for details.
  void SetCreatedByPrefab() { m_ComponentFlags.Add(xiiObjectFlags::CreatedByPrefab); }

  /// Checks whether the xiiObjectFlags::CreatedByPrefab flag is set on this component.
  bool WasCreatedByPrefab() const { return m_ComponentFlags.IsSet(xiiObjectFlags::CreatedByPrefab); }

  /// Deletes this component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent();

protected:
  friend class xiiWorld;
  friend class xiiGameObject;
  friend class xiiComponentManagerBase;

  /// Returns whether this component is dynamic and thus can only be attached to dynamic game objects.
  bool IsDynamic() const;

  virtual xiiWorldModuleTypeId   GetTypeId() const = 0;
  virtual xiiComponentMode::Enum GetMode() const   = 0;

  /// Can be overridden for basic initialization that depends on a valid hierarchy and position.
  ///
  /// All trivial initialization should be done in the constructor.
  /// For typical game code, you should prefer to use OnSimulationStarted().
  /// This method is called once for every component, after creation but only at the start of the next world update.
  /// Therefore the global position has already been computed and the owner xiiGameObject is set.
  /// Contrary to OnActivated() and OnSimulationStarted(), this function is always called for all components.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void Initialize();

  /// This method is called before the component is destroyed. A derived type can override this method to do common de-initialization
  /// work.
  ///
  /// This function is always called before destruction, even if the component is currently not active.
  /// The default implementation checks whether the component is currently active and will ensure OnDeactivated() gets called if necessary.
  /// For typical game code, prefer to use OnDeactivated().
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void Deinitialize();

  /// This method is called when the component gets activated.
  ///
  /// By default a component is active, but it can be created in an inactive state. In such a case OnActivated() is only called once a
  /// component is activated. If a component gets switched between active and inactive at runtime, OnActivated() and OnDeactivated() are
  /// called accordingly. In contrast Initialize() and Deinitialize() are only ever called once.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnActivated();

  /// This method is called when the component gets deactivated.
  ///
  /// Upon destruction, a component that is active first gets deactivated. Therefore OnDeactivated() should be used for typical game code
  /// cleanup.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnDeactivated();

  /// This method is called once for active components, at the start of the next world update, but only when the world is simulated.
  ///
  /// This is the one preferred method to setup typical game logic. In a pure game environment there is no practical difference between
  /// OnActivated() and OnSimulationStarted(), as OnSimulationStarted() will be called right after OnActivated().
  ///
  /// However, when a scene is open inside the editor, there is an important difference:
  /// OnActivated() is called once the component was created.
  /// OnSimulationStarted() is only called once the game simulation is started inside the editor.
  /// As an example, if a component starts a sound in OnActivated(), that sound will play right after the scene has been loaded into the
  /// editor. If instead the sound gets started in OnSimulationStarted(), it will only play once the user starts the game mode inside the
  /// editor.
  ///
  /// Additionally, OnSimulationStarted() is only executed once, even if the xiiWorld pauses and resumes world simulation multiple times.
  /// However, note that it will be called again after the component has been deactivated and is activated again.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnSimulationStarted();

  /// By default disabled. Enable to have OnUnhandledMessage() called for every unhandled message.
  void EnableUnhandledMessageHandler(bool enable);

  /// When EnableUnhandledMessageHandler() was activated, this is called for all messages for which there is no dedicated message handler.
  ///
  /// \return Should return true if the given message was handled, false otherwise.
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg);

  /// When EnableUnhandledMessageHandler() was activated, this is called for all messages for which there is no dedicated message handler.
  ///
  /// \return Should return true if the given message was handled, false otherwise.
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const;

protected:
  /// Messages will be dispatched to this type. Default is what GetDynamicRTTI() returns, can be redirected if necessary.
  const xiiRTTI* m_pMessageDispatchType = nullptr;

  bool IsInitialized() const;
  bool IsInitializing() const;
  bool IsSimulationStarted() const;

private:
  // Updates the component's active state depending on the owner object's active state.
  void UpdateActiveState(bool bOwnerActive);

  xiiGameObject* Reflection_GetOwner() const;
  xiiWorld*      Reflection_GetWorld() const;
  void           Reflection_Update(xiiTime deltaTime);

  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg) const;

  xiiComponentId              m_InternalId;
  xiiBitflags<xiiObjectFlags> m_ComponentFlags = xiiObjectFlags::ActiveFlag;
  xiiUInt32                   m_uiUniqueID     = xiiInvalidIndex;

  xiiComponentManagerBase* m_pManager = nullptr;
  xiiGameObject*           m_pOwner   = nullptr;

  static xiiWorldModuleTypeId s_TypeId;
};

struct xiiComponent_ScriptBaseClassFunctions
{
  enum Enum
  {
    Initialize,
    Deinitialize,
    OnActivated,
    OnDeactivated,
    OnSimulationStarted,
    Update,

    Count
  };
};

#include <Core/World/Implementation/Component_inl.h>
