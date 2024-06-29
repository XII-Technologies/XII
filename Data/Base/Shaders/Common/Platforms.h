#pragma once

#include "StandardMacros.h"

#ifndef PLATFORM_SHADER
#  define PLATFORM_SHADER XII_OFF
#endif

#define PLATFORM_VULKAN XII_OFF
#define PLATFORM_D3D12  XII_OFF
#define PLATFORM_D3D11  XII_OFF
#define PLATFORM_NULL   XII_OFF

#if defined(D3D_SM40_93) || defined(D3D_SM40) || defined(D3D_SM41) || defined(D3D_SM50)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER XII_ON

#  undef PLATFORM_D3D11
#  define PLATFORM_D3D11 XII_ON

// D3D11 does not support push constants, so we just emulate them via a normal constant buffer.

#  define BEGIN_PUSH_CONSTANTS(Name)        cbuffer Name
#  define END_PUSH_CONSTANTS(Name)          ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

float xiiEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float2 xiiEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float3 xiiEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float4 xiiEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

// Custom implementation of HLSL2021 intrinsic function which is not available in SM50 or lower.
// https://github.com/microsoft/DirectXShaderCompiler/wiki/HLSL-2021#logical-operation-short-circuiting-for-scalars
float2 select(bool2 condition, float2 yes, float2 no)
{
  return float2(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y);
}

float3 select(bool3 condition, float3 yes, float3 no)
{
  return float3(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z);
}

float4 select(bool4 condition, float4 yes, float4 no)
{
  return float4(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z, condition.w ? yes.w : no.w);
}
#endif

#if defined(D3D_SM51) || defined(D3D_SM60) || defined(D3D_SM61) || defined(D3D_SM61) || defined(D3D_SM63) || defined(D3D_SM64) || defined(D3D_SM65) || defined(D3D_SM66) || defined(D3D_SM67)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER XII_ON

#  undef PLATFORM_D3D12
#  define PLATFORM_D3D12 XII_ON

// D3D12 does not support push constants, so we just emulate them via a normal constant buffer.

#  define BEGIN_PUSH_CONSTANTS(Name)        cbuffer Name
#  define END_PUSH_CONSTANTS(Name)          ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

float xiiEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float2 xiiEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float3 xiiEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float4 xiiEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
#endif

#if defined(VK_SM60) || defined(VK_SM61) || defined(VK_SM62) || defined(VK_SM63) || defined(VK_SM64) || defined(VK_SM65) || defined(VK_SM66) || defined(VK_SM67)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER XII_ON

#  undef PLATFORM_VULKAN
#  define PLATFORM_VULKAN XII_ON

#  define BEGIN_PUSH_CONSTANTS(Name) struct XII_SHADER_STRUCT XII_PP_CONCAT(Name, _PushConstants)
#  define END_PUSH_CONSTANTS(Name) \
    ;                              \
    [[vk::push_constant]] XII_PP_CONCAT(Name, _PushConstants) Name;
#  define GET_PUSH_CONSTANT(Name, Constant) Name.Constant

// GetRenderTargetSamplePosition does not have an equivalent function in Vulkan so these values are hard-coded.
// https://learn.microsoft.com/windows/win32/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
static const float2 offsets[] =
  {
    // 1x MSAA
    float2(0, 0),
    // 2x MSAA
    float2(4, 4),
    float2(-4, -4),
    // 4x MSAA
    float2(-2, -6),
    float2(6, -2),
    float2(-6, 2),
    float2(2, 6),
    // 8x MSAA
    float2(1, -3),
    float2(-1, 3),
    float2(-5, 1),
    float2(-3, -5),
    float2(-5, 5),
    float2(-7, -1),
    float2(3, 7),
    float2(7, -7),
    // 16x MSAA
    float2(1, 1),
    float2(-1, -3),
    float2(-3, 2),
    float2(4, -1),
    float2(-5, -2),
    float2(2, 5),
    float2(5, 3),
    float2(3, -5),
    float2(-2, 6),
    float2(0, -7),
    float2(-4, -6),
    float2(-6, 4),
    float2(-8, 0),
    float2(7, -4),
    float2(6, 7),
    float2(-7, -8),
};

// Workaround for error: EvaluateAttributeAtSample intrinsic function unimplemented
// See https://github.com/microsoft/DirectXShaderCompiler/issues/3649
float xiiEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

float2 xiiEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

float3 xiiEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}
float4 xiiEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

#endif

#if defined(NULL_SM)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER XII_ON

#  undef PLATFORM_NULL
#  define PLATFORM_NULL XII_ON

float xiiEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return 0.0f;
}

float2 xiiEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return float2(0.0f, 0.0f);
}

float3 xiiEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return float3(0.0f, 0.0f, 0.0f);
}

float4 xiiEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

#endif
