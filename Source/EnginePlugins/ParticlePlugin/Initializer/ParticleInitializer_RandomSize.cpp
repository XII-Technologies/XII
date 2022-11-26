#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_RandomSize, 2, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_RandomSize>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Size", m_Size)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_RandomSize, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_RandomSize>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleInitializerFactory_RandomSize::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_RandomSize>();
}

void xiiParticleInitializerFactory_RandomSize::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_RandomSize* pInitializer = static_cast<xiiParticleInitializer_RandomSize*>(pInitializer0);

  pInitializer->m_hCurve = m_hCurve;
  pInitializer->m_Size   = m_Size;
}

void xiiParticleInitializerFactory_RandomSize::SetSizeCurveFile(const char* szFile)
{
  xiiCurve1DResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* xiiParticleInitializerFactory_RandomSize::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void xiiParticleInitializerFactory_RandomSize::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_Size.m_Value;
  stream << m_Size.m_fVariance;
}

void xiiParticleInitializerFactory_RandomSize::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_Size.m_Value;
  stream >> m_Size.m_fVariance;
}


void xiiParticleInitializer_RandomSize::CreateRequiredStreams()
{
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, true);
}

void xiiParticleInitializer_RandomSize::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Random Size");

  xiiFloat16* pSize = m_pStreamSize->GetWritableData<xiiFloat16>();

  xiiRandom& rng = GetRNG();

  if (!m_hCurve.IsValid())
  {
    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = (float)rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance);
    }
  }
  else
  {
    xiiResourceLock<xiiCurve1DResource> pResource(m_hCurve, xiiResourceAcquireMode::BlockTillLoaded);

    if (!pResource->GetDescriptor().m_Curves.IsEmpty())
    {
      const xiiCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

      double fMinX, fMaxX;
      curve.QueryExtents(fMinX, fMaxX);

      for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        const double f = rng.DoubleMinMax(fMinX, fMaxX);

        double val = curve.Evaluate(f);
        val        = curve.NormalizeValue(val);

        pSize[i] = (float)(val * rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance));
      }
    }
    else
    {
      for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        pSize[i] = 1.0f;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class xiiParticleInitializerFactory_RandomSize_1_2 : public xiiGraphPatch
{
public:
  xiiParticleInitializerFactory_RandomSize_1_2() :
    xiiGraphPatch("xiiParticleInitializerFactory_RandomSize", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Size").IgnoreResult();
  }
};

xiiParticleInitializerFactory_RandomSize_1_2 g_xiiParticleInitializerFactory_RandomSize_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
