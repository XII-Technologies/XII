/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Physical atmosphere parameters shared by LUT generation and composition.
/// All distances and extinction coefficients use kilometres.
DECLARE_CONSTANT_BUFFER_AUTO(xiiAtmosphereConstants)
{
  FLOAT4(PlanetAtmosphereRadiiScaleHeights); ///< x = planet radius, y = atmosphere radius, z = Rayleigh scale height, w = Mie scale height.
  FLOAT4(RayleighScattering);                ///< xyz = molecular scattering coefficient in km^-1.
  FLOAT4(MieScatteringAndPhase);             ///< xyz = aerosol scattering coefficient in km^-1, w = Henyey-Greenstein asymmetry.
  FLOAT4(MieAbsorption);                     ///< xyz = aerosol absorption coefficient in km^-1.
  FLOAT4(OzoneAbsorption);                   ///< xyz = ozone absorption coefficient in km^-1.
  FLOAT4(PlanetUpAndGroundAltitudeMeters);   ///< xyz = normalized world-space planet up, w = sea-level world altitude along up.
  UINT4(SampleCounts);                       ///< x = transmittance integration steps, y = sqrt multi-scatter samples.
};

#if XII_DISABLED(XII_SHADER_PLATFORM)
static_assert(sizeof(xiiAtmosphereConstants) == 112U, "Atmosphere constants must remain byte-compatible with the shaders.");
#endif
