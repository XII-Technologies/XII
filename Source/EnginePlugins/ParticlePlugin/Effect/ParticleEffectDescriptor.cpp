#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEffectDescriptor, 2, xiiRTTIDefaultAllocator<xiiParticleEffectDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("WhenInvisible", xiiEffectInvisibleUpdateRate, m_InvisibleUpdateRate),
    XII_MEMBER_PROPERTY("AlwaysShared", m_bAlwaysShared),
    XII_MEMBER_PROPERTY("SimulateInLocalSpace", m_bSimulateInLocalSpace),
    XII_MEMBER_PROPERTY("ApplyOwnerVelocity", m_fApplyInstanceVelocity)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("PreSimulateDuration", m_PreSimulateDuration),
    XII_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    XII_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new xiiExposeColorAlphaAttribute),
    XII_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_SET_ACCESSOR_PROPERTY("EventReactions", GetEventReactions, AddEventReaction, RemoveEventReaction)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEffectDescriptor::xiiParticleEffectDescriptor() = default;

xiiParticleEffectDescriptor::~xiiParticleEffectDescriptor()
{
  ClearSystems();
  ClearEventReactions();
}

void xiiParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}


void xiiParticleEffectDescriptor::ClearEventReactions()
{
  for (auto pReaction : m_EventReactions)
  {
    pReaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pReaction);
  }

  m_EventReactions.Clear();
}

enum class ParticleEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5, // m_bAlwaysShared
  Version_6, // added parameters
  Version_7, // added instance velocity
  Version_8, // added event reactions
  Version_9, // breaking change

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleEffectDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  inout_stream << uiVersion;

  const xiiUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  inout_stream << uiNumSystems;

  // Version 3
  inout_stream << m_bSimulateInLocalSpace;
  inout_stream << m_PreSimulateDuration;
  // Version 4
  inout_stream << m_InvisibleUpdateRate;
  // Version 5
  inout_stream << m_bAlwaysShared;

  // Version 3
  for (auto pSystem : m_ParticleSystems)
  {
    inout_stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(inout_stream);
  }

  // Version 6
  {
    xiiUInt8 paramCol = static_cast<xiiUInt8>(m_ColorParameters.GetCount());
    inout_stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }

    xiiUInt8 paramFloat = static_cast<xiiUInt8>(m_FloatParameters.GetCount());
    inout_stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      inout_stream << it.Key();
      inout_stream << it.Value();
    }
  }

  // Version 7
  inout_stream << m_fApplyInstanceVelocity;

  // Version 8
  {
    const xiiUInt32 uiNumReactions = m_EventReactions.GetCount();
    inout_stream << uiNumReactions;

    for (auto pReaction : m_EventReactions)
    {
      inout_stream << pReaction->GetDynamicRTTI()->GetTypeName();

      pReaction->Save(inout_stream);
    }
  }
}


void xiiParticleEffectDescriptor::Load(xiiStreamReader& inout_stream)
{
  ClearSystems();
  ClearEventReactions();

  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion < (int)ParticleEffectVersion::Version_9)
  {
    xiiLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  xiiUInt32 uiNumSystems = 0;
  inout_stream >> uiNumSystems;

  inout_stream >> m_bSimulateInLocalSpace;
  inout_stream >> m_PreSimulateDuration;
  inout_stream >> m_InvisibleUpdateRate;
  inout_stream >> m_bAlwaysShared;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  xiiStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    inout_stream >> sType;

    const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
    XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

    pSystem = pRtti->GetAllocator()->Allocate<xiiParticleSystemDescriptor>();

    pSystem->Load(inout_stream);
  }

  xiiStringBuilder key;
  m_ColorParameters.Clear();
  m_FloatParameters.Clear();

  xiiUInt8 paramCol;
  inout_stream >> paramCol;
  for (xiiUInt32 i = 0; i < paramCol; ++i)
  {
    xiiColor val;
    inout_stream >> key;
    inout_stream >> val;
    m_ColorParameters[key] = val;
  }

  xiiUInt8 paramFloat;
  inout_stream >> paramFloat;
  for (xiiUInt32 i = 0; i < paramFloat; ++i)
  {
    float val;
    inout_stream >> key;
    inout_stream >> val;
    m_FloatParameters[key] = val;
  }

  inout_stream >> m_fApplyInstanceVelocity;

  xiiUInt32 uiNumReactions = 0;
  inout_stream >> uiNumReactions;

  m_EventReactions.SetCountUninitialized(uiNumReactions);

  for (auto& pReaction : m_EventReactions)
  {
    inout_stream >> sType;

    const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
    XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect event reaction type '{0}'", sType);

    pReaction = pRtti->GetAllocator()->Allocate<xiiParticleEventReactionFactory>();

    pReaction->Load(inout_stream);
  }
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);
