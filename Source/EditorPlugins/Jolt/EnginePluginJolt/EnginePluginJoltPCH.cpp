/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginJolt/EnginePluginJoltPCH.h>

XII_STATICLINK_LIBRARY(EnginePluginJolt)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(EnginePluginJolt_Implementation_JoltDebugVisualization);
  XII_STATICLINK_REFERENCE(EnginePluginJolt_Implementation_JoltAuthoring);
}