#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Light/ParticleTypeLight.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeLightFactory, 1, xiiRTTIDefaultAllocator<xiiParticleTypeLightFactory>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, 1000.0f)),
    XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, 100000.0f)),
    XII_MEMBER_PROPERTY("Percentage", m_uiPercentage)->AddAttributes(new xiiDefaultValueAttribute(50), new xiiClampValueAttribute(1, 100)),
    XII_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    XII_MEMBER_PROPERTY("IntensityScaleParam", m_sIntensityParameter),
    XII_MEMBER_PROPERTY("SizeScaleParam", m_sSizeScaleParameter),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeLight, 1, xiiRTTIDefaultAllocator<xiiParticleTypeLight>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleTypeLightFactory::xiiParticleTypeLightFactory()
{
  m_fSizeFactor  = 5.0f;
  m_fIntensity   = 10.0f;
  m_uiPercentage = 50;
}


const xiiRTTI* xiiParticleTypeLightFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypeLight>();
}

void xiiParticleTypeLightFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  xiiParticleTypeLight* pType = static_cast<xiiParticleTypeLight*>(pObject);

  pType->m_fSizeFactor         = m_fSizeFactor;
  pType->m_fIntensity          = m_fIntensity;
  pType->m_uiPercentage        = m_uiPercentage;
  pType->m_sTintColorParameter = xiiTempHashedString(m_sTintColorParameter.GetData());
  pType->m_sIntensityParameter = xiiTempHashedString(m_sIntensityParameter.GetData());
  pType->m_sSizeScaleParameter = xiiTempHashedString(m_sSizeScaleParameter.GetData());
}

enum class TypeLightVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added tint color and intensity parameter

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypeLightFactory::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)TypeLightVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSizeFactor;
  inout_stream << m_fIntensity;
  inout_stream << m_uiPercentage;

  // Version 2
  inout_stream << m_sTintColorParameter;
  inout_stream << m_sIntensityParameter;
  inout_stream << m_sSizeScaleParameter;
}

void xiiParticleTypeLightFactory::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSizeFactor;
  inout_stream >> m_fIntensity;
  inout_stream >> m_uiPercentage;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sTintColorParameter;
    inout_stream >> m_sIntensityParameter;
    inout_stream >> m_sSizeScaleParameter;
  }
}

void xiiParticleTypeLight::CreateRequiredStreams()
{
  m_pStreamOnOff = nullptr;

  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", xiiProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", xiiProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_uiPercentage < 100)
  {
    CreateStream("OnOff", xiiProcessingStream::DataType::Int, &m_pStreamOnOff, false); /// \todo Initialize (instead of during extraction)
  }
}


void xiiParticleTypeLight::ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const
{
  XII_PROFILE_SCOPE("PFX: Light");

  const xiiVec4*           pPosition = m_pStreamPosition->GetData<xiiVec4>();
  const xiiFloat16*        pSize     = m_pStreamSize->GetData<xiiFloat16>();
  const xiiColorLinear16f* pColor    = m_pStreamColor->GetData<xiiColorLinear16f>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  xiiInt32* pOnOff = nullptr;

  if (m_pStreamOnOff)
  {
    pOnOff = m_pStreamOnOff->GetWritableData<xiiInt32>();

    if (pOnOff == nullptr)
      return;
  }

  xiiRandom& rng = GetRNG();

  const xiiUInt32 uiNumParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const xiiUInt32 uiBatchId = 1; // no shadows

  const xiiColor tintColor      = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, xiiColor::White);
  const float    intensityScale = GetOwnerEffect()->GetFloatParameter(m_sIntensityParameter, 1.0f);
  const float    sizeScale      = GetOwnerEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f);

  const float sizeFactor = m_fSizeFactor * sizeScale;
  const float intensity  = intensityScale * m_fIntensity;

  xiiTransform transform;

  if (this->GetOwnerEffect()->IsSimulatedInLocalSpace())
    transform = instanceTransform;
  else
    transform.SetIdentity();

  for (xiiUInt32 i = 0; i < uiNumParticles; ++i)
  {
    if (pOnOff)
    {
      if (pOnOff[i] == 0)
      {
        if ((xiiUInt32)rng.IntMinMax(0, 100) <= m_uiPercentage)
          pOnOff[i] = 1;
        else
          pOnOff[i] = -1;
      }

      if (pOnOff[i] < 0)
        continue;
    }

    auto pRenderData = xiiCreateRenderDataForThisFrame<xiiPointLightRenderData>(nullptr);

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = transform * pPosition[i].GetAsVec3();
    pRenderData->m_LightColor                  = tintColor * pColor[i].ToLinearFloat();
    pRenderData->m_fIntensity                  = intensity;
    pRenderData->m_fRange                      = pSize[i] * sizeFactor;
    pRenderData->m_uiShadowDataOffset          = xiiInvalidIndex;

    float fScreenSpaceSize = xiiLightComponent::CalculateScreenSpaceSize(xiiBoundingSphere(pRenderData->m_GlobalTransform.m_vPosition, pRenderData->m_fRange * 0.5f), *ref_msg.m_pView->GetCullingCamera());
    pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

    ref_msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::Light, xiiRenderData::Caching::Never);
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Light_ParticleTypeLight);
