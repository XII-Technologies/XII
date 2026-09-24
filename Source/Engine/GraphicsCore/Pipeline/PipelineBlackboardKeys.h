/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Compile-time string keys for the per-view render graph blackboard.
///
/// All passes share this header to avoid typos and ensure consistent key names.
namespace xiiRGBlackboardKeys
{
  // Stage 1 - Visibility & Setup.

  constexpr xiiStringView k_FrameIndex                = "FrameIndex"_xiisv;               ///< uint32 - monotonically increasing frame counter.
  constexpr xiiStringView k_ActiveLightCount          = "ActiveLightCount"_xiisv;         ///< uint32 - number of valid light entries in the light data buffer.
  constexpr xiiStringView k_InstanceWorldMatrixBuffer = "InstanceWorldMatrices"_xiisv;    ///< xiiRenderGraphBufferHandle - per-instance world matrices (float4x3 structs).
  constexpr xiiStringView k_InstanceBoundsBuffer      = "InstanceBounds"_xiisv;           ///< xiiRenderGraphBufferHandle - per-instance AABB (center + extents + radius).
  constexpr xiiStringView k_InstanceLODBuffer         = "InstanceLOD"_xiisv;              ///< xiiRenderGraphBufferHandle - per-instance LOD level + meshlet metadata.
  constexpr xiiStringView k_VisibleCandidateBuffer    = "VisibleCandidates"_xiisv;        ///< xiiRenderGraphBufferHandle - coarse-frustum-culled instance index list [0]=count.
  constexpr xiiStringView k_SurvivingInstanceBuffer   = "SurvivingInstances"_xiisv;       ///< xiiRenderGraphBufferHandle - Hi-Z occlusion-culled instance index list [0]=count.
  constexpr xiiStringView k_DrawIndirectCommands      = "DrawIndirectCommands"_xiisv;     ///< xiiRenderGraphBufferHandle - packed DrawIndexedIndirect args, one per material bin.
  constexpr xiiStringView k_DrawCountBuffer           = "DrawCounts"_xiisv;               ///< xiiRenderGraphBufferHandle - per-material-bin indirect draw count.
  constexpr xiiStringView k_DrawShadowCasterCommands  = "DrawShadowCasterCommands"_xiisv; ///< xiiRenderGraphBufferHandle - packed indirect args for shadow depth renders.
  constexpr xiiStringView k_ReflectionProbeMask       = "ReflectionProbeMask"_xiisv;      ///< xiiRenderGraphBufferHandle - per-probe visibility bits (bitfield of active probes).
  constexpr xiiStringView k_SkinnedVertexBuffer       = "SkinnedVertexBuffer"_xiisv;      ///< xiiRenderGraphBufferHandle - deformed vertex streams (written by skinning pass).
  constexpr xiiStringView k_ParticleVertexBuffer      = "ParticleVertexBuffer"_xiisv;     ///< xiiRenderGraphBufferHandle - particle vertex data (written by GPU particle sim).
  constexpr xiiStringView k_ParticleIndexBuffer       = "ParticleIndexBuffer"_xiisv;      ///< xiiRenderGraphBufferHandle - particle index data (written by GPU particle sim).

  // Stage 1 - Per-Frame Constant Buffers (uploaded once, read by all passes).

  constexpr xiiStringView k_PerFrameCameraBuffer = "PerFrameCameraBuffer"_xiisv; ///< xiiGALBuffer* - camera constants structured buffer.
  constexpr xiiStringView k_PerFrameLightBuffer  = "PerFrameLightBuffer"_xiisv;  ///< xiiGALBuffer* - global light constants buffer.
  constexpr xiiStringView k_PerFrameGlobalBuffer = "PerFrameGlobalBuffer"_xiisv; ///< xiiGALBuffer* - global frame constants buffer.
  constexpr xiiStringView k_LightingDataReady    = "LightingDataReady"_xiisv;    ///< xiiRenderGraphBufferHandle - tiny token written after per-frame light buffers are uploaded.

