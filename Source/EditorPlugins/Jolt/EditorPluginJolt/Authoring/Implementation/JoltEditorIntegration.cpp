/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Authoring/JoltComponentAdapters.h>
#include <EditorPluginJolt/Authoring/JoltEditorActions.h>

XII_PLUGIN_DEPENDENCY(xiiEditorPluginScene);
XII_PLUGIN_DEPENDENCY(xiiEnginePluginJolt);

XII_PLUGIN_ON_LOADED()
{
  xiiRegisterJoltComponentAdapters();
  xiiRegisterJoltEditorActions();
}
XII_PLUGIN_ON_UNLOADED()
{
  xiiUnregisterJoltComponentAdapters();
  xiiUnregisterJoltEditorActions();
}

XII_STATICLINK_FILE(EditorPluginJolt, EditorPluginJolt_Authoring_Implementation_JoltEditorIntegration);
