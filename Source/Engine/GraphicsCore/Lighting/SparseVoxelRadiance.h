/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Configuration for the camera-centred far-field radiance clipmap.
struct XII_GRAPHICSCORE_DLL xiiSparseVoxelRadianceSettings
{
  xiiUInt32 m_uiClipmapLevels          = 5U;
  xiiUInt32 m_uiClipmapBrickResolution = 8U;
  xiiUInt32 m_uiBrickVoxelResolution   = 8U;
  xiiUInt32 m_uiMaxResidentBricks      = 4096U;
  xiiUInt32 m_uiBrickUpdateBudget      = 64U;
  xiiUInt32 m_uiRefreshIntervalFrames  = 120U;
  float     m_fBaseVoxelSize           = 0.5f;
  float     m_fTemporalHysteresis      = 0.95f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSparseVoxelRadianceSettings);

struct XII_GRAPHICSCORE_DLL xiiSparseVoxelBrickFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None     = 0U,
    Resident = XII_BIT(0),
    Valid    = XII_BIT(1),
    Dirty    = XII_BIT(2),

    Default = None
  };

  struct Bits
  {
    StorageType Resident : 1;
    StorageType Valid : 1;
    StorageType Dirty : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiSparseVoxelBrickFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSparseVoxelBrickFlags);

/// Tool-facing state of one physical brick slot in the sparse radiance pool.
struct XII_GRAPHICSCORE_DLL xiiSparseVoxelBrickState
{
  xiiUInt64                             m_uiPackedKey       = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32                             m_uiPhysicalBrick   = xiiInvalidIndex;
  xiiUInt32                             m_uiClipmapLevel    = 0U;
  xiiVec3I32                            m_vCell             = xiiVec3I32::MakeZero();
  xiiVec3                               m_vWorldMinimum     = xiiVec3::MakeZero();
  float                                 m_fVoxelSize        = 0.0f;
  xiiUInt64                             m_uiLastUsedFrame   = 0U;
  xiiUInt64                             m_uiLastUpdateFrame = 0U;
  xiiBitflags<xiiSparseVoxelBrickFlags> m_Flags;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSparseVoxelBrickState);

/// One brick selected for geometry voxelization and radiance injection.
struct XII_GRAPHICSCORE_DLL xiiSparseVoxelBrickUpdate
{
  xiiUInt64 m_uiPackedKey     = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32 m_uiPhysicalBrick = xiiInvalidIndex;
  xiiUInt32 m_uiClipmapLevel  = 0U;
  xiiVec3   m_vWorldMinimum   = xiiVec3::MakeZero();
  float     m_fVoxelSize      = 0.0f;
  float     m_fHistoryWeight  = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSparseVoxelBrickUpdate);

struct XII_GRAPHICSCORE_DLL xiiSparseVoxelRadianceFrameStats
{
  xiiUInt32 m_uiRequiredBrickCount  = 0U;
  xiiUInt32 m_uiResidentBrickCount  = 0U;
  xiiUInt32 m_uiDirtyBrickCount     = 0U;
  xiiUInt32 m_uiScheduledBrickCount = 0U;
  xiiUInt32 m_uiAllocatedBrickCount = 0U;
  xiiUInt32 m_uiEvictedBrickCount   = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSparseVoxelRadianceFrameStats);

/// Subsystem-owned sparse voxel radiance residency and update scheduler.
///
/// The manager maintains nested camera-centred clipmaps in a fixed physical
/// brick pool. Overlapping world bricks retain their physical slots and
/// temporal history while newly exposed slabs are allocated deterministically.
class XII_GRAPHICSCORE_DLL xiiSparseVoxelRadianceManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSparseVoxelRadianceManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, SparseVoxelRadianceManager);

public:
  xiiSparseVoxelRadianceManager() = delete;

  [[nodiscard]] static xiiResult Configure(const xiiSparseVoxelRadianceSettings& settings);
  [[nodiscard]] static bool      IsInitialized();

  static void BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex);
  static void CommitBrickUpdate(xiiUInt32 uiPhysicalBrick, xiiUInt64 uiPackedKey, bool bValid);

  [[nodiscard]] static xiiArrayPtr<const xiiSparseVoxelBrickState>  GetBricks();
  [[nodiscard]] static xiiArrayPtr<const xiiSparseVoxelBrickUpdate> GetScheduledUpdates();
  [[nodiscard]] static xiiSparseVoxelRadianceFrameStats             GetFrameStats();
  [[nodiscard]] static const xiiSparseVoxelRadianceSettings&        GetConfiguration();
  [[nodiscard]] static const xiiSparseVoxelBrickState*              FindBrick(xiiUInt64 uiPackedKey);

  /// Collision-free key used by CPU residency, GPU page tables, captures, and tools.
  [[nodiscard]] static xiiUInt64 PackBrickKey(xiiUInt32 uiClipmapLevel, const xiiVec3I32& vCell);
  static void                    UnpackBrickKey(xiiUInt64 uiPackedKey, xiiUInt32& out_uiClipmapLevel, xiiVec3I32& out_vCell);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};

