#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_RandomColor, 1, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_RandomColor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    XII_MEMBER_PROPERTY("Color1", m_Color1)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White), new xiiExposeColorAlphaAttribute()),
    XII_MEMBER_PROPERTY("Color2", m_Color2)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White), new xiiExposeColorAlphaAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_RandomColor, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_RandomColor>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleInitializerFactory_RandomColor::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_RandomColor>();
}

void xiiParticleInitializerFactory_RandomColor::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_RandomColor* pInitializer = static_cast<xiiParticleInitializer_RandomColor*>(pInitializer0);

  pInitializer->m_hGradient = m_hGradient;
  pInitializer->m_Color1    = m_Color1;
  pInitializer->m_Color2    = m_Color2;
}

void xiiParticleInitializerFactory_RandomColor::SetColorGradientFile(const char* szFile)
{
  xiiColorGradientResourceHandle hGradient;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hGradient = xiiResourceManager::LoadResource<xiiColorGradientResource>(szFile);
  }

  SetColorGradient(hGradient);
}


const char* xiiParticleInitializerFactory_RandomColor::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void xiiParticleInitializerFactory_RandomColor::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_hGradient;
  inout_stream << m_Color1;
  inout_stream << m_Color2;
}

void xiiParticleInitializerFactory_RandomColor::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hGradient;
  inout_stream >> m_Color1;
  inout_stream >> m_Color2;
}


void xiiParticleInitializer_RandomColor::CreateRequiredStreams()
{
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, true);
}

void xiiParticleInitializer_RandomColor::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Random Color");

  xiiColorLinear16f* pColor = m_pStreamColor->GetWritableData<xiiColorLinear16f>();

  xiiRandom& rng = GetRNG();

  if (!m_hGradient.IsValid())
  {
    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleZeroToOneInclusive();
      pColor[i]     = xiiMath::Lerp(m_Color1, m_Color2, f);
    }
  }
  else
  {
    xiiResourceLock<xiiColorGradientResource> pResource(m_hGradient, xiiResourceAcquireMode::BlockTillLoaded);

    double                  fMinValue, fMaxValue;
    const xiiColorGradient& gradient = pResource->GetDescriptor().m_Gradient;
    gradient.GetExtents(fMinValue, fMaxValue);

    xiiColorGammaUB color;
    float           intensity;

    const bool bMulColor = (m_Color1 != xiiColor::White) && (m_Color2 != xiiColor::White);

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const double f = rng.DoubleMinMax(fMinValue, fMaxValue);

      gradient.Evaluate(f, color, intensity);

      xiiColor result = color;
      result.r *= intensity;
      result.g *= intensity;
      result.b *= intensity;

      if (bMulColor)
      {
        const float f2 = (float)rng.DoubleZeroToOneInclusive();
        result *= xiiMath::Lerp(m_Color1, m_Color2, f2);
      }

      pColor[i] = result;
    }
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
