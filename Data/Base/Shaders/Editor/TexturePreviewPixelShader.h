/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Shaders/Common/Common.h>
#include <Shaders/Editor/TexturePreviewConstants.h>

struct PS_IN
{
  float4 Position : SV_Position;
  float3 Normal : NORMAL;
  float2 UV : TEXCOORD0;
};

#if PREVIEW_CUBE
TextureCube<float4> PreviewTexture;
#else
Texture2D<float4> PreviewTexture;
#endif
SamplerState PreviewSampler;

float4 main(PS_IN input) : SV_Target
{
#if PREVIEW_CUBE
  float3 uv = CubeMapDirection(normalize(input.Normal));
#else
  float2 uv = input.UV;
#endif
  float4 texel = Mip < 0 ? PreviewTexture.Sample(PreviewSampler, uv) : PreviewTexture.SampleLevel(PreviewSampler, uv, Mip);
  float3 color = texel.rgb;
  if (Channel == 0)
  {
#if PREVIEW_CUBE
    clip(texel.a - AlphaThreshold);
#else
    int checker = (int(floor(input.UV.x * 16)) + int(floor(input.UV.y * 16))) & 1;
    color       = lerp(checker == 0 ? float3(0.66, 0.66, 0.66) : float3(0.82, 0.82, 0.82), color, texel.a);
#endif
  }
  else if (Channel == 2)
    color = texel.rrr;
  else if (Channel == 3)
    color = texel.ggg;
  else if (Channel == 4)
    color = texel.bbb;
  else if (Channel == 5)
    color = texel.aaa;
  else if (Channel == 6 || Channel == 7)
  {
    float coverage = Channel == 6 ? texel.r : texel.a;
    color          = coverage >= AlphaThreshold ? float3(1, 1, 1) : float3(0, 0, 0);
  }
  // Inspect raw linear-format channel values as authored; sRGB textures were decoded by hardware.
  if (IsLinear != 0 || Channel == 5)
    color = SrgbToLinear(color);
  if (OutputIsSrgb == 0)
    color = LinearToSrgb(color);
  return float4(color, 1);
}
