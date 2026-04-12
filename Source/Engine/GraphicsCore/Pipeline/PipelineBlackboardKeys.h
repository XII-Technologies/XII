#pragma once

/// \brief Compile-time string keys for the per-view render graph blackboard.
///
/// All passes share this header to avoid typos and ensure consistent key names.
/// Keys with a (*) suffix have a corresponding typed helper written by the named pass.
namespace xiiRGBlackboardKeys
{
  //
  // Stage 0 - Dynamic Resolution (CPU, pre-graph)
  //
  constexpr const char* k_DynamicResolutionScale = "DynamicResolutionScale"; ///< float - [MinScale, 1.0] current PID-smoothed render scale.
  constexpr const char* k_RenderWidth            = "RenderWidth";            ///< uint32 - scaled render width in pixels (aligned to 2).
  constexpr const char* k_RenderHeight           = "RenderHeight";           ///< uint32 - scaled render height in pixels (aligned to 2).

  //
  // Stage 1 - Visibility & Setup
  //
  constexpr const char* k_FrameIndex                = "FrameIndex";               ///< uint32 - monotonically increasing frame counter.
  constexpr const char* k_ActiveLightCount          = "ActiveLightCount";         ///< uint32 - number of valid light entries in the light data buffer.
  constexpr const char* k_InstanceWorldMatrixBuffer = "InstanceWorldMatrices";    ///< xiiRGBufferHandle - per-instance world matrices (float4x3 structs).
  constexpr const char* k_InstanceBoundsBuffer      = "InstanceBounds";           ///< xiiRGBufferHandle - per-instance AABB (center + extents + radius).
  constexpr const char* k_InstanceLODBuffer         = "InstanceLOD";              ///< xiiRGBufferHandle - per-instance LOD level + meshlet metadata.
  constexpr const char* k_VisibleCandidateBuffer    = "VisibleCandidates";        ///< xiiRGBufferHandle - coarse-frustum-culled instance index list [0]=count.
  constexpr const char* k_SurvivingInstanceBuffer   = "SurvivingInstances";       ///< xiiRGBufferHandle - Hi-Z occlusion-culled instance index list [0]=count.
  constexpr const char* k_DrawIndirectCommands      = "DrawIndirectCommands";     ///< xiiRGBufferHandle - packed DrawIndexedIndirect args, one per material bin.
  constexpr const char* k_DrawCountBuffer           = "DrawCounts";               ///< xiiRGBufferHandle - per-material-bin indirect draw count.
  constexpr const char* k_DrawShadowCasterCommands  = "DrawShadowCasterCommands"; ///< xiiRGBufferHandle - packed indirect args for shadow depth renders.
  constexpr const char* k_ReflectionProbeMask       = "ReflectionProbeMask";      ///< xiiRGBufferHandle - per-probe visibility bits (bitfield of active probes).
  constexpr const char* k_SkinnedVertexBuffer       = "SkinnedVertexBuffer";      ///< xiiRGBufferHandle - deformed vertex streams (written by skinning pass).
  constexpr const char* k_ParticleVertexBuffer      = "ParticleVertexBuffer";     ///< xiiRGBufferHandle - particle vertex data (written by GPU particle sim).
  constexpr const char* k_ParticleIndexBuffer       = "ParticleIndexBuffer";      ///< xiiRGBufferHandle - particle index data (written by GPU particle sim).

  //
  // Stage 1 - Per-Frame Constant Buffers (uploaded once, read by all passes)
  //
  constexpr const char* k_PerFrameCameraBuffer = "PerFrameCameraBuffer"; ///< xiiGALBuffer* - camera constants structured buffer.
  constexpr const char* k_PerFrameLightBuffer  = "PerFrameLightBuffer";  ///< xiiGALBuffer* - global light constants buffer.
  constexpr const char* k_PerFrameGlobalBuffer = "PerFrameGlobalBuffer"; ///< xiiGALBuffer* - global frame constants buffer.

  //
  // Stage 1 - Clustering
  //
  constexpr const char* k_ClusterDescriptors = "ClusterDescriptors"; ///< xiiRGBufferHandle - frustum cluster AABB descriptors (float4 per cluster).
  constexpr const char* k_LightIndexBuffer   = "LightIndexBuffer";   ///< xiiRGBufferHandle - per-cluster compact light index list.
  constexpr const char* k_LightGridBuffer    = "LightGrid";          ///< xiiRGBufferHandle - cluster → (offset, count) pairs (uint2 per cluster).

