/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <InspectorPlugin/InspectorPluginPCH.h>

XII_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(InspectorPlugin_InspectorPlugin);
  XII_STATICLINK_REFERENCE(InspectorPlugin_Startup);
}
