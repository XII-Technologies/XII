/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-view procedural cloud shadow projection parameters.
DECLARE_CONSTANT_BUFFER_AUTO(xiiCloudShadowConstants)
{
  FLOAT4(LayerOriginAndInvScale);      ///< xyz = point on cloud plane in metres, w = inverse base shadow scale.
  FLOAT4(ProjectionAxisUAndDetail);    ///< xyz = cloud plane U axis, w = detail frequency multiplier.
  FLOAT4(ProjectionAxisVAndCoverage);  ///< xyz = cloud plane V axis, w = fractional cloud coverage.
  FLOAT4(LayerNormalAndOpticalDepth);  ///< xyz = cloud plane normal, w = vertical optical depth.
  FLOAT4(WindStrengthAndEnabled);      ///< xy = wind velocity in m/s, z = shadow strength, w = enabled.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiCloudShadowConstants) == 80U, "Cloud shadow constants must remain byte-compatible with the shaders.");
#endif
