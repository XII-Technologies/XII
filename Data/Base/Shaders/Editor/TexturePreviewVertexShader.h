/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Shaders/Editor/TexturePreviewConstants.h>

struct VS_IN
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 UV : TEXCOORD0;
};

struct VS_OUT
{
  float4 Position : SV_Position;
  float3 Normal : NORMAL;
  float2 UV : TEXCOORD0;
};

VS_OUT main(VS_IN input)
{
  VS_OUT output;
  output.Position = mul(ViewProjection, float4(input.Position, 1));
  // Camera matrices use forward depth; the render graph's scene depth is reversed Z.
  output.Position.z = output.Position.w - output.Position.z;
  output.Normal = input.Normal;
  output.UV = input.UV;
  return output;
}
