/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>

namespace
{
  constexpr xiiUInt32 s_uiCoordinateBits = 19U;
  constexpr xiiUInt32 s_uiCoordinateBias = XII_BIT(s_uiCoordinateBits - 1U);
  constexpr xiiUInt64 s_uiCoordinateMask = XII_BIT(s_uiCoordinateBits) - 1ULL;
  constexpr xiiUInt32 s_uiLevelShift     = s_uiCoordinateBits * 3U;

  bool IsConfigurationValid(const xiiSparseVoxelRadianceSettings& settings)
  {
    const xiiUInt64 uiRequiredBricks = static_cast<xiiUInt64>(settings.m_uiClipmapBrickResolution) * settings.m_uiClipmapBrickResolution *
                                       settings.m_uiClipmapBrickResolution * settings.m_uiClipmapLevels;
    return settings.m_uiClipmapLevels > 0U && settings.m_uiClipmapLevels <= 8U &&
           settings.m_uiClipmapBrickResolution >= 2U && settings.m_uiClipmapBrickResolution <= 64U &&
           settings.m_uiBrickVoxelResolution >= 2U && settings.m_uiBrickVoxelResolution <= 32U &&
           uiRequiredBricks <= settings.m_uiMaxResidentBricks && settings.m_uiMaxResidentBricks <= 1048576U &&
           settings.m_uiBrickUpdateBudget > 0U && settings.m_uiRefreshIntervalFrames > 0U &&
           xiiMath::IsFinite(settings.m_fBaseVoxelSize) && settings.m_fBaseVoxelSize > 0.0f &&
           xiiMath::IsFinite(settings.m_fTemporalHysteresis) && settings.m_fTemporalHysteresis >= 0.0f && settings.m_fTemporalHysteresis < 1.0f;
  }

  bool IsCoordinateRepresentable(xiiInt32 value)
  {
    return value >= -static_cast<xiiInt32>(s_uiCoordinateBias) && value < static_cast<xiiInt32>(s_uiCoordinateBias);
  }
}

class xiiSparseVoxelRadianceManager::State
{
public:
  xiiSparseVoxelRadianceSettings             m_Settings;
  xiiDynamicArray<xiiSparseVoxelBrickState>  m_Bricks;
  xiiDynamicArray<xiiSparseVoxelBrickUpdate> m_ScheduledUpdates;
  xiiDynamicArray<xiiUInt32>                  m_FreeBricks;
  xiiHashTable<xiiUInt64, xiiUInt32>          m_BrickLookup;
  xiiSparseVoxelRadianceFrameStats            m_Stats;
  xiiUInt64                                   m_uiFrameIndex = 0U;
  bool                                        m_bEngineStarted = false;
  bool                                        m_bInitialized = false;
};

