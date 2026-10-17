/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Particles/ParticleSystemManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>

#include <type_traits>

static_assert(!std::is_default_constructible_v<xiiRenderGraphManager>, "Render graphs must be owned by the render graph subsystem.");
static_assert(!std::is_default_constructible_v<xiiMaterialManager>, "Material storage must be owned by the material subsystem.");
static_assert(!std::is_default_constructible_v<xiiGeometryResidencyManager>, "Geometry residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiLightingManager>, "Lighting contexts must be owned by the lighting subsystem.");
static_assert(!std::is_default_constructible_v<xiiVirtualShadowMapManager>, "Virtual shadow residency must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiAtmosphereManager>, "Atmosphere LUT state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiRayTracingResources>, "Ray tracing scene state must be owned by its subsystem.");
static_assert(!std::is_default_constructible_v<xiiSensorRenderingManager>, "Sensor profiles must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiDisplayOutputManager>, "Display defaults must be owned by their subsystem.");
static_assert(!std::is_default_constructible_v<xiiVolumetricMediumManager>, "Volumetric media must be owned by their subsystem.");
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
    XII_TEST_BOOL(xiiLightingManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiVirtualShadowMapManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiAtmosphereManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiRayTracingResources::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiSensorRenderingManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiDisplayOutputManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiVolumetricMediumManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiMaterialManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiParticleSystemManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGALPipelineCache::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiGALRenderPassCache::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiRenderGraphManager::IsSubsystemInitialized());
    XII_TEST_BOOL(xiiViewRenderResourceManager::IsSubsystemInitialized());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Context handles are inert before allocation")
  {
    const xiiLightingContextHandle           lightingHandle;
    const xiiRenderGraphContextHandle        renderGraphHandle;
    const xiiParticleSystemRuntimeHandle     particleHandle;
    const xiiViewRenderResourceContextHandle viewResourceHandle;

    XII_TEST_BOOL(lightingHandle.IsInvalidated());
    XII_TEST_BOOL(renderGraphHandle.IsInvalidated());
    XII_TEST_BOOL(particleHandle.IsInvalidated());
    XII_TEST_BOOL(viewResourceHandle.IsInvalidated());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiLightingContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderGraphContextHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiParticleSystemRuntimeHandle>() != nullptr);
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiViewRenderResourceContextHandle>() != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Render graph passes publish explicit resource handles")
  {
    struct PublishData
    {
      xiiRenderGraphTextureHandle m_hTexture;
      xiiRenderGraphBufferHandle  m_hBuffer;
    };

    xiiRenderGraph graph("Subsystem Resource Publication");
    graph.BeginSetup(1U);
    auto publishPass = graph.AddPass<PublishData>("Publish Resources", xiiGALCommandQueueFlags::Transfer, [](PublishData& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription textureDescription;
        textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
        textureDescription.m_Size.width  = 16U;
        textureDescription.m_Size.height = 16U;
        textureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
        textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
        data.m_hTexture                  = builder.WriteTexture("Published Texture", textureDescription, xiiGALResourceStateFlags::CopyDestination);

        xiiGALBufferCreationDescription bufferDescription;
        bufferDescription.m_uiSize    = 64U;
        bufferDescription.m_BindFlags = xiiGALBindFlags::ShaderResource;
        data.m_hBuffer                = builder.WriteBuffer("Published Buffer", bufferDescription, xiiGALResourceStateFlags::CopyDestination); }, [](const PublishData&, xiiRenderGraphPassContext&) {});

    const xiiRenderGraphTextureHandle hTexture = publishPass.first->m_hTexture;
    const xiiRenderGraphBufferHandle  hBuffer  = publishPass.first->m_hBuffer;
    XII_TEST_BOOL(hTexture.IsValid());
    XII_TEST_BOOL(hBuffer.IsValid());
    graph.EndSetup();
  }
}
