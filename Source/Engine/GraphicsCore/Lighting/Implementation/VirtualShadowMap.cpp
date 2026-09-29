/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>

namespace
{
  constexpr xiiUInt32 s_uiMaximumLightId = (1U << 24U) - 1U;
  constexpr xiiUInt32 s_uiMaximumMipLevel = (1U << 6U) - 1U;
  constexpr xiiUInt32 s_uiMaximumPageCoordinate = (1U << 17U) - 1U;

  bool IsConfigurationValid(const xiiVirtualShadowMapSettings& settings)
  {
    if (!xiiMath::IsPowerOf2(settings.m_uiVirtualResolution) || !xiiMath::IsPowerOf2(settings.m_uiPageSize))
      return false;
    if (settings.m_uiVirtualResolution < settings.m_uiPageSize || settings.m_uiVirtualResolution % settings.m_uiPageSize != 0U)
      return false;
    if (settings.m_uiVirtualResolution / settings.m_uiPageSize > s_uiMaximumPageCoordinate + 1U)
      return false;
    return settings.m_uiPhysicalPageCount > 0U && settings.m_uiMaxFeedbackRequests > 0U && settings.m_uiMaxPageAllocations > 0U && settings.m_uiFramesInFlight > 0U;
  }
} // namespace

class xiiVirtualShadowMapManagerState
{
public:
  struct Slot
  {
    xiiVirtualShadowPageMapping m_Mapping;
    bool                        m_bAllocated = false;
  };

  xiiVirtualShadowMapSettings              m_Settings;
  xiiDynamicArray<Slot>                    m_Slots;
  xiiDynamicArray<xiiUInt32>               m_FreePages;
  xiiHashTable<xiiUInt64, xiiUInt32>       m_PageLookup;
  xiiDynamicArray<xiiVirtualShadowPageUpdate>  m_PageTableUpdates;
  xiiDynamicArray<xiiVirtualShadowPageMapping> m_DirtyPages;
  xiiVirtualShadowMapStats                 m_Stats;
  xiiUInt64                                m_uiFrameIndex = 0U;
  xiiUInt64                                m_uiCompletedFrame = 0U;
  bool                                     m_bInitialized = false;
};

