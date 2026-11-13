/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <MujocoPlugin/Resources/Implementation/MuJoCoCompiledModel.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

#include <memory>

struct xiiMuJoCoPhysicsWorldModule::NativeSimulation
{
  struct BodyForce
  {
    xiiInt32 m_iBody;
    xiiVec3  m_vForce;
    xiiVec3  m_vTorque;
    xiiVec3  m_vPosition;
    bool     m_bAtPosition;
  };

  // Members are destroyed in reverse order so mjData is released first.
  xiiSharedPtr<xiiMuJoCoCompiledModel>              m_pModel;
  std::unique_ptr<mjData, decltype(&mj_deleteData)> m_pData{nullptr, mj_deleteData};
  xiiDynamicArray<mjtNum>                           m_PendingControls;
  xiiDynamicArray<mjtNum>                           m_NeutralControls;
  xiiDynamicArray<xiiUInt8>                         m_ControlPending;
  xiiDynamicArray<BodyForce>                        m_BodyForces;
  struct SceneBinding
  {
    xiiGameObjectHandle m_hObject;
    xiiComponentHandle  m_hComponent;
  };
  xiiDynamicArray<SceneBinding> m_BodySceneBindings;
  xiiUInt64                     m_uiStateRevision = 1;
  xiiUInt64                     m_uiStepIndex     = 0;
  xiiMuJoCoStepDiagnostics      m_StepDiagnostics;
  bool                          m_WarningsReported[mjNWARNING] = {};
  bool                          m_bObservationsDirty           = false;
  bool                          m_bStateDirty                  = false;
  bool                          m_bControlsPending             = false;
  bool                          m_bExternalForcesApplied       = false;
};
