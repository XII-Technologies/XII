#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ProcGen, ProcGenPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceForAssetType("ProcGen Graph", xiiGetStaticRTTI<xiiProcGenGraphResource>());

    xiiProcGenGraphResourceDescriptor desc;
    xiiProcGenGraphResourceHandle hResource = xiiResourceManager::CreateResource<xiiProcGenGraphResource>("ProcGenGraphMissing", std::move(desc), "Fallback for missing ProcGen Graph Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiProcGenGraphResource>(hResource);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiProcGenGraphResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on
