#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReactionFactory, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EventType", m_sEventType),
    XII_MEMBER_PROPERTY("Probability", m_uiProbability)->AddAttributes(new xiiDefaultValueAttribute(100), new xiiClampValueAttribute(1, 100)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReaction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

xiiParticleEventReaction* xiiParticleEventReactionFactory::CreateEventReaction(xiiParticleEffectInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetEventReactionType();

  xiiParticleEventReaction* pReaction = pRtti->GetAllocator()->Allocate<xiiParticleEventReaction>();
  pReaction->Reset(pOwner);
  pReaction->m_sEventName    = xiiTempHashedString(m_sEventType.GetData());
  pReaction->m_uiProbability = m_uiProbability;

  CopyReactionProperties(pReaction, true);

  return pReaction;
}

enum class ReactionVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added probability

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleEventReactionFactory::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEventType;

  // Version 2
  stream << m_uiProbability;
}


void xiiParticleEventReactionFactory::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEventType;

  if (uiVersion >= 2)
  {
    stream >> m_uiProbability;
  }
}

//////////////////////////////////////////////////////////////////////////

xiiParticleEventReaction::xiiParticleEventReaction()  = default;
xiiParticleEventReaction::~xiiParticleEventReaction() = default;

void xiiParticleEventReaction::Reset(xiiParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction);
