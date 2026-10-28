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
#if (!defined(XII_SCENE_SURFACE_QUERY) || defined(XII_SCENE_DIAGNOSTICS)) && !defined(XII_SCENE_DEPTH_ONLY) && !defined(XII_SCENE_PARTICLE_ATTRIBUTES)
  float3 WorldTangent : TEXCOORD1;
  float  TangentSign : TEXCOORD2;
#endif
  float2 TexCoord : TEXCOORD3;
#if defined(XII_SCENE_DIAGNOSTICS)
  float3                DiagnosticWorldPosition : TEXCOORD11;
  float2                DiagnosticUV1 : TEXCOORD12;
  float4                DiagnosticColor : TEXCOORD13;
  noperspective float2  DiagnosticBarycentrics : TEXCOORD14;
  nointerpolation uint4 DiagnosticIdentity : TEXCOORD15; // flags, LOD, resident meshlet index, source primitive plus one
#  if defined(XII_SCENE_CULL_DIAGNOSTICS)
  nointerpolation uint DiagnosticDecision : TEXCOORD16;
#  endif
#endif
  nointerpolation uint MaterialIndex : TEXCOORD4;
#if defined(XII_SCENE_WORLD_ATTRIBUTES)
  float3 WorldPosition : TEXCOORD7;
#  if defined(XII_SCENE_SURFACE_FLAGS)
  nointerpolation uint SurfaceFlags : TEXCOORD8;
#  endif
#  if !defined(XII_SCENE_SURFACE_NO_IDENTITY)
  nointerpolation uint ObjectUserData : TEXCOORD8;
#  endif
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
