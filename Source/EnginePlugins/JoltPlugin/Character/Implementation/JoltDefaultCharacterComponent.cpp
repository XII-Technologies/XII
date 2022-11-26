#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltDefaultCharacterComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

xiiCVarBool cvar_JoltCcFootCheck("Jolt.CC.FootCheck", true, xiiCVarFlags::Default, "Stay down");

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltDefaultCharacterComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ShapeRadius", m_fShapeRadius)->AddAttributes(new xiiDefaultValueAttribute(0.25f)),
    XII_MEMBER_PROPERTY("CrouchHeight", m_fCylinderHeightCrouch)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("StandHeight", m_fCylinderHeightStand)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new xiiDefaultValueAttribute(0.15f)),
    XII_MEMBER_PROPERTY("WalkSpeedCrouching", m_fWalkSpeedCrouching)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("WalkSpeedStanding", m_fWalkSpeedStanding)->AddAttributes(new xiiDefaultValueAttribute(2.5f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("WalkSpeedRunning", m_fWalkSpeedRunning)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new xiiDefaultValueAttribute(2.5f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("MaxStepUp", m_fMaxStepUp)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("MaxStepDown", m_fMaxStepDown)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, 1000.0f)),
    XII_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(90.0f)), new xiiClampValueAttribute(xiiAngle::Degree(1.0f), xiiAngle::Degree(360.0f))),
    XII_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new xiiDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new xiiDefaultValueAttribute(xiiStringView("Footstep"))),
    XII_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new xiiDefaultValueAttribute(3.0f)),
    XII_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")),
    XII_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCapsuleVisualizerAttribute("StandHeight", "ShapeRadius", xiiColor::WhiteSmoke, nullptr, xiiVisualizerAnchor::NegZ),
    new xiiCapsuleVisualizerAttribute("CrouchHeight", "ShapeRadius", xiiColor::LightSlateGrey, nullptr, xiiVisualizerAnchor::NegZ),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgMoveCharacterController, SetInputState),
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgApplyRootMotion, OnApplyRootMotion),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    //XII_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    XII_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsStandingOnGround),
    XII_SCRIPT_FUNCTION_PROPERTY(IsSlidingOnGround),
    XII_SCRIPT_FUNCTION_PROPERTY(IsInAir),
    XII_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJoltDefaultCharacterComponent::xiiJoltDefaultCharacterComponent()  = default;
xiiJoltDefaultCharacterComponent::~xiiJoltDefaultCharacterComponent() = default;

void xiiJoltDefaultCharacterComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingSphere(xiiVec3(0, 0, GetShapeRadius()), GetShapeRadius()), xiiInvalidSpatialDataCategory);
  msg.AddBounds(xiiBoundingSphere(xiiVec3(0, 0, GetCurrentCapsuleHeight() - GetShapeRadius()), GetShapeRadius()), xiiInvalidSpatialDataCategory);
}

void xiiJoltDefaultCharacterComponent::OnApplyRootMotion(xiiMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion = msg.m_vTranslation;
  m_InputRotateZ += msg.m_RotationZ;
}

void xiiJoltDefaultCharacterComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_RotateSpeed;
  s << m_fShapeRadius;
  s << m_fCylinderHeightCrouch;
  s << m_fCylinderHeightStand;
  s << m_fWalkSpeedCrouching;
  s << m_fWalkSpeedStanding;
  s << m_fWalkSpeedRunning;
  s << m_fMaxStepUp;
  s << m_fMaxStepDown;
  s << m_fJumpImpulse;
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;
  s << m_fAirFriction;
  s << m_fAirSpeed;
  s << m_fFootRadius;

  stream.WriteGameObjectHandle(m_hHeadObject);
}

void xiiJoltDefaultCharacterComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = stream.GetStream();

  s >> m_RotateSpeed;
  s >> m_fShapeRadius;
  s >> m_fCylinderHeightCrouch;
  s >> m_fCylinderHeightStand;
  s >> m_fWalkSpeedCrouching;
  s >> m_fWalkSpeedStanding;
  s >> m_fWalkSpeedRunning;
  s >> m_fMaxStepUp;
  s >> m_fMaxStepDown;
  s >> m_fJumpImpulse;
  s >> m_sWalkSurfaceInteraction;
  s >> m_fWalkInteractionDistance;
  s >> m_fRunInteractionDistance;
  s >> m_hFallbackWalkSurface;
  s >> m_fAirFriction;
  s >> m_fAirSpeed;
  s >> m_fFootRadius;

  m_hHeadObject = stream.ReadGameObjectHandle();

  ResetInternalState();
}

void xiiJoltDefaultCharacterComponent::ResetInternalState()
{
  m_fShapeRadius          = xiiMath::Clamp(m_fShapeRadius, 0.05f, 5.0f);
  m_fFootRadius           = xiiMath::Clamp(m_fFootRadius, 0.01f, m_fShapeRadius);
  m_fCylinderHeightCrouch = xiiMath::Max(m_fCylinderHeightCrouch, 0.01f);
  m_fCylinderHeightStand  = xiiMath::Max(m_fCylinderHeightStand, m_fCylinderHeightCrouch);
  m_fMaxStepUp            = xiiMath::Clamp(m_fMaxStepUp, 0.0f, m_fCylinderHeightStand);
  m_fMaxStepDown          = xiiMath::Clamp(m_fMaxStepDown, 0.0f, m_fCylinderHeightStand);

  m_fNextCylinderHeight    = m_fCylinderHeightStand;
  m_fCurrentCylinderHeight = m_fNextCylinderHeight;

  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  m_fAccumulatedWalkDistance = 0;
}

void xiiJoltDefaultCharacterComponent::ResetInputState()
{
  m_vInputDirection.SetZero();
  m_InputRotateZ     = xiiAngle();
  m_uiInputCrouchBit = 0;
  m_uiInputRunBit    = 0;
  m_uiInputJumpBit   = 0;
  m_vAbsoluteRootMotion.SetZero();
}

void xiiJoltDefaultCharacterComponent::SetInputState(xiiMsgMoveCharacterController& msg)
{
  const float fDistanceToMove = xiiMath::Max(xiiMath::Abs((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards)), xiiMath::Abs((float)(msg.m_fStrafeRight - msg.m_fStrafeLeft)));

  m_vInputDirection += xiiVec2((float)(msg.m_fMoveForwards - msg.m_fMoveBackwards), (float)(msg.m_fStrafeRight - msg.m_fStrafeLeft));
  m_vInputDirection.NormalizeIfNotZero(xiiVec2::ZeroVector()).IgnoreResult();
  m_vInputDirection *= fDistanceToMove;

  m_InputRotateZ += m_RotateSpeed * (float)(msg.m_fRotateRight - msg.m_fRotateLeft);

  if (msg.m_bRun)
  {
    m_uiInputRunBit = 1;
  }

  if (msg.m_bJump)
  {
    m_uiInputJumpBit = 1;
  }

  if (msg.m_bCrouch)
  {
    m_uiInputCrouchBit = 1;
  }
}

float xiiJoltDefaultCharacterComponent::GetCurrentCylinderHeight() const
{
  return m_fCurrentCylinderHeight;
}

float xiiJoltDefaultCharacterComponent::GetCurrentCapsuleHeight() const
{
  return GetCurrentCylinderHeight() + 2.0f * GetShapeRadius();
}

float xiiJoltDefaultCharacterComponent::GetShapeRadius() const
{
  return m_fShapeRadius;
}

void xiiJoltDefaultCharacterComponent::TeleportCharacter(const xiiVec3& vGlobalFootPosition)
{
  TeleportToPosition(vGlobalFootPosition);
}

void xiiJoltDefaultCharacterComponent::OnActivated()
{
  SUPER::OnActivated();

  ResetInternalState();

  GetOwner()->UpdateLocalBounds();
}

void xiiJoltDefaultCharacterComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

