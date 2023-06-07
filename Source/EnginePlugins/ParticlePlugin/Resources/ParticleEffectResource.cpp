#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEffectResource, 1, xiiRTTIDefaultAllocator<xiiParticleEffectResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiParticleEffectResource);
// clang-format on

xiiParticleEffectResource::xiiParticleEffectResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiParticleEffectResource::~xiiParticleEffectResource() = default;

xiiResourceLoadDesc xiiParticleEffectResource::UnloadData(Unload WhatToUnload)
{
  /// \todo Clear something
  // m_Desc.m_System1

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiParticleEffectResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Desc.Load(*Stream);

  return res;
}

void xiiParticleEffectResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  /// \todo Better statistics
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiParticleEffectResource) + sizeof(xiiParticleEffectResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiParticleEffectResource, xiiParticleEffectResourceDescriptor)
{
  m_Desc = descriptor;

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  return res;
}

void xiiParticleEffectResourceDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  m_Effect.Save(inout_stream);
}

void xiiParticleEffectResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  m_Effect.Load(inout_stream);
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Resources_ParticleEffectResource);
