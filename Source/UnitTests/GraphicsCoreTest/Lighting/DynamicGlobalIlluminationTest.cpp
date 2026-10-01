/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>

XII_CREATE_SIMPLE_TEST(Lighting, DynamicGlobalIllumination)
{
  xiiDDGISettings settings;
  settings.m_uiProbeCountX              = 4U;
  settings.m_uiProbeCountY              = 2U;
  settings.m_uiProbeCountZ              = 4U;
  settings.m_fProbeSpacing              = 2.0f;
  settings.m_uiProbeUpdateBudget        = 32U;
  settings.m_fTemporalHysteresis        = 0.9f;
  settings.m_fMaximumRelocationDistance = 0.75f;
  XII_TEST_BOOL(xiiDDGIManager::Configure(settings).Succeeded());

  xiiDDGIManager::BeginFrame(xiiVec3::MakeZero(), 1U);
  XII_TEST_INT(xiiDDGIManager::GetFrameStats().m_uiProbeCount, 32U);
  XII_TEST_INT(xiiDDGIManager::GetFrameStats().m_uiInvalidProbeCount, 32U);
  XII_TEST_INT(xiiDDGIManager::GetFrameStats().m_uiScrolledProbeCount, 32U);
  XII_TEST_INT(xiiDDGIManager::GetScheduledUpdates().GetCount(), 32U);

  for (const xiiDDGIProbeUpdate& update : xiiDDGIManager::GetScheduledUpdates())
    xiiDDGIManager::CommitProbeUpdate(update.m_uiPhysicalProbe, xiiVec3::MakeZero(), true);

  // Moving by one probe cell preserves the overlapping toroidal history and
  // invalidates only one newly exposed YZ slab.
  xiiDDGIManager::BeginFrame(xiiVec3(2.1f, 0.0f, 0.0f), 2U);
  XII_TEST_INT(xiiDDGIManager::GetFrameStats().m_uiScrolledProbeCount, 8U);
  XII_TEST_INT(xiiDDGIManager::GetFrameStats().m_uiInvalidProbeCount, 8U);

  const xiiDDGIProbeUpdate& relocatedUpdate = xiiDDGIManager::GetScheduledUpdates()[0];
  xiiDDGIManager::CommitProbeUpdate(relocatedUpdate.m_uiPhysicalProbe, xiiVec3(10.0f, 0.0f, 0.0f), true, true);
  const xiiDDGIProbeState& relocatedProbe = xiiDDGIManager::GetProbes()[relocatedUpdate.m_uiPhysicalProbe];
  XII_TEST_FLOAT(relocatedProbe.m_vRelocationOffset.GetLength(), settings.m_fMaximumRelocationDistance, 0.001f);
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid));
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::Relocated));
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::InsideGeometry));

  XII_TEST_BOOL(xiiDDGIManager::Configure(xiiDDGISettings()).Succeeded());
}