JPH::Ref<JPH::Shape> xiiJoltDefaultCharacterComponent::MakeNextCharacterShape()
{
  const float fTotalCapsuleHeight = m_fNextCylinderHeight + 2.0f * GetShapeRadius();

  JPH::CapsuleShapeSettings opt;
  opt.mRadius               = GetShapeRadius();
  opt.mHalfHeightOfCylinder = 0.5f * m_fNextCylinderHeight;

  JPH::RotatedTranslatedShapeSettings up;
  up.mInnerShapePtr = opt.Create().Get();
  up.mPosition      = JPH::Vec3(0, 0, fTotalCapsuleHeight * 0.5f);
  up.mRotation      = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ());

  return up.Create().Get();
}

void xiiJoltDefaultCharacterComponent::SetHeadObjectReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
}

void xiiJoltDefaultCharacterComponent::OnSimulationStarted()
{
  ResetInternalState();

  // creates the CC, so the next shape size must be set already
  SUPER::OnSimulationStarted();

  xiiGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
    m_fHeadTargetHeight = m_fHeadHeightOffset;
  }
}

void xiiJoltDefaultCharacterComponent::ApplyRotationZ()
{
  if (m_InputRotateZ.GetRadian() == 0.0f)
    return;

  xiiQuat qRotZ;
  qRotZ.SetFromAxisAndAngle(xiiVec3(0, 0, 1), m_InputRotateZ);
  m_InputRotateZ.SetRadian(0.0);

  GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
}

void xiiJoltDefaultCharacterComponent::SetFallbackWalkSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackWalkSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  if (m_hFallbackWalkSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

const char* xiiJoltDefaultCharacterComponent::GetFallbackWalkSurfaceFile() const
{
  if (!m_hFallbackWalkSurface.IsValid())
    return "";

  return m_hFallbackWalkSurface.GetResourceID();
}

void xiiJoltDefaultCharacterComponent::ApplyCrouchState()
{
  if (m_uiInputCrouchBit == m_uiIsCrouchingBit)
    return;

  m_fNextCylinderHeight = m_uiInputCrouchBit ? m_fCylinderHeightCrouch : m_fCylinderHeightStand;

  if (TryChangeShape(MakeNextCharacterShape().GetPtr()).Succeeded())
  {
    m_uiIsCrouchingBit       = m_uiInputCrouchBit;
    m_fCurrentCylinderHeight = m_fNextCylinderHeight;
  }
}

void xiiJoltDefaultCharacterComponent::StoreLateralVelocity()
{
  const xiiVec3 endPosition = GetOwner()->GetGlobalPosition();
  const xiiVec3 vVelocity   = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_vVelocityLateral.Set(vVelocity.x, vVelocity.y);
}

void xiiJoltDefaultCharacterComponent::ClampLateralVelocity()
{
  const xiiVec3 endPosition = GetOwner()->GetGlobalPosition();
  const xiiVec3 vVelocity   = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  xiiVec2 vRealDirLateral(vVelocity.x, vVelocity.y);

  if (!vRealDirLateral.IsZero())
  {
    vRealDirLateral.Normalize();

    const float fSpeedAlongRealDir = vRealDirLateral.Dot(m_vVelocityLateral);

    m_vVelocityLateral.SetLength(fSpeedAlongRealDir).IgnoreResult();
  }
  else
    m_vVelocityLateral.SetZero();
}

void xiiJoltDefaultCharacterComponent::InteractWithSurfaces(const ContactPoint& contact, const Config& cfg)
{
  if (cfg.m_sGroundInteraction.IsEmpty())
  {
    m_fAccumulatedWalkDistance = 0;
    return;
  }

  const xiiVec2 vIntendedWalkAmount = (cfg.m_vVelocity * GetUpdateTimeDelta()).GetAsVec2() + m_vAbsoluteRootMotion.GetAsVec2();

  const xiiVec3 vOldPos = m_PreviousTransform.m_vPosition;
  const xiiVec3 vNewPos = GetOwner()->GetGlobalPosition();

  m_fAccumulatedWalkDistance += xiiMath::Min(vIntendedWalkAmount.GetLength(), (vNewPos - vOldPos).GetLength());

  if (m_fAccumulatedWalkDistance < cfg.m_fGroundInteractionDistanceThreshold)
    return;

  m_fAccumulatedWalkDistance = 0.0f;

  SpawnContactInteraction(contact, cfg.m_sGroundInteraction, m_hFallbackWalkSurface);
}

void xiiJoltDefaultCharacterComponent::MoveHeadObject()
{
  xiiGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadTargetHeight = m_fHeadHeightOffset;

    if (IsCrouching())
    {
      m_fHeadTargetHeight -= (m_fCylinderHeightStand - m_fCylinderHeightCrouch);
    }

    xiiVec3 pos = pHeadObject->GetLocalPosition();

    const float fTimeDiff = xiiMath::Max(GetUpdateTimeDelta(), 0.005f); // prevent stuff from breaking at high frame rates
    const float fFactor   = 1.0f - xiiMath::Pow(0.001f, fTimeDiff);
    pos.z                 = xiiMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

    pHeadObject->SetLocalPosition(pos);
  }
}

