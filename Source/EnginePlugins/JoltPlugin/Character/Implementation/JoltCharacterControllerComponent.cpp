#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiJoltCharacterDebugFlags, 1)
XII_BITFLAGS_CONSTANTS(xiiJoltCharacterDebugFlags::PrintState, xiiJoltCharacterDebugFlags::VisShape, xiiJoltCharacterDebugFlags::VisContacts,  xiiJoltCharacterDebugFlags::VisCasts, xiiJoltCharacterDebugFlags::VisGroundContact, xiiJoltCharacterDebugFlags::VisFootCheck)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltCharacterControllerComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("PresenceCollisionLayer", m_uiPresenceCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new xiiDefaultValueAttribute(70.0f), new xiiClampValueAttribute(0.1f, 10000.0f)),
    XII_ACCESSOR_PROPERTY("Strength", GetStrength, SetStrength)->AddAttributes(new xiiDefaultValueAttribute(500.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("MaxClimbingSlope", GetMaxClimbingSlope, SetMaxClimbingSlope)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(40))),
    XII_BITFLAGS_MEMBER_PROPERTY("DebugFlags", xiiJoltCharacterDebugFlags , m_DebugFlags),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Character"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiJoltCharacterControllerComponent::xiiJoltCharacterControllerComponent()  = default;
xiiJoltCharacterControllerComponent::~xiiJoltCharacterControllerComponent() = default;

void xiiJoltCharacterControllerComponent::SetObjectToIgnore(xiiUInt32 uiObjectFilterID)
{
  m_BodyFilter.m_uiObjectFilterIDToIgnore = uiObjectFilterID;
}

void xiiJoltCharacterControllerComponent::ClearObjectToIgnore()
{
  m_BodyFilter.ClearFilter();
}

void xiiJoltCharacterControllerComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_DebugFlags;

  s << m_uiCollisionLayer;
  s << m_uiPresenceCollisionLayer;
  s << m_fMass;
  s << m_fStrength;
  s << m_MaxClimbingSlope;
}

void xiiJoltCharacterControllerComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = stream.GetStream();

  s >> m_DebugFlags;

  s >> m_uiCollisionLayer;
  s >> m_uiPresenceCollisionLayer;
  s >> m_fMass;
  s >> m_fStrength;
  s >> m_MaxClimbingSlope;
}

void xiiJoltCharacterControllerComponent::OnDeactivated()
{
  if (m_pCharacter)
  {
    if (xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>())
    {
      pModule->ActivateCharacterController(this, false);
    }

    m_pCharacter->Release();
    m_pCharacter = nullptr;
  }

  SUPER::OnDeactivated();
}

void xiiJoltCharacterControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  JPH::CharacterVirtualSettings opt;
  opt.mUp               = JPH::Vec3::sAxisZ();
  opt.mSupportingVolume = JPH::Plane(opt.mUp, -GetShapeRadius());
  opt.mShape            = MakeNextCharacterShape();
  opt.mMaxSlopeAngle    = m_MaxClimbingSlope.GetRadian();
  opt.mMass             = m_fMass;
  opt.mMaxStrength      = m_fStrength;

  const xiiTransform ownTrans = GetOwner()->GetGlobalTransform();

  m_pCharacter = new JPH::CharacterVirtual(&opt, xiiJoltConversionUtils::ToVec3(ownTrans.m_vPosition), xiiJoltConversionUtils::ToQuat(ownTrans.m_qRotation), pModule->GetJoltSystem());
  m_pCharacter->AddRef();

  pModule->ActivateCharacterController(this, true);

  CreatePresenceBody();
}

void xiiJoltCharacterControllerComponent::SetMaxClimbingSlope(xiiAngle slope)
{
  m_MaxClimbingSlope = slope;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxSlopeAngle(m_MaxClimbingSlope.GetRadian());
  }
}

void xiiJoltCharacterControllerComponent::SetMass(float mass)
{
  m_fMass = mass;

  if (m_pCharacter)
  {
    m_pCharacter->SetMass(m_fMass);
  }
}

void xiiJoltCharacterControllerComponent::SetStrength(float strength)
{
  m_fStrength = strength;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxStrength(m_fStrength);
  }
}

