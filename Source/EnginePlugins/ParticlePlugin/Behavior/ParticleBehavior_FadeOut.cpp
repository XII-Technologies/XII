#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_FadeOut.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_FadeOut, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_FadeOut>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("StartAlpha", m_fStartAlpha)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Exponent", m_fExponent)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_FadeOut, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_FadeOut>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleBehaviorFactory_FadeOut::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_FadeOut>();
}

void xiiParticleBehaviorFactory_FadeOut::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_FadeOut* pBehavior = static_cast<xiiParticleBehavior_FadeOut*>(pObject);

  pBehavior->m_fStartAlpha = m_fStartAlpha;
  pBehavior->m_fExponent   = m_fExponent;
}

void xiiParticleBehaviorFactory_FadeOut::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fStartAlpha;
  stream << m_fExponent;
}

void xiiParticleBehaviorFactory_FadeOut::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fStartAlpha;
  stream >> m_fExponent;
}

void xiiParticleBehavior_FadeOut::CreateRequiredStreams()
{
  CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void xiiParticleBehavior_FadeOut::Process(xiiUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // set the update interval such that once the effect becomes visible,
    // all particles get fully updated
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate         = 0;
    return;
  }

  XII_PROFILE_SCOPE("PFX: Fade Out");

  xiiProcessingStreamIterator<xiiFloat16Vec2>    itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  {
    for (xiiUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itColor.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  if (m_fStartAlpha <= 1.0f)
  {
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a           = m_fStartAlpha * xiiMath::Pow(fLifeTimeFraction, m_fExponent);

      for (xiiUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
      {
        itLifeTime.Advance();
        itColor.Advance();
      }
    }
  }
  else
  {
    // this case has to clamp alpha to 1
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a           = xiiMath::Min(1.0f, m_fStartAlpha * xiiMath::Pow(fLifeTimeFraction, m_fExponent));

      for (xiiUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
      {
        itLifeTime.Advance();
        itColor.Advance();
      }
    }
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
