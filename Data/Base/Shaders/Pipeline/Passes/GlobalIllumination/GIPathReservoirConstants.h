/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiGIPathReservoirConstants)
{
  UINT4(ReservoirGrid);          ///< Grid width, height, tile size, previous storage valid.
  UINT4(ReservoirViewport);      ///< Full render width and height in xy.
  FLOAT4(ReservoirReprojection); ///< Previous minus current NDC jitter in xy.
};

#define XII_GI_PATH_RESERVOIR_STRIDE 64

#if XII_ENABLED(XII_SHADER_PLATFORM)
struct xiiGIPathReservoir
{
  float4 EndpointAndWeight; ///< World endpoint and normalized inverse area density.
  float4 ReceiverAndCount;  ///< Previous primary world position and effective candidate count.
  float4 FreshRadiance;     ///< Sum of the anchor pixel's freshly traced incident radiance.
  uint4  Keys;              ///< Tail seed, packed receiver normal, object ID, material signature.
};
#endif