xiiResult xiiJoltCharacterControllerComponent::TryChangeShape(JPH::Shape* pNewShape)
{
  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);
  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  if (m_pCharacter->SetShape(pNewShape, 0.01f, broadphaseFilter, objectFilter, m_BodyFilter, *pModule->GetTempAllocator()))
  {
    RemovePresenceBody();
    CreatePresenceBody();

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiJoltCharacterControllerComponent::RawMoveWithVelocity(const xiiVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown)
{
  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);
  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);

  m_pCharacter->SetLinearVelocity(xiiJoltConversionUtils::ToVec3(vVelocity));

  // Settings for our update function
  JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
  updateSettings.mStickToFloorStepDown = JPH::Vec3(0, 0, -fMaxStepDown);
  updateSettings.mWalkStairsStepUp     = fMaxStairStepUp > 0 ? JPH::Vec3(0, 0, fMaxStairStepUp) : JPH::Vec3::sZero();

  // Update the character position
  m_pCharacter->ExtendedUpdate(GetUpdateTimeDelta(), xiiJoltConversionUtils::ToVec3(pModule->GetCharacterGravity()), updateSettings, broadphaseFilter, objectFilter, m_BodyFilter, *pModule->GetTempAllocator());

  GetOwner()->SetGlobalPosition(xiiJoltConversionUtils::ToSimdVec3(m_pCharacter->GetPosition()));
}

// float xiiJoltCharacterControllerComponent::ClampedRawMoveIntoDirection(const xiiVec3& vDirection)
//{
//   float fMaxDistance = vDirection.GetLength();
//
//   if (fMaxDistance <= 0.0f)
//     return 0.0f;
//
//   const xiiVec3 vDirNormal = vDirection / fMaxDistance;
//   const xiiVec3 vShapePos = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetCenterOfMassTransform().GetTranslation());
//
//   xiiHybridArray<ContactPoint, 32> contacts;
//   CollectCastContacts(contacts, GetJoltCharacter()->GetShape(), vShapePos, xiiQuat::IdentityQuaternion(), vDirection /*+ vDirNormal*/);
//
//   xiiDebugRenderer::DrawCross(GetWorld(), vShapePos, 0.2f, xiiColor::GreenYellow);
//
//
//   const float fPadding = GetJoltCharacter()->GetCharacterPadding();
//   float fMoveDistance = fMaxDistance;
//
//   if (!contacts.IsEmpty())
//   {
//     // float fFraction = 1.0f;
//
//     xiiQuat rot;
//     xiiUInt32 uiClosestContact = 0;
//     float fClosestContact = 1.1f;
//
//     for (xiiUInt32 c = 0; c < contacts.GetCount(); ++c)
//     {
//       if (contacts[c].m_fCastFraction < fClosestContact)
//       {
//         uiClosestContact = c;
//         fClosestContact = contacts[c].m_fCastFraction;
//       }
//     }
//
//     const ContactPoint& cont = contacts[uiClosestContact];
//     const xiiAngle alpha = xiiMath::ACos(cont.m_vContactNormal.Dot(-vDirNormal));
//     const float sinAlpha = xiiMath::Sin(alpha);
//     const float gegenKath = fPadding;
//     const float fHypoth = gegenKath / sinAlpha;
//
//     // for (const ContactPoint& contact : contacts)
//     //{
//     //   // ignore contacts that we are moving away from or parallel to
//     //   if (contact.m_vContactNormal.Dot(vDirNormal) >= 0.0f)
//     //   {
//     //     rot.SetShortestRotation(xiiVec3::UnitXAxis(), contact.m_vContactNormal);
//     //     xiiDebugRenderer::DrawCylinder(GetWorld(), 0.0f, 0.05f, 0.1f, xiiColor::ZeroColor(), xiiColor::DimGrey, xiiTransform(contact.m_vPosition, rot));
//     //     continue;
//     //   }
//
//     //  fFraction = xiiMath::Min(contact.m_fCastFraction, fFraction);
//     fMoveDistance = xiiMath::Min((fMaxDistance /*+ 1.0f*/) * cont.m_fCastFraction, fMaxDistance) - fHypoth;
//
//     //  {
//     //    rot.SetShortestRotation(xiiVec3::UnitXAxis(), contact.m_vContactNormal);
//     //    xiiDebugRenderer::DrawCylinder(GetWorld(), 0.0f, 0.05f, 0.1f, xiiColor::ZeroColor(), xiiColor::Black, xiiTransform(contact.m_vPosition, rot));
//     //  }
//     //}
//   }
//
//   if (fMoveDistance > 0.0)
//   {
//     RawMoveIntoDirection(vDirNormal * fMoveDistance, false);
//   }
//
//   return fMaxDistance;
// }

