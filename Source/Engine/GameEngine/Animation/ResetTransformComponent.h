#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using xiiResetTransformComponentManager = xiiComponentManager<class xiiResetTransformComponent, xiiBlockStorageType::Compact>;

/// \brief This component sets the local transform of its owner to known values when the simulation starts.
///
/// This component is meant for use cases where an object may be activated and deactivated over and over.
/// For example due to a state machine switching between different object states by (de-)activating a sub-tree of objects.
///
/// Every time an object becomes active, it may want to start moving again from a fixed location.
/// This component helps with that, by reseting the local transform of its owner to such a fixed location once.
///
/// After that, it does nothing else, until it gets deactivated and reactivated again.
class XII_GAMEENGINE_DLL xiiResetTransformComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiResetTransformComponent, xiiComponent, xiiResetTransformComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiResetTransformComponent

public:
  xiiResetTransformComponent();
  ~xiiResetTransformComponent();

  xiiVec3 m_vLocalPosition       = xiiVec3::MakeZero();
  xiiQuat m_qLocalRotation       = xiiQuat::MakeIdentity();
  xiiVec3 m_vLocalScaling        = xiiVec3(1, 1, 1);
  float   m_fLocalUniformScaling = 1.0f;

  bool m_bResetLocalPositionX = true;
  bool m_bResetLocalPositionY = true;
  bool m_bResetLocalPositionZ = true;
  bool m_bResetLocalRotation  = true;
  bool m_bResetLocalScaling   = true;
};
