/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
Texture2D<float4> g_HDRBase;
Texture2D<float4> g_OITAccum;
Texture2D<float>  g_OITReveal;
XII_STORAGE_IMAGE_FORMAT(XII_OIT_STORAGE_FORMAT)
RWTexture2D<float4>        g_HDROut;
[numthreads(8, 8, 1)] void main(uint3 id : SV_DispatchThreadID) {
  uint2 size;
  g_HDROut.GetDimensions(size.x, size.y);
  if (any(id.xy >= size))
    return;
  const float4 opaque      = g_HDRBase.Load(int3(id.xy, 0));
  const float4 accumulated = g_OITAccum.Load(int3(id.xy, 0));
  const float  reveal      = saturate(g_OITReveal.Load(int3(id.xy, 0)));
  float3       result      = opaque.rgb;
  if (reveal < 1.0f && accumulated.a > 1.0e-6f)
  {
    result = accumulated.rgb / accumulated.a * (1 - reveal) + opaque.rgb * reveal;
#if !XII_OIT_FULL_PRECISION
    result = clamp(result, -65504.0f, 65504.0f);
#endif
  }
  g_HDROut[id.xy] = float4(result, opaque.a);
}
