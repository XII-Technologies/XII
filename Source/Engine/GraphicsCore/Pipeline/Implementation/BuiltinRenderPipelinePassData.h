/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Components/Fog/VolumetricCloudComponent.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Components/Render/DecalComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Lighting/GpuShadowRaster.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
#include <Shaders/Pipeline/Passes/Atmosphere/CloudShadowConstants.h>
#include <Shaders/Pipeline/Passes/Denoising/TemporalDenoiseConstants.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/ReSTIRGIConstants.h>
#include <Shaders/Pipeline/Passes/HiZPyramid/HiZBuildConstants.h>
#include <Shaders/Pipeline/Passes/HiZPyramid/HiZOcclusionConstants.h>
#include <Shaders/Pipeline/Passes/LightClustering/LightClusteringConstants.h>
#include <Shaders/Pipeline/Passes/MotionVectors/MotionVectorConstants.h>
#include <Shaders/Pipeline/Passes/ReSTIR/ReSTIRDIConstants.h>
#include <Shaders/Pipeline/Passes/Reflections/RTReflectionDenoiseConstants.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/DrawCommandBuildConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/FrustumCullingConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/InstanceUpdateConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/LODSelectionConstants.h>
#include <Shaders/Pipeline/Passes/Visibility/ShadowCasterCullingConstants.h>
#include <Shaders/Pipeline/Passes/Volumetrics/VolumetricMediumConstants.h>
#include <Shaders/Pipeline/Passes/Volumetrics/VolumetricTemporalConstants.h>
#include <Shaders/Pipeline/ReflectionProbeData.h>

#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>

struct xiiLightingDataUploadData
{
  xiiRenderGraphBufferHandle m_hCamera, m_hLight, m_hGlobal;

  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hLightingDataReady; ///< UAV out token written after persistent lighting buffers have been uploaded.
};


struct xiiFrustumCullInstanceBounds
{
  xiiVec4 m_CenterRadius;
  xiiVec4 m_Extents;
};

struct xiiFrustumCullData
{
  xiiRenderGraphBufferHandle m_hInstanceBounds;    ///< SRV in (one world-space bound per extracted mesh packet).
  xiiRenderGraphBufferHandle m_hVisibleCandidates; ///< UAV out ([0]=count, [1..]=visible packet indices).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiFrustumCullInstanceBounds, xiiAlignedAllocatorWrapper> m_InstanceBounds;
  xiiFrustumCullingConstants                                                m_Constants       = {};
  xiiUInt32                                                                 m_uiInstanceCount = 0U;
};

struct xiiLODSelectData
{
  xiiRenderGraphBufferHandle m_hVisibleCandidates; ///< SRV in ([0]=count, [1..]=visible packet indices).
  xiiRenderGraphBufferHandle m_hPreferredLOD;      ///< SRV in (extracted LOD and flags for every packet).
  xiiRenderGraphBufferHandle m_hInstanceLOD;       ///< UAV out (packed LOD, material bin, and flags).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiUInt32> m_PreferredLOD;
  xiiLODSelectionConstants   m_Constants       = {};
  xiiUInt32                  m_uiInstanceCount = 0U;
};

struct alignas(16) xiiInstanceUpdateSource
{
  xiiShaderTransform m_GlobalTransform;
  xiiVec4            m_CenterRadius;
  xiiVec4            m_Extents;
};

struct xiiInstanceUpdateData
{
  xiiRenderGraphBufferHandle m_hInstanceSource;    ///< SRV in (current extracted transform and bounds).
  xiiRenderGraphBufferHandle m_hInstanceMatrices;  ///< UAV out (world transforms indexed by packet index).
  xiiRenderGraphBufferHandle m_hInstanceBoundsOut; ///< UAV out (world bounds indexed by packet index).
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiInstanceUpdateSource, xiiAlignedAllocatorWrapper> m_InstanceSource;
  xiiInstanceUpdateConstants                                           m_Constants       = {};
  xiiUInt32                                                            m_uiInstanceCount = 0U;
};

struct xiiExtractedDrawCommand
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndexCountPerInstance = 0U;
  xiiUInt32 m_uiInstanceCount         = 0U;
  xiiUInt32 m_uiStartIndexLocation    = 0U;
  xiiInt32  m_iBaseVertexLocation     = 0;
  xiiUInt32 m_uiStartInstanceLocation = 0U;
};

struct xiiDrawBuildData
{
  xiiRenderGraphBufferHandle m_hSurvivors;
  xiiRenderGraphBufferHandle m_hInstanceLOD;
  xiiRenderGraphBufferHandle m_hSourceCommands;
  xiiRenderGraphBufferHandle m_hDrawCommands;
  xiiRenderGraphBufferHandle m_hDrawCounts;
  xiiRenderGraphBufferHandle m_hConstants;

