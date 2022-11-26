#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
}

void OnUnloadPlugin() {}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
