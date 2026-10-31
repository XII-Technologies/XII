/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Geometry/TransparentSurface.h>
TransparentOutput main(GBufferVertex input, bool frontFace : SV_IsFrontFace)
{
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  return xiiShadeTransparentSurface(input, xiiSampleSceneSurface(material, input, frontFace));
}
