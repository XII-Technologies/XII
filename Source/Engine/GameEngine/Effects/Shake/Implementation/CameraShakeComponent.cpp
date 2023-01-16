#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Shake/CameraShakeComponent.h>
#include <GameEngine/Effects/Shake/CameraShakeVolumeComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCameraShakeComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinShake", m_MinShake),
    XII_MEMBER_PROPERTY("MaxShake", m_MaxShake)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(5))),
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
  const xiiTime tDuration = xiiTime::Seconds(1.0 / 30.0); // 30 Hz vibration seems to work well

  const xiiTime tNow = xiiTime::Now();

  if (tNow >= m_ReferenceTime + tDuration)
  {
    GetOwner()->SetLocalRotation(m_qNextTarget);
    GenerateKeyframe();
  }
  else
  {
    const float fLerp = xiiMath::Clamp((tNow - m_ReferenceTime).AsFloatInSeconds() / tDuration.AsFloatInSeconds(), 0.0f, 1.0f);

    xiiQuat q;
    q.SetSlerp(m_qPrevTarget, m_qNextTarget, fLerp);

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
    m_Rotation += xiiAngle::Radian(pWorld->GetRandomNumberGenerator().DoubleMinMax(xiiAngle::Degree(120).GetRadian(), xiiAngle::Degree(240).GetRadian()));
    m_Rotation.NormalizeRange();

    xiiQuat qRot;
    qRot.SetFromAxisAndAngle(xiiVec3::UnitXAxis(), m_Rotation);

    const xiiVec3 tiltAxis = qRot * xiiVec3::UnitZAxis();

    m_qNextTarget.SetFromAxisAndAngle(tiltAxis, deviation);
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

    pSpatial->FindObjectsInSphere(xiiBoundingSphere(vPosition, 0.5f), queryParams, volumes);

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

void xiiCameraShakeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_MinShake;
  s << m_MaxShake;
}

void xiiCameraShakeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_MinShake;
  s >> m_MaxShake;
}
