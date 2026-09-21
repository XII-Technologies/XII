/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class xiiWorld;

struct xiiWorldUpdatePhase
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    PreAsync,      ///< Update phase before any async update functions are called. This is the last phase where it is safe to modify the world (e.g. create or delete objects, add or remove components, etc.) without affecting the simulation.
    Async,         ///< Update phase where async update functions are called. During this phase, the world is considered to be in a "simulation step". This means that the state of the world should not be modified during this phase, as it may
    PostAsync,     ///< Update phase after all async update functions have been called. This is the first phase where it is safe to modify the world again after the simulation step.
    PostTransform, ///< Update phase after all transformations have been updated. This is the first phase where it is safe to read the final transformations of objects for this frame.
    COUNT,

    Default = PreAsync
  };
};

class XII_CORE_DLL xiiWorldModule : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldModule, xiiReflectedClass);

protected:
  xiiWorldModule(xiiWorld* pWorld);
  virtual ~xiiWorldModule();

public:
  /// Returns the corresponding world to this module.
  xiiWorld* GetWorld();

  /// Returns the corresponding world to this module.
  const xiiWorld* GetWorld() const;

  /// Same as GetWorld()->GetIndex(). Needed to break circular include dependencies.
  xiiUInt32 GetWorldIndex() const;

protected:
  friend class xiiWorld;
  friend class xiiInternal::WorldData;
  friend class xiiMemoryUtils;

  struct UpdateContext
  {
    xiiUInt32 m_uiFirstComponentIndex = 0;
    xiiUInt32 m_uiComponentCount      = 0;
  };

  /// Update function delegate.
  using UpdateFunction = xiiDelegate<void(const UpdateContext&)>;

  /// Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    UpdateFunctionDesc(const UpdateFunction& function, xiiStringView sFunctionName) :
      m_Function(function)
    {
      m_sFunctionName.Assign(sFunctionName);
    }

    UpdateFunction                     m_Function;                          ///< Delegate to the actual update function.
    xiiHashedString                    m_sFunctionName;                     ///< Name of the function. Use the XII_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description with the correct name.
    xiiHybridArray<xiiHashedString, 4> m_DependsOn;                         ///< Array of other functions on which this function depends on. This function will be called after all its dependencies have been called.
    xiiEnum<xiiWorldUpdatePhase>       m_Phase;                             ///< The update phase in which this update function should be called. See xiiWorld for a description on the different phases.
    bool                               m_bOnlyUpdateWhenSimulating = false; ///< The update function is only called when the world simulation is enabled.
    xiiUInt16                          m_uiAsyncPhaseBatchSize     = 0;     ///< Zero means m_Function is called once per frame, to update all components, but still in parallel with other world modules. Non-zero means m_Function is called multiple times (in parallel) with batches of roughly this size.
    float                              m_fPriority                 = 0.0f;  ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// Returns the allocator used by the world.
  xiiAllocator* GetAllocator();

  /// Returns the block allocator used by the world.
  xiiInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
  /// This method is called after the constructor. A derived type can override this method to do initialization work. Typically this
  /// is the method where updates function are registered.
  virtual void Initialize() {}

  /// This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual void Deinitialize() {}

  /// This method is called at the start of the next world update when the world is simulated. This method will be called after the
  /// initialization method.
  virtual void OnSimulationStarted() {}

  /// Called by xiiWorld::Clear(). Can be used to clear cached data when a world is completely cleared of objects (but not deleted).
  virtual void WorldClear() {}

  xiiWorld* const m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// Helper class to get component type ids and create new instances of world modules from rtti.
class XII_CORE_DLL xiiWorldModuleFactory
{
public:
  static xiiWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  xiiWorldModuleTypeId RegisterWorldModule();

  /// Returns the module type id to the given rtti module/component type.
  xiiWorldModuleTypeId GetTypeId(const xiiRTTI* pRtti);

  /// Creates a new instance of the world module with the given type id and world.
  xiiWorldModule* CreateWorldModule(xiiUInt16 uiTypeId, xiiWorld* pWorld);

  /// Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(xiiStringView sInterfaceName, xiiStringView sImplementationName);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  using CreatorFunc = xiiWorldModule* (*)(xiiAllocator*, xiiWorld*);

  xiiWorldModuleFactory();
  xiiWorldModuleTypeId RegisterWorldModule(const xiiRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const xiiPluginEvent& EventData);
  void        FillBaseTypeIds();
  void        ClearUnloadedTypeToIDs();
  void        AdjustBaseTypeId(const xiiRTTI* pParentRtti, const xiiRTTI* pRtti, xiiUInt16 uiParentTypeId);

  xiiHashTable<const xiiRTTI*, xiiWorldModuleTypeId> m_TypeToId;

  struct CreatorFuncContext
  {
    XII_DECLARE_POD_TYPE();

    CreatorFunc    m_Func;
    const xiiRTTI* m_pRtti;
  };

  xiiDynamicArray<CreatorFuncContext> m_CreatorFuncs;

  xiiHashTable<xiiString, xiiString> m_InterfaceImplementations;
};

/// Add this macro to the declaration of your module type.
#define XII_DECLARE_WORLD_MODULE()                       \
public:                                                  \
  static XII_ALWAYS_INLINE xiiWorldModuleTypeId TypeId() \
  {                                                      \
    return s_TypeId;                                     \
  }                                                      \
                                                         \
private:                                                 \
  static xiiWorldModuleTypeId s_TypeId;

/// Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define XII_IMPLEMENT_WORLD_MODULE(moduleType) \
  xiiWorldModuleTypeId moduleType::s_TypeId = xiiWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// Helper macro to create an update function description with proper name
#define XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) xiiWorldModule::UpdateFunctionDesc(xiiWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
