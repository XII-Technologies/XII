#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/MoveToComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMoveToComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning),
    XII_MEMBER_PROPERTY("TranslationSpeed", m_fMaxTranslationSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("TranslationAcceleration", m_fTranslationAcceleration),
    XII_MEMBER_PROPERTY("TranslationDeceleration", m_fTranslationDeceleration),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiMoveToComponent::xiiMoveToComponent()  = default;
xiiMoveToComponent::~xiiMoveToComponent() = default;

void xiiMoveToComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_Flags.GetValue();
  s << m_fCurTranslationSpeed;
  s << m_fMaxTranslationSpeed;
  s << m_fTranslationAcceleration;
  s << m_fTranslationDeceleration;
  s << m_vTargetPosition;
}


void xiiMoveToComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  auto& s = inout_stream.GetStream();

  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_Flags;
  s >> m_fCurTranslationSpeed;
  s >> m_fMaxTranslationSpeed;
  s >> m_fTranslationAcceleration;
  s >> m_fTranslationDeceleration;
  s >> m_vTargetPosition;
}

void xiiMoveToComponent::SetRunning(bool bRunning)
{
  m_Flags.AddOrRemove(xiiMoveToComponentFlags::Running, bRunning);
}

bool xiiMoveToComponent::IsRunning() const
{
  return m_Flags.IsSet(xiiMoveToComponentFlags::Running);
}

void xiiMoveToComponent::SetTargetPosition(const xiiVec3& vPos)
{
  m_vTargetPosition = vPos;
}

static float CalculateNewSpeed(float fRemainingDistance, float fCurSpeed, float fMaxSpeed, float fAcceleration, float fDeceleration, float fTimeStep)
{
  float fMaxAllowedSpeed = fMaxSpeed;

  if (fDeceleration > 0)
  {
    const float fMaxDecelerationTime     = (fMaxSpeed / fDeceleration);
    const float fMaxDecelerationDistance = fMaxDecelerationTime * fMaxSpeed;

    if (fRemainingDistance <= fMaxDecelerationDistance)
    {
      fMaxAllowedSpeed = fMaxSpeed * (fRemainingDistance / fMaxDecelerationDistance);
    }
  }

  float fMaxNewSpeed = fMaxSpeed;

  if (fAcceleration > 0)
  {
    fMaxNewSpeed = fCurSpeed + fTimeStep * fAcceleration;
  }

  return xiiMath::Clamp(fMaxNewSpeed, 0.0f, fMaxAllowedSpeed);
}

void xiiMoveToComponent::Update()
{
  if (!m_Flags.IsAnySet(xiiMoveToComponentFlags::Running))
    return;

  xiiGameObject* pOwner = GetOwner();

  const xiiVec3 vCurPos = pOwner->GetGlobalPosition();

  xiiVec3     vDiff            = m_vTargetPosition - vCurPos;
  const float fRemainingLength = vDiff.GetLength();

  if (xiiMath::IsZero(fRemainingLength, 0.002f))
  {
    SetRunning(false);
    pOwner->SetGlobalPosition(m_vTargetPosition);

    xiiMsgAnimationReachedEnd msg;
    m_ReachedEndMsgSender.SendEventMessage(msg, this, GetOwner());

    return;
  }

  const xiiVec3 vDir = vDiff / fRemainingLength;

  m_fCurTranslationSpeed = CalculateNewSpeed(fRemainingLength, m_fCurTranslationSpeed, m_fMaxTranslationSpeed, m_fTranslationAcceleration,
                                             m_fTranslationDeceleration, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  const float fTravelDist = xiiMath::Min<float>(fRemainingLength, m_fCurTranslationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  pOwner->SetGlobalPosition(vCurPos + vDir * fTravelDist);
}
