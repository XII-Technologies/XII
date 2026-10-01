/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>

namespace
{
  constexpr xiiUInt32 s_uiVolumetricCoordinateBits = 21U;
  constexpr xiiUInt32 s_uiVolumetricCoordinateBias = XII_BIT(s_uiVolumetricCoordinateBits - 1U);
  constexpr xiiUInt64 s_uiVolumetricCoordinateMask = XII_BIT(s_uiVolumetricCoordinateBits) - 1ULL;

  bool IsFiniteNonNegative(const xiiVec3& value)
  {
    return value.IsValid() && value.x >= 0.0f && value.y >= 0.0f && value.z >= 0.0f;
  }

  bool IsDescriptionValid(const xiiVolumetricMediumDescription& description)
  {
    return description.m_vCenter.IsValid() && description.m_vHalfExtents.IsValid() &&
      description.m_vHalfExtents.x > 0.0f && description.m_vHalfExtents.y > 0.0f && description.m_vHalfExtents.z > 0.0f &&
      IsFiniteNonNegative(description.m_vScattering) && IsFiniteNonNegative(description.m_vAbsorption) && IsFiniteNonNegative(description.m_vEmission) &&
      xiiMath::IsFinite(description.m_fAnisotropy) && description.m_fAnisotropy > -1.0f && description.m_fAnisotropy < 1.0f;
  }

  bool IsSettingsValid(const xiiVolumetricMediumSettings& settings)
  {
    return xiiMath::IsFinite(settings.m_fCellSizeMeters) && settings.m_fCellSizeMeters > 0.0f &&
      xiiMath::IsFinite(settings.m_fViewDistanceMeters) && settings.m_fViewDistanceMeters > 0.0f &&
      settings.m_uiMaxVisibleMedia > 0U && settings.m_uiMaxVisibleMedia <= 1024U &&
      settings.m_uiMaxCellsPerMedium > 0U && settings.m_uiMaxCellsPerMedium <= 65536U;
  }
} // namespace

class xiiVolumetricMediumManager::State
{
public:
  struct Slot
  {
    xiiVolumetricMediumDescription m_Description;
    xiiDynamicArray<xiiUInt64>      m_CellKeys;
    xiiUInt32                       m_uiGeneration = 1U;
    bool                            m_bAllocated  = false;
  };

  struct Cell
  {
    xiiDynamicArray<xiiUInt32> m_Media;
    bool                       m_bResident = true;
  };

  xiiMutex                                  m_Mutex;
  xiiVolumetricMediumSettings               m_Settings;
  xiiDynamicArray<Slot>                     m_Slots;
  xiiDynamicArray<xiiUInt32>                m_FreeSlots;
  xiiHashTable<xiiUInt64, Cell>             m_Cells;

  void RemoveSlotFromCells(xiiUInt32 uiSlot);
  [[nodiscard]] bool InsertSlotIntoCells(xiiUInt32 uiSlot);
};

