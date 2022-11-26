#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

XII_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(RenderDocPlugin_RenderDocSingleton);
}
