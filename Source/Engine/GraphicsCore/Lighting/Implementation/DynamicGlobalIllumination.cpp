/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/Sorting.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>


namespace
{
  static bool IsConfigurationValid(const xiiDDGISettings& settings)
  {
    const xiiUInt64 uiProbeCount = static_cast<xiiUInt64>(settings.m_uiProbeCountX) * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
    return settings.m_uiProbeCountX > 0U && settings.m_uiProbeCountY > 0U && settings.m_uiProbeCountZ > 0U &&
      settings.m_uiProbeCountX <= 1048576U && settings.m_uiProbeCountY <= 1048576U && settings.m_uiProbeCountZ <= 1048576U &&
      uiProbeCount <= 1048576U && xiiMath::IsFinite(settings.m_fProbeSpacing) && settings.m_fProbeSpacing > 0.0f &&
      settings.m_uiProbeUpdateBudget > 0U && xiiMath::IsFinite(settings.m_fTemporalHysteresis) &&
      settings.m_fTemporalHysteresis >= 0.0f && settings.m_fTemporalHysteresis < 1.0f &&
      xiiMath::IsFinite(settings.m_fMaximumRelocationDistance) && settings.m_fMaximumRelocationDistance >= 0.0f;
  }

  static xiiInt32 PositiveModulo(xiiInt32 value, xiiUInt32 divisor)
  {
    const xiiInt32 iDivisor = static_cast<xiiInt32>(divisor);
    const xiiInt32 result   = value % iDivisor;
    return result < 0 ? result + iDivisor : result;
  }
} // namespace

class xiiDDGIContext::State
{
public:
  xiiDDGISettings                     m_Settings;
  xiiDynamicArray<xiiDDGIProbeState>  m_Probes;
  xiiDynamicArray<xiiDDGIProbeUpdate> m_ScheduledUpdates;
  xiiDDGIFrameStats                   m_Stats;
  xiiVec3I32                          m_vMinimumCell = xiiVec3I32(xiiMath::MaxValue<xiiInt32>());
  xiiUInt64                           m_uiFrameIndex = 0U;
};

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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuDDGIProbeState, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuDDGIProbeState>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionAndLastUpdate", m_vPositionAndLastUpdate),
    XII_MEMBER_PROPERTY("CellAndFlags", m_vCellAndFlags),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuDDGIProbeUpdate, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuDDGIProbeUpdate>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionAndHistoryWeight", m_vPositionAndHistoryWeight),
    XII_MEMBER_PROPERTY("Metadata", m_vMetadata),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiDDGIContext::xiiDDGIContext() = default;

xiiDDGIContext::~xiiDDGIContext()
{
  Shutdown();
}

void xiiDDGIContext::Shutdown()
{
  m_pState.Clear();
}

xiiResult xiiDDGIContext::Initialize(const xiiDDGISettings& settings)
{
  if (!IsConfigurationValid(settings))
    return XII_FAILURE;
  xiiUniquePtr<State> next = XII_DEFAULT_NEW(State);
  next->m_Settings         = settings;
  const xiiUInt32 count    = settings.m_uiProbeCountX * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
  next->m_Probes.SetCount(count);
  next->m_Stats.m_uiProbeCount = count;
  m_pState                     = std::move(next);
  return XII_SUCCESS;
}

bool xiiDDGIContext::IsInitialized() const
{
  return m_pState != nullptr;
}

void xiiDDGIContext::InvalidateHistory()
{
  if (!IsInitialized())
    return;
  for (auto& probe : m_pState->m_Probes)
  {
    probe.m_Flags.Remove(xiiDDGIProbeFlags::Valid | xiiDDGIProbeFlags::Relocated | xiiDDGIProbeFlags::InsideGeometry);
    probe.m_Flags.Add(xiiDDGIProbeFlags::NeedsUpdate);
    probe.m_vRelocationOffset.SetZero();
    probe.m_uiLastUpdatedFrame = 0;
  }
  for (auto& update : m_pState->m_ScheduledUpdates)
    update.m_fHistoryWeight = 0.0f;
}

