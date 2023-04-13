#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizerFactory_Volume, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizerFactory_Volume>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizer_Volume, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizer_Volume>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleFinalizerFactory_Volume::xiiParticleFinalizerFactory_Volume()  = default;
xiiParticleFinalizerFactory_Volume::~xiiParticleFinalizerFactory_Volume() = default;

const xiiRTTI* xiiParticleFinalizerFactory_Volume::GetFinalizerType() const
{
  return xiiGetStaticRTTI<xiiParticleFinalizer_Volume>();
}

void xiiParticleFinalizerFactory_Volume::CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const
{
  xiiParticleFinalizer_Volume* pFinalizer = static_cast<xiiParticleFinalizer_Volume*>(pObject);
}

xiiParticleFinalizer_Volume::xiiParticleFinalizer_Volume()  = default;
xiiParticleFinalizer_Volume::~xiiParticleFinalizer_Volume() = default;

void xiiParticleFinalizer_Volume::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  m_pStreamSize = nullptr;
}

void xiiParticleFinalizer_Volume::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", xiiProcessingStream::DataType::Half);
}

void xiiParticleFinalizer_Volume::Process(xiiUInt64 uiNumElements)
{
  if (uiNumElements == 0)
    return;

  XII_PROFILE_SCOPE("PFX: Volume");

  const xiiSimdVec4f* pPosition = m_pStreamPosition->GetData<xiiSimdVec4f>();

  xiiSimdBBoxSphere volume;
  volume.SetFromPoints(pPosition, static_cast<xiiUInt32>(uiNumElements));

  float fMaxSize = 0;

  if (m_pStreamSize != nullptr)
  {
    const xiiFloat16* pSize = m_pStreamSize->GetData<xiiFloat16>();

    xiiSimdVec4f vMax;
    vMax.SetZero();

    constexpr xiiUInt32 uiElementsPerLoop = 4;
    for (xiiUInt64 i = 0; i < uiNumElements; i += uiElementsPerLoop)
    {
      const float x = pSize[i + 0];
      const float y = pSize[i + 1];
      const float z = pSize[i + 2];
      const float w = pSize[i + 3];

      vMax = vMax.CompMax(xiiSimdVec4f(x, y, z, w));
    }

    for (xiiUInt64 i = (uiNumElements / uiElementsPerLoop) * uiElementsPerLoop; i < uiNumElements; ++i)
    {
      fMaxSize = xiiMath::Max(fMaxSize, (float)pSize[i]);
    }

    fMaxSize = xiiMath::Max(fMaxSize, (float)vMax.HorizontalMax<4>());
  }

  GetOwnerSystem()->SetBoundingVolume(xiiSimdConversion::ToBBoxSphere(volume), fMaxSize);
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
