/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Material/MaterialSystem.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

#include <type_traits>

static_assert(!std::is_default_constructible_v<xiiLightingSystem>, "Lighting contexts must be created through xiiLightingManager.");
static_assert(!std::is_default_constructible_v<xiiMaterialSystem>, "Material storage must be created through xiiMaterialManager.");
static_assert(!std::is_default_constructible_v<xiiGpuVisibilitySystem>, "Visibility contexts must be created through xiiGpuVisibilityManager.");

XII_CREATE_SIMPLE_TEST_GROUP(Pipeline);

XII_CREATE_SIMPLE_TEST(Pipeline, SubsystemLifecycle)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Context handles are inert before allocation")
  {
    const xiiLightingContextHandle    lightingHandle;
    const xiiRenderGraphContextHandle renderGraphHandle;

    XII_TEST_BOOL(!lightingHandle.IsValid());
    XII_TEST_BOOL(!renderGraphHandle.IsValid());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiLightingContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderGraphContextHandle>() != nullptr);
  }
}
