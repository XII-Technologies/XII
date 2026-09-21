/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

struct xiiMsgComponentInternalTrigger;

struct xiiSpawnBoxComponentFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None              = 0,
    SpawnAtStart      = XII_BIT(0), ///< The component will schedule a spawn once at creation time
    SpawnContinuously = XII_BIT(1), ///< Every time a spawn duration has finished, a new one is started

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiSpawnBoxComponentFlags);

using xiiSpawnBoxComponentManager = xiiComponentManager<class xiiSpawnBoxComponent, xiiBlockStorageType::Compact>;

/// This component spawns prefabs inside a box.
///
/// The prefabs are spawned over a fixed duration.
/// The number of prefabs to spawn over the time duration is randomly chosen.
/// Each prefab may get rotated around the Z axis and tilted away from the Z axis.
/// If desired, the component can start spawning automatically, or it can be (re-)started from code.
/// If 'spawn continuously' is enabled, the component restarts itself after the spawn duration is over,
/// thus for every spawn duration the number of prefabs to spawn gets reevaluated.
class xiiSpawnBoxComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpawnBoxComponent, xiiComponent, xiiSpawnBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSpawnBoxComponent

public:
  /// When called, the component starts spawning the chosen number of prefabs over the set duration.
  ///
  /// If this is called while the component is already active, the internal state is reset and it starts over.
  void StartSpawning(); // [ scriptable ]

  void           SetHalfExtents(const xiiVec3& value);             // [ property ]
  const xiiVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  bool GetSpawnAtStart() const; // [ property ]
  void SetSpawnAtStart(bool b); // [ property ]

  bool GetSpawnContinuously() const; // [ property ]
  void SetSpawnContinuously(bool b); // [ property ]

  xiiTime                 m_SpawnDuration;         // [ property ]
  xiiUInt16               m_uiMinSpawnCount   = 5; // [ property ]
  xiiUInt16               m_uiSpawnCountRange = 5; // [ property ]
  xiiPrefabResourceHandle m_hPrefab;               // [ property ]

  /// The spawned object's forward direction may deviate this amount from the spawn box's forward rotation. This is accomplished by rotating around the Z axis.
  xiiAngle m_MaxRotationZ; // [ property ]

  /// The spawned object's Z (up) axis may deviate by this amount from the spawn box's Z axis.
  xiiAngle m_MaxTiltZ; // [ property ]


private:
  void OnTriggered(xiiMsgComponentInternalTrigger& msg);
  void Spawn(xiiUInt32 uiCount);
  void InternalStartSpawning(bool bFirstTime);

  xiiUInt16                              m_uiSpawned      = 0;
  xiiUInt16                              m_uiTotalToSpawn = 0;
  xiiTime                                m_StartTime;
  xiiBitflags<xiiSpawnBoxComponentFlags> m_Flags;
  xiiVec3                                m_vHalfExtents = xiiVec3(0.5f);
};
