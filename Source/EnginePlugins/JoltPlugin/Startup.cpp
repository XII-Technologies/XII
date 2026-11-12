/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    // Register resources and initialize global state for the Jolt physics engine.
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // Unregister resources and clean up global state for the Jolt physics engine.
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Startup);