void xiiJoltDefaultCharacterComponent::DebugVisualizations()
{
  if (m_DebugFlags.IsSet(xiiJoltCharacterDebugFlags::PrintState))
  {
    switch (GetJoltCharacter()->GetGroundState())
    {
      case JPH::CharacterBase::EGroundState::OnGround:
        xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", "Jolt: On Ground", xiiColor::Brown);
        break;
      case JPH::CharacterBase::EGroundState::InAir:
        xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", "Jolt: In Air", xiiColor::CornflowerBlue);
        break;
      case JPH::CharacterBase::EGroundState::NotSupported:
        xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", "Jolt: Not Supported", xiiColor::Yellow);
        break;
      case JPH::CharacterBase::EGroundState::OnSteepGround:
        xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", "Jolt: Steep", xiiColor::OrangeRed);
        break;
    }

    const xiiTransform newTransform   = GetOwner()->GetGlobalTransform();
    const float        fDistTraveled  = (m_PreviousTransform.m_vPosition - newTransform.m_vPosition).GetLength();
    const float        fSpeedTraveled = fDistTraveled * GetInverseUpdateTimeDelta();

    const float fDistTraveledLateral  = (m_PreviousTransform.m_vPosition.GetAsVec2() - newTransform.m_vPosition.GetAsVec2()).GetLength();
    const float fSpeedTraveledLateral = fDistTraveled * GetInverseUpdateTimeDelta();

    // xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", xiiFmt("Speed 1: {} m/s", fSpeedTraveled), xiiColor::WhiteSmoke);
    // xiiDebugRenderer::DrawInfoText(GetWorld(), xiiDebugRenderer::ScreenPlacement::TopLeft, "JCC", xiiFmt("Speed 2: {} m/s", fSpeedTraveledLateral), xiiColor::WhiteSmoke);
  }

  if (m_DebugFlags.IsSet(xiiJoltCharacterDebugFlags::VisGroundContact))
  {
    xiiVec3 gpos = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    xiiVec3 gnom = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());

    if (!gnom.IsZero(0.01f))
    {
      xiiQuat rot;
      rot.SetShortestRotation(xiiVec3::UnitXAxis(), gnom);

      xiiDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, xiiColor::ZeroColor(), xiiColor::Aquamarine, xiiTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsSet(xiiJoltCharacterDebugFlags::VisShape))
  {
    xiiTransform shapeTrans = GetOwner()->GetGlobalTransform();

    shapeTrans.m_vPosition.z += GetCurrentCapsuleHeight() * 0.5f;

    xiiDebugRenderer::DrawLineCapsuleZ(GetWorld(), GetCurrentCylinderHeight(), GetShapeRadius(), xiiColor::CornflowerBlue, shapeTrans);
  }
}

