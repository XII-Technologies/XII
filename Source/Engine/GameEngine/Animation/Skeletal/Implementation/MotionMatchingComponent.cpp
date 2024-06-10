#include <GameEngine/GameEnginePCH.h>

//#include <Core/Input/InputManager.h>
//#include <Core/WorldSerializer/WorldReader.h>
//#include <Core/WorldSerializer/WorldWriter.h>
//#include <GameEngine/Animation/Skeletal/MotionMatchingComponent.h>
//#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
//#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
//#include <GraphicsCore/Debug/DebugRenderer.h>
//#include <GraphicsFoundation/Device/Device.h>
//
//// clang-format off
//XII_BEGIN_COMPONENT_TYPE(xiiMotionMatchingComponent, 2, xiiComponentMode::Dynamic);
//{
//  XII_BEGIN_PROPERTIES
//  {
//    XII_ARRAY_ACCESSOR_PROPERTY("Animations", Animations_GetCount, Animations_GetValue, Animations_SetValue, Animations_Insert, Animations_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
//  }
//  XII_END_PROPERTIES;
//
//  XII_BEGIN_ATTRIBUTES
//  {
//      new xiiCategoryAttribute("Animation"),
//  }
//  XII_END_ATTRIBUTES;
//}
//XII_END_COMPONENT_TYPE
//// clang-format on
//
//xiiMotionMatchingComponent::xiiMotionMatchingComponent() = default;
//xiiMotionMatchingComponent::~xiiMotionMatchingComponent() = default;
//
//void xiiMotionMatchingComponent::SerializeComponent(xiiWorldWriter& stream) const
//{
//  SUPER::SerializeComponent(stream);
//  auto& s = stream.GetStream();
//
//  s.WriteArray(m_Animations);
//}
//
//void xiiMotionMatchingComponent::DeserializeComponent(xiiWorldReader& stream)
//{
//  SUPER::DeserializeComponent(stream);
//  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
//  auto& s = stream.GetStream();
//
//  if (uiVersion >= 2)
//  {
//    s.ReadArray(m_Animations);
//  }
//}
//
//void xiiMotionMatchingComponent::OnSimulationStarted()
//{
//  SUPER::OnSimulationStarted();
//
//  // make sure the skinning buffer is deleted
//  XII_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");
//
//  if (m_hMesh.IsValid())
//  {
//    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
//    m_hSkeleton = pMesh->GetSkeleton();
//  }
//
//  if (m_hSkeleton.IsValid())
//  {
//    xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
//
//    const xiiSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//    m_AnimationPose.Configure(skeleton);
//    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//    // m_SkinningMatrices = m_AnimationPose.GetAllTransforms();
//
//    // Create the buffer for the skinning matrices
//    xiiGALBufferCreationDescription BufferDesc;
//    BufferDesc.m_uiStructSize = sizeof(xiiMat4);
//    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
//    BufferDesc.m_bUseAsStructuredBuffer = true;
//    BufferDesc.m_bAllowShaderResourceView = true;
//    BufferDesc.m_ResourceAccess.m_bImmutable = false;
//
//    m_hSkinningTransformsBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(
//      BufferDesc, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
//  }
//
//  // m_AnimationClipSampler.RestartAnimation();
//
//  if (m_Animations.IsEmpty())
//    return;
//
//  m_Keyframe0.m_uiAnimClip = 0;
//  m_Keyframe0.m_uiKeyframe = 0;
//  m_Keyframe1.m_uiAnimClip = 0;
//  m_Keyframe1.m_uiKeyframe = 1;
//
//  for (xiiUInt32 anim = 0; anim < m_Animations.GetCount(); ++anim)
//  {
//    xiiResourceLock<xiiAnimationClipResource> pClip(m_Animations[anim], xiiResourceAcquireMode::BlockTillLoaded);
//    xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback);
//
//    PrecomputeMotion(m_MotionData, "Bip01_L_Foot", "Bip01_R_Foot", pClip->GetDescriptor(), anim, pSkeleton->GetDescriptor().m_Skeleton);
//  }
//
//  m_vLeftFootPos.SetZero();
//  m_vRightFootPos.SetZero();
//
//  ConfigureInput();
//}
//void xiiMotionMatchingComponent::ConfigureInput()
//{
//  xiiInputActionConfig iac;
//  iac.m_bApplyTimeScaling = false;
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_LeftStick_PosY;
//  iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyUp;
//  xiiInputManager::SetInputActionConfig("mm", "forward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_LeftStick_NegY;
//  iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyDown;
//  xiiInputManager::SetInputActionConfig("mm", "backward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_LeftStick_NegX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  xiiInputManager::SetInputActionConfig("mm", "left", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_LeftStick_PosX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  xiiInputManager::SetInputActionConfig("mm", "right", iac, true);
//
//  iac.m_bApplyTimeScaling = true;
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_RightStick_PosX;
//  iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyRight;
//  // iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyRight;
//  xiiInputManager::SetInputActionConfig("mm", "turnright", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = xiiInputSlot_Controller0_RightStick_NegX;
//  iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyLeft;
//  // iac.m_sInputSlotTrigger[1] = xiiInputSlot_KeyRight;
//  xiiInputManager::SetInputActionConfig("mm", "turnleft", iac, true);
//}
//
//xiiVec3 xiiMotionMatchingComponent::GetInputDirection() const
//{
//  float fw, bw, l, r;
//
//  xiiInputManager::GetInputActionState("mm", "forward", &fw);
//  xiiInputManager::GetInputActionState("mm", "backward", &bw);
//  xiiInputManager::GetInputActionState("mm", "left", &l);
//  xiiInputManager::GetInputActionState("mm", "right", &r);
//
//  xiiVec3 dir;
//  dir.y = -(fw - bw);
//  dir.x = r - l;
//  dir.z = 0;
//
//  // dir.NormalizeIfNotZero(xiiVec3::MakeZero());
//  return dir * 3.0f;
//}
//
//xiiQuat xiiMotionMatchingComponent::GetInputRotation() const
//{
//  float tl, tr;
//
//  xiiInputManager::GetInputActionState("mm", "turnleft", &tl);
//  xiiInputManager::GetInputActionState("mm", "turnright", &tr);
//
//  const xiiAngle turn = xiiAngle::MakeFromDegree((tr - tl) * 90.0f);
//
//  xiiQuat q;
//  q.SetFromAxisAndAngle(xiiVec3(0, 0, 1), turn);
//  return q;
//}
//
//void xiiMotionMatchingComponent::Update()
//{
//  if (!m_hSkeleton.IsValid() || m_Animations.IsEmpty())
//    return;
//
//  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback);
//  const xiiSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//
//  // xiiTransform rootMotion;
//  // rootMotion.SetIdentity();
//
//  const float fKeyframeFraction = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * 24.0f; // assuming 24 FPS in the animations
//
//  {
//    const xiiVec3 vTargetDir = GetInputDirection() / GetOwner()->GetGlobalScaling().x;
//
//    xiiStringBuilder tmp;
//    tmp.SetFormat("Gamepad: {0} / {1}", xiiArgF(vTargetDir.x, 1), xiiArgF(vTargetDir.y, 1));
//    xiiDebugRenderer::DrawInfoText(GetWorld(), tmp, xiiVec2I32(10, 10), xiiColor::White);
//
//    m_fKeyframeLerp += fKeyframeFraction;
//    while (m_fKeyframeLerp > 1.0f)
//    {
//
//      m_Keyframe0 = m_Keyframe1;
//      m_Keyframe1 = FindNextKeyframe(m_Keyframe1, vTargetDir);
//
//      // xiiLog::Info("Old KF: {0} | {1} - {2}", m_Keyframe0.m_uiAnimClip, m_Keyframe0.m_uiKeyframe, m_fKeyframeLerp);
//      m_fKeyframeLerp -= 1.0f;
//      // xiiLog::Info("New KF: {0} | {1} - {2}", m_Keyframe1.m_uiAnimClip, m_Keyframe1.m_uiKeyframe, m_fKeyframeLerp);
//    }
//  }
//
//  m_AnimationPose.SetToBindPoseInLocalSpace(skeleton);
//
//  {
//    xiiResourceLock<xiiAnimationClipResource> pAnimClip0(m_Animations[m_Keyframe0.m_uiAnimClip], xiiResourceAcquireMode::BlockTillLoaded);
//    xiiResourceLock<xiiAnimationClipResource> pAnimClip1(m_Animations[m_Keyframe1.m_uiAnimClip], xiiResourceAcquireMode::BlockTillLoaded);
//
//    const auto& animDesc0 = pAnimClip0->GetDescriptor();
//    const auto& animDesc1 = pAnimClip1->GetDescriptor();
//
//    const auto& animatedJoints0 = animDesc0.GetAllJointIndices();
//
//    for (xiiUInt32 b = 0; b < animatedJoints0.GetCount(); ++b)
//    {
//      const xiiHashedString sJointName = animatedJoints0.GetKey(b);
//      const xiiUInt32 uiAnimJointIdx0 = animatedJoints0.GetValue(b);
//      const xiiUInt32 uiAnimJointIdx1 = animDesc1.FindJointIndexByName(sJointName);
//
//      const xiiUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
//      if (uiSkeletonJointIdx != xiiInvalidJointIndex)
//      {
//        xiiArrayPtr<const xiiTransform> pTransforms0 = animDesc0.GetJointKeyframes(uiAnimJointIdx0);
//        xiiArrayPtr<const xiiTransform> pTransforms1 = animDesc1.GetJointKeyframes(uiAnimJointIdx1);
//
//        const xiiTransform jointTransform1 = pTransforms0[m_Keyframe0.m_uiKeyframe];
//        const xiiTransform jointTransform2 = pTransforms1[m_Keyframe1.m_uiKeyframe];
//
//        xiiTransform res;
//        res.m_vPosition = xiiMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, m_fKeyframeLerp);
//        res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, m_fKeyframeLerp);
//        res.m_vScale = xiiMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, m_fKeyframeLerp);
//
//        m_AnimationPose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
//      }
//    }
//
//    // root motion
//    {
//      auto* pOwner = GetOwner();
//
//      xiiVec3 vRootMotion0, vRootMotion1;
//      vRootMotion0.SetZero();
//      vRootMotion1.SetZero();
//
//      if (animDesc0.HasRootMotion())
//        vRootMotion0 = animDesc0.GetJointKeyframes(animDesc0.GetRootMotionJoint())[m_Keyframe0.m_uiKeyframe].m_vPosition;
//      if (animDesc1.HasRootMotion())
//        vRootMotion1 = animDesc1.GetJointKeyframes(animDesc1.GetRootMotionJoint())[m_Keyframe1.m_uiKeyframe].m_vPosition;
//
//      const xiiVec3 vRootMotion = xiiMath::Lerp(vRootMotion0, vRootMotion1, m_fKeyframeLerp) * fKeyframeFraction * pOwner->GetGlobalScaling().x;
//
//      const xiiQuat qRotate = GetInputRotation();
//
//      const xiiQuat qOldRot = pOwner->GetLocalRotation();
//      const xiiVec3 vNewPos = qOldRot * vRootMotion + pOwner->GetLocalPosition();
//      const xiiQuat qNewRot = qRotate * qOldRot;
//
//      pOwner->SetLocalPosition(vNewPos);
//      pOwner->SetLocalRotation(qNewRot);
//    }
//  }
//
//  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//  const xiiUInt16 uiLeftFootJoint = skeleton.FindJointByName("Bip01_L_Foot");
//  const xiiUInt16 uiRightFootJoint = skeleton.FindJointByName("Bip01_R_Foot");
//  if (uiLeftFootJoint != xiiInvalidJointIndex && uiRightFootJoint != xiiInvalidJointIndex)
//  {
//    xiiTransform tLeft, tRight;
//    xiiBoundingSphere sphere(xiiVec3::MakeZero(), 0.5f);
//
//    tLeft.SetFromMat4(m_AnimationPose.GetTransform(uiLeftFootJoint));
//    tRight.SetFromMat4(m_AnimationPose.GetTransform(uiRightFootJoint));
//
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiLeftFootJoint);
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiRightFootJoint);
//
//    // const float fScaleToPerSec = (float)(1.0 / GetWorld()->GetClock().GetTimeDiff().GetSeconds());
//
//    // const xiiVec3 vLeftFootVel = (tLeft.m_vPosition - m_vLeftFootPos) * fScaleToPerSec;
//    // const xiiVec3 vRightFootVel = (tRight.m_vPosition - m_vRightFootPos) * fScaleToPerSec;
//
//    m_vLeftFootPos = tLeft.m_vPosition;
//    m_vRightFootPos = tRight.m_vPosition;
//  }
//
//  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//  xiiArrayPtr<xiiMat4> pRenderMatrices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiMat4, m_AnimationPose.GetTransformCount());
//  xiiMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());
//
//  m_SkinningMatrices = pRenderMatrices;
//}
//
//void xiiMotionMatchingComponent::SetAnimation(xiiUInt32 uiIndex, const xiiAnimationClipResourceHandle& hResource)
//{
//  m_Animations.EnsureCount(uiIndex + 1);
//
//  m_Animations[uiIndex] = hResource;
//}
//
//xiiAnimationClipResourceHandle xiiMotionMatchingComponent::GetAnimation(xiiUInt32 uiIndex) const
//{
//  if (uiIndex >= m_Animations.GetCount())
//    return xiiAnimationClipResourceHandle();
//
//  return m_Animations[uiIndex];
//}
//
//xiiUInt32 xiiMotionMatchingComponent::Animations_GetCount() const
//{
//  return m_Animations.GetCount();
//}
//
//const char* xiiMotionMatchingComponent::Animations_GetValue(xiiUInt32 uiIndex) const
//{
//  const auto& hMat = GetAnimation(uiIndex);
//
//  if (!hMat.IsValid())
//    return "";
//
//  return hMat.GetResourceID();
//}
//
//void xiiMotionMatchingComponent::Animations_SetValue(xiiUInt32 uiIndex, const char* value)
//{
//  if (xiiStringUtils::IsNullOrEmpty(value))
//    SetAnimation(uiIndex, xiiAnimationClipResourceHandle());
//  else
//  {
//    auto hMat = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);
//    SetAnimation(uiIndex, hMat);
//  }
//}
//
//void xiiMotionMatchingComponent::Animations_Insert(xiiUInt32 uiIndex, const char* value)
//{
//  xiiAnimationClipResourceHandle hMat;
//
//  if (!xiiStringUtils::IsNullOrEmpty(value))
//    hMat = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);
//
//  m_Animations.Insert(hMat, uiIndex);
//}
//
//void xiiMotionMatchingComponent::Animations_Remove(xiiUInt32 uiIndex)
//{
//  m_Animations.RemoveAtAndCopy(uiIndex);
//}
//
//xiiMotionMatchingComponent::TargetKeyframe xiiMotionMatchingComponent::FindNextKeyframe(const TargetKeyframe& current, const xiiVec3& vTargetDir) const
//{
//  TargetKeyframe kf;
//  kf.m_uiAnimClip = current.m_uiAnimClip;
//  kf.m_uiKeyframe = current.m_uiKeyframe + 1;
//
//  {
//    // xiiResourceLock<xiiAnimationClipResource> pAnimClipCur(m_Animations[current.m_uiAnimClip], xiiResourceAcquireMode::NoFallback);
//    // const auto& animClip = pAnimClipCur->GetDescriptor();
//
//    // const xiiUInt32 uiLeftFootJoint = animClip.FindJointIndexByName("Bip01_L_Foot");
//    // const xiiUInt32 uiRightFootJoint = animClip.FindJointIndexByName("Bip01_R_Foot");
//
//    const xiiVec3 vLeftFootPos = m_vLeftFootPos;   // animClip.GetJointKeyframes(uiLeftFootJoint)[current.m_uiKeyframe].m_vPosition;
//    const xiiVec3 vRightFootPos = m_vRightFootPos; // animClip.GetJointKeyframes(uiRightFootJoint)[current.m_uiKeyframe].m_vPosition;
//
//    const xiiUInt32 uiBestMM = FindBestKeyframe(current, vLeftFootPos, vRightFootPos, vTargetDir);
//
//    TargetKeyframe nkf;
//    nkf.m_uiAnimClip = m_MotionData[uiBestMM].m_uiAnimClipIndex;
//    nkf.m_uiKeyframe = m_MotionData[uiBestMM].m_uiKeyframeIndex;
//
//    if ((nkf.m_uiAnimClip != kf.m_uiAnimClip) || (nkf.m_uiKeyframe != kf.m_uiKeyframe && nkf.m_uiKeyframe != current.m_uiKeyframe))
//    {
//      kf = nkf;
//    }
//  }
//
//  xiiResourceLock<xiiAnimationClipResource> pAnimClip(m_Animations[kf.m_uiAnimClip], xiiResourceAcquireMode::BlockTillLoaded);
//
//  if (kf.m_uiKeyframe >= pAnimClip->GetDescriptor().GetNumFrames())
//  {
//    // loop
//    kf.m_uiKeyframe = 0;
//  }
//
//  return kf;
//}
//
//void xiiMotionMatchingComponent::PrecomputeMotion(xiiDynamicArray<MotionData>& motionData, xiiTempHashedString jointName1, xiiTempHashedString jointName2,
//  const xiiAnimationClipResourceDescriptor& animClip, xiiUInt16 uiAnimClipIndex, const xiiSkeleton& skeleton)
//{
//  const xiiUInt16 uiRootJoint = animClip.HasRootMotion() ? animClip.GetRootMotionJoint() : 0xFFFFu;
//  // const xiiUInt16 uiJoint1IndexInAnim = animClip.FindJointIndexByName(jointName1);
//  // const xiiUInt16 uiJoint2IndexInAnim = animClip.FindJointIndexByName(jointName2);
//
//  const xiiUInt16 uiJoint1IndexInSkeleton = skeleton.FindJointByName(jointName1);
//  const xiiUInt16 uiJoint2IndexInSkeleton = skeleton.FindJointByName(jointName2);
//  if (uiJoint1IndexInSkeleton == xiiInvalidJointIndex || uiJoint2IndexInSkeleton == xiiInvalidJointIndex)
//    return;
//
//  const auto& jointNamesToIndices = animClip.GetAllJointIndices();
//
//  const xiiUInt32 uiFirstMotionDataIdx = motionData.GetCount();
//  motionData.Reserve(uiFirstMotionDataIdx + animClip.GetNumFrames());
//
//  const float fRootMotionToVelocity = animClip.GetFramesPerSecond();
//
//  xiiAnimationPose pose;
//  pose.Configure(skeleton);
//
//  for (xiiUInt16 uiFrameIdx = 0; uiFrameIdx < animClip.GetNumFrames(); ++uiFrameIdx)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//
//    for (xiiUInt32 b = 0; b < jointNamesToIndices.GetCount(); ++b)
//    {
//      const xiiUInt16 uiJointIndexInPose = skeleton.FindJointByName(jointNamesToIndices.GetKey(b));
//      if (uiJointIndexInPose != xiiInvalidJointIndex)
//      {
//        const xiiTransform jointTransform = animClip.GetJointKeyframes(jointNamesToIndices.GetValue(b))[uiFrameIdx];
//
//        pose.SetTransform(uiJointIndexInPose, jointTransform.GetAsMat4());
//      }
//    }
//
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    MotionData& md = motionData.ExpandAndGetRef();
//    md.m_vLeftFootPosition = pose.GetTransform(uiJoint1IndexInSkeleton).GetTranslationVector();
//    md.m_vRightFootPosition = pose.GetTransform(uiJoint2IndexInSkeleton).GetTranslationVector();
//    md.m_uiAnimClipIndex = uiAnimClipIndex;
//    md.m_uiKeyframeIndex = uiFrameIdx;
//    md.m_vLeftFootVelocity.SetZero();
//    md.m_vRightFootVelocity.SetZero();
//    md.m_vRootVelocity =
//      animClip.HasRootMotion() ? fRootMotionToVelocity * animClip.GetJointKeyframes(uiRootJoint)[uiFrameIdx].m_vPosition : xiiVec3::MakeZero();
//  }
//
//  // now compute the velocity
//  {
//    const float fScaleToVelPerSec = animClip.GetFramesPerSecond();
//
//    xiiUInt32 uiPrevMdIdx = motionData.GetCount() - 1;
//
//    for (xiiUInt32 uiMotionDataIdx = uiFirstMotionDataIdx; uiMotionDataIdx < motionData.GetCount(); ++uiMotionDataIdx)
//    {
//      {
//        xiiVec3 vel = motionData[uiMotionDataIdx].m_vLeftFootPosition - motionData[uiPrevMdIdx].m_vLeftFootPosition;
//        motionData[uiMotionDataIdx].m_vLeftFootVelocity = vel * fScaleToVelPerSec;
//      }
//      {
//        xiiVec3 vel = motionData[uiMotionDataIdx].m_vRightFootPosition - motionData[uiPrevMdIdx].m_vRightFootPosition;
//        motionData[uiMotionDataIdx].m_vRightFootVelocity = vel * fScaleToVelPerSec;
//      }
//
//      uiPrevMdIdx = uiMotionDataIdx;
//    }
//  }
//}
//
//xiiUInt32 xiiMotionMatchingComponent::FindBestKeyframe(
//  const TargetKeyframe& current, xiiVec3 vLeftFootPosition, xiiVec3 vRightFootPosition, xiiVec3 vTargetDir) const
//{
//  float fClosest = 1000000000.0f;
//  xiiUInt32 uiClosest = 0xFFFFFFFFu;
//
//  const float fDirWeight = 3.0f;
//
//  for (xiiUInt32 i = 0; i < m_MotionData.GetCount(); ++i)
//  {
//    const auto& md = m_MotionData[i];
//
//    float penaltyMul = 1.1f;
//    float penaltyAdd = 100;
//
//    if (md.m_uiAnimClipIndex == current.m_uiAnimClip)
//    {
//      // do NOT allow to transition backwards to a keyframe within a certain range
//      if (md.m_uiKeyframeIndex < current.m_uiKeyframe && md.m_uiKeyframeIndex + 10 > current.m_uiKeyframe)
//        continue;
//
//      penaltyMul = 1.0f;
//
//      if (md.m_uiKeyframeIndex == current.m_uiKeyframe)
//      {
//        penaltyAdd = 0;
//        penaltyMul = 0.9f;
//      }
//    }
//
//    const float dirDist = xiiMath::Pow((md.m_vRootVelocity - vTargetDir).GetLength(), fDirWeight);
//    const float leftFootDist = (md.m_vLeftFootPosition - vLeftFootPosition).GetLengthSquared();
//    const float rightFootDist = (md.m_vRightFootPosition - vRightFootPosition).GetLengthSquared();
//
//    const float fScore = dirDist + (leftFootDist + rightFootDist) * penaltyMul + penaltyAdd;
//
//    if (fScore < fClosest)
//    {
//      fClosest = fScore;
//      uiClosest = i;
//    }
//  }
//
//  return uiClosest;
//}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_MotionMatchingComponent);