  // Stage 1 - Clustering.

  constexpr xiiStringView k_ClusterDescriptors = "ClusterDescriptors"_xiisv; ///< xiiRenderGraphBufferHandle - frustum cluster AABB descriptors (float4 per cluster).
  constexpr xiiStringView k_LightIndexBuffer   = "LightIndexBuffer"_xiisv;   ///< xiiRenderGraphBufferHandle - per-cluster compact light index list.
  constexpr xiiStringView k_LightGridBuffer    = "LightGrid"_xiisv;          ///< xiiRenderGraphBufferHandle - cluster -> (offset, count) pairs (uint2 per cluster).

  // Stage 1 - Froxel / Volumetric

  constexpr xiiStringView k_FroxelMetadataBuffer   = "FroxelMetadata"_xiisv;   ///< xiiRenderGraphBufferHandle - froxel bounds + phase + density terms.
  constexpr xiiStringView k_FroxelScatteringBuffer = "FroxelScattering"_xiisv; ///< xiiRenderGraphTextureHandle - 3D froxel scattering/extinction (R16G16B16A16F vol texture).
  constexpr xiiStringView k_FroxelDepthRange       = "FroxelDepthRange"_xiisv; ///< xiiRenderGraphBufferHandle - (near, far, sliceCount, pad) packed into float4.

  // Stage 2 - Shadows.

  constexpr xiiStringView k_ShadowCascadeMatrices  = "ShadowCascadeMatrices"_xiisv; ///< xiiRenderGraphBufferHandle - cascade view-proj matrices (float4x4[4]).
  constexpr xiiStringView k_ShadowCascadeCount     = "ShadowCascadeCount"_xiisv;    ///< uint32 - active cascade count (0–4).
  constexpr xiiStringView k_DirectionalShadowAtlas = "DirShadowAtlas"_xiisv;        ///< xiiRenderGraphTextureHandle - cascaded shadow map texture (D32F array).
  constexpr xiiStringView k_LocalShadowAtlas       = "LocalShadowAtlas"_xiisv;      ///< xiiRenderGraphTextureHandle - spot/point shadow atlas (D32F).
  constexpr xiiStringView k_LocalShadowAtlasDescs  = "LocalShadowAtlasDescs"_xiisv; ///< xiiRenderGraphBufferHandle - per-light atlas placement data.
  constexpr xiiStringView k_RTRawShadowMask        = "RTRawShadows"_xiisv;          ///< xiiRenderGraphTextureHandle - raw RT shadow mask per light (R8_UNORM).
  constexpr xiiStringView k_RTFinalShadowMask      = "RTFinalShadows"_xiisv;        ///< xiiRenderGraphTextureHandle - denoised RT shadow mask.
  constexpr xiiStringView k_ContactShadowTerm      = "ContactShadows"_xiisv;        ///< xiiRenderGraphTextureHandle - screen-space contact shadow mask (R8_UNORM).

  // Stage 3 - Depth & Motion.

  constexpr xiiStringView k_OccluderDepthTexture  = "OccluderDepth"_xiisv;   ///< xiiRenderGraphTextureHandle - occluder-only depth prepass output (D32F).
  constexpr xiiStringView k_SceneDepthTexture     = "SceneDepth"_xiisv;      ///< xiiRenderGraphTextureHandle - full-resolution scene depth buffer (D32F reversed-Z).
  constexpr xiiStringView k_HiZPyramid            = "HiZPyramid"_xiisv;      ///< xiiRenderGraphTextureHandle - R32F max-depth pyramid covering all mip levels.
  constexpr xiiStringView k_VelocityBuffer        = "VelocityBuffer"_xiisv;  ///< xiiRenderGraphTextureHandle - screen-space velocity (R16G16F).
  constexpr xiiStringView k_NormalRoughnessBuffer = "NormalRoughness"_xiisv; ///< xiiRenderGraphTextureHandle - compact R8G8B8A8 oct-encoded normal + roughness.

  // Stage 4 - G-Buffer.