void xiiJoltDefaultCharacterComponent::CheckFeet()
{
  if (!cvar_JoltCcFootCheck)
  {
    // pretend we always touch the ground
    m_bFeetOnSolidGround = true;
    return;
  }

  if (m_fFootRadius <= 0 || m_fMaxStepDown <= 0.0f)
    return;

  m_bFeetOnSolidGround = false;

  xiiTransform shapeTrans = GetOwner()->GetGlobalTransform();
  xiiQuat      shapeRot;
  shapeRot.SetShortestRotation(xiiVec3(0, 1, 0), xiiVec3(0, 0, 1));

  const float radius     = m_fFootRadius;
  const float halfHeight = xiiMath::Max(0.0f, m_fMaxStepDown - radius);

  JPH::CapsuleShape shape(halfHeight, radius);

  xiiHybridArray<ContactPoint, 32> contacts;
  CollectContacts(contacts, &shape, shapeTrans.m_vPosition, shapeRot, 0.01f);

  for (const auto& contact : contacts)
  {
    xiiVec3 gpos = contact.m_vPosition;
    xiiVec3 gnom = contact.m_vSurfaceNormal;

    xiiColor color = xiiColor::LightYellow;
    xiiQuat  rot;

    if (gnom.IsZero(0.01f))
    {
      rot.SetShortestRotation(xiiVec3::UnitXAxis(), xiiVec3::UnitZAxis());
      color = xiiColor::OrangeRed;
    }
    else
    {
      rot.SetShortestRotation(xiiVec3::UnitXAxis(), gnom);

      if (gnom.Dot(xiiVec3::UnitZAxis()) > xiiMath::Cos(xiiAngle::Degree(40)))
      {
        m_bFeetOnSolidGround = true;
        color                = xiiColor::GreenYellow;
      }
    }

    if (m_DebugFlags.IsAnySet(xiiJoltCharacterDebugFlags::VisFootCheck))
    {
      xiiDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, xiiColor::ZeroColor(), color, xiiTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsAnySet(xiiJoltCharacterDebugFlags::VisFootCheck))
  {
    xiiDebugRenderer::DrawLineCapsuleZ(GetWorld(), halfHeight * 2.0f, radius, xiiColor::YellowGreen, xiiTransform(shapeTrans.m_vPosition));
  }
}

void xiiJoltDefaultCharacterComponent::DetermineConfig(Config& out_Inputs)
{
  // velocity
  {
    float fSpeed = 0;

    switch (GetGroundState())
    {
      case xiiJoltDefaultCharacterComponent::GroundState::OnGround:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        else if (m_uiInputRunBit)
        {
          fSpeed = m_fWalkSpeedRunning;
        }
        break;

      case xiiJoltDefaultCharacterComponent::GroundState::Sliding:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        break;

      case xiiJoltDefaultCharacterComponent::GroundState::InAir:
        fSpeed = m_fAirSpeed;
        break;
    }

    out_Inputs.m_vVelocity = GetOwner()->GetGlobalRotation() * m_vInputDirection.GetAsVec3(0) * fSpeed;
  }

  // ground interaction
  {
    switch (GetGroundState())
    {
      case xiiJoltDefaultCharacterComponent::GroundState::OnGround:
        out_Inputs.m_sGroundInteraction                  = (m_uiInputRunBit == 1) ? m_sWalkSurfaceInteraction : m_sWalkSurfaceInteraction; // TODO: run interaction
        out_Inputs.m_fGroundInteractionDistanceThreshold = (m_uiInputRunBit == 1) ? m_fRunInteractionDistance : m_fWalkInteractionDistance;
        break;

      case xiiJoltDefaultCharacterComponent::GroundState::Sliding:
        // TODO: slide interaction
        break;

      case GroundState::InAir:
        break;
    }
  }

  out_Inputs.m_bAllowCrouch         = true;
  out_Inputs.m_bAllowJump           = (GetGroundState() == GroundState::OnGround) && !IsCrouching() && m_bFeetOnSolidGround;
  out_Inputs.m_bApplyGroundVelocity = true;
  out_Inputs.m_fPushDownForce       = GetMass();
  out_Inputs.m_fMaxStepUp           = (m_bFeetOnSolidGround && !out_Inputs.m_vVelocity.IsZero()) ? m_fMaxStepUp : 0.0f;
  out_Inputs.m_fMaxStepDown         = ((GetGroundState() == GroundState::OnGround) || (GetGroundState() == GroundState::Sliding)) && m_bFeetOnSolidGround ? m_fMaxStepDown : 0.0f;
}

void xiiJoltDefaultCharacterComponent::UpdateCharacter()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();
  m_PreviousTransform         = GetOwner()->GetGlobalTransform();

  switch (GetJoltCharacter()->GetGroundState())
  {
    case JPH::CharacterBase::EGroundState::InAir:
    case JPH::CharacterBase::EGroundState::NotSupported:
      m_LastGroundState = GroundState::InAir;
      // TODO: filter out 'sliding' when touching a ceiling (should be 'in air')
      break;

    case JPH::CharacterBase::EGroundState::OnGround:
      m_LastGroundState = GroundState::OnGround;
      break;

    case JPH::CharacterBase::EGroundState::OnSteepGround:
      m_LastGroundState = GroundState::Sliding;
      break;
  }

  CheckFeet();

  Config cfg;
  DetermineConfig(cfg);

  ApplyCrouchState();

  if (m_uiInputJumpBit && cfg.m_bAllowJump)
  {
    m_fVelocityUp      = m_fJumpImpulse;
    cfg.m_fMaxStepUp   = 0;
    cfg.m_fMaxStepDown = 0;
  }

  xiiVec3 vGroundVelocity = xiiVec3::ZeroVector();

  ContactPoint groundContact;
  {
    groundContact.m_vPosition      = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    groundContact.m_vContactNormal = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());
    groundContact.m_vSurfaceNormal = groundContact.m_vContactNormal;
    groundContact.m_BodyID         = GetJoltCharacter()->GetGroundBodyID();
    groundContact.m_SubShapeID     = GetJoltCharacter()->GetGroundSubShapeID();

    /*vGroundVelocity =*/GetContactVelocityAndPushAway(groundContact, cfg.m_fPushDownForce);

    // TODO: on rotating surfaces I see the same error with this value and the one returned above
    vGroundVelocity   = xiiJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundVelocity());
    vGroundVelocity.z = 0.0f;

    if (!cfg.m_bApplyGroundVelocity)
      vGroundVelocity.SetZero();
  }

  // AIR: apply 'drag' to the lateral velocity
  // m_vVelocityLateral *= xiiMath::Pow(1.0f - m_fAirFriction, GetUpdateTimeDelta());

  xiiVec3 vVelocityToApply = cfg.m_vVelocity + vGroundVelocity; // TODO +m_vVelocityLateral.GetAsVec3(0);
  vVelocityToApply += GetInverseUpdateTimeDelta() * (GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);
  vVelocityToApply.z = m_fVelocityUp;

  RawMoveWithVelocity(vVelocityToApply, cfg.m_fMaxStepUp, cfg.m_fMaxStepDown);

  if (!cfg.m_sGroundInteraction.IsEmpty())
  {
    if (groundContact.m_vContactNormal.IsValid() && !groundContact.m_vContactNormal.IsZero(0.001f))
    {
      // TODO: sometimes the CC reports contacts with zero normals
      InteractWithSurfaces(groundContact, cfg);
    }
  }

  StoreLateralVelocity();
  // TODO: store or apply+clamp ClampLateralVelocity();

  // retrieve the actual up velocity
  float groundVerticalVelocity = GetJoltCharacter()->GetGroundVelocity().GetZ();
  if (GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
      && (m_fVelocityUp - groundVerticalVelocity) < 0.1f)                                   // And not moving away from ground
  {
    m_fVelocityUp = groundVerticalVelocity;
  }

  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  ApplyRotationZ();

  MoveHeadObject();

  DebugVisualizations();

  ResetInputState();
}