xiiUniquePtr<xiiSparseVoxelRadianceManager::State> xiiSparseVoxelRadianceManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, SparseVoxelRadianceManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiSparseVoxelRadianceManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiSparseVoxelRadianceManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiSparseVoxelRadianceManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiSparseVoxelRadianceManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSparseVoxelRadianceSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSparseVoxelRadianceSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ClipmapLevels", m_uiClipmapLevels)->AddAttributes(new xiiClampValueAttribute(1U, 8U)),
    XII_MEMBER_PROPERTY("ClipmapBrickResolution", m_uiClipmapBrickResolution)->AddAttributes(new xiiClampValueAttribute(2U, 64U)),
    XII_MEMBER_PROPERTY("BrickVoxelResolution", m_uiBrickVoxelResolution)->AddAttributes(new xiiClampValueAttribute(2U, 32U)),
    XII_MEMBER_PROPERTY("MaxResidentBricks", m_uiMaxResidentBricks)->AddAttributes(new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("BrickUpdateBudget", m_uiBrickUpdateBudget)->AddAttributes(new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("RefreshIntervalFrames", m_uiRefreshIntervalFrames)->AddAttributes(new xiiClampValueAttribute(1U, 1000000U)),
    XII_MEMBER_PROPERTY("BaseVoxelSize", m_fBaseVoxelSize)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("TemporalHysteresis", m_fTemporalHysteresis)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.9999f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiSparseVoxelBrickFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiSparseVoxelBrickFlags::Resident, xiiSparseVoxelBrickFlags::Valid, xiiSparseVoxelBrickFlags::Dirty)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSparseVoxelBrickState, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSparseVoxelBrickState>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PackedKey", m_uiPackedKey),
    XII_MEMBER_PROPERTY("PhysicalBrick", m_uiPhysicalBrick),
    XII_MEMBER_PROPERTY("ClipmapLevel", m_uiClipmapLevel),
    XII_MEMBER_PROPERTY("Cell", m_vCell),
    XII_MEMBER_PROPERTY("WorldMinimum", m_vWorldMinimum),
    XII_MEMBER_PROPERTY("VoxelSize", m_fVoxelSize),
    XII_MEMBER_PROPERTY("LastUsedFrame", m_uiLastUsedFrame),
    XII_MEMBER_PROPERTY("LastUpdateFrame", m_uiLastUpdateFrame),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiSparseVoxelBrickFlags, m_Flags),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSparseVoxelBrickUpdate, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSparseVoxelBrickUpdate>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PackedKey", m_uiPackedKey),
    XII_MEMBER_PROPERTY("PhysicalBrick", m_uiPhysicalBrick),
    XII_MEMBER_PROPERTY("ClipmapLevel", m_uiClipmapLevel),
    XII_MEMBER_PROPERTY("WorldMinimum", m_vWorldMinimum),
    XII_MEMBER_PROPERTY("VoxelSize", m_fVoxelSize),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSparseVoxelRadianceFrameStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSparseVoxelRadianceFrameStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequiredBrickCount", m_uiRequiredBrickCount),
    XII_MEMBER_PROPERTY("ResidentBrickCount", m_uiResidentBrickCount),
    XII_MEMBER_PROPERTY("DirtyBrickCount", m_uiDirtyBrickCount),
    XII_MEMBER_PROPERTY("ScheduledBrickCount", m_uiScheduledBrickCount),
    XII_MEMBER_PROPERTY("AllocatedBrickCount", m_uiAllocatedBrickCount),
    XII_MEMBER_PROPERTY("EvictedBrickCount", m_uiEvictedBrickCount),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiSparseVoxelRadianceManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Sparse voxel radiance manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  Configure(xiiSparseVoxelRadianceSettings()).IgnoreResult();
}

