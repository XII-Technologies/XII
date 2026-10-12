// XII Engine - shared canonical GPU-scene surface vertex interface.

#pragma once

#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Pipeline/PipelineCommon.xiiShader>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Visibility/GpuSceneCommon.h>

struct GBufferVertex
{
  float4 Position : SV_Position;
  float3 WorldNormal : TEXCOORD0;
  float3 WorldTangent : TEXCOORD1;
  float TangentSign : TEXCOORD2;
  float2 TexCoord : TEXCOORD3;
  nointerpolation uint MaterialIndex : TEXCOORD4;
  float4 PreviousClipPosition : TEXCOORD5;
  nointerpolation uint ObjectId : TEXCOORD6;
};

uint LoadByte(ByteAddressBuffer source, uint byteOffset)
{
  const uint word = source.Load(byteOffset & ~3u);
  return (word >> ((byteOffset & 3u) * 8u)) & 0xFFu;
}
