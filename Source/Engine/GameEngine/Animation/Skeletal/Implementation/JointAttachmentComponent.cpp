/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointAttachmentComponent.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJointAttachmentComponent, 1, xiiComponentMode::Dynamic);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    XII_MEMBER_PROPERTY("PositionOffset", m_vLocalPositionOffset),
    XII_MEMBER_PROPERTY("RotationOffset", m_vLocalRotationOffset),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJointAttachmentComponent::xiiJointAttachmentComponent()  = default;
xiiJointAttachmentComponent::~xiiJointAttachmentComponent() = default;

void xiiJointAttachmentComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToAttachTo;
  s << m_vLocalPositionOffset;
  s << m_vLocalRotationOffset;
}

void xiiJointAttachmentComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToAttachTo;
  s >> m_vLocalPositionOffset;
  s >> m_vLocalRotationOffset;

  m_uiJointIndex = xiiInvalidJointIndex;
}

void xiiJointAttachmentComponent::SetJointName(const char* szName)
{
  m_sJointToAttachTo.Assign(szName);
  m_uiJointIndex = xiiInvalidJointIndex;
}

const char* xiiJointAttachmentComponent::GetJointName() const
{
  return m_sJointToAttachTo.GetData();
}

void xiiJointAttachmentComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg)
{
  if (m_uiJointIndex == xiiInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToAttachTo);
  }

  if (m_uiJointIndex == xiiInvalidJointIndex)
    return;

  xiiMat4 bone;
  xiiQuat boneRot;

  msg.ComputeFullBoneTransform(m_uiJointIndex, bone, boneRot);

  xiiGameObject* pOwner = GetOwner();
  pOwner->SetLocalPosition(bone.GetTranslationVector() + bone.TransformDirection(m_vLocalPositionOffset));
  pOwner->SetLocalRotation(boneRot * m_vLocalRotationOffset);
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointAttachmentComponent);
