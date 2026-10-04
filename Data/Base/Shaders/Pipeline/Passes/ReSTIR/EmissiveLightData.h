/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// One world-space emissive triangle generated from the canonical GPU scene.
///
/// Radiance is expressed in nits. PowerWeight is area multiplied by Rec. 709 luminance and is
/// retained separately so proposal distributions do not have to reconstruct it from geometry.
struct XII_SHADER_STRUCT xiiGpuEmissiveTriangle
{
#if XII_DISABLED(XII_SHADER_PLATFORM)
  XII_DECLARE_POD_TYPE();
#endif
  FLOAT4(Position0AndArea);
  FLOAT4(Position1AndPowerWeight);
  FLOAT4(Position2AndInvArea);
  FLOAT4(NormalAndTwoSided);
  FLOAT4(RadianceAndLuminance);
  UINT4(Metadata); ///< x = stable ID, y = scene instance, z = source primitive, w = material index.
};

/// Counters written by emissive extraction. The saturating triangle count is safe to consume as a
/// dispatch or sampling bound; overflow records the number of valid triangles that did not fit.
struct XII_SHADER_STRUCT xiiGpuEmissiveLightHeader
{
#if XII_DISABLED(XII_SHADER_PLATFORM)
  XII_DECLARE_POD_TYPE();
#endif
  UINT1(TriangleCount);
  UINT1(OverflowCount);
  UINT1(CandidateCount);
  UINT1(FrameIndex);
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiGpuEmissiveTriangle) == 96U, "Emissive triangle records must match the HLSL structured-buffer stride.");
static_assert(sizeof(xiiGpuEmissiveLightHeader) == 16U, "Emissive light headers must match the HLSL structured-buffer stride.");
#endif
