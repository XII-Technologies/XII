#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <GraphicsCore/Declarations.h>

using xiiCompressedSkyVisibility = xiiUInt32;

namespace xiiBakingUtils
{
  XII_RENDERERCORE_DLL xiiVec3 FibonacciSphere(xiiUInt32 uiSampleIndex, xiiUInt32 uiNumSamples);

  XII_RENDERERCORE_DLL xiiCompressedSkyVisibility CompressSkyVisibility(const xiiAmbientCube<float>& skyVisibility);
  XII_RENDERERCORE_DLL void                       DecompressSkyVisibility(xiiCompressedSkyVisibility compressedSkyVisibility, xiiAmbientCube<float>& out_skyVisibility);
} // namespace xiiBakingUtils
