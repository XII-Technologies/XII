#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSimpleWindComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("MinWindStrength", xiiWindStrength, m_MinWindStrength),
    XII_ENUM_MEMBER_PROPERTY("MaxWindStrength", xiiWindStrength, m_MaxWindStrength),
    XII_MEMBER_PROPERTY("MaxDeviation", m_Deviation)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0), xiiAngle::Degree(180))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects/Wind"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColor::DodgerBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSimpleWindComponent::xiiSimpleWindComponent()  = default;
xiiSimpleWindComponent::~xiiSimpleWindComponent() = default;

void xiiSimpleWindComponent::Update()
{
  xiiSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<xiiSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  const xiiTime tCur  = GetWorld()->GetClock().GetAccumulatedTime();
  const float   fLerp = static_cast<float>((tCur - m_LastChange).GetSeconds() / (m_NextChange - m_LastChange).GetSeconds());

  xiiVec3 vCurWind;

  if (fLerp >= 1.0f)
  {
    ComputeNextState();

    vCurWind = m_vLastDirection * m_fLastStrength;
  }
  else
  {
    const float   fCurStrength = xiiMath::Lerp(m_fLastStrength, m_fNextStrength, fLerp);
    const xiiVec3 vCurDir      = xiiMath::Lerp(m_vLastDirection, m_vNextDirection, fLerp);

    vCurWind = vCurDir * fCurStrength;
  }

  pWindModule->SetFallbackWind(vCurWind);
}

void xiiSimpleWindComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_MinWindStrength;
  s << m_MaxWindStrength;
  s << m_Deviation;
}

void xiiSimpleWindComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  if (uiVersion == 1)
  {
    float m_fWindStrengthMin, m_fWindStrengthMax;
    s >> m_fWindStrengthMin;
    s >> m_fWindStrengthMax;
  }
  else
  {
    s >> m_MinWindStrength;
    s >> m_MaxWindStrength;
  }

  s >> m_Deviation;
}

void xiiSimpleWindComponent::OnActivated()
{
  SUPER::OnActivated();

  m_fNextStrength  = xiiWindStrength::GetInMetersPerSecond(m_MinWindStrength);
  m_vNextDirection = GetOwner()->GetGlobalDirForwards();
  m_NextChange     = GetWorld()->GetClock().GetAccumulatedTime();
  m_LastChange     = m_NextChange - xiiTime::Seconds(1);

  ComputeNextState();
}

void xiiSimpleWindComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  xiiSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<xiiSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  pWindModule->SetFallbackWind(xiiVec3::ZeroVector());
}

void xiiSimpleWindComponent::ComputeNextState()
{
  m_fLastStrength  = m_fNextStrength;
  m_vLastDirection = m_vNextDirection;
  m_LastChange     = GetWorld()->GetClock().GetAccumulatedTime();

  auto& rng = GetWorld()->GetRandomNumberGenerator();

  const xiiEnum<xiiWindStrength> minWind = xiiMath::Min(m_MinWindStrength, m_MaxWindStrength);
  const xiiEnum<xiiWindStrength> maxWind = xiiMath::Max(m_MinWindStrength, m_MaxWindStrength);

  const float fMinStrength = xiiWindStrength::GetInMetersPerSecond(minWind);
  const float fMaxStrength = xiiWindStrength::GetInMetersPerSecond(maxWind);

  float fStrengthDiff   = fMaxStrength - fMinStrength;
  float fStrengthChange = fStrengthDiff * 0.2f;

  m_NextChange    = m_LastChange + xiiTime::Seconds(rng.DoubleMinMax(2.0f, 5.0f));
  m_fNextStrength = xiiMath::Clamp<float>(m_fLastStrength + (float)rng.DoubleMinMax(-fStrengthChange, +fStrengthChange), fMinStrength, fMaxStrength);

  const xiiVec3 vMainDir = GetOwner()->GetGlobalDirForwards();

  if (m_Deviation < xiiAngle::Degree(1))
    m_vNextDirection = vMainDir;
  else
    m_vNextDirection = xiiVec3::CreateRandomDeviation(rng, m_Deviation, vMainDir);

  xiiCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);
  const float fRemoveUp = m_vNextDirection.Dot(cs.m_vUpDir);

  m_vNextDirection -= cs.m_vUpDir * fRemoveUp;
  m_vNextDirection.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();
}

void xiiSimpleWindComponent::Initialize()
{
  SUPER::Initialize();

  // make sure to query the wind interface before any simulation starts
  /*xiiWindWorldModuleInterface* pWindInterface =*/GetWorld()->GetOrCreateModule<xiiSimpleWindWorldModule>();
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindComponent);
