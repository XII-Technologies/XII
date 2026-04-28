/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiJointAttachmentComponentManager = xiiComponentManager<class xiiJointAttachmentComponent, xiiBlockStorageType::FreeList>;

/// \brief Used to expose an animated mesh's bone as a game object, such that objects can be attached to it to move along.
///
/// The animation system deals with bone animations internally.
/// Sometimes it is desirable to move certain objects along with a bone,
/// for example when a character should hold something in their hand.
///
/// This component references a bone by name, and takes care to position the owner object at the same location as the bone
/// whenever the animation pose changes.
/// Thus it is possible to attach other objects as child objects to this one, so that they move along as well.
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

  /// \brief Sets the bone name whose transform should be copied into this game object.
  void        SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;             // [ property ]

  /// \brief An additional local offset to be added to the transform.
  xiiVec3 m_vLocalPositionOffset = xiiVec3::MakeZero(); // [ property ]

  /// \brief An additional local offset to be added to the transform.
  xiiQuat m_vLocalRotationOffset = xiiQuat::MakeIdentity(); // [ property ]

protected:
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg); // [ msg handler ]

  xiiHashedString m_sJointToAttachTo;
  xiiUInt16       m_uiJointIndex = xiiInvalidJointIndex;
};
