#pragma once

#include "Platforms.h"

#if XII_ENABLED(PLATFORM_SHADER)

// HLSL

struct Transform
{
  float4 r0;
  float4 r1;
  float4 r2;
};

float4x4 TransformToMatrix(Transform t)
{
  return float4x4(t.r0, t.r1, t.r2, float4(0, 0, 0, 1));
}

float4 TransformToPosition(Transform t)
{
  return float4(t.r0.w, t.r1.w, t.r2.w, 1);
}

float3x3 TransformToRotation(Transform t)
{
  return float3x3(t.r0.xyz, t.r1.xyz, t.r2.xyz);
}

#  define XII_SHADER_STRUCT

// Automatic Resource Bindings
#  define DECLARE_CONSTANT_BUFFER_AUTO(Name)            cbuffer Name
#  define DECLARE_TEXTURE_AUTO(Name, Type)              Type Name
#  define DECLARE_SAMPLER_AUTO(Name)                    SamplerState Name
#  define DECLARE_BUFFER_AUTO(Name, Type)               Buffer<Type> Name
#  define DECLARE_STRUCTURED_BUFFER_AUTO(Name, Type)    StructuredBuffer<Type> Name
#  define DECLARE_RW_STRUCTURED_BUFFER_AUTO(Name, Type) RWStructuredBuffer<Type> Name
#  define DECLARE_BYTE_ADDRESS_BUFFER_AUTO(Name)        ByteAddressBuffer Name
#  define DECLARE_RW_BYTE_ADDRESS_BUFFER_AUTO(Name)     RWByteAddressBuffer Name
#  define DECLARE_COMBINED_IMAGE_SAMPLER_AUTO(Name, Type) \
    Type         Name;                                    \
    SamplerState Name##_AutoSampler

