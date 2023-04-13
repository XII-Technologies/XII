#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizerFactory_ApplyVelocity, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizerFactory_ApplyVelocity>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizer_ApplyVelocity, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizer_ApplyVelocity>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleFinalizerFactory_ApplyVelocity::xiiParticleFinalizerFactory_ApplyVelocity() {}

const xiiRTTI* xiiParticleFinalizerFactory_ApplyVelocity::GetFinalizerType() const
{
  return xiiGetStaticRTTI<xiiParticleFinalizer_ApplyVelocity>();
}

void xiiParticleFinalizerFactory_ApplyVelocity::CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const
{
  xiiParticleFinalizer_ApplyVelocity* pFinalizer = static_cast<xiiParticleFinalizer_ApplyVelocity*>(pObject);
}

xiiParticleFinalizer_ApplyVelocity::xiiParticleFinalizer_ApplyVelocity()
{
  // a bit later than the other finalizers
  m_fPriority = 525.0f;
}

xiiParticleFinalizer_ApplyVelocity::~xiiParticleFinalizer_ApplyVelocity() {}

void xiiParticleFinalizer_ApplyVelocity::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void xiiParticleFinalizer_ApplyVelocity::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: ApplyVelocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  xiiProcessingStreamIterator<xiiVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    xiiVec3& pos = reinterpret_cast<xiiVec3&>(itPosition.Current());

    pos += itVelocity.Current() * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