xiiResult xiiDDGIContext::BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized() || uiFrameIndex == 0 || uiFrameIndex <= m_pState->m_uiFrameIndex || !xiiMath::IsFinite(vCameraPosition.x) || !xiiMath::IsFinite(vCameraPosition.y) || !xiiMath::IsFinite(vCameraPosition.z))
    return XII_FAILURE;

  const xiiDDGISettings& settings     = m_pState->m_Settings;
  const xiiVec3          cellPosition = vCameraPosition / settings.m_fProbeSpacing;
  const xiiUInt32        counts[]     = {settings.m_uiProbeCountX, settings.m_uiProbeCountY, settings.m_uiProbeCountZ};
  for (xiiUInt32 axis = 0; axis < 3; ++axis)
  {
    const double minimum = static_cast<double>(xiiMath::Floor(cellPosition.GetData()[axis])) - counts[axis] / 2U;
    if (!xiiMath::IsFinite(minimum) || minimum < xiiMath::MinValue<xiiInt32>() || minimum + counts[axis] - 1.0 > xiiMath::MaxValue<xiiInt32>())
      return XII_FAILURE;
  }
  m_pState->m_uiFrameIndex = uiFrameIndex;
  m_pState->m_ScheduledUpdates.Clear();
  m_pState->m_Stats.m_uiInvalidProbeCount  = 0U;
  m_pState->m_Stats.m_uiScrolledProbeCount = 0U;

  const xiiVec3    vCellPosition = vCameraPosition / settings.m_fProbeSpacing;
  const xiiVec3I32 vCenterCell(static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.x)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.y)), static_cast<xiiInt32>(xiiMath::Floor(vCellPosition.z)));
  const xiiVec3I32 vMinimumCell = vCenterCell - xiiVec3I32(static_cast<xiiInt32>(settings.m_uiProbeCountX / 2U), static_cast<xiiInt32>(settings.m_uiProbeCountY / 2U), static_cast<xiiInt32>(settings.m_uiProbeCountZ / 2U));
  m_pState->m_vMinimumCell      = vMinimumCell;

  struct Candidate
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPhysicalProbe;
    float     m_fDistanceSquared;
    xiiUInt64 m_uiLastUpdatedFrame;
    bool      m_bNeedsUpdate;
  };
  xiiDynamicArray<Candidate> candidates;
  candidates.Reserve(m_pState->m_Probes.GetCount());

  for (xiiUInt32 z = 0U; z < settings.m_uiProbeCountZ; ++z)
  {
    for (xiiUInt32 y = 0U; y < settings.m_uiProbeCountY; ++y)
    {
      for (xiiUInt32 x = 0U; x < settings.m_uiProbeCountX; ++x)
      {
        const xiiVec3I32   vCell           = vMinimumCell + xiiVec3I32(static_cast<xiiInt32>(x), static_cast<xiiInt32>(y), static_cast<xiiInt32>(z));
        const xiiUInt32    uiSlotX         = static_cast<xiiUInt32>(PositiveModulo(vCell.x, settings.m_uiProbeCountX));
        const xiiUInt32    uiSlotY         = static_cast<xiiUInt32>(PositiveModulo(vCell.y, settings.m_uiProbeCountY));
        const xiiUInt32    uiSlotZ         = static_cast<xiiUInt32>(PositiveModulo(vCell.z, settings.m_uiProbeCountZ));
        const xiiUInt32    uiPhysicalProbe = (uiSlotZ * settings.m_uiProbeCountY + uiSlotY) * settings.m_uiProbeCountX + uiSlotX;
        xiiDDGIProbeState& probe           = m_pState->m_Probes[uiPhysicalProbe];

        if (probe.m_iCellX != vCell.x || probe.m_iCellY != vCell.y || probe.m_iCellZ != vCell.z)
        {
          probe                  = {};
          probe.m_iCellX         = vCell.x;
          probe.m_iCellY         = vCell.y;
          probe.m_iCellZ         = vCell.z;
          probe.m_vWorldPosition = (xiiVec3(static_cast<float>(vCell.x), static_cast<float>(vCell.y), static_cast<float>(vCell.z)) + xiiVec3(0.5f)) * settings.m_fProbeSpacing;
          probe.m_Flags          = xiiDDGIProbeFlags::NeedsUpdate;
          ++m_pState->m_Stats.m_uiScrolledProbeCount;
        }

        const bool bNeedsUpdate = probe.m_Flags.IsSet(xiiDDGIProbeFlags::NeedsUpdate) || !probe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid);
        if (bNeedsUpdate)
          ++m_pState->m_Stats.m_uiInvalidProbeCount;

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
  m_pState->m_ScheduledUpdates.Reserve(uiUpdateCount);
  for (xiiUInt32 i = 0U; i < uiUpdateCount; ++i)
  {
    const Candidate&         candidate = candidates[i];
    const xiiDDGIProbeState& probe     = m_pState->m_Probes[candidate.m_uiPhysicalProbe];
    m_pState->m_ScheduledUpdates.PushBack({candidate.m_uiPhysicalProbe, probe.m_vWorldPosition + probe.m_vRelocationOffset, probe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid) ? settings.m_fTemporalHysteresis : 0.0f});
  }
  m_pState->m_Stats.m_uiScheduledUpdateCount = uiUpdateCount;
  return XII_SUCCESS;
}

