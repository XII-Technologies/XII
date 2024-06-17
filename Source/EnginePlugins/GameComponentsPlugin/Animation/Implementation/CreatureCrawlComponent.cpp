#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/CreatureCrawlComponent.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiCreatureLeg, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiCreatureLeg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LegName", m_sLegObject),
    XII_MEMBER_PROPERTY("StepGroup", m_uiStepGroup),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiCreatureCrawlComponent, 1, xiiComponentMode::Dynamic);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Body", DummyGetter, SetBodyReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_MEMBER_PROPERTY("CastUp", m_fCastUp)->AddAttributes(new xiiDefaultValueAttribute(0.3f)),
    XII_MEMBER_PROPERTY("CastDown", m_fCastDown)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("StepDistance", m_fStepDistance)->AddAttributes(new xiiDefaultValueAttribute(0.4f)),
    XII_MEMBER_PROPERTY("MinLegDistance", m_fMinLegDistance)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.1f, 1.0f)),
    XII_ARRAY_MEMBER_PROPERTY("Legs", m_Legs),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiCreatureCrawlComponent::xiiCreatureCrawlComponent()  = default;
xiiCreatureCrawlComponent::~xiiCreatureCrawlComponent() = default;

void xiiCreatureCrawlComponent::SetBodyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hBody = resolver(szReference, GetHandle(), "Body");
}

void xiiCreatureCrawlComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fCastUp;
  s << m_fCastDown;
  s << m_fStepDistance;
  s << m_fMinLegDistance;

  inout_stream.WriteGameObjectHandle(m_hBody);
  s << m_Legs.GetCount();
  for (xiiUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s << m_Legs[i].m_sLegObject;
    s << m_Legs[i].m_uiStepGroup;
  }
}

void xiiCreatureCrawlComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fCastUp;
  s >> m_fCastDown;
  s >> m_fStepDistance;
  s >> m_fMinLegDistance;

  m_hBody           = inout_stream.ReadGameObjectHandle();
  xiiUInt32 numLegs = 0;
  s >> numLegs;
  m_Legs.SetCount(numLegs);
  for (xiiUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s >> m_Legs[i].m_sLegObject;
    s >> m_Legs[i].m_uiStepGroup;
  }
}

void xiiCreatureCrawlComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  const xiiTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

  for (auto& leg : m_Legs)
  {
    leg.m_vCurTargetPosAbs.SetZero();
    leg.m_fMoveLegFactor = 0.99f;
    leg.m_vRestPositionRelative.SetZero();
    leg.m_hLegObject.Invalidate();

    if (xiiGameObject* pLeg = GetOwner()->FindChildByName(leg.m_sLegObject))
    {
      leg.m_hLegObject            = pLeg->GetHandle();
      leg.m_vRestPositionRelative = invTrans * pLeg->GetGlobalPosition();
    }
  }
}

