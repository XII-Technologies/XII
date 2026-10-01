/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Lighting/LightingSystem.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/SparseVoxelRadiance/SparseVoxelRadianceConstants.h>

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
} // namespace

class xiiSparseVoxelRadianceManager::State
{
public:
  xiiSparseVoxelRadianceSettings             m_Settings;
  xiiDynamicArray<xiiSparseVoxelBrickState>  m_Bricks;
  xiiDynamicArray<xiiSparseVoxelBrickUpdate> m_ScheduledUpdates;
  xiiDynamicArray<xiiUInt32>                 m_FreeBricks;
  xiiHashTable<xiiUInt64, xiiUInt32>         m_BrickLookup;
  xiiSparseVoxelRadianceFrameStats           m_Stats;
  xiiDynamicArray<xiiVec3I32>                m_ClipmapMinimumCells;
  xiiSharedPtr<xiiGALBuffer>                 m_pRadiancePool;
  xiiSharedPtr<xiiGALComputePipelineState>   m_pUpdatePipeline;
  xiiUInt64                                  m_uiFrameIndex         = 0U;
  xiiUInt64                                  m_uiLastGpuUpdateFrame = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32                                  m_uiVoxelsPerBrick     = 0U;
  bool                                       m_bEngineStarted       = false;
  bool                                       m_bInitialized         = false;
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSparseVoxelBrickUpdate, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuSparseVoxelBrickUpdate>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WorldMinimumAndVoxelSize", m_vWorldMinimumAndVoxelSize),
    XII_MEMBER_PROPERTY("PhysicalLevelAndKey", m_vPhysicalLevelAndKey),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSparseVoxelLevel, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuSparseVoxelLevel>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinimumCellAndPageOffset", m_vMinimumCellAndPageOffset),
    XII_MEMBER_PROPERTY("VoxelAndBrickSize", m_vVoxelAndBrickSize),
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
  {
    s_pState->m_bEngineStarted = true;
    CreateGpuResources().IgnoreResult();
  }
}

void xiiSparseVoxelRadianceManager::EngineShutdown()
{
  if (s_pState != nullptr)
  {
    s_pState->m_pUpdatePipeline.Clear();
    s_pState->m_pRadiancePool.Clear();
    s_pState->m_bEngineStarted = false;
  }
}

void xiiSparseVoxelRadianceManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiSparseVoxelRadianceManager::CreateGpuResources()
{
  if (s_pState == nullptr || !s_pState->m_bEngineStarted || s_pState->m_uiVoxelsPerBrick == 0U)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return XII_FAILURE;

  const xiiUInt64 uiElementCount = static_cast<xiiUInt64>(s_pState->m_Settings.m_uiMaxResidentBricks) * s_pState->m_uiVoxelsPerBrick;
  const xiiUInt64 uiBufferSize   = uiElementCount * sizeof(xiiVec4);
  if (uiBufferSize == 0U || uiBufferSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  xiiGALBufferCreationDescription description;
  description.m_uiSize                     = static_cast<xiiUInt32>(uiBufferSize);
  description.m_uiElementByteStride        = sizeof(xiiVec4);
  description.m_BindFlags                  = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                       = xiiGALBufferMode::Structured;
  description.m_Usage                      = xiiGALResourceUsage::Default;
  xiiSharedPtr<xiiGALBuffer> pRadiancePool = pDevice->CreateBuffer(description);
  if (pRadiancePool == nullptr)
    return XII_FAILURE;
  pRadiancePool->SetDebugName("Sparse Voxel Radiance Pool");

  s_pState->m_pRadiancePool = std::move(pRadiancePool);
  return XII_SUCCESS;
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
    const xiiUInt32 uiPhysicalBrick                       = i - 1U;
    s_pState->m_Bricks[uiPhysicalBrick].m_uiPhysicalBrick = uiPhysicalBrick;
    s_pState->m_FreeBricks.PushBack(uiPhysicalBrick);
  }
  s_pState->m_BrickLookup.Clear();
  s_pState->m_BrickLookup.Reserve(settings.m_uiMaxResidentBricks);
  s_pState->m_ScheduledUpdates.Clear();
  s_pState->m_ClipmapMinimumCells.SetCount(settings.m_uiClipmapLevels);
  s_pState->m_Stats                = {};
  s_pState->m_uiVoxelsPerBrick     = settings.m_uiBrickVoxelResolution * settings.m_uiBrickVoxelResolution * settings.m_uiBrickVoxelResolution;
  s_pState->m_uiLastGpuUpdateFrame = xiiMath::MaxValue<xiiUInt64>();
  s_pState->m_bInitialized         = true;
  if (s_pState->m_bEngineStarted)
    return CreateGpuResources();
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
  out_vCell.x        = static_cast<xiiInt32>(uiPackedKey & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
  out_vCell.y        = static_cast<xiiInt32>((uiPackedKey >> s_uiCoordinateBits) & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
  out_vCell.z        = static_cast<xiiInt32>((uiPackedKey >> (s_uiCoordinateBits * 2U)) & s_uiCoordinateMask) - static_cast<xiiInt32>(s_uiCoordinateBias);
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

  const xiiSparseVoxelRadianceSettings& settings     = s_pState->m_Settings;
  const xiiUInt32                       uiResolution = settings.m_uiClipmapBrickResolution;
  xiiDynamicArray<RequiredBrick>        requiredBricks;
  requiredBricks.Reserve(uiResolution * uiResolution * uiResolution * settings.m_uiClipmapLevels);

  for (xiiUInt32 level = 0U; level < settings.m_uiClipmapLevels; ++level)
  {
    const float      fVoxelSize      = settings.m_fBaseVoxelSize * static_cast<float>(XII_BIT(level));
    const float      fBrickWorldSize = fVoxelSize * static_cast<float>(settings.m_uiBrickVoxelResolution);
    const xiiVec3    vCellPosition   = vCameraPosition / fBrickWorldSize;
    const xiiVec3I32 vCenterCell(static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.x)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.y)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.z)));
    const xiiVec3I32 vMinimumCell          = vCenterCell - xiiVec3I32(static_cast<xiiInt32>(uiResolution / 2U));
    s_pState->m_ClipmapMinimumCells[level] = vMinimumCell;

    for (xiiUInt32 z = 0U; z < uiResolution; ++z)
    {
      for (xiiUInt32 y = 0U; y < uiResolution; ++y)
      {
        for (xiiUInt32 x = 0U; x < uiResolution; ++x)
        {
          const xiiVec3I32 vCell       = vMinimumCell + xiiVec3I32(static_cast<xiiInt32>(x), static_cast<xiiInt32>(y), static_cast<xiiInt32>(z));
          const xiiUInt64  uiPackedKey = PackBrickKey(level, vCell);
          if (uiPackedKey == xiiMath::MaxValue<xiiUInt64>())
            continue;

          RequiredBrick& required     = requiredBricks.ExpandAndGetRef();
          required.m_uiPackedKey      = uiPackedKey;
          required.m_uiClipmapLevel   = level;
          required.m_vCell            = vCell;
          required.m_vWorldMinimum    = xiiVec3(static_cast<float>(vCell.x), static_cast<float>(vCell.y), static_cast<float>(vCell.z)) * fBrickWorldSize;
          required.m_fVoxelSize       = fVoxelSize;
          const xiiVec3 vBrickCenter  = required.m_vWorldMinimum + xiiVec3(fBrickWorldSize * 0.5f);
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
    brick                           = {};
    brick.m_uiPackedKey             = required.m_uiPackedKey;
    brick.m_uiPhysicalBrick         = uiPhysicalBrick;
    brick.m_uiClipmapLevel          = required.m_uiClipmapLevel;
    brick.m_vCell                   = required.m_vCell;
    brick.m_vWorldMinimum           = required.m_vWorldMinimum;
    brick.m_fVoxelSize              = required.m_fVoxelSize;
    brick.m_uiLastUsedFrame         = uiFrameIndex;
    brick.m_Flags                   = xiiSparseVoxelBrickFlags::Resident | xiiSparseVoxelBrickFlags::Dirty;
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

  s_pState->m_Stats.m_uiResidentBrickCount  = s_pState->m_BrickLookup.GetCount();
  s_pState->m_Stats.m_uiDirtyBrickCount     = candidates.GetCount();
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

xiiSparseVoxelRadianceManager::UpdateHandles xiiSparseVoxelRadianceManager::AddUpdatePass(xiiRenderGraph& graph, const xiiLightingSystem* pLightingSystem)
{
  UpdateHandles result;
  if (!IsInitialized() || pLightingSystem == nullptr)
    return result;
  if (s_pState->m_pRadiancePool == nullptr && CreateGpuResources().Failed())
    return result;

  if (s_pState->m_pUpdatePipeline == nullptr)
  {
    const xiiShaderResourceHandle                  hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/SparseVoxelRadianceUpdate.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
    const xiiShaderPermutationResourceHandle       hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
    xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid())
      return result;

    xiiGALComputePipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
    pipelineDescription.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    s_pState->m_pUpdatePipeline                      = xiiGALPipelineCache::GetPipeline(pipelineDescription);
    if (s_pState->m_pUpdatePipeline == nullptr)
      return result;
  }

  struct UpdatePassData
  {
    xiiRenderGraphBufferHandle                    m_hRadiancePool;
    xiiRenderGraphBufferHandle                    m_hPageTable;
    xiiRenderGraphBufferHandle                    m_hLevelData;
    xiiRenderGraphBufferHandle                    m_hUpdates;
    xiiRenderGraphBufferHandle                    m_hConstants;
    xiiDynamicArray<xiiUInt32>                    m_PageTable;
    xiiDynamicArray<xiiGpuSparseVoxelLevel>       m_LevelData;
    xiiDynamicArray<xiiGpuSparseVoxelBrickUpdate> m_Updates;
    const xiiLightingSystem*                      m_pLightingSystem = nullptr;
  };

  const bool bPerformUpdates = s_pState->m_uiLastGpuUpdateFrame != s_pState->m_uiFrameIndex;
  auto       pass            = graph.AddPass<UpdatePassData>(
    "Sparse Voxel Radiance Update", xiiGALCommandQueueFlags::Compute,
    [bPerformUpdates](UpdatePassData& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      const xiiUInt32                 uiResolution = s_pState->m_Settings.m_uiClipmapBrickResolution;
      const xiiUInt32                 uiPageCount  = s_pState->m_Settings.m_uiClipmapLevels * uiResolution * uiResolution * uiResolution;
      description.m_uiSize                         = xiiMath::Max(uiPageCount, 1U) * sizeof(xiiUInt32);
      description.m_uiElementByteStride            = sizeof(xiiUInt32);
      description.m_BindFlags                      = xiiGALBindFlags::ShaderResource;
      description.m_Mode                           = xiiGALBufferMode::Structured;
      description.m_Usage                          = xiiGALResourceUsage::Dynamic;
      description.m_CPUAccessFlags                 = xiiGALCPUAccessFlag::Write;
      data.m_hPageTable                            = builder.WriteBuffer(xiiRGBlackboardKeys::k_SparseVoxelPageTable, description, xiiGALResourceStateFlags::ShaderResource);

      description.m_uiSize              = xiiMath::Max(s_pState->m_Settings.m_uiClipmapLevels, 1U) * sizeof(xiiGpuSparseVoxelLevel);
      description.m_uiElementByteStride = sizeof(xiiGpuSparseVoxelLevel);
      data.m_hLevelData                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_SparseVoxelLevelData, description, xiiGALResourceStateFlags::ShaderResource);

      description.m_uiSize              = xiiMath::Max(bPerformUpdates ? s_pState->m_ScheduledUpdates.GetCount() : 0U, 1U) * sizeof(xiiGpuSparseVoxelBrickUpdate);
      description.m_uiElementByteStride = sizeof(xiiGpuSparseVoxelBrickUpdate);
      data.m_hUpdates                   = builder.WriteBuffer("SparseVoxelUpdates", description, xiiGALResourceStateFlags::ShaderResource);

      description                  = {};
      description.m_uiSize         = sizeof(xiiSparseVoxelRadianceConstants);
      description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      description.m_Usage          = xiiGALResourceUsage::Dynamic;
      description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants            = builder.WriteBuffer(xiiRGBlackboardKeys::k_SparseVoxelConstants, description, xiiGALResourceStateFlags::ConstantBuffer);

      data.m_hRadiancePool = builder.ImportBuffer(xiiRGBlackboardKeys::k_SparseVoxelRadiancePool, s_pState->m_pRadiancePool, s_pState->m_pRadiancePool->GetResourceState());
      data.m_hRadiancePool = bPerformUpdates ? builder.WriteBuffer(data.m_hRadiancePool, xiiGALResourceStateFlags::UnorderedAccess) : builder.ReadBuffer(data.m_hRadiancePool, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(data.m_hRadiancePool, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(bPerformUpdates);
      builder.SetPassAllowMerge(false);
    },
    [](const UpdatePassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      {
        xiiGALMapHelper<xiiUInt32> mapped(cmd, context.GetBuffer(data.m_hPageTable), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        xiiMemoryUtils::Copy(mapped.GetMappedData(), data.m_PageTable.GetData(), data.m_PageTable.GetCount());
      }
      {
        xiiGALMapHelper<xiiGpuSparseVoxelLevel> mapped(cmd, context.GetBuffer(data.m_hLevelData), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        xiiMemoryUtils::Copy(mapped.GetMappedData(), data.m_LevelData.GetData(), data.m_LevelData.GetCount());
      }
      if (!data.m_Updates.IsEmpty())
      {
        xiiGALMapHelper<xiiGpuSparseVoxelBrickUpdate> mapped(cmd, context.GetBuffer(data.m_hUpdates), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        xiiMemoryUtils::Copy(mapped.GetMappedData(), data.m_Updates.GetData(), data.m_Updates.GetCount());
      }
      {
        xiiGALMapHelper<xiiSparseVoxelRadianceConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->ClipmapAndUpdateCounts = xiiVec4U32(s_pState->m_Settings.m_uiClipmapLevels, s_pState->m_Settings.m_uiClipmapBrickResolution,
                                                                        s_pState->m_Settings.m_uiBrickVoxelResolution, data.m_Updates.GetCount());
        constants->PoolLayout             = xiiVec4U32(s_pState->m_uiVoxelsPerBrick, s_pState->m_Settings.m_uiMaxResidentBricks, static_cast<xiiUInt32>(s_pState->m_uiFrameIndex), 0U);
        constants->RadianceSettings       = xiiVec4(s_pState->m_Settings.m_fTemporalHysteresis, 0.0f, 0.0f, 0.0f);
      }

      if (!data.m_Updates.IsEmpty())
      {
        cmd.SetPipelineState(s_pState->m_pUpdatePipeline);
        data.m_pLightingSystem->BindFrameConstants(cmd, xiiGALShaderType::Compute);
        cmd.ResolveAndSetConstantBuffer("xiiSparseVoxelRadianceConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelUpdates", context.GetBuffer(data.m_hUpdates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetUnorderedAccessBufferView("g_SparseVoxelRadiancePool", context.GetBuffer(data.m_hRadiancePool)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
        const xiiUInt32 uiVoxelUpdateCount = data.m_Updates.GetCount() * s_pState->m_uiVoxelsPerBrick;
        cmd.DispatchCompute({(uiVoxelUpdateCount + 63U) / 64U, 1U, 1U});

        for (const xiiGpuSparseVoxelBrickUpdate& update : data.m_Updates)
        {
          const xiiUInt64 uiPackedKey = static_cast<xiiUInt64>(update.m_vPhysicalLevelAndKey.z) << 32U | update.m_vPhysicalLevelAndKey.y;
          CommitBrickUpdate(update.m_vPhysicalLevelAndKey.x, uiPackedKey, true);
        }
      }
      if (s_pState->m_uiLastGpuUpdateFrame != s_pState->m_uiFrameIndex)
        s_pState->m_uiLastGpuUpdateFrame = s_pState->m_uiFrameIndex;
    });

  pass.first->m_pLightingSystem   = pLightingSystem;
  const xiiUInt32 uiResolution    = s_pState->m_Settings.m_uiClipmapBrickResolution;
  const xiiUInt32 uiPagesPerLevel = uiResolution * uiResolution * uiResolution;
  pass.first->m_PageTable.SetCount(s_pState->m_Settings.m_uiClipmapLevels * uiPagesPerLevel);
  for (xiiUInt32& uiPage : pass.first->m_PageTable)
    uiPage = xiiInvalidIndex;
  pass.first->m_LevelData.SetCount(s_pState->m_Settings.m_uiClipmapLevels);
  for (xiiUInt32 level = 0U; level < s_pState->m_Settings.m_uiClipmapLevels; ++level)
  {
    const float             fVoxelSize      = s_pState->m_Settings.m_fBaseVoxelSize * static_cast<float>(XII_BIT(level));
    const float             fBrickWorldSize = fVoxelSize * s_pState->m_Settings.m_uiBrickVoxelResolution;
    const xiiVec3I32        vMinimumCell    = s_pState->m_ClipmapMinimumCells[level];
    xiiGpuSparseVoxelLevel& gpuLevel        = pass.first->m_LevelData[level];
    gpuLevel.m_vMinimumCellAndPageOffset    = xiiVec4I32(vMinimumCell.x, vMinimumCell.y, vMinimumCell.z, static_cast<xiiInt32>(level * uiPagesPerLevel));
    gpuLevel.m_vVoxelAndBrickSize           = xiiVec4(fVoxelSize, fBrickWorldSize, 1.0f / fVoxelSize, 1.0f / fBrickWorldSize);

    for (xiiUInt32 z = 0U; z < uiResolution; ++z)
    {
      for (xiiUInt32 y = 0U; y < uiResolution; ++y)
      {
        for (xiiUInt32 x = 0U; x < uiResolution; ++x)
        {
          const xiiVec3I32 vCell           = vMinimumCell + xiiVec3I32(static_cast<xiiInt32>(x), static_cast<xiiInt32>(y), static_cast<xiiInt32>(z));
          xiiUInt32        uiPhysicalBrick = xiiInvalidIndex;
          if (s_pState->m_BrickLookup.TryGetValue(PackBrickKey(level, vCell), uiPhysicalBrick) && s_pState->m_Bricks[uiPhysicalBrick].m_Flags.IsSet(xiiSparseVoxelBrickFlags::Valid))
            pass.first->m_PageTable[level * uiPagesPerLevel + (z * uiResolution + y) * uiResolution + x] = uiPhysicalBrick;
        }
      }
    }
  }

  if (bPerformUpdates)
  {
    pass.first->m_Updates.Reserve(s_pState->m_ScheduledUpdates.GetCount());
    for (const xiiSparseVoxelBrickUpdate& update : s_pState->m_ScheduledUpdates)
    {
      xiiUInt32 uiHistoryBits = 0U;
      xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&uiHistoryBits), reinterpret_cast<const xiiUInt8*>(&update.m_fHistoryWeight), sizeof(float));
      xiiGpuSparseVoxelBrickUpdate& gpuUpdate = pass.first->m_Updates.ExpandAndGetRef();
      gpuUpdate.m_vWorldMinimumAndVoxelSize   = xiiVec4(update.m_vWorldMinimum, update.m_fVoxelSize);
      gpuUpdate.m_vPhysicalLevelAndKey        = xiiVec4U32(update.m_uiPhysicalBrick, static_cast<xiiUInt32>(update.m_uiPackedKey),
                                                           static_cast<xiiUInt32>(update.m_uiPackedKey >> 32U), uiHistoryBits);
    }
  }

  result.m_hRadiancePool = pass.first->m_hRadiancePool;
  result.m_hPageTable    = pass.first->m_hPageTable;
  result.m_hLevelData    = pass.first->m_hLevelData;
  result.m_hConstants    = pass.first->m_hConstants;
  return result;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_SparseVoxelRadiance);
