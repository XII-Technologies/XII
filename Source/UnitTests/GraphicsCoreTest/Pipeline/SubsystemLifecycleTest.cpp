/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/GpuShadowRaster.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Particles/ParticleSystemManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>

#include <type_traits>

static_assert(!std::is_default_constructible_v<xiiRenderGraphManager>, "Render graphs must be owned by the render graph subsystem.");
static_assert(!std::is_default_constructible_v<xiiMaterialManager>, "Material storage must be owned by the material subsystem.");
static_assert(!std::is_default_constructible_v<xiiGeometryResidencyManager>, "Geometry residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiGpuVisibilityManager>, "Visibility contexts must be owned by the visibility subsystem.");
static_assert(!std::is_default_constructible_v<xiiLightingManager>, "Lighting contexts must be owned by the lighting subsystem.");
static_assert(!std::is_default_constructible_v<xiiGpuShadowRasterManager>, "GPU shadow raster state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiVirtualShadowMapManager>, "Virtual shadow residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiDDGIManager>, "DDGI state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiSparseVoxelRadianceManager>, "Sparse radiance state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiAtmosphereManager>, "Atmosphere LUT state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiRayTracingSceneManager>, "Ray tracing scene state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiSensorRenderingManager>, "Sensor profiles must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiDisplayOutputManager>, "Display defaults must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiVolumetricMediumManager>, "Volumetric media must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiSceneDatabaseManager>, "Render scenes must be owned by the scene database subsystem.");
static_assert(!std::is_default_constructible_v<xiiSceneDatabase>, "Scene database storage must be created through the subsystem.");
static_assert(!std::is_default_constructible_v<xiiParticleSystemManager>, "Particle runtimes must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiParticleSystemRuntime>, "Particle runtime storage must be created through the subsystem.");
static_assert(!std::is_default_constructible_v<xiiGALPipelineCache>, "Pipeline cache storage must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiGALRenderPassCache>, "Render-pass cache storage must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiViewRenderResourceManager>, "Persistent view resources must be owned by their subsystem.");

XII_CREATE_SIMPLE_TEST_GROUP(Pipeline);

XII_CREATE_SIMPLE_TEST(Pipeline, SubsystemLifecycle)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Graphics services are subsystem owned")
  {
    XII_TEST_BOOL(xiiGeometryResidencyManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGpuVisibilityManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiLightingManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGpuShadowRasterManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiVirtualShadowMapManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiDDGIManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiSparseVoxelRadianceManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiAtmosphereManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiRayTracingSceneManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiSensorRenderingManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiDisplayOutputManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiMaterialManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiParticleSystemManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGALPipelineCache::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGALRenderPassCache::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiRenderGraphManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiViewRenderResourceManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiSceneDatabaseManager::IsSubsystemInitialized());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Context handles are inert before allocation")
  {
    const xiiLightingContextHandle       lightingHandle;
    const xiiRenderGraphContextHandle    renderGraphHandle;
    const xiiGpuVisibilityContextHandle  visibilityHandle;
    const xiiSceneDatabaseContextHandle  sceneHandle;
    const xiiParticleSystemRuntimeHandle particleHandle;
    const xiiViewRenderResourceContextHandle viewResourceHandle;

    XII_TEST_BOOL(!lightingHandle.IsValid());
    XII_TEST_BOOL(!renderGraphHandle.IsValid());
    XII_TEST_BOOL(!visibilityHandle.IsValid());
    XII_TEST_BOOL(!sceneHandle.IsValid());
    XII_TEST_BOOL(!particleHandle.IsValid());
    XII_TEST_BOOL(!viewResourceHandle.IsValid());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiLightingContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderGraphContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGpuVisibilityContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiSceneDatabaseContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiParticleSystemRuntimeHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiViewRenderResourceContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGpuShadowRasterDescription>() != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scene database contexts are generation checked")
  {
    XII_TEST_BOOL(xiiSceneDatabaseManager::IsInitialized());
    const xiiSceneDatabaseContextHandle handle = xiiSceneDatabaseManager::CreateContext(16U);
    XII_TEST_BOOL(handle.IsValid());
    XII_TEST_BOOL(xiiSceneDatabaseManager::IsValid(handle));
    XII_TEST_BOOL(xiiSceneDatabaseManager::GetDatabase(handle) != nullptr);

    xiiSceneDatabaseManager::DestroyContext(handle);
    XII_TEST_BOOL(!xiiSceneDatabaseManager::IsValid(handle));
  }
}
