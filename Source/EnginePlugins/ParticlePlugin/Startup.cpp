#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceForAssetType("Particle Effect", xiiGetStaticRTTI<xiiParticleEffectResource>());

    xiiParticleEffectResourceDescriptor desc;
    xiiParticleEffectResourceHandle hEffect = xiiResourceManager::CreateResource<xiiParticleEffectResource>("ParticleEffectMissing", std::move(desc), "Fallback for missing Particle Effects");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiParticleEffectResource>(hEffect);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Startup);
