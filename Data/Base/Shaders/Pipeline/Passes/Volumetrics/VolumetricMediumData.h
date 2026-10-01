/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Shader-side mirror of xiiGpuVolumetricMedium. The C++ representation has an
/// explicit size assertion and is uploaded as a 96-byte structured-buffer record.
struct xiiGpuVolumetricMedium
{
  float4 CenterAndShape;           ///< xyz = centre in metres, w = xiiVolumetricMediumShape.
  float4 Rotation;                 ///< Local-to-world unit quaternion (xyz, w).
  float4 HalfExtentsAndAnisotropy; ///< xyz = half extents in metres, w = Henyey-Greenstein g.
  float4 ScatteringAndPriority;    ///< xyz = scattering coefficient in m^-1.
  float4 AbsorptionAndPadding;     ///< xyz = absorption coefficient in m^-1.
  float4 EmissionAndPadding;       ///< xyz = emitted radiance in cd/m^2.
};

float3 RotateVolumetricVector(float3 vector, float4 quaternion)
{
  return vector + 2.0f * cross(quaternion.xyz, cross(quaternion.xyz, vector) + quaternion.w * vector);
}

/// Per-froxel optical properties. Keeping the coefficients separate lets the
/// integration pass apply Beer-Lambert extinction and scattering correctly.
struct xiiFroxelMediumData
{
  float4 ScatteringAndExtinction; ///< rgb = sigma_s, a = scalar sigma_t in m^-1.
  float4 EmissionAndAnisotropy;   ///< rgb = emitted radiance, a = scattering-weighted phase g.
};
