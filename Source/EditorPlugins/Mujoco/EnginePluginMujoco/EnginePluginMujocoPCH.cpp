/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginMujoco/EnginePluginMujocoPCH.h>
#include <Foundation/Configuration/Plugin.h>

XII_PLUGIN_DEPENDENCY(xiiMujocoPlugin);
XII_PLUGIN_DEPENDENCY(xiiEnginePluginScene);

XII_STATICLINK_LIBRARY(EnginePluginMujoco)
{
  if (bReturn)
  {
    return;
  }

  XII_STATICLINK_REFERENCE(EnginePluginMujoco_Implementation_MuJoCoDebugVisualization);
  XII_STATICLINK_REFERENCE(EnginePluginMujoco_Implementation_MuJoCoInspectorMessages);
}
