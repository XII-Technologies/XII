#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>

/// \brief Base class for all component managers. Do not derive directly from this class, but derive from xiiComponentManager instead.
///
/// Every component type has its corresponding manager type. The manager stores the components in memory blocks to minimize overhead
/// on creation and deletion of components. Each manager can also register update functions to update its components during
/// the different update phases of xiiWorld.
/// Use xiiWorld::CreateComponentManager to create an instance of a component manager within a specific world.
class XII_CORE_DLL xiiComponentManagerBase : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiComponentManagerBase, xiiWorldModule);

protected:
  xiiComponentManagerBase(xiiWorld* pWorld);
  virtual ~xiiComponentManagerBase();

public:
  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const xiiComponentHandle& hComponent) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const xiiComponentHandle& hComponent, xiiComponent*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const xiiComponentHandle& hComponent, const xiiComponent*& out_pComponent) const;

  /// \brief Returns the number of components managed by this manager.
  xiiUInt32 GetComponentCount() const;

  /// \brief Create a new component instance and returns a handle to it.
  xiiComponentHandle CreateComponent(xiiGameObject* pOwnerObject);

  /// \brief Create a new component instance and returns a handle to it.
  template <typename ComponentType>
  xiiTypedComponentHandle<ComponentType> CreateComponent(xiiGameObject* pOwnerObject, ComponentType*& out_pComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const xiiComponentHandle& hComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(xiiComponent* pComponent);

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a xiiComponentManagerBase pointer.
  virtual void CollectAllComponents(xiiDynamicArray<xiiComponentHandle>& out_allComponents, bool bOnlyActive) = 0;

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a xiiComponentManagerBase pointer.
  virtual void CollectAllComponents(xiiDynamicArray<xiiComponent*>& out_allComponents, bool bOnlyActive) = 0;

protected:
  /// \cond
  // Internal methods.
  friend class xiiWorld;
  friend class xiiInternal::WorldData;

  virtual void Deinitialize() override;

protected:
  friend class xiiWorldReader;

  xiiComponentHandle CreateComponentNoInit(xiiGameObject* pOwnerObject, xiiComponent*& out_pComponent);
  void               InitializeComponent(xiiComponent* pComponent);
  void               DeinitializeComponent(xiiComponent* pComponent);
  void               PatchIdTable(xiiComponent* pComponent);

  virtual xiiComponent* CreateComponentStorage()                                                             = 0;
  virtual void          DeleteComponentStorage(xiiComponent* pComponent, xiiComponent*& out_pMovedComponent) = 0;

  /// \endcond

  xiiIdTable<xiiComponentId, xiiComponent*> m_Components;
};

template <typename T, xiiBlockStorageType::Enum StorageType>
class xiiComponentManager : public xiiComponentManagerBase
{
public:
  using ComponentType = T;
  using SUPER         = xiiComponentManagerBase;

  /// \brief Although the constructor is public always use xiiWorld::CreateComponentManager to create an instance.
  xiiComponentManager(xiiWorld* pWorld);
  virtual ~xiiComponentManager();

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const xiiComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const xiiComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// \brief Returns an iterator over all components.
  typename xiiBlockStorage<ComponentType, xiiInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator GetComponents(xiiUInt32 uiStartIndex = 0);

  /// \brief Returns an iterator over all components.
  typename xiiBlockStorage<ComponentType, xiiInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator GetComponents(xiiUInt32 uiStartIndex = 0) const;

  /// \brief Returns the type id corresponding to the component type managed by this manager.
  static xiiWorldModuleTypeId TypeId();

  virtual void CollectAllComponents(xiiDynamicArray<xiiComponentHandle>& out_allComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(xiiDynamicArray<xiiComponent*>& out_allComponents, bool bOnlyActive) override;

protected:
  friend ComponentType;
  friend class xiiComponentManagerFactory;

  virtual xiiComponent* CreateComponentStorage() override;
  virtual void          DeleteComponentStorage(xiiComponent* pComponent, xiiComponent*& out_pMovedComponent) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  xiiBlockStorage<ComponentType, xiiInternal::DEFAULT_BLOCK_SIZE, StorageType> m_ComponentStorage;
};


//////////////////////////////////////////////////////////////////////////

struct xiiComponentUpdateType
{
  enum Enum
  {
    Always,
    WhenSimulating
  };
};

/// \brief Simple component manager implementation that calls an update method on all components every frame.
template <typename ComponentType, xiiComponentUpdateType::Enum UpdateType, xiiBlockStorageType::Enum StorageType = xiiBlockStorageType::FreeList>
class xiiComponentManagerSimple final : public xiiComponentManager<ComponentType, StorageType>
{
public:
  xiiComponentManagerSimple(xiiWorld* pWorld);

