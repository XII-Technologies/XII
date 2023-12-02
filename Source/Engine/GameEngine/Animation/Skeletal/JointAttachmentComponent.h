#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiJointAttachmentComponentManager = xiiComponentManager<class xiiJointAttachmentComponent, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiJointAttachmentComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJointAttachmentComponent, xiiComponent, xiiJointAttachmentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJointAttachmentComponent

public:
  xiiJointAttachmentComponent();
  ~xiiJointAttachmentComponent();

  void        SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;             // [ property ]

  xiiVec3 m_vLocalPositionOffset = xiiVec3::ZeroVector();         // [ property ]
  xiiQuat m_vLocalRotationOffset = xiiQuat::IdentityQuaternion(); // [ property ]

protected:
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg); // [ msg handler ]

  xiiHashedString m_sJointToAttachTo;
  xiiUInt16       m_uiJointIndex = xiiInvalidJointIndex;
};