  xiiDynamicArray<xiiExtractedDrawCommand> m_SourceCommands;
  xiiDrawCommandBuildConstants             m_Constants       = {};
  xiiUInt32                                m_uiInstanceCount = 0U;
  bool                                     m_bUploadSource   = false;
};

struct xiiShadowCasterBuildData
{
  xiiRenderGraphBufferHandle m_hSourceCommands;
  xiiRenderGraphBufferHandle m_hInstanceBounds;
  xiiRenderGraphBufferHandle m_hCascadeConstants;
  xiiRenderGraphBufferHandle m_hShadowCasterCommands;
  xiiRenderGraphBufferHandle m_hShadowCasterCounts;
  xiiRenderGraphBufferHandle m_hConstants;

  xiiShadowCasterCullingConstants m_Constants       = {};
  xiiUInt32                       m_uiInstanceCount = 0U;
};

struct xiiClusterBuildData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hLightingDataReady;  ///< SRV in dependency token that ensures persistent lighting buffers are uploaded.
  xiiRenderGraphBufferHandle m_hClusterConstants;   ///< SRV in (structured buffer of cluster build constants, including cluster counts and depth range, consumed by the Cluster Build pass).
  xiiRenderGraphBufferHandle m_hClusterDescriptors; ///< UAV out (structured buffer of cluster descriptors, one per cluster, consumed by main lighting pass).
};

struct xiiLightListClearData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterConstants;
  xiiRenderGraphBufferHandle m_hLightGridBuffer;
  xiiUInt32                  m_uiTotalClusters = 0U;
};

struct xiiLightListData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterConstants;      ///< Constant buffer in (cluster dimensions and active light count).
  xiiRenderGraphBufferHandle m_hClusterDescriptors;    ///< SRV in (world-space cluster AABBs used for exact conservative rejection).
  xiiRenderGraphBufferHandle m_hLightIndexBuffer;      ///< UAV out (20-bit light index; the sampling pass later fills the upper 12-bit CDF).
  xiiRenderGraphBufferHandle m_hLightGridBuffer;       ///< UAV out (structured buffer of uint, containing compact light lists per cluster, consumed by main lighting pass).
  xiiUInt32                  m_uiActiveLightCount = 0; ///< Number of active lights to process (from extraction). This is used to avoid processing the entire buffer when only a subset is populated.
};

struct xiiLightSamplingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hClusterConstants;
  xiiRenderGraphBufferHandle m_hLightGridBuffer;
  xiiRenderGraphBufferHandle m_hLightIndexBuffer;
  xiiRenderGraphBufferHandle m_hLightSamplingTotals;
  xiiUInt32                  m_uiTotalClusters = 0U;
};

struct xiiReflectionProbeSelectData
{
  xiiRenderGraphBufferHandle                                         m_hClusterDescriptors; ///< SRV in: world-space lighting-cluster AABBs.
  xiiRenderGraphBufferHandle                                         m_hProbeData;          ///< SRV in: active probe transforms, volumes, and texture slots.
  xiiRenderGraphBufferHandle                                         m_hProbeConstants;     ///< Constant buffer in: active probe and cluster counts.
  xiiRenderGraphBufferHandle                                         m_hProbeClusters;      ///< UAV out: primary and secondary probe indices per cluster.
  xiiDynamicArray<xiiGPUReflectionProbe, xiiAlignedAllocatorWrapper> m_Probes;
  xiiUInt32                                                          m_uiTotalClusterCount = 1U;
};

struct xiiFroxelAllocationData
{
  xiiRenderGraphBufferHandle  m_hFroxelMetadata;
  xiiRenderGraphTextureHandle m_hFroxelScattering;
  xiiRenderGraphBufferHandle  m_hVolumetricMedia;
  xiiRenderGraphBufferHandle  m_hVolumetricConstants;
  xiiGpuVolumetricMediumArray m_Media;
  xiiUInt32                   m_uiMediumBufferCapacity = 1U;
  xiiUInt32                   m_uiRenderWidth          = 1920U;
  xiiUInt32                   m_uiRenderHeight         = 1080U;
};

struct xiiShadowCascadeSetupData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle m_hCascadeMatrices;      ///< CPU-uploaded constant buffer of cascade view-projection matrices and split depths.
  xiiUInt32                  m_uiActiveCascades = 0U; ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes.
  xiiMat4                    m_CascadeViewProjection[4];
  xiiVec4                    m_vCascadeSplitDepths = xiiVec4::MakeZero();
  xiiVec4                    m_vCascadeWorldRadii  = xiiVec4::MakeZero();
};

struct xiiLocalShadowAtlasAllocationData
{
  xiiRenderGraphBufferHandle   m_hLocalShadowAtlasDescriptors;
  xiiLocalShadowAtlasDataArray m_ShadowData;
};

struct xiiLocalShadowAtlasClearData
{
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;
};

