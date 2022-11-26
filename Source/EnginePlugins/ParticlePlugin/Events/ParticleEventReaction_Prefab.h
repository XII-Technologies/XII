#pragma once

#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

using xiiPrefabResourceHandle = xiiTypedResourceHandle<class xiiPrefabResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleEventReactionFactory_Prefab final : public xiiParticleEventReactionFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReactionFactory_Prefab, xiiParticleEventReactionFactory);

public:
  xiiParticleEventReactionFactory_Prefab();

  virtual const xiiRTTI* GetEventReactionType() const override;
  virtual void           CopyReactionProperties(xiiParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  xiiString                               m_sPrefab;
  xiiEnum<xiiSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  // const xiiRangeView<const char*, xiiUInt32> GetParameters() const;
  // void SetParameter(const char* szKey, const xiiVariant& value);
  // void RemoveParameter(const char* szKey);
  // bool GetParameter(const char* szKey, xiiVariant& out_value) const;

private:
  // xiiSharedPtr<xiiParticlePrefabParameters> m_Parameters;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEventReaction_Prefab final : public xiiParticleEventReaction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEventReaction_Prefab, xiiParticleEventReaction);

public:
  xiiParticleEventReaction_Prefab();
  ~xiiParticleEventReaction_Prefab();

  xiiPrefabResourceHandle                 m_hPrefab;
  xiiEnum<xiiSurfaceInteractionAlignment> m_Alignment;

  // xiiSharedPtr<xiiParticlePrefabParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const xiiParticleEvent& e) override;
};
