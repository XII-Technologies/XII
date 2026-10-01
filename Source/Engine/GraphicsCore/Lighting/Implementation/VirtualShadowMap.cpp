/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Lighting/GpuShadowRaster.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

namespace
{
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
      settings.m_uiMaxPageRasterizations > 0U && settings.m_uiFramesInFlight > 0U;
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
    xiiUInt32                   m_uiVirtualTableBucket = xiiInvalidIndex;
  };

  xiiVirtualShadowMapSettings                  m_Settings;
  xiiDynamicArray<Slot>                        m_Slots;
  xiiDynamicArray<xiiUInt32>                   m_FreePages;
  xiiHashTable<xiiUInt64, xiiUInt32>           m_PageLookup;
  xiiDynamicArray<xiiVirtualShadowPageUpdate>  m_PageTableUpdates;
  xiiDynamicArray<xiiVirtualShadowPageMapping> m_DirtyPages;
  xiiVirtualShadowMapStats                     m_Stats;
  xiiSharedPtr<xiiGALBuffer>                   m_pPhysicalPageTable;
  xiiSharedPtr<xiiGALBuffer>                   m_pVirtualPageTable;
  xiiSharedPtr<xiiGALTexture>                  m_pPhysicalAtlas;
  xiiSharedPtr<xiiGALBuffer>                   m_pFeedbackBuffer;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  m_FeedbackReadbackRing;
  xiiDynamicArray<xiiUInt64>                   m_FeedbackReadbackFrames;
  xiiSharedPtr<xiiGALComputePipelineState>     m_pFeedbackPipeline;
  xiiUInt64                                    m_uiFrameIndex               = 0U;
  xiiUInt64                                    m_uiCompletedFrame           = 0U;
  xiiUInt64                                    m_uiAllFrameMask             = 0U;
  xiiUInt64                                    m_uiFeedbackClearFrame       = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32                                    m_uiRasterizationsThisFrame  = 0U;
  xiiUInt32                                    m_uiPhysicalPagesPerRow      = 0U;
  xiiUInt32                                    m_uiPhysicalPageRows         = 0U;
  xiiUInt32                                    m_uiVirtualPageTableCapacity = 0U;
  bool                                         m_bEngineStarted             = false;
  bool                                         m_bInitialized               = false;
};

xiiUniquePtr<xiiVirtualShadowMapManagerState> xiiVirtualShadowMapManager::s_pState;