struct xiiDirectionalShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hCascadeMatrices;        ///< ConstantBuffer in (cascade view-projection matrices and split depths from this frame's Shadow Cascade Setup pass).
  xiiRenderGraphBufferHandle  m_hShadowCasterCommands;   ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per cascade-per-bin, from this frame's Shadow Caster Build pass).
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas; ///< SRV in (texture atlas for directional shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiActiveCascades = 3U;   ///< Number of active shadow cascades for the current frame, used to avoid processing unused cascades in the Shadow Passes and main lighting pass.
};

struct xiiSpotShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per spot light, from this frame's Shadow Caster Build pass).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;    ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiSpotLightCount = 0; ///< Number of active spot lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

struct xiiPointShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hShadowCasterCommands; ///< SRV in (structured buffer of DrawIndexedIndirectArguments, one per point light, from this frame's Shadow Caster Build pass).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;     ///< Same atlas for spot and point lights, with different tile allocations. UAV out (texture atlas for local shadow maps, written by Shadow Passes, read by main lighting pass).
  xiiUInt32                   m_uiPointLightCount = 0; ///< Number of active point lights for the current frame, used to avoid processing when zero and to drive atlas tile allocation in a full implementation.
};

struct xiiRayTracedShadowData
{
  xiiRenderGraphTextureHandle                 m_hRTRawShadowMask; ///< UAV out (texture containing raw ray-traced shadow masks, written by Ray-Traced Shadow Pass, read by Shadow Denoise Pass).
  xiiRenderGraphTextureHandle                 m_hSceneDepth;      ///< SRV in (depth texture from main render pass, used for ray-traced shadow ray generation and occlusion testing).
  xiiRenderGraphTextureHandle                 m_hGBufferNormal;   ///< SRV in (surface normal used to offset the shadow-ray origin).
  xiiRenderGraphBufferHandle                  m_hSceneDependency; ///< BuildASRead dependency on this frame's TLAS build.
  xiiRenderGraphBufferHandle                  m_hMaterialData;    ///< SRV in (canonical material records used by any-hit alpha testing).
  xiiRenderGraphBufferHandle                  m_hGeometryData;    ///< SRV in (geometry addressing records used by any-hit alpha testing).
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

struct xiiShadowDenoiseData
{
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  xiiRenderGraphTextureHandle m_hRTRawShadowMask;
  xiiRenderGraphTextureHandle m_hPreviousShadowMask;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hRTFinalShadowMask;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

struct xiiContactShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;    ///< SRV in (depth texture from main render pass, used for contact shadow ray generation and occlusion testing).
  xiiRenderGraphTextureHandle m_hContactShadow; ///< UAV out (texture containing contact shadow masks, written by this pass, read by main lighting pass).
};

struct xiiDepthPrepassData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthStencil out (full-resolution reversed-Z scene depth, written by this pass and consumed by later depth-dependent passes).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin, from this frame's Draw Build pass).
};

struct xiiHiZPyramidData
{
  xiiRenderGraphTextureHandle m_hSceneDepth;        ///< ShaderResource in (scene depth texture written by Depth Prepass, used as mip-0 source for Hi-Z generation).
  xiiRenderGraphTextureHandle m_hHiZPyramid;        ///< UnorderedAccess out (R32F reversed-Z minimum hierarchy consumed by conservative occlusion).
  xiiRenderGraphTextureHandle m_hHiZClosestPyramid; ///< UnorderedAccess out (R32F reversed-Z maximum hierarchy consumed by screen-space ray traversal).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiUInt32                   m_uiMipLevels = 1U; ///< Number of mips in the Hi-Z pyramid, derived from the current viewport size.
};

struct xiiHiZOcclusionCullData
{
  xiiRenderGraphTextureHandle m_hHiZPyramid;         ///< ShaderResource in (Hi-Z pyramid generated by this frame's Hi-Z Pyramid pass).
  xiiRenderGraphBufferHandle  m_hVisibleCandidates;  ///< ShaderResource in (visible instance candidate list generated by this frame's Frustum Culling pass).
  xiiRenderGraphBufferHandle  m_hSurvivingInstances; ///< UnorderedAccess out (instance list surviving Hi-Z occlusion culling, consumed by later depth/lighting passes).
  xiiRenderGraphBufferHandle  m_hInstanceBounds;     ///< ShaderResource in (instance bounds buffer for occlusion testing).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiHiZOcclusionConstants    m_Constants       = {};
  xiiUInt32                   m_uiInstanceCount = 0U; ///< Maximum number of candidates to process.
};

struct xiiMotionVectorsData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;     ///< ShaderResource in (current reversed-Z scene depth).
  xiiRenderGraphTextureHandle m_hVelocityBuffer; ///< RenderTarget out (current-to-previous NDC velocity).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiMotionVectorConstants    m_Constants             = {};
  xiiMat4                     m_CurrentViewProjection = xiiMat4::MakeIdentity();
  xiiVec2                     m_vCurrentJitter        = xiiVec2::MakeZero();
};

