/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeComponent.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCameraShakeComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinShake", m_MinShake),
    XII_MEMBER_PROPERTY("MaxShake", m_MaxShake)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(5))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects/CameraShake"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCameraShakeComponent::xiiCameraShakeComponent()  = default;
xiiCameraShakeComponent::~xiiCameraShakeComponent() = default;

void xiiCameraShakeComponent::Update()
{
  const xiiTime tDuration = xiiTime::MakeFromSeconds(1.0 / 30.0); // 30 Hz vibration seems to work well

  const xiiTime tNow = xiiTime::Now();

  if (tNow >= m_ReferenceTime + tDuration)
  {
    GetOwner()->SetLocalRotation(m_qNextTarget);
    GenerateKeyframe();
  }
  else
  {
    const float fLerp = xiiMath::Clamp((tNow - m_ReferenceTime).AsFloatInSeconds() / tDuration.AsFloatInSeconds(), 0.0f, 1.0f);

    xiiQuat q = xiiQuat::MakeSlerp(m_qPrevTarget, m_qNextTarget, fLerp);

    GetOwner()->SetLocalRotation(q);
  }
}

void xiiCameraShakeComponent::GenerateKeyframe()
{
  m_qPrevTarget = m_qNextTarget;

  m_ReferenceTime = xiiTime::Now();

  xiiWorld* pWorld = GetWorld();

  // fade out shaking over a second, if the vibration stopped
  m_fLastStrength -= pWorld->GetClock().GetTimeDiff().AsFloatInSeconds();

  const float fShake = xiiMath::Clamp(GetStrengthAtPosition(), 0.0f, 1.0f);

  m_fLastStrength = xiiMath::Max(m_fLastStrength, fShake);

  xiiAngle deviation;
  deviation = xiiMath::Lerp(m_MinShake, m_MaxShake, m_fLastStrength);

  if (deviation > xiiAngle())
  {
    m_Rotation += xiiAngle::MakeFromRadian(pWorld->GetRandomNumberGenerator().FloatMinMax(xiiAngle::MakeFromDegree(120).GetRadian(), xiiAngle::MakeFromDegree(240).GetRadian()));
    m_Rotation.NormalizeRange();

    xiiQuat qRot;
    qRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), m_Rotation);

    const xiiVec3 tiltAxis = qRot * xiiVec3::MakeAxisZ();

    m_qNextTarget = xiiQuat::MakeFromAxisAndAngle(tiltAxis, deviation);
  }
  else
  {
    m_qNextTarget.SetIdentity();
  }
}

float xiiCameraShakeComponent::GetStrengthAtPosition() const
{
  float force = 0;

  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    const xiiVec3 vPosition = GetOwner()->GetGlobalPosition();

    xiiHybridArray<xiiGameObject*, 16> volumes;

    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = xiiCameraShakeVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(xiiBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const xiiSimdVec4f pos = xiiSimdConversion::ToVec3(vPosition);

    for (xiiGameObject* pObj : volumes)
    {
      xiiCameraShakeVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force = xiiMath::Max(force, pVol->ComputeForceAtGlobalPosition(pos));
      }
    }
  }

  return force;
}

void xiiCameraShakeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_MinShake;
  s << m_MaxShake;
}

void xiiCameraShakeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinShake;
  s >> m_MaxShake;
}