  constexpr xiiStringView k_GBufferAlbedo   = "GBufferAlbedo"_xiisv;   ///< xiiRenderGraphTextureHandle - R8G8B8A8_UNORM albedo (rgb) + AO (a).
  constexpr xiiStringView k_GBufferNormal   = "GBufferNormal"_xiisv;   ///< xiiRenderGraphTextureHandle - R16G16_SNORM oct-encoded world-space normals.
  constexpr xiiStringView k_GBufferMaterial = "GBufferMaterial"_xiisv; ///< xiiRenderGraphTextureHandle - R8G8B8A8: r=roughness, g=metallic, b=specular, a=matID.
  constexpr xiiStringView k_GBufferEmissive = "GBufferEmissive"_xiisv; ///< xiiRenderGraphTextureHandle - R16G16B16A16F emissive + special-purpose channel.

  // Stage 5 - Lighting Preparation.

  constexpr xiiStringView k_BRDFLut                    = "BRDFLut"_xiisv;          ///< xiiRenderGraphTextureHandle - 256x256 R16G16F GGX split-sum BRDF LUT (persistent).
  constexpr xiiStringView k_DDGIIrradiance             = "DDGIIrradiance"_xiisv;   ///< xiiRenderGraphTextureHandle - DDGI probe irradiance atlas (if DDGI enabled).
  constexpr xiiStringView k_AtmosphereTransmittanceLUT = "AtmTransmittance"_xiisv; ///< xiiRenderGraphTextureHandle - 256x64 R16G16B16A16F atmosphere transmittance LUT.
  constexpr xiiStringView k_AtmosphereMultiScatterLUT  = "AtmMultiScatter"_xiisv;  ///< xiiRenderGraphTextureHandle - 32x32 R16G16B16A16F multiple-scattering LUT.
  constexpr xiiStringView k_RawAOTexture               = "RawAO"_xiisv;            ///< xiiRenderGraphTextureHandle - raw GTAO / HBAO+ term (R8_UNORM).
  constexpr xiiStringView k_StableAOTexture            = "StableAO"_xiisv;         ///< xiiRenderGraphTextureHandle - temporally-denoised AO (R8_UNORM).

  // Stage 6 - Main Lighting.

  constexpr xiiStringView k_DirectLightingBuffer   = "DirectLighting"_xiisv;       ///< xiiRenderGraphTextureHandle - direct lighting HDR (R16G16B16A16F).
  constexpr xiiStringView k_IndirectLightingBuffer = "IndirectLighting"_xiisv;     ///< xiiRenderGraphTextureHandle - indirect lighting HDR (R16G16B16A16F).
  constexpr xiiStringView k_SSRTexture             = "SSRTerm"_xiisv;              ///< xiiRenderGraphTextureHandle - screen-space reflection radiance (R16G16B16A16F).
  constexpr xiiStringView k_RTRawGI                = "RTRawGI"_xiisv;              ///< xiiRenderGraphTextureHandle - raw RT indirect diffuse before denoising.
  constexpr xiiStringView k_RTFinalGI              = "RTFinalGI"_xiisv;            ///< xiiRenderGraphTextureHandle - denoised RT GI.
  constexpr xiiStringView k_RTRawReflections       = "RTRawReflections"_xiisv;     ///< xiiRenderGraphTextureHandle - raw RT reflection radiance.
  constexpr xiiStringView k_RTFinalReflections     = "RTFinalReflections"_xiisv;   ///< xiiRenderGraphTextureHandle - denoised RT reflections.
  constexpr xiiStringView k_VolumetricScattering   = "VolumetricScattering"_xiisv; ///< xiiRenderGraphTextureHandle - integrated volumetric light contribution.
  constexpr xiiStringView k_SkyRadiance            = "SkyRadiance"_xiisv;          ///< xiiRenderGraphTextureHandle - sky + atmosphere contribution.

  // Stage 7 - Forward / Composite.

  constexpr xiiStringView k_HDRSceneColor = "HDRSceneColor"_xiisv; ///< xiiRenderGraphTextureHandle - combined HDR scene color after opaque (R16G16B16A16F).

