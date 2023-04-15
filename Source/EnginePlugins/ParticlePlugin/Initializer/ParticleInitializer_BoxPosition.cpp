#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_BoxPosition, 1, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_BoxPosition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    XII_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, 0))),
    XII_MEMBER_PROPERTY("ScaleXParam", m_sScaleXParameter),
    XII_MEMBER_PROPERTY("ScaleYParam", m_sScaleYParameter),
    XII_MEMBER_PROPERTY("ScaleZParam", m_sScaleZParameter),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiBoxVisualizerAttribute("Size", 1.0f, xiiColor::MediumVioletRed, nullptr, xiiVisualizerAnchor::Center, xiiVec3::OneVector(), "PositionOffset")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_BoxPosition, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_BoxPosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleInitializerFactory_BoxPosition::xiiParticleInitializerFactory_BoxPosition()
{
  m_vPositionOffset.SetZero();
  m_vSize.Set(0, 0, 0);
}

const xiiRTTI* xiiParticleInitializerFactory_BoxPosition::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_BoxPosition>();
}

void xiiParticleInitializerFactory_BoxPosition::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_BoxPosition* pInitializer = static_cast<xiiParticleInitializer_BoxPosition*>(pInitializer0);

  const float fScaleX = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  xiiVec3 vSize = m_vSize;
  vSize.x *= fScaleX;
  vSize.y *= fScaleY;
  vSize.z *= fScaleZ;

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_vSize           = vSize;
}

float xiiParticleInitializerFactory_BoxPosition::GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const
{
  const float fScaleX = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  float fSpawnMultiplier = 1.0f;

  if (m_vSize.x != 0.0f)
    fSpawnMultiplier *= fScaleX;

  if (m_vSize.y != 0.0f)
    fSpawnMultiplier *= fScaleY;

  if (m_vSize.z != 0.0f)
    fSpawnMultiplier *= fScaleZ;

  return fSpawnMultiplier;
}

void xiiParticleInitializerFactory_BoxPosition::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_vSize;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleXParameter;
  inout_stream << m_sScaleYParameter;
  inout_stream << m_sScaleZParameter;
}

void xiiParticleInitializerFactory_BoxPosition::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_vSize;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleXParameter;
    inout_stream >> m_sScaleYParameter;
    inout_stream >> m_sScaleZParameter;
  }
}

void xiiParticleInitializer_BoxPosition::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, true);
}

void xiiParticleInitializer_BoxPosition::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Box Position");

  xiiSimdVec4f* pPosition = m_pStreamPosition->GetWritableData<xiiSimdVec4f>();

  xiiRandom& rng = GetRNG();

  if (m_vSize.IsZero())
  {
    xiiVec4 pos0 = (GetOwnerSystem()->GetTransform() * m_vPositionOffset).GetAsVec4(0);

    xiiSimdVec4f pos;
    pos.Load<4>(&pos0.x);

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    xiiTransform ownerTransform = GetOwnerSystem()->GetTransform();

    xiiSimdVec4f     pos;
    xiiSimdTransform transform;
    transform.m_Position.Load<3>(&ownerTransform.m_vPosition.x);
    transform.m_Rotation.m_v.Load<4>(&ownerTransform.m_qRotation.v.x);
    transform.m_Scale.Load<3>(&ownerTransform.m_vScale.x);

    float p0[4];
    p0[3] = 0;

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      p0[0] = (float)(rng.DoubleMinMax(-m_vSize.x, m_vSize.x) * 0.5) + m_vPositionOffset.x;
      p0[1] = (float)(rng.DoubleMinMax(-m_vSize.y, m_vSize.y) * 0.5) + m_vPositionOffset.y;
      p0[2] = (float)(rng.DoubleMinMax(-m_vSize.z, m_vSize.z) * 0.5) + m_vPositionOffset.z;

      pos.Load<4>(p0);

      pPosition[i] = transform.TransformPosition(pos);
    }
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
