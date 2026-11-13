/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/Components/MuJoCoJointComponent.h>
#include <MujocoPlugin/Components/MuJoCoModelComponent.h>
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
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiMuJoCoActuatorComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiMuJoCoJointComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiMuJoCoBodyComponent>());
        pWorld->DeleteModule(xiiGetStaticRTTI<xiiMuJoCoModelComponent>());
        pWorld->DeleteModule<xiiMuJoCoPhysicsWorldModule>();
      }
    }
    xiiMuJoCoModelResource::CleanupDynamicPluginReferences();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Startup);