  virtual void Initialize() override;

  /// \brief A simple update function that iterates over all components and calls Update() on every component
  void SimpleUpdate(const xiiWorldModule::UpdateContext& context);

private:
  static void SimpleUpdateName(xiiStringBuilder& out_sName);
};

//////////////////////////////////////////////////////////////////////////

#define XII_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType)                                             \
public:                                                                                                                   \
  using ComponentManagerType = managerType;                                                                               \
  virtual xiiWorldModuleTypeId GetTypeId() const override                                                                 \
  {                                                                                                                       \
    return s_TypeId;                                                                                                      \
  }                                                                                                                       \
  static XII_ALWAYS_INLINE xiiWorldModuleTypeId TypeId()                                                                  \
  {                                                                                                                       \
    return s_TypeId;                                                                                                      \
  }                                                                                                                       \
  xiiTypedComponentHandle<componentType> GetHandle() const                                                                \
  {                                                                                                                       \
    return xiiTypedComponentHandle<componentType>(xiiComponent::GetHandle());                                             \
  }                                                                                                                       \
  virtual xiiComponentMode::Enum                GetMode() const override;                                                 \
  static xiiTypedComponentHandle<componentType> CreateComponent(xiiGameObject* pOwnerObject, componentType*& pComponent); \
  static void                                   DeleteComponent(componentType* pComponent);                               \
  void                                          DeleteComponent();                                                        \
                                                                                                                          \
private:                                                                                                                  \
  friend managerType;                                                                                                     \
  static xiiWorldModuleTypeId s_TypeId

#define XII_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType) \
public:                                                                   \
  virtual xiiWorldModuleTypeId GetTypeId() const override                 \
  {                                                                       \
    return xiiWorldModuleTypeId(-1);                                      \
  }                                                                       \
  static XII_ALWAYS_INLINE xiiWorldModuleTypeId TypeId()                  \
  {                                                                       \
    return xiiWorldModuleTypeId(-1);                                      \
  }

/// \brief Add this macro to a custom component type inside the type declaration.
#define XII_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  XII_ADD_DYNAMIC_REFLECTION(componentType, baseType);                   \
  XII_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType);

/// \brief Add this macro to a custom abstract component type inside the type declaration.
#define XII_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  XII_ADD_DYNAMIC_REFLECTION(componentType, baseType);               \
  XII_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType);


/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see XII_BEGIN_DYNAMIC_REFLECTED_TYPE
#define XII_BEGIN_COMPONENT_TYPE(componentType, version, mode)                                                                           \
  xiiWorldModuleTypeId componentType::s_TypeId =                                                                                         \
    xiiWorldModuleFactory::GetInstance()->RegisterWorldModule<typename componentType::ComponentManagerType, componentType>();            \
  xiiComponentMode::Enum componentType::GetMode() const                                                                                  \
  {                                                                                                                                      \
    return mode;                                                                                                                         \
  }                                                                                                                                      \
  xiiTypedComponentHandle<componentType> componentType::CreateComponent(xiiGameObject* pOwnerObject, componentType*& out_pComponent)     \
  {                                                                                                                                      \
    return pOwnerObject->GetWorld()->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pOwnerObject, out_pComponent); \
  }                                                                                                                                      \
  void componentType::DeleteComponent(componentType* pComponent)                                                                         \
  {                                                                                                                                      \
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());                                                            \
  }                                                                                                                                      \
  void componentType::DeleteComponent()                                                                                                  \
  {                                                                                                                                      \
    GetOwningManager()->DeleteComponent(GetHandle());                                                                                    \
  }                                                                                                                                      \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, xiiRTTINoAllocator)

/// \brief Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see XII_BEGIN_DYNAMIC_REFLECTED_TYPE
#define XII_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version) XII_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(componentType, version)

/// \brief Ends the component implementation code block that was opened with XII_BEGIN_COMPONENT_TYPE.
#define XII_END_COMPONENT_TYPE          XII_END_DYNAMIC_REFLECTED_TYPE
#define XII_END_ABSTRACT_COMPONENT_TYPE XII_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE

#include <Core/World/Implementation/ComponentManager_inl.h>
