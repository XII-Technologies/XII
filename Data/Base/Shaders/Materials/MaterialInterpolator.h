#pragma once

#include <Shaders/Common/Common.h>

// Defines:
// USE_WORLDPOS
// USE_NORMAL
// USE_TANGENT
// USE_TEXCOORD0
// USE_TEXCOORD1
// USE_COLOR0
// USE_COLOR1
// USE_SKINNING
// USE_DEBUG_INTERPOLATOR
// CUSTOM_INTERPOLATOR
// VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX

struct VS_IN
{
  float3 Position : ATTRIB0;

#if defined(USE_NORMAL)
  float3 Normal : ATTRIB1;
#endif

#if defined(USE_TANGENT)
  float4 Tangent : ATTRIB2;
#endif

#if defined(USE_TEXCOORD0)
  float2 TexCoord0 : ATTRIB3;

#  if defined(USE_TEXCOORD1)
  float2 TexCoord1 : ATTRIB4;
#  endif
#endif

#if defined(USE_COLOR0)
  float4 Color0 : ATTRIB5;

#  if defined(USE_COLOR1)
  float4 Color1 : ATTRIB6;
#  endif
#endif

#if defined(USE_SKINNING)
  float4 BoneWeights : ATTRIB7;
  uint4  BoneIndices : ATTRIB8;
#endif

  uint InstanceID : ATTRIB9;
  uint VertexID : ATTRIB10;
};

#if defined(VERTEX_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO && defined(VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX)
#      define RENDER_TARGET_ARRAY_INDEX
#    endif
#  endif
#  define STAGE_TEMPLATE VS_OUT
#  include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#  undef STAGE_TEMPLATE

#elif defined(GEOMETRY_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO && !defined(VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX)
#      define STAGE_TEMPLATE VS_OUT
#      include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#      undef STAGE_TEMPLATE

#      define RENDER_TARGET_ARRAY_INDEX
#      define STAGE_TEMPLATE GS_OUT
#      include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#      undef STAGE_TEMPLATE
#    endif
#  endif

#elif defined(PIXEL_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO
#      define RENDER_TARGET_ARRAY_INDEX
#    endif
#  endif
#  define STAGE_TEMPLATE PS_IN
#  include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#  undef STAGE_TEMPLATE
#endif

// typedef VS_OUT PS_IN;
