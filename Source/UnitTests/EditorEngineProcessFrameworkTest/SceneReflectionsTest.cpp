/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <GraphicsFoundationTest/Pipeline/SceneReflectionTestUtils.h>

XII_CREATE_SIMPLE_TEST(EditorViews, TemporalScreenReflections) { xiiSceneReflectionTest::Run(true, true, true); }

XII_CREATE_SIMPLE_TEST(EditorViews, ProbeFallbackReplacesScreenConfidence) { xiiSceneReflectionTest::Run(true, true, true, true); }
