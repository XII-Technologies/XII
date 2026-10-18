/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Directional Lambert shading and ordinary shadow projection.
/// Geometry, camera and mesh layout use the existing canonical raster constants.
DECLARE_CONSTANT_BUFFER_AUTO(xiiDirectionalMeshConstants)
{
  MAT4(ShadowViewProjectionMatrix);
  FLOAT4(SunDirectionIntensity);
  FLOAT4(AmbientColor);
  FLOAT4(ShadowTexelSize);
};