struct xiiVelocityDilationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hVelocityInput;   ///< ShaderResource in (screen-space velocity buffer generated by the Motion Vectors pass).
  xiiRenderGraphTextureHandle m_hSceneDepth;      ///< ShaderResource in (reversed-Z depth used to select the foreground surface).
  xiiRenderGraphTextureHandle m_hVelocityDilated; ///< UnorderedAccess out (depth-aware velocity consumed by temporal passes).
};

struct xiiGBufferBaseData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested G-Buffer rendering).
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;        ///< RenderTarget out (albedo and AO target).
  xiiRenderGraphTextureHandle m_hGBufferNormal;        ///< RenderTarget out (encoded normal target).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;      ///< RenderTarget out (material properties target).
  xiiRenderGraphTextureHandle m_hGBufferEmissive;      ///< RenderTarget out (emissive target).
  xiiRenderGraphTextureHandle m_hGBufferObjectId;      ///< RenderTarget out (stable object identity target).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

struct xiiNormalRoughnessPrepassData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;           ///< DepthRead in (scene depth generated in Stage 3, used for depth-tested rendering).
  xiiRenderGraphTextureHandle m_hNormalRoughness;      ///< RenderTarget out (compact normal/roughness/specular buffer consumed by GTAO and lighting prep passes).
  xiiRenderGraphBufferHandle  m_hDrawIndirectCommands; ///< IndirectArgument in (buffer of DrawIndexedIndirectArguments, one per draw bin).
};

struct xiiBRDFLutGenerationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hBRDFLut;                 ///< Imported persistent BRDF LUT texture.
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch BRDF LUT generation.
};

struct xiiAtmosphereTransmittanceData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< Imported persistent atmosphere transmittance LUT texture.
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters used by the integration.
  xiiAtmosphereConstants      m_Constants;
  xiiAtmosphereLUTHandle      m_hCache;
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch transmittance LUT generation.
};

struct xiiAtmosphereMultiScatterData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hMultiScatterLUT;  ///< Imported persistent atmosphere multi-scatter LUT texture.
  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters used by the integration.
  xiiAtmosphereConstants      m_Constants;
  xiiAtmosphereLUTHandle      m_hCache;
  bool                        m_bNeedsGeneration = false; ///< Whether this frame must dispatch multi-scatter LUT generation.
};

struct xiiSkyIrradianceConvolutionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hTransmittanceLUT; ///< ShaderResource in (atmosphere transmittance LUT).
  xiiRenderGraphTextureHandle m_hMultiScatterLUT;  ///< ShaderResource in (atmosphere multi-scatter LUT).
  xiiRenderGraphTextureHandle m_hSkyRadiance;      ///< UnorderedAccess out (sky radiance texture used by later lighting passes).
  xiiRenderGraphBufferHandle  m_hConstants;        ///< Physical atmosphere parameters.
  xiiAtmosphereConstants      m_Constants;
};

struct xiiVolumetricFogInitializationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle  m_hFroxelMetadata;   ///< ShaderResource in (froxel metadata buffer).
  xiiRenderGraphTextureHandle m_hFroxelScattering; ///< UnorderedAccess inout (froxel scattering texture).
};

struct xiiDDGIProbeSamplingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDDGIIrradiance; ///< UnorderedAccess out (DDGI irradiance result texture).
  xiiRenderGraphTextureHandle m_hSceneDepth;     ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hGBufferNormal;  ///< ShaderResource in (GBuffer normal texture).
  xiiRenderGraphTextureHandle m_hProbeIrradianceAtlas;
  xiiRenderGraphTextureHandle m_hProbeDistanceAtlas;
  xiiRenderGraphBufferHandle  m_hProbeStates;
  xiiRenderGraphBufferHandle  m_hProbeConstants;
};

struct xiiSparseVoxelRadianceGatherData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphBufferHandle  m_hRadiancePool;
  xiiRenderGraphBufferHandle  m_hPageTable;
  xiiRenderGraphBufferHandle  m_hLevelData;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiRenderGraphTextureHandle m_hIrradiance;
};

