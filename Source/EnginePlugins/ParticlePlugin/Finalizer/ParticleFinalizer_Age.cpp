#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizerFactory_Age, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizerFactory_Age>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizer_Age, 1, xiiRTTIDefaultAllocator<xiiParticleFinalizer_Age>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleFinalizerFactory_Age::xiiParticleFinalizerFactory_Age() = default;

const xiiRTTI* xiiParticleFinalizerFactory_Age::GetFinalizerType() const
{
  return xiiGetStaticRTTI<xiiParticleFinalizer_Age>();
}

void xiiParticleFinalizerFactory_Age::CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const
{
  xiiParticleFinalizer_Age* pFinalizer = static_cast<xiiParticleFinalizer_Age*>(pObject);

  pFinalizer->m_LifeTime            = m_LifeTime;
  pFinalizer->m_sOnDeathEvent       = xiiTempHashedString(m_sOnDeathEvent.GetData());
  pFinalizer->m_sLifeScaleParameter = xiiTempHashedString(m_sLifeScaleParameter.GetData());

  if (pFinalizer->m_bHasOnDeathEventHandler)
  {
    pFinalizer->m_bHasOnDeathEventHandler = false;
    pFinalizer->GetOwnerSystem()->RemoveParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }

  if (!pFinalizer->m_sOnDeathEvent.IsEmpty())
  {
    pFinalizer->m_bHasOnDeathEventHandler = true;
    pFinalizer->GetOwnerSystem()->AddParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }
}

xiiParticleFinalizer_Age::xiiParticleFinalizer_Age() = default;

xiiParticleFinalizer_Age::~xiiParticleFinalizer_Age()
{
  if (m_bHasOnDeathEventHandler)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleFinalizer_Age::OnParticleDeath, this));
  }
}

void xiiParticleFinalizer_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", xiiProcessingStream::DataType::Half2, &m_pStreamLifeTime, true);

  m_pStreamPosition = nullptr;
  m_pStreamVelocity = nullptr;

  if (!m_sOnDeathEvent.IsEmpty())
  {
    CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
    CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}

void xiiParticleFinalizer_Age::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Age Init");

  xiiFloat16Vec2* pLifeTime  = m_pStreamLifeTime->GetWritableData<xiiFloat16Vec2>();
  const float     fLifeScale = xiiMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f, 2.0f);

  if (m_LifeTime.m_fVariance == 0)
  {
    const float tLifeTime    = (fLifeScale * (float)m_LifeTime.m_Value.GetSeconds()) + 0.01f; // make sure it's not zero
    const float tInvLifeTime = 1.0f / tLifeTime;

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
  else // random range
  {
    xiiRandom& rng = GetRNG();

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime =
        (fLifeScale * (float)rng.DoubleVariance(m_LifeTime.m_Value.GetSeconds(), m_LifeTime.m_fVariance)) + 0.01f; // make sure it's not zero
      const float tInvLifeTime = 1.0f / tLifeTime;

      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
}

void xiiParticleFinalizer_Age::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Age");

  xiiFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<xiiFloat16Vec2>();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  for (xiiUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x = pLifeTime[i].x - tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      /// \todo Get current element index from iterator ?
      m_pStreamGroup->RemoveElement(i);
    }
  }
}

void xiiParticleFinalizer_Age::OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e)
{
  const xiiVec4* pPosition = m_pStreamPosition->GetData<xiiVec4>();
  const xiiVec3* pVelocity = m_pStreamVelocity->GetData<xiiVec3>();

  xiiParticleEvent pe;
  pe.m_EventType  = m_sOnDeathEvent;
  pe.m_vPosition  = pPosition[e.m_uiElementIndex].GetAsVec3();
  pe.m_vDirection = pVelocity[e.m_uiElementIndex];
  pe.m_vNormal.SetZero();

  GetOwnerEffect()->AddParticleEvent(pe);
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Age);
