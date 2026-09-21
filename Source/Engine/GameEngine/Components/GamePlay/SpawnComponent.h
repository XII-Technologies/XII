/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct xiiMsgComponentInternalTrigger;

struct xiiSpawnComponentFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None              = 0,
    SpawnAtStart      = XII_BIT(0), ///< The component will schedule a spawn once at creation time
    SpawnContinuously = XII_BIT(1), ///< Every time a scheduled spawn was done, a new one is scheduled
    AttachAsChild     = XII_BIT(2), ///< All objects spawned will be attached as children to this node
    SpawnInFlight     = XII_BIT(3), ///< [internal] A spawn trigger message has been posted.

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
    StorageType SpawnContinuously : 1;
    StorageType AttachAsChild : 1;
    StorageType SpawnInFlight : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiSpawnComponentFlags);

using xiiSpawnComponentManager = xiiComponentManager<class xiiSpawnComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiSpawnComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpawnComponent, xiiComponent, xiiSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSpawnComponent

public:
  xiiSpawnComponent();
  ~xiiSpawnComponent();

  /// Checks whether the last spawn time was long enough ago that a call to TriggerManualSpawn() would succeed.
  bool CanTriggerManualSpawn() const; // [ scriptable ]

  /// Spawns a new object, unless the minimum spawn delay has not been reached between calls to this function.
  ///
  /// Manual spawns and continuous (scheduled) spawns are independent from each other regarding minimum spawn delays.
  /// If this function is called in too short intervals, it is ignored and false is returned.
  /// Returns true, if an object was spawned.
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const xiiVec3& vLocalOffset = xiiVec3::MakeZero()); // [ scriptable ]

  /// Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  bool GetSpawnAtStart() const; // [ property ]
  void SetSpawnAtStart(bool b); // [ property ]

  bool GetSpawnContinuously() const; // [ property ]
  void SetSpawnContinuously(bool b); // [ property ]

  bool GetAttachAsChild() const; // [ property ]
  void SetAttachAsChild(bool b); // [ property ]

  xiiPrefabResourceHandle m_hPrefab; // [ property ]

  /// The minimum delay between spawning objects. This is also enforced for manually spawning things.
  xiiTime m_MinDelay; // [ property ]

  /// For scheduled spawns (continuous / at start) this is an additional random range on top of the minimum spawn delay.
  xiiTime m_DelayRange; // [ property ]

  /// The spawned object's orientation may deviate by this amount around the X axis. 180° is completely random orientation.
  xiiAngle m_MaxDeviation; // [ property ]

  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;                                         // [ property ] (exposed parameter)
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                         RemoveParameter(xiiStringView sKey);                           // [ property ] (exposed parameter)
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

protected:
  xiiBitflags<xiiSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const xiiTransform& tLocalSpawn);
  bool         SpawnOnce(const xiiVec3& vLocalOffset);
  void         OnTriggered(xiiMsgComponentInternalTrigger& msg);

  xiiTime m_LastManualSpawn;
};
