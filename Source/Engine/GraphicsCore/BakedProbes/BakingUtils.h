#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <GraphicsCore/Declarations.h>

using xiiCompressedSkyVisibility = xiiUInt32;

namespace xiiBakingUtils
{
  XII_GRAPHICSCORE_DLL xiiVec3 FibonacciSphere(xiiUInt32 uiSampleIndex, xiiUInt32 uiNumSamples);

  XII_GRAPHICSCORE_DLL xiiCompressedSkyVisibility CompressSkyVisibility(const xiiAmbientCube<float>& skyVisibility);
  XII_GRAPHICSCORE_DLL void                       DecompressSkyVisibility(xiiCompressedSkyVisibility compressedSkyVisibility, xiiAmbientCube<float>& out_skyVisibility);
} // namespace xiiBakingUtils
