#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using xiiRotorComponentManager = xiiComponentManagerSimple<class xiiRotorComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Applies a rotation to the game object that it is attached to.
///
/// The rotation may be endless, or limited to a certain amount of rotation.
/// It may also automatically turn around and accelerate and decelerate.
class XII_GAMEENGINE_DLL xiiRotorComponent : public xiiTransformComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRotorComponent, xiiTransformComponent, xiiRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRotorComponent

public:
  xiiRotorComponent();
  ~xiiRotorComponent();

  /// \brief How much to rotate before reaching the end and either stopping or turning around.
  /// Set to zero for endless rotation.
  xiiInt32 m_iDegreeToRotate = 0; // [ property ]

  /// \brief The acceleration to reach the target speed.
  float m_fAcceleration = 1.0f; // [ property ]

  /// \brief The deceleration to brake to zero speed before reaching the end rotation.
  float m_fDeceleration = 1.0f; // [ property ]

  /// \brief The axis around which to rotate. In local space of the game object.
  xiiEnum<xiiBasisAxis> m_Axis = xiiBasisAxis::PositiveZ; // [ property ]

  /// \brief How much the rotation axis may randomly deviate to not have all objects rotate the same way.
  xiiAngle m_AxisDeviation; // [ property ]

protected:
  void Update();

  xiiVec3 m_vRotationAxis = xiiVec3(0, 0, 1);
  xiiQuat m_qLastRotation = xiiQuat::MakeIdentity();
};
