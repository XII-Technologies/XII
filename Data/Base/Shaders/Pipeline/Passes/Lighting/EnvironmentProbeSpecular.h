/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/StorageImageFormat.h>
#include <Shaders/Pipeline/EnvironmentBRDF.h>
#include <Shaders/Pipeline/Passes/Lighting/SurfaceGeometryInputs.h>
SamplerState LinearClampSampler;
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightingSampling.h>
Texture2D<float2> g_BRDFLut;
Texture2D<float>  g_Occlusion;
XII_STORAGE_IMAGE_FORMAT("rgba32f")
RWTexture2D<float4> g_Lighting;
XII_STORAGE_IMAGE_FORMAT("rgba32f")
RWTexture2D<float4>        g_ProbeIrradiance;
[numthreads(8, 8, 1)] void main(uint3 tid : SV_DispatchThreadID) {
  if (any(tid.xy >= SurfaceExtent.xy))
    return;
  SurfaceSample surface;
  float3        lighting = 0;
  float4        diffuse  = 0;
  if (LoadSurfaceGeometry(tid.xy, surface))
  {
    const float  roughness = max(surface.Roughness, 0.02f);
    const float  NoV       = saturate(dot(surface.Normal, surface.View));
    const float2 brdf      = g_BRDFLut.SampleLevel(LinearClampSampler, float2(NoV, roughness), 0);
    float        coverage;
    const float3 radiance  = xiiSampleReflectionEnvironment(surface.Position, reflect(-surface.View, surface.Normal), roughness, coverage);
    const float  occlusion = saturate(surface.Occlusion * (SurfaceExtent.z != 0 ? g_Occlusion.Load(int3(tid.xy, 0)) : 1));
    lighting               = radiance * xiiEnvironmentSpecularResponse(surface.Albedo, surface.Metallic, surface.Specular, brdf) * (coverage * occlusion * SurfaceEvaluation.x);
    float        diffuseCoverage;
    const float3 irradiance      = xiiSampleReflectionIrradiance(surface.Position, surface.Normal, diffuseCoverage);
    const float3 remainingEnergy = 1.0f - saturate(xiiEnvironmentSpecularResponse(surface.Albedo, surface.Metallic, surface.Specular, brdf));
    diffuse                      = float4(irradiance * remainingEnergy * 3.1415926536f, diffuseCoverage);
  }
  g_Lighting[tid.xy]        = float4(all(isfinite(lighting)) ? max(lighting, 0) : 0, 1);
  g_ProbeIrradiance[tid.xy] = all(isfinite(diffuse)) ? max(diffuse, 0) : 0;
}