xiiUniquePtr<xiiVirtualShadowMapManagerState> xiiVirtualShadowMapManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, VirtualShadowMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiVirtualShadowMapManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiVirtualShadowMapManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowMapSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowMapSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VirtualResolution", m_uiVirtualResolution)->AddAttributes(new xiiDefaultValueAttribute(16384U), new xiiClampValueAttribute(128U, 131072U)),
    XII_MEMBER_PROPERTY("PageSize", m_uiPageSize)->AddAttributes(new xiiDefaultValueAttribute(128U), new xiiClampValueAttribute(32U, 512U)),
    XII_MEMBER_PROPERTY("PhysicalPageCount", m_uiPhysicalPageCount)->AddAttributes(new xiiDefaultValueAttribute(4096U), new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("MaxFeedbackRequests", m_uiMaxFeedbackRequests)->AddAttributes(new xiiDefaultValueAttribute(16384U), new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("MaxPageAllocations", m_uiMaxPageAllocations)->AddAttributes(new xiiDefaultValueAttribute(512U), new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowPageId, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowPageId>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LightId", m_uiLightId),
    XII_MEMBER_PROPERTY("MipLevel", m_uiMipLevel),
    XII_MEMBER_PROPERTY("PageX", m_uiPageX),
    XII_MEMBER_PROPERTY("PageY", m_uiPageY),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowPageRequest, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowPageRequest>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Page", m_Page),
    XII_MEMBER_PROPERTY("Priority", m_uiPriority),
    XII_MEMBER_PROPERTY("Pinned", m_bPinned),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVirtualShadowPageUpdateType, 1)
  XII_ENUM_CONSTANTS(xiiVirtualShadowPageUpdateType::Map, xiiVirtualShadowPageUpdateType::Unmap)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowPageUpdate, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowPageUpdate>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Page", m_Page),
    XII_MEMBER_PROPERTY("PhysicalPage", m_uiPhysicalPage),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiVirtualShadowPageUpdateType, m_Type),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowPageMapping, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowPageMapping>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Page", m_Page),
    XII_MEMBER_PROPERTY("PhysicalPage", m_uiPhysicalPage),
    XII_MEMBER_PROPERTY("LastUsedFrame", m_uiLastUsedFrame),
    XII_MEMBER_PROPERTY("Priority", m_uiPriority),
    XII_MEMBER_PROPERTY("Pinned", m_bPinned),
    XII_MEMBER_PROPERTY("NeedsRendering", m_bNeedsRendering),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiVirtualShadowMapStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowMapStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ResidentPageCount", m_uiResidentPageCount),
    XII_MEMBER_PROPERTY("FreePageCount", m_uiFreePageCount),
    XII_MEMBER_PROPERTY("FeedbackRequestCount", m_uiFeedbackRequestCount),
    XII_MEMBER_PROPERTY("UniqueRequestCount", m_uiUniqueRequestCount),
    XII_MEMBER_PROPERTY("AllocationCount", m_uiAllocationCount),
    XII_MEMBER_PROPERTY("EvictionCount", m_uiEvictionCount),
    XII_MEMBER_PROPERTY("DroppedRequestCount", m_uiDroppedRequestCount),
    XII_MEMBER_PROPERTY("DirtyPageCount", m_uiDirtyPageCount),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiVirtualShadowPageId::IsValid() const
{
  return m_uiLightId <= s_uiMaximumLightId && m_uiMipLevel <= s_uiMaximumMipLevel && m_uiPageX <= s_uiMaximumPageCoordinate && m_uiPageY <= s_uiMaximumPageCoordinate;
}

xiiUInt64 xiiVirtualShadowPageId::GetPackedValue() const
{
  if (!IsValid())
    return xiiMath::MaxValue<xiiUInt64>();

  return static_cast<xiiUInt64>(m_uiLightId) |
         (static_cast<xiiUInt64>(m_uiMipLevel) << 24U) |
         (static_cast<xiiUInt64>(m_uiPageX) << 30U) |
         (static_cast<xiiUInt64>(m_uiPageY) << 47U);
}

void xiiVirtualShadowMapManager::Startup()
{
  s_pState = XII_DEFAULT_NEW(xiiVirtualShadowMapManagerState);
  Configure(xiiVirtualShadowMapSettings()).IgnoreResult();
}

void xiiVirtualShadowMapManager::Shutdown()
{
  s_pState.Clear();
}

xiiResult xiiVirtualShadowMapManager::Configure(const xiiVirtualShadowMapSettings& settings)
{
  if (s_pState == nullptr || !IsConfigurationValid(settings))
    return XII_FAILURE;

  s_pState->m_Settings = settings;
  s_pState->m_Slots.Clear();
  s_pState->m_Slots.SetCount(settings.m_uiPhysicalPageCount);
  s_pState->m_FreePages.Clear();
  s_pState->m_FreePages.Reserve(settings.m_uiPhysicalPageCount);
  for (xiiUInt32 uiPage = settings.m_uiPhysicalPageCount; uiPage > 0U; --uiPage)
    s_pState->m_FreePages.PushBack(uiPage - 1U);

  s_pState->m_PageLookup.Clear();
  s_pState->m_PageLookup.Reserve(settings.m_uiPhysicalPageCount);
  s_pState->m_PageTableUpdates.Clear();
  s_pState->m_DirtyPages.Clear();
  s_pState->m_Stats = {};
  s_pState->m_Stats.m_uiFreePageCount = settings.m_uiPhysicalPageCount;
  s_pState->m_uiFrameIndex = 0U;
  s_pState->m_uiCompletedFrame = 0U;
  s_pState->m_bInitialized = true;
  return XII_SUCCESS;
}

bool xiiVirtualShadowMapManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

void xiiVirtualShadowMapManager::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  if (!IsInitialized())
    return;

  XII_ASSERT_DEV(uiCompletedFrame <= uiFrameIndex, "The completed GPU frame cannot be newer than the current frame.");
  s_pState->m_uiFrameIndex = uiFrameIndex;
  s_pState->m_uiCompletedFrame = xiiMath::Min(uiCompletedFrame, uiFrameIndex);
  s_pState->m_PageTableUpdates.Clear();
  s_pState->m_Stats.m_uiFeedbackRequestCount = 0U;
  s_pState->m_Stats.m_uiUniqueRequestCount = 0U;
  s_pState->m_Stats.m_uiAllocationCount = 0U;
  s_pState->m_Stats.m_uiEvictionCount = 0U;
  s_pState->m_Stats.m_uiDroppedRequestCount = 0U;
}

