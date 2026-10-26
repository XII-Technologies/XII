/// Copyright (c) Theophilus Eriata. All Rights Reserved.

// XII Engine - shared canonical GPU-scene surface vertex interface.

#pragma once

#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Pipeline/PipelineCommon.xiiShader>
#include <Shaders/Visibility/GpuSceneCommon.h>

struct GBufferVertex
{
  float4 Position : SV_Position;
#if defined(XII_SCENE_PARTICLE_ATTRIBUTES)
  float4                ParticleColor : TEXCOORD9;
  nointerpolation float SoftDistance : TEXCOORD10;
#endif
#if !defined(XII_SCENE_DEPTH_ONLY) && !defined(XII_SCENE_PARTICLE_ATTRIBUTES)
  float3 WorldNormal : TEXCOORD0;
#endif
#if !defined(XII_SCENE_SURFACE_QUERY) && !defined(XII_SCENE_DEPTH_ONLY) && !defined(XII_SCENE_PARTICLE_ATTRIBUTES)
  float3 WorldTangent : TEXCOORD1;
  float  TangentSign : TEXCOORD2;
#endif
  float2               TexCoord : TEXCOORD3;
  nointerpolation uint MaterialIndex : TEXCOORD4;
#if defined(XII_SCENE_WORLD_ATTRIBUTES)
  float3               WorldPosition : TEXCOORD7;
#  if defined(XII_SCENE_SURFACE_FLAGS)
  nointerpolation uint SurfaceFlags : TEXCOORD8;
#  endif
#  if !defined(XII_SCENE_SURFACE_NO_IDENTITY)
  nointerpolation uint ObjectUserData : TEXCOORD8;
#endif
#endif
#if !defined(XII_SCENE_SURFACE_QUERY) && !defined(XII_SCENE_DEPTH_ONLY) && !defined(XII_SCENE_PARTICLE_ATTRIBUTES)
  float4 PreviousClipPosition : TEXCOORD5;
#endif
#if !defined(XII_SCENE_DEPTH_ONLY) && !defined(XII_SCENE_SURFACE_NO_IDENTITY)
  nointerpolation uint ObjectId : TEXCOORD6;
#endif
};

uint LoadByte(ByteAddressBuffer source, uint byteOffset)
{
  const uint word = source.Load(byteOffset & ~3u);
  return (word >> ((byteOffset & 3u) * 8u)) & 0xFFu;
}
