#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Components/JoltRopeComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltRopeComponent, 1, xiiComponentMode::Dynamic)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
      XII_MEMBER_PROPERTY("AttachToOrigin", m_bAttachToOrigin)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("AttachToAnchor", m_bAttachToAnchor)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new xiiDefaultValueAttribute(16), new xiiClampValueAttribute(2, 64)),
      XII_MEMBER_PROPERTY("Slack", m_fSlack)->AddAttributes(new xiiDefaultValueAttribute(0.3f)),
      XII_MEMBER_PROPERTY("Mass", m_fTotalMass)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, 1000.0f)),
      XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.01f, 0.5f)),
      XII_MEMBER_PROPERTY("BendStiffness", m_fBendStiffness)->AddAttributes(new xiiClampValueAttribute(0.0f,   xiiVariant())),
      XII_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(30)), new xiiClampValueAttribute(xiiAngle::Degree(5), xiiAngle::Degree(90))),
      XII_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(15)), new xiiClampValueAttribute(xiiAngle::Degree(0.01f), xiiAngle::Degree(90))),
      XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
      XII_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
      XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
      XII_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
      XII_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgPhysicsAddForce, AddForceAtPos),
      XII_MESSAGE_HANDLER(xiiMsgPhysicsAddImpulse, AddImpulseAtPos),
    }
    XII_END_MESSAGEHANDLERS;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Physics/Jolt/Animation"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltRopeComponent::xiiJoltRopeComponent()  = default;
xiiJoltRopeComponent::~xiiJoltRopeComponent() = default;

void xiiJoltRopeComponent::SetSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hSurface);
}

const char* xiiJoltRopeComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

void xiiJoltRopeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_uiPieces;
  s << m_fThickness;
  s << m_bAttachToOrigin;
  s << m_bAttachToAnchor;
  s << m_bSelfCollision;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;
  s << m_fBendStiffness;
  s << m_fTotalMass;
  s << m_fSlack;
  s << m_bCCD;

  stream.WriteGameObjectHandle(m_hAnchor);
}

void xiiJoltRopeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;
  s >> m_bAttachToOrigin;
  s >> m_bAttachToAnchor;
  s >> m_bSelfCollision;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;
  s >> m_fBendStiffness;
  s >> m_fTotalMass;
  s >> m_fSlack;
  s >> m_bCCD;

  m_hAnchor = stream.ReadGameObjectHandle();
}

void xiiJoltRopeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CreateRope();
}

void xiiJoltRopeComponent::OnActivated()
{
  UpdatePreview();
}

void xiiJoltRopeComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  // tell the render components, that the rope is gone
  xiiMsgRopePoseUpdated poseMsg;
  GetOwner()->SendMessage(poseMsg);

  SUPER::OnDeactivated();
}