struct xiiGroundTruthAmbientOcclusionData
{
  xiiRenderGraphTextureHandle                 m_hSceneDepth;          ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                 m_hNormalRoughness;     ///< ShaderResource in (normal/roughness buffer).
  xiiRenderGraphTextureHandle                 m_hRawAmbientOcclusion; ///< UnorderedAccess out (raw ambient occlusion result).
  xiiRenderGraphBufferHandle                  m_hSceneDependency;
  xiiRenderGraphBufferHandle                  m_hMaterialData;
  xiiRenderGraphBufferHandle                  m_hGeometryData;
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

struct xiiGroundTruthAmbientOcclusionDenoiseData
{
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  xiiRenderGraphTextureHandle m_hRawAmbientOcclusion;      ///< ShaderResource in (raw ambient visibility).
  xiiRenderGraphTextureHandle m_hPreviousAmbientOcclusion; ///< ShaderResource in (previous temporally accumulated visibility).
  xiiRenderGraphTextureHandle m_hSceneDepth;               ///< ShaderResource in (current reversed-Z depth).
  xiiRenderGraphTextureHandle m_hGBufferNormal;            ///< ShaderResource in (current octahedral normal).
  xiiRenderGraphTextureHandle m_hVelocity;                 ///< ShaderResource in (current-to-previous NDC velocity).
  xiiRenderGraphTextureHandle m_hPreviousSurface;          ///< ShaderResource in (previous normal and linear depth).
  xiiRenderGraphTextureHandle m_hStableAmbientOcclusion;   ///< UnorderedAccess out (persistent denoised visibility).
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

struct xiiReSTIRDITemporalData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;
  xiiRenderGraphBufferHandle  m_hLightSamplingTotals;
  xiiRenderGraphBufferHandle  m_hEmissiveTriangles;
  xiiRenderGraphBufferHandle  m_hEmissiveLightHeader;
  xiiRenderGraphBufferHandle  m_hEmissiveTriangleAliases;
  xiiRenderGraphBufferHandle  m_hEmissiveBlockAliases;
  xiiRenderGraphBufferHandle  m_hEmissiveAliasHeader;
  xiiRenderGraphTextureHandle m_hPreviousReservoir;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiRenderGraphTextureHandle m_hTemporalReservoir;
  bool                        m_bHistoryValid = false;
};

struct xiiReSTIRDISpatialData
{
  xiiUInt32 m_uiHistoryWriteIndex = 0U;
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;
  xiiRenderGraphTextureHandle m_hTemporalReservoir;
  xiiRenderGraphBufferHandle  m_hEmissiveTriangles;
  xiiRenderGraphBufferHandle  m_hEmissiveLightHeader;
  xiiRenderGraphTextureHandle m_hReservoir;
  xiiRenderGraphTextureHandle m_hReservoirSurface;
};

struct xiiReSTIRDIVisibilityData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle  m_hEmissiveTriangles;
  xiiRenderGraphBufferHandle  m_hEmissiveLightHeader;
  xiiRenderGraphTextureHandle m_hReservoir;
};

struct xiiReSTIRDIEmissiveVisibilityData
{
  xiiRenderGraphTextureHandle                 m_hSceneDepth;
  xiiRenderGraphTextureHandle                 m_hGBufferNormal;
  xiiRenderGraphBufferHandle                  m_hEmissiveTriangles;
  xiiRenderGraphBufferHandle                  m_hEmissiveLightHeader;
  xiiRenderGraphTextureHandle                 m_hReservoir;
  xiiRenderGraphBufferHandle                  m_hSceneDependency;
  xiiRenderGraphBufferHandle                  m_hMaterialData;
  xiiRenderGraphBufferHandle                  m_hGeometryData;
  xiiRenderGraphBufferHandle                  m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>              m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracingPipeline;
  xiiUInt32                                   m_uiShaderRecordStride   = 0U;
  bool                                        m_bUseHardwareRayTracing = false;
};

struct xiiDeferredDirectLightingData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hGBufferAlbedo;            ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle m_hGBufferNormal;            ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle m_hGBufferMaterial;          ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle m_hGBufferEmissive;          ///< ShaderResource in (RGB emissive, A transmission factor).
  xiiRenderGraphTextureHandle m_hSceneDepth;               ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle m_hRayTracedFinalShadowMask; ///< ShaderResource in (denoised ray traced shadows).
  xiiRenderGraphTextureHandle m_hContactShadowTerm;        ///< ShaderResource in (contact shadow mask).
  xiiRenderGraphBufferHandle  m_hShadowCascadeConstants;   ///< ConstantBuffer in (directional cascade matrices, split depths, and active count).
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas;   ///< ShaderResource in (directional shadow atlas).
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;         ///< ShaderResource in (local light shadow atlas).
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;      ///< ShaderResource in (cluster light grid).
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;     ///< ShaderResource in (cluster light indices).
  xiiRenderGraphBufferHandle  m_hEmissiveTriangles;    ///< ShaderResource in (GPU-extracted world-space emissive triangles).
  xiiRenderGraphBufferHandle  m_hEmissiveLightHeader;  ///< ShaderResource in (bounded triangle count and overflow state).
  xiiRenderGraphTextureHandle m_hDirectLightReservoir; ///< ShaderResource in (spatially reused ReSTIR DI sample).
  xiiRenderGraphTextureHandle m_hReservoirSurface;     ///< ShaderResource in (history state transition and dependency).
  xiiRenderGraphBufferHandle  m_hCloudShadowConstants; ///< ConstantBuffer in (world-space cloud shadow projection).
  xiiRenderGraphBufferHandle  m_hVirtualShadowPageTable;
  xiiRenderGraphTextureHandle m_hVirtualShadowAtlas;
  xiiRenderGraphBufferHandle  m_hVirtualShadowSamplingConstants;
  xiiRenderGraphTextureHandle m_hDirectLightingBuffer; ///< UnorderedAccess out (direct lighting HDR buffer).
};

