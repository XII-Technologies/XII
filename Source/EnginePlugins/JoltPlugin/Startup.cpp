#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCore.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Triangle", xiiGetStaticRTTI<xiiJoltMeshResource>());
    xiiResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Convex", xiiGetStaticRTTI<xiiJoltMeshResource>());

    xiiJoltMeshResourceDescriptor desc;
    xiiJoltMeshResourceHandle hResource = xiiResourceManager::CreateResource<xiiJoltMeshResource>("Missing Jolt Mesh", std::move(desc), "Empty collision mesh");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiJoltMeshResource>(hResource);

    xiiJoltCore::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeMissingFallback<xiiJoltMeshResource>(xiiJoltMeshResourceHandle());
    xiiJoltCore::Shutdown();

    xiiJoltMeshResource::CleanupDynamicPluginReferences();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on
