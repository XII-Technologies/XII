/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Types/UniquePtr.h>

/// A component manager that does no update at all on components and expects only a single instance to be created per world.
///
/// Easy access to this single component is provided through the GetSingletonComponent() function.
/// If a second component is created, the manager will log an error. The first created component will be used as the 'singleton',
/// all other components are ignored.
/// Use this for components derived from xiiSettingsComponent, of which one should only have zero or one per world.
template <typename ComponentType>
class xiiSettingsComponentManager : public xiiComponentManagerBase
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSettingsComponentManager);

public:
  xiiSettingsComponentManager(xiiWorld* pWorld);
  ~xiiSettingsComponentManager();

  /// Returns the first component of this type that has been created.
  ComponentType*       GetSingletonComponent();
  const ComponentType* GetSingletonComponent() const;

  static xiiWorldModuleTypeId TypeId();

  // xiiComponentManagerBase implementation
  virtual void CollectAllComponents(xiiDynamicArray<xiiComponentHandle>& out_allComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(xiiDynamicArray<xiiComponent*>& out_allComponents, bool bOnlyActive) override;

private:
  friend class xiiComponentManagerFactory;

  virtual xiiComponent* CreateComponentStorage() override;
  virtual void          DeleteComponentStorage(xiiComponent* pComponent, xiiComponent*& out_pMovedComponent) override;

  xiiHybridArray<xiiUniquePtr<ComponentType>, 2> m_Components;
};

#include <Core/World/Implementation/SettingsComponentManager_inl.h>