void xiiDDGIContext::CommitProbeUpdate(xiiUInt32 uiPhysicalProbe, const xiiVec3& vRelocationOffset, bool bValid, bool bInsideGeometry)
{
  if (!IsInitialized() || uiPhysicalProbe >= m_pState->m_Probes.GetCount())
    return;

  xiiDDGIProbeState& probe            = m_pState->m_Probes[uiPhysicalProbe];
  xiiVec3            vClampedOffset   = vRelocationOffset;
  const float        fMaximumDistance = m_pState->m_Settings.m_fMaximumRelocationDistance;
  if (vClampedOffset.GetLengthSquared() > fMaximumDistance * fMaximumDistance && vClampedOffset.NormalizeIfNotZero(xiiVec3::MakeZero()).Succeeded())
    vClampedOffset *= fMaximumDistance;

  probe.m_vRelocationOffset  = vClampedOffset;
  probe.m_uiLastUpdatedFrame = m_pState->m_uiFrameIndex;
  probe.m_Flags.Remove(xiiDDGIProbeFlags::NeedsUpdate | xiiDDGIProbeFlags::Valid | xiiDDGIProbeFlags::Relocated | xiiDDGIProbeFlags::InsideGeometry);
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::Valid, bValid);
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::Relocated, !vClampedOffset.IsZero(0.0001f));
  probe.m_Flags.AddOrRemove(xiiDDGIProbeFlags::InsideGeometry, bInsideGeometry);
}

xiiArrayPtr<const xiiDDGIProbeState> xiiDDGIContext::GetProbes() const
{
  if (!IsInitialized())
    return {};
  return xiiArrayPtr<const xiiDDGIProbeState>(m_pState->m_Probes.GetData(), m_pState->m_Probes.GetCount());
}

xiiArrayPtr<const xiiDDGIProbeUpdate> xiiDDGIContext::GetScheduledUpdates() const
{
  if (!IsInitialized())
    return {};
  return xiiArrayPtr<const xiiDDGIProbeUpdate>(m_pState->m_ScheduledUpdates.GetData(), m_pState->m_ScheduledUpdates.GetCount());
}

xiiDDGIFrameStats xiiDDGIContext::GetFrameStats() const
{
  return IsInitialized() ? m_pState->m_Stats : xiiDDGIFrameStats();
}

const xiiDDGISettings& xiiDDGIContext::GetConfiguration() const
{
  XII_ASSERT_RELEASE(IsInitialized(), "DDGI context is not initialized.");
  return m_pState->m_Settings;
}

xiiVec3I32 xiiDDGIContext::GetMinimumCell() const
{
  return IsInitialized() ? m_pState->m_vMinimumCell : xiiVec3I32::MakeZero();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_DynamicGlobalIllumination);
