#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameEngine/GameEngineDLL.h>

using xiiCameraShakeComponentManager = xiiComponentManagerSimple<class xiiCameraShakeComponent, xiiComponentUpdateType::WhenSimulating>;

class XII_GAMEENGINE_DLL xiiCameraShakeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCameraShakeComponent, xiiComponent, xiiCameraShakeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraShakeComponent

  xiiAngle m_MinShake;
  xiiAngle m_MaxShake;

public:
  xiiCameraShakeComponent();
  ~xiiCameraShakeComponent();

protected:
  void Update();

  void  GenerateKeyframe();
  float GetStrengthAtPosition() const;

  float    m_fLastStrength = 0.0f;
  xiiTime  m_ReferenceTime;
  xiiAngle m_Rotation;
  xiiQuat  m_qPrevTarget = xiiQuat::IdentityQuaternion();
  xiiQuat  m_qNextTarget = xiiQuat::IdentityQuaternion();
};