struct xiiShadowSamplingConstantsData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphBufferHandle        m_hCloudShadowConstants;
  xiiRenderGraphBufferHandle        m_hVirtualShadowSamplingConstants;
  xiiCloudShadowConstants           m_CloudShadowConstants;
  xiiVirtualShadowSamplingConstants m_VirtualShadowConstants;
};

struct xiiDeferredIndirectLightingData
{
  xiiRenderGraphTextureHandle                                            m_hGBufferAlbedo;           ///< ShaderResource in (G-Buffer albedo).
  xiiRenderGraphTextureHandle                                            m_hGBufferNormal;           ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                                            m_hGBufferMaterial;         ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle                                            m_hGBufferEmissive;         ///< ShaderResource in (RGB emissive, A transmission factor).
  xiiRenderGraphTextureHandle                                            m_hSceneDepth;              ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                                            m_hStableAmbientOcclusion;  ///< ShaderResource in (stable ambient occlusion).
  xiiRenderGraphTextureHandle                                            m_hBRDFLut;                 ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle                                            m_hDDGIIrradiance;          ///< ShaderResource in (DDGI irradiance texture).
  xiiRenderGraphTextureHandle                                            m_hSparseVoxelIrradiance;   ///< ShaderResource in (far-field sparse voxel irradiance).
  xiiRenderGraphTextureHandle                                            m_hSkyRadiance;             ///< ShaderResource in (sky radiance texture).
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeData;     ///< ShaderResource in (active local reflection probes).
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeClusters; ///< ShaderResource in (two selected probes per cluster).
  xiiHybridArray<xiiRenderGraphTextureHandle, XII_MAX_REFLECTION_PROBES> m_hReflectionProbeTextures;
  xiiRenderGraphTextureHandle                                            m_hFallbackReflectionProbe;
  xiiRenderGraphTextureHandle                                            m_hIndirectLightingBuffer; ///< UnorderedAccess out (indirect lighting HDR buffer).
  xiiRenderGraphTextureHandle                                            m_hEnvironmentSpecular;    ///< UnorderedAccess out (probe/sky specular contribution for tier replacement).
};

struct xiiRayTracedGlobalIlluminationData
{
  xiiRenderGraphTextureHandle                                            m_hSceneDepth;                     ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                                            m_hGBufferNormal;                  ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                                            m_hGBufferAlbedo;                  ///< ShaderResource in (surface albedo for diffuse bounce response).
  xiiRenderGraphTextureHandle                                            m_hAtmosphereTransmittanceLUT;     ///< ShaderResource in (physical atmosphere transmittance).
  xiiRenderGraphTextureHandle                                            m_hAtmosphereMultiScatterLUT;      ///< ShaderResource in (physical atmosphere multi-scattering).
  xiiRenderGraphTextureHandle                                            m_hFallbackReflectionProbe;        ///< ShaderResource fallback for unused probe descriptors.
  xiiHybridArray<xiiRenderGraphTextureHandle, XII_MAX_REFLECTION_PROBES> m_hReflectionProbeTextures;        ///< ShaderResource in (active local probe cubemaps).
  xiiRenderGraphTextureHandle                                            m_hRayTracedRawGlobalIllumination; ///< UnorderedAccess out (raw RT GI texture).
  xiiRenderGraphBufferHandle                                             m_hLightingDataReady;
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeData;
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeConstants;
  xiiRenderGraphBufferHandle                                             m_hAtmosphereConstants;
  xiiRenderGraphBufferHandle                                             m_hEmissiveTriangles;
  xiiRenderGraphBufferHandle                                             m_hEmissiveLightHeader;
  xiiRenderGraphBufferHandle                                             m_hEmissiveTriangleAliases;
  xiiRenderGraphBufferHandle                                             m_hEmissiveBlockAliases;
  xiiRenderGraphBufferHandle                                             m_hEmissiveAliasHeader;
  xiiRenderGraphBufferHandle                                             m_hSceneDependency;
  xiiRenderGraphBufferHandle                                             m_hMaterialData;
  xiiRenderGraphBufferHandle                                             m_hGeometryData;
  xiiRenderGraphBufferHandle                                             m_hShaderBindingTable;
  xiiSharedPtr<xiiGALTopLevelAS>                                         m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState>                            m_pRayTracingPipeline;
  xiiAtmosphereConstants                                                 m_AtmosphereConstants;
  xiiUInt32                                                              m_uiShaderRecordStride   = 0U;
  bool                                                                   m_bUseHardwareRayTracing = false;
};

struct xiiReSTIRGITemporalData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hCurrentRadiance;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSample;
  xiiRenderGraphTextureHandle m_hPreviousState;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hTemporalSample;
  xiiRenderGraphTextureHandle m_hTemporalState;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
};