xiiUniquePtr<xiiVolumetricMediumManager::State> xiiVolumetricMediumManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, VolumetricMediumManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiVolumetricMediumManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiVolumetricMediumManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVolumetricMediumShape, 1)
  XII_ENUM_CONSTANTS(xiiVolumetricMediumShape::Sphere, xiiVolumetricMediumShape::Box)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVolumetricMediumHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVolumetricMediumHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVolumetricMediumDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVolumetricMediumDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Shape", xiiVolumetricMediumShape, m_Shape),
    XII_MEMBER_PROPERTY("Center", m_vCenter),
    XII_MEMBER_PROPERTY("HalfExtents", m_vHalfExtents)->AddAttributes(new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("Scattering", m_vScattering)->AddAttributes(new xiiSuffixAttribute(" m^-1")),
    XII_MEMBER_PROPERTY("Absorption", m_vAbsorption)->AddAttributes(new xiiSuffixAttribute(" m^-1")),
    XII_MEMBER_PROPERTY("Emission", m_vEmission)->AddAttributes(new xiiSuffixAttribute(" cd/m^2")),
    XII_MEMBER_PROPERTY("Anisotropy", m_fAnisotropy)->AddAttributes(new xiiClampValueAttribute(-0.999f, 0.999f)),
    XII_MEMBER_PROPERTY("Priority", m_iPriority),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVolumetricMediumSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVolumetricMediumSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CellSizeMeters", m_fCellSizeMeters)->AddAttributes(new xiiClampValueAttribute(0.01f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ViewDistanceMeters", m_fViewDistanceMeters)->AddAttributes(new xiiClampValueAttribute(0.01f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MaxVisibleMedia", m_uiMaxVisibleMedia)->AddAttributes(new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MaxCellsPerMedium", m_uiMaxCellsPerMedium)->AddAttributes(new xiiClampValueAttribute(1U, 65536U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVolumetricMedium, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVolumetricMedium>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CenterAndShape", m_vCenterAndShape),
    XII_MEMBER_PROPERTY("HalfExtentsAndAnisotropy", m_vHalfExtentsAndAnisotropy),
    XII_MEMBER_PROPERTY("ScatteringAndPriority", m_vScatteringAndPriority),
    XII_MEMBER_PROPERTY("AbsorptionAndPadding", m_vAbsorptionAndPadding),
    XII_MEMBER_PROPERTY("EmissionAndPadding", m_vEmissionAndPadding),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVolumetricMediumStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVolumetricMediumStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RegisteredMedia", m_uiRegisteredMedia),
    XII_MEMBER_PROPERTY("ResidentCells", m_uiResidentCells),
    XII_MEMBER_PROPERTY("StreamedOutCells", m_uiStreamedOutCells),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiVolumetricMediumManager::State::RemoveSlotFromCells(xiiUInt32 uiSlot)
{
  auto& slot = m_Slots[uiSlot];
  for (xiiUInt64 uiKey : slot.m_CellKeys)
  {
    auto it = m_Cells.Find(uiKey);
    if (!it.IsValid())
      continue;

    it.Value().m_Media.RemoveAndCopy(uiSlot);
    if (it.Value().m_Media.IsEmpty() && it.Value().m_bResident)
      m_Cells.Remove(it);
  }
  slot.m_CellKeys.Clear();
}

bool xiiVolumetricMediumManager::State::InsertSlotIntoCells(xiiUInt32 uiSlot)
{
  auto&       slot      = m_Slots[uiSlot];
  const float fCellSize = m_Settings.m_fCellSizeMeters;
  const xiiVec3 vMinimum = slot.m_Description.m_vCenter - slot.m_Description.m_vHalfExtents;
  const xiiVec3 vMaximum = slot.m_Description.m_vCenter + slot.m_Description.m_vHalfExtents;
  const xiiVec3I32 vMinimumCell(static_cast<xiiInt32>(xiiMath::Floor(vMinimum.x / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor(vMinimum.y / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor(vMinimum.z / fCellSize)));
  const xiiVec3I32 vMaximumCell(static_cast<xiiInt32>(xiiMath::Floor(vMaximum.x / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor(vMaximum.y / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor(vMaximum.z / fCellSize)));
  const xiiUInt64 uiCellCount = static_cast<xiiUInt64>(vMaximumCell.x - vMinimumCell.x + 1) * static_cast<xiiUInt64>(vMaximumCell.y - vMinimumCell.y + 1) * static_cast<xiiUInt64>(vMaximumCell.z - vMinimumCell.z + 1);
  if (uiCellCount > m_Settings.m_uiMaxCellsPerMedium)
    return false;

  slot.m_CellKeys.Reserve(static_cast<xiiUInt32>(uiCellCount));
  for (xiiInt32 z = vMinimumCell.z; z <= vMaximumCell.z; ++z)
    for (xiiInt32 y = vMinimumCell.y; y <= vMaximumCell.y; ++y)
      for (xiiInt32 x = vMinimumCell.x; x <= vMaximumCell.x; ++x)
      {
        const xiiUInt64 uiKey = xiiVolumetricMediumManager::PackCellKey(xiiVec3I32(x, y, z));
        auto& cell = m_Cells[uiKey];
        cell.m_Media.PushBack(uiSlot);
        slot.m_CellKeys.PushBack(uiKey);
      }
  return true;
}

void xiiVolumetricMediumManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Volumetric medium manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

void xiiVolumetricMediumManager::Shutdown()
{
  s_pState.Clear();
}

bool xiiVolumetricMediumManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

xiiResult xiiVolumetricMediumManager::Configure(const xiiVolumetricMediumSettings& settings)
{
  if (s_pState == nullptr || !IsSettingsValid(settings))
    return XII_FAILURE;

  XII_LOCK(s_pState->m_Mutex);
  if (!s_pState->m_Slots.IsEmpty())
    return XII_FAILURE;
  s_pState->m_Settings = settings;
  return XII_SUCCESS;
}

const xiiVolumetricMediumSettings& xiiVolumetricMediumManager::GetConfiguration()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Volumetric medium subsystem is not started.");
  return s_pState->m_Settings;
}

xiiVolumetricMediumHandle xiiVolumetricMediumManager::RegisterMedium(const xiiVolumetricMediumDescription& description)
{
  if (s_pState == nullptr || !IsDescriptionValid(description))
    return {};

  XII_LOCK(s_pState->m_Mutex);
  const xiiUInt32 uiSlot = s_pState->m_FreeSlots.IsEmpty() ? s_pState->m_Slots.GetCount() : s_pState->m_FreeSlots.PeekBack();
  if (!s_pState->m_FreeSlots.IsEmpty())
    s_pState->m_FreeSlots.PopBack();
  else
    s_pState->m_Slots.ExpandAndGetRef();

  State::Slot& slot     = s_pState->m_Slots[uiSlot];
  slot.m_Description   = description;
  slot.m_bAllocated    = true;
  if (!s_pState->InsertSlotIntoCells(uiSlot))
  {
    slot.m_bAllocated = false;
    s_pState->m_FreeSlots.PushBack(uiSlot);
    return {};
  }
  return {uiSlot, slot.m_uiGeneration};
}

void xiiVolumetricMediumManager::UnregisterMedium(xiiVolumetricMediumHandle handle)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  if (handle.m_uiIndex >= s_pState->m_Slots.GetCount())
    return;
  State::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  if (!slot.m_bAllocated || slot.m_uiGeneration != handle.m_uiGeneration)
    return;
  s_pState->RemoveSlotFromCells(handle.m_uiIndex);
  slot.m_bAllocated = false;
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    ++slot.m_uiGeneration;
  s_pState->m_FreeSlots.PushBack(handle.m_uiIndex);
}

xiiResult xiiVolumetricMediumManager::UpdateMedium(xiiVolumetricMediumHandle handle, const xiiVolumetricMediumDescription& description)
{
  if (s_pState == nullptr || !IsDescriptionValid(description))
    return XII_FAILURE;
  XII_LOCK(s_pState->m_Mutex);
  if (handle.m_uiIndex >= s_pState->m_Slots.GetCount())
    return XII_FAILURE;
  State::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  if (!slot.m_bAllocated || slot.m_uiGeneration != handle.m_uiGeneration)
    return XII_FAILURE;

  const xiiVolumetricMediumDescription previous = slot.m_Description;
  s_pState->RemoveSlotFromCells(handle.m_uiIndex);
  slot.m_Description = description;
  if (s_pState->InsertSlotIntoCells(handle.m_uiIndex))
    return XII_SUCCESS;

  slot.m_Description = previous;
  XII_VERIFY(s_pState->InsertSlotIntoCells(handle.m_uiIndex), "Failed to restore a previously valid volumetric medium.");
  return XII_FAILURE;
}

bool xiiVolumetricMediumManager::IsValid(xiiVolumetricMediumHandle handle)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  return handle.m_uiIndex < s_pState->m_Slots.GetCount() && s_pState->m_Slots[handle.m_uiIndex].m_bAllocated && s_pState->m_Slots[handle.m_uiIndex].m_uiGeneration == handle.m_uiGeneration;
}

void xiiVolumetricMediumManager::SetCellResident(const xiiVec3I32& vCell, bool bResident)
{
  if (s_pState == nullptr)
    return;
  XII_LOCK(s_pState->m_Mutex);
  s_pState->m_Cells[PackCellKey(vCell)].m_bResident = bResident;
}

bool xiiVolumetricMediumManager::IsCellResident(const xiiVec3I32& vCell)
{
  if (s_pState == nullptr)
    return false;
  XII_LOCK(s_pState->m_Mutex);
  const State::Cell* pCell = s_pState->m_Cells.GetValue(PackCellKey(vCell));
  return pCell != nullptr && pCell->m_bResident;
}

void xiiVolumetricMediumManager::GatherGpuMedia(const xiiVec3& vViewPosition, xiiGpuVolumetricMediumArray& out_media)
{
  out_media.Clear();
  if (s_pState == nullptr || !vViewPosition.IsValid())
    return;

  struct Candidate
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiSlot;
    xiiInt32  m_iPriority;
    float     m_fDistanceSquared;
  };

  XII_LOCK(s_pState->m_Mutex);
  const float fCellSize = s_pState->m_Settings.m_fCellSizeMeters;
  const float fRange = s_pState->m_Settings.m_fViewDistanceMeters;
  const xiiVec3I32 vMinimumCell(static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.x - fRange) / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.y - fRange) / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.z - fRange) / fCellSize)));
  const xiiVec3I32 vMaximumCell(static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.x + fRange) / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.y + fRange) / fCellSize)), static_cast<xiiInt32>(xiiMath::Floor((vViewPosition.z + fRange) / fCellSize)));

  xiiHashSet<xiiUInt32> visited;
  xiiDynamicArray<Candidate> candidates;
  for (xiiInt32 z = vMinimumCell.z; z <= vMaximumCell.z; ++z)
    for (xiiInt32 y = vMinimumCell.y; y <= vMaximumCell.y; ++y)
      for (xiiInt32 x = vMinimumCell.x; x <= vMaximumCell.x; ++x)
      {
        const State::Cell* pCell = s_pState->m_Cells.GetValue(PackCellKey(xiiVec3I32(x, y, z)));
        if (pCell == nullptr || !pCell->m_bResident)
          continue;
        for (xiiUInt32 uiSlot : pCell->m_Media)
        {
          if (visited.Contains(uiSlot))
            continue;
          visited.Insert(uiSlot);
          const State::Slot& slot = s_pState->m_Slots[uiSlot];
          if (!slot.m_bAllocated || !slot.m_Description.m_bEnabled)
            continue;
          const float fDistanceSquared = (slot.m_Description.m_vCenter - vViewPosition).GetLengthSquared();
          if (fDistanceSquared > xiiMath::Square(fRange + slot.m_Description.m_vHalfExtents.GetLength()))
            continue;
          candidates.PushBack({uiSlot, slot.m_Description.m_iPriority, fDistanceSquared});
        }
      }

  candidates.Sort([](const Candidate& lhs, const Candidate& rhs) {
    return lhs.m_iPriority != rhs.m_iPriority ? lhs.m_iPriority > rhs.m_iPriority :
      (lhs.m_fDistanceSquared != rhs.m_fDistanceSquared ? lhs.m_fDistanceSquared < rhs.m_fDistanceSquared : lhs.m_uiSlot < rhs.m_uiSlot);
  });

  const xiiUInt32 uiCount = xiiMath::Min(candidates.GetCount(), s_pState->m_Settings.m_uiMaxVisibleMedia);
  out_media.SetCountUninitialized(uiCount);
  for (xiiUInt32 i = 0U; i < uiCount; ++i)
  {
    const auto& description = s_pState->m_Slots[candidates[i].m_uiSlot].m_Description;
    auto& gpu = out_media[i];
    gpu.m_vCenterAndShape             = xiiVec4(description.m_vCenter, static_cast<float>(description.m_Shape.GetValue()));
    gpu.m_vHalfExtentsAndAnisotropy  = xiiVec4(description.m_vHalfExtents, description.m_fAnisotropy);
    gpu.m_vScatteringAndPriority     = xiiVec4(description.m_vScattering, static_cast<float>(description.m_iPriority));
    gpu.m_vAbsorptionAndPadding      = xiiVec4(description.m_vAbsorption, 0.0f);
    gpu.m_vEmissionAndPadding        = xiiVec4(description.m_vEmission, 0.0f);
  }
}

