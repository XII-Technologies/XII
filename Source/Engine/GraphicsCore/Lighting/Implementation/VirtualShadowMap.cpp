/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>


namespace
{
  xiiUInt64 NextRasterRevision()
  {
    static xiiAtomicInteger64 revision;
    return static_cast<xiiUInt64>(revision.Increment());
  }

  constexpr xiiUInt32 s_uiMaximumLightId        = (1U << 24U) - 1U;
  constexpr xiiUInt32 s_uiMaximumMipLevel       = (1U << 6U) - 1U;
  constexpr xiiUInt32 s_uiMaximumPageCoordinate = (1U << 17U) - 1U;

  xiiUInt32 HashVirtualPageKey(xiiUInt32 uiKeyLow, xiiUInt32 uiKeyHigh)
  {
    xiiUInt32 uiHash = uiKeyLow ^ (uiKeyHigh * 0x9E3779B9U);
    uiHash ^= uiHash >> 16U;
    uiHash *= 0x7FEB352DU;
    uiHash ^= uiHash >> 15U;
    uiHash *= 0x846CA68BU;
    uiHash ^= uiHash >> 16U;
    return uiHash;
  }

  bool IsConfigurationValid(const xiiVirtualShadowMapSettings& settings)
  {
    if (!xiiMath::IsPowerOf2(settings.m_uiVirtualResolution) || !xiiMath::IsPowerOf2(settings.m_uiPageSize))
      return false;
    if (settings.m_uiVirtualResolution < settings.m_uiPageSize || settings.m_uiVirtualResolution % settings.m_uiPageSize != 0U)
      return false;
    if (settings.m_uiVirtualResolution / settings.m_uiPageSize > s_uiMaximumPageCoordinate + 1U)
      return false;
    return settings.m_uiPhysicalPageCount > 0U && settings.m_uiMaxFeedbackRequests > 0U && settings.m_uiMaxPageAllocations > 0U &&
      settings.m_uiMaxPageRasterizations > 0U && settings.m_uiFramesInFlight > 0U && settings.m_uiFramesInFlight <= 64U &&
      settings.m_uiPhysicalPageCount <= (1U << 26);
  }
} // namespace

class xiiVirtualShadowMapManagerState
{
public:
  struct Slot
  {
    xiiVirtualShadowPageMapping m_Mapping;
    bool                        m_bAllocated           = false;
    xiiUInt64                   m_uiDirtyFrameMask     = 0U;
    xiiUInt64                   m_uiUploadRevision     = 0U;
    xiiUInt64                   m_uiLastCapturedFrame  = 0U;
    xiiUInt32                   m_uiVirtualTableBucket = xiiInvalidIndex;
    xiiUInt64                   m_uiContentRevision    = 0;
    xiiUInt64                   m_uiPendingRevision    = 0;
    xiiUInt64                   m_uiPlannedFrame       = xiiMath::MaxValue<xiiUInt64>();
  };

  struct PendingRaster
  {
    xiiVirtualShadowRasterTicket m_Work;
    xiiSharedPtr<xiiGALFence>    m_Fence;
  };
  struct PendingUpload
  {
    xiiSharedPtr<const xiiVirtualShadowTableUpload> m_Upload;
    xiiSharedPtr<xiiGALFence>                       m_Completion;
  };
  struct RetiredPage
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiPhysicalPage;
    xiiUInt64 m_uiLastCapturedFrame;
  };
  xiiDynamicArray<RetiredPage>                    m_RetiredPages;
  xiiDynamicArray<PendingUpload>                  m_PendingUploads;
  xiiDynamicArray<PendingRaster>                  m_PendingRasters;
  xiiSharedPtr<const xiiVirtualShadowTableUpload> m_PreparedUpload;
  xiiDynamicArray<xiiGpuVirtualShadowPage>        m_VirtualRecords;
  xiiDynamicArray<xiiUInt64>                      m_UploadedVirtualRevisions;
  xiiUInt64                                       m_uiResidencyRevision = 0, m_uiCpuTableRevision = 0;
  xiiUInt64                                       m_uiRecordedUploadFrame = xiiMath::MaxValue<xiiUInt64>(), m_uiRecordedUploadRevision = 0;


  xiiUInt64                                    m_uiConfigurationRevision = 0;
  xiiVirtualShadowMapSettings                  m_Settings;
  xiiDynamicArray<Slot>                        m_Slots;
  xiiDynamicArray<xiiUInt32>                   m_FreePages;
  xiiHashTable<xiiUInt64, xiiUInt32>           m_PageLookup;
  xiiDynamicArray<xiiVirtualShadowPageMapping> m_DirtyPages;
  xiiVirtualShadowMapStats                     m_Stats;
  xiiSharedPtr<xiiGALBuffer>                   m_pPhysicalPageTable;
  xiiSharedPtr<xiiGALBuffer>                   m_pVirtualPageTable;
  xiiSharedPtr<xiiGALTexture>                  m_pPhysicalAtlas;
  xiiUInt64                                    m_uiFrameIndex               = 0U;
  xiiUInt64                                    m_uiCompletedFrame           = 0U;
  xiiUInt64                                    m_uiAllFrameMask             = 0U;
  xiiUInt32                                    m_uiRasterizationsThisFrame  = 0U;
  xiiUInt32                                    m_uiRetirementsThisFrame     = 0U;
  xiiUInt32                                    m_uiPhysicalPagesPerRow      = 0U;
  xiiUInt32                                    m_uiPhysicalPageRows         = 0U;
  xiiUInt32                                    m_uiVirtualPageTableCapacity = 0U;
  bool                                         m_bEngineStarted             = false;
  bool                                         m_bInitialized               = false;
};

xiiUniquePtr<xiiVirtualShadowMapManagerState> xiiVirtualShadowMapManager::s_pState;

namespace
{
  void DirtyTableSlot(xiiVirtualShadowMapManagerState& state, xiiUInt32 index)
  {
    auto& slot                  = state.m_Slots[index];
    slot.m_uiDirtyFrameMask     = state.m_uiAllFrameMask;
    slot.m_uiUploadRevision     = NextRasterRevision();
    state.m_uiResidencyRevision = slot.m_uiUploadRevision;
  }

