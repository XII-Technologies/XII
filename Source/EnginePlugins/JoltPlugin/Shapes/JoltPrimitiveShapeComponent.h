/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <JoltPlugin/Resources/JoltCollisionMeshResource.h>

/// Shapes contribute to the nearest active ancestor actor. Local offsets are in the owning object's space.
class XII_JOLTPLUGIN_DLL xiiJoltShapeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, xiiComponent);

public:
  const xiiVec3&                  GetLocalPosition() const { return m_vLocalPosition; }
  void                            SetLocalPosition(const xiiVec3& vPosition);
  const xiiQuat&                  GetLocalRotation() const { return m_qLocalRotation; }
  void                            SetLocalRotation(const xiiQuat& qRotation);
  const xiiSurfaceResourceHandle& GetSurface() const { return m_hSurface; }
  void                            SetSurface(const xiiSurfaceResourceHandle& hSurface);
  void                            SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                            DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  void OnActivated() override;
  void OnSimulationStarted() override;
  void OnDeactivated() override;

  void QueueBodyUpdate();

private:
  xiiVec3                  m_vLocalPosition = xiiVec3::MakeZero();
  xiiQuat                  m_qLocalRotation = xiiQuat::MakeIdentity();
  xiiSurfaceResourceHandle m_hSurface;
};

using xiiJoltBoxShapeComponentManager = xiiComponentManager<class xiiJoltBoxShapeComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltBoxShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltBoxShapeComponent, xiiJoltShapeComponent, xiiJoltBoxShapeComponentManager);

public:
  xiiJoltBoxShapeComponent();
  ~xiiJoltBoxShapeComponent() override;

  /// Full box dimensions in the shape object's local space.
  const xiiVec3& GetDimensions() const { return m_vDimensions; }
  void           SetDimensions(const xiiVec3& vDimensions);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  xiiVec3 m_vDimensions = xiiVec3(1.0F);
};

using xiiJoltSphereShapeComponentManager = xiiComponentManager<class xiiJoltSphereShapeComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltSphereShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltSphereShapeComponent, xiiJoltShapeComponent, xiiJoltSphereShapeComponentManager);

public:
  xiiJoltSphereShapeComponent();
  ~xiiJoltSphereShapeComponent() override;

  float GetRadius() const { return m_fRadius; }
  void  SetRadius(float fRadius);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  float m_fRadius = 0.5F;
};

using xiiJoltCapsuleShapeComponentManager = xiiComponentManager<class xiiJoltCapsuleShapeComponent, xiiBlockStorageType::Compact>;
/// Z-up capsule. Height is the cylinder segment length, excluding the hemispheres.
class XII_JOLTPLUGIN_DLL xiiJoltCapsuleShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltCapsuleShapeComponent, xiiJoltShapeComponent, xiiJoltCapsuleShapeComponentManager);

public:
  xiiJoltCapsuleShapeComponent();
  ~xiiJoltCapsuleShapeComponent() override;
  float GetRadius() const { return m_fRadius; }
  void  SetRadius(float fValue);
  float GetHeight() const { return m_fHeight; }
  void  SetHeight(float fValue);
  void  SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void  DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  float m_fRadius = 0.5F;
  float m_fHeight = 1.0F;
};

using xiiJoltCylinderShapeComponentManager = xiiComponentManager<class xiiJoltCylinderShapeComponent, xiiBlockStorageType::Compact>;
/// Z-up cylinder. Height is the full end-to-end length.
class XII_JOLTPLUGIN_DLL xiiJoltCylinderShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltCylinderShapeComponent, xiiJoltShapeComponent, xiiJoltCylinderShapeComponentManager);

public:
  xiiJoltCylinderShapeComponent();
  ~xiiJoltCylinderShapeComponent() override;
  float GetRadius() const { return m_fRadius; }
  void  SetRadius(float fValue);
  float GetHeight() const { return m_fHeight; }
  void  SetHeight(float fValue);
  void  SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void  DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  float m_fRadius = 0.5F;
  float m_fHeight = 1.0F;
};

using xiiJoltConvexHullShapeComponentManager = xiiComponentManager<class xiiJoltConvexHullShapeComponent, xiiBlockStorageType::Compact>;
class XII_JOLTPLUGIN_DLL xiiJoltConvexHullShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltConvexHullShapeComponent, xiiJoltShapeComponent, xiiJoltConvexHullShapeComponentManager);

public:
  xiiJoltConvexHullShapeComponent();
  ~xiiJoltConvexHullShapeComponent() override;
  const xiiJoltCollisionMeshResourceHandle& GetCollisionMesh() const { return m_hCollisionMesh; }
  void                                      SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh);
  void                                      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                                      DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  xiiJoltCollisionMeshResourceHandle m_hCollisionMesh;
};

using xiiJoltMeshShapeComponentManager = xiiComponentManager<class xiiJoltMeshShapeComponent, xiiBlockStorageType::Compact>;
class XII_JOLTPLUGIN_DLL xiiJoltMeshShapeComponent final : public xiiJoltShapeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltMeshShapeComponent, xiiJoltShapeComponent, xiiJoltMeshShapeComponentManager);

public:
  xiiJoltMeshShapeComponent();
  ~xiiJoltMeshShapeComponent() override;
  const xiiJoltCollisionMeshResourceHandle& GetCollisionMesh() const { return m_hCollisionMesh; }
  void                                      SetCollisionMesh(const xiiJoltCollisionMeshResourceHandle& hMesh);
  void                                      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                                      DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  xiiJoltCollisionMeshResourceHandle m_hCollisionMesh;
};