void xiiSparseVoxelRadianceManager::EngineStartup()
{
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiSparseVoxelRadianceManager::EngineShutdown()
{
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = false;
}

void xiiSparseVoxelRadianceManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiSparseVoxelRadianceManager::Configure(const xiiSparseVoxelRadianceSettings& settings)
{
  if (s_pState == nullptr || !IsConfigurationValid(settings))
    return XII_FAILURE;

  s_pState->m_Settings = settings;
  s_pState->m_Bricks.Clear();
  s_pState->m_Bricks.SetCount(settings.m_uiMaxResidentBricks);
  s_pState->m_FreeBricks.Clear();
  s_pState->m_FreeBricks.Reserve(settings.m_uiMaxResidentBricks);
  for (xiiUInt32 i = settings.m_uiMaxResidentBricks; i > 0U; --i)
  {
    const xiiUInt32 uiPhysicalBrick = i - 1U;
    s_pState->m_Bricks[uiPhysicalBrick].m_uiPhysicalBrick = uiPhysicalBrick;
    s_pState->m_FreeBricks.PushBack(uiPhysicalBrick);
  }
  s_pState->m_BrickLookup.Clear();
  s_pState->m_BrickLookup.Reserve(settings.m_uiMaxResidentBricks);
  s_pState->m_ScheduledUpdates.Clear();
  s_pState->m_Stats = {};
  s_pState->m_bInitialized = true;
  return XII_SUCCESS;
}

bool xiiSparseVoxelRadianceManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

xiiUInt64 xiiSparseVoxelRadianceManager::PackBrickKey(xiiUInt32 uiClipmapLevel, const xiiVec3I32& vCell)
{
  if (uiClipmapLevel >= 8U || !IsCoordinateRepresentable(vCell.x) || !IsCoordinateRepresentable(vCell.y) || !IsCoordinateRepresentable(vCell.z))
    return xiiMath::MaxValue<xiiUInt64>();

  const xiiUInt64 x = static_cast<xiiUInt32>(vCell.x + static_cast<xiiInt32>(s_uiCoordinateBias));
  const xiiUInt64 y = static_cast<xiiUInt32>(vCell.y + static_cast<xiiInt32>(s_uiCoordinateBias));
  const xiiUInt64 z = static_cast<xiiUInt32>(vCell.z + static_cast<xiiInt32>(s_uiCoordinateBias));
  return x | (y << s_uiCoordinateBits) | (z << (s_uiCoordinateBits * 2U)) | (static_cast<xiiUInt64>(uiClipmapLevel) << s_uiLevelShift);
}

void xiiSparseVoxelRadianceManager::UnpackBrickKey(xiiUInt64 uiPackedKey, xiiUInt32& out_uiClipmapLevel, xiiVec3I32& out_vCell)
{
  out_uiClipmapLevel = static_cast<xiiUInt32>((uiPackedKey >> s_uiLevelShift) & 0x7ULL);
  out_vCell.x = static_cast<xiiInt32>(uiPackedKey & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
  out_vCell.y = static_cast<xiiInt32>((uiPackedKey >> s_uiCoordinateBits) & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
  out_vCell.z = static_cast<xiiInt32>((uiPackedKey >> (s_uiCoordinateBits * 2U)) & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
}

void xiiSparseVoxelRadianceManager::BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized())
    return;

  struct RequiredBrick
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64  m_uiPackedKey;
    xiiUInt32  m_uiClipmapLevel;
    xiiVec3I32 m_vCell;
    xiiVec3    m_vWorldMinimum;
    float      m_fVoxelSize;
    float      m_fDistanceSquared;
  };

  s_pState->m_uiFrameIndex = uiFrameIndex;
  s_pState->m_ScheduledUpdates.Clear();
  s_pState->m_Stats = {};

  const xiiSparseVoxelRadianceSettings& settings = s_pState->m_Settings;
  const xiiUInt32 uiResolution = settings.m_uiClipmapBrickResolution;
  xiiDynamicArray<RequiredBrick> requiredBricks;
  requiredBricks.Reserve(uiResolution * uiResolution * uiResolution * settings.m_uiClipmapLevels);

  for (xiiUInt32 level = 0U; level < settings.m_uiClipmapLevels; ++level)
  {
    const float fVoxelSize = settings.m_fBaseVoxelSize * static_cast<float>(XII_BIT(level));
    const float fBrickWorldSize = fVoxelSize * static_cast<float>(settings.m_uiBrickVoxelResolution);
    const xiiVec3 vCellPosition = vCameraPosition / fBrickWorldSize;
    const xiiVec3I32 vCenterCell(static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.x)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.y)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.z)));
    const xiiVec3I32 vMinimumCell = vCenterCell - xiiVec3I32(static_cast<xiiInt32>(uiResolution / 2U));

    for (xiiUInt32 z = 0U; z < uiResolution; ++z)
    {
      for (xiiUInt32 y = 0U; y < uiResolution; ++y)
      {
        for (xiiUInt32 x = 0U; x < uiResolution; ++x)
        {
          const xiiVec3I32 vCell = vMinimumCell + xiiVec3I32(static_cast<xiiInt32>(x), static_cast<xiiInt32>(y), static_cast<xiiInt32>(z));
          const xiiUInt64 uiPackedKey = PackBrickKey(level, vCell);
          if (uiPackedKey == xiiMath::MaxValue<xiiUInt64>())
            continue;

          RequiredBrick& required = requiredBricks.ExpandAndGetRef();
          required.m_uiPackedKey = uiPackedKey;
          required.m_uiClipmapLevel = level;
          required.m_vCell = vCell;
          required.m_vWorldMinimum = xiiVec3(static_cast<float>(vCell.x), static_cast<float>(vCell.y), static_cast<float>(vCell.z)) * fBrickWorldSize;
          required.m_fVoxelSize = fVoxelSize;
          const xiiVec3 vBrickCenter = required.m_vWorldMinimum + xiiVec3(fBrickWorldSize * 0.5f);
          required.m_fDistanceSquared = (vBrickCenter - vCameraPosition).GetLengthSquared();
        }
      }
    }
  }

  s_pState->m_Stats.m_uiRequiredBrickCount = requiredBricks.GetCount();

  // First mark every retained brick. The allocation pass can then evict only
  // slots outside the new clipmap windows, independent of enumeration order.
  for (const RequiredBrick& required : requiredBricks)
  {
    xiiUInt32 uiPhysicalBrick = xiiInvalidIndex;
    if (s_pState->m_BrickLookup.TryGetValue(required.m_uiPackedKey, uiPhysicalBrick))
      s_pState->m_Bricks[uiPhysicalBrick].m_uiLastUsedFrame = uiFrameIndex;
  }

  struct EvictionCandidate
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPhysicalBrick;
    xiiUInt64 m_uiLastUsedFrame;
  };

  xiiDynamicArray<EvictionCandidate> evictionCandidates;
  evictionCandidates.Reserve(s_pState->m_BrickLookup.GetCount());
  for (xiiUInt32 candidate = 0U; candidate < s_pState->m_Bricks.GetCount(); ++candidate)
  {
    const xiiSparseVoxelBrickState& brick = s_pState->m_Bricks[candidate];
    if (brick.m_Flags.IsSet(xiiSparseVoxelBrickFlags::Resident) && brick.m_uiLastUsedFrame != uiFrameIndex)
      evictionCandidates.PushBack({candidate, brick.m_uiLastUsedFrame});
  }
  evictionCandidates.Sort([](const EvictionCandidate& lhs, const EvictionCandidate& rhs) {
    if (lhs.m_uiLastUsedFrame != rhs.m_uiLastUsedFrame)
      return lhs.m_uiLastUsedFrame < rhs.m_uiLastUsedFrame;
    return lhs.m_uiPhysicalBrick < rhs.m_uiPhysicalBrick;
  });
  xiiUInt32 uiNextEviction = 0U;

  for (const RequiredBrick& required : requiredBricks)
  {
    xiiUInt32 uiPhysicalBrick = xiiInvalidIndex;
    if (s_pState->m_BrickLookup.TryGetValue(required.m_uiPackedKey, uiPhysicalBrick))
      continue;

    if (!s_pState->m_FreeBricks.IsEmpty())
    {
      uiPhysicalBrick = s_pState->m_FreeBricks.PeekBack();
      s_pState->m_FreeBricks.PopBack();
    }
    else
    {
      if (uiNextEviction >= evictionCandidates.GetCount())
        continue;
      uiPhysicalBrick = evictionCandidates[uiNextEviction++].m_uiPhysicalBrick;
      s_pState->m_BrickLookup.Remove(s_pState->m_Bricks[uiPhysicalBrick].m_uiPackedKey);
      ++s_pState->m_Stats.m_uiEvictedBrickCount;
    }

    xiiSparseVoxelBrickState& brick = s_pState->m_Bricks[uiPhysicalBrick];
    brick = {};
    brick.m_uiPackedKey = required.m_uiPackedKey;
    brick.m_uiPhysicalBrick = uiPhysicalBrick;
    brick.m_uiClipmapLevel = required.m_uiClipmapLevel;
    brick.m_vCell = required.m_vCell;
    brick.m_vWorldMinimum = required.m_vWorldMinimum;
    brick.m_fVoxelSize = required.m_fVoxelSize;
    brick.m_uiLastUsedFrame = uiFrameIndex;
    brick.m_Flags = xiiSparseVoxelBrickFlags::Resident | xiiSparseVoxelBrickFlags::Dirty;
    s_pState->m_BrickLookup.Insert(required.m_uiPackedKey, uiPhysicalBrick);
    ++s_pState->m_Stats.m_uiAllocatedBrickCount;
  }

  struct UpdateCandidate
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPhysicalBrick;
    xiiUInt32 m_uiClipmapLevel;
    float     m_fDistanceSquared;
    xiiUInt64 m_uiPackedKey;
  };

  xiiDynamicArray<UpdateCandidate> candidates;
  candidates.Reserve(requiredBricks.GetCount());
  for (const RequiredBrick& required : requiredBricks)
  {
    xiiUInt32 uiPhysicalBrick = xiiInvalidIndex;
    if (!s_pState->m_BrickLookup.TryGetValue(required.m_uiPackedKey, uiPhysicalBrick))
      continue;

    xiiSparseVoxelBrickState& brick = s_pState->m_Bricks[uiPhysicalBrick];
    if (brick.m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid) && uiFrameIndex >= brick.m_uiLastUpdateFrame && uiFrameIndex - brick.m_uiLastUpdateFrame >= settings.m_uiRefreshIntervalFrames)
      brick.m_Flags.Add(xiiSparseVoxelBrickFlags::Dirty);
    if (brick.m_Flags.IsSet(xiiSparseVoxelBrickFlags::Dirty))
      candidates.PushBack({uiPhysicalBrick, required.m_uiClipmapLevel, required.m_fDistanceSquared, required.m_uiPackedKey});
  }

  candidates.Sort([](const UpdateCandidate& lhs, const UpdateCandidate& rhs) {
    if (lhs.m_uiClipmapLevel != rhs.m_uiClipmapLevel)
      return lhs.m_uiClipmapLevel < rhs.m_uiClipmapLevel;
    if (lhs.m_fDistanceSquared != rhs.m_fDistanceSquared)
      return lhs.m_fDistanceSquared < rhs.m_fDistanceSquared;
    return lhs.m_uiPackedKey < rhs.m_uiPackedKey;
  });

  const xiiUInt32 uiUpdateCount = xiiMath::Min(settings.m_uiBrickUpdateBudget, candidates.GetCount());
  s_pState->m_ScheduledUpdates.Reserve(uiUpdateCount);
  for (xiiUInt32 i = 0U; i < uiUpdateCount; ++i)
  {
    const xiiSparseVoxelBrickState& brick = s_pState->m_Bricks[candidates[i].m_uiPhysicalBrick];
    s_pState->m_ScheduledUpdates.PushBack({brick.m_uiPackedKey, brick.m_uiPhysicalBrick, brick.m_uiClipmapLevel, brick.m_vWorldMinimum, brick.m_fVoxelSize,
      brick.m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid) ? settings.m_fTemporalHysteresis : 0.0f});
  }

  s_pState->m_Stats.m_uiResidentBrickCount = s_pState->m_BrickLookup.GetCount();
  s_pState->m_Stats.m_uiDirtyBrickCount = candidates.GetCount();
  s_pState->m_Stats.m_uiScheduledBrickCount = s_pState->m_ScheduledUpdates.GetCount();
}