  bool InvalidateSlot(xiiVirtualShadowMapManagerState& state, xiiUInt32 uiPhysicalPage)
  {
    if (uiPhysicalPage >= state.m_Slots.GetCount() || !state.m_Slots[uiPhysicalPage].m_bAllocated)
      return false;

    auto& slot = state.m_Slots[uiPhysicalPage];
    DirtyTableSlot(state, uiPhysicalPage);
    slot.m_uiContentRevision = NextRasterRevision();
    slot.m_uiPlannedFrame    = xiiMath::MaxValue<xiiUInt64>();
    if (slot.m_Mapping.m_bNeedsRendering)
      return false;

    slot.m_Mapping.m_bNeedsRendering = true;
    state.m_DirtyPages.PushBack(slot.m_Mapping);
    ++state.m_Stats.m_uiInvalidationCount;
    state.m_Stats.m_uiDirtyPageCount = state.m_DirtyPages.GetCount();
    return true;
  }
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, VirtualShadowMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiVirtualShadowMapManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiVirtualShadowMapManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiVirtualShadowMapManager::EngineShutdown();
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
    XII_MEMBER_PROPERTY("MaxPageRasterizations", m_uiMaxPageRasterizations)->AddAttributes(new xiiDefaultValueAttribute(128U), new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVirtualShadowPage, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVirtualShadowPage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VirtualKeyLow", m_uiVirtualKeyLow),
    XII_MEMBER_PROPERTY("VirtualKeyHigh", m_uiVirtualKeyHigh),
    XII_MEMBER_PROPERTY("PhysicalPage", m_uiPhysicalPage),
    XII_MEMBER_PROPERTY("Flags", m_uiFlags),
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
    XII_MEMBER_PROPERTY("RetiredPageCount", m_uiRetiredPageCount),
    XII_MEMBER_PROPERTY("FeedbackRequestCount", m_uiFeedbackRequestCount),
    XII_MEMBER_PROPERTY("UniqueRequestCount", m_uiUniqueRequestCount),
    XII_MEMBER_PROPERTY("AllocationCount", m_uiAllocationCount),
    XII_MEMBER_PROPERTY("EvictionCount", m_uiEvictionCount),
    XII_MEMBER_PROPERTY("DroppedRequestCount", m_uiDroppedRequestCount),
    XII_MEMBER_PROPERTY("InvalidationCount", m_uiInvalidationCount),
    XII_MEMBER_PROPERTY("RasterizedPageCount", m_uiRasterizedPageCount),
    XII_MEMBER_PROPERTY("DirtyPageCount", m_uiDirtyPageCount),
    XII_MEMBER_PROPERTY("PhysicalAtlasWidth", m_uiPhysicalAtlasWidth),
    XII_MEMBER_PROPERTY("PhysicalAtlasHeight", m_uiPhysicalAtlasHeight),
    XII_MEMBER_PROPERTY("VirtualPageTableCapacity", m_uiVirtualPageTableCapacity),
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

xiiVirtualShadowPageId xiiVirtualShadowPageId::FromPackedValue(xiiUInt64 uiPackedValue)
{
  xiiVirtualShadowPageId page;
  page.m_uiLightId  = static_cast<xiiUInt32>(uiPackedValue & 0x00FFFFFFULL);
  page.m_uiMipLevel = static_cast<xiiUInt32>((uiPackedValue >> 24U) & 0x3FULL);
  page.m_uiPageX    = static_cast<xiiUInt32>((uiPackedValue >> 30U) & 0x1FFFFULL);
  page.m_uiPageY    = static_cast<xiiUInt32>((uiPackedValue >> 47U) & 0x1FFFFULL);
  return page;
}

void xiiVirtualShadowMapManager::Startup()
{
  s_pState = XII_DEFAULT_NEW(xiiVirtualShadowMapManagerState);
  Configure(xiiVirtualShadowMapSettings()).IgnoreResult();
}

void xiiVirtualShadowMapManager::EngineStartup()
{
  s_pState->m_bEngineStarted = true;
}

void xiiVirtualShadowMapManager::EngineShutdown()
{
  if (s_pState != nullptr)
  {
    s_pState->m_PendingUploads.Clear();
    s_pState->m_PendingRasters.Clear();
    s_pState->m_PreparedUpload.Clear();
    s_pState->m_pPhysicalAtlas.Clear();
    s_pState->m_pVirtualPageTable.Clear();
    s_pState->m_pPhysicalPageTable.Clear();
    // Captures retain old allocations; a restarted device gets a distinct atlas.
    for (const auto& retired : s_pState->m_RetiredPages)
      s_pState->m_FreePages.PushBack(retired.m_uiPhysicalPage);
    s_pState->m_RetiredPages.Clear();
    s_pState->m_Stats.m_uiRetiredPageCount = 0;
    s_pState->m_Stats.m_uiFreePageCount    = s_pState->m_FreePages.GetCount();
    for (auto& slot : s_pState->m_Slots)
      slot.m_uiLastCapturedFrame = 0;
    s_pState->m_bEngineStarted = false;
  }
}

void xiiVirtualShadowMapManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiVirtualShadowMapManager::CreateGpuResources()
{
  if (s_pState == nullptr || !s_pState->m_bEngineStarted)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice               = xiiGALDevice::GetDefaultDevice();
  const xiiUInt64                  uiPhysicalRecordCount = static_cast<xiiUInt64>(s_pState->m_Settings.m_uiPhysicalPageCount) * s_pState->m_Settings.m_uiFramesInFlight;
  const xiiUInt64                  uiPhysicalBufferSize  = uiPhysicalRecordCount * sizeof(xiiGpuVirtualShadowPage);
  const xiiUInt64                  uiVirtualRecordCount  = static_cast<xiiUInt64>(s_pState->m_uiVirtualPageTableCapacity) * s_pState->m_Settings.m_uiFramesInFlight;
  const xiiUInt64                  uiVirtualBufferSize   = uiVirtualRecordCount * sizeof(xiiGpuVirtualShadowPage);
  if (pDevice == nullptr || uiPhysicalBufferSize > xiiMath::MaxValue<xiiUInt32>() || uiVirtualBufferSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  const xiiUInt64 uiAtlasWidth              = static_cast<xiiUInt64>(s_pState->m_uiPhysicalPagesPerRow) * s_pState->m_Settings.m_uiPageSize;
  const xiiUInt64 uiAtlasHeight             = static_cast<xiiUInt64>(s_pState->m_uiPhysicalPageRows) * s_pState->m_Settings.m_uiPageSize;
  const xiiUInt32 uiMaximumTextureDimension = pDevice->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension;
  if (uiAtlasWidth == 0U || uiAtlasHeight == 0U || uiAtlasWidth > uiMaximumTextureDimension || uiAtlasHeight > uiMaximumTextureDimension)
    return XII_FAILURE;

  xiiGALBufferCreationDescription description;
  description.m_uiSize                          = static_cast<xiiUInt32>(uiPhysicalBufferSize);
  description.m_uiElementByteStride             = sizeof(xiiGpuVirtualShadowPage);
  description.m_BindFlags                       = xiiGALBindFlags::ShaderResource;
  description.m_Mode                            = xiiGALBufferMode::Structured;
  description.m_Usage                           = xiiGALResourceUsage::Mutable;
  xiiSharedPtr<xiiGALBuffer> pPhysicalPageTable = pDevice->CreateBuffer(description);
  if (pPhysicalPageTable == nullptr)
    return XII_FAILURE;
  pPhysicalPageTable->SetDebugName("Virtual Shadow Physical Page Table");

  description.m_uiSize                         = static_cast<xiiUInt32>(uiVirtualBufferSize);
  xiiSharedPtr<xiiGALBuffer> pVirtualPageTable = pDevice->CreateBuffer(description);
  if (pVirtualPageTable == nullptr)
    return XII_FAILURE;
  pVirtualPageTable->SetDebugName("Virtual Shadow Hashed Page Table");

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type                  = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Format                = xiiGALResourceFormat::D32Float;
  textureDescription.m_Size.width            = static_cast<xiiUInt32>(uiAtlasWidth);
  textureDescription.m_Size.height           = static_cast<xiiUInt32>(uiAtlasHeight);
  textureDescription.m_uiMipLevels           = 1U;
  textureDescription.m_BindFlags             = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage                 = xiiGALResourceUsage::Default;
  xiiSharedPtr<xiiGALTexture> pPhysicalAtlas = pDevice->CreateTexture(textureDescription);
  if (pPhysicalAtlas == nullptr)
    return XII_FAILURE;
  pPhysicalAtlas->SetDebugName("Virtual Shadow Physical Atlas");

  s_pState->m_pPhysicalPageTable = std::move(pPhysicalPageTable);
  s_pState->m_pVirtualPageTable  = std::move(pVirtualPageTable);
  s_pState->m_pPhysicalAtlas     = std::move(pPhysicalAtlas);
  s_pState->m_PreparedUpload.Clear();
  s_pState->m_uiConfigurationRevision  = NextRasterRevision();
  s_pState->m_uiResidencyRevision      = NextRasterRevision();
  s_pState->m_uiCpuTableRevision       = 0;
  s_pState->m_uiRecordedUploadFrame    = xiiMath::MaxValue<xiiUInt64>();
  s_pState->m_uiRecordedUploadRevision = 0;
  for (auto& revision : s_pState->m_UploadedVirtualRevisions)
    revision = 0;
  for (xiiUInt32 i = 0; i < s_pState->m_Slots.GetCount(); ++i)
  {
    if (s_pState->m_Slots[i].m_bAllocated && !s_pState->m_Slots[i].m_Mapping.m_bNeedsRendering)
      InvalidateSlot(*s_pState, i);
    s_pState->m_Slots[i].m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
    s_pState->m_Slots[i].m_uiUploadRevision = s_pState->m_uiResidencyRevision;
  }

  return XII_SUCCESS;
}

xiiResult xiiVirtualShadowMapManager::Configure(const xiiVirtualShadowMapSettings& settings)
{
  if (s_pState == nullptr || !IsConfigurationValid(settings))
    return XII_FAILURE;

  // Keep the current residency/allocation state intact if replacement allocation fails.
  auto previous                       = std::move(s_pState);
  s_pState                            = XII_DEFAULT_NEW(xiiVirtualShadowMapManagerState);
  s_pState->m_bEngineStarted          = previous->m_bEngineStarted;
  s_pState->m_Settings                = settings;
  s_pState->m_uiConfigurationRevision = NextRasterRevision();
  s_pState->m_PreparedUpload.Clear();
  s_pState->m_uiRecordedUploadFrame    = xiiMath::MaxValue<xiiUInt64>();
  s_pState->m_uiRecordedUploadRevision = 0;
  s_pState->m_VirtualRecords.Clear();
  s_pState->m_uiCpuTableRevision  = 0;
  s_pState->m_uiResidencyRevision = NextRasterRevision();
  s_pState->m_UploadedVirtualRevisions.Clear();
  s_pState->m_UploadedVirtualRevisions.SetCount(settings.m_uiFramesInFlight, 0);

  s_pState->m_Slots.Clear();
  s_pState->m_Slots.SetCount(settings.m_uiPhysicalPageCount);
  s_pState->m_FreePages.Clear();
  s_pState->m_FreePages.Reserve(settings.m_uiPhysicalPageCount);
  for (xiiUInt32 uiPage = settings.m_uiPhysicalPageCount; uiPage > 0U; --uiPage)
    s_pState->m_FreePages.PushBack(uiPage - 1U);

  s_pState->m_PageLookup.Clear();
  s_pState->m_PageLookup.Reserve(settings.m_uiPhysicalPageCount);
  s_pState->m_DirtyPages.Clear();
  s_pState->m_Stats                              = {};
  s_pState->m_Stats.m_uiFreePageCount            = settings.m_uiPhysicalPageCount;
  s_pState->m_uiPhysicalPagesPerRow              = xiiMath::Max(static_cast<xiiUInt32>(xiiMath::Ceil(xiiMath::Sqrt(static_cast<float>(settings.m_uiPhysicalPageCount)))), 1U);
  s_pState->m_uiPhysicalPageRows                 = (settings.m_uiPhysicalPageCount + s_pState->m_uiPhysicalPagesPerRow - 1U) / s_pState->m_uiPhysicalPagesPerRow;
  s_pState->m_uiVirtualPageTableCapacity         = xiiMath::PowerOfTwo_Ceil(xiiMath::Max(settings.m_uiPhysicalPageCount * 2U, 2U));
  s_pState->m_Stats.m_uiPhysicalAtlasWidth       = s_pState->m_uiPhysicalPagesPerRow * settings.m_uiPageSize;
  s_pState->m_Stats.m_uiPhysicalAtlasHeight      = s_pState->m_uiPhysicalPageRows * settings.m_uiPageSize;
  s_pState->m_Stats.m_uiVirtualPageTableCapacity = s_pState->m_uiVirtualPageTableCapacity;
  s_pState->m_uiFrameIndex                       = 0U;
  s_pState->m_uiCompletedFrame                   = 0U;
  s_pState->m_uiAllFrameMask                     = settings.m_uiFramesInFlight >= 64U ? xiiMath::MaxValue<xiiUInt64>() : (xiiUInt64(1) << settings.m_uiFramesInFlight) - 1U;
  for (auto& slot : s_pState->m_Slots)
  {
    slot.m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
    slot.m_uiUploadRevision = s_pState->m_uiResidencyRevision;
  }
  s_pState->m_bInitialized = true;
  if (s_pState->m_bEngineStarted && previous->m_pPhysicalAtlas && CreateGpuResources().Failed())
  {
    s_pState = std::move(previous);
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

bool xiiVirtualShadowMapManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
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
  s_pState->m_PreparedUpload.Clear();
  s_pState->m_uiFrameIndex     = uiFrameIndex;
  s_pState->m_uiCompletedFrame = xiiMath::Min(uiCompletedFrame, uiFrameIndex);
  for (xiiUInt32 i = s_pState->m_RetiredPages.GetCount(); i-- > 0;)
    if (s_pState->m_RetiredPages[i].m_uiLastCapturedFrame <= s_pState->m_uiCompletedFrame)
    {
      s_pState->m_FreePages.PushBack(s_pState->m_RetiredPages[i].m_uiPhysicalPage);
      s_pState->m_RetiredPages.RemoveAtAndSwap(i);
    }
  s_pState->m_Stats.m_uiFreePageCount        = s_pState->m_FreePages.GetCount();
  s_pState->m_Stats.m_uiRetiredPageCount     = s_pState->m_RetiredPages.GetCount();
  s_pState->m_Stats.m_uiFeedbackRequestCount = 0U;
  s_pState->m_Stats.m_uiUniqueRequestCount   = 0U;
  s_pState->m_Stats.m_uiAllocationCount      = 0U;
  s_pState->m_Stats.m_uiEvictionCount        = 0U;
  s_pState->m_Stats.m_uiDroppedRequestCount  = 0U;
  s_pState->m_Stats.m_uiInvalidationCount    = 0U;
  s_pState->m_Stats.m_uiRasterizedPageCount  = 0U;
  s_pState->m_uiRasterizationsThisFrame      = 0U;
  s_pState->m_uiRetirementsThisFrame         = 0U;
  for (xiiUInt32 i = s_pState->m_PendingUploads.GetCount(); i-- > 0;)
  {
    const auto& pending   = s_pState->m_PendingUploads[i];
    const bool  completed = pending.m_Completion->GetCompletedValue() >= 1;
    if (!completed && pending.m_Upload->m_uiFrame > s_pState->m_uiCompletedFrame)
      continue;
    if (completed)
      AcknowledgeTableUpload(*pending.m_Upload);
    s_pState->m_PendingUploads.RemoveAtAndSwap(i);
  }
  for (xiiUInt32 i = s_pState->m_PendingRasters.GetCount(); i-- > 0;)
  {
    const auto& pending   = s_pState->m_PendingRasters[i];
    const bool  completed = pending.m_Fence->GetCompletedValue() >= 1;
    if (!completed && pending.m_Work.m_uiFrame > s_pState->m_uiCompletedFrame)
      continue;
    if (completed)
      XII_IGNORE_UNUSED(MarkPageRendered(pending.m_Work));
    auto& slot = s_pState->m_Slots[pending.m_Work.m_uiPhysicalPage];
    if (slot.m_uiPendingRevision == pending.m_Work.m_uiContentRevision)
      slot.m_uiPendingRevision = 0;
    s_pState->m_PendingRasters.RemoveAtAndSwap(i);
  }
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
  xiiUInt32 uiAllocationsRemaining = s_pState->m_Settings.m_uiMaxPageAllocations - xiiMath::Min(s_pState->m_Settings.m_uiMaxPageAllocations, s_pState->m_Stats.m_uiAllocationCount);
  uiAllocationsRemaining -= xiiMath::Min(uiAllocationsRemaining, s_pState->m_uiRetirementsThisFrame);
  const xiiUInt32 uiBasePagesPerAxis = s_pState->m_Settings.m_uiVirtualResolution / s_pState->m_Settings.m_uiPageSize;

  for (const xiiVirtualShadowPageRequest& request : sortedRequests)
  {
    const xiiUInt64 uiKey          = request.m_Page.GetPackedValue();
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
      auto&      mapping        = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
      const bool bPinnedChanged = request.m_bPinned && !mapping.m_bPinned;
      mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
      mapping.m_uiPriority      = xiiMath::Max(mapping.m_uiPriority, request.m_uiPriority);
      mapping.m_bPinned |= request.m_bPinned;
      if (bPinnedChanged)
        DirtyTableSlot(*s_pState, uiPhysicalPage);
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
          uiOldestFrame  = slot.m_Mapping.m_uiLastUsedFrame;
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
      ++s_pState->m_Stats.m_uiEvictionCount;
      auto& slot = s_pState->m_Slots[uiPhysicalPage];
      // A newer immutable table may still reference this atlas page even when
      // its demand is old. Unmap it now, but retain the physical page until all
      // graphs in its last captured GPU frame have retired.
      if (slot.m_uiLastCapturedFrame > s_pState->m_uiCompletedFrame)
      {
        slot.m_bAllocated        = false;
        slot.m_uiContentRevision = NextRasterRevision();
        slot.m_uiPendingRevision = 0;
        DirtyTableSlot(*s_pState, uiPhysicalPage);
        s_pState->m_RetiredPages.PushBack({uiPhysicalPage, slot.m_uiLastCapturedFrame});
        ++s_pState->m_uiRetirementsThisFrame;
        --uiAllocationsRemaining;
        ++s_pState->m_Stats.m_uiDroppedRequestCount;
        continue;
      }
    }

    auto& slot                       = s_pState->m_Slots[uiPhysicalPage];
    slot.m_bAllocated                = true;
    slot.m_Mapping.m_Page            = request.m_Page;
    slot.m_Mapping.m_uiPhysicalPage  = uiPhysicalPage;
    slot.m_Mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
    slot.m_Mapping.m_uiPriority      = request.m_uiPriority;
    slot.m_Mapping.m_bPinned         = request.m_bPinned;
    slot.m_Mapping.m_bNeedsRendering = true;
    DirtyTableSlot(*s_pState, uiPhysicalPage);
    slot.m_uiContentRevision    = NextRasterRevision();
    slot.m_uiLastCapturedFrame  = 0;
    slot.m_uiVirtualTableBucket = xiiInvalidIndex;
    slot.m_uiPendingRevision    = 0;
    slot.m_uiPlannedFrame       = xiiMath::MaxValue<xiiUInt64>();
    s_pState->m_PageLookup.Insert(uiKey, uiPhysicalPage);
    s_pState->m_DirtyPages.PushBack(slot.m_Mapping);
    --uiAllocationsRemaining;
    ++s_pState->m_Stats.m_uiAllocationCount;
  }

  s_pState->m_Stats.m_uiResidentPageCount = s_pState->m_PageLookup.GetCount();
  s_pState->m_Stats.m_uiFreePageCount     = s_pState->m_FreePages.GetCount();
  s_pState->m_Stats.m_uiDirtyPageCount    = s_pState->m_DirtyPages.GetCount();
  s_pState->m_Stats.m_uiRetiredPageCount  = s_pState->m_RetiredPages.GetCount();
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

xiiArrayPtr<const xiiVirtualShadowPageMapping> xiiVirtualShadowMapManager::GetDirtyPages()
{
  if (!IsInitialized())
    return {};

  return xiiArrayPtr<const xiiVirtualShadowPageMapping>(s_pState->m_DirtyPages.GetData(), s_pState->m_DirtyPages.GetCount());
}

bool xiiVirtualShadowMapManager::InvalidatePage(const xiiVirtualShadowPageId& page)
{
  if (!IsInitialized())
    return false;

  xiiUInt32 uiPhysicalPage = xiiInvalidIndex;
  if (!s_pState->m_PageLookup.TryGetValue(page.GetPackedValue(), uiPhysicalPage))
    return false;

  InvalidateSlot(*s_pState, uiPhysicalPage);
  return true;
}

xiiUInt32 xiiVirtualShadowMapManager::InvalidateRegion(xiiUInt32 uiLightId, xiiUInt32 uiMipLevel, const xiiRectU32& pageRegion)
{
  if (!IsInitialized() || pageRegion.width == 0U || pageRegion.height == 0U)
    return 0U;

  const xiiUInt32 uiStableLightId = uiLightId & s_uiMaximumLightId;
  const xiiUInt64 uiMaxPageX      = static_cast<xiiUInt64>(pageRegion.x) + pageRegion.width;
  const xiiUInt64 uiMaxPageY      = static_cast<xiiUInt64>(pageRegion.y) + pageRegion.height;
  xiiUInt32       uiInvalidated   = 0U;
  for (xiiUInt32 uiPhysicalPage = 0U; uiPhysicalPage < s_pState->m_Slots.GetCount(); ++uiPhysicalPage)
  {
    const auto& slot = s_pState->m_Slots[uiPhysicalPage];
    const auto& page = slot.m_Mapping.m_Page;
    if (slot.m_bAllocated && page.m_uiLightId == uiStableLightId && page.m_uiMipLevel == uiMipLevel && page.m_uiPageX >= pageRegion.x && static_cast<xiiUInt64>(page.m_uiPageX) < uiMaxPageX && page.m_uiPageY >= pageRegion.y && static_cast<xiiUInt64>(page.m_uiPageY) < uiMaxPageY && InvalidateSlot(*s_pState, uiPhysicalPage))
      ++uiInvalidated;
  }
  return uiInvalidated;
}

xiiUInt32 xiiVirtualShadowMapManager::InvalidateLight(xiiUInt32 uiLightId)
{
  if (!IsInitialized())
    return 0U;

  const xiiUInt32 uiStableLightId = uiLightId & s_uiMaximumLightId;
  xiiUInt32       uiInvalidated   = 0U;
  for (xiiUInt32 uiPhysicalPage = 0U; uiPhysicalPage < s_pState->m_Slots.GetCount(); ++uiPhysicalPage)
  {
    const auto& slot = s_pState->m_Slots[uiPhysicalPage];
    if (slot.m_bAllocated && slot.m_Mapping.m_Page.m_uiLightId == uiStableLightId && InvalidateSlot(*s_pState, uiPhysicalPage))
      ++uiInvalidated;
  }
  return uiInvalidated;
}

xiiUInt32 xiiVirtualShadowMapManager::InvalidateAll()
{
  if (!IsInitialized())
    return 0U;

  xiiUInt32 uiInvalidated = 0U;
  for (xiiUInt32 uiPhysicalPage = 0U; uiPhysicalPage < s_pState->m_Slots.GetCount(); ++uiPhysicalPage)
  {
    if (InvalidateSlot(*s_pState, uiPhysicalPage))
      ++uiInvalidated;
  }
  return uiInvalidated;
}

bool xiiVirtualShadowMapManager::MarkPageRendered(const xiiVirtualShadowRasterTicket& page)
{
  if (!IsInitialized() || page.m_uiPhysicalPage >= s_pState->m_Slots.GetCount())
    return false;
  const auto& slot = s_pState->m_Slots[page.m_uiPhysicalPage];
  if (!slot.m_bAllocated || !slot.m_Mapping.m_bNeedsRendering || slot.m_Mapping.m_Page.GetPackedValue() != page.m_Page.GetPackedValue() || page.m_uiContentRevision == 0 || slot.m_uiContentRevision != page.m_uiContentRevision)
    return false;
  auto& mapping             = s_pState->m_Slots[page.m_uiPhysicalPage].m_Mapping;
  mapping.m_bNeedsRendering = false;
  DirtyTableSlot(*s_pState, page.m_uiPhysicalPage);
  for (xiiUInt32 i = 0U; i < s_pState->m_DirtyPages.GetCount(); ++i)
  {
    if (s_pState->m_DirtyPages[i].m_uiPhysicalPage == page.m_uiPhysicalPage)
    {
      s_pState->m_DirtyPages.RemoveAtAndSwap(i);
      break;
    }
  }
  s_pState->m_Stats.m_uiDirtyPageCount = s_pState->m_DirtyPages.GetCount();
  ++s_pState->m_Stats.m_uiRasterizedPageCount;
  return true;
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

xiiUInt64 xiiVirtualShadowMapManager::GetConfigurationRevision()
{
  return IsInitialized() ? s_pState->m_uiConfigurationRevision : 0;
}

xiiSharedPtr<xiiGALTexture> xiiVirtualShadowMapManager::GetPhysicalAtlas()
{
  if (!IsInitialized())
    return nullptr;
  if (s_pState->m_pPhysicalAtlas == nullptr && CreateGpuResources().Failed())
    return nullptr;
  return s_pState->m_pPhysicalAtlas;
}

bool xiiVirtualShadowMapManager::GetPhysicalPageViewport(xiiUInt32 uiPhysicalPage, xiiRectU32& out_viewport)
{
  if (!IsInitialized() || uiPhysicalPage >= s_pState->m_Settings.m_uiPhysicalPageCount || s_pState->m_uiPhysicalPagesPerRow == 0U)
    return false;

  const xiiUInt32 uiPageX = uiPhysicalPage % s_pState->m_uiPhysicalPagesPerRow;
  const xiiUInt32 uiPageY = uiPhysicalPage / s_pState->m_uiPhysicalPagesPerRow;
  out_viewport            = xiiRectU32(uiPageX * s_pState->m_Settings.m_uiPageSize, uiPageY * s_pState->m_Settings.m_uiPageSize, s_pState->m_Settings.m_uiPageSize, s_pState->m_Settings.m_uiPageSize);
  return true;
}

bool xiiVirtualShadowMapManager::BuildPageViewProjection(const xiiMat4& cascadeViewProjection, const xiiVirtualShadowPageId& page, xiiUInt32 uiVirtualResolution, xiiUInt32 uiPageSize, xiiMat4& out_pageViewProjection)
{
  if (!page.IsValid() || uiVirtualResolution == 0U || uiPageSize == 0U || uiVirtualResolution % uiPageSize != 0U)
    return false;

  const xiiUInt32 uiBasePagesPerAxis = uiVirtualResolution / uiPageSize;
  if (uiBasePagesPerAxis == 0U || page.m_uiMipLevel >= 32U)
    return false;

  const xiiUInt32 uiPagesPerAxis = xiiMath::Max(uiBasePagesPerAxis >> page.m_uiMipLevel, 1U);
  if (page.m_uiPageX >= uiPagesPerAxis || page.m_uiPageY >= uiPagesPerAxis)
    return false;

  // Convert the selected virtual-UV tile back into the full [-1, 1] clip
  // range. Y is inverted because shadow UV uses the top-left texture origin.
  const float fPagesPerAxis = static_cast<float>(uiPagesPerAxis);
  xiiMat4     crop          = xiiMat4::MakeIdentity();
  crop.Element(0, 0)        = fPagesPerAxis;
  crop.Element(1, 1)        = fPagesPerAxis;
  crop.Element(3, 0)        = fPagesPerAxis - 2.0f * static_cast<float>(page.m_uiPageX) - 1.0f;
  crop.Element(3, 1)        = 1.0f - fPagesPerAxis + 2.0f * static_cast<float>(page.m_uiPageY);
  out_pageViewProjection    = crop * cascadeViewProjection;
  return out_pageViewProjection.IsValid();
}

void xiiVirtualShadowMapManager::GatherRasterPages(xiiArrayPtr<const xiiMat4> cascadeViewProjections, xiiUInt32 uiLightId, xiiDynamicArray<xiiVirtualShadowRasterPage>& out_pages, xiiUInt32 maxPages)
{
  out_pages.Clear();
  if (!IsInitialized() || cascadeViewProjections.IsEmpty())
    return;
  const xiiVirtualShadowMapSettings& settings = GetConfiguration();
  xiiDynamicArray<xiiUInt32>         rasterCandidates;
  rasterCandidates.Reserve(s_pState->m_DirtyPages.GetCount());
  for (const xiiVirtualShadowPageMapping& dirtyPage : GetDirtyPages())
  {
    if (dirtyPage.m_uiPhysicalPage >= s_pState->m_Slots.GetCount())
      continue;

    const auto& slot = s_pState->m_Slots[dirtyPage.m_uiPhysicalPage];
    if ((s_pState->m_PreparedUpload && slot.m_uiVirtualTableBucket == xiiInvalidIndex) || !slot.m_bAllocated || !slot.m_Mapping.m_bNeedsRendering || slot.m_Mapping.m_Page.m_uiLightId != (uiLightId & s_uiMaximumLightId) || slot.m_Mapping.m_Page.m_uiMipLevel >= cascadeViewProjections.GetCount() || slot.m_uiPlannedFrame == s_pState->m_uiFrameIndex || slot.m_uiPendingRevision == slot.m_uiContentRevision)
      continue;

    rasterCandidates.PushBack(dirtyPage.m_uiPhysicalPage);
  }

  // The dirty list records insertion order, which is not a useful visibility
  // policy once raster work is bounded. Rank live slot metadata every frame so
  // a newly invalidated near-field page cannot sit behind stale low-priority
  // allocations. Stable page keys make otherwise equal schedules reproducible.
  rasterCandidates.Sort([](xiiUInt32 uiLhs, xiiUInt32 uiRhs) {
    const xiiVirtualShadowPageMapping& lhs = s_pState->m_Slots[uiLhs].m_Mapping;
    const xiiVirtualShadowPageMapping& rhs = s_pState->m_Slots[uiRhs].m_Mapping;
    if (lhs.m_bPinned != rhs.m_bPinned)
      return lhs.m_bPinned;
    if (lhs.m_uiPriority != rhs.m_uiPriority)
      return lhs.m_uiPriority > rhs.m_uiPriority;
    if (lhs.m_uiLastUsedFrame != rhs.m_uiLastUsedFrame)
      return lhs.m_uiLastUsedFrame > rhs.m_uiLastUsedFrame;
    if (lhs.m_Page.m_uiMipLevel != rhs.m_Page.m_uiMipLevel)
      return lhs.m_Page.m_uiMipLevel < rhs.m_Page.m_uiMipLevel;
    return lhs.m_Page.GetPackedValue() < rhs.m_Page.GetPackedValue();
  });

  for (xiiUInt32 uiPhysicalPage : rasterCandidates)
  {
    if (out_pages.GetCount() >= maxPages || s_pState->m_uiRasterizationsThisFrame >= settings.m_uiMaxPageRasterizations)
      break;

    auto& slot    = s_pState->m_Slots[uiPhysicalPage];
    auto& mapping = slot.m_Mapping;

    xiiMat4 pageViewProjection;
    if (!BuildPageViewProjection(cascadeViewProjections[mapping.m_Page.m_uiMipLevel], mapping.m_Page, settings.m_uiVirtualResolution, settings.m_uiPageSize, pageViewProjection))
      continue;

    xiiRectU32 viewport;
    if (!GetPhysicalPageViewport(mapping.m_uiPhysicalPage, viewport))
      continue;
    auto& work                = out_pages.ExpandAndGetRef();
    work.m_Page               = mapping.m_Page;
    work.m_uiPhysicalPage     = uiPhysicalPage;
    work.m_uiContentRevision  = slot.m_uiContentRevision;
    work.m_uiFrame            = s_pState->m_uiFrameIndex;
    work.m_ViewProjection     = pageViewProjection;
    work.m_Viewport           = viewport;
    slot.m_uiPlannedFrame     = s_pState->m_uiFrameIndex;
    mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
    ++s_pState->m_uiRasterizationsThisFrame;
  }
}

bool xiiVirtualShadowMapManager::IsRasterWorkCurrent(const xiiVirtualShadowRasterTicket& page)
{
  if (!IsInitialized() || page.m_uiPhysicalPage >= s_pState->m_Slots.GetCount() || page.m_uiFrame != s_pState->m_uiFrameIndex)
    return false;
  const auto& slot = s_pState->m_Slots[page.m_uiPhysicalPage];
  return slot.m_bAllocated && slot.m_Mapping.m_bNeedsRendering && page.m_uiContentRevision != 0 &&
    slot.m_uiContentRevision == page.m_uiContentRevision && slot.m_Mapping.m_Page.GetPackedValue() == page.m_Page.GetPackedValue();
}

xiiSharedPtr<const xiiVirtualShadowPagePublication> xiiVirtualShadowMapManager::CapturePagePublication(xiiSharedPtr<const xiiVirtualShadowTableUpload> upload, xiiArrayPtr<const xiiVirtualShadowRasterPage> pages)
{
  if (!upload || !IsInitialized() || upload != s_pState->m_PreparedUpload || upload->m_uiFrame != s_pState->m_uiFrameIndex || upload->m_uiConfigurationRevision != s_pState->m_uiConfigurationRevision)
    return nullptr;
  xiiSharedPtr<xiiVirtualShadowPagePublication> publication = XII_DEFAULT_NEW(xiiVirtualShadowPagePublication);
  publication->m_Upload                                     = std::move(upload);
  const auto&           source                              = *publication->m_Upload;
  xiiHashSet<xiiUInt32> seen;
  for (const auto& page : pages)
  {
    if (!IsRasterWorkCurrent(page) || seen.Contains(page.m_uiPhysicalPage))
      return nullptr;
    seen.Insert(page.m_uiPhysicalPage);
    const auto& slot = s_pState->m_Slots[page.m_uiPhysicalPage];
    if (slot.m_uiVirtualTableBucket == xiiInvalidIndex || slot.m_uiVirtualTableBucket >= source.m_Layout.m_uiVirtualTableCapacity)
      return nullptr;
    const xiiUInt64 physicalOffset = (xiiUInt64(source.m_Layout.m_uiFrameBaseIndex) + page.m_uiPhysicalPage) * sizeof(xiiGpuVirtualShadowPage);
    const xiiUInt64 virtualOffset  = (xiiUInt64(source.m_Layout.m_uiVirtualTableBaseIndex) + slot.m_uiVirtualTableBucket) * sizeof(xiiGpuVirtualShadowPage);
    if (physicalOffset + sizeof(xiiGpuVirtualShadowPage) > source.m_PhysicalBuffer->GetDescription().m_uiSize || virtualOffset + sizeof(xiiGpuVirtualShadowPage) > source.m_VirtualBuffer->GetDescription().m_uiSize)
      return nullptr;
    auto& record                            = publication->m_Records.ExpandAndGetRef();
    record.m_Ticket                         = page;
    record.m_uiPhysicalByteOffset           = static_cast<xiiUInt32>(physicalOffset);
    record.m_uiVirtualByteOffset            = static_cast<xiiUInt32>(virtualOffset);
    const auto key                          = page.m_Page.GetPackedValue();
    record.m_CleanRecord.m_uiVirtualKeyLow  = static_cast<xiiUInt32>(key);
    record.m_CleanRecord.m_uiVirtualKeyHigh = static_cast<xiiUInt32>(key >> 32);
    record.m_CleanRecord.m_uiPhysicalPage   = page.m_uiPhysicalPage;
    record.m_CleanRecord.m_uiFlags          = 1U | (slot.m_Mapping.m_bPinned ? 4U : 0U);
  }
  return publication;
}

void xiiVirtualShadowMapManager::RecordPagePublication(const xiiVirtualShadowPagePublication& publication, xiiSharedPtr<xiiGALFence> completion)
{
  if (!completion || !publication.m_Upload || !IsInitialized() || publication.m_Upload->m_uiConfigurationRevision != s_pState->m_uiConfigurationRevision || publication.m_Upload->m_uiFrame != s_pState->m_uiFrameIndex)
    return;
  for (const auto& record : publication.m_Records)
  {
    if (!IsRasterWorkCurrent(record.m_Ticket))
      continue;
    auto& slot = s_pState->m_Slots[record.m_Ticket.m_uiPhysicalPage];
    if (slot.m_uiPendingRevision == record.m_Ticket.m_uiContentRevision)
      continue;
    slot.m_uiPendingRevision = record.m_Ticket.m_uiContentRevision;
    auto& pending            = s_pState->m_PendingRasters.ExpandAndGetRef();
    pending.m_Work           = record.m_Ticket;
    pending.m_Fence          = completion;
  }
}

xiiSharedPtr<const xiiVirtualShadowTableUpload> xiiVirtualShadowMapManager::CaptureTableUpload(xiiUInt64 frame)
{
  if (!IsInitialized() || frame != s_pState->m_uiFrameIndex)
    return nullptr;
  if (s_pState->m_PreparedUpload)
    return s_pState->m_PreparedUpload;
  if ((!s_pState->m_pPhysicalPageTable || !s_pState->m_pVirtualPageTable || !s_pState->m_pPhysicalAtlas) && CreateGpuResources().Failed())
    return nullptr;
  auto& state = *s_pState;
  for (auto& slot : state.m_Slots)
    if (slot.m_bAllocated)
      slot.m_uiLastCapturedFrame = frame;
  if (state.m_uiCpuTableRevision != state.m_uiResidencyRevision)
  {
    state.m_VirtualRecords.SetCount(state.m_uiVirtualPageTableCapacity);
    for (auto& record : state.m_VirtualRecords)
      record = {};
    for (auto& slot : state.m_Slots)
      slot.m_uiVirtualTableBucket = xiiInvalidIndex;
    const auto mask = state.m_uiVirtualPageTableCapacity - 1;
    for (xiiUInt32 i = 0; i < state.m_Slots.GetCount(); ++i)
    {
      auto& slot = state.m_Slots[i];
      if (!slot.m_bAllocated)
        continue;
      const auto key    = slot.m_Mapping.m_Page.GetPackedValue();
      auto       bucket = HashVirtualPageKey(static_cast<xiiUInt32>(key), static_cast<xiiUInt32>(key >> 32)) & mask;
      while (state.m_VirtualRecords[bucket].m_uiFlags & 1U)
        bucket = (bucket + 1U) & mask;
      auto& record                = state.m_VirtualRecords[bucket];
      record.m_uiVirtualKeyLow    = static_cast<xiiUInt32>(key);
      record.m_uiVirtualKeyHigh   = static_cast<xiiUInt32>(key >> 32);
      record.m_uiPhysicalPage     = i;
      record.m_uiFlags            = 1U | (slot.m_Mapping.m_bNeedsRendering ? 2U : 0U) | (slot.m_Mapping.m_bPinned ? 4U : 0U);
      slot.m_uiVirtualTableBucket = bucket;
    }
    state.m_uiCpuTableRevision = state.m_uiResidencyRevision;
  }
  xiiSharedPtr<xiiVirtualShadowTableUpload> upload = XII_DEFAULT_NEW(xiiVirtualShadowTableUpload);
  upload->m_PhysicalBuffer                         = state.m_pPhysicalPageTable;
  upload->m_VirtualBuffer                          = state.m_pVirtualPageTable;
  upload->m_Atlas                                  = state.m_pPhysicalAtlas;
  upload->m_uiFrame                                = frame;
  upload->m_uiVirtualResolution                    = state.m_Settings.m_uiVirtualResolution;
  upload->m_uiPageSize                             = state.m_Settings.m_uiPageSize;
  upload->m_uiConfigurationRevision                = state.m_uiConfigurationRevision;
  upload->m_uiResidencyRevision                    = state.m_uiResidencyRevision;
  const auto slice                                 = static_cast<xiiUInt32>(frame % state.m_Settings.m_uiFramesInFlight);
  const auto bit                                   = xiiUInt64(1) << slice;
  auto&      layout                                = upload->m_Layout;
  layout.m_uiFrameBaseIndex                        = slice * state.m_Settings.m_uiPhysicalPageCount;
  layout.m_uiPhysicalPageCount                     = state.m_Settings.m_uiPhysicalPageCount;
  layout.m_uiVirtualTableBaseIndex                 = slice * state.m_uiVirtualPageTableCapacity;
  layout.m_uiVirtualTableCapacity                  = state.m_uiVirtualPageTableCapacity;
  if (state.m_UploadedVirtualRevisions[slice] != state.m_uiResidencyRevision)
    upload->m_VirtualRecords = state.m_VirtualRecords;
  for (xiiUInt32 i = 0; i < state.m_Slots.GetCount(); ++i)
  {
    const auto& slot = state.m_Slots[i];
    if (!(slot.m_uiDirtyFrameMask & bit))
      continue;
    if (upload->m_PhysicalRanges.IsEmpty() || upload->m_PhysicalRanges.PeekBack().m_uiFirstPhysicalPage + upload->m_PhysicalRanges.PeekBack().m_uiRecordCount != i)
      upload->m_PhysicalRanges.PushBack({i, upload->m_PhysicalRecords.GetCount(), 0});
    ++upload->m_PhysicalRanges.PeekBack().m_uiRecordCount;
    upload->m_PhysicalRevisions.PushBack(slot.m_uiUploadRevision);
    auto&      record         = upload->m_PhysicalRecords.ExpandAndGetRef();
    const auto key            = slot.m_bAllocated ? slot.m_Mapping.m_Page.GetPackedValue() : 0;
    record.m_uiVirtualKeyLow  = static_cast<xiiUInt32>(key);
    record.m_uiVirtualKeyHigh = static_cast<xiiUInt32>(key >> 32);
    record.m_uiPhysicalPage   = i;
    record.m_uiFlags          = slot.m_bAllocated ? (1U | (slot.m_Mapping.m_bNeedsRendering ? 2U : 0U) | (slot.m_Mapping.m_bPinned ? 4U : 0U)) : 0;
  }
  state.m_PreparedUpload = upload;
  return state.m_PreparedUpload;
}

void xiiVirtualShadowMapManager::AcknowledgeTableUpload(const xiiVirtualShadowTableUpload& upload)
{
  if (!IsInitialized() || upload.m_uiConfigurationRevision != s_pState->m_uiConfigurationRevision)
    return;
  const auto slice = static_cast<xiiUInt32>(upload.m_uiFrame % s_pState->m_Settings.m_uiFramesInFlight);
  const auto bit   = xiiUInt64(1) << slice;
  for (const auto& range : upload.m_PhysicalRanges)
    for (xiiUInt32 i = 0; i < range.m_uiRecordCount; ++i)
    {
      auto& slot = s_pState->m_Slots[range.m_uiFirstPhysicalPage + i];
      if (slot.m_uiUploadRevision == upload.m_PhysicalRevisions[range.m_uiFirstRecord + i])
        slot.m_uiDirtyFrameMask &= ~bit;
    }
  if (!upload.m_VirtualRecords.IsEmpty())
    s_pState->m_UploadedVirtualRevisions[slice] = xiiMath::Max(s_pState->m_UploadedVirtualRevisions[slice], upload.m_uiResidencyRevision);
}

void xiiVirtualShadowMapManager::RecordTableUpload(xiiSharedPtr<const xiiVirtualShadowTableUpload> upload, xiiSharedPtr<xiiGALFence> completion)
{
  if (!upload || !completion || !IsInitialized() || upload->m_uiConfigurationRevision != s_pState->m_uiConfigurationRevision || upload->m_uiFrame != s_pState->m_uiFrameIndex || IsTableUploadRecorded(*upload))
    return;
  s_pState->m_uiRecordedUploadFrame    = upload->m_uiFrame;
  s_pState->m_uiRecordedUploadRevision = upload->m_uiResidencyRevision;
  s_pState->m_PendingUploads.PushBack({std::move(upload), std::move(completion)});
}

bool xiiVirtualShadowMapManager::IsTableUploadRecorded(const xiiVirtualShadowTableUpload& upload)
{
  return IsInitialized() && upload.m_uiConfigurationRevision == s_pState->m_uiConfigurationRevision &&
    upload.m_uiFrame == s_pState->m_uiRecordedUploadFrame && upload.m_uiResidencyRevision == s_pState->m_uiRecordedUploadRevision;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_VirtualShadowMap);
