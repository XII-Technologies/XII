#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Components/JoltRagdollComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO
 * max force clamping ?
 * mass distribution
 * communication with anim controller
 * drive to pose
 * shape scale
 */

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltRagdollStart, 1)
  XII_ENUM_CONSTANTS(xiiJoltRagdollStart::BindPose, xiiJoltRagdollStart::WaitForPose, xiiJoltRagdollStart::Wait)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltRagdollConstraint, 1, xiiRTTIDefaultAllocator<xiiJoltRagdollConstraint>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Bone", m_sBone),
    XII_MEMBER_PROPERTY("Position", m_vRelativePosition)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("Position")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltRagdollComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    XII_ENUM_MEMBER_PROPERTY("Start", xiiJoltRagdollStart, m_Start),
    XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_ARRAY_MEMBER_PROPERTY("Constraints", m_Constraints),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseProposal, OnAnimationPoseProposal),
    XII_MESSAGE_HANDLER(xiiMsgPhysicsAddForce, AddForceAtPos),
    XII_MESSAGE_HANDLER(xiiMsgPhysicsAddImpulse, AddImpulseAtPos),
    XII_MESSAGE_HANDLER(xiiMsgRetrieveBoneState, OnRetrieveBoneState),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiResult xiiJoltRagdollConstraint::Serialize(xiiStreamWriter& stream) const
{
  stream << m_sBone;
  stream << m_vRelativePosition;
  return XII_SUCCESS;
}

xiiResult xiiJoltRagdollConstraint::Deserialize(xiiStreamReader& stream)
{
  stream >> m_sBone;
  stream >> m_vRelativePosition;
  return XII_SUCCESS;
}

xiiJoltRagdollComponent::xiiJoltRagdollComponent()  = default;
xiiJoltRagdollComponent::~xiiJoltRagdollComponent() = default;

void xiiJoltRagdollComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Start;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_uiCollisionLayer;
  s.WriteArray(m_Constraints).AssertSuccess();
}

void xiiJoltRagdollComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = stream.GetStream();

  s >> m_Start;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_uiCollisionLayer;
  s.ReadArray(m_Constraints).AssertSuccess();
}

void xiiJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_Start == xiiJoltRagdollStart::BindPose)
  {
    SetupLimbsFromBindPose();
  }
}

void xiiJoltRagdollComponent::OnDeactivated()
{
  ClearPhysicsObjects();

  SUPER::OnDeactivated();
}

void xiiJoltRagdollComponent::Update()
{
  if (!m_bLimbsSetup)
    return;

  RetrievePhysicsPose();

  ApplyImpulse();
}

bool xiiJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    xiiMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  return m_hSkeleton.IsValid();
}

void xiiJoltRagdollComponent::ClearPhysicsObjects()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (m_pRagdollSettings)
  {
    m_pRagdollSettings->Release();
    m_pRagdollSettings = nullptr;
  }

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  pModule->DeallocateUserData(m_uiJoltUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  m_pJoltUserData = nullptr;

  m_Limbs.Clear();
  m_LimbPoses.Clear();
  m_bLimbsSetup = false;

  m_NextImpulse = {};
}

