/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>

namespace
{
  static bool IsConfigurationValid(const xiiDDGISettings& settings)
  {
    const xiiUInt64 uiProbeCount = static_cast<xiiUInt64>(settings.m_uiProbeCountX) * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
    return settings.m_uiProbeCountX > 0U && settings.m_uiProbeCountY > 0U && settings.m_uiProbeCountZ > 0U &&
           uiProbeCount <= 1048576U && xiiMath::IsFinite(settings.m_fProbeSpacing) && settings.m_fProbeSpacing > 0.0f &&
           settings.m_uiProbeUpdateBudget > 0U && xiiMath::IsFinite(settings.m_fTemporalHysteresis) &&
           settings.m_fTemporalHysteresis >= 0.0f && settings.m_fTemporalHysteresis < 1.0f &&
           xiiMath::IsFinite(settings.m_fMaximumRelocationDistance) && settings.m_fMaximumRelocationDistance >= 0.0f;
  }

  static xiiInt32 PositiveModulo(xiiInt32 value, xiiUInt32 divisor)
  {
    const xiiInt32 iDivisor = static_cast<xiiInt32>(divisor);
    const xiiInt32 result = value % iDivisor;
    return result < 0 ? result + iDivisor : result;
  }
}

class xiiDDGIManager::State
{
public:
  xiiDDGISettings m_Settings;
  xiiDynamicArray<xiiDDGIProbeState> m_Probes;
  xiiDynamicArray<xiiDDGIProbeUpdate> m_ScheduledUpdates;
  xiiDDGIFrameStats m_Stats;
  xiiVec3I32 m_vMinimumCell = xiiVec3I32(xiiMath::MaxValue<xiiInt32>());
  xiiVec3 m_vCameraPosition = xiiVec3::MakeZero();
  xiiUInt64 m_uiFrameIndex = 0U;
  bool m_bInitialized = false;
};

xiiUniquePtr<xiiDDGIManager::State> xiiDDGIManager::s_pState;

XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, DDGIManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiDDGIManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiDDGIManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDDGISettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDDGISettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProbeCountX", m_uiProbeCountX)->AddAttributes(new xiiClampValueAttribute(1U, 256U)),
    XII_MEMBER_PROPERTY("ProbeCountY", m_uiProbeCountY)->AddAttributes(new xiiClampValueAttribute(1U, 256U)),
    XII_MEMBER_PROPERTY("ProbeCountZ", m_uiProbeCountZ)->AddAttributes(new xiiClampValueAttribute(1U, 256U)),
    XII_MEMBER_PROPERTY("ProbeSpacing", m_fProbeSpacing)->AddAttributes(new xiiClampValueAttribute(0.1f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ProbeUpdateBudget", m_uiProbeUpdateBudget)->AddAttributes(new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("TemporalHysteresis", m_fTemporalHysteresis)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.9999f)),
    XII_MEMBER_PROPERTY("MaximumRelocationDistance", m_fMaximumRelocationDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiDDGIProbeFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiDDGIProbeFlags::Valid, xiiDDGIProbeFlags::NeedsUpdate, xiiDDGIProbeFlags::Relocated, xiiDDGIProbeFlags::InsideGeometry)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDDGIProbeState, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDDGIProbeState>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CellX", m_iCellX),
    XII_MEMBER_PROPERTY("CellY", m_iCellY),
    XII_MEMBER_PROPERTY("CellZ", m_iCellZ),
    XII_MEMBER_PROPERTY("WorldPosition", m_vWorldPosition),
    XII_MEMBER_PROPERTY("RelocationOffset", m_vRelocationOffset),
    XII_MEMBER_PROPERTY("LastUpdatedFrame", m_uiLastUpdatedFrame),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiDDGIProbeFlags, m_Flags),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDDGIProbeUpdate, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDDGIProbeUpdate>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PhysicalProbe", m_uiPhysicalProbe),
    XII_MEMBER_PROPERTY("WorldPosition", m_vWorldPosition),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDDGIFrameStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDDGIFrameStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProbeCount", m_uiProbeCount),
    XII_MEMBER_PROPERTY("InvalidProbeCount", m_uiInvalidProbeCount),
    XII_MEMBER_PROPERTY("ScheduledUpdateCount", m_uiScheduledUpdateCount),
    XII_MEMBER_PROPERTY("ScrolledProbeCount", m_uiScrolledProbeCount),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiDDGIManager::Startup()
{
  s_pState = XII_DEFAULT_NEW(State);
  Configure(xiiDDGISettings()).IgnoreResult();
}

void xiiDDGIManager::Shutdown()
{
  s_pState.Clear();
}

xiiResult xiiDDGIManager::Configure(const xiiDDGISettings& settings)
{
  if (s_pState == nullptr || !IsConfigurationValid(settings))
    return XII_FAILURE;

  s_pState->m_Settings = settings;
  const xiiUInt32 uiProbeCount = settings.m_uiProbeCountX * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
  s_pState->m_Probes.Clear();
  s_pState->m_Probes.SetCount(uiProbeCount);
  s_pState->m_ScheduledUpdates.Clear();
  s_pState->m_vMinimumCell = xiiVec3I32(xiiMath::MaxValue<xiiInt32>());
  s_pState->m_Stats = {};
  s_pState->m_Stats.m_uiProbeCount = uiProbeCount;
  s_pState->m_bInitialized = true;
  return XII_SUCCESS;
}

bool xiiDDGIManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

void xiiDDGIManager::BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized())
    return;

  const xiiDDGISettings& settings = s_pState->m_Settings;
  s_pState->m_uiFrameIndex = uiFrameIndex;
  s_pState->m_vCameraPosition = vCameraPosition;
  s_pState->m_ScheduledUpdates.Clear();
  s_pState->m_Stats.m_uiInvalidProbeCount = 0U;
  s_pState->m_Stats.m_uiScrolledProbeCount = 0U;

  const xiiVec3 vCellPosition = vCameraPosition / settings.m_fProbeSpacing;
  const xiiVec3I32 vCenterCell(static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.x)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.y)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.z)));
  const xiiVec3I32 vMinimumCell = vCenterCell - xiiVec3I32(static_cast<xiiInt32>(settings.m_uiProbeCountX / 2U), static_cast<xiiInt32>(settings.m_uiProbeCountY / 2U), static_cast<xiiInt32>(settings.m_uiProbeCountZ / 2U));
  s_pState->m_vMinimumCell = vMinimumCell;

  struct Candidate
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPhysicalProbe;
    float m_fDistanceSquared;
    xiiUInt64 m_uiLastUpdatedFrame;
    bool m_bNeedsUpdate;
  };
  xiiDynamicArray<Candidate> candidates;
  candidates.Reserve(s_pState->m_Probes.GetCount());

  for (xiiUInt32 z = 0U; z < settings.m_uiProbeCountZ; ++z)
  {
    for (xiiUInt32 y = 0U; y < settings.m_uiProbeCountY; ++y)
    {
      for (xiiUInt32 x = 0U; x < settings.m_uiProbeCountX; ++x)
      {
        const xiiVec3I32 vCell = vMinimumCell + xiiVec3I32(static_cast<xiiInt32>(x), static_cast<xiiInt32>(y), static_cast<xiiInt32>(z));
        const xiiUInt32 uiSlotX = static_cast<xiiUInt32>(PositiveModulo(vCell.x, settings.m_uiProbeCountX));
        const xiiUInt32 uiSlotY = static_cast<xiiUInt32>(PositiveModulo(vCell.y, settings.m_uiProbeCountY));
        const xiiUInt32 uiSlotZ = static_cast<xiiUInt32>(PositiveModulo(vCell.z, settings.m_uiProbeCountZ));
        const xiiUInt32 uiPhysicalProbe = (uiSlotZ * settings.m_uiProbeCountY + uiSlotY) * settings.m_uiProbeCountX + uiSlotX;
        xiiDDGIProbeState& probe = s_pState->m_Probes[uiPhysicalProbe];

        if (probe.m_iCellX != vCell.x || probe.m_iCellY != vCell.y || probe.m_iCellZ != vCell.z)
        {
          probe = {};
          probe.m_iCellX = vCell.x;
          probe.m_iCellY = vCell.y;
          probe.m_iCellZ = vCell.z;
          probe.m_vWorldPosition = (xiiVec3(static_cast<float>(vCell.x), static_cast<float>(vCell.y), static_cast<float>(vCell.z)) + xiiVec3(0.5f)) * settings.m_fProbeSpacing;
          probe.m_Flags = xiiDDGIProbeFlags::NeedsUpdate;
          ++s_pState->m_Stats.m_uiScrolledProbeCount;
        }

        const bool bNeedsUpdate = probe.m_Flags.IsSet(xiiDDGIProbeFlags::NeedsUpdate) || !probe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid);
        if (bNeedsUpdate)
          ++s_pState->m_Stats.m_uiInvalidProbeCount;

        candidates.PushBack({uiPhysicalProbe, (probe.m_vWorldPosition + probe.m_vRelocationOffset - vCameraPosition).GetLengthSquared(), probe.m_uiLastUpdatedFrame, bNeedsUpdate});
      }
    }
  }

  candidates.Sort([](const Candidate& lhs, const Candidate& rhs) {
    if (lhs.m_bNeedsUpdate != rhs.m_bNeedsUpdate)
      return lhs.m_bNeedsUpdate;
    if (lhs.m_bNeedsUpdate && lhs.m_fDistanceSquared != rhs.m_fDistanceSquared)
      return lhs.m_fDistanceSquared < rhs.m_fDistanceSquared;
    if (lhs.m_uiLastUpdatedFrame != rhs.m_uiLastUpdatedFrame)
      return lhs.m_uiLastUpdatedFrame < rhs.m_uiLastUpdatedFrame;
    return lhs.m_uiPhysicalProbe < rhs.m_uiPhysicalProbe;
  });

  const xiiUInt32 uiUpdateCount = xiiMath::Min(settings.m_uiProbeUpdateBudget, candidates.GetCount());
  s_pState->m_ScheduledUpdates.Reserve(uiUpdateCount);
  for (xiiUInt32 i = 0U; i < uiUpdateCount; ++i)
  {
    const Candidate& candidate = candidates[i];
    const xiiDDGIProbeState& probe = s_pState->m_Probes[candidate.m_uiPhysicalProbe];
    s_pState->m_ScheduledUpdates.PushBack({candidate.m_uiPhysicalProbe, probe.m_vWorldPosition + probe.m_vRelocationOffset,
      probe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid) ? settings.m_fTemporalHysteresis : 0.0f});
  }
  s_pState->m_Stats.m_uiScheduledUpdateCount = uiUpdateCount;
}

