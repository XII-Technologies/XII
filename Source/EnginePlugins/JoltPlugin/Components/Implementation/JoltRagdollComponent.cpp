#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
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
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/Shape/CompoundShape.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO

 * prevent crashes with zero bodies
 * import sphere/box/capsule shapes
 * configure joints correctly (+ different types)
 * move stiffness into skeleton (joint setting)

  * external constraints
 * max force clamping / point vs area impulse ?
 * communication with anim controller
 * drive to pose
 */

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltRagdollStartMode, 1)
  XII_ENUM_CONSTANTS(xiiJoltRagdollStartMode::WithBindPose, xiiJoltRagdollStartMode::WithNextAnimPose, xiiJoltRagdollStartMode::WithCurrentMeshPose)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltRagdollComponent, 2, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    XII_ENUM_ACCESSOR_PROPERTY("StartMode", xiiJoltRagdollStartMode, GetStartMode, SetStartMode),
    XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new xiiDefaultValueAttribute(50.0f)),
    XII_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_MEMBER_PROPERTY("OwnerVelocityScale", m_fOwnerVelocityScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("CenterPosition", m_vCenterPosition),
    XII_MEMBER_PROPERTY("CenterVelocity", m_fCenterVelocity)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_MEMBER_PROPERTY("CenterAngularVelocity", m_fCenterAngularVelocity)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgRetrieveBoneState, OnRetrieveBoneState),
    XII_MESSAGE_HANDLER(xiiMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    XII_MESSAGE_HANDLER(xiiMsgPhysicsAddForce, OnMsgPhysicsAddForce),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Animation"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    XII_SCRIPT_FUNCTION_PROPERTY(SetInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    XII_SCRIPT_FUNCTION_PROPERTY(AddInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
  }
  XII_END_FUNCTIONS;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

XII_DEFINE_AS_POD_TYPE(JPH::Vec3);

//////////////////////////////////////////////////////////////////////////

xiiJoltRagdollComponentManager::xiiJoltRagdollComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiJoltRagdollComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

xiiJoltRagdollComponentManager::~xiiJoltRagdollComponentManager() = default;

void xiiJoltRagdollComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltRagdollComponentManager::Update, this);
    desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void xiiJoltRagdollComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  XII_PROFILE_SCOPE("UpdateRagdolls");

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();

  for (auto it : pModule->GetActiveRagdolls())
  {
    xiiJoltRagdollComponent* pComponent = it.Key();

    pComponent->Update(false);
  }

  for (xiiJoltRagdollComponent* pComponent : pModule->GetRagdollsPutToSleep())
  {
    pComponent->Update(true);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiJoltRagdollComponent::xiiJoltRagdollComponent()  = default;
xiiJoltRagdollComponent::~xiiJoltRagdollComponent() = default;

void xiiJoltRagdollComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_StartMode;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_fOwnerVelocityScale;
  s << m_fCenterVelocity;
  s << m_fCenterAngularVelocity;
  s << m_vCenterPosition;
  s << m_fMass;
  s << m_fStiffness;
}

void xiiJoltRagdollComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  if (uiVersion < 2)
    return;

  s >> m_StartMode;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_fOwnerVelocityScale;
  s >> m_fCenterVelocity;
  s >> m_fCenterAngularVelocity;
  s >> m_vCenterPosition;
  s >> m_fMass;
  s >> m_fStiffness;
}

void xiiJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_StartMode == xiiJoltRagdollStartMode::WithBindPose)
  {
    CreateLimbsFromBindPose();
  }
  if (m_StartMode == xiiJoltRagdollStartMode::WithCurrentMeshPose)
  {
    CreateLimbsFromCurrentMeshPose();
  }
}

void xiiJoltRagdollComponent::OnDeactivated()
{
  DestroyAllLimbs();

  SUPER::OnDeactivated();
}

