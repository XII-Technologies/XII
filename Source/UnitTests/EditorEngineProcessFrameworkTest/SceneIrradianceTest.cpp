/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/RayAnalyticLightingTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/RayEmissiveLightingTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

XII_CREATE_SIMPLE_TEST(EditorViews, GeometryDependentDiffuseTransportAndIndependentWorlds) { xiiSceneIrradianceTest::Run(true); }

XII_CREATE_SIMPLE_TEST(EditorViews, OffscreenRayDiffuseTransportAndWorldRetirement) { xiiSceneIrradianceTest::Run(true, true); }

XII_CREATE_SIMPLE_TEST(EditorViews, AnalyticRayLightingVisibilityAndViewSelection) { xiiRayAnalyticLightingTest::Run(true); }

XII_CREATE_SIMPLE_TEST(EditorViews, EmissiveRaySamplingEnergyVisibilityAndPanelReference)
{
  xiiRayEmissiveLightingTest::Run(true);
}