void xiiCreatureCrawlComponent::Update()
{
  if (m_pPhysicsInterface == nullptr)
  {
    m_pPhysicsInterface = GetWorld()->GetModule<xiiPhysicsWorldModuleInterface>();
    return;
  }

  const xiiUInt32 uiNumLegs = m_Legs.GetCount();

  const xiiVec3 vCenterPos = GetOwner()->GetGlobalPosition();
  const xiiQuat qCenterRot = GetOwner()->GetGlobalRotation();

  xiiHybridArray<xiiVec3, 8> vNewTargetPos;
  vNewTargetPos.SetCount(uiNumLegs);

  xiiWorld* pWorld = GetWorld();

  // TODO: make step duration configurable
  // TODO: make step height configurable
  const xiiTime tStepDuration = xiiTime::MakeFromMilliseconds(150);
  const float   fStepHeight   = 0.3f;
  const float   fMoveAdd      = xiiMath::Min<float>(1.0f, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds() / tStepDuration.GetSeconds());

  xiiHybridArray<bool, 8> bLegMoving;
  bLegMoving.SetCount(uiNumLegs);
  bool bAnyLegMoving = false;

  for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
  {
    vNewTargetPos[i] = qCenterRot * (m_Legs[i].m_vRestPositionRelative + xiiVec3(0, 0, m_fCastUp));
    bLegMoving[i]    = m_Legs[i].m_fMoveLegFactor < 1.0f;
    bAnyLegMoving    = bAnyLegMoving || bLegMoving[i];
  }

  bool bComputeBodyTilt = true;

  for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (m_Legs[i].m_hLegObject.IsInvalidated())
      continue;

    // find a place where the leg can stand
    // if none found at the rest position, try pulling the leg closer to the body in two steps (unless forbidden, see m_fMinLegDistance)

    xiiPhysicsCastResult res;
    if (m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i], xiiVec3(0, 0, -1.0f), m_fCastDown, xiiPhysicsQueryParameters(0, xiiPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * xiiMath::Lerp(m_fMinLegDistance, 1.0f, 0.5f), xiiVec3(0, 0, -1.0f), m_fCastDown, xiiPhysicsQueryParameters(0, xiiPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * m_fMinLegDistance, xiiVec3(0, 0, -1.0f), m_fCastDown, xiiPhysicsQueryParameters(0, xiiPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else
    {
      bComputeBodyTilt = false;

      vNewTargetPos[i] += vCenterPos;
      m_Legs[i].m_vCurTargetPosAbs = xiiMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], fMoveAdd);
    }
  }

  xiiGameObject* pBody;
  if (!m_hBody.IsInvalidated() && pWorld->TryGetObject(m_hBody, pBody) && m_Legs.GetCount() == 4 /* TODO: remove safety check*/)
  {
    const xiiQuat qOwnerRot = GetOwner()->GetGlobalRotation();

    xiiQuat qNewBodyTilt;

    if (bComputeBodyTilt)
    {
      // TODO: compute body tilt from N points:
      // const xiiTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

      // xiiVec3 vAvgDir(0);

      // for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
      //{
      //   xiiVec3 vDir = invTrans * vNewTargetPos[i];
      //   vAvgDir += vDir;
      // }

      xiiVec3 vAvgLegLeft  = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[2]) * 0.5f);
      xiiVec3 vAvgLegRight = qOwnerRot.GetInverse() * ((vNewTargetPos[1] + vNewTargetPos[3]) * 0.5f);
      vAvgLegLeft.x        = 0.0f;
      vAvgLegRight.x       = 0.0f;

      xiiVec3 vAvgLegFwd  = qOwnerRot.GetInverse() * ((vNewTargetPos[2] + vNewTargetPos[3]) * 0.5f);
      xiiVec3 vAvgLegBack = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[1]) * 0.5f);
      vAvgLegFwd.y        = 0.0f;
      vAvgLegBack.y       = 0.0f;

      const xiiVec3 vSideTilt = (vAvgLegRight - vAvgLegLeft).GetNormalized();
      const xiiQuat qSideTilt = xiiQuat::MakeShortestRotation(xiiVec3(0, 1, 0), vSideTilt);

      const xiiVec3 vFwdTilt = (vAvgLegFwd - vAvgLegBack).GetNormalized();
      const xiiQuat qFwdTilt = xiiQuat::MakeShortestRotation(xiiVec3(1, 0, 0), vFwdTilt);

      qNewBodyTilt = qSideTilt * qFwdTilt;
    }
    else
    {
      qNewBodyTilt = xiiQuat::MakeIdentity();
    }

    m_qBodyTilt = xiiQuat::MakeSlerp(m_qBodyTilt, qNewBodyTilt, fMoveAdd);
    pBody->SetGlobalRotation(qOwnerRot * m_qBodyTilt);

    pBody->UpdateGlobalTransform();
  }

  if (!bAnyLegMoving)
  {
    float     fMaxDistSqr = 0;
    xiiUInt32 uiStepGroup = xiiInvalidIndex;

    // check whether to move a leg
    for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
    {
      const float ds = (m_Legs[i].m_vCurTargetPosAbs - vNewTargetPos[i]).GetLengthSquared();

      if (ds > fMaxDistSqr)
      {
        fMaxDistSqr = ds;
        uiStepGroup = m_Legs[i].m_uiStepGroup;
      }
    }

    const xiiTime tNow = pWorld->GetClock().GetAccumulatedTime();

    // TODO: make step delay configurable ?

    if (fMaxDistSqr >= xiiMath::Square(m_fStepDistance) ||
        (fMaxDistSqr >= xiiMath::Square(m_fStepDistance * 0.25) && (tNow - m_LastMove > xiiTime::MakeFromSeconds(0.6f))))
    {
      m_LastMove = tNow;

      for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
      {
        if (m_Legs[i].m_uiStepGroup == uiStepGroup)
        {
          // move all legs from the same group simultaneously
          m_Legs[i].m_fMoveLegFactor = 0.0f;
        }
      }
    }
  }

  for (xiiUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (bLegMoving[i])
    {
      // TODO: nicer step curve (sine instead of linear)

      m_Legs[i].m_fMoveLegFactor += fMoveAdd;

      if (m_Legs[i].m_fMoveLegFactor >= 1.0f)
      {
        m_Legs[i].m_fMoveLegFactor   = 1.0f;
        m_Legs[i].m_vCurTargetPosAbs = vNewTargetPos[i];
      }
      else
      {
        float fSrcHeight = m_Legs[i].m_vCurTargetPosAbs.z;
        float fDstHeight = vNewTargetPos[i].z;
        float fMidHeight = xiiMath::Max(fSrcHeight, fDstHeight) + fStepHeight;

        if (m_Legs[i].m_fMoveLegFactor < 0.5f)
        {
          vNewTargetPos[i].z = xiiMath::Lerp(m_Legs[i].m_vCurTargetPosAbs.z, fMidHeight, m_Legs[i].m_fMoveLegFactor * 2.0f);
        }
        else
        {
          vNewTargetPos[i].z = xiiMath::Lerp(fMidHeight, vNewTargetPos[i].z, (m_Legs[i].m_fMoveLegFactor - 0.5f) * 2.0f);
        }
      }

      vNewTargetPos[i] = xiiMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], m_Legs[i].m_fMoveLegFactor);
    }
    else
    {
      vNewTargetPos[i] = m_Legs[i].m_vCurTargetPosAbs;
    }

    xiiGameObject* pLegTarget;
    if (pWorld->TryGetObject(m_Legs[i].m_hLegObject, pLegTarget))
    {
      pLegTarget->SetGlobalPosition(vNewTargetPos[i]);
    }
  }
}
