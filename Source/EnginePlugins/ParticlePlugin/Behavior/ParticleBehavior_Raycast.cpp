#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_Raycast, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_Raycast>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Reaction", xiiParticleRaycastHitReaction, m_Reaction),
    XII_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new xiiDefaultValueAttribute(0.6f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_Raycast, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_Raycast>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleRaycastHitReaction, 1)
  XII_ENUM_CONSTANTS(xiiParticleRaycastHitReaction::Bounce, xiiParticleRaycastHitReaction::Die, xiiParticleRaycastHitReaction::Stop)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiParticleBehaviorFactory_Raycast::xiiParticleBehaviorFactory_Raycast()  = default;
xiiParticleBehaviorFactory_Raycast::~xiiParticleBehaviorFactory_Raycast() = default;

const xiiRTTI* xiiParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_Raycast>();
}

void xiiParticleBehaviorFactory_Raycast::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_Raycast* pBehavior = static_cast<xiiParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction         = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent  = xiiTempHashedString(m_sOnCollideEvent.GetData());
  pBehavior->m_fBounceFactor    = m_fBounceFactor;

  pBehavior->m_pPhysicsModule = (xiiPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(xiiGetStaticRTTI<xiiPhysicsWorldModuleInterface>());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void xiiParticleBehaviorFactory_Raycast::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_uiCollisionLayer;
  inout_stream << m_sOnCollideEvent;

  xiiParticleRaycastHitReaction::StorageType hr = m_Reaction.GetValue();
  inout_stream << hr;

  inout_stream << m_fBounceFactor;
}

void xiiParticleBehaviorFactory_Raycast::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiCollisionLayer;
    inout_stream >> m_sOnCollideEvent;

    xiiParticleRaycastHitReaction::StorageType hr;
    inout_stream >> hr;
    m_Reaction.SetValue(hr);
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fBounceFactor;
  }
}

void xiiParticleBehaviorFactory_Raycast::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
  inout_finalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

xiiParticleBehavior_Raycast::xiiParticleBehavior_Raycast()
{
  // do this right after xiiParticleFinalizer_ApplyVelocity has run
  m_fPriority = 526.0f;
}

void xiiParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", xiiProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void xiiParticleBehavior_Raycast::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  xiiProcessingStreamIterator<xiiVec4>       itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<const xiiVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiVec3>       itVelocity(m_pStreamVelocity, uiNumElements, 0);

  xiiPhysicsCastResult hitResult;

  xiiUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const xiiVec3 vLastPos = itLastPosition.Current();
    const xiiVec3 vCurPos  = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const xiiVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(0.001f))
      {
        xiiVec3 vDirection = vChange;

        const float fMaxLen = vDirection.GetLengthAndNormalize();

        xiiPhysicsQueryParameters query(m_uiCollisionLayer);
        query.m_ShapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic;

        if (m_pPhysicsModule != nullptr && m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen, query))
        {
          if (m_Reaction == xiiParticleRaycastHitReaction::Bounce)
          {
            const xiiVec3 vNewDir = vChange.GetReflectedVector(hitResult.m_vNormal) * m_fBounceFactor;

            itPosition.Current() = xiiVec3(hitResult.m_vPosition + hitResult.m_vNormal * 0.05f + vNewDir).GetAsVec4(0);
            itVelocity.Current() = vNewDir / tDiff;
          }
          else if (m_Reaction == xiiParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }
          else if (m_Reaction == xiiParticleRaycastHitReaction::Stop)
          {
            itVelocity.Current().SetZero();
          }

          if (!m_sOnCollideEvent.IsEmpty())
          {
            xiiParticleEvent e;
            e.m_EventType  = m_sOnCollideEvent;
            e.m_vPosition  = hitResult.m_vPosition;
            e.m_vNormal    = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->AddParticleEvent(e);
          }
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    ++i;
  }
}

void xiiParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<xiiPhysicsWorldModuleInterface>();
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
