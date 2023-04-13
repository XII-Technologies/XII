#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizerFactory_LastPosition, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizerFactory_LastPosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizer_LastPosition, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizer_LastPosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleFinalizerFactory_LastPosition::xiiParticleFinalizerFactory_LastPosition() {}

const xiiRTTI* xiiParticleFinalizerFactory_LastPosition::GetFinalizerType() const
{
  return xiiGetStaticRTTI<xiiParticleFinalizer_LastPosition>();
}

void xiiParticleFinalizerFactory_LastPosition::CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const
{
  xiiParticleFinalizer_LastPosition* pFinalizer = static_cast<xiiParticleFinalizer_LastPosition*>(pObject);
}

//////////////////////////////////////////////////////////////////////////

xiiParticleFinalizer_LastPosition::xiiParticleFinalizer_LastPosition()
{
  // do this at the start of the frame, but after the initializers
  m_fPriority = -499.0f;
}

xiiParticleFinalizer_LastPosition::~xiiParticleFinalizer_LastPosition() = default;

void xiiParticleFinalizer_LastPosition::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", xiiProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
}

void xiiParticleFinalizer_LastPosition::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: LastPosition");

  xiiProcessingStreamIterator<xiiVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    xiiVec3  curPos  = itPosition.Current().GetAsVec3();
    xiiVec3& lastPos = itLastPosition.Current();

    lastPos = curPos;

    itPosition.Advance();
    itLastPosition.Advance();
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
