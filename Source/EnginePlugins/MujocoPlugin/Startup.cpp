/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Mujoco, MujocoPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_SHUTDOWN
  {
    // Plugin changes run with world updates quiesced. Detach delegates while the module code is loaded.
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiWorld::GetWorldCount(); ++uiIndex)
    {
      if (xiiWorld* pWorld = xiiWorld::GetWorld(uiIndex))
      {
        XII_LOCK(pWorld->GetWriteMarker());
        pWorld->DeleteModule<xiiMuJoCoPhysicsWorldModule>();
      }
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Startup);