  //
  // Stage 1 - Froxel / Volumetric
  //
  constexpr const char* k_FroxelMetadataBuffer   = "FroxelMetadata";   ///< xiiRGBufferHandle - froxel bounds + phase + density terms.
  constexpr const char* k_FroxelScatteringBuffer = "FroxelScattering"; ///< xiiRGTextureHandle - 3D froxel scattering/extinction (R16G16B16A16F vol texture).
  constexpr const char* k_FroxelDepthRange       = "FroxelDepthRange"; ///< xiiRGBufferHandle - (near, far, sliceCount, pad) packed into float4.

  //
  // Stage 2 - Shadows
  //
  constexpr const char* k_ShadowCascadeMatrices  = "ShadowCascadeMatrices"; ///< xiiRGBufferHandle - cascade view-proj matrices (float4x4[4]).
  constexpr const char* k_ShadowCascadeCount     = "ShadowCascadeCount";    ///< uint32 - active cascade count (0–4).
  constexpr const char* k_DirectionalShadowAtlas = "DirShadowAtlas";        ///< xiiRGTextureHandle - cascaded shadow map texture (D32F array).
  constexpr const char* k_LocalShadowAtlas       = "LocalShadowAtlas";      ///< xiiRGTextureHandle - spot/point shadow atlas (D32F).
  constexpr const char* k_LocalShadowAtlasDescs  = "LocalShadowAtlasDescs"; ///< xiiRGBufferHandle - per-light atlas placement data.
  constexpr const char* k_RTRawShadowMask        = "RTRawShadows";          ///< xiiRGTextureHandle - raw RT shadow mask per light (R8_UNORM).
  constexpr const char* k_RTFinalShadowMask      = "RTFinalShadows";        ///< xiiRGTextureHandle - denoised RT shadow mask.
  constexpr const char* k_ContactShadowTerm      = "ContactShadows";        ///< xiiRGTextureHandle - screen-space contact shadow mask (R8_UNORM).

  //
  // Stage 3 - Depth & Motion
  //
  constexpr const char* k_OccluderDepthTexture  = "OccluderDepth";   ///< xiiRGTextureHandle - occluder-only depth prepass output (D32F).
  constexpr const char* k_SceneDepthTexture     = "SceneDepth";      ///< xiiRGTextureHandle - full-resolution scene depth buffer (D32F reversed-Z).
  constexpr const char* k_HiZPyramid            = "HiZPyramid";      ///< xiiRGTextureHandle - R32F max-depth pyramid covering all mip levels.
  constexpr const char* k_VelocityBuffer        = "VelocityBuffer";  ///< xiiRGTextureHandle - screen-space velocity (R16G16F).
  constexpr const char* k_NormalRoughnessBuffer = "NormalRoughness"; ///< xiiRGTextureHandle - compact R8G8B8A8 oct-encoded normal + roughness.

  //
  // Stage 4 - G-Buffer
  //
  constexpr const char* k_GBufferAlbedo   = "GBufferAlbedo";   ///< xiiRGTextureHandle - R8G8B8A8_UNORM albedo (rgb) + AO (a).
  constexpr const char* k_GBufferNormal   = "GBufferNormal";   ///< xiiRGTextureHandle - R16G16_SNORM oct-encoded world-space normals.
  constexpr const char* k_GBufferMaterial = "GBufferMaterial"; ///< xiiRGTextureHandle - R8G8B8A8: r=roughness, g=metallic, b=specular, a=matID.
  constexpr const char* k_GBufferEmissive = "GBufferEmissive"; ///< xiiRGTextureHandle - R16G16B16A16F emissive + special-purpose channel.

