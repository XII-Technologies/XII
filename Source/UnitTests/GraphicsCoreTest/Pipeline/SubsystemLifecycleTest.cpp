/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>

#include <type_traits>

static_assert(!std::is_default_constructible_v<xiiRenderGraphManager>, "Render graphs must be owned by the render graph subsystem.");
static_assert(!std::is_default_constructible_v<xiiMaterialManager>, "Material storage must be owned by the material subsystem.");
static_assert(!std::is_default_constructible_v<xiiGeometryResidencyManager>, "Geometry residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiGpuVisibilityManager>, "Visibility contexts must be owned by the visibility subsystem.");
static_assert(!std::is_default_constructible_v<xiiLightingManager>, "Lighting contexts must be owned by the lighting subsystem.");
static_assert(!std::is_default_constructible_v<xiiVirtualShadowMapManager>, "Virtual shadow residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiDDGIManager>, "DDGI state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiSparseVoxelRadianceManager>, "Sparse radiance state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiAtmosphereManager>, "Atmosphere LUT state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiRayTracingSceneManager>, "Ray tracing scene state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiSensorRenderingManager>, "Sensor profiles must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiDisplayOutputManager>, "Display defaults must be owned by their subsystem.");

XII_CREATE_SIMPLE_TEST_GROUP(Pipeline);

XII_CREATE_SIMPLE_TEST(Pipeline, SubsystemLifecycle)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Context handles are inert before allocation")
  {
    const xiiLightingContextHandle    lightingHandle;
    const xiiRenderGraphContextHandle renderGraphHandle;
    const xiiGpuVisibilityContextHandle visibilityHandle;

    XII_TEST_BOOL(!lightingHandle.IsValid());
    XII_TEST_BOOL(!renderGraphHandle.IsValid());
    XII_TEST_BOOL(!visibilityHandle.IsValid());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiLightingContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderGraphContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGpuVisibilityContextHandle>() != nullptr);
  }
}
