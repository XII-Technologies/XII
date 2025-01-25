#pragma once

#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Pipeline/Declarations.h>

struct XII_GRAPHICSCORE_DLL xiiViewRenderMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    DiffuseLitOnly,
    SpecularLitOnly,
    LightCount,
    DecalCount,
    TexCoordsUV0,
    TexCoordsUV1,
    VertexColors0,
    VertexColors1,
    VertexNormals,
    VertexTangents,
    PixelNormals,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    EmissiveColor,
    Roughness,
    Occlusion,
    Depth,
    StaticVsDynamic,
    BoneWeights,

    ENUM_COUNT,

    Default = None
  };

  static xiiTempHashedString GetPermutationValue(Enum renderMode);
  static int                 GetRenderPassForShader(Enum renderMode);
  static void                GetDebugText(Enum renderMode, xiiStringBuilder& out_sDebugText);
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiViewRenderMode);
