#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_SpherePosition, 2, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_SpherePosition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(0.25f), new xiiClampValueAttribute(0.01f, 100.0f)),
    XII_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    XII_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    XII_MEMBER_PROPERTY("Speed", m_Speed),
    XII_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereVisualizerAttribute("Radius", xiiColor::MediumVioletRed, nullptr, xiiVisualizerAnchor::Center, xiiVec3::OneVector(), "PositionOffset"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_SpherePosition, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_SpherePosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleInitializerFactory_SpherePosition::xiiParticleInitializerFactory_SpherePosition()
{
  m_fRadius = 0.25f;
  m_vPositionOffset.SetZero();
  m_bSpawnOnSurface = false;
  m_bSetVelocity    = false;
}

const xiiRTTI* xiiParticleInitializerFactory_SpherePosition::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_SpherePosition>();
}

void xiiParticleInitializerFactory_SpherePosition::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_SpherePosition* pInitializer = static_cast<xiiParticleInitializer_SpherePosition*>(pInitializer0);

  const float fScale = pInitializer->GetOwnerEffect()->GetFloatParameter(xiiTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  pInitializer->m_fRadius         = xiiMath::Max(m_fRadius * fScale, 0.01f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity    = m_bSetVelocity;
  pInitializer->m_Speed           = m_Speed;
  pInitializer->m_vPositionOffset = m_vPositionOffset;
}

float xiiParticleInitializerFactory_SpherePosition::GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const
{
  const float fScale = pEffect->GetFloatParameter(xiiTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  if (m_fRadius != 0.0f && fScale != 1.0f)
  {
    if (m_bSpawnOnSurface)
    {
      // original surface area
      const float s0 = 1.0f; /*4.0f * xiiMath::Pi<float>() * m_fRadius * m_fRadius; */
      // new surface area
      const float s1 = 1.0f /*4.0f * xiiMath::Pi<float>() * m_fRadius * m_fRadius */ * fScale * fScale;

      return s1 / s0;
    }
    else
    {
      // original volume
      const float v0 = 1.0f;
      /* 4.0f / 3.0f * xiiMath::Pi<float>() * m_fRadius* m_fRadius* m_fRadius; */
      // new volume
      const float v1 = 1.0f /* 4.0f / 3.0f * xiiMath::Pi<float>() * m_fRadius * m_fRadius * m_fRadius*/ * fScale * fScale * fScale;

      return v1 / v0;
    }
  }

  return 1.0f;
}

void xiiParticleInitializerFactory_SpherePosition::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
}

void xiiParticleInitializerFactory_SpherePosition::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
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
  }
}

void xiiParticleInitializerFactory_SpherePosition::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiParticleInitializer_SpherePosition::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void xiiParticleInitializer_SpherePosition::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Sphere Position");

  const xiiVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  xiiVec4* pPosition = m_pStreamPosition->GetWritableData<xiiVec4>();
  xiiVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<xiiVec3>() : nullptr;

  xiiRandom& rng = GetRNG();

  const xiiTransform trans = GetOwnerSystem()->GetTransform();

  for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    xiiVec3 pos       = xiiVec3::CreateRandomPointInSphere(rng) * m_fRadius;
    xiiVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

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

class xiiParticleInitializerFactory_SpherePosition_1_2 : public xiiGraphPatch
{
public:
  xiiParticleInitializerFactory_SpherePosition_1_2() :
    xiiGraphPatch("xiiParticleInitializerFactory_SpherePosition", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

xiiParticleInitializerFactory_SpherePosition_1_2 g_xiiParticleInitializerFactory_SpherePosition_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
