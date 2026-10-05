/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Canonical PBR surface record shared by raster, ray-query, sensor and debug shaders.
///
/// Authored material schemas may arrange parameters freely. GraphicsCore resolves well-known PBR
/// semantics into this fixed record when a material revision is uploaded, keeping hot shaders free
/// from schema reflection and hashed lookups.
struct XII_SHADER_STRUCT xiiGpuSurfaceMaterial
{
  FLOAT4(BaseColorOpacity);          ///< xyz = linear base color, w = opacity.
  FLOAT4(EmissiveColorAndRoughness); ///< xyz = emitted radiance in nits, w = perceptual roughness.
  FLOAT4(SurfaceParameters);         ///< x = metallic, y = dielectric specular, z = transmission, w = occlusion.
  FLOAT4(LayerParameters);           ///< x = alpha cutoff, y = normal scale, z = clear coat, w = clear-coat roughness.
  UINT4(TextureIndices0);            ///< Base color, normal, metallic-roughness and occlusion bindless SRVs.
  UINT4(TextureIndices1);            ///< Emissive, height, clear-coat and transmission bindless SRVs.
  UINT4(Metadata);                   ///< x = shading model, y = feature flags, z = effective alpha mode, w = blend mode.
  FLOAT4(AdvancedParameters);        ///< x = thickness, y = index of refraction, z = anisotropy, w = sheen roughness.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiGpuSurfaceMaterial) == 128U, "GPU surface-material records must match the HLSL structured-buffer stride.");
static_assert(alignof(xiiGpuSurfaceMaterial) == 16U, "GPU surface-material records require aligned CPU storage.");
#endif
