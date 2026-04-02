#pragma once

/// \brief Compile-time string keys for the per-view render graph blackboard.
///
/// All passes share this header to avoid typos and ensure consistent key names.
/// Keys with a (*) suffix have a corresponding typed helper written by the named pass.
namespace xiiRGBlackboardKeys
{
  // Pass 1 — Frame Setup
  constexpr const char* k_GPUFrameTimeMs      = "GPUFrameTimeMs";       ///< float  — last completed GPU frame duration.
  constexpr const char* k_GPUFrameTimestampBegin = "GPUTSBegin";        ///< uint64 — raw GPU timestamp at frame begin.
  constexpr const char* k_GPUFrameTimestampEnd   = "GPUTSEnd";          ///< uint64 — raw GPU timestamp at frame end (prev frame).
  constexpr const char* k_FrameIndex          = "FrameIndex";           ///< uint64 — monotonically increasing frame counter.

  // Pass 2 — Dynamic Resolution
  constexpr const char* k_DynamicResolutionScale = "DynamicResolutionScale"; ///< float — [0.5, 1.0] render scale.
  constexpr const char* k_RenderWidth          = "RenderWidth";         ///< uint32 — scaled render width in pixels.
  constexpr const char* k_RenderHeight         = "RenderHeight";        ///< uint32 — scaled render height in pixels.

  // Pass 3 — Per-Frame Buffer Upload
  constexpr const char* k_PerFrameCameraBuffer = "PerFrameCameraBuffer"; ///< xiiGALBuffer* — camera constants structured buffer.
  constexpr const char* k_PerFrameLightBuffer  = "PerFrameLightBuffer";  ///< xiiGALBuffer* — global light constants buffer.
  constexpr const char* k_PerFrameGlobalBuffer = "PerFrameGlobalBuffer"; ///< xiiGALBuffer* — global frame constants buffer.

  // Pass 4 — Skinning and Morph
  constexpr const char* k_SkinnedVertexBuffer  = "SkinnedVertexBuffer";  ///< xiiRGBufferHandle — deformed vertex streams.

  // Pass 5 — Instance Transform and Bounds
  constexpr const char* k_InstanceWorldMatrixBuffer = "InstanceWorldMatrices";  ///< xiiRGBufferHandle — per-instance world matrices.
  constexpr const char* k_InstanceBoundsBuffer      = "InstanceBounds";         ///< xiiRGBufferHandle — per-instance AABB.

  // Pass 6 — LOD and Meshlet
  constexpr const char* k_InstanceLODBuffer    = "InstanceLOD";          ///< xiiRGBufferHandle — per-instance LOD level + metadata.

  // Pass 7 — Coarse Frustum Cull
  constexpr const char* k_VisibleCandidateBuffer = "VisibleCandidates";  ///< xiiRGBufferHandle — coarsely visible instance list.

  // Pass 8 — Occluder Depth Prepass
  constexpr const char* k_OccluderDepthTexture = "OccluderDepth";        ///< xiiRGTextureHandle — occluder depth buffer.

  // Pass 9 — Hi-Z Pyramid
  constexpr const char* k_HiZPyramid           = "HiZPyramid";          ///< xiiRGTextureHandle — full mip-chain depth pyramid.

  // Pass 10 — Hi-Z Occlusion Cull
  constexpr const char* k_SurvivingInstanceBuffer = "SurvivingInstances"; ///< xiiRGBufferHandle — occlusion-culled instance list.

  // Pass 11 — Draw Command Build
  constexpr const char* k_DrawIndirectCommands = "DrawIndirectCommands"; ///< xiiRGBufferHandle — packed DrawIndexedIndirect args.
  constexpr const char* k_DrawCountBuffer      = "DrawCounts";           ///< xiiRGBufferHandle — per-material-bin draw count.

  // Pass 12 — Main Depth Prepass
  constexpr const char* k_SceneDepthTexture    = "SceneDepth";           ///< xiiRGTextureHandle — full-res scene depth.

  // Pass 13 — Motion Vectors
  constexpr const char* k_VelocityBuffer       = "VelocityBuffer";       ///< xiiRGTextureHandle — screen-space velocity.

  // Pass 14 — Normal-Roughness Prepass
  constexpr const char* k_NormalRoughnessBuffer = "NormalRoughness";     ///< xiiRGTextureHandle — compact R8G8B8A8 normal+roughness.

  // Passes 15-17 — Directional Shadows
  constexpr const char* k_ShadowCascadeMatrices    = "ShadowCascadeMatrices"; ///< xiiRGBufferHandle — cascade view-proj matrices.
  constexpr const char* k_ShadowCascadeCount       = "ShadowCascadeCount";    ///< uint32 — active cascade count (0–4).
  constexpr const char* k_DirectionalShadowAtlas   = "DirShadowAtlas";        ///< xiiRGTextureHandle — cascaded shadow map texture.

  // Passes 18-19 — Local Light Shadows
  constexpr const char* k_LocalShadowAtlas    = "LocalShadowAtlas";      ///< xiiRGTextureHandle — spot/point shadow atlas.
  constexpr const char* k_LocalShadowAtlasDescs = "LocalShadowAtlasDescs"; ///< xiiRGBufferHandle — per-light atlas placement data.

  // Pass 20 — Contact Shadows
  constexpr const char* k_ContactShadowTerm   = "ContactShadows";        ///< xiiRGTextureHandle — screen-space contact shadow mask.

  // Pass 21 — Cluster Grid
  constexpr const char* k_ClusterDescriptors  = "ClusterDescriptors";    ///< xiiRGBufferHandle — frustum cluster AABB descriptors.

