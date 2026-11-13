/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>

using xiiJoltDynamicActorComponentManager = xiiComponentManager<class xiiJoltDynamicActorComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltDynamicActorComponent final : public xiiJoltActorComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltDynamicActorComponent, xiiJoltActorComponent, xiiJoltDynamicActorComponentManager);

public:
  xiiJoltDynamicActorComponent();
  ~xiiJoltDynamicActorComponent() override;

  /// Mass in kilograms. Zero derives mass and inertia from the shape and density.
  float GetMass() const { return m_fMass; }
  void  SetMass(float fMass);
  float GetDensity() const { return m_fDensity; }
  void  SetDensity(float fDensity);
  float GetGravityFactor() const { return m_fGravityFactor; }
  void  SetGravityFactor(float fFactor);
  float GetLinearDamping() const { return m_fLinearDamping; }
  void  SetLinearDamping(float fDamping);
  float GetAngularDamping() const { return m_fAngularDamping; }
  void  SetAngularDamping(float fDamping);
  bool  GetAllowSleeping() const { return m_bAllowSleeping; }
  void  SetAllowSleeping(bool bAllowSleeping);

  // Initial values are used when a new simulation body is created.
  const xiiVec3& GetInitialLinearVelocity() const { return m_vInitialLinearVelocity; }
  void           SetInitialLinearVelocity(const xiiVec3& vVelocity);
  const xiiVec3& GetInitialAngularVelocity() const { return m_vInitialAngularVelocity; }
  void           SetInitialAngularVelocity(const xiiVec3& vVelocity);
  bool           GetStartAsleep() const { return m_bStartAsleep; }
  void           SetStartAsleep(bool bStartAsleep) { m_bStartAsleep = bStartAsleep; }

  struct BodyState
  {
    xiiVec3 m_vPosition        = xiiVec3::MakeZero();
    xiiQuat m_qRotation        = xiiQuat::MakeIdentity();
    xiiVec3 m_vLinearVelocity  = xiiVec3::MakeZero();
    xiiVec3 m_vAngularVelocity = xiiVec3::MakeZero();
    bool    m_bActive          = false;
  };

  /// Live body operations require the world write marker. Velocities are in world space.
  [[nodiscard]] bool      TryGetBodyState(BodyState& out_state) const;
  [[nodiscard]] xiiResult SetLinearVelocity(const xiiVec3& vVelocity);
  [[nodiscard]] xiiResult SetAngularVelocity(const xiiVec3& vVelocity);
  [[nodiscard]] xiiResult ActivateBody();
  /// Applies a world-space force at the center of mass for the next fixed step.
  [[nodiscard]] xiiResult AddForce(const xiiVec3& vForce);
  /// Applies a world-space impulse at the center of mass.
  [[nodiscard]] xiiResult AddImpulse(const xiiVec3& vImpulse);
  bool                    IsBodyActive() const;

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  float   m_fMass                   = 0.0f;
  float   m_fDensity                = 1000.0f;
  float   m_fGravityFactor          = 1.0f;
  float   m_fLinearDamping          = 0.05f;
  float   m_fAngularDamping         = 0.05f;
  xiiVec3 m_vInitialLinearVelocity  = xiiVec3::MakeZero();
  xiiVec3 m_vInitialAngularVelocity = xiiVec3::MakeZero();
  bool    m_bAllowSleeping          = true;
  bool    m_bStartAsleep            = false;
};
