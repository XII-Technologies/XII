#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenVRPlugin/Basics.h>

XII_STATICLINK_LIBRARY(OpenVRPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRSingleton);
  XII_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRStartup);
}

XII_DYNAMIC_PLUGIN_IMPLEMENTATION(XII_OPENVRPLUGIN_DLL, xiiOpenVRPlugin);
