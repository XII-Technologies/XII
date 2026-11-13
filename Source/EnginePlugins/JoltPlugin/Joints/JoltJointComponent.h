/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Angle.h>

struct xiiJoltJointMotorMode
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Off,
    Velocity,
    Position,
    Default = Off
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltJointMotorMode);

/// Shared actor references and lifecycle for native two-body constraints.
/// An invalid ActorA is a world anchor; ActorB must reference an actor in the same world.
/// Local frame translations are scaled by the actor's scene scale when created. Frame scale itself must be one.
/// Frame X is the hinge/slider axis; frame Y defines the angular zero reference.
class XII_JOLTPLUGIN_DLL xiiJoltJointComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltJointComponent, xiiComponent);

public:
  const xiiPhysicsWorldModuleInterface::JointConfig& GetJointConfig() const { return m_Config; }
  void                                               SetJointConfig(const xiiPhysicsWorldModuleInterface::JointConfig& config);
  void                                               SetActorA(xiiGameObjectHandle hActor);
  void                                               SetActorB(xiiGameObjectHandle hActor);
  xiiGameObjectHandle                                GetActorA() const { return m_Config.m_hActorA; }
  xiiGameObjectHandle                                GetActorB() const { return m_Config.m_hActorB; }
  void                                               SetActorAReference(const char* szReference);
  void                                               SetActorBReference(const char* szReference);
  /// Reference strings are resolved by the world; runtime references are stored and serialized as handles.
  const char*         GetActorReference() const { return ""; }
  const xiiTransform& GetLocalFrameA() const { return m_Config.m_LocalFrameA; }
  const xiiTransform& GetLocalFrameB() const { return m_Config.m_LocalFrameB; }
  void                SetLocalFrameA(const xiiTransform& frame);
  void                SetLocalFrameB(const xiiTransform& frame);

  /// Captures the owner's world pose as both attachment frames at the current actor scene poses.
  [[nodiscard]] xiiResult CaptureJointFrames();
  /// Explicitly repairs a broken joint and requests native recreation.
  void RecreateJoint();
  bool IsBroken() const { return m_bBroken; }
  /// Breaking thresholds are forces in N and torques in N m; zero disables the threshold.
  float GetBreakForce() const { return m_fBreakForce; }
  void  SetBreakForce(float fValue);
  void  SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void  DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  void  OnSimulationStarted() override;
  void  OnDeactivated() override;
  void  QueueSettingsUpdate(bool bRecreate = false);
  float GetBreakTorqueValue() const { return m_fBreakTorque; }
  void  SetBreakTorqueValue(float fValue);

private:
  friend class xiiJoltPhysicsWorldModule;
  xiiPhysicsWorldModuleInterface::JointConfig m_Config;
  float                                       m_fBreakForce  = 0.0f;
  float                                       m_fBreakTorque = 0.0f;
  bool                                        m_bBroken      = false;
};

using xiiJoltFixedJointComponentManager = xiiComponentManager<class xiiJoltFixedJointComponent, xiiBlockStorageType::Compact>;
class XII_JOLTPLUGIN_DLL xiiJoltFixedJointComponent final : public xiiJoltJointComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltFixedJointComponent, xiiJoltJointComponent, xiiJoltFixedJointComponentManager);

public:
  xiiJoltFixedJointComponent();
  ~xiiJoltFixedJointComponent() override;
  float GetBreakTorque() const { return GetBreakTorqueValue(); }
  void  SetBreakTorque(float fValue) { SetBreakTorqueValue(fValue); }
};

using xiiJoltPointJointComponentManager = xiiComponentManager<class xiiJoltPointJointComponent, xiiBlockStorageType::Compact>;
/// Connects the two frame positions; rotation remains unconstrained.
class XII_JOLTPLUGIN_DLL xiiJoltPointJointComponent final : public xiiJoltJointComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltPointJointComponent, xiiJoltJointComponent, xiiJoltPointJointComponentManager);

public:
  xiiJoltPointJointComponent();
  ~xiiJoltPointJointComponent() override;
};

using xiiJoltHingeJointComponentManager = xiiComponentManager<class xiiJoltHingeJointComponent, xiiBlockStorageType::Compact>;
/// Local frame X is the hinge axis. Limits must contain zero and remain within [-pi, pi].
/// Target velocity is radians/second; maximum motor torque is N m. Only force breaking is supported.
class XII_JOLTPLUGIN_DLL xiiJoltHingeJointComponent final : public xiiJoltJointComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltHingeJointComponent, xiiJoltJointComponent, xiiJoltHingeJointComponentManager);