const xiiJoltMaterial* xiiJoltRopeComponent::GetJoltMaterial()
{
  if (m_hSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(m_hSurface, xiiResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<const xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return xiiJoltCore::GetDefaultMaterial();
}

void xiiJoltRopeComponent::CreateRope()
{
  const xiiTransform tRoot = GetOwner()->GetGlobalTransform();

  xiiHybridArray<xiiTransform, 65> pieces;
  float                            fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength).Failed())
    return;

  pieces.PopBack(); // don't need the last transform

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  m_uiObjectFilterID          = pModule->CreateObjectFilterID();

  const xiiJoltMaterial* pMaterial = GetJoltMaterial();

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::Ref<JPH::RagdollSettings> opt = new JPH::RagdollSettings();
  opt->mSkeleton                     = new JPH::Skeleton();
  opt->mSkeleton->GetJoints().resize(pieces.GetCount());
  opt->mParts.resize(pieces.GetCount());

  const float fMass = m_fTotalMass / pieces.GetCount();

  xiiStringBuilder name;

  JPH::CapsuleShapeSettings capsule;
  capsule.mRadius               = m_fThickness * 0.5f;
  capsule.mHalfHeightOfCylinder = fPieceLength * 0.5f;
  capsule.mMaterial             = pMaterial;
  capsule.mUserData             = reinterpret_cast<xiiUInt64>(pUserData);

  JPH::RotatedTranslatedShapeSettings capsOffset;
  capsOffset.mInnerShapePtr = capsule.Create().Get();
  capsOffset.mPosition      = JPH::Vec3(fPieceLength * 0.5f, 0, 0);
  capsOffset.mRotation      = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), xiiAngle::Degree(-90).GetRadian());
  capsOffset.mUserData      = reinterpret_cast<xiiUInt64>(pUserData);

  xiiVec3 vNextJointPos;

  for (xiiUInt32 idx = 0; idx < pieces.GetCount(); ++idx)
  {
    // skeleton
    {
      auto& joint = opt->mSkeleton->GetJoint(idx);

      // set previous name as parent name
      joint.mParentName = name;

      name.Format("Link{}", idx);
      joint.mName             = name;
      joint.mParentJointIndex = static_cast<xiiInt32>(idx) - 1;
    }

    auto& part          = opt->mParts[idx];
    part.mObjectLayer   = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Rope);
    part.mGravityFactor = m_fGravityFactor;
    part.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    part.mMotionType    = JPH::EMotionType::Dynamic;
    part.mPosition      = xiiJoltConversionUtils::ToVec3(pieces[idx].m_vPosition);
    part.mRotation      = xiiJoltConversionUtils::ToQuat(pieces[idx].m_qRotation);
    part.mUserData      = reinterpret_cast<xiiUInt64>(pUserData);
    part.SetShape(capsOffset.Create().Get()); // shape is cached, only 1 is created
    part.mMassPropertiesOverride.mMass = fMass;
    part.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
    part.mRestitution                  = pMaterial->m_fRestitution;
    part.mFriction                     = pMaterial->m_fFriction;
    part.mLinearDamping                = 0.1f;
    part.mAngularDamping               = 0.1f;
    part.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    part.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below

    if (idx > 0)
    {
      JPH::SwingTwistConstraintSettings* pConstraint = new JPH::SwingTwistConstraintSettings();
      pConstraint->mDrawConstraintSize               = 0.1f;
      pConstraint->mPosition1                        = xiiJoltConversionUtils::ToVec3(vNextJointPos);
      pConstraint->mPosition2                        = xiiJoltConversionUtils::ToVec3(vNextJointPos);
      pConstraint->mNormalHalfConeAngle              = m_MaxBend.GetRadian();
      pConstraint->mPlaneHalfConeAngle               = m_MaxBend.GetRadian();
      pConstraint->mTwistAxis1                       = xiiJoltConversionUtils::ToVec3(pieces[idx - 1].m_qRotation * xiiVec3(1, 0, 0)).Normalized();
      pConstraint->mTwistAxis2                       = xiiJoltConversionUtils::ToVec3(pieces[idx].m_qRotation * xiiVec3(1, 0, 0)).Normalized();
      pConstraint->mPlaneAxis1                       = xiiJoltConversionUtils::ToVec3(pieces[idx - 1].m_qRotation * xiiVec3(0, 1, 0)).Normalized();
      pConstraint->mPlaneAxis2                       = xiiJoltConversionUtils::ToVec3(pieces[idx].m_qRotation * xiiVec3(0, 1, 0)).Normalized();
      pConstraint->mTwistMinAngle                    = -m_MaxTwist.GetRadian();
      pConstraint->mTwistMaxAngle                    = m_MaxTwist.GetRadian();
      pConstraint->mMaxFrictionTorque                = m_fBendStiffness;
      part.mToParent                                 = pConstraint;
    }

    vNextJointPos = pieces[idx].m_vPosition + pieces[idx].m_qRotation * xiiVec3(fPieceLength, 0, 0);

    if ((m_bAttachToOrigin && idx == 0) || (m_bAttachToAnchor && idx + 1 == pieces.GetCount()))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      part.mObjectLayer = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Query);
    }
  }

  opt->Stabilize();

  if (m_bSelfCollision)
  {
    // overrides the group filter above to one that allows collision with itself, except for directly joined bodies
    opt->DisableParentChildCollisions();
  }

  m_pRagdoll = opt->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<xiiUInt64>(pUserData), pModule->GetJoltSystem());
  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  if (m_bAttachToOrigin)
  {
    m_pConstraintOrigin = CreateConstraint(GetOwner()->GetHandle(), pieces[0], m_pRagdoll->GetBodyID(0).GetIndexAndSequenceNumber());
  }

  if (m_bAttachToAnchor)
  {
    xiiTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_vPosition.x = fPieceLength;

    xiiTransform lastPiece;
    lastPiece.SetGlobalTransform(pieces.PeekBack(), localTransform);

    lastPiece.m_qRotation = -lastPiece.m_qRotation;

    m_pConstraintAnchor = CreateConstraint(m_hAnchor, lastPiece, m_pRagdoll->GetBodyIDs().back().GetIndexAndSequenceNumber());
  }
}