void xiiVirtualShadowMapManager::SubmitFeedback(xiiArrayPtr<const xiiVirtualShadowPageRequest> requests)
{
  if (!IsInitialized() || requests.IsEmpty())
    return;

  const xiiUInt32 uiRequestCount = xiiMath::Min(requests.GetCount(), s_pState->m_Settings.m_uiMaxFeedbackRequests);
  s_pState->m_Stats.m_uiFeedbackRequestCount += uiRequestCount;
  s_pState->m_Stats.m_uiDroppedRequestCount += requests.GetCount() - uiRequestCount;

  xiiDynamicArray<xiiVirtualShadowPageRequest> sortedRequests;
  sortedRequests.Reserve(uiRequestCount);
  for (xiiUInt32 i = 0U; i < uiRequestCount; ++i)
    sortedRequests.PushBack(requests[i]);

  sortedRequests.Sort([](const xiiVirtualShadowPageRequest& lhs, const xiiVirtualShadowPageRequest& rhs) {
    if (lhs.m_uiPriority != rhs.m_uiPriority)
      return lhs.m_uiPriority > rhs.m_uiPriority;
    return lhs.m_Page.GetPackedValue() < rhs.m_Page.GetPackedValue();
  });

  xiiHashTable<xiiUInt64, xiiUInt8> uniqueRequests;
  uniqueRequests.Reserve(uiRequestCount);
  xiiUInt32 uiAllocationsRemaining = s_pState->m_Settings.m_uiMaxPageAllocations;
  const xiiUInt32 uiBasePagesPerAxis = s_pState->m_Settings.m_uiVirtualResolution / s_pState->m_Settings.m_uiPageSize;

  for (const xiiVirtualShadowPageRequest& request : sortedRequests)
  {
    const xiiUInt64 uiKey = request.m_Page.GetPackedValue();
    const xiiUInt32 uiPagesPerAxis = xiiMath::Max(uiBasePagesPerAxis >> xiiMath::Min(request.m_Page.m_uiMipLevel, 31U), 1U);
    if (!request.m_Page.IsValid() || request.m_Page.m_uiPageX >= uiPagesPerAxis || request.m_Page.m_uiPageY >= uiPagesPerAxis)
    {
      ++s_pState->m_Stats.m_uiDroppedRequestCount;
      continue;
    }
    if (uniqueRequests.Contains(uiKey))
      continue;
    uniqueRequests.Insert(uiKey, 1U);
    ++s_pState->m_Stats.m_uiUniqueRequestCount;

    xiiUInt32 uiPhysicalPage = xiiInvalidIndex;
    if (s_pState->m_PageLookup.TryGetValue(uiKey, uiPhysicalPage))
    {
      auto& mapping = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
      mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
      mapping.m_uiPriority = xiiMath::Max(mapping.m_uiPriority, request.m_uiPriority);
      mapping.m_bPinned |= request.m_bPinned;
      continue;
    }

    if (uiAllocationsRemaining == 0U)
    {
      ++s_pState->m_Stats.m_uiDroppedRequestCount;
      continue;
    }

    if (!s_pState->m_FreePages.IsEmpty())
    {
      uiPhysicalPage = s_pState->m_FreePages.PeekBack();
      s_pState->m_FreePages.PopBack();
    }
    else
    {
      xiiUInt64 uiOldestFrame = xiiMath::MaxValue<xiiUInt64>();
      for (xiiUInt32 uiCandidate = 0U; uiCandidate < s_pState->m_Slots.GetCount(); ++uiCandidate)
      {
        const auto& slot = s_pState->m_Slots[uiCandidate];
        if (!slot.m_bAllocated || slot.m_Mapping.m_bPinned || slot.m_Mapping.m_uiLastUsedFrame > s_pState->m_uiCompletedFrame)
          continue;
        if (slot.m_Mapping.m_uiLastUsedFrame < uiOldestFrame || (slot.m_Mapping.m_uiLastUsedFrame == uiOldestFrame && uiCandidate < uiPhysicalPage))
        {
          uiOldestFrame = slot.m_Mapping.m_uiLastUsedFrame;
          uiPhysicalPage = uiCandidate;
        }
      }

      if (uiPhysicalPage == xiiInvalidIndex)
      {
        ++s_pState->m_Stats.m_uiDroppedRequestCount;
        continue;
      }

      auto& evicted = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
      s_pState->m_PageLookup.Remove(evicted.m_Page.GetPackedValue());
      for (xiiUInt32 i = 0U; i < s_pState->m_DirtyPages.GetCount(); ++i)
      {
        if (s_pState->m_DirtyPages[i].m_uiPhysicalPage == uiPhysicalPage)
        {
          s_pState->m_DirtyPages.RemoveAtAndSwap(i);
          break;
        }
      }
      s_pState->m_PageTableUpdates.PushBack({evicted.m_Page, uiPhysicalPage, xiiVirtualShadowPageUpdateType::Unmap});
      ++s_pState->m_Stats.m_uiEvictionCount;
    }

    auto& slot = s_pState->m_Slots[uiPhysicalPage];
    slot.m_bAllocated = true;
    slot.m_Mapping.m_Page = request.m_Page;
    slot.m_Mapping.m_uiPhysicalPage = uiPhysicalPage;
    slot.m_Mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
    slot.m_Mapping.m_uiPriority = request.m_uiPriority;
    slot.m_Mapping.m_bPinned = request.m_bPinned;
    slot.m_Mapping.m_bNeedsRendering = true;
    s_pState->m_PageLookup.Insert(uiKey, uiPhysicalPage);
    s_pState->m_DirtyPages.PushBack(slot.m_Mapping);
    s_pState->m_PageTableUpdates.PushBack({request.m_Page, uiPhysicalPage, xiiVirtualShadowPageUpdateType::Map});
    --uiAllocationsRemaining;
    ++s_pState->m_Stats.m_uiAllocationCount;
  }

  s_pState->m_Stats.m_uiResidentPageCount = s_pState->m_PageLookup.GetCount();
  s_pState->m_Stats.m_uiFreePageCount = s_pState->m_FreePages.GetCount();
  s_pState->m_Stats.m_uiDirtyPageCount = s_pState->m_DirtyPages.GetCount();
}