void xiiJoltRagdollComponent::SetGravityFactor(float factor)
{
  if (m_fGravityFactor == factor)
    return;

  m_fGravityFactor = factor;

  if (!m_pRagdoll)
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  for (xiiUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void xiiJoltRagdollComponent::AddForceAtPos(xiiMsgPhysicsAddForce& msg)
{
  // if (m_pPxAggregate != nullptr)
  //{
  //   XII_PX_WRITE_LOCK(*m_pPxAggregate->getScene());

  //  PxRigidBody* pBody = m_pPxRootBody;

  //  if (msg.m_pInternalPhysicsActor != nullptr)
  //    pBody = reinterpret_cast<PxRigidBody*>(msg.m_pInternalPhysicsActor);

  //  PxRigidBodyExt::addForceAtPos(*pBody, xiiJoltConversionUtils::ToVec3(msg.m_vForce), xiiJoltConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  //}
}

void xiiJoltRagdollComponent::AddImpulseAtPos(xiiMsgPhysicsAddImpulse& msg)
{
  // XII_ASSERT_DEV(!msg.m_vImpulse.IsNaN() && !msg.m_vGlobalPosition.IsNaN(), "xiiMsgPhysicsAddImpulse contains invalid (NaN) impulse or position");

  // if (msg.m_vImpulse.GetLengthSquared() > m_NextImpulse.m_vImpulse.GetLengthSquared())
  //{
  //   m_NextImpulse.m_vPos = msg.m_vGlobalPosition;
  //   m_NextImpulse.m_vImpulse = msg.m_vImpulse;
  //   m_NextImpulse.m_pRigidBody = static_cast<PxRigidDynamic*>(msg.m_pInternalPhysicsActor);

  //  //if (m_NextImpulse.m_pRigidBody)
  //  //{
  //  //  XII_ASSERT_DEBUG(xiiStringUtils::IsEqual(m_NextImpulse.m_pRigidBody->getConcreteTypeName(), "PxRigidDynamic"), "Expected PxRigidDynamic, got {}", m_NextImpulse.m_pRigidBody->getConcreteTypeName());
  //  //}
  //}
}

void xiiJoltRagdollComponent::ApplyImpulse()
{
  // if (m_NextImpulse.m_vImpulse.IsZero())
  //   return;

  // xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  // XII_PX_WRITE_LOCK(*pModule->GetPxScene());

  // if (m_NextImpulse.m_pRigidBody == nullptr)
  //{
  //   float fBestDist = xiiMath::HighValue<float>();
  //   m_NextImpulse.m_pRigidBody = m_pPxRootBody;

  //  // search for the best limb to apply the impulse to
  //  for (const auto& limb : m_Limbs)
  //  {
  //    if (limb.m_pPxBody == nullptr)
  //      continue;

  //    const float fDistSqr = (xiiJoltConversionUtils::ToVec3(limb.m_pPxBody->getGlobalPose().p) - m_NextImpulse.m_vPos).GetLengthSquared();

  //    if (fDistSqr < fBestDist)
  //    {
  //      fBestDist = fDistSqr;
  //      m_NextImpulse.m_pRigidBody = limb.m_pPxBody;
  //    }
  //  }
  //}

  // JoltRigidBodyExt::addForceAtPos(*m_NextImpulse.m_pRigidBody, xiiJoltConversionUtils::ToVec3(m_NextImpulse.m_vImpulse), xiiJoltConversionUtils::ToVec3(m_NextImpulse.m_vPos), PxForceMode::eIMPULSE);

  // m_NextImpulse = {};
}

void xiiJoltRagdollComponent::OnAnimationPoseProposal(xiiMsgAnimationPoseProposal& msg)
{
  // if (!m_bShapesCreated)
  //   return;

  // msg.m_bContinueAnimating = false;

  // xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  // XII_PX_WRITE_LOCK(*pModule->GetPxScene());

  // for (xiiUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  //{
  //   if (m_ArticulationLinks[i].m_pLink == nullptr)
  //   {
  //     // no need to do anything, just pass the original pose through
  //   }
  //   else
  //   {
  //     if (PxArticulationJoint* pJoint = (PxArticulationJoint*)m_ArticulationLinks[i].m_pLink->getInboundJoint())
  //     {
  //       xiiQuat rot;
  //       rot.SetIdentity();

  //      pJoint->setDriveType(PxArticulationJointDriveType::eTARGET);
  //      pJoint->setTargetOrientation(xiiJoltConversionUtils::ToQuat(rot));
  //      pJoint->setStiffness(100);
  //      //pJoint->setTargetVelocity(PxVec3(1, 1, 1));
  //    }
  //  }
  //}
}

void xiiJoltRagdollComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_Start == xiiJoltRagdollStart::Wait)
    return;

  poseMsg.m_bContinueAnimating = false; // TODO: change this

  if (m_bLimbsSetup)
  {
    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  m_LimbPoses = poseMsg.m_ModelTransforms;

  SetupLimbs(poseMsg);
}

void xiiJoltRagdollComponent::OnRetrieveBoneState(xiiMsgRetrieveBoneState& msg) const
{
  if (!m_bLimbsSetup)
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (xiiUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    xiiMat4 mJoint = m_LimbPoses[uiJointIdx];

    const auto&     joint       = skeleton.GetJointByIndex(uiJointIdx);
    const xiiUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != xiiInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const xiiMat4 mParent = m_LimbPoses[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t       = msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

#if JOINT_DEBUG_DRAW
static void AddLine(xiiHybridArray<xiiDebugRenderer::Line, 32>& lines, const xiiTransform& transform, const xiiVec3& dir, const xiiColor& color)
{
  auto& l        = lines.ExpandAndGetRef();
  l.m_start      = transform.m_vPosition;
  l.m_end        = transform.TransformPosition(dir);
  l.m_startColor = color;
  l.m_endColor   = color;
}
#endif

void xiiJoltRagdollComponent::RetrievePhysicsPose()
{
  if (!m_bLimbsSetup)
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  if (IsSleeping())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);

  const xiiTransform rootTransform    = pSkeleton->GetDescriptor().m_RootTransform;
  const xiiMat4      invRootTransform = rootTransform.GetAsMat4().GetInverse();

  xiiMat4 scale;
  scale.SetScalingMatrix(rootTransform.m_vScale);

  xiiMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_LimbPoses;
  poseMsg.m_pRootTransform  = &rootTransform;
  poseMsg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;

  JPH::Vec3 joltRootPos;
  JPH::Quat joltRootRot;
  m_pRagdoll->GetRootTransform(joltRootPos, joltRootRot);

  const xiiTransform newRootTransform = xiiJoltConversionUtils::ToTransform(joltRootPos, joltRootRot);
  GetOwner()->SetGlobalTransform(newRootTransform * m_RootBodyLocalTransform.GetInverse());

  const xiiMat4 mInv = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * newRootTransform.GetInverse().GetAsMat4();

#if JOINT_DEBUG_DRAW
  xiiHybridArray<xiiDebugRenderer::Line, 32> lines;
#endif

  for (xiiUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_pBodyDesc == nullptr)
    {
      // no need to do anything, just pass the original pose through
      continue;
    }

    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(m_Limbs[uiLimbIdx].m_uiPartIndex);
    JPH::BodyLockRead bodyRead(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyId);

    const xiiTransform limbGlobalPose = xiiJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());

    m_LimbPoses[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;

#if JOINT_DEBUG_DRAW
    if (auto joint = m_ArticulationLinks[uiLimbIdx]->getInboundJoint())
    {
      // joint in parent frame
      {
        const xiiTransform jointParentPose = xiiJoltConversionUtils::ToTransform(joint->getParentArticulationLink().getGlobalPose());
        const xiiTransform jointLocalPose  = xiiJoltConversionUtils::ToTransform(joint->getParentPose());

        xiiTransform jointGlobalPose;
        jointGlobalPose.SetGlobalTransform(jointParentPose, jointLocalPose);

        const float s = 0.1f;

        AddLine(lines, jointGlobalPose, xiiVec3(s, 0, 0), xiiColor::Red);
        AddLine(lines, jointGlobalPose, xiiVec3(-s, 0, 0), xiiColor::DarkRed);

        AddLine(lines, jointGlobalPose, xiiVec3(0, s, 0), xiiColor::Lime);
        AddLine(lines, jointGlobalPose, xiiVec3(0, -s, 0), xiiColor::DarkGreen);

        AddLine(lines, jointGlobalPose, xiiVec3(0, 0, s), xiiColor::Blue);
        AddLine(lines, jointGlobalPose, xiiVec3(0, 0, -s), xiiColor::DarkBlue);
      }

      // joint in child frame
      {
        const xiiTransform jointChildPose = xiiJoltConversionUtils::ToTransform(m_ArticulationLinks[uiLimbIdx]->getGlobalPose());
        const xiiTransform jointLocalPose = xiiJoltConversionUtils::ToTransform(joint->getChildPose());

        xiiTransform jointGlobalPose;
        jointGlobalPose.SetGlobalTransform(jointChildPose, jointLocalPose);

        const float s = 0.05f;

        AddLine(lines, jointGlobalPose, xiiVec3(s, 0, 0), xiiColor::Red);
        AddLine(lines, jointGlobalPose, xiiVec3(0, s, 0), xiiColor::Lime);
        AddLine(lines, jointGlobalPose, xiiVec3(0, 0, s), xiiColor::Blue);
      }
    }
#endif
  }

#if JOINT_DEBUG_DRAW
  xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::White);
#endif

  GetOwner()->SendMessage(poseMsg);
}

void xiiJoltRagdollComponent::WakeUp()
{
  m_pRagdoll->Activate();
}

bool xiiJoltRagdollComponent::IsSleeping() const
{
  const xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterface(), m_pRagdoll->GetBodyID(0));

  if (!lock.Succeeded())
    return true;

  return !lock.GetBody().IsActive();
}

