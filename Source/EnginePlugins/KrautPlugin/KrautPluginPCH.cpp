#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/KrautDeclarations.h>

#include <Foundation/Configuration/Startup.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Kraut, KrautPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::RegisterResourceForAssetType("Kraut Tree", xiiGetStaticRTTI<xiiKrautGeneratorResource>());

    xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<xiiKrautTreeResource, xiiMaterialResource>();
    xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<xiiKrautTreeResource, xiiMeshResource>();

    {
      xiiKrautTreeResourceDescriptor desc;
      desc.m_Details.m_Bounds.SetInvalid();

        xiiKrautTreeResourceHandle hResource = xiiResourceManager::CreateResource<xiiKrautTreeResource>("Missing Kraut Tree Mesh", std::move(desc), "Empty Kraut Tree Mesh");
      xiiResourceManager::SetResourceTypeMissingFallback<xiiKrautTreeResource>(hResource);
    }

    //{
    //  xiiKrautGeneratorResourceHandle hResource = xiiResourceManager::LoadResource<xiiKrautGeneratorResource>("Kraut/KrautFallback.tree");
    //  xiiResourceManager::SetResourceTypeMissingFallback<xiiKrautGeneratorResource>(hResource);
    //}
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeMissingFallback<xiiKrautTreeResource>(xiiKrautTreeResourceHandle());
    xiiResourceManager::SetResourceTypeMissingFallback<xiiKrautGeneratorResource>(xiiKrautGeneratorResourceHandle());

    xiiKrautTreeResource::CleanupDynamicPluginReferences();
    xiiKrautGeneratorResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on
