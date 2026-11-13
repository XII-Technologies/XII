/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

XII_STATICLINK_LIBRARY(JoltPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(JoltPlugin_Startup);
  XII_STATICLINK_REFERENCE(JoltPlugin_System_Implementation_JoltJobSystem);
  XII_STATICLINK_REFERENCE(JoltPlugin_System_Implementation_JoltPhysicsWorldModule);
}