void xiiJoltRagdollComponent::SetupLimbsFromBindPose()
{
  if (m_bLimbsSetup)
    return;

  if (!EnsureSkeletonIsKnown())
  {
    xiiLog::Error("No skeleton available to ragdoll.");
    return;
  }

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          desc = pSkeleton->GetDescriptor();

  m_LimbPoses.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto getBone = [&](xiiUInt32 i, auto f) -> xiiMat4 {
    const auto&   j  = desc.m_Skeleton.GetJointByIndex(i);
    const xiiMat4 bm = j.GetBindPoseLocalTransform().GetAsMat4();

    if (j.GetParentIndex() != xiiInvalidJointIndex)
    {
      const xiiMat4 pbm = f(j.GetParentIndex(), f);

      return pbm * bm;
    }

    return bm;
  };

  for (xiiUInt32 i = 0; i < m_LimbPoses.GetCount(); ++i)
  {
    m_LimbPoses[i] = getBone(i, getBone);
  }

  xiiMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform  = &desc.m_RootTransform;
  msg.m_pSkeleton       = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_LimbPoses;

  SetupLimbs(msg);
}

void xiiJoltRagdollComponent::CreateConstraints()
{
  // if (m_Constraints.IsEmpty())
  //   return;

  // xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  // const xiiTransform ownTransform = GetOwner()->GetGlobalTransform();

  // for (auto& constraint : m_Constraints)
  //{
  //   for (const auto& limb : m_Limbs)
  //   {
  //     if (limb.m_sName != constraint.m_sBone)
  //       continue;

  //    const xiiTransform pos(ownTransform.TransformPosition(constraint.m_vRelativePosition));

  //    auto pJoint = PxSphericalJointCreate(*(xiiJolt::GetSingleton()->GetJoltAPI()), nullptr, xiiJoltConversionUtils::ToTransform(pos), limb.m_pPxBody, xiiJoltConversionUtils::ToTransform(xiiTransform::IdentityTransform()));

  //    pJoint->setConstraintFlag(physx::PxConstraintFlag::ePROJECTION, true);
  //    pJoint->setProjectionLinearTolerance(0.05f);

  //    break;
  //  }
  //}
}

