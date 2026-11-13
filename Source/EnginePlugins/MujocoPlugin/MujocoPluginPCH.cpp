/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

XII_STATICLINK_LIBRARY(MujocoPlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(MujocoPlugin_Startup);
  XII_STATICLINK_REFERENCE(MujocoPlugin_Components_Implementation_MuJoCoActuatorComponent);
  XII_STATICLINK_REFERENCE(MujocoPlugin_Components_Implementation_MuJoCoBodyComponent);
  XII_STATICLINK_REFERENCE(MujocoPlugin_Components_Implementation_MuJoCoJointComponent);
  XII_STATICLINK_REFERENCE(MujocoPlugin_Components_Implementation_MuJoCoModelComponent);
  XII_STATICLINK_REFERENCE(MujocoPlugin_Resources_Implementation_MuJoCoModelResource);
  XII_STATICLINK_REFERENCE(MujocoPlugin_System_Implementation_MuJoCoPhysicsWorldModule);
  XII_STATICLINK_REFERENCE(MujocoPlugin_System_Implementation_MuJoCoState);
}
