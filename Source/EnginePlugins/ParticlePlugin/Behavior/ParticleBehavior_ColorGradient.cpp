#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_ColorGradient, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_ColorGradient>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    XII_MEMBER_PROPERTY("TintColor", m_TintColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ENUM_MEMBER_PROPERTY("ColorGradientMode", xiiParticleColorGradientMode, m_GradientMode),
    XII_MEMBER_PROPERTY("GradientMaxSpeed", m_fMaxSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 100.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_ColorGradient, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_ColorGradient>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleBehaviorFactory_ColorGradient::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_ColorGradient>();
}

void xiiParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_ColorGradient* pBehavior = static_cast<xiiParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_hGradient    = m_hGradient;
  pBehavior->m_GradientMode = m_GradientMode;
  pBehavior->m_fMaxSpeed    = m_fMaxSpeed;
  pBehavior->m_TintColor    = m_TintColor;

  // the gradient resource may not be specified yet, so defer evaluation until an element is created
  pBehavior->m_InitColor = xiiColor::RebeccaPurple;
}

void xiiParticleBehaviorFactory_ColorGradient::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 4;
  stream << uiVersion;

  stream << m_hGradient;

  // version 3
  stream << m_GradientMode;
  stream << m_fMaxSpeed;

  // Version 4
  stream << m_TintColor;
}

void xiiParticleBehaviorFactory_ColorGradient::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;

  if (uiVersion >= 3)
  {
    stream >> m_GradientMode;
    stream >> m_fMaxSpeed;
  }

  if (uiVersion >= 4)
  {
    stream >> m_TintColor;
  }
}

void xiiParticleBehaviorFactory_ColorGradient::SetColorGradientFile(const char* szFile)
{
  xiiColorGradientResourceHandle hGradient;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hGradient = xiiResourceManager::LoadResource<xiiColorGradientResource>(szFile);
  }

  SetColorGradient(hGradient);
}

const char* xiiParticleBehaviorFactory_ColorGradient::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void xiiParticleBehavior_ColorGradient::CreateRequiredStreams()
{
  m_pStreamColor    = nullptr;
  m_pStreamVelocity = nullptr;

  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_GradientMode == xiiParticleColorGradientMode::Age)
  {
    CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
  else if (m_GradientMode == xiiParticleColorGradientMode::Speed)
  {
    CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}

void xiiParticleBehavior_ColorGradient::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  if (!m_hGradient.IsValid())
    return;

  XII_PROFILE_SCOPE("PFX: Color Gradient Init");

  // query the init color from the gradient
  if (m_InitColor == xiiColor::RebeccaPurple)
  {
    m_InitColor = m_TintColor;

    xiiResourceLock<xiiColorGradientResource> pGradient(m_hGradient, xiiResourceAcquireMode::BlockTillLoaded);

    if (pGradient.GetAcquireResult() != xiiResourceAcquireResult::MissingFallback)
    {
      const xiiColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

      xiiColor rgba;
      xiiUInt8 alpha;
      gradient.EvaluateColor(0, rgba);
      gradient.EvaluateAlpha(0, alpha);
      rgba.a = xiiMath::ColorByteToFloat(alpha);

      m_InitColor = rgba;
    }
  }

  const xiiColorLinear16f initCol16 = m_InitColor;

  xiiProcessingStreamIterator<xiiColorLinear16f> itColor(m_pStreamColor, uiNumElements, uiStartIndex);
  while (!itColor.HasReachedEnd())
  {
    itColor.Current() = initCol16;
    itColor.Advance();
  }
}

void xiiParticleBehavior_ColorGradient::Process(xiiUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // set the update interval such that once the effect becomes visible,
    // all particles get fully updated
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate         = 0;
    return;
  }

  if (!m_hGradient.IsValid())
    return;

  XII_PROFILE_SCOPE("PFX: Color Gradient");

  xiiResourceLock<xiiColorGradientResource> pGradient(m_hGradient, xiiResourceAcquireMode::BlockTillLoaded);

  if (pGradient.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback)
    return;

  const xiiColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

  xiiProcessingStreamIterator<xiiColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itColor.Advance(m_uiFirstToUpdate);

  if (m_GradientMode == xiiParticleColorGradientMode::Age)
  {
    xiiProcessingStreamIterator<xiiFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    // skip the first n particles
    itLifeTime.Advance(m_uiFirstToUpdate);

    while (!itLifeTime.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
        const float posx              = 1.0f - fLifeTimeFraction;

        xiiColor rgba;
        xiiUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = xiiMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba * m_TintColor;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }
  else if (m_GradientMode == xiiParticleColorGradientMode::Speed)
  {
    xiiProcessingStreamIterator<xiiVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    // skip the first n particles
    itVelocity.Advance(m_uiFirstToUpdate);

    while (!itVelocity.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fSpeed = itVelocity.Current().GetLength();
        const float posx   = fSpeed / m_fMaxSpeed; // no need to clamp the range, the color lookup will already do that

        xiiColor rgba;
        xiiUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = xiiMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba * m_TintColor;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itVelocity.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
