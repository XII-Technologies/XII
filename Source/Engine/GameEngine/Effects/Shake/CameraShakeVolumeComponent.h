#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgComponentInternalTrigger;
struct xiiMsgDeleteGameObject;

class XII_GAMEENGINE_DLL xiiCameraShakeVolumeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiCameraShakeVolumeComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraShakeVolumeComponent

public:
  xiiCameraShakeVolumeComponent();
  ~xiiCameraShakeVolumeComponent();

  static xiiSpatialData::Category SpatialDataCategory;

  xiiTime m_BurstDuration; // [ property ]
  float   m_fStrength;     // [ property ]

  float ComputeForceAtGlobalPosition(const xiiSimdVec4f& globalPos) const;

  virtual float ComputeForceAtLocalPosition(const xiiSimdVec4f& localPos) const = 0;

  xiiEnum<xiiOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using xiiCameraShakeVolumeSphereComponentManager = xiiComponentManager<class xiiCameraShakeVolumeSphereComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiCameraShakeVolumeSphereComponent : public xiiCameraShakeVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCameraShakeVolumeSphereComponent, xiiCameraShakeVolumeComponent, xiiCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraShakeVolumeSphereComponent

public:
  xiiCameraShakeVolumeSphereComponent();
  ~xiiCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const xiiSimdVec4f& localPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void  SetRadius(float val);                   // [ property ]

private:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);

  float        m_fRadius = 1.0f;
  xiiSimdFloat m_fOneDivRadius;
};