JPH::Constraint* xiiJoltRopeComponent::CreateConstraint(const xiiGameObjectHandle& hTarget, const xiiTransform& dstLoc, xiiUInt32 uiBodyID)
{
  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();


  JPH::BodyID bodyIDs[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(uiBodyID)};

  xiiGameObject* pObject = nullptr;
  if (!GetWorld()->TryGetObject(hTarget, pObject))
    return nullptr;

  const auto location = pObject->GetGlobalTransform();

  xiiJoltDynamicActorComponent* pActor = nullptr;
  if (!pObject->TryGetComponentOfBaseType(pActor))
  {
    pObject = pObject->GetParent();

    if (pObject)
    {
      pObject->TryGetComponentOfBaseType(pActor);
    }
  }

  if (pActor)
  {
    pActor->EnsureSimulationStarted();
    bodyIDs[0] = JPH::BodyID(pActor->GetJoltBodyID());
  }


  // create the joint
  {
    JPH::BodyLockMultiWrite bodies(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyIDs, 2);

    if (bodies.GetBody(1) == nullptr)
      return nullptr;

    JPH::Body* pAnchor = bodies.GetBody(0) != nullptr ? bodies.GetBody(0) : &JPH::Body::sFixedToWorld;

    const xiiVec3 vTwistAxis1 = location.m_qRotation * xiiVec3(1, 0, 0);
    const xiiVec3 vTwistAxis2 = dstLoc.m_qRotation * xiiVec3(1, 0, 0);

    xiiQuat qAxis1to2;
    qAxis1to2.SetShortestRotation(vTwistAxis1, vTwistAxis2);

    const xiiVec3 vOrthoAxis1 = location.m_qRotation * xiiVec3(0, 1, 0);
    const xiiVec3 vOrthoAxis2 = qAxis1to2 * vOrthoAxis1;


    JPH::SwingTwistConstraintSettings constraint;
    constraint.mSpace               = JPH::EConstraintSpace::WorldSpace;
    constraint.mDrawConstraintSize  = 0.1f;
    constraint.mPosition1           = xiiJoltConversionUtils::ToVec3(location.m_vPosition);
    constraint.mPosition2           = xiiJoltConversionUtils::ToVec3(dstLoc.m_vPosition);
    constraint.mNormalHalfConeAngle = m_MaxBend.GetRadian();
    constraint.mPlaneHalfConeAngle  = m_MaxBend.GetRadian();

    constraint.mTwistAxis1 = xiiJoltConversionUtils::ToVec3(vTwistAxis1).Normalized();
    constraint.mTwistAxis2 = xiiJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();

    constraint.mPlaneAxis1 = xiiJoltConversionUtils::ToVec3(vOrthoAxis1).Normalized();
    constraint.mPlaneAxis2 = xiiJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();

    constraint.mTwistMinAngle     = -m_MaxTwist.GetRadian();
    constraint.mTwistMaxAngle     = m_MaxTwist.GetRadian();
    constraint.mMaxFrictionTorque = m_fBendStiffness;

    auto* pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(pConstraint);

    return pConstraint;
  }
}

void xiiJoltRopeComponent::UpdatePreview()
{
  xiiVec3 vNewPreviewRefPos = GetOwner()->GetGlobalPosition();

  xiiGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchor, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();

  // TODO: use a hash value instead
  vNewPreviewRefPos.x += m_fSlack;
  vNewPreviewRefPos.y += (float)m_uiPieces;

  if (vNewPreviewRefPos != m_vPreviewRefPos)
  {
    m_vPreviewRefPos = vNewPreviewRefPos;
    SendPreviewPose();
  }
}

