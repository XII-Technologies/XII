/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/RayAnalyticLightingTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>
#include <GraphicsFoundationTest/Pipeline/TemporalIrradianceTestUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneIrradiance);
XII_CREATE_SIMPLE_TEST(SceneIrradiance, GeometryTransportsEmissionToDiffuseReceivers) { xiiSceneIrradianceTest::Run(false); }

XII_CREATE_SIMPLE_TEST(SceneIrradiance, OffscreenRayDiffuseTransportAndWorldRetirement) { xiiSceneIrradianceTest::Run(false, true); }

XII_CREATE_SIMPLE_TEST(SceneIrradiance, AuthoredScreenFallbackWithoutRayHardware) { xiiSceneIrradianceTest::Run(false, true, false); }

XII_CREATE_SIMPLE_TEST(SceneIrradiance, AnalyticRayLightingVisibilityAndViewSelection) { xiiRayAnalyticLightingTest::Run(false); }

XII_CREATE_SIMPLE_TEST(SceneIrradiance, TemporalIrradianceReconstruction) { xiiTemporalIrradianceTest::Run(false); }
