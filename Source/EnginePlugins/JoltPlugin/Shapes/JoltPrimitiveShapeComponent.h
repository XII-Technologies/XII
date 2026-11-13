/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Vec3.h>

/// A primitive uses the transform of its owning object relative to the actor when the body is built.
/// Attach one active primitive to the actor object or a descendant without an intervening actor.
class XII_JOLTPLUGIN_DLL xiiJoltShapeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, xiiComponent);

protected:
  void OnActivated() override;
  void OnDeactivated() override;

  void QueueBodyUpdate();
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
  xiiVec3 m_vDimensions = xiiVec3(1.0f);
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
  float m_fRadius = 0.5f;
};