  // Pass 22 — Light List
  constexpr const char* k_LightIndexBuffer    = "LightIndexBuffer";      ///< xiiRGBufferHandle — per-cluster compact light index list.
  constexpr const char* k_LightGridBuffer     = "LightGrid";             ///< xiiRGBufferHandle — cluster → (offset, count) pairs.

  // Pass 23-24 — Decals
  constexpr const char* k_DecalTileList       = "DecalTileList";         ///< xiiRGBufferHandle — per-tile decal index list.

  // Pass 25 — Atmosphere LUT
  constexpr const char* k_AtmosphereTransmittanceLUT = "AtmTransmittance"; ///< xiiRGTextureHandle — LUT (256×64 R16G16B16A16F).
  constexpr const char* k_AtmosphereMultiScatterLUT  = "AtmMultiScatter";  ///< xiiRGTextureHandle — LUT (32×32 R16G16B16A16F).

  // Pass 26 — Volumetric Froxel Grid
  constexpr const char* k_FroxelMetadataBuffer = "FroxelMetadata";       ///< xiiRGBufferHandle — froxel bounds + phase terms.
  constexpr const char* k_FroxelScatteringBuffer = "FroxelScattering";   ///< xiiRGTextureHandle — 3D froxel scattering/extinction texture.

  // Passes 27-28 — GBuffer
  constexpr const char* k_GBufferAlbedo       = "GBufferAlbedo";         ///< xiiRGTextureHandle — R8G8B8A8_UNORM albedo + AO.
  constexpr const char* k_GBufferNormal       = "GBufferNormal";         ///< xiiRGTextureHandle — R16G16_SNORM oct-encoded normals.
  constexpr const char* k_GBufferMaterial     = "GBufferMaterial";       ///< xiiRGTextureHandle — R8G8B8A8 roughness/metallic/specular.
  constexpr const char* k_GBufferEmissive     = "GBufferEmissive";       ///< xiiRGTextureHandle — R16G16B16A16F emissive + special.

  // Passes 29-30 — AO
  constexpr const char* k_RawAOTexture        = "RawAO";                 ///< xiiRGTextureHandle — raw GTAO term (R8_UNORM).
  constexpr const char* k_StableAOTexture     = "StableAO";              ///< xiiRGTextureHandle — denoised stable AO.

  // Pass 31 — SSR
  constexpr const char* k_SSRTexture          = "SSRTerm";               ///< xiiRGTextureHandle — screen-space reflection radiance.

  // Passes 36-38 — RT Shadows
  constexpr const char* k_RTRawShadowMask     = "RTRawShadows";          ///< xiiRGTextureHandle — raw RT shadow mask per light.
  constexpr const char* k_RTFinalShadowMask   = "RTFinalShadows";        ///< xiiRGTextureHandle — denoised RT shadow mask.

  // Passes 39-41 — RT Reflections
  constexpr const char* k_RTRawReflections    = "RTRawReflections";      ///< xiiRGTextureHandle — raw RT reflection radiance.
  constexpr const char* k_RTFinalReflections  = "RTFinalReflections";    ///< xiiRGTextureHandle — denoised RT reflections.

  // Passes 42-44 — RT GI
  constexpr const char* k_RTRawGI             = "RTRawGI";               ///< xiiRGTextureHandle — raw RT indirect diffuse.
  constexpr const char* k_RTFinalGI           = "RTFinalGI";             ///< xiiRGTextureHandle — denoised RT GI.

  // Passes 46-47 — Lighting Combine
  constexpr const char* k_DirectLightingBuffer  = "DirectLighting";      ///< xiiRGTextureHandle — direct lighting HDR R16G16B16A16F.
  constexpr const char* k_IndirectLightingBuffer = "IndirectLighting";   ///< xiiRGTextureHandle — indirect lighting HDR R16G16B16A16F.

  // Pass 48 — Volumetric Lighting
  constexpr const char* k_VolumetricScattering  = "VolumetricScattering"; ///< xiiRGTextureHandle — volumetric contribution.

  // Pass 49 — Sky
  constexpr const char* k_SkyRadiance           = "SkyRadiance";         ///< xiiRGTextureHandle — sky contribution.

  // Pass 50 — Opaque Composite
  constexpr const char* k_HDRSceneColor          = "HDRSceneColor";      ///< xiiRGTextureHandle — combined HDR scene color after opaque.

  // Pass 53-54 — Exposure
  constexpr const char* k_LuminanceHistogram     = "LuminanceHistogram"; ///< xiiRGBufferHandle — 256-bin histogram.
  constexpr const char* k_CurrentExposure        = "CurrentExposure";    ///< xiiRGBufferHandle — single float exposure value.

  // Pass 55 — TAA
  constexpr const char* k_TAAResolvedColor       = "TAAResolved";        ///< xiiRGTextureHandle — temporally-resolved color.

  // Pass 56 — Upscaling
  constexpr const char* k_UpscaledColor          = "UpscaledColor";      ///< xiiRGTextureHandle — upscaled output at native res.

  // Pass 57-59 — Bloom
  constexpr const char* k_BloomTexture           = "BloomTexture";       ///< xiiRGTextureHandle — bloom-composited HDR.

  // Pass 60 — Tone Mapping
  constexpr const char* k_LDRSceneColor          = "LDRSceneColor";      ///< xiiRGTextureHandle — tone-mapped LDR.

  // Pass 61 — Color Grading
  constexpr const char* k_GradedColor            = "GradedColor";        ///< xiiRGTextureHandle — graded + filmic output.

  // Pass 62 — Sharpening
  constexpr const char* k_SharpenedColor         = "SharpenedColor";     ///< xiiRGTextureHandle — final sharpened output.
}
