#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using xiiJointOverrideComponentManager = xiiComponentManager<class xiiJointOverrideComponent, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiJointOverrideComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJointOverrideComponent, xiiComponent, xiiJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

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
  void OnAnimationPosePreparing(xiiMsgAnimationPosePreparing& msg); // [ msg handler ]

  xiiHashedString m_sJointToOverride;
  xiiUInt16       m_uiJointIndex = xiiInvalidJointIndex;
};
