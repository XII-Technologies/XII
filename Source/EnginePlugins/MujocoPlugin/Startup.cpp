/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Foundation/Configuration/Startup.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Mujoco, MujocoPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    // Register resources and initialize global state for the Mujoco physics engine.
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // Unregister resources and clean up global state for the Mujoco physics engine.
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Startup);