void xiiSparseVoxelRadianceManager::CommitBrickUpdate(xiiUInt32 uiPhysicalBrick, xiiUInt64 uiPackedKey, bool bValid)
{
  if (!IsInitialized() || uiPhysicalBrick >= s_pState->m_Bricks.GetCount())
    return;

  xiiSparseVoxelBrickState& brick = s_pState->m_Bricks[uiPhysicalBrick];
  if (brick.m_uiPackedKey != uiPackedKey || !brick.m_Flags.IsSet(xiiSparseVoxelBrickFlags::Resident))
    return;

  brick.m_Flags.Remove(xiiSparseVoxelBrickFlags::Dirty | xiiSparseVoxelBrickFlags::Valid);
  brick.m_Flags.AddOrRemove(xiiSparseVoxelBrickFlags::Valid, bValid);
  brick.m_uiLastUpdateFrame = s_pState->m_uiFrameIndex;
}

xiiArrayPtr<const xiiSparseVoxelBrickState> xiiSparseVoxelRadianceManager::GetBricks()
{
  return IsInitialized() ? xiiArrayPtr<const xiiSparseVoxelBrickState>(s_pState->m_Bricks.GetData(), s_pState->m_Bricks.GetCount()) : xiiArrayPtr<const xiiSparseVoxelBrickState>();
}

