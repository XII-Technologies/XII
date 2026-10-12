/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>

XII_CREATE_SIMPLE_TEST(Lighting, DynamicGlobalIllumination)
{
  xiiDDGIContext  context;
  xiiDDGISettings settings;
  settings.m_uiProbeCountX              = 4U;
  settings.m_uiProbeCountY              = 2U;
  settings.m_uiProbeCountZ              = 4U;
  settings.m_fProbeSpacing              = 2.0f;
  settings.m_uiProbeUpdateBudget        = 32U;
  settings.m_fTemporalHysteresis        = 0.9f;
  settings.m_fMaximumRelocationDistance = 0.75f;
  XII_TEST_BOOL(context.Initialize(settings).Succeeded());

  XII_TEST_RESULT(context.BeginFrame(xiiVec3::MakeZero(), 1U));
  XII_TEST_INT(context.GetFrameStats().m_uiProbeCount, 32U);
  XII_TEST_INT(context.GetFrameStats().m_uiInvalidProbeCount, 32U);
  XII_TEST_INT(context.GetFrameStats().m_uiScrolledProbeCount, 32U);
  XII_TEST_INT(context.GetScheduledUpdates().GetCount(), 32U);

  for (const xiiDDGIProbeUpdate& update : context.GetScheduledUpdates())
    context.CommitProbeUpdate(update.m_uiPhysicalProbe, xiiVec3::MakeZero(), true);

  // Moving by one probe cell preserves the overlapping toroidal history and
  // invalidates only one newly exposed YZ slab.
  XII_TEST_RESULT(context.BeginFrame(xiiVec3(2.1f, 0.0f, 0.0f), 2U));
  XII_TEST_INT(context.GetFrameStats().m_uiScrolledProbeCount, 8U);
  XII_TEST_INT(context.GetFrameStats().m_uiInvalidProbeCount, 8U);

  const xiiDDGIProbeUpdate& relocatedUpdate = context.GetScheduledUpdates()[0];
  context.CommitProbeUpdate(relocatedUpdate.m_uiPhysicalProbe, xiiVec3(10.0f, 0.0f, 0.0f), true, true);
  const xiiDDGIProbeState& relocatedProbe = context.GetProbes()[relocatedUpdate.m_uiPhysicalProbe];
  XII_TEST_FLOAT(relocatedProbe.m_vRelocationOffset.GetLength(), settings.m_fMaximumRelocationDistance, 0.001f);
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::Valid));
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::Relocated));
  XII_TEST_BOOL(relocatedProbe.m_Flags.IsSet(xiiDDGIProbeFlags::InsideGeometry));
}

XII_CREATE_SIMPLE_TEST(Lighting, IndependentDDGIContexts)
{
  xiiDDGIContext first, second;
  XII_TEST_BOOL(!first.IsInitialized());
  xiiDDGISettings settings;
  settings.m_uiProbeCountX = settings.m_uiProbeCountY = settings.m_uiProbeCountZ = 2;
  settings.m_uiProbeUpdateBudget                                                 = 8;
  XII_TEST_RESULT(first.Initialize(settings));
  XII_TEST_RESULT(second.Initialize(settings));
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 1));
  for (const auto& update : first.GetScheduledUpdates())
    first.CommitProbeUpdate(update.m_uiPhysicalProbe, xiiVec3::MakeZero(), true);
  const auto retained = first.GetProbes()[0];
  XII_TEST_RESULT(second.BeginFrame(xiiVec3(1000, 2000, -3000), 1));
  for (const auto& update : second.GetScheduledUpdates())
    second.CommitProbeUpdate(update.m_uiPhysicalProbe, xiiVec3::MakeZero(), true);
  XII_TEST_BOOL(first.GetProbes()[0].m_vWorldPosition == retained.m_vWorldPosition);
  XII_TEST_BOOL(first.GetProbes()[0].m_Flags.IsSet(xiiDDGIProbeFlags::Valid));
  XII_TEST_BOOL(first.GetProbes()[0].m_vWorldPosition != second.GetProbes()[0].m_vWorldPosition);
  XII_TEST_BOOL(first.BeginFrame(xiiVec3(5000), 1).Failed());
  XII_TEST_BOOL(first.GetProbes()[0].m_vWorldPosition == retained.m_vWorldPosition);
  auto invalid            = settings;
  invalid.m_uiProbeCountX = 0;
  XII_TEST_BOOL(first.Initialize(invalid).Failed());
  XII_TEST_INT(first.GetProbes().GetCount(), 8);
  XII_TEST_BOOL(first.GetProbes()[0].m_Flags.IsSet(xiiDDGIProbeFlags::Valid));
  invalid.m_uiProbeCountX = invalid.m_uiProbeCountY = 4194304;
  invalid.m_uiProbeCountZ                           = 1048576;
  XII_TEST_BOOL(first.Initialize(invalid).Failed());
  first.InvalidateHistory();
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 2));
  XII_TEST_INT(first.GetFrameStats().m_uiInvalidProbeCount, 8);
  for (const auto& update : first.GetScheduledUpdates())
    XII_TEST_FLOAT(update.m_fHistoryWeight, 0, 0);
  XII_TEST_BOOL(second.GetProbes()[0].m_Flags.IsSet(xiiDDGIProbeFlags::Valid));
  first.Shutdown();
  first.Shutdown();
  XII_TEST_BOOL(!first.IsInitialized());
  XII_TEST_BOOL(first.GetProbes().IsEmpty());
  XII_TEST_BOOL(second.IsInitialized());
  XII_TEST_RESULT(first.Initialize(settings));
  XII_TEST_RESULT(first.BeginFrame(xiiVec3::MakeZero(), 1));
  XII_TEST_INT(first.GetFrameStats().m_uiScrolledProbeCount, 8);
}
