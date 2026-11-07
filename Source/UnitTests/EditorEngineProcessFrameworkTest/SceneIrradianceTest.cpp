/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

XII_CREATE_SIMPLE_TEST(EditorViews, GeometryDependentDiffuseTransportAndIndependentWorlds) { xiiSceneIrradianceTest::Run(true); }

XII_CREATE_SIMPLE_TEST(EditorViews, OffscreenRayDiffuseTransportAndWorldRetirement) { xiiSceneIrradianceTest::Run(true, true); }
