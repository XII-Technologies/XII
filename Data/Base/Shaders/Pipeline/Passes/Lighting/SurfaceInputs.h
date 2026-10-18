/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Pipeline/Passes/Lighting/SurfaceGeometryInputs.h>
Texture2D<float4> g_SurfaceEmissive;
bool LoadSurface(uint2 pixel,out SurfaceSample result)
{
  if(!LoadSurfaceGeometry(pixel,result)) return false;
  result.Transmission=(result.MaterialFlags&64)!=0 ? saturate(g_SurfaceEmissive.Load(int3(pixel,0)).a):0;
  return isfinite(result.Transmission);
}
