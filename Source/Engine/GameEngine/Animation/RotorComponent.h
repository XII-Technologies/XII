#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef xiiComponentManagerSimple<class xiiRotorComponent, xiiComponentUpdateType::WhenSimulating> xiiRotorComponentManager;

class XII_GAMEENGINE_DLL xiiRotorComponent : public xiiTransformComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRotorComponent, xiiTransformComponent, xiiRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRotorComponent

public:
  xiiRotorComponent();
  ~xiiRotorComponent();

  xiiInt32              m_iDegreeToRotate = 0;                       // [ property ]
  float                 m_fAcceleration   = 1.0f;                    // [ property ]
  float                 m_fDeceleration   = 1.0f;                    // [ property ]
  xiiEnum<xiiBasisAxis> m_Axis            = xiiBasisAxis::PositiveZ; // [ property ]
  xiiAngle              m_AxisDeviation;                             // [ property ]

protected:
  void Update();

  xiiVec3 m_vRotationAxis = xiiVec3(0, 0, 1);
  xiiQuat m_qLastRotation = xiiQuat::IdentityQuaternion();
};
