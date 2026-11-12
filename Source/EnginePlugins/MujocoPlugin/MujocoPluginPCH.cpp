/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

XII_STATICLINK_LIBRARY(MujocoPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(MujocoPlugin_Startup);
}
