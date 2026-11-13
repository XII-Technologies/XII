/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/World/WorldModule.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

/// Owns the Jolt simulation for one XII world. Access requires the world's write marker.
class XII_JOLTPLUGIN_DLL xiiJoltPhysicsWorldModule final : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltPhysicsWorldModule, xiiWorldModule);

public:
  explicit xiiJoltPhysicsWorldModule(xiiWorld* pWorld);
  ~xiiJoltPhysicsWorldModule() override;

  /// Sets the fixed step and per-update catch-up limit. A new configuration resets accumulated time.
  [[nodiscard]] xiiResult SetSimulationStep(xiiTime fixedStep, xiiUInt32 uiMaxCatchUpSteps);

  /// Sets a symmetric relationship between two collision layers. The native world must be empty.
  [[nodiscard]] xiiResult SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled);

private:
  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);

  struct NativeWorld;
  xiiUniquePtr<NativeWorld> m_pNativeWorld;
  xiiTime                   m_FixedStep = xiiTime::MakeFromSeconds(1.0 / 60.0);
  xiiTime                   m_AccumulatedTime;
  xiiUInt32                 m_uiMaxCatchUpSteps = 4;
};
