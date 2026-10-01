/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

#if XII_ENABLED(XII_SHADER_PLATFORM)
static const uint XII_MAX_REFLECTION_PROBES         = 64u;
static const uint XII_REFLECTION_PROBE_SHAPE_SPHERE = 0u;
static const uint XII_REFLECTION_PROBE_SHAPE_BOX    = 1u;
#else
static constexpr xiiUInt32 XII_MAX_REFLECTION_PROBES         = 64U;
static constexpr xiiUInt32 XII_REFLECTION_PROBE_SHAPE_SPHERE = 0U;
static constexpr xiiUInt32 XII_REFLECTION_PROBE_SHAPE_BOX    = 1U;
#endif

/// GPU representation of one local reflection probe.
struct XII_SHADER_STRUCT xiiGPUReflectionProbe
{
  MAT4(WorldToProbe);          ///< Converts world positions and directions to the capture's local space.
  FLOAT4(PositionAndRadius);   ///< xyz = capture position, w = conservative world-space influence radius.
  FLOAT4(HalfExtentsAndBlend); ///< xyz = local box half extents, w = blend distance.
  FLOAT4(ProbeParameters);     ///< x = sphere radius, y = intensity, z = saturation, w = influence shape.
  UINT4(Metadata);             ///< x = stable ID, y = cubemap descriptor slot, z = signed priority bits, w = parallax correction flag.
};

DECLARE_CONSTANT_BUFFER_AUTO(xiiReflectionProbeConstants)
{
  UINT1(ActiveProbeCount);
  UINT1(TotalClusterCount);
  UINT2(_ReflectionProbePadding);
};
