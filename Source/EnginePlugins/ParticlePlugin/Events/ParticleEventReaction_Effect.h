#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEventReactionFactory_Effect final : public xiiParticleEventReactionFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReactionFactory_Effect, xiiParticleEventReactionFactory);

public:
  xiiParticleEventReactionFactory_Effect();

  virtual const xiiRTTI* GetEventReactionType() const override;
  virtual void           CopyReactionProperties(xiiParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  xiiString                               m_sEffect;
  xiiEnum<xiiSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;
  void                                       SetParameter(const char* szKey, const xiiVariant& value);
  void                                       RemoveParameter(const char* szKey);
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const;

private:
  xiiSharedPtr<xiiParticleEffectParameters> m_pParameters;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEventReaction_Effect final : public xiiParticleEventReaction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReaction_Effect, xiiParticleEventReaction);

public:
  xiiParticleEventReaction_Effect();
  ~xiiParticleEventReaction_Effect();

  xiiParticleEffectResourceHandle           m_hEffect;
  xiiEnum<xiiSurfaceInteractionAlignment>   m_Alignment;
  xiiSharedPtr<xiiParticleEffectParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const xiiParticleEvent& e) override;
};