bool xiiVirtualShadowMapManager::TryGetMapping(const xiiVirtualShadowPageId& page, xiiVirtualShadowPageMapping& out_mapping)
{
  if (!IsInitialized())
    return false;

  xiiUInt32 uiPhysicalPage = xiiInvalidIndex;
  if (!s_pState->m_PageLookup.TryGetValue(page.GetPackedValue(), uiPhysicalPage))
    return false;

  out_mapping = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
  return true;
}

xiiArrayPtr<const xiiVirtualShadowPageUpdate> xiiVirtualShadowMapManager::GetPageTableUpdates()
{
  if (!IsInitialized())
    return {};

  return xiiArrayPtr<const xiiVirtualShadowPageUpdate>(s_pState->m_PageTableUpdates.GetData(), s_pState->m_PageTableUpdates.GetCount());
}

xiiArrayPtr<const xiiVirtualShadowPageMapping> xiiVirtualShadowMapManager::GetDirtyPages()
{
  if (!IsInitialized())
    return {};

  return xiiArrayPtr<const xiiVirtualShadowPageMapping>(s_pState->m_DirtyPages.GetData(), s_pState->m_DirtyPages.GetCount());
}

void xiiVirtualShadowMapManager::MarkPageRendered(xiiUInt32 uiPhysicalPage)
{
  if (!IsInitialized() || uiPhysicalPage >= s_pState->m_Slots.GetCount() || !s_pState->m_Slots[uiPhysicalPage].m_bAllocated)
    return;

  auto& mapping = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
  mapping.m_bNeedsRendering = false;
  for (xiiUInt32 i = 0U; i < s_pState->m_DirtyPages.GetCount(); ++i)
  {
    if (s_pState->m_DirtyPages[i].m_uiPhysicalPage == uiPhysicalPage)
    {
      s_pState->m_DirtyPages.RemoveAtAndSwap(i);
      break;
    }
  }
  s_pState->m_Stats.m_uiDirtyPageCount = s_pState->m_DirtyPages.GetCount();
}

xiiVirtualShadowMapStats xiiVirtualShadowMapManager::GetStats()
{
  return IsInitialized() ? s_pState->m_Stats : xiiVirtualShadowMapStats();
}

const xiiVirtualShadowMapSettings& xiiVirtualShadowMapManager::GetConfiguration()
{
  XII_ASSERT_RELEASE(IsInitialized(), "Virtual shadow-map manager is not initialized.");
  return s_pState->m_Settings;
}
