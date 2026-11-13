/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/WorldModule.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>

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
  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);

  struct NativeSimulation;
  [[nodiscard]] xiiResult InstallModel(xiiUniquePtr<NativeSimulation> pSimulation, xiiStringView sSource);

  xiiUniquePtr<NativeSimulation> m_pSimulation;
  xiiTime                        m_AccumulatedTime;
  xiiUInt32                      m_uiMaxCatchUpSteps = 4;
};