struct xiiReSTIRGISpatialData
{
  xiiUInt32 m_uiHistoryWriteIndex = 0U;
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hTemporalSample;
  xiiRenderGraphTextureHandle m_hTemporalState;
  xiiRenderGraphTextureHandle m_hReservoirSample;
  xiiRenderGraphTextureHandle m_hReservoirState;
  xiiRenderGraphTextureHandle m_hReservoirSurface;
  xiiRenderGraphTextureHandle m_hResampledRadiance;
  xiiRenderGraphBufferHandle  m_hConstants;
};

struct xiiRayTracedGlobalIlluminationDenoiseData
{
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  xiiRenderGraphTextureHandle m_hRawGlobalIllumination;
  xiiRenderGraphTextureHandle m_hPreviousGlobalIllumination;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hFinalGlobalIllumination;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
  bool                        m_bSignalValid  = false;
};

struct xiiRayTracedReflectionsData
{
  xiiRenderGraphTextureHandle                                            m_hSceneDepth;                 ///< ShaderResource in (scene depth texture).
  xiiRenderGraphTextureHandle                                            m_hGBufferNormal;              ///< ShaderResource in (G-Buffer normal).
  xiiRenderGraphTextureHandle                                            m_hGBufferMaterial;            ///< ShaderResource in (G-Buffer material).
  xiiRenderGraphTextureHandle                                            m_hGBufferAlbedo;              ///< ShaderResource in (primary base color and metallic F0 source).
  xiiRenderGraphTextureHandle                                            m_hBRDFLut;                    ///< ShaderResource in (BRDF lookup texture).
  xiiRenderGraphTextureHandle                                            m_hAtmosphereTransmittanceLUT; ///< ShaderResource in (physical atmosphere transmittance).
  xiiRenderGraphTextureHandle                                            m_hAtmosphereMultiScatterLUT;  ///< ShaderResource in (physical atmosphere multi-scattering).
  xiiRenderGraphTextureHandle                                            m_hFallbackReflectionProbe;    ///< ShaderResource fallback for unused probe descriptors.
  xiiHybridArray<xiiRenderGraphTextureHandle, XII_MAX_REFLECTION_PROBES> m_hReflectionProbeTextures;    ///< ShaderResource in (active local probe cubemaps).
  xiiRenderGraphTextureHandle                                            m_hRayTracedRawReflections;    ///< UnorderedAccess out (raw RT reflections texture).
  xiiRenderGraphTextureHandle                                            m_hRayTracedHitDistance;       ///< UnorderedAccess out (primary hit distance; zero denotes an environment miss).
  xiiRenderGraphBufferHandle                                             m_hLightingDataReady;          ///< ShaderResource dependency on persistent light upload.
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeData;        ///< ShaderResource active local reflection probes.
  xiiRenderGraphBufferHandle                                             m_hReflectionProbeConstants;   ///< ConstantBuffer active reflection probe count.
  xiiRenderGraphBufferHandle                                             m_hAtmosphereConstants;        ///< ConstantBuffer physical atmosphere parameters.
  xiiRenderGraphBufferHandle                                             m_hEmissiveTriangles;          ///< ShaderResource GPU-extracted emissive triangles.
  xiiRenderGraphBufferHandle                                             m_hEmissiveLightHeader;        ///< ShaderResource emitted-count and overflow state.
  xiiRenderGraphBufferHandle                                             m_hEmissiveTriangleAliases;    ///< ShaderResource per-triangle alias entries.
  xiiRenderGraphBufferHandle                                             m_hEmissiveBlockAliases;       ///< ShaderResource power-weighted block aliases.
  xiiRenderGraphBufferHandle                                             m_hEmissiveAliasHeader;        ///< ShaderResource alias hierarchy metadata.
  xiiRenderGraphBufferHandle                                             m_hSceneDependency;            ///< BuildASRead dependency on this frame's TLAS build.
  xiiRenderGraphBufferHandle                                             m_hMaterialData;               ///< ShaderResource canonical material records indexed by TLAS instance.
  xiiRenderGraphBufferHandle                                             m_hGeometryData;               ///< ShaderResource geometry records indexed by TLAS instance.
  xiiRenderGraphBufferHandle                                             m_hShaderBindingTable;         ///< RayTracing in (shader group records).
  xiiSharedPtr<xiiGALTopLevelAS>                                         m_pTopLevelAS;
  xiiSharedPtr<xiiGALRayTracingPipelineState>                            m_pRayTracingPipeline;
  xiiAtmosphereConstants                                                 m_AtmosphereConstants;
  xiiUInt32                                                              m_uiShaderRecordStride   = 0U;
  bool                                                                   m_bUseHardwareRayTracing = false;
};

