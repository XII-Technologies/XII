/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Canonical surface record used by ray-tracing hit shaders.
///
/// Authored material schemas remain free to arrange their own parameter blocks. The ray-tracing
/// scene resolves the well-known PBR properties into this stable representation so hit shaders do
/// not depend on a particular schema layout or frame-sliced material-buffer offset.
struct XII_SHADER_STRUCT xiiRayTracingMaterialData
{
  FLOAT4(BaseColorOpacity);             ///< xyz = linear base color, w = opacity.
  FLOAT4(EmissiveColorAndRoughness);    ///< xyz = emitted radiance in nits, w = perceptual roughness.
  FLOAT4(SurfaceParameters);            ///< x = metallic, y = dielectric specular, z = transmission, w = occlusion.
  UINT4(Metadata);                      ///< x = material slot, y = stable object ID, z = shading model, w = feature flags.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiRayTracingMaterialData) == 64U, "Ray-tracing material records must match the HLSL structured-buffer stride.");
static_assert(alignof(xiiRayTracingMaterialData) == 16U, "Ray-tracing material records require aligned CPU storage.");
#endif
