#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;

struct XII_PARTICLEPLUGIN_DLL xiiParticleEffectResourceDescriptor
{
  virtual void Save(xiiStreamWriter& inout_stream) const;
  virtual void Load(xiiStreamReader& inout_stream);

  xiiParticleEffectDescriptor m_Effect;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEffectResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEffectResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiParticleEffectResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiParticleEffectResource, xiiParticleEffectResourceDescriptor);

public:
  xiiParticleEffectResource();
  ~xiiParticleEffectResource();

  const xiiParticleEffectResourceDescriptor& GetDescriptor() { return m_Desc; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiParticleEffectResourceDescriptor m_Desc;
};
