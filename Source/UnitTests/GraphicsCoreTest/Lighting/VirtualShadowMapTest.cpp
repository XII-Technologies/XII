/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/VirtualShadowMap.h>

namespace
{
  xiiVirtualShadowPageRequest MakeRequest(xiiUInt32 uiLight, xiiUInt32 uiX, xiiUInt32 uiPriority, bool bPinned = false)
  {
    xiiVirtualShadowPageRequest request;
    request.m_Page.m_uiLightId = uiLight;
    request.m_Page.m_uiPageX   = uiX;
    request.m_Page.m_uiPageY   = 0U;
    request.m_uiPriority       = uiPriority;
    request.m_bPinned          = bPinned;
    return request;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Lighting, VirtualShadowMapResidency)
{
  {
    xiiVirtualShadowPageId page;
    page.m_uiLightId  = 0x00ABCDEU;
    page.m_uiMipLevel = 47U;
    page.m_uiPageX    = 0x1ABCDU;
    page.m_uiPageY    = 0x12345U;

    const xiiVirtualShadowPageId decoded = xiiVirtualShadowPageId::FromPackedValue(page.GetPackedValue());
    XII_TEST_INT(decoded.m_uiLightId, page.m_uiLightId);
    XII_TEST_INT(decoded.m_uiMipLevel, page.m_uiMipLevel);
    XII_TEST_INT(decoded.m_uiPageX, page.m_uiPageX);
    XII_TEST_INT(decoded.m_uiPageY, page.m_uiPageY);
  }

  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution   = 1024U;
  settings.m_uiPageSize            = 128U;
  settings.m_uiPhysicalPageCount   = 2U;
  settings.m_uiMaxFeedbackRequests = 16U;
  settings.m_uiMaxPageAllocations  = 2U;
  settings.m_uiFramesInFlight      = 2U;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::Configure(settings).Succeeded());

  const xiiVirtualShadowMapStats configuredStats = xiiVirtualShadowMapManager::GetStats();
  XII_TEST_INT(configuredStats.m_uiPhysicalAtlasWidth, 256U);
  XII_TEST_INT(configuredStats.m_uiPhysicalAtlasHeight, 128U);
  xiiRectU32 secondPageViewport;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::GetPhysicalPageViewport(1U, secondPageViewport));
  XII_TEST_INT(secondPageViewport.x, 128U);
  XII_TEST_INT(secondPageViewport.y, 0U);
  XII_TEST_INT(secondPageViewport.width, 128U);
  XII_TEST_INT(secondPageViewport.height, 128U);
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::GetPhysicalPageViewport(2U, secondPageViewport));

  xiiVirtualShadowMapManager::BeginFrame(1U, 0U);
  xiiVirtualShadowPageRequest initial[] = {
    MakeRequest(1U, 0U, 10U),
    MakeRequest(1U, 0U, 1U), // duplicate must not consume a page
    MakeRequest(2U, 0U, 20U, true),
  };
  xiiVirtualShadowMapManager::SubmitFeedback(initial);

  xiiVirtualShadowMapStats stats = xiiVirtualShadowMapManager::GetStats();
  XII_TEST_INT(stats.m_uiUniqueRequestCount, 2U);
  XII_TEST_INT(stats.m_uiResidentPageCount, 2U);
  XII_TEST_INT(stats.m_uiAllocationCount, 2U);
  XII_TEST_INT(stats.m_uiDirtyPageCount, 2U);

  xiiVirtualShadowPageMapping mapping;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(initial[0].m_Page, mapping));
  const xiiUInt32 uiFirstPhysicalPage = mapping.m_uiPhysicalPage;
  xiiVirtualShadowMapManager::MarkPageRendered(uiFirstPhysicalPage);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 1U);

  // Frame 1 is still in flight, so neither page can be reused.
  xiiVirtualShadowMapManager::BeginFrame(2U, 0U);
  const xiiVirtualShadowPageRequest blocked = MakeRequest(3U, 0U, 100U);
  xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&blocked, 1U));
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::TryGetMapping(blocked.m_Page, mapping));
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDroppedRequestCount, 1U);

  // Once frame 1 retires, the unpinned oldest page can be evicted. The pinned
  // page remains resident regardless of its age.
  xiiVirtualShadowMapManager::BeginFrame(3U, 1U);
  xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&blocked, 1U));
  XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(blocked.m_Page, mapping));
  XII_TEST_INT(mapping.m_uiPhysicalPage, uiFirstPhysicalPage);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiEvictionCount, 1U);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetPageTableUpdates().GetCount(), 2U);

  // Restore production defaults for any tests executing after this one.
  XII_TEST_BOOL(xiiVirtualShadowMapManager::Configure(xiiVirtualShadowMapSettings()).Succeeded());
}