xiiResult xiiJoltRopeComponent::CreateSegmentTransforms(xiiDynamicArray<xiiTransform>& transforms, float& out_fPieceLength) const
{
  out_fPieceLength = 0.0f;

  if (m_uiPieces == 0)
    return XII_FAILURE;

  const xiiSimdVec4f vAnchorA = xiiSimdConversion::ToVec3(GetOwner()->GetGlobalPosition());

  const xiiGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return XII_FAILURE;

  const xiiSimdVec4f vAnchorB = xiiSimdConversion::ToVec3(pAnchor->GetGlobalPosition());

  const float fLength = (vAnchorB - vAnchorA).GetLength<3>();
  if (xiiMath::IsZero(fLength, 0.001f))
    return XII_FAILURE;

  // the rope simulation always introduces some sag,
  // (m_fSlack - 0.1f) puts the rope under additional tension to counteract the imprecise simulation
  // we could also drastically ramp up the simulation steps, but that costs way too much performance
  const float fIntendedRopeLength = fLength + fLength * (xiiMath::Abs(m_fSlack) - 0.1f);

  xiiRopeSimulator rope;
  rope.m_bFirstNodeIsFixed = true;
  rope.m_bLastNodeIsFixed  = true;
  rope.m_fDampingFactor    = 0.97f;
  rope.m_fSegmentLength    = fIntendedRopeLength / m_uiPieces;
  rope.m_Nodes.SetCount(m_uiPieces + 1);
  rope.m_vAcceleration.Set(0, 0, xiiMath::Sign(m_fSlack) * -1);

  for (xiiUInt16 i = 0; i < m_uiPieces + 1; ++i)
  {
    rope.m_Nodes[i].m_vPosition         = vAnchorA + (vAnchorB - vAnchorA) * ((float)i / (float)m_uiPieces);
    rope.m_Nodes[i].m_vPreviousPosition = rope.m_Nodes[i].m_vPosition;
  }

  rope.SimulateTillEquilibrium(0.001f, 200);

  transforms.SetCountUninitialized(m_uiPieces + 1);

  out_fPieceLength = 0.0f;

  for (xiiUInt16 idx = 0; idx < m_uiPieces; ++idx)
  {
    const xiiSimdVec4f p0  = rope.m_Nodes[idx].m_vPosition;
    const xiiSimdVec4f p1  = rope.m_Nodes[idx + 1].m_vPosition;
    xiiSimdVec4f       dir = p1 - p0;

    const xiiSimdFloat len = dir.GetLength<3>();
    out_fPieceLength += len;

    if (len <= 0.001f)
      dir = xiiSimdVec4f(1, 0, 0, 0);
    else
      dir /= len;

    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = xiiSimdConversion::ToVec3(p0);
    transforms[idx].m_qRotation.SetShortestRotation(xiiVec3::UnitXAxis(), xiiSimdConversion::ToVec3(dir));
  }

  out_fPieceLength /= m_uiPieces;

  {
    xiiUInt32 idx = m_uiPieces;
    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = xiiSimdConversion::ToVec3(rope.m_Nodes[idx].m_vPosition);
    transforms[idx].m_qRotation = transforms[idx - 1].m_qRotation;
  }

  return XII_SUCCESS;
}

void xiiJoltRopeComponent::DestroyPhysicsShapes()
{
  if (m_pRagdoll)
  {
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;

    if (m_pConstraintOrigin)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintOrigin);
      m_pConstraintOrigin->Release();
      m_pConstraintOrigin = nullptr;
    }

    if (m_pConstraintAnchor)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor);
      m_pConstraintAnchor->Release();
      m_pConstraintAnchor = nullptr;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }
}

// applying forces to a ragdoll is a problem,
// since ropes have many links, things like explosions tend to apply the same force to each link
// thus multiplying the effect
// the only reliable solution seems to be to prevent too large incoming forces
// therefore a 'frame budget' is used to only apply a certain amount of force during a single frame
// this effectively ignores most forces that are applied to multiple links and just moves one or two links
constexpr float g_fMaxForce = 1.5f;

void xiiJoltRopeComponent::Update()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  m_fMaxForcePerFrame = g_fMaxForce * 2.0f;

  if (m_pRagdoll == nullptr)
    return;

  // at runtime, allow to disengage the connection
  {
    if (!m_bAttachToOrigin && m_pConstraintOrigin)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintOrigin);
      m_pConstraintOrigin->Release();
      m_pConstraintOrigin = nullptr;
      m_pRagdoll->Activate();
    }

    if (!m_bAttachToAnchor && m_pConstraintAnchor)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor);
      m_pConstraintAnchor->Release();
      m_pConstraintAnchor = nullptr;
      m_pRagdoll->Activate();
    }
  }

  // if (m_fWindInfluence > 0.0f)
  //{
  //   if (const xiiWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<xiiWindWorldModuleInterface>())
  //   {
  //     xiiVec3 ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

  //    const xiiVec3 vWind = pWind->GetWindAt(m_RopeSim.m_Nodes.PeekBack().m_vPosition) * m_fWindInfluence;

  //    xiiVec3 windForce = vWind;
  //    windForce += pWind->ComputeWindFlutter(vWind, ropeDir, 10.0f, GetOwner()->GetStableRandomSeed());

  //    if (!windForce.IsZero())
  //    {
  //      // apply force to all articulation links
  //    }
  //  }
  //}

  // if one is inactive, all the linked bodies are inactive
  if (!pModule->GetJoltSystem()->GetBodyInterface().IsActive(m_pRagdoll->GetBodyID(0)))
    return;

  xiiHybridArray<xiiTransform, 32> poses(xiiFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(static_cast<xiiUInt32>(m_pRagdoll->GetBodyCount()) + 1);

  xiiMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  JPH::Vec3 rootPos;
  JPH::Quat rootRot;
  m_pRagdoll->GetRootTransform(rootPos, rootRot);

  xiiTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition  = xiiJoltConversionUtils::ToVec3(rootPos);

  GetOwner()->SetGlobalPosition(rootTransform.m_vPosition);

  auto& lockInterface = pModule->GetJoltSystem()->GetBodyLockInterface();

  xiiTransform global;
  global.m_vScale.Set(1);

  JPH::BodyLockMultiRead lock(lockInterface, m_pRagdoll->GetBodyIDs().data(), (int)m_pRagdoll->GetBodyCount());

  for (xiiUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    if (auto pBody = lock.GetBody(i))
    {
      global.m_vPosition = xiiJoltConversionUtils::ToVec3(pBody->GetPosition());
      global.m_qRotation = xiiJoltConversionUtils::ToQuat(pBody->GetRotation());

      poses[i].SetLocalTransform(rootTransform, global);
    }
  }

  // last pose
  {
    const xiiUInt32 uiLastIdx = static_cast<xiiUInt32>(m_pRagdoll->GetBodyCount());

    xiiTransform tLocal;
    tLocal.SetIdentity();
    tLocal.m_vPosition.x = (poses[uiLastIdx - 1].m_vPosition - poses[uiLastIdx - 2].m_vPosition).GetLength();

    poses.PeekBack().SetGlobalTransform(poses[uiLastIdx - 1], tLocal);
  }

  GetOwner()->SendMessage(poseMsg);
}

void xiiJoltRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  xiiDynamicArray<xiiTransform> pieces(xiiFrameAllocator::GetCurrentAllocator());

  xiiMsgRopePoseUpdated poseMsg;
  float                 fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength).Succeeded())
  {
    poseMsg.m_LinkTransforms = pieces;

    const xiiTransform tOwner = GetOwner()->GetGlobalTransform();

    for (auto& n : pieces)
    {
      n.SetLocalTransform(tOwner, n);
    }
  }

  GetOwner()->PostMessage(poseMsg, xiiTime::Zero(), xiiObjectMsgQueueType::AfterInitialized);
}

void xiiJoltRopeComponent::SetGravityFactor(float fGravity)
{
  if (m_fGravityFactor == fGravity)
    return;

  m_fGravityFactor = fGravity;

  if (!m_pRagdoll)
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  for (xiiUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void xiiJoltRopeComponent::SetAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor(resolver(szReference, GetHandle(), "Anchor"));
}

void xiiJoltRopeComponent::SetAnchor(xiiGameObjectHandle hActor)
{
  m_hAnchor = hActor;
}

void xiiJoltRopeComponent::AddForceAtPos(xiiMsgPhysicsAddForce& msg)
{
  if (m_pRagdoll == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  JPH::BodyID bodyId;

  if (msg.m_pInternalPhysicsActor != nullptr)
    bodyId = JPH::BodyID(reinterpret_cast<size_t>(msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  else
    bodyId = m_pRagdoll->GetBodyID(0);

  xiiVec3     vImp    = msg.m_vForce;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();
  pModule->GetJoltSystem()->GetBodyInterface().AddForce(bodyId, xiiJoltConversionUtils::ToVec3(vImp), xiiJoltConversionUtils::ToVec3(msg.m_vGlobalPosition));
}

void xiiJoltRopeComponent::AddImpulseAtPos(xiiMsgPhysicsAddImpulse& msg)
{
  if (m_pRagdoll == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  JPH::BodyID bodyId;

  if (msg.m_pInternalPhysicsActor != nullptr)
    bodyId = JPH::BodyID(reinterpret_cast<size_t>(msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  else
    bodyId = m_pRagdoll->GetBodyID(0);

  xiiVec3     vImp    = msg.m_vImpulse;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();
  pModule->GetJoltSystem()->GetBodyInterface().AddImpulse(bodyId, xiiJoltConversionUtils::ToVec3(vImp), xiiJoltConversionUtils::ToVec3(msg.m_vGlobalPosition));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiJoltRopeComponentManager::xiiJoltRopeComponentManager(xiiWorld* pWorld) :
  xiiComponentManager(pWorld)
{
}

xiiJoltRopeComponentManager::~xiiJoltRopeComponentManager() = default;

void xiiJoltRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltRopeComponentManager::Update, this);
    desc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void xiiJoltRopeComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePreview();
      }
    }

    return;
  }

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();
  if (pModule == nullptr)
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
