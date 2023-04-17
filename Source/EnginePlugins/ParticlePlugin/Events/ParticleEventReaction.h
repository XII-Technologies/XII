#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiParticleEffectInstance;
class xiiParticleEventReaction;

/// \brief Base class for all particle event reactions
class XII_PARTICLEPLUGIN_DLL xiiParticleEventReactionFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReactionFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetEventReactionType() const                                                     = 0;
  virtual void           CopyReactionProperties(xiiParticleEventReaction* pObject, bool bFirstTime) const = 0;

  xiiParticleEventReaction* CreateEventReaction(xiiParticleEffectInstance* pOwner) const;

  virtual void Save(xiiStreamWriter& inout_stream) const;
  virtual void Load(xiiStreamReader& inout_stream);

  xiiString m_sEventType;
  xiiUInt8  m_uiProbability = 100;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEventReaction : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReaction, xiiReflectedClass);

  friend class xiiParticleEventReactionFactory;
  friend class xiiParticleEffectInstance;

protected:
  xiiParticleEventReaction();
  ~xiiParticleEventReaction();

  void Reset(xiiParticleEffectInstance* pOwner);

  virtual void ProcessEvent(const xiiParticleEvent& e) = 0;

  xiiTempHashedString        m_sEventName;
  xiiUInt8                   m_uiProbability;
  xiiParticleEffectInstance* m_pOwnerEffect = nullptr;
};
