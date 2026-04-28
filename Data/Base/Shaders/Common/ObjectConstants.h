/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include "ShaderResourceMacros.h"

struct XII_SHADER_STRUCT xiiPerInstanceData
{
  TRANSFORM(ObjectToWorld);
  TRANSFORM(ObjectToWorldNormal);
  FLOAT1(BoundingSphereRadius);
  UINT1(GameObjectID);
  UINT1(VertexColorAccessData);

  INT1(Reserved);
  COLOR4F(Color);
};

#if XII_ENABLED(XII_SHADER_PLATFORM)
DECLARE_STRUCTURED_BUFFER_AUTO(perInstanceData, xiiPerInstanceData);

#  if defined(USE_SKINNING)
DECLARE_STRUCTURED_BUFFER_AUTO(skinningTransforms, Transform);
#  endif

DECLARE_BUFFER_AUTO(perInstanceVertexColors, uint);

#else // C++

XII_DEFINE_AS_POD_TYPE(xiiPerInstanceData);

static_assert(sizeof(xiiPerInstanceData) == 128);
#endif

DECLARE_CONSTANT_BUFFER(xiiObjectConstants, 2, 0)
{
  UINT1(InstanceDataOffset);
};

#if XII_ENABLED(XII_SHADER_PLATFORM)

// Access to instance should usually go through this macro!
// It's a macro so it can work with arbitrary input structs (for VS/GS/PS...)
#  if defined(CAMERA_MODE) && CAMERA_MODE == CAMERA_MODE_STEREO
#    define GetInstanceData() perInstanceData[G.Input.InstanceID / 2 + InstanceDataOffset]
#  else
#    define GetInstanceData() perInstanceData[G.Input.InstanceID + InstanceDataOffset]
#  endif

#  define VERTEX_COLOR_ACCESS_OFFSET_BITS 28
#  define VERTEX_COLOR_ACCESS_OFFSET_MASK ((1 << VERTEX_COLOR_ACCESS_OFFSET_BITS) - 1)

uint GetNumInstanceVertexColorsHelper(uint accessData)
{
  return accessData >> VERTEX_COLOR_ACCESS_OFFSET_BITS;
}

uint GetInstanceVertexColorsHelper(uint accessData, uint vertexID, uint colorIndex)
{
  uint numColorsPerVertex = GetNumInstanceVertexColorsHelper(accessData);
  uint offset             = (accessData & VERTEX_COLOR_ACCESS_OFFSET_MASK) + (vertexID * numColorsPerVertex + colorIndex);
  return colorIndex < numColorsPerVertex ? perInstanceVertexColors[offset] : 0;
}

#  define GetNumInstanceVertexColors()        GetNumInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData)
#  define GetInstanceVertexColors(colorIndex) GetInstanceVertexColorsHelper(GetInstanceData().VertexColorAccessData, G.Input.VertexID, colorIndex)

#endif
