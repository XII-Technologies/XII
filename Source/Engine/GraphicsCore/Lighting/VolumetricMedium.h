/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Shape used by a bounded participating medium.
struct XII_GRAPHICSCORE_DLL xiiVolumetricMediumShape
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Sphere,
    Box,

    Default = Box
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricMediumShape);

/// Generation-checked handle to subsystem-owned volumetric medium state.
struct XII_GRAPHICSCORE_DLL xiiVolumetricMediumHandle
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;

  [[nodiscard]] bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }
  [[nodiscard]] bool operator==(const xiiVolumetricMediumHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiGeneration == rhs.m_uiGeneration; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricMediumHandle);

/// Authoring data for smoke, fog, steam, dust, or another bounded participating medium.
/// Coefficients use inverse metres. Scattering plus absorption is the extinction coefficient.
struct XII_GRAPHICSCORE_DLL xiiVolumetricMediumDescription
{
  xiiEnum<xiiVolumetricMediumShape> m_Shape = xiiVolumetricMediumShape::Box;
  xiiVec3                           m_vCenter = xiiVec3::MakeZero();
  xiiQuat                           m_qRotation = xiiQuat::MakeIdentity();
  xiiVec3                           m_vHalfExtents = xiiVec3(1.0f);
  xiiVec3                           m_vScattering = xiiVec3(0.08f);
  xiiVec3                           m_vAbsorption = xiiVec3(0.02f);
  xiiVec3                           m_vEmission = xiiVec3::MakeZero();
  float                             m_fAnisotropy = 0.0f;
  xiiInt32                          m_iPriority = 0;
  bool                              m_bEnabled = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricMediumDescription);

/// Configuration of the sparse world-cell hierarchy used for medium streaming.
struct XII_GRAPHICSCORE_DLL xiiVolumetricMediumSettings
{
  float     m_fCellSizeMeters       = 64.0f;
  float     m_fViewDistanceMeters   = 512.0f;
  xiiUInt32 m_uiMaxVisibleMedia     = 32U;
  xiiUInt32 m_uiMaxCellsPerMedium   = 4096U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricMediumSettings);

/// GPU representation consumed by froxel construction.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVolumetricMedium
{
  XII_DECLARE_POD_TYPE();

  xiiVec4 m_vCenterAndShape;
  xiiVec4 m_vRotation;
  xiiVec4 m_vHalfExtentsAndAnisotropy;
  xiiVec4 m_vScatteringAndPriority;
  xiiVec4 m_vAbsorptionAndPadding;
  xiiVec4 m_vEmissionAndPadding;
};

static_assert(sizeof(xiiGpuVolumetricMedium) == 96U);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVolumetricMedium);

using xiiGpuVolumetricMediumArray = xiiDynamicArray<xiiGpuVolumetricMedium, xiiAlignedAllocatorWrapper>;

struct XII_GRAPHICSCORE_DLL xiiVolumetricMediumStats
{
  xiiUInt32 m_uiRegisteredMedia = 0U;
  xiiUInt32 m_uiResidentCells   = 0U;
  xiiUInt32 m_uiStreamedOutCells = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVolumetricMediumStats);

/// Subsystem-owned sparse hierarchy of bounded participating media.
///
/// World streaming marks cells resident or non-resident without invalidating stable medium handles.
/// Per-view queries visit only overlapping resident cells, deduplicate volumes spanning multiple
/// cells, and return a priority/distance sorted GPU working set.
class XII_GRAPHICSCORE_DLL xiiVolumetricMediumManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVolumetricMediumManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, VolumetricMediumManager);

public:
  xiiVolumetricMediumManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static xiiResult Configure(const xiiVolumetricMediumSettings& settings);
  [[nodiscard]] static const xiiVolumetricMediumSettings& GetConfiguration();

  [[nodiscard]] static xiiVolumetricMediumHandle RegisterMedium(const xiiVolumetricMediumDescription& description);
  static void                                          UnregisterMedium(xiiVolumetricMediumHandle handle);
  [[nodiscard]] static xiiResult                       UpdateMedium(xiiVolumetricMediumHandle handle, const xiiVolumetricMediumDescription& description);
  [[nodiscard]] static bool                            IsValid(xiiVolumetricMediumHandle handle);

  /// Changes streaming residency for one spatial cell. Medium handles remain valid while streamed out.
  static void SetCellResident(const xiiVec3I32& vCell, bool bResident);
  [[nodiscard]] static bool IsCellResident(const xiiVec3I32& vCell);

  /// Builds the bounded GPU working set for a view. The result is deterministic for identical state.
  static void GatherGpuMedia(const xiiVec3& vViewPosition, xiiGpuVolumetricMediumArray& out_media);
  [[nodiscard]] static xiiVolumetricMediumStats GetStats();

  [[nodiscard]] static xiiUInt64 PackCellKey(const xiiVec3I32& vCell);
  static void                    UnpackCellKey(xiiUInt64 uiKey, xiiVec3I32& out_vCell);

private:
  static void Startup();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};
