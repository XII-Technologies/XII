/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Pipeline/Passes/Lighting/SurfaceLightingConstants.h>
Texture2D<float> g_SurfaceDepth;
Texture2D<float2> g_SurfaceNormal;
Texture2D<float4> g_SurfaceAlbedo;
Texture2D<float4> g_SurfaceMaterial;
Texture2D<float4> g_SurfaceEmissive;
struct SurfaceSample { float3 Position,Normal,View,Albedo; float Roughness,Metallic,Specular,Transmission; };
bool LoadSurface(uint2 pixel,out SurfaceSample result)
{
  result=(SurfaceSample)0;
  const float depth=g_SurfaceDepth.Load(int3(pixel,0));
  if(!isfinite(depth) || depth<=0 || depth>1) return false;
  const float2 uv=(float2(pixel)+0.5f)/float2(SurfaceExtent.xy);
  const float4 world=mul(SurfaceInverseViewProjection,float4(uv.x*2-1,1-uv.y*2,depth,1));
  if(!all(isfinite(world)) || abs(world.w)<1e-8f) return false;
  result.Position=world.xyz/world.w;
  const float2 encoded=g_SurfaceNormal.Load(int3(pixel,0))*2-1;
  float3 normal=float3(encoded,1-abs(encoded.x)-abs(encoded.y));
  if(normal.z<0) normal.xy=(1-abs(normal.yx))*float2(normal.x>=0 ? 1:-1,normal.y>=0 ? 1:-1);
  result.Normal=normalize(normal);
  const float3 toCamera=SurfaceCameraPositionNear.xyz-result.Position;
  result.View=dot(toCamera,toCamera)>1e-12f ? normalize(toCamera):result.Normal;
  result.Albedo=max(g_SurfaceAlbedo.Load(int3(pixel,0)).rgb,0);
  const float4 material=g_SurfaceMaterial.Load(int3(pixel,0));
  result.Roughness=clamp(material.r,0.01f,1); result.Metallic=saturate(material.g); result.Specular=saturate(material.b);
  const uint flags=(uint)round(saturate(material.a)*255);
  result.Transmission=(flags&64)!=0 ? saturate(g_SurfaceEmissive.Load(int3(pixel,0)).a):0;
  return all(isfinite(result.Normal)) && all(isfinite(result.View)) && all(isfinite(result.Albedo)) && all(isfinite(material));
}
