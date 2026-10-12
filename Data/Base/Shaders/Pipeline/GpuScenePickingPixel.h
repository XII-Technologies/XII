// XII Engine - shared material coverage and native surface query output.

#pragma once

#include <Shaders/Pipeline/GpuSceneSurfaceMaterial.h>

uint4 main(GBufferVertex input, bool isFrontFace : SV_IsFrontFace) : SV_Target0
{
  const xiiGpuSurfaceMaterial material = xiiLoadSceneSurfaceMaterial(input.MaterialIndex, MaterialBaseIndex);
  const float4                color    = xiiSampleSceneBaseColor(material, input.TexCoord);
  xiiClipSceneSurface(material, color, isFrontFace, true);
  const float3 normal    = normalize(input.WorldNormal) * (isFrontFace ? 1.0f : -1.0f);
  const float2 octNormal = EncodeOctNormal(normal);
  return uint4(input.ObjectId, asuint(input.Position.z), asuint(octNormal.x), asuint(octNormal.y));
}
