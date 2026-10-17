/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/Types/ScopeExit.h>
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
  settings.m_uiVirtualResolution     = 1024U;
  settings.m_uiPageSize              = 128U;
  settings.m_uiPhysicalPageCount     = 2U;
  settings.m_uiMaxFeedbackRequests   = 16U;
  settings.m_uiMaxPageAllocations    = 2U;
  settings.m_uiMaxPageRasterizations = 2U;
  settings.m_uiFramesInFlight        = 2U;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::Configure(settings).Succeeded());

  const xiiVirtualShadowMapStats configuredStats = xiiVirtualShadowMapManager::GetStats();
  XII_TEST_INT(configuredStats.m_uiPhysicalAtlasWidth, 256U);
  XII_TEST_INT(configuredStats.m_uiPhysicalAtlasHeight, 128U);
  XII_TEST_INT(configuredStats.m_uiVirtualPageTableCapacity, 4U);
  xiiRectU32 secondPageViewport;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::GetPhysicalPageViewport(1U, secondPageViewport));
  XII_TEST_INT(secondPageViewport.x, 128U);
  XII_TEST_INT(secondPageViewport.y, 0U);
  XII_TEST_INT(secondPageViewport.width, 128U);
  XII_TEST_INT(secondPageViewport.height, 128U);
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::GetPhysicalPageViewport(2U, secondPageViewport));

  {
    xiiVirtualShadowPageId page;
    page.m_uiLightId  = 1U;
    page.m_uiMipLevel = 1U;
    page.m_uiPageX    = 1U;
    page.m_uiPageY    = 2U;

    xiiMat4 pageProjection;
    XII_TEST_BOOL(xiiVirtualShadowMapManager::BuildPageViewProjection(xiiMat4::MakeIdentity(), page, 1024U, 128U, pageProjection));
    XII_TEST_VEC4(pageProjection * xiiVec4(-0.5f, 0.0f, 0.25f, 1.0f), xiiVec4(-1.0f, 1.0f, 0.25f, 1.0f), 0.0001f);
    XII_TEST_VEC4(pageProjection * xiiVec4(0.0f, -0.5f, 0.25f, 1.0f), xiiVec4(1.0f, -1.0f, 0.25f, 1.0f), 0.0001f);

    page.m_uiPageX = 4U;
    XII_TEST_BOOL(!xiiVirtualShadowMapManager::BuildPageViewProjection(xiiMat4::MakeIdentity(), page, 1024U, 128U, pageProjection));
  }

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
  const xiiUInt32                             uiFirstPhysicalPage = mapping.m_uiPhysicalPage;
  const auto                                  projection          = xiiMat4::MakeIdentity();
  xiiDynamicArray<xiiVirtualShadowRasterPage> work;
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 1, work);
  if (!XII_TEST_INT(work.GetCount(), 1))
    return;
  XII_TEST_BOOL(xiiVirtualShadowMapManager::MarkPageRendered(work[0]));
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 1U);

  XII_TEST_INT(xiiVirtualShadowMapManager::InvalidateLight(1U), 1U);
  XII_TEST_INT(xiiVirtualShadowMapManager::InvalidateLight(1U), 0U); // already dirty
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiInvalidationCount, 1U);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 2U);
  XII_TEST_BOOL(xiiVirtualShadowMapManager::InvalidatePage(initial[0].m_Page));
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(work[0]));
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 1, work);
  if (!XII_TEST_INT(work.GetCount(), 1))
    return;
  const auto oldWork = work[0];
  XII_TEST_BOOL(xiiVirtualShadowMapManager::MarkPageRendered(work[0]));
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
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(oldWork));
  XII_TEST_BOOL(mapping.m_bNeedsRendering);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiEvictionCount, 1U);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetPageTableUpdates().GetCount(), 2U);

  // Restore production defaults for any tests executing after this one.
  XII_TEST_BOOL(xiiVirtualShadowMapManager::Configure(xiiVirtualShadowMapSettings()).Succeeded());
}