xiiVolumetricMediumStats xiiVolumetricMediumManager::GetStats()
{
  xiiVolumetricMediumStats stats;
  if (s_pState == nullptr)
    return stats;
  XII_LOCK(s_pState->m_Mutex);
  stats.m_uiRegisteredMedia = s_pState->m_Slots.GetCount() - s_pState->m_FreeSlots.GetCount();
  for (auto it = s_pState->m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_bResident)
      ++stats.m_uiResidentCells;
    else
      ++stats.m_uiStreamedOutCells;
  }
  return stats;
}

xiiUInt64 xiiVolumetricMediumManager::PackCellKey(const xiiVec3I32& vCell)
{
  XII_ASSERT_DEV(vCell.x >= -static_cast<xiiInt32>(s_uiVolumetricCoordinateBias) && vCell.x < static_cast<xiiInt32>(s_uiVolumetricCoordinateBias) &&
                   vCell.y >= -static_cast<xiiInt32>(s_uiVolumetricCoordinateBias) && vCell.y < static_cast<xiiInt32>(s_uiVolumetricCoordinateBias) &&
                   vCell.z >= -static_cast<xiiInt32>(s_uiVolumetricCoordinateBias) && vCell.z < static_cast<xiiInt32>(s_uiVolumetricCoordinateBias),
    "Volumetric medium cell coordinate exceeds the packed-key range.");
  return ((static_cast<xiiUInt64>(vCell.x + s_uiVolumetricCoordinateBias) & s_uiVolumetricCoordinateMask) << (s_uiVolumetricCoordinateBits * 2U)) |
    ((static_cast<xiiUInt64>(vCell.y + s_uiVolumetricCoordinateBias) & s_uiVolumetricCoordinateMask) << s_uiVolumetricCoordinateBits) |
    (static_cast<xiiUInt64>(vCell.z + s_uiVolumetricCoordinateBias) & s_uiVolumetricCoordinateMask);
}

void xiiVolumetricMediumManager::UnpackCellKey(xiiUInt64 uiKey, xiiVec3I32& out_vCell)
{
  out_vCell.x = static_cast<xiiInt32>((uiKey >> (s_uiVolumetricCoordinateBits * 2U)) & s_uiVolumetricCoordinateMask) - static_cast<xiiInt32>(s_uiVolumetricCoordinateBias);
  out_vCell.y = static_cast<xiiInt32>((uiKey >> s_uiVolumetricCoordinateBits) & s_uiVolumetricCoordinateMask) - static_cast<xiiInt32>(s_uiVolumetricCoordinateBias);
  out_vCell.z = static_cast<xiiInt32>(uiKey & s_uiVolumetricCoordinateMask) - static_cast<xiiInt32>(s_uiVolumetricCoordinateBias);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_VolumetricMedium);
