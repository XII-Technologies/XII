/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/WorldModule.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

/// Owns the compiled MuJoCo model and simulation state for one XII world.
/// Access requires the world's write marker. Clearing the world releases the model and state.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoPhysicsWorldModule final : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoPhysicsWorldModule, xiiWorldModule);

public:
  explicit xiiMuJoCoPhysicsWorldModule(xiiWorld* pWorld);
  ~xiiMuJoCoPhysicsWorldModule() override;

  /// Loads an MJCF (.xml or .mjcf) or MJB (.mjb) from a loose file. XII paths are resolved to native paths.
  /// A failed load preserves the active simulation.
  [[nodiscard]] xiiResult LoadModel(xiiStringView sPath);

  /// Loads an MJB produced by the same MuJoCo version. The input is borrowed for the duration of the call.
  /// A failed load preserves the active simulation.
  [[nodiscard]] xiiResult LoadModelFromBuffer(xiiArrayPtr<const xiiUInt8> modelBytes);

  /// Restores the active model's initial state and evaluates it at time zero.
  void ResetSimulation();

  /// Sets the maximum number of fixed steps per world update. The limit must be positive.
  [[nodiscard]] xiiResult SetMaxCatchUpSteps(xiiUInt32 uiMaxCatchUpSteps);

private:
  friend class xiiMuJoCoModelComponent;
  friend class xiiMuJoCoBodyComponent;

  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);
  void RequestModel(xiiComponentHandle hOwner, const xiiMuJoCoModelResourceHandle& hModel);
  void ReleaseModel(xiiComponentHandle hOwner);
  void UpdateRequestedModel();
  void ClearSimulation();

  void RegisterBody(xiiComponentHandle hComponent, const xiiString& sBodyName);
  void UnregisterBody(xiiComponentHandle hComponent);

  struct BodyBinding
  {
    xiiComponentHandle  m_hComponent;
    xiiGameObjectHandle m_hObject;
    xiiString           m_sBodyName;
    xiiInt32            m_iBodyId = -1;
    xiiUInt32           m_uiDepth = 0;
  };

  void ResolveBody(BodyBinding& ref_binding) const;
  void SynchronizeBodies();

  struct NativeSimulation;
  [[nodiscard]] xiiResult InstallModel(xiiSharedPtr<xiiMuJoCoCompiledModel> pModel, xiiStringView sSource);

  xiiUniquePtr<NativeSimulation> m_pSimulation;
  xiiMuJoCoModelResourceHandle   m_hRequestedModel;
  const xiiMuJoCoModelResource*  m_pRequestedResource = nullptr; ///< Kept alive by m_hRequestedModel.
  xiiComponentHandle             m_hModelOwner;
  xiiDynamicArray<BodyBinding>   m_BodyBindings;
  xiiTime                        m_AccumulatedTime;
  xiiUInt32                      m_uiMaxCatchUpSteps   = 4;
  xiiUInt32                      m_uiAttemptedRevision = 0;
  bool                           m_bAttemptedRequest   = false;
  bool                           m_bSyncBodies         = false;
};
