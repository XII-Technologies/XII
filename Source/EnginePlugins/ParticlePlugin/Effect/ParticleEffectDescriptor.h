#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEffectDescriptor final : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEffectDescriptor, xiiReflectedClass);

public:
  xiiParticleEffectDescriptor();
  ~xiiParticleEffectDescriptor();

  void                                                   AddParticleSystem(xiiParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void                                                   RemoveParticleSystem(xiiParticleSystemDescriptor* pSystem) { m_ParticleSystems.RemoveAndCopy(pSystem); }
  const xiiHybridArray<xiiParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void                                                       AddEventReaction(xiiParticleEventReactionFactory* pSystem) { m_EventReactions.PushBack(pSystem); }
  void                                                       RemoveEventReaction(xiiParticleEventReactionFactory* pSystem) { m_EventReactions.RemoveAndCopy(pSystem); }
  const xiiHybridArray<xiiParticleEventReactionFactory*, 4>& GetEventReactions() const { return m_EventReactions; }


  void Save(xiiStreamWriter& inout_stream) const;
  void Load(xiiStreamReader& inout_stream);

  void ClearSystems();
  void ClearEventReactions();

  xiiEnum<xiiEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  bool                                  m_bSimulateInLocalSpace  = false;
  bool                                  m_bAlwaysShared          = false;
  float                                 m_fApplyInstanceVelocity = 0.0f;
  xiiTime                               m_PreSimulateDuration;
  xiiMap<xiiString, float>              m_FloatParameters;
  xiiMap<xiiString, xiiColor>           m_ColorParameters;

private:
  xiiHybridArray<xiiParticleSystemDescriptor*, 4>     m_ParticleSystems;
  xiiHybridArray<xiiParticleEventReactionFactory*, 4> m_EventReactions;
};
