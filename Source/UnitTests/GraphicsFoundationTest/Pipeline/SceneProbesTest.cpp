/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/SceneProbeTestUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneProbes);
XII_CREATE_SIMPLE_TEST(SceneProbes, AuthoredProbeLightingAndForwardSurfaces) { xiiSceneProbeTest::Run(false); }

XII_CREATE_SIMPLE_TEST(SceneProbes, ExplicitProbeBypassOnRestrictedStorage) { xiiSceneProbeTest::Run(false, true); }