struct xiiRayTracedReflectionsDenoiseData
{
  xiiUInt32                   m_uiHistoryWriteIndex = 0U;
  xiiRenderGraphTextureHandle m_hRawReflections;
  xiiRenderGraphTextureHandle m_hRawHitDistance;
  xiiRenderGraphTextureHandle m_hPreviousReflections;
  xiiRenderGraphTextureHandle m_hPreviousHitDistance;
  xiiRenderGraphTextureHandle m_hPreviousMoments;
  xiiRenderGraphTextureHandle m_hPreviousIdentitySignature;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferObjectId;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hFinalReflections;
  xiiRenderGraphTextureHandle m_hCurrentHitDistance;
  xiiRenderGraphTextureHandle m_hCurrentMoments;
  xiiRenderGraphTextureHandle m_hCurrentIdentitySignature;
  xiiRenderGraphBufferHandle  m_hConstants;
  bool                        m_bHistoryValid = false;
  bool                        m_bSignalValid  = false;
};



struct xiiVolumetricLightInjectionData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelScattering; ///< UnorderedAccess inout (initialized scattering and injected light).
  xiiRenderGraphBufferHandle  m_hFroxelMetadata;
  xiiRenderGraphBufferHandle  m_hLightGridBuffer;
  xiiRenderGraphBufferHandle  m_hLightIndexBuffer;
  xiiRenderGraphBufferHandle  m_hShadowCascadeConstants;
  xiiRenderGraphTextureHandle m_hDirectionalShadowAtlas;
  xiiRenderGraphTextureHandle m_hLocalShadowAtlas;
  xiiRenderGraphBufferHandle  m_hLocalShadowAtlasDescriptors;
  xiiRenderGraphBufferHandle  m_hCloudShadowConstants;
};

struct xiiVolumetricFogIntegrationData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelScatteringBuffer; ///< ShaderResource in (pre-lit froxel scattering buffer).
  xiiRenderGraphTextureHandle m_hFroxelIntegratedBuffer; ///< UnorderedAccess out (front-to-back scattering prefix volume).
};

struct xiiVolumetricFogResolveData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hFroxelIntegratedBuffer;
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hVolumetricScattering;
};

struct xiiVolumetricFogTemporalReprojectionData
{
  xiiUInt32 m_uiHistoryWriteIndex = 0U;
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hCurrentScattering;  ///< ShaderResource in (current integrated scattering).
  xiiRenderGraphTextureHandle m_hPreviousScattering; ///< ShaderResource in (previous temporally filtered scattering).
  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hVelocity;
  xiiRenderGraphTextureHandle m_hPreviousSurface;
  xiiRenderGraphTextureHandle m_hVolumetricScattering; ///< UnorderedAccess out (current persistent history and final signal).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiMat4                     m_PreviousViewProjection = xiiMat4::MakeIdentity();
  bool                        m_bHistoryValid          = false;
};



struct alignas(16) xiiGPUDecalInstance
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_WorldToDecal      = xiiMat4::MakeIdentity();
  xiiVec4   m_AtlasUVRect       = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);
  xiiVec4   m_Tint              = xiiVec4(1.0f);
  xiiVec4   m_UVOffsetScale     = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);
  xiiVec4   m_ExtentsOpacity    = xiiVec4(1.0f, 1.0f, 0.25f, 1.0f);
  xiiVec4   m_WorldCenterRadius = xiiVec4::MakeZero();
  xiiVec4   m_SurfaceParams     = xiiVec4(1.0f, 0.5f, 0.0f, 0.0f);
  xiiUInt32 m_uiChannelMask     = 0U;
  xiiUInt32 m_uiMode            = 0U;
  xiiUInt32 m_uiPriority        = 0U;
  xiiUInt32 m_uiFlags           = 0U;
};

struct xiiDecalUploadData
{
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;

  xiiDynamicArray<xiiGPUDecalInstance, xiiAlignedAllocatorWrapper> m_Decals;
  xiiUInt32                                                        m_uiDecalCount = 0U;
};

struct xiiDecalCullBatchData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hVisibleList;
  xiiRenderGraphBufferHandle  m_hProjectedTileList;
  xiiRenderGraphBufferHandle  m_hMeshDrawCommands;

  xiiUInt32 m_uiDecalCount = 0U;
  xiiUInt32 m_uiTileCount  = 0U;
};

struct xiiProjectedDecalResolveData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hProjectedTileList;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;

  xiiUInt32 m_uiDecalCount = 0U;
};

struct xiiMeshDecalDrawData
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hSceneDepth;
  xiiRenderGraphBufferHandle  m_hDecalData;
  xiiRenderGraphBufferHandle  m_hMeshDrawCommands;
  xiiRenderGraphTextureHandle m_hAtlasAlbedo;
  xiiRenderGraphTextureHandle m_hAtlasNormal;
  xiiRenderGraphTextureHandle m_hAtlasMaterial;
  xiiRenderGraphTextureHandle m_hAtlasEmissive;
  xiiRenderGraphTextureHandle m_hGBufferAlbedo;
  xiiRenderGraphTextureHandle m_hGBufferNormal;
  xiiRenderGraphTextureHandle m_hGBufferMaterial;
  xiiRenderGraphTextureHandle m_hGBufferEmissive;

  xiiUInt32 m_uiDecalCount = 0U;
};
