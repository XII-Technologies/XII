/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/SceneReflectionTestUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneReflections);

XII_CREATE_SIMPLE_TEST(SceneReflections, WorldSurfaceOutput) { xiiSceneReflectionTest::Run(true); }
XII_CREATE_SIMPLE_TEST(SceneReflections, UnsupportedStorageHasExplicitAuthoredFallback) { xiiSceneReflectionTest::Run(false); }
XII_CREATE_SIMPLE_TEST(SceneReflections, TemporalWorldSurfaceOutput) { xiiSceneReflectionTest::Run(true, true); }

XII_CREATE_SIMPLE_TEST(SceneReflections, ProbeFallbackReplacesScreenConfidence) { xiiSceneReflectionTest::Run(true, true, false, true); }