void xiiJoltCharacterControllerComponent::RawMoveIntoDirection(const xiiVec3& vDirection)
{
  if (vDirection.IsZero())
    return;

  RawMoveWithVelocity(vDirection * GetInverseUpdateTimeDelta(), 0.0f, 0.0f);
}

void xiiJoltCharacterControllerComponent::RawMoveToPosition(const xiiVec3& vTargetPosition)
{
  RawMoveIntoDirection(vTargetPosition - GetOwner()->GetGlobalPosition());
}

void xiiJoltCharacterControllerComponent::TeleportToPosition(const xiiVec3& vGlobalFootPos)
{
  m_pCharacter->SetPosition(xiiJoltConversionUtils::ToVec3(vGlobalFootPos));

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);
  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  m_pCharacter->RefreshContacts(broadphaseFilter, objectFilter, m_BodyFilter, *pModule->GetTempAllocator());
}

bool xiiJoltCharacterControllerComponent::StickToGround(float fMaxDist)
{
  if (m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::InAir || m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::NotSupported)
    return false;

  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);
  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  return m_pCharacter->StickToFloor(JPH::Vec3(0, 0, -fMaxDist), broadphaseFilter, objectFilter, m_BodyFilter, *pModule->GetTempAllocator());
}

void xiiJoltCharacterControllerComponent::CollectCastContacts(xiiDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const xiiVec3& vQueryPosition, const xiiQuat& qQueryRotation, const xiiVec3& vSweepDir) const
{
  out_Contacts.Clear();

  class ContactCastCollector : public JPH::CastShapeCollector
  {
  public:
    xiiDynamicArray<ContactPoint>* m_pContacts      = nullptr;
    const JPH::BodyLockInterface*  m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::ShapeCastResult& inResult) override
    {
      auto& contact            = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition      = xiiJoltConversionUtils::ToVec3(inResult.mContactPointOn2);
      contact.m_vContactNormal = xiiJoltConversionUtils::ToVec3(-inResult.mPenetrationAxis.Normalized());
      contact.m_BodyID         = inResult.mBodyID2;
      contact.m_fCastFraction  = inResult.mFraction;
      contact.m_SubShapeID     = inResult.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = xiiJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, inResult.mContactPointOn2));
    }
  };

  const xiiJoltWorldModule* pModule     = GetWorld()->GetModule<xiiJoltWorldModule>();
  const auto                pJoltSystem = pModule->GetJoltSystem();

  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);
  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  ContactCastCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts      = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(xiiJoltConversionUtils::ToQuat(qQueryRotation), xiiJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::ShapeCast castOpt(pShape, JPH::Vec3::sReplicate(1.0f), trans, xiiJoltConversionUtils::ToVec3(vSweepDir));

  JPH::ShapeCastSettings settings;
  pJoltSystem->GetNarrowPhaseQuery().CastShape(castOpt, settings, collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

void xiiJoltCharacterControllerComponent::CollectContacts(xiiDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const xiiVec3& vQueryPosition, const xiiQuat& qQueryRotation, float fCollisionTolerance) const
{
  out_Contacts.Clear();

  class ContactCollector : public JPH::CollideShapeCollector
  {
  public:
    xiiDynamicArray<ContactPoint>* m_pContacts      = nullptr;
    const JPH::BodyLockInterface*  m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::CollideShapeResult& inResult) override
    {
      auto& contact            = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition      = xiiJoltConversionUtils::ToVec3(inResult.mContactPointOn2);
      contact.m_vContactNormal = xiiJoltConversionUtils::ToVec3(-inResult.mPenetrationAxis.Normalized());
      contact.m_BodyID         = inResult.mBodyID2;
      contact.m_SubShapeID     = inResult.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = xiiJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, inResult.mContactPointOn2));
    }
  };

  const xiiJoltWorldModule* pModule     = GetWorld()->GetModule<xiiJoltWorldModule>();
  const auto                pJoltSystem = pModule->GetJoltSystem();

  xiiJoltObjectLayerFilter     objectFilter(m_uiCollisionLayer);
  xiiJoltBroadPhaseLayerFilter broadphaseFilter(xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  ContactCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts      = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(xiiJoltConversionUtils::ToQuat(qQueryRotation), xiiJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::CollideShapeSettings settings;
  settings.mCollisionTolerance = fCollisionTolerance;
  settings.mBackFaceMode       = JPH::EBackFaceMode::CollideWithBackFaces;

  pJoltSystem->GetNarrowPhaseQuery().CollideShape(pShape, JPH::Vec3::sReplicate(1.0f), trans, settings, collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

xiiVec3 xiiJoltCharacterControllerComponent::GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce)
{
  if (contact.m_BodyID.IsInvalid())
    return xiiVec3::ZeroVector();

  xiiJoltWorldModule* pModule     = GetWorld()->GetModule<xiiJoltWorldModule>();
  auto                pJoltSystem = pModule->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pJoltSystem->GetBodyLockInterface(), contact.m_BodyID);

  if (!bodyLock.Succeeded())
    return xiiVec3::ZeroVector();

  const JPH::Vec3 vGroundPos = xiiJoltConversionUtils::ToVec3(contact.m_vPosition);

  if (fPushForce > 0 && bodyLock.GetBody().IsDynamic())
  {
    const xiiVec3 vPushDir = -contact.m_vSurfaceNormal * fPushForce;

    bodyLock.GetBody().AddForce(xiiJoltConversionUtils::ToVec3(vPushDir), vGroundPos);
    pJoltSystem->GetBodyInterfaceNoLock().ActivateBody(contact.m_BodyID);
  }

  xiiVec3 vGroundVelocity = xiiVec3::ZeroVector();

  if (bodyLock.GetBody().IsKinematic())
  {
    vGroundVelocity   = xiiJoltConversionUtils::ToVec3(bodyLock.GetBody().GetPointVelocity(vGroundPos));
    vGroundVelocity.z = 0;
  }

  return vGroundVelocity;
}

void xiiJoltCharacterControllerComponent::SpawnContactInteraction(const ContactPoint& contact, const xiiHashedString& sSurfaceInteraction, xiiSurfaceResourceHandle hFallbackSurface, const xiiVec3& vInteractionNormal)
{
  if (contact.m_BodyID.IsInvalid())
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();

  xiiSurfaceResourceHandle hSurface = hFallbackSurface;

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterfaceNoLock(), contact.m_BodyID);
  if (lock.Succeeded())
  {
    auto pMat = static_cast<const xiiJoltMaterial*>(lock.GetBody().GetShape()->GetMaterial(contact.m_SubShapeID));
    if (pMat && pMat->m_pSurface)
    {
      hSurface = static_cast<const xiiJoltMaterial*>(pMat)->m_pSurface->GetResourceHandle();
    }
  }

  if (hSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(hSurface, xiiResourceAcquireMode::AllowLoadingFallback);
    pSurface->InteractWithSurface(GetWorld(), xiiGameObjectHandle(), contact.m_vPosition, contact.m_vSurfaceNormal, vInteractionNormal, sSurfaceInteraction, &GetOwner()->GetTeamID());
  }
}