xiiArrayPtr<const xiiSparseVoxelBrickUpdate> xiiSparseVoxelRadianceManager::GetScheduledUpdates()
{
  return IsInitialized() ? xiiArrayPtr<const xiiSparseVoxelBrickUpdate>(s_pState->m_ScheduledUpdates.GetData(), s_pState->m_ScheduledUpdates.GetCount()) : xiiArrayPtr<const xiiSparseVoxelBrickUpdate>();
}

xiiSparseVoxelRadianceFrameStats xiiSparseVoxelRadianceManager::GetFrameStats()
{
  return IsInitialized() ? s_pState->m_Stats : xiiSparseVoxelRadianceFrameStats();
}

const xiiSparseVoxelRadianceSettings& xiiSparseVoxelRadianceManager::GetConfiguration()
{
  XII_ASSERT_RELEASE(IsInitialized(), "Sparse voxel radiance manager is not initialized.");
  return s_pState->m_Settings;
}

const xiiSparseVoxelBrickState* xiiSparseVoxelRadianceManager::FindBrick(xiiUInt64 uiPackedKey)
{
  if (!IsInitialized())
    return nullptr;
  xiiUInt32 uiPhysicalBrick = xiiInvalidIndex;
  return s_pState->m_BrickLookup.TryGetValue(uiPackedKey, uiPhysicalBrick) ? &s_pState->m_Bricks[uiPhysicalBrick] : nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_SparseVoxelRadiance);