void xiiJoltRagdollComponent::SetupJoltBasics(/*physx::PxPhysics* pPxApi,*/ xiiJoltWorldModule* pModule)
{
  XII_ASSERT_DEBUG(m_uiJoltUserDataIndex == xiiInvalidIndex, "Can't initialize twice.");

  m_uiObjectFilterID    = pModule->CreateObjectFilterID();
  m_uiJoltUserDataIndex = pModule->AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);
}

void xiiJoltRagdollComponent::FinishSetupLimbs()
{
  m_pRagdollSettings->Stabilize();

  // if (m_bSelfCollision)
  // TODO: use GetGroupFilterIgnoreSame when m_bSelfCollision is false (see ropes)
  {
    m_pRagdollSettings->DisableParentChildCollisions();
  }

  static JPH::CollisionGroup::GroupID s_iRagdollCounter = 0;
  ++s_iRagdollCounter;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  m_pRagdoll                  = m_pRagdollSettings->CreateRagdoll(s_iRagdollCounter, reinterpret_cast<xiiUInt64>(m_pJoltUserData), pModule->GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
}

void xiiJoltRagdollComponent::SetupLimbs(const xiiMsgAnimationPoseUpdated& pose)
{
  m_bLimbsSetup = true;

  if (!EnsureSkeletonIsKnown())
  {
    xiiLog::Error("No skeleton available to ragdoll.");
    return;
  }

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  SetupJoltBasics(pModule);

  xiiResourceLock<xiiSkeletonResource> pSkeletonResource(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  m_pRagdollSettings = new JPH::RagdollSettings();
  m_pRagdollSettings->AddRef();
  m_pRagdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  m_pRagdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  m_pRagdollSettings->mSkeleton->GetJoints().reserve(m_pRagdollSettings->mParts.size());

  SetupLimbBodiesAndGeometry(pSkeletonResource.GetPointer(), pose);

  SetupLimbJoints(pSkeletonResource.GetPointer());

  FinishSetupLimbs();

  CreateConstraints();
}

void xiiJoltRagdollComponent::SetupLimbBodiesAndGeometry(const xiiSkeletonResource* pSkeleton, const xiiMsgAnimationPoseUpdated& pose)
{
  xiiMap<xiiUInt16, LimbConfig> limbStructure(xiiFrameAllocator::GetCurrentAllocator());
  limbStructure.FindOrAdd(xiiInvalidJointIndex); // dummy root link

  const auto& skeleton   = pSkeleton->GetDescriptor().m_Skeleton;
  const auto  srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // physx::PxVec3 vPxVelocity = xiiJoltConversionUtils::ToVec3(GetOwner()->GetVelocity());

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == xiiSkeletonJointGeometryType::None)
      continue;

    const xiiSkeletonJoint& thisJoint = skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    xiiUInt16 uiParentJointIdx = thisJoint.GetParentIndex();

    // find the parent joint that is also part of the ragdoll
    while (!limbStructure.Contains(uiParentJointIdx))
    {
      uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
    }

    const auto& parentLimb = limbStructure[uiParentJointIdx];
    auto&       thisLimb   = limbStructure[geo.m_uiAttachedToJoint];

    if (thisLimb.m_pBodyDesc == nullptr)
    {
      thisLimb.m_uiPartIndex = (xiiUInt16)m_pRagdollSettings->mParts.size();
      m_pRagdollSettings->mParts.resize(m_pRagdollSettings->mParts.size() + 1);
      m_pRagdollSettings->mSkeleton->GetJoints().resize(m_pRagdollSettings->mParts.size());
      m_pRagdollSettings->mSkeleton->GetJoints().back().mName             = thisJoint.GetName().GetData();
      m_pRagdollSettings->mSkeleton->GetJoints().back().mParentJointIndex = parentLimb.m_uiPartIndex == xiiInvalidJointIndex ? -1 : parentLimb.m_uiPartIndex;

      if (parentLimb.m_uiPartIndex != xiiInvalidJointIndex)
      {
        const xiiSkeletonJoint& parentJoint                           = skeleton.GetJointByIndex(thisJoint.GetParentIndex());
        m_pRagdollSettings->mSkeleton->GetJoints().back().mParentName = parentJoint.GetName().GetData();
      }

      auto* pBodyDesc      = &m_pRagdollSettings->mParts.back();
      thisLimb.m_pBodyDesc = pBodyDesc;

      ComputeLimbGlobalTransform(thisLimb.m_GlobalTransform, pose, geo.m_uiAttachedToJoint);
      CreateLimbBody(parentLimb, thisLimb);

      m_Limbs[geo.m_uiAttachedToJoint].m_uiPartIndex = thisLimb.m_uiPartIndex;
      m_Limbs[geo.m_uiAttachedToJoint].m_pBodyDesc   = thisLimb.m_pBodyDesc; // TODO keep ?
      m_Limbs[geo.m_uiAttachedToJoint].m_sName       = thisJoint.GetName();

      if (m_pRagdollSettings->mParts.size() == 1) // first body that was added
      {
        // m_pPxRootBody = thisLimb.m_pPxBody;
        m_RootBodyLocalTransform.SetLocalTransform(GetOwner()->GetGlobalTransform(), thisLimb.m_GlobalTransform);
      }
    }

    AddLimbGeometry(srcBoneDir, thisLimb, geo);

    // TODO mass distribution
    {
      float fMass = 4.0f;

      // if (parentLimb.m_pPxBody)
      //{
      //   fMass = 0.9f * parentLimb.m_pPxBody->getMass();
      // }

      // JoltRigidBodyExt::setMassAndUpdateInertia(*thisLimb.m_pPxBody, fMass);
    }
  }
}

