#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Animation/FollowPathComponent.h>
#include <GameEngine/Animation/PathComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiFollowPathComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Path", DummyGetter, SetPathObject)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("StartDistance", GetDistanceAlongPath, SetDistanceAlongPath)->AddAttributes(new xiiClampValueAttribute(0.0f, {})),
    XII_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new xiiDefaultValueAttribute(true)), // Whether the animation should start right away.
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiPropertyAnimMode, m_Mode),
    XII_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("LookAhead", m_fLookAhead)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    XII_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation/Paths"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiFollowPathComponent::xiiFollowPathComponent()  = default;
xiiFollowPathComponent::~xiiFollowPathComponent() = default;

void xiiFollowPathComponent::Update(bool bForce)
{
  if (!bForce && (!m_bIsRunning || m_fSpeed == 0.0f))
    return;

  if (m_hPathObject.IsInvalidated())
    return;

  xiiWorld* pWorld = GetWorld();

  xiiGameObject* pPathObject = nullptr;
  if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
    return;

  xiiPathComponent* pPathComponent;
  if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
    return;

  pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

  auto& clock = pWorld->GetClock();

  float fToAdvance = m_fSpeed * clock.GetTimeDiff().AsFloatInSeconds();

  if (!m_bIsRunningForwards)
  {
    fToAdvance = -fToAdvance;
  }

  {
    if (!pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance) && fToAdvance != 0.0f)
    {
      xiiMsgAnimationReachedEnd msg;
      m_ReachedEndEvent.SendEventMessage(msg, this, GetOwner());

      if (m_Mode == xiiPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(m_PathSampler, fToAdvance);
      }
      else if (m_Mode == xiiPropertyAnimMode::BackAndForth)
      {
        m_bIsRunningForwards = !m_bIsRunningForwards;
        fToAdvance           = -fToAdvance;
        pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance);
      }
      else
      {
        m_bIsRunning = false;
      }
    }
  }

  xiiPathComponent::LinearSampler samplerAhead;

  float fLookAhead = xiiMath::Max(m_fLookAhead, 0.02f);

  {
    samplerAhead = m_PathSampler;
    if (!pPathComponent->AdvanceLinearSamplerBy(samplerAhead, fLookAhead) && fLookAhead != 0.0f)
    {
      if (m_Mode == xiiPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(samplerAhead, fLookAhead);
      }
    }
  }

  auto transform      = pPathComponent->SampleLinearizedRepresentation(m_PathSampler);
  auto transformAhead = pPathComponent->SampleLinearizedRepresentation(samplerAhead);

  if (m_bLastStateValid)
  {
    const float fSmoothing = xiiMath::Clamp(m_fSmoothing, 0.0f, 0.99f);

    transform.m_vPosition      = xiiMath::Lerp(transform.m_vPosition, m_vLastPosition, fSmoothing);
    transform.m_vUpDirection   = xiiMath::Lerp(transform.m_vUpDirection, m_vLastUpDir, fSmoothing);
    transformAhead.m_vPosition = xiiMath::Lerp(transformAhead.m_vPosition, m_vLastTargetPosition, fSmoothing);
  }

  xiiVec3 vTarget = transformAhead.m_vPosition - transform.m_vPosition;
  vTarget.NormalizeIfNotZero(xiiVec3::UnitXAxis()).IgnoreResult();

  xiiVec3 vUp    = transform.m_vUpDirection;
  xiiVec3 vRight = vTarget.CrossRH(vUp);
  vRight.NormalizeIfNotZero(xiiVec3::UnitYAxis()).IgnoreResult();
  vUp = vRight.CrossRH(vTarget);
  vUp.NormalizeIfNotZero(xiiVec3::UnitZAxis()).IgnoreResult();

  {
    m_bLastStateValid     = true;
    m_vLastPosition       = transform.m_vPosition;
    m_vLastUpDir          = transform.m_vUpDirection;
    m_vLastTargetPosition = transformAhead.m_vPosition;
  }

  xiiMat3 mRot;
  mRot.SetColumn(0, vTarget);
  mRot.SetColumn(1, -vRight);
  mRot.SetColumn(2, vUp);

  xiiTransform tFinal;
  tFinal.m_vPosition = transform.m_vPosition;
  tFinal.m_vScale.Set(1);
  tFinal.m_qRotation.SetFromMat3(mRot);

  GetOwner()->SetGlobalTransform(pPathObject->GetGlobalTransform() * tFinal);
}

void xiiFollowPathComponent::SetPathObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPathObject = resolver(szReference, GetHandle(), "Path");
}

void xiiFollowPathComponent::SetDistanceAlongPath(float fDistance)
{
  m_bLastStateValid = false;
  m_fStartDistance  = fDistance;

  if (IsActiveAndInitialized())
  {
    if (m_hPathObject.IsInvalidated())
      return;

    xiiWorld* pWorld = GetWorld();

    xiiGameObject* pPathObject = nullptr;
    if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
      return;

    xiiPathComponent* pPathComponent = nullptr;
    if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
      return;

    pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

    pPathComponent->SetLinearSamplerTo(m_PathSampler, m_fStartDistance);

    xiiVec3 m_vLastPosition;
    xiiVec3 m_vLastTargetPosition;
    xiiVec3 m_vLastUpDir;

    Update(true);
  }
}

float xiiFollowPathComponent::GetDistanceAlongPath() const
{
  return m_fStartDistance;
}

void xiiFollowPathComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  stream.WriteGameObjectHandle(m_hPathObject);

  s << m_fStartDistance;
  s << m_fSpeed;
  s << m_fLookAhead;
  s << m_Mode;
  s << m_fSmoothing;
  s << m_bIsRunning;
  s << m_bIsRunningForwards;
}

void xiiFollowPathComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  m_hPathObject = stream.ReadGameObjectHandle();

  s >> m_fStartDistance;
  s >> m_fSpeed;
  s >> m_fLookAhead;
  s >> m_Mode;
  s >> m_fSmoothing;
  s >> m_bIsRunning;
  s >> m_bIsRunningForwards;
}

void xiiFollowPathComponent::OnActivated()
{
  SUPER::OnActivated();

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

void xiiFollowPathComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

bool xiiFollowPathComponent::IsRunning(void) const
{
  return m_bIsRunning;
}

void xiiFollowPathComponent::SetRunning(bool b)
{
  m_bIsRunning = b;
}

void xiiFollowPathComponent::SetDirectionForwards(bool bForwards)
{
  m_bIsRunningForwards = bForwards;
}

void xiiFollowPathComponent::ToggleDirection()
{
  m_bIsRunningForwards = !m_bIsRunningForwards;
}

bool xiiFollowPathComponent::IsDirectionForwards() const
{
  return m_bIsRunningForwards;
}