// xiiBitflags<xiiJoltCharacterControllerComponent::ShapeContacts> xiiJoltCharacterControllerComponent::ClassifyContacts(const xiiDynamicArray<ContactPoint>& contacts, xiiAngle maxSlopeAngle, const xiiVec3& vCenterPos, xiiUInt32* out_pBestGroundContact)
//{
//   xiiBitflags<ShapeContacts> flags;
//
//   if (contacts.IsEmpty())
//     return flags;
//
//   const float fMaxSlopeAngleCos = xiiMath::Cos(maxSlopeAngle);
//
//   float fFlattestContactCos = -2.0f;    // overall flattest contact point
//   float fClosestContactFraction = 2.0f; // closest contact point that is flat enough
//
//   if (out_pBestGroundContact)
//     *out_pBestGroundContact = xiiInvalidIndex;
//
//   for (xiiUInt32 idx = 0; idx < contacts.GetCount(); ++idx)
//   {
//     const auto& contact = contacts[idx];
//
//     const float fContactAngleCos = contact.m_vSurfaceNormal.Dot(xiiVec3(0, 0, 1));
//
//     if (contact.m_vPosition.z > vCenterPos.z) // contact above
//     {
//       if (fContactAngleCos < -fMaxSlopeAngleCos)
//       {
//         // TODO: have dedicated max angle value
//         flags.Add(ShapeContacts::Ceiling);
//       }
//     }
//     else // contact below
//     {
//       if (fContactAngleCos > fMaxSlopeAngleCos) // is contact flat enough to stand on?
//       {
//         flags.Add(ShapeContacts::FlatGround);
//
//         if (out_pBestGroundContact && contact.m_fCastFraction < fClosestContactFraction) // contact closer than previous one?
//         {
//           fClosestContactFraction = contact.m_fCastFraction;
//           *out_pBestGroundContact = idx;
//         }
//       }
//       else
//       {
//         flags.Add(ShapeContacts::SteepGround);
//
//         if (out_pBestGroundContact && fContactAngleCos > fFlattestContactCos) // is contact flatter than previous one?
//         {
//           fFlattestContactCos = fContactAngleCos;
//
//           if (!flags.IsSet(ShapeContacts::FlatGround))
//           {
//             *out_pBestGroundContact = idx;
//           }
//         }
//       }
//     }
//   }
//
//   if (flags.IsSet(ShapeContacts::FlatGround))
//   {
//     flags.Remove(ShapeContacts::SteepGround);
//   }
//
//   if (flags.IsSet(ShapeContacts::SteepGround))
//   {
//     return flags;
//   }
//
//   return flags;
// }
//
// xiiUInt32 xiiJoltCharacterControllerComponent::FindFlattestContact(const xiiDynamicArray<ContactPoint>& contacts, const xiiVec3& vNormal, ContactFilter filter)
//{
//   xiiUInt32 uiBestIdx = xiiInvalidIndex;
//
//   float fFlattestContactCos = -2.0f; // overall flattest contact point
//
//   for (xiiUInt32 idx = 0; idx < contacts.GetCount(); ++idx)
//   {
//     const auto& contact = contacts[idx];
//
//     if (filter.IsValid() && !filter(contact))
//       continue;
//
//     const float fContactAngleCos = contact.m_vSurfaceNormal.Dot(xiiVec3(0, 0, 1));
//
//     if (fContactAngleCos > fFlattestContactCos) // is contact flatter than previous one?
//     {
//       fFlattestContactCos = fContactAngleCos;
//       uiBestIdx = idx;
//     }
//   }
//
//   return uiBestIdx;
// }