public:
  xiiJoltHingeJointComponent();
  ~xiiJoltHingeJointComponent() override;
  bool                           GetLimitsEnabled() const { return m_bLimitsEnabled; }
  void                           SetLimitsEnabled(bool value);
  xiiAngle                       GetLowerLimit() const { return m_LowerLimit; }
  void                           SetLowerLimit(xiiAngle value);
  xiiAngle                       GetUpperLimit() const { return m_UpperLimit; }
  void                           SetUpperLimit(xiiAngle value);
  xiiEnum<xiiJoltJointMotorMode> GetMotorMode() const { return m_MotorMode; }
  void                           SetMotorMode(xiiEnum<xiiJoltJointMotorMode> value);
  float                          GetTargetVelocity() const { return m_fTargetVelocity; }
  void                           SetTargetVelocity(float value);
  xiiAngle                       GetTargetAngle() const { return m_TargetAngle; }
  void                           SetTargetAngle(xiiAngle value);
  float                          GetMaxMotorTorque() const { return m_fMaxMotorTorque; }
  void                           SetMaxMotorTorque(float value);
  float                          GetMotorFrequency() const { return m_fMotorFrequency; }
  void                           SetMotorFrequency(float value);
  float                          GetMotorDamping() const { return m_fMotorDamping; }
  void                           SetMotorDamping(float value);
  void                           SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                           DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  bool                           m_bLimitsEnabled  = true;
  xiiAngle                       m_LowerLimit      = xiiAngle::MakeFromDegree(-90.0f);
  xiiAngle                       m_UpperLimit      = xiiAngle::MakeFromDegree(90.0f);
  xiiEnum<xiiJoltJointMotorMode> m_MotorMode       = xiiJoltJointMotorMode::Off;
  float                          m_fTargetVelocity = 0.0f;
  xiiAngle                       m_TargetAngle     = xiiAngle::MakeFromRadian(0.0f);
  float                          m_fMaxMotorTorque = 100.0f;
  float                          m_fMotorFrequency = 2.0f;
  float                          m_fMotorDamping   = 1.0f;
};

using xiiJoltSliderJointComponentManager = xiiComponentManager<class xiiJoltSliderJointComponent, xiiBlockStorageType::Compact>;
/// Local frame X is the translation axis. Limits must contain zero. Positions are meters, velocity meters/second.
class XII_JOLTPLUGIN_DLL xiiJoltSliderJointComponent final : public xiiJoltJointComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltSliderJointComponent, xiiJoltJointComponent, xiiJoltSliderJointComponentManager);

public:
  xiiJoltSliderJointComponent();
  ~xiiJoltSliderJointComponent() override;
  float                          GetBreakTorque() const { return GetBreakTorqueValue(); }
  void                           SetBreakTorque(float fValue) { SetBreakTorqueValue(fValue); }
  bool                           GetLimitsEnabled() const { return m_bLimitsEnabled; }
  void                           SetLimitsEnabled(bool value);
  float                          GetLowerLimit() const { return m_fLowerLimit; }
  void                           SetLowerLimit(float value);
  float                          GetUpperLimit() const { return m_fUpperLimit; }
  void                           SetUpperLimit(float value);
  xiiEnum<xiiJoltJointMotorMode> GetMotorMode() const { return m_MotorMode; }
  void                           SetMotorMode(xiiEnum<xiiJoltJointMotorMode> value);
  float                          GetTargetVelocity() const { return m_fTargetVelocity; }
  void                           SetTargetVelocity(float value);
  float                          GetTargetPosition() const { return m_fTargetPosition; }
  void                           SetTargetPosition(float value);
  float                          GetMaxMotorForce() const { return m_fMaxMotorForce; }
  void                           SetMaxMotorForce(float value);
  float                          GetMotorFrequency() const { return m_fMotorFrequency; }
  void                           SetMotorFrequency(float value);
  float                          GetMotorDamping() const { return m_fMotorDamping; }
  void                           SetMotorDamping(float value);
  void                           SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void                           DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  bool                           m_bLimitsEnabled  = true;
  float                          m_fLowerLimit     = -1.0f;
  float                          m_fUpperLimit     = 1.0f;
  xiiEnum<xiiJoltJointMotorMode> m_MotorMode       = xiiJoltJointMotorMode::Off;
  float                          m_fTargetVelocity = 0.0f;
  float                          m_fTargetPosition = 0.0f;
  float                          m_fMaxMotorForce  = 100.0f;
  float                          m_fMotorFrequency = 2.0f;
  float                          m_fMotorDamping   = 1.0f;
};

using xiiJoltDistanceJointComponentManager = xiiComponentManager<class xiiJoltDistanceJointComponent, xiiBlockStorageType::Compact>;
/// Distances are meters. Equal minimum and maximum select fixed distance. Spring frequency zero uses hard limits.
class XII_JOLTPLUGIN_DLL xiiJoltDistanceJointComponent final : public xiiJoltJointComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltDistanceJointComponent, xiiJoltJointComponent, xiiJoltDistanceJointComponentManager);

public:
  xiiJoltDistanceJointComponent();
  ~xiiJoltDistanceJointComponent() override;
  float GetMinDistance() const { return m_fMinDistance; }
  void  SetMinDistance(float value);
  float GetMaxDistance() const { return m_fMaxDistance; }
  void  SetMaxDistance(float value);
  float GetSpringFrequency() const { return m_fSpringFrequency; }
  void  SetSpringFrequency(float value);
  float GetSpringDamping() const { return m_fSpringDamping; }
  void  SetSpringDamping(float value);
  void  SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void  DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  float m_fMinDistance     = 1.0f;
  float m_fMaxDistance     = 1.0f;
  float m_fSpringFrequency = 0.0f;
  float m_fSpringDamping   = 1.0f;
};