void xiiDDGIManager::CommitProbeUpdate(xiiUInt32 uiPhysicalProbe, const xiiVec3& vRelocationOffset, bool bValid, bool bInsideGeometry)
{
  if (!IsInitialized() || uiPhysicalProbe >= s_pState->m_Probes.GetCount())
    return;

  xiiDDGIProbeState& probe = s_pState->m_Probes[uiPhysicalProbe];
  xiiVec3 vClampedOffset = vRelocationOffset;
  const float fMaximumDistance = s_pState->m_Settings.m_fMaximumRelocationDistance;
  if (vClampedOffset.GetLengthSquared() > fMaximumDistance * fMaximumDistance && vClampedOffset.NormalizeIfNotZero(xiiVec3::MakeZero()).Succeeded())
    vClampedOffset *= fMaximumDistance;

  probe.m_vRelocationOffset = vClampedOffset;
  probe.m_uiLastUpdatedFrame = s_pState->m_uiFrameIndex;
  probe.m_Flags.Remove(xiiDDGIProbeFlags::NeedsUpdate | xiiDDGIProbeFlags::Valid | xiiDDGIProbeFlags::Relocated | xiiDDGIProbeFlags::InsideGeometry);
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::Valid, bValid);
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::Relocated, !vClampedOffset.IsZero(0.0001f));
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::InsideGeometry, bInsideGeometry);
}

xiiArrayPtr<const xiiDDGIProbeState> xiiDDGIManager::GetProbes()
{
  if (!IsInitialized())
    return {};
  return xiiArrayPtr<const xiiDDGIProbeState>(s_pState->m_Probes.GetData(), s_pState->m_Probes.GetCount());
}

xiiArrayPtr<const xiiDDGIProbeUpdate> xiiDDGIManager::GetScheduledUpdates()
{
  if (!IsInitialized())
    return {};
  return xiiArrayPtr<const xiiDDGIProbeUpdate>(s_pState->m_ScheduledUpdates.GetData(), s_pState->m_ScheduledUpdates.GetCount());
}

xiiDDGIFrameStats xiiDDGIManager::GetFrameStats()
{
  return IsInitialized() ? s_pState->m_Stats : xiiDDGIFrameStats();
}

const xiiDDGISettings& xiiDDGIManager::GetConfiguration()
{
  XII_ASSERT_RELEASE(IsInitialized(), "DDGI manager is not initialized.");
  return s_pState->m_Settings;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_DynamicGlobalIllumination);