namespace
{
  bool InvalidateSlot(xiiVirtualShadowMapManagerState& state, xiiUInt32 uiPhysicalPage)
  {
    if (uiPhysicalPage >= state.m_Slots.GetCount() || !state.m_Slots[uiPhysicalPage].m_bAllocated)
      return false;

    auto& slot              = state.m_Slots[uiPhysicalPage];
    slot.m_uiDirtyFrameMask = state.m_uiAllFrameMask;
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVirtualShadowFeedback, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVirtualShadowFeedback>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VirtualKeyLow", m_uiVirtualKeyLow),
    XII_MEMBER_PROPERTY("VirtualKeyHigh", m_uiVirtualKeyHigh),
    XII_MEMBER_PROPERTY("Priority", m_uiPriority),
    XII_MEMBER_PROPERTY("Flags", m_uiFlags),
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
  CreateGpuResources().IgnoreResult();
}

void xiiVirtualShadowMapManager::EngineShutdown()
{
  if (s_pState != nullptr)
  {
    s_pState->m_pFeedbackPipeline.Clear();
    s_pState->m_FeedbackReadbackRing.Clear();
    s_pState->m_FeedbackReadbackFrames.Clear();
    s_pState->m_pFeedbackBuffer.Clear();
    s_pState->m_pPhysicalAtlas.Clear();
    s_pState->m_pVirtualPageTable.Clear();
    s_pState->m_pPhysicalPageTable.Clear();
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

  const xiiUInt64 uiFeedbackSize = static_cast<xiiUInt64>(s_pState->m_Settings.m_uiMaxFeedbackRequests + 1U) * sizeof(xiiGpuVirtualShadowFeedback);
  if (uiFeedbackSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  description                                = {};
  description.m_uiSize                       = static_cast<xiiUInt32>(uiFeedbackSize);
  description.m_uiElementByteStride          = sizeof(xiiGpuVirtualShadowFeedback);
  description.m_BindFlags                    = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                         = xiiGALBufferMode::Structured;
  description.m_Usage                        = xiiGALResourceUsage::Default;
  xiiSharedPtr<xiiGALBuffer> pFeedbackBuffer = pDevice->CreateBuffer(description);
  if (pFeedbackBuffer == nullptr)
    return XII_FAILURE;
  pFeedbackBuffer->SetDebugName("Virtual Shadow Feedback");

  description                  = {};
  description.m_uiSize         = static_cast<xiiUInt32>(uiFeedbackSize);
  description.m_Usage          = xiiGALResourceUsage::Staging;
  description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> feedbackReadbackRing;
  feedbackReadbackRing.SetCount(s_pState->m_Settings.m_uiFramesInFlight);
  for (xiiUInt32 i = 0U; i < s_pState->m_Settings.m_uiFramesInFlight; ++i)
  {
    feedbackReadbackRing[i] = pDevice->CreateBuffer(description);
    if (feedbackReadbackRing[i] == nullptr)
      return XII_FAILURE;
  }

  s_pState->m_pPhysicalPageTable   = std::move(pPhysicalPageTable);
  s_pState->m_pVirtualPageTable    = std::move(pVirtualPageTable);
  s_pState->m_pPhysicalAtlas       = std::move(pPhysicalAtlas);
  s_pState->m_pFeedbackBuffer      = std::move(pFeedbackBuffer);
  s_pState->m_FeedbackReadbackRing = std::move(feedbackReadbackRing);
  s_pState->m_FeedbackReadbackFrames.SetCount(s_pState->m_Settings.m_uiFramesInFlight, xiiMath::MaxValue<xiiUInt64>());
  return XII_SUCCESS;
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
  s_pState->m_bInitialized                       = true;
  if (s_pState->m_bEngineStarted)
    return CreateGpuResources();
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
  s_pState->m_uiFrameIndex     = uiFrameIndex;
  s_pState->m_uiCompletedFrame = xiiMath::Min(uiCompletedFrame, uiFrameIndex);
  s_pState->m_PageTableUpdates.Clear();
  s_pState->m_Stats.m_uiFeedbackRequestCount = 0U;
  s_pState->m_Stats.m_uiUniqueRequestCount   = 0U;
  s_pState->m_Stats.m_uiAllocationCount      = 0U;
  s_pState->m_Stats.m_uiEvictionCount        = 0U;
  s_pState->m_Stats.m_uiDroppedRequestCount  = 0U;
  s_pState->m_Stats.m_uiInvalidationCount    = 0U;
  s_pState->m_Stats.m_uiRasterizedPageCount  = 0U;
  s_pState->m_uiRasterizationsThisFrame      = 0U;

  if (s_pState->m_pFeedbackBuffer == nullptr)
    return;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return;

  for (xiiUInt32 uiSlot = 0U; uiSlot < s_pState->m_FeedbackReadbackFrames.GetCount(); ++uiSlot)
  {
    const xiiUInt64 uiFeedbackFrame = s_pState->m_FeedbackReadbackFrames[uiSlot];
    if (uiFeedbackFrame == xiiMath::MaxValue<xiiUInt64>() || uiFeedbackFrame > s_pState->m_uiCompletedFrame || uiFrameIndex < uiFeedbackFrame + s_pState->m_Settings.m_uiFramesInFlight)
      continue;

    xiiGALCommandListCreationDescription commandListDescription;
    commandListDescription.m_QueueFlags          = xiiGALCommandQueueFlags::Transfer;
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
    if (pCommandList == nullptr)
      continue;

    pCommandList->Begin();
    void* pMappedData = nullptr;
    if (pCommandList->MapBuffer(s_pState->m_FeedbackReadbackRing[uiSlot], xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pMappedData).Succeeded())
    {
      const xiiGpuVirtualShadowFeedback*           pFeedback = static_cast<const xiiGpuVirtualShadowFeedback*>(pMappedData);
      const xiiUInt32                              uiCount   = xiiMath::Min(pFeedback[0].m_uiVirtualKeyLow, s_pState->m_Settings.m_uiMaxFeedbackRequests);
      xiiDynamicArray<xiiVirtualShadowPageRequest> requests;
      requests.Reserve(uiCount);
      for (xiiUInt32 i = 0U; i < uiCount; ++i)
      {
        const xiiGpuVirtualShadowFeedback& feedback     = pFeedback[i + 1U];
        xiiVirtualShadowPageRequest&       request      = requests.ExpandAndGetRef();
        const xiiUInt64                    uiPackedPage = static_cast<xiiUInt64>(feedback.m_uiVirtualKeyLow) | (static_cast<xiiUInt64>(feedback.m_uiVirtualKeyHigh) << 32U);
        request.m_Page                                  = xiiVirtualShadowPageId::FromPackedValue(uiPackedPage);
        request.m_uiPriority                            = feedback.m_uiPriority;
        request.m_bPinned                               = (feedback.m_uiFlags & 1U) != 0U;
      }
      pCommandList->UnmapBuffer(s_pState->m_FeedbackReadbackRing[uiSlot], xiiGALMapType::Read).IgnoreResult();
      s_pState->m_FeedbackReadbackFrames[uiSlot] = xiiMath::MaxValue<xiiUInt64>();
      SubmitFeedback(requests);
    }
    pCommandList->End();
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
  xiiUInt32       uiAllocationsRemaining = s_pState->m_Settings.m_uiMaxPageAllocations;
  const xiiUInt32 uiBasePagesPerAxis     = s_pState->m_Settings.m_uiVirtualResolution / s_pState->m_Settings.m_uiPageSize;

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
        s_pState->m_Slots[uiPhysicalPage].m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
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
      s_pState->m_PageTableUpdates.PushBack({evicted.m_Page, uiPhysicalPage, xiiVirtualShadowPageUpdateType::Unmap});
      ++s_pState->m_Stats.m_uiEvictionCount;
    }

    auto& slot                       = s_pState->m_Slots[uiPhysicalPage];
    slot.m_bAllocated                = true;
    slot.m_Mapping.m_Page            = request.m_Page;
    slot.m_Mapping.m_uiPhysicalPage  = uiPhysicalPage;
    slot.m_Mapping.m_uiLastUsedFrame = s_pState->m_uiFrameIndex;
    slot.m_Mapping.m_uiPriority      = request.m_uiPriority;
    slot.m_Mapping.m_bPinned         = request.m_bPinned;
    slot.m_Mapping.m_bNeedsRendering = true;
    slot.m_uiDirtyFrameMask          = s_pState->m_uiAllFrameMask;
    s_pState->m_PageLookup.Insert(uiKey, uiPhysicalPage);
    s_pState->m_DirtyPages.PushBack(slot.m_Mapping);
    s_pState->m_PageTableUpdates.PushBack({request.m_Page, uiPhysicalPage, xiiVirtualShadowPageUpdateType::Map});
    --uiAllocationsRemaining;
    ++s_pState->m_Stats.m_uiAllocationCount;
  }

  s_pState->m_Stats.m_uiResidentPageCount = s_pState->m_PageLookup.GetCount();
  s_pState->m_Stats.m_uiFreePageCount     = s_pState->m_FreePages.GetCount();
  s_pState->m_Stats.m_uiDirtyPageCount    = s_pState->m_DirtyPages.GetCount();
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
    if (slot.m_bAllocated && page.m_uiLightId == uiStableLightId && page.m_uiMipLevel == uiMipLevel &&
        page.m_uiPageX >= pageRegion.x && static_cast<xiiUInt64>(page.m_uiPageX) < uiMaxPageX &&
        page.m_uiPageY >= pageRegion.y && static_cast<xiiUInt64>(page.m_uiPageY) < uiMaxPageY && InvalidateSlot(*s_pState, uiPhysicalPage))
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

void xiiVirtualShadowMapManager::MarkPageRendered(xiiUInt32 uiPhysicalPage)
{
  if (!IsInitialized() || uiPhysicalPage >= s_pState->m_Slots.GetCount() || !s_pState->m_Slots[uiPhysicalPage].m_bAllocated)
    return;

  auto& mapping                                        = s_pState->m_Slots[uiPhysicalPage].m_Mapping;
  mapping.m_bNeedsRendering                            = false;
  s_pState->m_Slots[uiPhysicalPage].m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
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
  out_viewport            = xiiRectU32(uiPageX * s_pState->m_Settings.m_uiPageSize, uiPageY * s_pState->m_Settings.m_uiPageSize,
                                       s_pState->m_Settings.m_uiPageSize, s_pState->m_Settings.m_uiPageSize);
  return true;
}

bool xiiVirtualShadowMapManager::BuildPageViewProjection(const xiiMat4& cascadeViewProjection, const xiiVirtualShadowPageId& page,
                                                         xiiUInt32 uiVirtualResolution, xiiUInt32 uiPageSize, xiiMat4& out_pageViewProjection)
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

xiiVirtualShadowMapManager::UploadHandles xiiVirtualShadowMapManager::AddRasterPasses(xiiRenderGraph& graph, const UploadHandles& upload,
                                                                                      const xiiGpuVisibilityOutputs&                    visibility,
                                                                                      const xiiGeometryResidencyManager::UploadHandles& geometry,
                                                                                      xiiArrayPtr<const xiiMat4>                        cascadeViewProjections,
                                                                                      xiiUInt32 uiDirectionalLightId, xiiUInt32 uiVertexStride,
                                                                                      xiiUInt32 uiMeshDispatchGroupCountX, xiiUInt32 uiMeshDispatchGroupCountY)
{
  UploadHandles result = upload;
  if (!IsInitialized() || !xiiGpuShadowRasterManager::IsSupported() || !result.m_hPhysicalAtlas.IsValid() || !result.m_hVirtualPageTable.IsValid() || cascadeViewProjections.IsEmpty() ||
      !visibility.m_hSceneInstances.IsValid() || !visibility.m_hVisibleMeshlets.IsValid() || !visibility.m_hVisibleMeshletCount.IsValid() ||
      !visibility.m_hIndirectCommands.IsValid() || !visibility.m_hIndirectCommandCount.IsValid() || !geometry.m_hGeometryMetadata.IsValid() ||
      !geometry.m_hMeshletMetadata.IsValid() || uiVertexStride == 0U || uiMeshDispatchGroupCountX == 0U || uiMeshDispatchGroupCountY == 0U)
    return result;

  struct RenderedPage
  {
    xiiVirtualShadowPageId  m_Page;
    xiiUInt32               m_uiPhysicalPage     = xiiInvalidIndex;
    xiiUInt32               m_uiRecordByteOffset = 0U;
    xiiGpuVirtualShadowPage m_CleanRecord;
  };
  xiiDynamicArray<RenderedPage> renderedPages;

  const xiiVirtualShadowMapSettings& settings = GetConfiguration();
  xiiDynamicArray<xiiUInt32>         rasterCandidates;
  rasterCandidates.Reserve(s_pState->m_DirtyPages.GetCount());
  for (const xiiVirtualShadowPageMapping& dirtyPage : GetDirtyPages())
  {
    if (dirtyPage.m_uiPhysicalPage >= s_pState->m_Slots.GetCount())
      continue;

    const auto& slot = s_pState->m_Slots[dirtyPage.m_uiPhysicalPage];
    if (!slot.m_bAllocated || !slot.m_Mapping.m_bNeedsRendering ||
        slot.m_Mapping.m_Page.m_uiLightId != (uiDirectionalLightId & s_uiMaximumLightId) ||
        slot.m_Mapping.m_Page.m_uiMipLevel >= cascadeViewProjections.GetCount())
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
    if (s_pState->m_uiRasterizationsThisFrame >= settings.m_uiMaxPageRasterizations)
      break;

    const auto& mapping = s_pState->m_Slots[uiPhysicalPage].m_Mapping;

    xiiMat4 pageViewProjection;
    if (!BuildPageViewProjection(cascadeViewProjections[mapping.m_Page.m_uiMipLevel], mapping.m_Page,
                                 settings.m_uiVirtualResolution, settings.m_uiPageSize, pageViewProjection))
      continue;

    xiiRectU32 viewport;
    if (!GetPhysicalPageViewport(mapping.m_uiPhysicalPage, viewport))
      continue;
    const xiiVirtualShadowMapManagerState::Slot& slot = s_pState->m_Slots[uiPhysicalPage];
    if (slot.m_uiVirtualTableBucket == xiiInvalidIndex || slot.m_uiVirtualTableBucket >= result.m_uiVirtualTableCapacity)
      continue;

    xiiGpuShadowRasterDescription rasterDescription;
    rasterDescription.m_ViewProjectionMatrix      = pageViewProjection;
    rasterDescription.m_Viewport                  = xiiVec4U32(viewport.x, viewport.y, viewport.width, viewport.height);
    rasterDescription.m_uiVertexStride            = uiVertexStride;
    rasterDescription.m_uiMeshDispatchGroupCountX = uiMeshDispatchGroupCountX;
    rasterDescription.m_uiMeshDispatchGroupCountY = uiMeshDispatchGroupCountY;
    rasterDescription.m_bClearViewport            = true;

    xiiStringBuilder passName;
    passName.SetFormat("Virtual Shadow Page L{} ({}, {})", mapping.m_Page.m_uiMipLevel, mapping.m_Page.m_uiPageX, mapping.m_Page.m_uiPageY);
    result.m_hPhysicalAtlas = xiiGpuShadowRasterManager::AddPass(graph, passName, result.m_hPhysicalAtlas, visibility, geometry, rasterDescription);

    RenderedPage& renderedPage                    = renderedPages.ExpandAndGetRef();
    renderedPage.m_Page                           = mapping.m_Page;
    renderedPage.m_uiPhysicalPage                 = mapping.m_uiPhysicalPage;
    const xiiUInt64 uiVirtualKey                  = mapping.m_Page.GetPackedValue();
    renderedPage.m_uiRecordByteOffset             = (result.m_uiVirtualTableBaseIndex + slot.m_uiVirtualTableBucket) * sizeof(xiiGpuVirtualShadowPage);
    renderedPage.m_CleanRecord.m_uiVirtualKeyLow  = static_cast<xiiUInt32>(uiVirtualKey);
    renderedPage.m_CleanRecord.m_uiVirtualKeyHigh = static_cast<xiiUInt32>(uiVirtualKey >> 32U);
    renderedPage.m_CleanRecord.m_uiPhysicalPage   = mapping.m_uiPhysicalPage;
    renderedPage.m_CleanRecord.m_uiFlags          = 1U | (mapping.m_bPinned ? 4U : 0U);
    ++s_pState->m_uiRasterizationsThisFrame;
    ++s_pState->m_Stats.m_uiRasterizedPageCount;
  }

  if (renderedPages.IsEmpty())
    return result;

  struct CompletionPassData
  {
    xiiRenderGraphTextureHandle   m_hPhysicalAtlas;
    xiiRenderGraphBufferHandle    m_hVirtualPageTable;
    xiiDynamicArray<RenderedPage> m_RenderedPages;
  };

  auto completionPass = graph.AddPass<CompletionPassData>(
    "Virtual Shadow Page Completion", xiiGALCommandQueueFlags::Graphics,
    [hPhysicalAtlas = result.m_hPhysicalAtlas, hVirtualPageTable = result.m_hVirtualPageTable](CompletionPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hPhysicalAtlas    = builder.ReadTexture(hPhysicalAtlas, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVirtualPageTable = builder.WriteBuffer(hVirtualPageTable, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const CompletionPassData& data, xiiRenderGraphPassContext& context) {
      for (const RenderedPage& renderedPage : data.m_RenderedPages)
      {
        xiiVirtualShadowPageMapping currentMapping;
        if (xiiVirtualShadowMapManager::TryGetMapping(renderedPage.m_Page, currentMapping) && currentMapping.m_uiPhysicalPage == renderedPage.m_uiPhysicalPage)
        {
          context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hVirtualPageTable), renderedPage.m_uiRecordByteOffset,
                                                xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&renderedPage.m_CleanRecord), sizeof(renderedPage.m_CleanRecord)));
          xiiVirtualShadowMapManager::MarkPageRendered(renderedPage.m_uiPhysicalPage);
        }
      }
    });
  completionPass.first->m_RenderedPages = std::move(renderedPages);
  result.m_hPhysicalAtlas               = completionPass.first->m_hPhysicalAtlas;
  result.m_hVirtualPageTable            = completionPass.first->m_hVirtualPageTable;
  return result;
}

xiiVirtualShadowMapManager::UploadHandles xiiVirtualShadowMapManager::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  UploadHandles result;
  if (!IsInitialized())
    return result;
  if ((s_pState->m_pPhysicalPageTable == nullptr || s_pState->m_pVirtualPageTable == nullptr) && CreateGpuResources().Failed())
    return result;

