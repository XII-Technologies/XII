#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_CylinderPosition, 2, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_CylinderPosition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.01f, 100.0f)),
    XII_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    XII_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    XII_MEMBER_PROPERTY("Speed", m_Speed),
    XII_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
    XII_MEMBER_PROPERTY("ScaleHeightParam", m_sScaleHeightParameter),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCylinderVisualizerAttribute(xiiBasisAxis::PositiveZ, "Height", "Radius", xiiColor::MediumVioletRed, nullptr, xiiVisualizerAnchor::Center, xiiVec3::OneVector(), "PositionOffset")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_CylinderPosition, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_CylinderPosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleInitializerFactory_CylinderPosition::xiiParticleInitializerFactory_CylinderPosition()
{
  m_vPositionOffset.SetZero();
  m_fRadius         = 0.25f;
  m_fHeight         = 1.0f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity    = false;
}

const xiiRTTI* xiiParticleInitializerFactory_CylinderPosition::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_CylinderPosition>();
}

void xiiParticleInitializerFactory_CylinderPosition::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_CylinderPosition* pInitializer = static_cast<xiiParticleInitializer_CylinderPosition*>(pInitializer0);

  const float fScaleRadius = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_fRadius         = xiiMath::Max(m_fRadius * fScaleRadius, 0.01f); // prevent 0 radius
  pInitializer->m_fHeight         = xiiMath::Max(m_fHeight * fScaleHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity    = m_bSetVelocity;
  pInitializer->m_Speed           = m_Speed;
}

float xiiParticleInitializerFactory_CylinderPosition::GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const
{
  const float fScaleRadius = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  if (m_bSpawnOnSurface)
  {
    const float s0 = /* 2.0f * xiiMath::Pi<float>() * m_fRadius **/ m_fRadius + /* 2.0f * xiiMath::Pi<float>() * m_fRadius **/ m_fHeight;
    const float s1 = /* 2.0f * xiiMath::Pi<float>() * m_fRadius **/ m_fRadius * fScaleRadius * fScaleRadius +
      /*2.0f * xiiMath::Pi<float>() * m_fRadius **/ fScaleRadius * m_fHeight * fScaleHeight;

    return s1 / s0;
  }
  else
  {
    const float v0 = 1.0f /* xiiMath::Pi<float>() * m_fRadius * m_fRadius*/;
    const float v1 = 1.0f /* xiiMath::Pi<float>() * m_fRadius * m_fRadius*/ * fScaleRadius * fScaleRadius;

    return v1 / v0;
  }
}

void xiiParticleInitializerFactory_CylinderPosition::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_fHeight;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
  inout_stream << m_sScaleHeightParameter;
}

void xiiParticleInitializerFactory_CylinderPosition::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
  inout_stream >> m_fHeight;
  inout_stream >> m_bSpawnOnSurface;
  inout_stream >> m_bSetVelocity;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleRadiusParameter;
    inout_stream >> m_sScaleHeightParameter;
  }
}

void xiiParticleInitializerFactory_CylinderPosition::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void xiiParticleInitializer_CylinderPosition::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Cylinder Position");

  const xiiVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  xiiVec4* pPosition = m_pStreamPosition->GetWritableData<xiiVec4>();
  xiiVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<xiiVec3>() : nullptr;

  xiiRandom& rng = GetRNG();

  const float fRadiusSqr  = m_fRadius * m_fRadius;
  const float fHalfHeight = m_fHeight * 0.5f;

  const xiiTransform trans = GetOwnerSystem()->GetTransform();

  for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    xiiVec3 pos;
    float   len = 0.0f;
    pos.z       = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.y = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);

      len = pos.GetLengthSquared();
    } while (len > fRadiusSqr ||
             len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    xiiVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    if (m_fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(-fHalfHeight, fHalfHeight);
    }

    pos += m_vPositionOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      pVelocity[i] = startVel + trans.m_qRotation * normalPos * fSpeed;
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}

//////////////////////////////////////////////////////////////////////////

class xiiParticleInitializerFactory_CylinderPosition_1_2 : public xiiGraphPatch
{
public:
  xiiParticleInitializerFactory_CylinderPosition_1_2() :
    xiiGraphPatch("xiiParticleInitializerFactory_CylinderPosition", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

xiiParticleInitializerFactory_CylinderPosition_1_2 g_xiiParticleInitializerFactory_CylinderPosition_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
