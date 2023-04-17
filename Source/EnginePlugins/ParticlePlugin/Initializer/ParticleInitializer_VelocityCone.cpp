#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_VelocityCone, 2, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_VelocityCone>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(30)), new xiiClampValueAttribute(xiiAngle::Degree(1), xiiAngle::Degree(89))),
    XII_MEMBER_PROPERTY("Speed", m_Speed),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiConeVisualizerAttribute(xiiBasisAxis::PositiveZ, "Angle", 1.0f, nullptr, xiiColor::CornflowerBlue)
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_VelocityCone, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_VelocityCone>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleInitializerFactory_VelocityCone::xiiParticleInitializerFactory_VelocityCone()
{
  m_Angle = xiiAngle::Degree(45);
}

const xiiRTTI* xiiParticleInitializerFactory_VelocityCone::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_VelocityCone>();
}

void xiiParticleInitializerFactory_VelocityCone::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_VelocityCone* pInitializer = static_cast<xiiParticleInitializer_VelocityCone*>(pInitializer0);

  pInitializer->m_Angle = xiiMath::Clamp(m_Angle, xiiAngle::Degree(1), xiiAngle::Degree(89));
  pInitializer->m_Speed = m_Speed;
}

void xiiParticleInitializerFactory_VelocityCone::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_Angle;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;
}

void xiiParticleInitializerFactory_VelocityCone::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_Angle;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;
}

void xiiParticleInitializerFactory_VelocityCone::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void xiiParticleInitializer_VelocityCone::CreateRequiredStreams()
{
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
}

void xiiParticleInitializer_VelocityCone::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Velocity Cone");

  const xiiVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  xiiVec3* pVelocity = m_pStreamVelocity->GetWritableData<xiiVec3>();

  xiiRandom& rng = GetRNG();

  // const float dist = 1.0f / xiiMath::Tan(m_Angle);

  for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    const xiiVec3 dir = xiiVec3::CreateRandomDeviationZ(rng, m_Angle);
    // dir.z = 0;
    // float len = 0.0f;

    // do
    //{
    //  // random point in a rectangle
    //  dir.x = (float)rng.DoubleMinMax(-1.0, 1.0);
    //  dir.y = (float)rng.DoubleMinMax(-1.0, 1.0);

    //  // discard points outside the circle
    //  len = dir.GetLengthSquared();
    //} while (len > 1.0f);

    // dir.z = dist;
    // dir.Normalize();

    const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

    pVelocity[i] = startVel + GetOwnerSystem()->GetTransform().m_qRotation * dir * fSpeed;
  }
}

//////////////////////////////////////////////////////////////////////////

class xiiParticleInitializerFactory_VelocityCone_1_2 : public xiiGraphPatch
{
public:
  xiiParticleInitializerFactory_VelocityCone_1_2() :
    xiiGraphPatch("xiiParticleInitializerFactory_VelocityCone", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

xiiParticleInitializerFactory_VelocityCone_1_2 g_xiiParticleInitializerFactory_VelocityCone_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
