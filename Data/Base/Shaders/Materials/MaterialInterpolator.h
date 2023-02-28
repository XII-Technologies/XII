#pragma once

#include <Shaders/Common/Common.h>

////////// Defines //////////

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

////////// Attributes //////////

// ATTRIB0  - Position;
// ATTRIB1  - Tangent;
// ATTRIB2  - Normal;
// ATTRIB3  - Color0;
// ATTRIB4  - Color1;
// ATTRIB5  - Color2;
// ATTRIB6  - Color3;
// ATTRIB7  - Color4;
// ATTRIB8  - Color5;
// ATTRIB9  - Color6;
// ATTRIB10 - Color7;
// ATTRIB11 - TexCoord0;
// ATTRIB12 - TexCoord1;
// ATTRIB13 - TexCoord2;
// ATTRIB14 - TexCoord3;
// ATTRIB15 - TexCoord4;
// ATTRIB16 - TexCoord5;
// ATTRIB17 - TexCoord6;
// ATTRIB18 - TexCoord7;
// ATTRIB19 - TexCoord8;
// ATTRIB20 - TexCoord9;
// ATTRIB21 - BiTangent;
// ATTRIB22 - BoneIndices0;
// ATTRIB23 - BoneIndices1;
// ATTRIB24 - BoneWeights0;
// ATTRIB25 - BoneWeights1;

///////////////////////////////

struct VS_IN
{
  float3 Position : ATTRIB0;

#if defined(USE_NORMAL)
  float3 Normal : ATTRIB2;
#endif

#if defined(USE_TANGENT)
  float4 Tangent : ATTRIB1;
#endif

#if defined(USE_TEXCOORD0)
  float2 TexCoord0 : ATTRIB11;

#  if defined(USE_TEXCOORD1)
  float2 TexCoord1 : ATTRIB12;
#  endif
#endif

#if defined(USE_COLOR0)
  float4 Color0 : ATTRIB3;

#  if defined(USE_COLOR1)
  float4 Color1 : ATTRIB4;
#  endif
#endif

#if defined(USE_SKINNING)
  float4 BoneWeights : ATTRIB24;
  uint4  BoneIndices : ATTRIB22;
#endif

  uint InstanceID : SV_InstanceID;
  uint VertexID : SV_VertexID;
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