  struct Upload
  {
    xiiUInt32               m_uiPhysicalPage = 0U;
    xiiUInt32               m_uiByteOffset   = 0U;
    xiiUInt64               m_uiFrameBit     = 0U;
    xiiGpuVirtualShadowPage m_Record;
  };
  struct UploadPassData
  {
    xiiRenderGraphBufferHandle               m_hPhysicalPageTable;
    xiiRenderGraphBufferHandle               m_hVirtualPageTable;
    xiiRenderGraphTextureHandle              m_hPhysicalAtlas;
    xiiDynamicArray<Upload>                  m_PhysicalUploads;
    xiiDynamicArray<xiiGpuVirtualShadowPage> m_VirtualTable;
    xiiUInt32                                m_uiVirtualTableByteOffset = 0U;
  };

  auto pass = graph.AddPass<UploadPassData>(
    "Virtual Shadow Page Table Upload", xiiGALCommandQueueFlags::Transfer,
    [](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hPhysicalPageTable = builder.ImportBuffer(xiiRGBlackboardKeys::k_VirtualShadowPhysicalPageTable, s_pState->m_pPhysicalPageTable, xiiGALResourceStateFlags::ShaderResource);
      data.m_hPhysicalPageTable = builder.WriteBuffer(data.m_hPhysicalPageTable, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hPhysicalPageTable, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVirtualPageTable = builder.ImportBuffer(xiiRGBlackboardKeys::k_VirtualShadowPageTable, s_pState->m_pVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVirtualPageTable = builder.WriteBuffer(data.m_hVirtualPageTable, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
      data.m_hPhysicalAtlas = builder.ImportTexture(xiiRGBlackboardKeys::k_VirtualShadowAtlas, s_pState->m_pPhysicalAtlas, s_pState->m_pPhysicalAtlas->GetResourceState());
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      for (const Upload& upload : data.m_PhysicalUploads)
        cmd.UpdateBuffer(context.GetBuffer(data.m_hPhysicalPageTable), upload.m_uiByteOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&upload.m_Record), sizeof(upload.m_Record)));

      if (!data.m_VirtualTable.IsEmpty())
      {
        cmd.UpdateBuffer(context.GetBuffer(data.m_hVirtualPageTable), data.m_uiVirtualTableByteOffset,
                         xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_VirtualTable.GetData()), data.m_VirtualTable.GetCount() * sizeof(xiiGpuVirtualShadowPage)));
      }

      for (const Upload& upload : data.m_PhysicalUploads)
      {
        if (upload.m_uiPhysicalPage >= s_pState->m_Slots.GetCount())
          continue;
        auto&           slot    = s_pState->m_Slots[upload.m_uiPhysicalPage];
        const xiiUInt64 uiKey   = slot.m_bAllocated ? slot.m_Mapping.m_Page.GetPackedValue() : 0U;
        const xiiUInt32 uiFlags = slot.m_bAllocated ? (1U | (slot.m_Mapping.m_bNeedsRendering ? 2U : 0U) | (slot.m_Mapping.m_bPinned ? 4U : 0U)) : 0U;
        if (upload.m_Record.m_uiVirtualKeyLow == static_cast<xiiUInt32>(uiKey) && upload.m_Record.m_uiVirtualKeyHigh == static_cast<xiiUInt32>(uiKey >> 32U) && upload.m_Record.m_uiFlags == uiFlags)
          slot.m_uiDirtyFrameMask &= ~upload.m_uiFrameBit;
      }
    },
    true);

  const xiiUInt32 uiFrameSlice     = static_cast<xiiUInt32>(uiFrameIndex % s_pState->m_Settings.m_uiFramesInFlight);
  const xiiUInt64 uiFrameBit       = xiiUInt64(1) << uiFrameSlice;
  result.m_hPhysicalPageTable      = pass.first->m_hPhysicalPageTable;
  result.m_hVirtualPageTable       = pass.first->m_hVirtualPageTable;
  result.m_hPhysicalAtlas          = pass.first->m_hPhysicalAtlas;
  result.m_uiFrameBaseIndex        = uiFrameSlice * s_pState->m_Settings.m_uiPhysicalPageCount;
  result.m_uiPhysicalPageCount     = s_pState->m_Settings.m_uiPhysicalPageCount;
  result.m_uiVirtualTableBaseIndex = uiFrameSlice * s_pState->m_uiVirtualPageTableCapacity;
  result.m_uiVirtualTableCapacity  = s_pState->m_uiVirtualPageTableCapacity;

  pass.first->m_uiVirtualTableByteOffset = result.m_uiVirtualTableBaseIndex * sizeof(xiiGpuVirtualShadowPage);
  pass.first->m_VirtualTable.SetCount(s_pState->m_uiVirtualPageTableCapacity);
  xiiMemoryUtils::ZeroFill(pass.first->m_VirtualTable.GetData(), pass.first->m_VirtualTable.GetCount());
  for (xiiGpuVirtualShadowPage& record : pass.first->m_VirtualTable)
    record.m_uiPhysicalPage = xiiInvalidIndex;
  for (auto& slot : s_pState->m_Slots)
    slot.m_uiVirtualTableBucket = xiiInvalidIndex;

  const xiiUInt32 uiVirtualTableMask = s_pState->m_uiVirtualPageTableCapacity - 1U;
  for (xiiUInt32 uiPhysicalPage = 0U; uiPhysicalPage < s_pState->m_Slots.GetCount(); ++uiPhysicalPage)
  {
    auto& slot = s_pState->m_Slots[uiPhysicalPage];
    if (!slot.m_bAllocated)
      continue;

    const xiiUInt64 uiKey        = slot.m_Mapping.m_Page.GetPackedValue();
    const xiiUInt32 uiKeyLow     = static_cast<xiiUInt32>(uiKey);
    const xiiUInt32 uiKeyHigh    = static_cast<xiiUInt32>(uiKey >> 32U);
    xiiUInt32       uiTableIndex = HashVirtualPageKey(uiKeyLow, uiKeyHigh) & uiVirtualTableMask;
    while ((pass.first->m_VirtualTable[uiTableIndex].m_uiFlags & 1U) != 0U)
      uiTableIndex = (uiTableIndex + 1U) & uiVirtualTableMask;

    xiiGpuVirtualShadowPage& record = pass.first->m_VirtualTable[uiTableIndex];
    record.m_uiVirtualKeyLow        = uiKeyLow;
    record.m_uiVirtualKeyHigh       = uiKeyHigh;
    record.m_uiPhysicalPage         = uiPhysicalPage;
    record.m_uiFlags                = 1U | (slot.m_Mapping.m_bNeedsRendering ? 2U : 0U) | (slot.m_Mapping.m_bPinned ? 4U : 0U);
    slot.m_uiVirtualTableBucket     = uiTableIndex;
  }

  for (xiiUInt32 uiPhysicalPage = 0U; uiPhysicalPage < s_pState->m_Slots.GetCount(); ++uiPhysicalPage)
  {
    const auto& slot = s_pState->m_Slots[uiPhysicalPage];
    if ((slot.m_uiDirtyFrameMask & uiFrameBit) == 0U)
      continue;

    Upload& upload                     = pass.first->m_PhysicalUploads.ExpandAndGetRef();
    upload.m_uiPhysicalPage            = uiPhysicalPage;
    upload.m_uiByteOffset              = (result.m_uiFrameBaseIndex + uiPhysicalPage) * sizeof(xiiGpuVirtualShadowPage);
    upload.m_uiFrameBit                = uiFrameBit;
    const xiiUInt64 uiKey              = slot.m_bAllocated ? slot.m_Mapping.m_Page.GetPackedValue() : 0U;
    upload.m_Record.m_uiVirtualKeyLow  = static_cast<xiiUInt32>(uiKey);
    upload.m_Record.m_uiVirtualKeyHigh = static_cast<xiiUInt32>(uiKey >> 32U);
    upload.m_Record.m_uiPhysicalPage   = uiPhysicalPage;
    upload.m_Record.m_uiFlags          = slot.m_bAllocated ? (1U | (slot.m_Mapping.m_bNeedsRendering ? 2U : 0U) | (slot.m_Mapping.m_bPinned ? 4U : 0U)) : 0U;
  }

  return result;
}

