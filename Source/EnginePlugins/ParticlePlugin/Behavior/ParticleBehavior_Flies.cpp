#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Flies.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_Flies, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_Flies>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FlySpeed", m_fSpeed)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 1000.0f)),
    XII_MEMBER_PROPERTY("PathLength", m_fPathLength)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("MaxEmitterDistance", m_fMaxEmitterDistance)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("MaxSteeringAngle", m_MaxSteeringAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(30)), new xiiClampValueAttribute(xiiAngle::Degree(1.0f), xiiAngle::Degree(180.0f))),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_Flies, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_Flies>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleBehaviorFactory_Flies::xiiParticleBehaviorFactory_Flies()  = default;
xiiParticleBehaviorFactory_Flies::~xiiParticleBehaviorFactory_Flies() = default;

const xiiRTTI* xiiParticleBehaviorFactory_Flies::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_Flies>();
}

void xiiParticleBehaviorFactory_Flies::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_Flies* pBehavior = static_cast<xiiParticleBehavior_Flies*>(pObject);

  pBehavior->m_fSpeed              = m_fSpeed;
  pBehavior->m_fPathLength         = m_fPathLength;
  pBehavior->m_fMaxEmitterDistance = m_fMaxEmitterDistance;
  pBehavior->m_MaxSteeringAngle    = m_MaxSteeringAngle;
}

void xiiParticleBehaviorFactory_Flies::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleBehaviorFactory_Flies::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  stream << uiVersion;

  stream << m_fSpeed;
  stream << m_fPathLength;
  stream << m_fMaxEmitterDistance;
  stream << m_MaxSteeringAngle;
}

void xiiParticleBehaviorFactory_Flies::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fSpeed;
  stream >> m_fPathLength;
  stream >> m_fMaxEmitterDistance;
  stream >> m_MaxSteeringAngle;
}

void xiiParticleBehavior_Flies::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);

  m_TimeToChangeDir.SetZero();
}

void xiiParticleBehavior_Flies::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Flies");

  const xiiTime tCur             = GetOwnerEffect()->GetTotalEffectLifeTime();
  const bool    bChangeDirection = tCur >= m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  m_TimeToChangeDir = tCur + xiiTime::Seconds(m_fPathLength / m_fSpeed);

  const xiiVec3 vEmitterPos                  = GetOwnerSystem()->GetTransform().m_vPosition;
  const float   fMaxDistanceToEmitterSquared = xiiMath::Square(m_fMaxEmitterDistance);

  xiiProcessingStreamIterator<xiiVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  xiiQuat qRot;

  while (!itPosition.HasReachedEnd())
  {
    // if (pLifeArray[i] == pMaxLifeArray[i])

    const xiiVec3 vPartToEm = vEmitterPos - itPosition.Current().GetAsVec3();
    const float   fDist     = vPartToEm.GetLengthSquared();
    const xiiVec3 vVelocity = itVelocity.Current();
    xiiVec3       vDir      = vVelocity;
    vDir.NormalizeIfNotZero().IgnoreResult();

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      xiiVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero().IgnoreResult();

      qRot.SetFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      itVelocity.Current() = qRot * vVelocity;
    }
    else
    {
      itVelocity.Current() = xiiVec3::CreateRandomDeviation(GetRNG(), m_MaxSteeringAngle, vDir) * m_fSpeed;
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Flies);

