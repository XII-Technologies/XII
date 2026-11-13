/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

struct xiiJoltCharacterGroundState
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    OnGround,
    OnSteepGround,
    NotSupported,
    InAir,
    Default = InAir
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltCharacterGroundState);

using xiiJoltCharacterControllerComponentManager = xiiComponentManager<class xiiJoltCharacterControllerComponent, xiiBlockStorageType::Compact>;

/// Upright Z-up capsule with its origin at the feet. Dimensions are in world units and independent of visual scale.
/// Commands require world write access. Read live state under the world read marker.
class XII_JOLTPLUGIN_DLL xiiJoltCharacterControllerComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltCharacterControllerComponent, xiiComponent, xiiJoltCharacterControllerComponentManager);

public:
  xiiJoltCharacterControllerComponent();
  ~xiiJoltCharacterControllerComponent() override;

  /// Total capsule height, including both hemispheres. Native creation requires height >= 2 * radius.
  float     GetRadius() const { return m_fRadius; }
  void      SetRadius(float value);
  float     GetHeight() const { return m_fHeight; }
  void      SetHeight(float value);
  xiiUInt8  GetCollisionLayer() const { return m_uiCollisionLayer; }
  void      SetCollisionLayer(xiiUInt8 value);
  float     GetMass() const { return m_fMass; }
  void      SetMass(float value);
  float     GetMaxStrength() const { return m_fMaxStrength; }
  void      SetMaxStrength(float value);
  xiiAngle  GetMaxSlope() const { return m_MaxSlope; }
  void      SetMaxSlope(xiiAngle value);
  float     GetStepUpHeight() const { return m_fStepUpHeight; }
  void      SetStepUpHeight(float value);
  float     GetStepDownDistance() const { return m_fStepDownDistance; }
  void      SetStepDownDistance(float value);
  float     GetPenetrationRecoverySpeed() const { return m_fPenetrationRecoverySpeed; }
  void      SetPenetrationRecoverySpeed(float value);
  float     GetContactTolerance() const { return m_fContactTolerance; }
  void      SetContactTolerance(float value);
  float     GetContactPadding() const { return m_fContactPadding; }
  void      SetContactPadding(float value);
  xiiUInt32 GetMaxCollisionIterations() const { return m_uiMaxCollisionIterations; }
  void      SetMaxCollisionIterations(xiiUInt32 value);
  float     GetGravityFactor() const { return m_fGravityFactor; }
  void      SetGravityFactor(float value);

  /// Sets persistent desired horizontal speed in world space. Jump and gravity control the vertical speed.
  void SetMovementVelocity(const xiiVec3& vVelocity);
  /// Transforms a local movement vector by the owner's world rotation when the command is issued.
  void SetLocalMovementVelocity(const xiiVec3& vVelocity);
  /// Requests a jump at the next fixed step, provided the character has walkable support.
  void Jump(float fSpeed);
  void StopMovement();
  /// Places the feet at a world-space position and resets support and vertical velocity.
  void Teleport(const xiiVec3& vPosition);
  void SetSimulationEnabled(bool bEnabled);
  bool GetSimulationEnabled() const { return m_bSimulationEnabled; }

  struct State
  {
    xiiVec3                              m_vPosition       = xiiVec3::MakeZero();
    xiiVec3                              m_vVelocity       = xiiVec3::MakeZero(); ///< Actual displacement divided by the last fixed step.
    xiiVec3                              m_vGroundNormal   = xiiVec3::MakeZero();
    xiiVec3                              m_vGroundVelocity = xiiVec3::MakeZero();
    xiiGameObjectHandle                  m_hGroundObject;
    xiiEnum<xiiJoltCharacterGroundState> m_GroundState;
    bool                                 m_bTouchingDynamicBody = false;
  };

  [[nodiscard]] bool                TryGetState(State& out_state) const;
  bool                              IsGrounded() const;
  xiiJoltCharacterGroundState::Enum GetGroundState() const;
  xiiVec3                           GetPosition() const;
  xiiVec3                           GetVelocity() const;
  xiiVec3                           GetGroundNormal() const;
  xiiVec3                           GetGroundVelocity() const;

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  friend class xiiJoltPhysicsWorldModule;

  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void QueueCharacterUpdate();

  float     m_fRadius                   = 0.35f;
  float     m_fHeight                   = 1.8f;
  xiiUInt8  m_uiCollisionLayer          = 0;
  float     m_fMass                     = 70.0f;
  float     m_fMaxStrength              = 100.0f;
  xiiAngle  m_MaxSlope                  = xiiAngle::MakeFromDegree(50.0f);
  float     m_fStepUpHeight             = 0.35f;
  float     m_fStepDownDistance         = 0.35f;
  float     m_fPenetrationRecoverySpeed = 1.0f;
  float     m_fContactTolerance         = 0.001f;
  float     m_fContactPadding           = 0.02f;
  xiiUInt32 m_uiMaxCollisionIterations  = 5U;
  float     m_fGravityFactor            = 1.0f;
  bool      m_bSimulationEnabled        = true;

  xiiVec3 m_vDesiredVelocity  = xiiVec3::MakeZero();
  xiiVec3 m_vTeleportPosition = xiiVec3::MakeZero();
  float   m_fJumpSpeed        = 0.0f;
  bool    m_bTeleportPending  = false;
};