void xiiVirtualShadowMapManager::AddFeedbackPasses(xiiRenderGraph& graph, xiiRenderGraphTextureHandle hSceneDepth,
                                                   xiiRenderGraphBufferHandle hCascadeConstants, xiiUInt32 uiWidth, xiiUInt32 uiHeight,
                                                   const xiiMat4& inverseViewProjection, float fNearPlane,
                                                   xiiUInt32 uiDirectionalLightId, xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized() || !hSceneDepth.IsValid() || !hCascadeConstants.IsValid() || uiWidth == 0U || uiHeight == 0U)
    return;
  if ((s_pState->m_pFeedbackBuffer == nullptr || s_pState->m_FeedbackReadbackRing.IsEmpty()) && CreateGpuResources().Failed())
    return;

  if (s_pState->m_pFeedbackPipeline == nullptr)
  {
    const xiiShaderResourceHandle                  hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/VirtualShadowFeedback.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
    const xiiShaderPermutationResourceHandle       hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
    xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid())
      return;
    xiiGALComputePipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
    pipelineDescription.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    s_pState->m_pFeedbackPipeline                    = xiiGALPipelineCache::GetPipeline(pipelineDescription);
    if (s_pState->m_pFeedbackPipeline == nullptr)
      return;
  }

  struct FeedbackPassData
  {
    xiiRenderGraphTextureHandle m_hSceneDepth;
    xiiRenderGraphBufferHandle  m_hCascadeConstants;
    xiiRenderGraphBufferHandle  m_hFeedback;
    xiiRenderGraphBufferHandle  m_hConstants;
    xiiUInt32                   m_uiWidth               = 0U;
    xiiUInt32                   m_uiHeight              = 0U;
    xiiUInt32                   m_uiLightId             = 0U;
    xiiMat4                     m_InverseViewProjection = xiiMat4::MakeIdentity();
    float                       m_fNearPlane            = 0.1f;
    bool                        m_bClear                = false;
  };

  const bool bClear                = s_pState->m_uiFeedbackClearFrame != uiFrameIndex;
  s_pState->m_uiFeedbackClearFrame = uiFrameIndex;
  auto feedbackPass                = graph.AddPass<FeedbackPassData>(
    "Virtual Shadow Feedback", xiiGALCommandQueueFlags::Compute,
    [hSceneDepth, hCascadeConstants](FeedbackPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hSceneDepth       = builder.ReadTexture(hSceneDepth, xiiGALResourceStateFlags::ShaderResource);
      data.m_hCascadeConstants = builder.ReadBuffer(hCascadeConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_hFeedback         = builder.ImportBuffer("Virtual Shadow Feedback", s_pState->m_pFeedbackBuffer, s_pState->m_pFeedbackBuffer->GetResourceState());
      data.m_hFeedback         = builder.WriteBuffer(data.m_hFeedback, xiiGALResourceStateFlags::UnorderedAccess);
      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiVirtualShadowFeedbackConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer("xiiVirtualShadowFeedbackConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
    },
    [](const FeedbackPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd       = context.GetCommandList();
      xiiGALBuffer*      pFeedback = context.GetBuffer(data.m_hFeedback);
      if (data.m_bClear)
      {
        const xiiGpuVirtualShadowFeedback zero = {};
        cmd.UpdateBuffer(pFeedback, 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&zero), sizeof(zero)));
      }
      {
        xiiGALMapHelper<xiiVirtualShadowFeedbackConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->VirtualShadowInverseViewProjection = data.m_InverseViewProjection;
        constants->VirtualResolution                  = s_pState->m_Settings.m_uiVirtualResolution;
        constants->PageSize                           = s_pState->m_Settings.m_uiPageSize;
        constants->MaxFeedbackRequests                = s_pState->m_Settings.m_uiMaxFeedbackRequests;
        constants->DirectionalLightId                 = data.m_uiLightId;
        constants->VirtualShadowNearPlane             = data.m_fNearPlane;
        constants->_VirtualShadowFeedbackPadding      = xiiVec3::MakeZero();
      }
      cmd.SetPipelineState(s_pState->m_pFeedbackPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiVirtualShadowFeedbackConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hCascadeConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VirtualShadowFeedback", pFeedback->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(data.m_uiWidth + 63U) / 64U, (data.m_uiHeight + 63U) / 64U, 1U});
    });
  feedbackPass.first->m_uiWidth               = uiWidth;
  feedbackPass.first->m_uiHeight              = uiHeight;
  feedbackPass.first->m_uiLightId             = uiDirectionalLightId & 0x00FFFFFFU;
  feedbackPass.first->m_InverseViewProjection = inverseViewProjection;
  feedbackPass.first->m_fNearPlane            = xiiMath::Max(fNearPlane, 0.0001f);
  feedbackPass.first->m_bClear                = bClear;

  struct ReadbackPassData
  {
    xiiRenderGraphBufferHandle m_hFeedback;
    xiiRenderGraphBufferHandle m_hReadback;
    xiiUInt32                  m_uiSlot  = 0U;
    xiiUInt64                  m_uiFrame = 0U;
  };
  const xiiUInt32 uiSlot       = static_cast<xiiUInt32>(uiFrameIndex % s_pState->m_Settings.m_uiFramesInFlight);
  auto            readbackPass = graph.AddPass<ReadbackPassData>(
    "Virtual Shadow Feedback Readback", xiiGALCommandQueueFlags::Transfer,
    [hFeedback = feedbackPass.first->m_hFeedback, uiSlot](ReadbackPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hFeedback = builder.ReadBuffer(hFeedback, xiiGALResourceStateFlags::CopySource);
      data.m_hReadback = builder.ImportBuffer("Virtual Shadow Feedback Readback", s_pState->m_FeedbackReadbackRing[uiSlot], s_pState->m_FeedbackReadbackRing[uiSlot]->GetResourceState());
      data.m_hReadback = builder.WriteBuffer(data.m_hReadback, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hFeedback, xiiGALResourceStateFlags::UnorderedAccess);
      builder.ExportBuffer(data.m_hReadback, xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const ReadbackPassData& data, xiiRenderGraphPassContext& context) {
      context.GetCommandList().CopyBuffer(context.GetBuffer(data.m_hFeedback), context.GetBuffer(data.m_hReadback));
      s_pState->m_FeedbackReadbackFrames[data.m_uiSlot] = data.m_uiFrame;
    },
    true);
  readbackPass.first->m_uiSlot  = uiSlot;
  readbackPass.first->m_uiFrame = uiFrameIndex;
}
