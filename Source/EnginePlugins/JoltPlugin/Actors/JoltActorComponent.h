/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/World/ComponentManager.h>

/// Common lifecycle and collision-layer assignment for Jolt rigid bodies.
class XII_JOLTPLUGIN_DLL xiiJoltActorComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltActorComponent, xiiComponent);

public:
  xiiUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }
  void     SetCollisionLayer(xiiUInt8 uiCollisionLayer);

  bool GetReportContacts() const { return m_bReportContacts; }
  void SetReportContacts(bool bReportContacts);

  /// Default surface for colliders without an override. Jolt's friction coefficient maps to dynamic friction.
  const xiiSurfaceResourceHandle& GetSurface() const { return m_hSurface; }
  void                            SetSurface(const xiiSurfaceResourceHandle& hSurface);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  void OnSimulationStarted() override;
  void OnDeactivated() override;

  void QueueBodyUpdate();

private:
  friend class xiiJoltShapeComponent;

  static void QueueAncestorBodyUpdate(xiiGameObject* pObject);
  void        QueueSiblingBodyUpdates();

  xiiUInt8                 m_uiCollisionLayer = 0;
  bool                     m_bReportContacts  = false;
  xiiSurfaceResourceHandle m_hSurface;
};

using xiiJoltStaticActorComponentManager = xiiComponentManager<class xiiJoltStaticActorComponent, xiiBlockStorageType::Compact>;

/// Adds an immovable body using the object's world pose when the body is created.
class XII_JOLTPLUGIN_DLL xiiJoltStaticActorComponent final : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltStaticActorComponent, xiiJoltActorComponent, xiiJoltStaticActorComponentManager);

public:
  xiiJoltStaticActorComponent();
  ~xiiJoltStaticActorComponent() override;
};