void xiiJoltCharacterControllerComponent::VisualizeContact(const ContactPoint& contact, const xiiColor& color) const
{
  xiiTransform trans;
  trans.m_vPosition = contact.m_vPosition;
  trans.m_qRotation.SetShortestRotation(xiiVec3::UnitXAxis(), contact.m_vContactNormal);
  trans.m_vScale.Set(1.0f);

  xiiDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, xiiColor::ZeroColor(), color, trans);
}

void xiiJoltCharacterControllerComponent::VisualizeContacts(const xiiDynamicArray<ContactPoint>& contacts, const xiiColor& color) const
{
  for (const auto& ct : contacts)
  {
    VisualizeContact(ct, color);
  }
}

void xiiJoltCharacterControllerComponent::Update(xiiTime deltaTime)
{
  m_fUpdateTimeDelta        = deltaTime.AsFloatInSeconds();
  m_fInverseUpdateTimeDelta = static_cast<float>(1.0 / deltaTime.GetSeconds());

  MovePresenceBody(deltaTime);

  UpdateCharacter();
}

void xiiJoltCharacterControllerComponent::CreatePresenceBody()
{
  xiiJoltWorldModule*    pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  const xiiSimdTransform trans   = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem   = pModule->GetJoltSystem();
  auto* pBodies   = &pSystem->GetBodyInterface();
  auto* pMaterial = xiiJoltCore::GetDefaultMaterial();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(m_pCharacter->GetShape());

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  xiiUInt32 m_uiObjectFilterID = pModule->CreateObjectFilterID();

  bodyCfg.mPosition    = xiiJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation    = xiiJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType  = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiPresenceCollisionLayer, xiiJoltBroadphaseLayer::Character);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<xiiUInt64>(pUserData);

  JPH::Body* pBody   = pBodies->CreateBody(bodyCfg);
  m_uiPresenceBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);
}

void xiiJoltCharacterControllerComponent::RemovePresenceBody()
{
  if (m_uiPresenceBodyID == xiiInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  m_uiPresenceBodyID = xiiInvalidIndex;

  if (bodyId.IsInvalid())
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();
  auto*               pBodies = &pSystem->GetBodyInterface();

  pBodies->RemoveBody(bodyId);
  pBodies->DestroyBody(bodyId);

  pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

void xiiJoltCharacterControllerComponent::MovePresenceBody(xiiTime deltaTime)
{
  if (m_uiPresenceBodyID == xiiInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  if (bodyId.IsInvalid())
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();
  auto*               pBodies = &pSystem->GetBodyInterface();

  const xiiSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  const float tDiff = deltaTime.AsFloatInSeconds();

  pBodies->MoveKinematic(bodyId, xiiJoltConversionUtils::ToVec3(trans.m_Position), xiiJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized(), tDiff);
}
