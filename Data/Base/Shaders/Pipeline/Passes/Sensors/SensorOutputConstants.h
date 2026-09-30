/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Physical calibration and detector response for one sensor capture.
DECLARE_CONSTANT_BUFFER_AUTO(xiiSensorOutputConstants)
{
  FLOAT4(Intrinsics);                 // fx, fy, cx, cy in sensor pixels.
  FLOAT4(RangeAndExposure);           // near metres, far metres, exposure seconds, rolling-shutter seconds.
  FLOAT4(SpectralSensitivityAndQE);   // RGB spectral weights, quantum efficiency.
  FLOAT4(SignalConversion);           // radiance-to-electrons, analog gain, saturation electrons, wavelength nm.
  FLOAT4(NoiseParameters);            // read noise e-, shot-noise scale, base depth sigma m, depth sigma / m.
  UINT4(OutputDescription);           // type, noise model, output bit depth, random seed.
  UINT4(FrameAndResolution);           // frame index, width, height, shutter type.
};