void xiiJoltRagdollComponent::SetupLimbJoints(const xiiSkeletonResource* pSkeleton)
{
  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;

  // the main direction of Jolt bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  const xiiQuat qBoneDirAdjustment = -xiiBasisAxis::GetBasisRotation(srcBoneDir, xiiBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (xiiUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_pBodyDesc == nullptr)
      continue;

    const xiiSkeletonJoint& thisJoint    = skeleton.GetJointByIndex(uiLimbIdx);
    xiiUInt16               uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != xiiInvalidJointIndex && m_Limbs[uiParentLimb].m_pBodyDesc == nullptr)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == xiiInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    // TODO: all this stuff

    // const xiiTransform parentTransform = xiiJoltConversionUtils::ToTransform(parentLimb.m_pPxBody->getGlobalPose());
    // const xiiTransform thisTransform = xiiJoltConversionUtils::ToTransform(thisLimb.m_pPxBody->getGlobalPose());

    xiiTransform parentJointFrame;
    // parentJointFrame.SetLocalTransform(parentTransform, thisTransform); // TODO this should just be the constant local offset from child to parent (rotation is overridden anyway, position should never differ)
    // parentJointFrame.m_qRotation = thisJoint.GetLocalOrientation() * qBoneDirAdjustment;

    xiiTransform thisJointFrame;
    // thisJointFrame.SetIdentity();
    // thisJointFrame.m_qRotation = qBoneDirAdjustment;

    CreateLimbJoint(thisJoint, parentLimb.m_pBodyDesc, parentJointFrame, thisLimb.m_pBodyDesc, thisJointFrame);
  }
}

