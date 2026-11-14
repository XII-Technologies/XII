/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Transform.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>

using xiiJoltKinematicActorComponentManager = xiiComponentManager<class xiiJoltKinematicActorComponent, xiiBlockStorageType::Compact>;

/// By default the owner's world transform supplies the target. Velocity control instead writes the solved pose back to the owner.
/// Commands require world write access. Read live state under the world read marker.
class XII_JOLTPLUGIN_DLL xiiJoltKinematicActorComponent final : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltKinematicActorComponent, xiiJoltActorComponent, xiiJoltKinematicActorComponentManager);

public:
  xiiJoltKinematicActorComponent();
  ~xiiJoltKinematicActorComponent() override;

  struct BodyState
  {
    xiiVec3 m_vPosition        = xiiVec3::MakeZero();
    xiiQuat m_qRotation        = xiiQuat::MakeIdentity();
    xiiVec3 m_vLinearVelocity  = xiiVec3::MakeZero();
    xiiVec3 m_vAngularVelocity = xiiVec3::MakeZero();
  };

  /// Moves to a world-space target over the next batch of fixed steps. Scale is taken from the scene.
  void SetTargetTransform(const xiiTransform& transform);
  /// Prescribes world-space center-of-mass velocity in units per second and angular velocity in radians per second.
  void SetVelocity(const xiiVec3& vLinear, const xiiVec3& vAngular);
  /// Places the body at a world-space pose at the next physics update and clears its prescribed velocity.
  void               Teleport(const xiiTransform& transform);
  [[nodiscard]] bool TryGetBodyState(BodyState& out_state) const;

private:
  friend class xiiJoltPhysicsWorldModule;

  void OnDeactivated() override;

  xiiVec3 m_vLinearVelocity   = xiiVec3::MakeZero();
  xiiVec3 m_vAngularVelocity  = xiiVec3::MakeZero();
  xiiVec3 m_vTeleportPosition = xiiVec3::MakeZero();
  xiiQuat m_qTeleportRotation = xiiQuat::MakeIdentity();
  bool    m_bUseVelocity      = false;
  bool    m_bTeleportPending  = false;
};
