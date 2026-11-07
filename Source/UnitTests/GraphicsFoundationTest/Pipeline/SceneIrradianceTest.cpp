/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/Pipeline/SceneIrradianceTestUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(SceneIrradiance);
XII_CREATE_SIMPLE_TEST(SceneIrradiance, GeometryTransportsEmissionToDiffuseReceivers) { xiiSceneIrradianceTest::Run(false); }