XII_CREATE_SIMPLE_TEST(Lighting, VirtualShadowRasterPlanning)
{
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution = 64;
  settings.m_uiPageSize          = 16;
  settings.m_uiPhysicalPageCount = settings.m_uiMaxPageAllocations = 3;
  settings.m_uiMaxPageRasterizations                               = 2;
  settings.m_uiMaxFeedbackRequests                                 = 8;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
    return;
  XII_SCOPE_EXIT(xiiVirtualShadowMapManager::Configure(xiiVirtualShadowMapSettings()).IgnoreResult());
  xiiVirtualShadowMapManager::BeginFrame(1, 0);
  xiiVirtualShadowPageRequest requests[] = {MakeRequest(7, 0, 10), MakeRequest(7, 1, 1, true), MakeRequest(7, 2, 20)};
  xiiVirtualShadowMapManager::SubmitFeedback(requests);
  const auto                                  projection = xiiMat4::MakeIdentity();
  xiiDynamicArray<xiiVirtualShadowRasterPage> planned, repeated;
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 7, planned);
  if (!XII_TEST_INT(planned.GetCount(), 2))
    return;
  XII_TEST_INT(planned[0].m_Page.m_uiPageX, 1); // Pinned content precedes higher priority unpinned work.
  XII_TEST_INT(planned[1].m_Page.m_uiPageX, 2);
  XII_TEST_BOOL(xiiVirtualShadowMapManager::IsRasterWorkCurrent(planned[0]));
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 7, repeated);
  XII_TEST_BOOL(repeated.IsEmpty());
  XII_TEST_BOOL(xiiVirtualShadowMapManager::MarkPageRendered(planned[0]));
  XII_TEST_BOOL(xiiVirtualShadowMapManager::InvalidatePage(planned[1].m_Page)); // Already dirty, but its captured content changed.
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::IsRasterWorkCurrent(planned[1]));
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(planned[1]));
  xiiVirtualShadowMapManager::BeginFrame(2, 0);
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 7, repeated);
  if (!XII_TEST_INT(repeated.GetCount(), 2))
    return;
  XII_TEST_INT(repeated[0].m_Page.m_uiPageX, 2);
  XII_TEST_BOOL(repeated[0].m_uiContentRevision != planned[1].m_uiContentRevision);
  const auto abandoned = repeated[0];
  xiiVirtualShadowMapManager::BeginFrame(3, 1);
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 7, repeated);
  if (!XII_TEST_INT(repeated.GetCount(), 2))
    return;
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::IsRasterWorkCurrent(abandoned));
  XII_TEST_BOOL(repeated[0].m_uiContentRevision == abandoned.m_uiContentRevision);
  const auto beforeReset = repeated[0];
  XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings));
  xiiVirtualShadowMapManager::BeginFrame(1, 0);
  xiiVirtualShadowMapManager::SubmitFeedback(requests);
  xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 7, repeated);
  if (!XII_TEST_INT(repeated.GetCount(), 2))
    return;
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(beforeReset));
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDirtyPageCount, 3);
}


XII_CREATE_SIMPLE_TEST(Lighting, VirtualShadowSharedFeedbackBudget)
{
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution  = 64;
  settings.m_uiPageSize           = 16;
  settings.m_uiPhysicalPageCount  = 8;
  settings.m_uiMaxPageAllocations = 2;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
    return;
  XII_SCOPE_EXIT(xiiVirtualShadowMapManager::Configure(xiiVirtualShadowMapSettings()).IgnoreResult());
  const auto revision = xiiVirtualShadowMapManager::GetConfigurationRevision();
  XII_TEST_BOOL(revision != 0);
  xiiVirtualShadowMapManager::BeginFrame(1, 0);
  const xiiVirtualShadowPageRequest camera[] = {MakeRequest(1, 0, 10), MakeRequest(1, 1, 9)};
  const auto                        sensor   = MakeRequest(2, 0, 20);
  xiiVirtualShadowMapManager::SubmitFeedback(camera);
  xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&sensor, 1));
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiAllocationCount, 2);
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiDroppedRequestCount, 1);
  xiiVirtualShadowPageMapping mapping;
  XII_TEST_BOOL(!xiiVirtualShadowMapManager::TryGetMapping(sensor.m_Page, mapping));
  xiiVirtualShadowMapManager::BeginFrame(2, 1);
  xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&sensor, 1));
  XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(sensor.m_Page, mapping));
  XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiAllocationCount, 1);
  XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings));
  XII_TEST_BOOL(xiiVirtualShadowMapManager::GetConfigurationRevision() != revision);
}
