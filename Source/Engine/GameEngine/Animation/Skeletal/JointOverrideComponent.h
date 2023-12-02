#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiJointOverrideComponentManager = xiiComponentManager<class xiiJointOverrideComponent, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiJointOverrideComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJointOverrideComponent, xiiComponent, xiiJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJointOverrideComponent

public:
  xiiJointOverrideComponent();
  ~xiiJointOverrideComponent();

  void        SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;             // [ property ]

  bool m_bOverridePosition = false; // [ property ]
  bool m_bOverrideRotation = true;  // [ property ]
  bool m_bOverrideScale    = false; // [ property ]

protected:
  void OnAnimationPosePreparing(xiiMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  xiiHashedString   m_sJointToOverride;
  mutable xiiUInt16 m_uiJointIndex = xiiInvalidJointIndex;
};
