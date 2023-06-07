#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ColorAnimationComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiColorAnimationComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    XII_MEMBER_PROPERTY("Duration", m_Duration),
    XII_ENUM_MEMBER_PROPERTY("SetColorMode", xiiSetColorMode, m_SetColorMode),
    XII_ENUM_MEMBER_PROPERTY("AnimationMode", xiiPropertyAnimMode, m_AnimationMode),
    XII_ACCESSOR_PROPERTY("RandomStartOffset", GetRandomStartOffset, SetRandomStartOffset)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("ApplyToChildren", GetApplyRecursive, SetApplyRecursive),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiColorAnimationComponent::xiiColorAnimationComponent() = default;

void xiiColorAnimationComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_hGradient;
  s << m_Duration;

  // version 2
  s << m_SetColorMode;
  s << m_AnimationMode;
  s << GetRandomStartOffset();
  s << GetApplyRecursive();
}

void xiiColorAnimationComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_hGradient;
  s >> m_Duration;

  if (uiVersion >= 2)
  {
    s >> m_SetColorMode;
    s >> m_AnimationMode;
    bool b;
    s >> b;
    SetRandomStartOffset(b);
    s >> b;
    SetApplyRecursive(b);
  }
}

void xiiColorAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetRandomStartOffset())
  {
    m_CurAnimTime = xiiTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_Duration.GetSeconds()));
  }
}

void xiiColorAnimationComponent::SetColorGradientFile(const char* szFile)
{
  xiiColorGradientResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiColorGradientResource>(szFile);
  }

  SetColorGradient(hResource);
}

const char* xiiColorAnimationComponent::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void xiiColorAnimationComponent::SetColorGradient(const xiiColorGradientResourceHandle& hResource)
{
  m_hGradient = hResource;
}

bool xiiColorAnimationComponent::GetApplyRecursive() const
{
  return GetUserFlag(0);
}

void xiiColorAnimationComponent::SetApplyRecursive(bool value)
{
  SetUserFlag(0, value);
}

bool xiiColorAnimationComponent::GetRandomStartOffset() const
{
  return GetUserFlag(1);
}

void xiiColorAnimationComponent::SetRandomStartOffset(bool value)
{
  SetUserFlag(1, value);
}

void xiiColorAnimationComponent::Update()
{
  if (!m_hGradient.IsValid() || m_Duration <= xiiTime::Zero())
    return;

  xiiTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  const bool bReverse = GetUserFlag(0);

  if (bReverse)
    m_CurAnimTime -= tDiff;
  else
    m_CurAnimTime += tDiff;

  switch (m_AnimationMode)
  {
    case xiiPropertyAnimMode::Once:
    {
      m_CurAnimTime = xiiMath::Min(m_CurAnimTime, m_Duration);
      break;
    }

    case xiiPropertyAnimMode::Loop:
    {
      if (m_CurAnimTime >= m_Duration)
        m_CurAnimTime -= m_Duration;

      break;
    }

    case xiiPropertyAnimMode::BackAndForth:
    {
      if (m_CurAnimTime > m_Duration)
      {
        SetUserFlag(0, !bReverse);

        const xiiTime tOver = m_Duration - m_CurAnimTime;

        m_CurAnimTime = m_Duration - tOver;
      }
      else if (m_CurAnimTime < xiiTime::Zero())
      {
        SetUserFlag(0, !bReverse);

        m_CurAnimTime = -m_CurAnimTime;
      }

      break;
    }
  }

  xiiResourceLock<xiiColorGradientResource> pGradient(m_hGradient, xiiResourceAcquireMode::AllowLoadingFallback);

  if (pGradient.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiMsgSetColor msg;
  msg.m_Color = pGradient->Evaluate(m_CurAnimTime.GetSeconds() / m_Duration.GetSeconds());
  msg.m_Mode  = m_SetColorMode;

  if (GetApplyRecursive())
    GetOwner()->SendMessageRecursive(msg);
  else
    GetOwner()->SendMessage(msg);
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ColorAnimationComponent);
