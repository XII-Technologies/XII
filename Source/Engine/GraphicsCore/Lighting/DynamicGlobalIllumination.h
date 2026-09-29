/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Runtime controls for the camera-centred mid-field DDGI clipmap.
struct XII_GRAPHICSCORE_DLL xiiDDGISettings
{
  xiiUInt32 m_uiProbeCountX = 24U;
  xiiUInt32 m_uiProbeCountY = 12U;
  xiiUInt32 m_uiProbeCountZ = 24U;
  float     m_fProbeSpacing = 4.0f;
  xiiUInt32 m_uiProbeUpdateBudget = 128U;
  float     m_fTemporalHysteresis = 0.97f;
  float     m_fMaximumRelocationDistance = 1.8f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGISettings);

struct XII_GRAPHICSCORE_DLL xiiDDGIProbeFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None          = 0U,
    Valid         = XII_BIT(0),
    NeedsUpdate   = XII_BIT(1),
    Relocated     = XII_BIT(2),
    InsideGeometry = XII_BIT(3),

    Default = None
  };

  struct Bits
  {
    StorageType Valid : 1;
    StorageType NeedsUpdate : 1;
    StorageType Relocated : 1;
    StorageType InsideGeometry : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDDGIProbeFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeFlags);

/// Tool-facing state of one physical slot in the scrolling DDGI volume.
struct XII_GRAPHICSCORE_DLL xiiDDGIProbeState
{
  xiiInt32 m_iCellX = xiiMath::MaxValue<xiiInt32>();
  xiiInt32 m_iCellY = xiiMath::MaxValue<xiiInt32>();
  xiiInt32 m_iCellZ = xiiMath::MaxValue<xiiInt32>();
  xiiVec3  m_vWorldPosition = xiiVec3::MakeZero();
  xiiVec3  m_vRelocationOffset = xiiVec3::MakeZero();
  xiiUInt64 m_uiLastUpdatedFrame = 0U;
  xiiBitflags<xiiDDGIProbeFlags> m_Flags = xiiDDGIProbeFlags::NeedsUpdate;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeState);

/// One probe selected for ray tracing and temporal integration this frame.
struct XII_GRAPHICSCORE_DLL xiiDDGIProbeUpdate
{
  xiiUInt32 m_uiPhysicalProbe = xiiInvalidIndex;
  xiiVec3   m_vWorldPosition = xiiVec3::MakeZero();
  float     m_fHistoryWeight = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeUpdate);

struct XII_GRAPHICSCORE_DLL xiiDDGIFrameStats
{
  xiiUInt32 m_uiProbeCount = 0U;
  xiiUInt32 m_uiInvalidProbeCount = 0U;
  xiiUInt32 m_uiScheduledUpdateCount = 0U;
  xiiUInt32 m_uiScrolledProbeCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIFrameStats);

/// Process-wide DDGI clipmap scheduler.
///
/// The state is allocated during GraphicsCore subsystem startup, never from a
/// global constructor. Logical world cells map to stable toroidal physical
/// slots, preserving temporal history for the overlapping region when the
/// camera moves and invalidating only newly exposed probe slabs.
class XII_GRAPHICSCORE_DLL xiiDDGIManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDDGIManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, DDGIManager);

public:
  xiiDDGIManager() = delete;

  [[nodiscard]] static xiiResult Configure(const xiiDDGISettings& settings);
  [[nodiscard]] static bool IsInitialized();

  static void BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex);
  static void CommitProbeUpdate(xiiUInt32 uiPhysicalProbe, const xiiVec3& vRelocationOffset, bool bValid, bool bInsideGeometry = false);

  [[nodiscard]] static xiiArrayPtr<const xiiDDGIProbeState> GetProbes();
  [[nodiscard]] static xiiArrayPtr<const xiiDDGIProbeUpdate> GetScheduledUpdates();
  [[nodiscard]] static xiiDDGIFrameStats GetFrameStats();
  [[nodiscard]] static const xiiDDGISettings& GetConfiguration();

private:
  static void Startup();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};

