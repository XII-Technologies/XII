/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

struct xiiMsgComponentInternalTrigger;
using xiiTimedDeathComponentManager = xiiComponentManager<class xiiTimedDeathComponent, xiiBlockStorageType::Compact>;
using xiiPrefabResourceHandle       = xiiTypedResourceHandle<class xiiPrefabResource>;

/// This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the values has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class XII_GAMEENGINE_DLL xiiTimedDeathComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTimedDeathComponent, xiiComponent, xiiTimedDeathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  /// Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiTimedDeathComponent

public:
  xiiTimedDeathComponent();
  ~xiiTimedDeathComponent();

  xiiTime m_MinDelay   = xiiTime::MakeFromSeconds(1.0); // [ property ]
  xiiTime m_DelayRange = xiiTime::MakeFromSeconds(0.0); // [ property ]

  xiiPrefabResourceHandle m_hTimeoutPrefab; ///< [ property ] Spawned when the component is killed due to the timeout

protected:
  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
};
