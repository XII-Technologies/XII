#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_SizeCurve, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_SizeCurve>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    XII_MEMBER_PROPERTY("BaseSize", m_fBaseSize)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("CurveScale", m_fCurveScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_SizeCurve, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_SizeCurve>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleBehaviorFactory_SizeCurve::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_SizeCurve>();
}

void xiiParticleBehaviorFactory_SizeCurve::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_SizeCurve* pBehavior = static_cast<xiiParticleBehavior_SizeCurve*>(pObject);

  pBehavior->m_hCurve      = m_hCurve;
  pBehavior->m_fBaseSize   = m_fBaseSize;
  pBehavior->m_fCurveScale = m_fCurveScale;
}

void xiiParticleBehaviorFactory_SizeCurve::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_fBaseSize;
  stream << m_fCurveScale;
}

void xiiParticleBehaviorFactory_SizeCurve::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_fBaseSize;
  stream >> m_fCurveScale;
}

void xiiParticleBehaviorFactory_SizeCurve::SetSizeCurveFile(const char* szFile)
{
  xiiCurve1DResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* xiiParticleBehaviorFactory_SizeCurve::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void xiiParticleBehavior_SizeCurve::CreateRequiredStreams()
{
  CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, false);
}


void xiiParticleBehavior_SizeCurve::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiFloat16> itSize(m_pStreamSize, uiNumElements, uiStartIndex);
  while (!itSize.HasReachedEnd())
  {
    itSize.Current() = m_fBaseSize;
    itSize.Advance();
  }
}

void xiiParticleBehavior_SizeCurve::Process(xiiUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // reduce the update interval when the effect is not visible
    m_uiCurrentUpdateInterval = 32;
  }
  else
  {
    m_uiCurrentUpdateInterval = 2;
  }

  if (!m_hCurve.IsValid())
    return;

  XII_PROFILE_SCOPE("PFX: Size Curve");

  xiiProcessingStreamIterator<xiiFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiFloat16>     itSize(m_pStreamSize, uiNumElements, 0);

  xiiResourceLock<xiiCurve1DResource> pCurve(m_hCurve, xiiResourceAcquireMode::BlockTillLoaded);

  if (pCurve.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback)
    return;

  if (pCurve->GetDescriptor().m_Curves.IsEmpty())
    return;

  auto& curve = pCurve->GetDescriptor().m_Curves[0];

  double fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  // skip the first n particles
  {
    for (xiiUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  while (!itLifeTime.HasReachedEnd())
  {
    // if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = 1.0f - (itLifeTime.Current().x * itLifeTime.Current().y);

      const double evalPos = curve.ConvertNormalizedPos(fLifeTimeFraction);
      double       val     = curve.Evaluate(evalPos);
      val                  = curve.NormalizeValue(val);

      itSize.Current() = m_fBaseSize + (float)val * m_fCurveScale;
    }

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    for (xiiUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