  // Stage 8 - Transparency.

  constexpr xiiStringView k_OITAccumulateBuffer = "OITAccumulate"_xiisv;      ///< xiiRenderGraphTextureHandle - WBOIT weighted accumulation target (R16G16B16A16F).
  constexpr xiiStringView k_OITRevealBuffer     = "OITReveal"_xiisv;          ///< xiiRenderGraphTextureHandle - WBOIT reveal (transmittance) target (R8_UNORM).
  constexpr xiiStringView k_DecalDataBuffer     = "DecalData"_xiisv;          ///< xiiRenderGraphBufferHandle - packed decal instance records uploaded from extracted render data.
  constexpr xiiStringView k_DecalVisibleList    = "DecalVisibleList"_xiisv;   ///< xiiRenderGraphBufferHandle - GPU-visible decal indices, [0]=count.
  constexpr xiiStringView k_DecalDrawCommands   = "DecalDrawCommands"_xiisv;  ///< xiiRenderGraphBufferHandle - GPU-built mesh decal command/index stream.
  constexpr xiiStringView k_DecalTileList       = "DecalTileList"_xiisv;      ///< xiiRenderGraphBufferHandle - per-tile projected decal index list.
  constexpr xiiStringView k_DecalAtlasAlbedo    = "DecalAtlasAlbedo"_xiisv;   ///< xiiRenderGraphTextureHandle - active packed decal albedo atlas.
  constexpr xiiStringView k_DecalAtlasNormal    = "DecalAtlasNormal"_xiisv;   ///< xiiRenderGraphTextureHandle - active packed decal normal atlas.
  constexpr xiiStringView k_DecalAtlasMaterial  = "DecalAtlasMaterial"_xiisv; ///< xiiRenderGraphTextureHandle - active packed decal material atlas.
  constexpr xiiStringView k_DecalAtlasEmissive  = "DecalAtlasEmissive"_xiisv; ///< xiiRenderGraphTextureHandle - active packed decal emissive atlas.
  constexpr xiiStringView k_DecalCount          = "DecalCount"_xiisv;         ///< uint32 - number of decals uploaded for this view.

  // Stage 9 - Screen-Space Effects.

  constexpr xiiStringView k_PlanarReflectionMap = "PlanarReflectionMap"_xiisv; ///< xiiRenderGraphTextureHandle - planar reflection render target.

  // Stage 10 - Temporal Reconstruction.

  constexpr xiiStringView k_LuminanceHistogram = "LuminanceHistogram"_xiisv; ///< xiiRenderGraphBufferHandle - 256-bin log-luminance histogram.
  constexpr xiiStringView k_CurrentExposure    = "CurrentExposure"_xiisv;    ///< xiiRenderGraphBufferHandle - single float EV100 exposure value.
  constexpr xiiStringView k_TAAResolvedColor   = "TAAResolved"_xiisv;        ///< xiiRenderGraphTextureHandle - temporally-resolved color (R16G16B16A16F).
  constexpr xiiStringView k_UpscaledColor      = "UpscaledColor"_xiisv;      ///< xiiRenderGraphTextureHandle - upscaled output at native resolution.

  // Stage 11 - Post-Processing.

  constexpr xiiStringView k_BloomTexture   = "BloomTexture"_xiisv;   ///< xiiRenderGraphTextureHandle - bloom-composited HDR result.
  constexpr xiiStringView k_GradedColor    = "GradedColor"_xiisv;    ///< xiiRenderGraphTextureHandle - color-graded + filmic output.
  constexpr xiiStringView k_LDRSceneColor  = "LDRSceneColor"_xiisv;  ///< xiiRenderGraphTextureHandle - tone-mapped LDR output.
  constexpr xiiStringView k_SharpenedColor = "SharpenedColor"_xiisv; ///< xiiRenderGraphTextureHandle - final sharpened LDR output.
} // namespace xiiRGBlackboardKeys
