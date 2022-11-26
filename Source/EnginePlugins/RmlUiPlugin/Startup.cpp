#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static xiiRmlUiResourceLoader s_RmlUiResourceLoader;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RmlUi, RmlUiPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {    
  }

  ON_CORESYSTEMS_SHUTDOWN
  {    
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiResourceManager::SetResourceTypeLoader<xiiRmlUiResource>(&s_RmlUiResourceLoader);

    xiiResourceManager::RegisterResourceForAssetType("RmlUi", xiiGetStaticRTTI<xiiRmlUiResource>());

    {
      xiiRmlUiResourceDescriptor desc;
      xiiRmlUiResourceHandle hResource = xiiResourceManager::CreateResource<xiiRmlUiResource>("RmlUiMissing", std::move(desc), "Fallback for missing rml ui resource");
      xiiResourceManager::SetResourceTypeMissingFallback<xiiRmlUiResource>(hResource);
    }

    if (xiiRmlUi::GetSingleton() == nullptr)
    {
      XII_DEFAULT_NEW(xiiRmlUi);
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (xiiRmlUi* pRmlUi = xiiRmlUi::GetSingleton())
    {
      XII_DEFAULT_DELETE(pRmlUi);
    }

    xiiResourceManager::SetResourceTypeLoader<xiiRmlUiResource>(nullptr);

    xiiRmlUiResource::CleanupDynamicPluginReferences();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on