void xiiJoltRagdollComponent::Update(bool bForce)
{
  if (!HasCreatedLimbs())
    return;

  UpdateOwnerPosition();

  const xiiVisibilityState visState = GetOwner()->GetVisibilityState();
  if (!bForce && visState != xiiVisibilityState::Direct)
  {
    m_ElapsedTimeSinceUpdate += xiiClock::GetGlobalClock()->GetTimeDiff();

    if (visState == xiiVisibilityState::Indirect && m_ElapsedTimeSinceUpdate < xiiTime::Milliseconds(200))
    {
      // when the ragdoll is only visible by shadows or reflections, update it infrequently
      return;
    }

    if (visState == xiiVisibilityState::Invisible && m_ElapsedTimeSinceUpdate < xiiTime::Milliseconds(500))
    {
      // when the ragdoll is entirely invisible, update it very rarely
      return;
    }
  }

  RetrieveRagdollPose();
  SendAnimationPoseMsg();

  m_ElapsedTimeSinceUpdate.SetZero();
}

xiiResult xiiJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    xiiMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  if (!m_hSkeleton.IsValid())
  {
    xiiLog::Error("No skeleton available for ragdoll on object '{}'.", GetOwner()->GetName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

bool xiiJoltRagdollComponent::HasCreatedLimbs() const
{
  return m_pRagdoll != nullptr;
}

void xiiJoltRagdollComponent::CreateLimbsFromBindPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          desc = pSkeleton->GetDescriptor();

  m_CurrentLimbTransforms.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto ComputeFullJointTransform = [&](xiiUInt32 uiJointIdx, auto self) -> xiiMat4 {
    const auto&   joint          = desc.m_Skeleton.GetJointByIndex(uiJointIdx);
    const xiiMat4 jointTransform = joint.GetBindPoseLocalTransform().GetAsMat4();

    if (joint.GetParentIndex() != xiiInvalidJointIndex)
    {
      const xiiMat4 parentTransform = self(joint.GetParentIndex(), self);

      return parentTransform * jointTransform;
    }

    return jointTransform;
  };

  for (xiiUInt32 i = 0; i < m_CurrentLimbTransforms.GetCount(); ++i)
  {
    m_CurrentLimbTransforms[i] = ComputeFullJointTransform(i, ComputeFullJointTransform);
  }

  xiiMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform  = &desc.m_RootTransform;
  msg.m_pSkeleton       = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void xiiJoltRagdollComponent::CreateLimbsFromCurrentMeshPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  xiiAnimatedMeshComponent* pMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType<xiiAnimatedMeshComponent>(pMesh))
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  xiiTransform tRoot;
  pMesh->RetrievePose(m_CurrentLimbTransforms, tRoot, pSkeleton->GetDescriptor().m_Skeleton);

  xiiMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform  = &tRoot;
  msg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void xiiJoltRagdollComponent::DestroyAllLimbs()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiJoltUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_pJoltUserData = nullptr;
  }

  m_CurrentLimbTransforms.Clear();
  m_Limbs.Clear();
}

void xiiJoltRagdollComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  if (!m_pRagdoll)
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  for (xiiUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void xiiJoltRagdollComponent::SetStartMode(xiiEnum<xiiJoltRagdollStartMode> mode)
{
  if (m_StartMode == mode)
    return;

  m_StartMode = mode;
}

void xiiJoltRagdollComponent::OnMsgPhysicsAddImpulse(xiiMsgPhysicsAddImpulse& ref_msg)
{
  if (!HasCreatedLimbs())
  {
    m_vInitialImpulsePosition += ref_msg.m_vGlobalPosition;
    m_vInitialImpulseDirection += ref_msg.m_vImpulse;
    m_uiNumInitialImpulses++;
    return;
  }

  JPH::BodyID bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  if (!bodyId.IsInvalid())
  {
    auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
    pBodies->AddImpulse(bodyId, xiiJoltConversionUtils::ToVec3(ref_msg.m_vImpulse), xiiJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void xiiJoltRagdollComponent::OnMsgPhysicsAddForce(xiiMsgPhysicsAddForce& ref_msg)
{
  if (!HasCreatedLimbs())
    return;

  JPH::BodyID bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  if (!bodyId.IsInvalid())
  {
    auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
    pBodies->AddForce(bodyId, xiiJoltConversionUtils::ToVec3(ref_msg.m_vForce), xiiJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void xiiJoltRagdollComponent::SetInitialImpulse(const xiiVec3& vPosition, const xiiVec3& vDirectionAndStrength)
{
  if (vDirectionAndStrength.IsZero())
  {
    m_vInitialImpulsePosition.SetZero();
    m_vInitialImpulseDirection.SetZero();
    m_uiNumInitialImpulses = 0;
  }
  else
  {
    m_vInitialImpulsePosition  = vPosition;
    m_vInitialImpulseDirection = vDirectionAndStrength;
    m_uiNumInitialImpulses     = 1;
  }
}

void xiiJoltRagdollComponent::AddInitialImpulse(const xiiVec3& vPosition, const xiiVec3& vDirectionAndStrength)
{
  m_vInitialImpulsePosition += vPosition;
  m_vInitialImpulseDirection += vDirectionAndStrength;
  m_uiNumInitialImpulses++;
}

void xiiJoltRagdollComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (HasCreatedLimbs())
  {
    ref_poseMsg.m_bContinueAnimating = false; // TODO: change this

    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  if (m_StartMode != xiiJoltRagdollStartMode::WithNextAnimPose)
    return;

  m_CurrentLimbTransforms = ref_poseMsg.m_ModelTransforms;

  CreateLimbsFromPose(ref_poseMsg);
}

void xiiJoltRagdollComponent::OnRetrieveBoneState(xiiMsgRetrieveBoneState& ref_msg) const
{
  if (!HasCreatedLimbs())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (xiiUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    xiiMat4 mJoint = m_CurrentLimbTransforms[uiJointIdx];

    const auto&     joint       = skeleton.GetJointByIndex(uiJointIdx);
    const xiiUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != xiiInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const xiiMat4 mParent = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t       = ref_msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

void xiiJoltRagdollComponent::SendAnimationPoseMsg()
{
  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const xiiTransform                   rootTransform = pSkeleton->GetDescriptor().m_RootTransform;

  xiiMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_CurrentLimbTransforms;
  poseMsg.m_pRootTransform  = &rootTransform;
  poseMsg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;

  GetOwner()->SendMessage(poseMsg);
}

xiiTransform xiiJoltRagdollComponent::GetRagdollRootTransform() const
{
  JPH::Vec3 joltRootPos;
  JPH::Quat joltRootRot;
  m_pRagdoll->GetRootTransform(joltRootPos, joltRootRot);

  xiiTransform res = xiiJoltConversionUtils::ToTransform(joltRootPos, joltRootRot);
  res.m_vScale     = GetOwner()->GetGlobalScaling();

  return res;
}

void xiiJoltRagdollComponent::UpdateOwnerPosition()
{
  GetOwner()->SetGlobalTransform(GetRagdollRootTransform() * m_RootBodyLocalTransform.GetInverse());
}

void xiiJoltRagdollComponent::RetrieveRagdollPose()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const xiiTransform                   rootTransform    = pSkeleton->GetDescriptor().m_RootTransform;
  const xiiMat4                        invRootTransform = rootTransform.GetAsMat4().GetInverse();
  const xiiMat4                        mInv             = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * GetRagdollRootTransform().GetInverse().GetAsMat4();

  const xiiVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float   fObjectScale = xiiMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  xiiMat4 scale;
  scale.SetScalingMatrix(rootTransform.m_vScale * fObjectScale);

  for (xiiUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex == xiiInvalidJointIndex)
    {
      // no need to do anything, just pass the original pose through
      continue;
    }

    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(m_Limbs[uiLimbIdx].m_uiPartIndex);
    JPH::BodyLockRead bodyRead(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyId);

    const xiiTransform limbGlobalPose = xiiJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());

    m_CurrentLimbTransforms[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;
  }
}

void xiiJoltRagdollComponent::CreateLimbsFromPose(const xiiMsgAnimationPoseUpdated& pose)
{
  XII_ASSERT_DEBUG(!HasCreatedLimbs(), "Limbs are already created.");

  if (EnsureSkeletonIsKnown().Failed())
    return;

  const xiiVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float   fObjectScale = xiiMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  xiiJoltWorldModule& worldModule = *GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  m_uiObjectFilterID              = worldModule.CreateObjectFilterID();
  m_uiJoltUserDataIndex           = worldModule.AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);

  xiiResourceLock<xiiSkeletonResource> pSkeletonResource(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  JPH::Ref<JPH::RagdollSettings> ragdollSettings = new JPH::RagdollSettings();
  XII_SCOPE_EXIT(m_pRagdollSettings = nullptr);

  m_pRagdollSettings = ragdollSettings.GetPtr();
  m_pRagdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  m_pRagdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  m_pRagdollSettings->mSkeleton->GetJoints().reserve(m_pRagdollSettings->mParts.size());

  CreateAllLimbs(*pSkeletonResource.GetPointer(), pose, worldModule, fObjectScale);
  ApplyBodyMass();
  SetupLimbJoints(pSkeletonResource.GetPointer());
  ApplyPartInitialVelocity();

  if (m_bSelfCollision)
  {
    // enables collisions between all bodies except the ones that are directly connected to each other
    m_pRagdollSettings->DisableParentChildCollisions();
  }

  m_pRagdollSettings->Stabilize();

  m_pRagdoll = m_pRagdollSettings->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<xiiUInt64>(m_pJoltUserData), worldModule.GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  ApplyInitialImpulse(worldModule, pSkeletonResource->GetDescriptor().m_fMaxImpulse);
}

void xiiJoltRagdollComponent::ConfigureRagdollPart(void* pRagdollSettingsPart, const xiiTransform& globalTransform, xiiUInt8 uiCollisionLayer, xiiJoltWorldModule& worldModule)
{
  JPH::RagdollSettings::Part* pPart = reinterpret_cast<JPH::RagdollSettings::Part*>(pRagdollSettingsPart);

  pPart->mPosition      = xiiJoltConversionUtils::ToVec3(globalTransform.m_vPosition);
  pPart->mRotation      = xiiJoltConversionUtils::ToQuat(globalTransform.m_qRotation).Normalized();
  pPart->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pPart->mGravityFactor = m_fGravityFactor;
  pPart->mUserData      = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
  pPart->mObjectLayer   = xiiJoltCollisionFiltering::ConstructObjectLayer(uiCollisionLayer, xiiJoltBroadphaseLayer::Ragdoll);
  pPart->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  pPart->mCollisionGroup.SetGroupFilter(worldModule.GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below
}

void xiiJoltRagdollComponent::ApplyPartInitialVelocity()
{
  JPH::Vec3       vCommonVelocity = xiiJoltConversionUtils::ToVec3(GetOwner()->GetVelocity() * m_fOwnerVelocityScale);
  const JPH::Vec3 vCenterPos      = xiiJoltConversionUtils::ToVec3(GetOwner()->GetGlobalTransform() * m_vCenterPosition);

  xiiCoordinateSystem coord;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coord);
  xiiRandom& rng = GetOwner()->GetWorld()->GetRandomNumberGenerator();

  for (JPH::RagdollSettings::Part& part : m_pRagdollSettings->mParts)
  {
    part.mLinearVelocity = vCommonVelocity;

    if (m_fCenterVelocity != 0.0f)
    {
      const JPH::Vec3 vVelocityDir = (part.mPosition - vCenterPos).NormalizedOr(JPH::Vec3::sZero());
      part.mLinearVelocity += vVelocityDir * xiiMath::Min(part.mMaxLinearVelocity, m_fCenterVelocity);
    }

    if (m_fCenterAngularVelocity != 0.0f)
    {
      const xiiVec3 vVelocityDir = xiiJoltConversionUtils::ToVec3(part.mPosition - vCenterPos);
      xiiVec3       vRotationDir = vVelocityDir.CrossRH(coord.m_vUpDir);
      vRotationDir.NormalizeIfNotZero(coord.m_vUpDir).IgnoreResult();

      xiiVec3 vRotationAxis = xiiVec3::CreateRandomDeviation(rng, xiiAngle::Degree(30.0f), vRotationDir);
      vRotationAxis *= rng.Bool() ? 1.0f : -1.0f;

      float fSpeed = rng.FloatVariance(m_fCenterAngularVelocity, 0.5f);
      fSpeed       = xiiMath::Min(fSpeed, part.mMaxAngularVelocity * 0.95f);

      part.mAngularVelocity = xiiJoltConversionUtils::ToVec3(vRotationAxis) * fSpeed;
    }
  }
}

void xiiJoltRagdollComponent::ApplyInitialImpulse(xiiJoltWorldModule& worldModule, float fMaxImpulse)
{
  if (m_uiNumInitialImpulses == 0)
    return;

  if (m_uiNumInitialImpulses > 1)
  {
    xiiLog::Info("Impulses: {} - {}", m_uiNumInitialImpulses, m_vInitialImpulseDirection.GetLength());
  }

  auto pJoltSystem = worldModule.GetJoltSystem();

  m_vInitialImpulsePosition /= m_uiNumInitialImpulses;

  float fImpulse = m_vInitialImpulseDirection.GetLength();

  if (fImpulse > fMaxImpulse)
  {
    fImpulse = fMaxImpulse;
    m_vInitialImpulseDirection.SetLength(fImpulse).AssertSuccess();
  }

  const JPH::Vec3 vImpulsePosition   = xiiJoltConversionUtils::ToVec3(m_vInitialImpulsePosition);
  float           fLowestDistanceSqr = 100000;

  JPH::BodyID closestBody;

  for (xiiUInt32 uiBodyIdx = 0; uiBodyIdx < m_pRagdoll->GetBodyCount(); ++uiBodyIdx)
  {
    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(uiBodyIdx);
    JPH::BodyLockRead bodyRead(pJoltSystem->GetBodyLockInterface(), bodyId);

    const float fDistanceToImpulseSqr = (bodyRead.GetBody().GetPosition() - vImpulsePosition).LengthSq();

    if (fDistanceToImpulseSqr < fLowestDistanceSqr)
    {
      fLowestDistanceSqr = fDistanceToImpulseSqr;
      closestBody        = bodyId;
    }
  }

  pJoltSystem->GetBodyInterface().AddImpulse(closestBody, xiiJoltConversionUtils::ToVec3(m_vInitialImpulseDirection), vImpulsePosition);
}


void xiiJoltRagdollComponent::ApplyBodyMass()
{
  if (m_fMass <= 0.0f)
    return;

  float fPartMass = m_fMass / m_pRagdollSettings->mParts.size();

  for (auto& part : m_pRagdollSettings->mParts)
  {
    part.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
    part.mMassPropertiesOverride.mMass = fPartMass;
  }
}

void xiiJoltRagdollComponent::ComputeLimbModelSpaceTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiPoseJointIndex)
{
  xiiMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiPoseJointIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void xiiJoltRagdollComponent::ComputeLimbGlobalTransform(xiiTransform& transform, const xiiMsgAnimationPoseUpdated& pose, xiiUInt32 uiPoseJointIndex)
{
  xiiTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiPoseJointIndex);
  transform.SetGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void xiiJoltRagdollComponent::CreateAllLimbs(const xiiSkeletonResource& skeletonResource, const xiiMsgAnimationPoseUpdated& pose, xiiJoltWorldModule& worldModule, float fObjectScale)
{
  xiiMap<xiiUInt16, LimbConstructionInfo> limbConstructionInfos(xiiFrameAllocator::GetCurrentAllocator());
  limbConstructionInfos.FindOrAdd(xiiInvalidJointIndex); // dummy root link

  xiiUInt16                                             uiLastLimbIdx = xiiInvalidJointIndex;
  xiiHybridArray<const xiiSkeletonResourceGeometry*, 8> geometries;

  for (const auto& geo : skeletonResource.GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == xiiSkeletonJointGeometryType::None)
      continue;

    if (geo.m_uiAttachedToJoint != uiLastLimbIdx)
    {
      CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);
      geometries.Clear();
      uiLastLimbIdx = geo.m_uiAttachedToJoint;
    }

    geometries.PushBack(&geo);
  }

  CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);

  // get the limb with the lowest index (ie. the first one added) as the root joint
  // and use it's transform to compute m_RootBodyLocalTransform
  m_RootBodyLocalTransform.SetLocalTransform(GetOwner()->GetGlobalTransform(), limbConstructionInfos.GetIterator().Value().m_GlobalTransform);
}

void xiiJoltRagdollComponent::CreateLimb(const xiiSkeletonResource& skeletonResource, xiiMap<xiiUInt16, LimbConstructionInfo>& limbConstructionInfos, xiiArrayPtr<const xiiSkeletonResourceGeometry*> geometries, const xiiMsgAnimationPoseUpdated& pose, xiiJoltWorldModule& worldModule, float fObjectScale)
{
  if (geometries.IsEmpty())
    return;

  const xiiSkeleton& skeleton = skeletonResource.GetDescriptor().m_Skeleton;

  const xiiUInt16         uiThisJointIdx   = geometries[0]->m_uiAttachedToJoint;
  const xiiSkeletonJoint& thisLimbJoint    = skeleton.GetJointByIndex(uiThisJointIdx);
  xiiUInt16               uiParentJointIdx = thisLimbJoint.GetParentIndex();

  // find the parent joint that is also part of the ragdoll
  while (!limbConstructionInfos.Contains(uiParentJointIdx))
  {
    uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
  }
  // now uiParentJointIdx is either the index of a limb that has been created before, or xiiInvalidJointIndex

  LimbConstructionInfo&       thisLimbInfo   = limbConstructionInfos[uiThisJointIdx];
  const LimbConstructionInfo& parentLimbInfo = limbConstructionInfos[uiParentJointIdx];

  thisLimbInfo.m_uiJoltPartIndex = (xiiUInt16)m_pRagdollSettings->mParts.size();
  m_pRagdollSettings->mParts.resize(m_pRagdollSettings->mParts.size() + 1);

  m_Limbs[uiThisJointIdx].m_uiPartIndex = thisLimbInfo.m_uiJoltPartIndex;

  m_pRagdollSettings->mSkeleton->AddJoint(thisLimbJoint.GetName().GetData(), parentLimbInfo.m_uiJoltPartIndex != xiiInvalidJointIndex ? parentLimbInfo.m_uiJoltPartIndex : -1);

  ComputeLimbGlobalTransform(thisLimbInfo.m_GlobalTransform, pose, uiThisJointIdx);
  ConfigureRagdollPart(&m_pRagdollSettings->mParts[thisLimbInfo.m_uiJoltPartIndex], thisLimbInfo.m_GlobalTransform, thisLimbJoint.GetCollisionLayer(), worldModule);
  CreateAllLimbGeoShapes(thisLimbInfo, geometries, thisLimbJoint, skeletonResource, fObjectScale);
}

JPH::Shape* xiiJoltRagdollComponent::CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const xiiSkeletonResourceGeometry& geo, const xiiJoltMaterial* pJoltMaterial, const xiiQuat& qBoneDirAdjustment, const xiiTransform& skeletonRootTransform, xiiTransform& out_shapeTransform, float fObjectScale)
{
  out_shapeTransform.SetIdentity();
  out_shapeTransform.m_vPosition = qBoneDirAdjustment * geo.m_Transform.m_vPosition * fObjectScale;
  out_shapeTransform.m_qRotation = qBoneDirAdjustment * geo.m_Transform.m_qRotation;

  JPH::Ref<JPH::Shape> pShape;

  switch (geo.m_Type)
  {
    case xiiSkeletonJointGeometryType::Sphere:
    {
      JPH::SphereShapeSettings shape;
      shape.mUserData = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mRadius   = geo.m_Transform.m_vScale.z * fObjectScale;

      pShape = shape.Create().Get();
    }
    break;

    case xiiSkeletonJointGeometryType::Box:
    {
      JPH::BoxShapeSettings shape;
      shape.mUserData   = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
      shape.mMaterial   = pJoltMaterial;
      shape.mHalfExtent = xiiJoltConversionUtils::ToVec3(geo.m_Transform.m_vScale * 0.5f) * fObjectScale;

      out_shapeTransform.m_vPosition += qBoneDirAdjustment * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case xiiSkeletonJointGeometryType::Capsule:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData             = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
      shape.mMaterial             = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius               = geo.m_Transform.m_vScale.z * fObjectScale;

      xiiQuat qRot;
      qRot.SetFromAxisAndAngle(xiiVec3::UnitZAxis(), xiiAngle::Degree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation * qRot;
      out_shapeTransform.m_vPosition += qBoneDirAdjustment * xiiVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case xiiSkeletonJointGeometryType::ConvexMesh:
    {
      // convex mesh vertices are in "global space" of the mesh file format
      // so first move them into global space of the XII convention (skeletonRootTransform)
      // then move them to the global position of the ragdoll object
      // then apply the inverse global transform of the limb, to move everything into local space of the limb

      out_shapeTransform = limbConstructionInfo.m_GlobalTransform.GetInverse() * GetOwner()->GetGlobalTransform() * skeletonRootTransform;
      out_shapeTransform.m_vPosition *= fObjectScale;

      xiiHybridArray<JPH::Vec3, 256> verts;
      verts.SetCountUninitialized(geo.m_VertexPositions.GetCount());

      for (xiiUInt32 i = 0; i < verts.GetCount(); ++i)
      {
        verts[i] = xiiJoltConversionUtils::ToVec3(geo.m_VertexPositions[i] * fObjectScale);
      }

      JPH::ConvexHullShapeSettings shape(verts.GetData(), (int)verts.GetCount());
      shape.mUserData = reinterpret_cast<xiiUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;

      const auto shapeRes = shape.Create();

      if (shapeRes.HasError())
      {
        xiiLog::Error("Cooking convex ragdoll piece failed: {}", shapeRes.GetError().c_str());
        return nullptr;
      }

      pShape = shapeRes.Get();
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  pShape->AddRef();
  return pShape;
}

void xiiJoltRagdollComponent::CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, xiiArrayPtr<const xiiSkeletonResourceGeometry*> geometries, const xiiSkeletonJoint& thisLimbJoint, const xiiSkeletonResource& skeletonResource, float fObjectScale)
{
  const xiiJoltMaterial* pJoltMaterial = xiiJoltCore::GetDefaultMaterial();

  if (thisLimbJoint.GetSurface().IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(thisLimbJoint.GetSurface(), xiiResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      pJoltMaterial = static_cast<xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  const xiiTransform& skeletonRootTransform = skeletonResource.GetDescriptor().m_RootTransform;

  const auto    srcBoneDir         = skeletonResource.GetDescriptor().m_Skeleton.m_BoneDirection;
  const xiiQuat qBoneDirAdjustment = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveX, srcBoneDir);

  JPH::RagdollSettings::Part* pBodyDesc = &m_pRagdollSettings->mParts[limbConstructionInfo.m_uiJoltPartIndex];

  if (geometries.GetCount() > 1)
  {
    JPH::StaticCompoundShapeSettings compound;

    for (const xiiSkeletonResourceGeometry* pGeo : geometries)
    {
      xiiTransform shapeTransform;
      if (JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *pGeo, pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale))
      {
        compound.AddShape(xiiJoltConversionUtils::ToVec3(shapeTransform.m_vPosition), xiiJoltConversionUtils::ToQuat(shapeTransform.m_qRotation), pSubShape);
        pSubShape->Release(); // had to manual AddRef once
      }
    }

    const auto compoundRes = compound.Create();
    if (!compoundRes.IsValid())
    {
      xiiLog::Error("Creating a compound shape for a ragdoll failed: {}", compoundRes.GetError().c_str());
      return;
    }

    pBodyDesc->SetShape(compoundRes.Get());
  }
  else
  {
    xiiTransform shapeTransform;
    JPH::Shape*  pSubShape = CreateLimbGeoShape(limbConstructionInfo, *geometries[0], pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale);

    if (!shapeTransform.IsEqual(xiiTransform::IdentityTransform(), 0.001f))
    {
      JPH::RotatedTranslatedShapeSettings outerShape;
      outerShape.mInnerShapePtr = pSubShape;
      outerShape.mPosition      = xiiJoltConversionUtils::ToVec3(shapeTransform.m_vPosition);
      outerShape.mRotation      = xiiJoltConversionUtils::ToQuat(shapeTransform.m_qRotation);
      outerShape.mUserData      = reinterpret_cast<xiiUInt64>(m_pJoltUserData);

      pBodyDesc->SetShape(outerShape.Create().Get());
    }
    else
    {
      pBodyDesc->SetShape(pSubShape);
    }

    pSubShape->Release(); // had to manual AddRef once
  }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



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

    if (thisLimb.m_uiPartIndex == xiiInvalidJointIndex)
      continue;

    const xiiSkeletonJoint& thisJoint    = skeleton.GetJointByIndex(uiLimbIdx);
    xiiUInt16               uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != xiiInvalidJointIndex && m_Limbs[uiParentLimb].m_uiPartIndex == xiiInvalidJointIndex)
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

    CreateLimbJoint(thisJoint, &m_pRagdollSettings->mParts[parentLimb.m_uiPartIndex], parentJointFrame, &m_pRagdollSettings->mParts[thisLimb.m_uiPartIndex], thisJointFrame);
  }
}

void xiiJoltRagdollComponent::CreateLimbJoint(const xiiSkeletonJoint& thisJoint, void* pParentBodyDesc, const xiiTransform& parentFrame, void* pThisBodyDesc, const xiiTransform& thisFrame)
{
  if (thisJoint.GetJointType() == xiiSkeletonJointType::None)
    return;

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

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);