#  if XII_ENABLED(PLATFORM_VULKAN)
#    define DECLARE_CONSTANT_BUFFER(Name, Slot, Set)            cbuffer Name : register(b##Slot, space##Set)
#    define DECLARE_TEXTURE(Name, Type, Slot, Set)              Type Name : register(t##Slot, space##Set)
#    define DECLARE_SAMPLER(Name, Slot, Set)                    SamplerState Name : register(s##Slot, space##Set)
#    define DECLARE_BUFFER(Name, Type, Slot, Set)               Buffer<Type> Name : register(u##Slot, space##Set)
#    define DECLARE_STRUCTURED_BUFFER(Name, Type, Slot, Set)    StructuredBuffer<Type> Name : register(u##Slot, space##Set)
#    define DECLARE_RW_STRUCTURED_BUFFER(Name, Type, Slot, Set) RWStructuredBuffer<Type> Name : register(u##Slot, space##Set)
#    define DECLARE_BYTE_ADDRESS_BUFFER(Name, Slot, Set)        ByteAddressBuffer Name : register(t##Slot, space##Set)
#    define DECLARE_RW_BYTE_ADDRESS_BUFFER(Name, Slot, Set)     RWByteAddressBuffer Name : register(u##Slot, space##Set)
#    define DECLARE_COMBINED_IMAGE_SAMPLER(Name, Type, Slot, Set)                     \
      [[vk::combinedImageSampler]] Type         Name : register(t##Slot, space##Set); \
      [[vk::combinedImageSampler]] SamplerState Name##_AutoSampler : register(s##Slot, space##Set)
#    define BEGIN_PUSH_CONSTANTS(Name) struct XII_SHADER_STRUCT XII_PP_CONCAT(Name, _PushConstants)
#    define END_PUSH_CONSTANTS(Name) \
      ;                              \
      [[vk::push_constant]] XII_PP_CONCAT(Name, _PushConstants) Name;
#    define GET_PUSH_CONSTANT(Name, Constant) Name.Constant
#  endif

#  define FLOAT1(Name)                            float Name
#  define FLOAT2(Name)                            float2 Name
#  define FLOAT3(Name)                            float3 Name
#  define FLOAT4(Name)                            float4 Name
#  define INT1(Name)                              int Name
#  define INT2(Name)                              int2 Name
#  define INT3(Name)                              int3 Name
#  define INT4(Name)                              int4 Name
#  define UINT1(Name)                             uint Name
#  define UINT2(Name)                             uint2 Name
#  define UINT3(Name)                             uint3 Name
#  define UINT4(Name)                             uint4 Name
#  define MAT3(Name)                              float3x3 Name
#  define MAT4(Name)                              float4x4 Name
#  define TRANSFORM(Name)                         Transform Name
#  define COLOR4F(Name)                           float4 Name
#  define COLOR4UB(Name)                          uint Name
#  define BOOL1(Name)                             bool Name
#  define PACKEDHALF2(Name1, Name2, CombinedName) uint CombinedName
#  define PACKEDCOLOR4H(Name)     \
    uint XII_PP_CONCAT(Name, RG); \
    uint XII_PP_CONCAT(Name, GB)
#  define UNPACKHALF2(Name1, Name2, CombinedName) \
    float Name1 = f16tof32(CombinedName);         \
    float Name2 = f16tof32(CombinedName >> 16)
#  define UNPACKCOLOR4H(Name) RGBA16FToFloat4(XII_PP_CONCAT(Name, RG), XII_PP_CONCAT(Name, GB))

#else

// C++

#  include <Foundation/Basics/Platform/Common.h>
#  include <GraphicsFoundation/Shader/Types.h>

#  define XII_SHADER_STRUCT                        alignas(16)
#  define DECLARE_CONSTANT_BUFFER(Name, Slot, Set) struct alignas(16) Name
#  define DECLARE_TEXTURE(Name, Type, Slot, Set)
#  define DECLARE_SAMPLER(Name, Slot, Set)
#  define DECLARE_BUFFER(Name, Type, Slot, Set)
#  define DECLARE_STRUCTURED_BUFFER(Name, Type, Slot, Set)
#  define DECLARE_RW_STRUCTURED_BUFFER(Name, Type, Slot, Set)
#  define DECLARE_BYTE_ADDRESS_BUFFER(Name, Slot, Set)
#  define DECLARE_RW_BYTE_ADDRESS_BUFFER(Name, Slot, Set)
#  define DECLARE_COMBINED_IMAGE_SAMPLER(Name, Type, Slot, Set)
#  define BEGIN_PUSH_CONSTANTS(Name) struct XII_SHADER_STRUCT Name
#  define END_PUSH_CONSTANTS(Name)   ;

#  define DECLARE_CONSTANT_BUFFER_AUTO(Name) struct alignas(16) Name
#  define DECLARE_TEXTURE_AUTO(Name, Type)
#  define DECLARE_SAMPLER_AUTO(Name)
#  define DECLARE_BUFFER_AUTO(Name, Type)
#  define DECLARE_STRUCTURED_BUFFER_AUTO(Name, Type)
#  define DECLARE_RW_STRUCTURED_BUFFER_AUTO(Name, Type)
#  define DECLARE_BYTE_ADDRESS_BUFFER_AUTO(Name)
#  define DECLARE_RW_BYTE_ADDRESS_BUFFER_AUTO(Name)
#  define DECLARE_COMBINED_IMAGE_SAMPLER_AUTO(Name, Type)

#  define FLOAT1(Name)    float Name
#  define FLOAT2(Name)    xiiVec2 Name
#  define FLOAT3(Name)    xiiVec3 Name
#  define FLOAT4(Name)    xiiVec4 Name
#  define INT1(Name)      int Name
#  define INT2(Name)      xiiVec2I32 Name
#  define INT3(Name)      xiiVec3I32 Name
#  define INT4(Name)      xiiVec4I32 Name
#  define UINT1(Name)     xiiUInt32 Name
#  define UINT2(Name)     xiiVec2U32 Name
#  define UINT3(Name)     xiiVec3U32 Name
#  define UINT4(Name)     xiiVec4U32 Name
#  define MAT3(Name)      xiiShaderMat3 Name
#  define MAT4(Name)      xiiShaderMat4 Name
#  define TRANSFORM(Name) xiiShaderTransform Name
#  define COLOR4F(Name)   xiiColor Name
#  define COLOR4UB(Name)  xiiColorGammaUB Name
#  define BOOL1(Name)     xiiShaderBool Name
#  define PACKEDHALF2(Name1, Name2, CombinedName) \
    xiiFloat16 Name1;                             \
    xiiFloat16 Name2
#  define PACKEDCOLOR4H(Name) xiiColorLinear16f Name

#endif