void xiiJoltRagdollComponent::CreateLimbBody(const LimbConfig& parentLimb, LimbConfig& thisLimb)
{
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(parentLimb.m_pBodyDesc);
  JPH::RagdollSettings::Part* pLink       = reinterpret_cast<JPH::RagdollSettings::Part*>(thisLimb.m_pBodyDesc);

  pLink->mPosition      = xiiJoltConversionUtils::ToVec3(thisLimb.m_GlobalTransform.m_vPosition);
  pLink->mRotation      = xiiJoltConversionUtils::ToQuat(thisLimb.m_GlobalTransform.m_qRotation).Normalized();
  pLink->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pLink->mGravityFactor = m_fGravityFactor;
  pLink->mUserData      = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
  pLink->mObjectLayer   = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Ragdoll);
  pLink->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // pLink->setLinearVelocity(vPxVelocity, false);

  // TODO: setup self-collision
}

void xiiJoltRagdollComponent::ComputeLimbModelSpaceTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiIndex)
{
  xiiMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void xiiJoltRagdollComponent::ComputeLimbGlobalTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiIndex)
{
  xiiTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiIndex);
  transform.SetGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void xiiJoltRagdollComponent::AddLimbGeometry(xiiBasisAxis::Enum srcBoneDir, LimbConfig& limb, const xiiSkeletonResourceGeometry& geo)
{
  // TODO: compound shapes

  JPH::RagdollSettings::Part* pBodyDesc = reinterpret_cast<JPH::RagdollSettings::Part*>(limb.m_pBodyDesc);

  // physx::PxMaterial* pxMaterial = nullptr;
  // if (geo.m_hSurface.IsValid())
  //{
  //   xiiResourceLock<xiiSurfaceResource> pSurface(geo.m_hSurface, xiiResourceAcquireMode::BlockTillLoaded);

  //  if (pSurface->m_pPhysicsMaterial != nullptr)
  //  {
  //    pxMaterial = static_cast<physx::PxMaterial*>(pSurface->m_pPhysicsMaterial);
  //  }
  //}
  // else
  //  pxMaterial = xiiJolt::GetSingleton()->GetDefaultMaterial();

  // JoltShape* pShape = nullptr;

  const xiiQuat qBoneDirAdjustment = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveX, srcBoneDir);

  const xiiQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  xiiTransform st;
  st.SetIdentity();
  st.m_vPosition = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
  st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

  if (geo.m_Type == xiiSkeletonJointGeometryType::Sphere)
  {
    JPH::SphereShapeSettings shape;
    shape.mRadius  = geo.m_Transform.m_vScale.z;
    shape.mDensity = 100.0f;
    // TODO: shape.mUserData =
    // TODO: material

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else if (geo.m_Type == xiiSkeletonJointGeometryType::Box)
  {
    JPH::BoxShapeSettings shape;
    shape.mHalfExtent = xiiJoltConversionUtils::ToVec3(geo.m_Transform.m_vScale * 0.5f);
    // TODO: shape.mMaterial = ...
    // TODO: shape.mUserData = ...

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else if (geo.m_Type == xiiSkeletonJointGeometryType::Capsule)
  {
    JPH::CapsuleShapeSettings shape;
    shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f;
    shape.mRadius               = geo.m_Transform.m_vScale.z;
    // TODO: shape.mMaterial = ...
    // TODO: shape.mUserData = ...

    xiiQuat qRot;
    qRot.SetFromAxisAndAngle(xiiVec3::UnitZAxis(), xiiAngle::Degree(-90));
    st.m_qRotation = st.m_qRotation * qRot;

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  if (!st.IsEqual(xiiTransform::IdentityTransform(), 0.001f))
  {
    JPH::RotatedTranslatedShapeSettings dec;
    dec.mInnerShapePtr = pBodyDesc->GetShape();
    dec.mPosition      = xiiJoltConversionUtils::ToVec3(st.m_vPosition);
    dec.mRotation      = xiiJoltConversionUtils::ToQuat(st.m_qRotation);
    // TODO: dec.mUserData = ...

    pBodyDesc->SetShape(dec.Create().Get());
  }

  // pShape->setLocalPose(xiiJoltConversionUtils::ToTransform(st));
  // pShape->setSimulationFilterData(pxFilterData);
  // pShape->setQueryFilterData(pxFilterData);
  // pShape->userData = m_pJoltUserData;
}

void xiiJoltRagdollComponent::CreateLimbJoint(const xiiSkeletonJoint& thisJoint, void* pParentBodyDesc, const xiiTransform& parentFrame, void* pThisBodyDesc, const xiiTransform& thisFrame)
{
  JPH::RagdollSettings::Part* pLink       = reinterpret_cast<JPH::RagdollSettings::Part*>(pThisBodyDesc);
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pParentBodyDesc);

  xiiTransform tParent = xiiJoltConversionUtils::ToTransform(pParentLink->mPosition, pParentLink->mRotation);
  xiiTransform tThis   = xiiJoltConversionUtils::ToTransform(pLink->mPosition, pLink->mRotation);

  {
    JPH::SwingTwistConstraintSettings* pJoint = new JPH::SwingTwistConstraintSettings();
    pLink->mToParent                          = pJoint;

    const xiiQuat offsetRot = thisJoint.GetLocalOrientation();

    xiiQuat qTwist;
    qTwist.SetFromAxisAndAngle(xiiVec3::UnitYAxis(), thisJoint.GetTwistLimitCenterAngle());

    pJoint->mDrawConstraintSize  = 0.1f;
    pJoint->mPosition1           = pLink->mPosition;
    pJoint->mPosition2           = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian(); // TODO: disable ?
    pJoint->mPlaneHalfConeAngle  = thisJoint.GetHalfSwingLimitY().GetRadian(); // TODO: disable ?
    pJoint->mTwistMinAngle       = -thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mTwistMaxAngle       = thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mMaxFrictionTorque   = m_fStiffness;
    pJoint->mPlaneAxis1          = xiiJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * qTwist * xiiVec3::UnitZAxis());
    pJoint->mPlaneAxis2          = xiiJoltConversionUtils::ToVec3(tThis.m_qRotation * qTwist * xiiVec3::UnitZAxis());
    pJoint->mTwistAxis1          = xiiJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * xiiVec3::UnitYAxis());
    pJoint->mTwistAxis2          = xiiJoltConversionUtils::ToVec3(tThis.m_qRotation * xiiVec3::UnitYAxis());
  }
}
