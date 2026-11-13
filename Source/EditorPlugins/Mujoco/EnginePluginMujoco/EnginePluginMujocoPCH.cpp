/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginMujoco/EnginePluginMujocoPCH.h>

XII_STATICLINK_LIBRARY(EnginePluginMujoco)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(EnginePluginMujoco_Implementation_MuJoCoDebugVisualization);
}
