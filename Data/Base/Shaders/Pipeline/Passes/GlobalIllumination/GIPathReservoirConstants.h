/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiGIPathReservoirConstants)
{
  UINT4(ReservoirGrid);          ///< Grid width, height, tile size, previous storage valid.
  FLOAT4(ReservoirReprojection); ///< Previous minus current NDC jitter in xy.
};

#define XII_GI_PATH_RESERVOIR_STRIDE 48

#if XII_ENABLED(XII_SHADER_PLATFORM)
struct xiiGIPathReservoir
{
  float4 EndpointAndWeight; ///< World endpoint and normalized inverse area density.
  float4 ReceiverAndCount;  ///< Previous primary world position and effective candidate count.
  uint4  Keys;              ///< Tail seed, packed receiver normal, object ID, material signature.
};
#endif