  //
  // Stage 5 - Lighting Preparation
  //
  constexpr const char* k_BRDFLut                    = "BRDFLut";          ///< xiiRGTextureHandle - 256×256 R16G16F GGX split-sum BRDF LUT (persistent).
  constexpr const char* k_DDGIIrradiance             = "DDGIIrradiance";   ///< xiiRGTextureHandle - DDGI probe irradiance atlas (if DDGI enabled).
  constexpr const char* k_AtmosphereTransmittanceLUT = "AtmTransmittance"; ///< xiiRGTextureHandle - 256×64 R16G16B16A16F atmosphere transmittance LUT.
  constexpr const char* k_AtmosphereMultiScatterLUT  = "AtmMultiScatter";  ///< xiiRGTextureHandle - 32×32 R16G16B16A16F multiple-scattering LUT.
  constexpr const char* k_RawAOTexture               = "RawAO";            ///< xiiRGTextureHandle - raw GTAO / HBAO+ term (R8_UNORM).
  constexpr const char* k_StableAOTexture            = "StableAO";         ///< xiiRGTextureHandle - temporally-denoised AO (R8_UNORM).

  //
  // Stage 6 - Main Lighting
  //
  constexpr const char* k_DirectLightingBuffer   = "DirectLighting";       ///< xiiRGTextureHandle - direct lighting HDR (R16G16B16A16F).
  constexpr const char* k_IndirectLightingBuffer = "IndirectLighting";     ///< xiiRGTextureHandle - indirect lighting HDR (R16G16B16A16F).
  constexpr const char* k_SSRTexture             = "SSRTerm";              ///< xiiRGTextureHandle - screen-space reflection radiance (R16G16B16A16F).
  constexpr const char* k_RTRawGI                = "RTRawGI";              ///< xiiRGTextureHandle - raw RT indirect diffuse before denoising.
  constexpr const char* k_RTFinalGI              = "RTFinalGI";            ///< xiiRGTextureHandle - denoised RT GI.
  constexpr const char* k_RTRawReflections       = "RTRawReflections";     ///< xiiRGTextureHandle - raw RT reflection radiance.
  constexpr const char* k_RTFinalReflections     = "RTFinalReflections";   ///< xiiRGTextureHandle - denoised RT reflections.
  constexpr const char* k_VolumetricScattering   = "VolumetricScattering"; ///< xiiRGTextureHandle - integrated volumetric light contribution.
  constexpr const char* k_SkyRadiance            = "SkyRadiance";          ///< xiiRGTextureHandle - sky + atmosphere contribution.

  //
  // Stage 7 - Forward / Composite
  //
  constexpr const char* k_HDRSceneColor = "HDRSceneColor"; ///< xiiRGTextureHandle - combined HDR scene color after opaque (R16G16B16A16F).

  //
  // Stage 8 - Transparency
  //
  constexpr const char* k_OITAccumulateBuffer = "OITAccumulate"; ///< xiiRGTextureHandle - WBOIT weighted accumulation target (R16G16B16A16F).
  constexpr const char* k_OITRevealBuffer     = "OITReveal";     ///< xiiRGTextureHandle - WBOIT reveal (transmittance) target (R8_UNORM).
  constexpr const char* k_DecalTileList       = "DecalTileList"; ///< xiiRGBufferHandle - per-tile decal index list.

  //
  // Stage 9 - Screen-Space Effects
  //
  constexpr const char* k_PlanarReflectionMap = "PlanarReflectionMap"; ///< xiiRGTextureHandle - planar reflection render target.

  //
  // Stage 10 - Temporal Reconstruction
  //
  constexpr const char* k_LuminanceHistogram = "LuminanceHistogram"; ///< xiiRGBufferHandle - 256-bin log-luminance histogram.
  constexpr const char* k_CurrentExposure    = "CurrentExposure";    ///< xiiRGBufferHandle - single float EV100 exposure value.
  constexpr const char* k_TAAResolvedColor   = "TAAResolved";        ///< xiiRGTextureHandle - temporally-resolved color (R16G16B16A16F).
  constexpr const char* k_UpscaledColor      = "UpscaledColor";      ///< xiiRGTextureHandle - upscaled output at native resolution.

  //
  // Stage 11 - Post-Processing
  //
  constexpr const char* k_BloomTexture   = "BloomTexture";   ///< xiiRGTextureHandle - bloom-composited HDR result.
  constexpr const char* k_GradedColor    = "GradedColor";    ///< xiiRGTextureHandle - color-graded + filmic output.
  constexpr const char* k_LDRSceneColor  = "LDRSceneColor";  ///< xiiRGTextureHandle - tone-mapped LDR output.
  constexpr const char* k_SharpenedColor = "SharpenedColor"; ///< xiiRGTextureHandle - final sharpened LDR output.

} // namespace xiiRGBlackboardKeys
