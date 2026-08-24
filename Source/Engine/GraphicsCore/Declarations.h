/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>

class xiiShaderStageBinary;
struct xiiInputLayoutInfo;

using xiiTexture2DResourceHandle         = xiiTypedResourceHandle<class xiiTexture2DResource>;
using xiiTextureCubeResourceHandle       = xiiTypedResourceHandle<class xiiTextureCubeResource>;
using xiiTexture3DResourceHandle         = xiiTypedResourceHandle<class xiiTexture3DResource>;
using xiiMeshBufferResourceHandle        = xiiTypedResourceHandle<class xiiMeshBufferResource>;
using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;
using xiiMeshResourceHandle              = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiMaterialResourceHandle          = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiShaderResourceHandle            = xiiTypedResourceHandle<class xiiShaderResource>;
using xiiShaderPermutationResourceHandle = xiiTypedResourceHandle<class xiiShaderPermutationResource>;
using xiiDecalResourceHandle             = xiiTypedResourceHandle<class xiiDecalResource>;
using xiiDecalAtlasResourceHandle        = xiiTypedResourceHandle<class xiiDecalAtlasResource>;
using xiiParticleGraphResourceHandle     = xiiTypedResourceHandle<class xiiParticleGraphResource>;

struct XII_GRAPHICSCORE_DLL xiiMeshImportTransform
{
  using StorageType = xiiInt8;

  enum Enum
  {
    Blender_YUp,
    Blender_ZUp,

    Custom = 127,

    Default = Blender_YUp
  };

  static xiiBasisAxis::Enum GetRightDir(xiiMeshImportTransform::Enum transform, xiiBasisAxis::Enum dir);
  static xiiBasisAxis::Enum GetUpDir(xiiMeshImportTransform::Enum transform, xiiBasisAxis::Enum dir);
  static bool               GetFlipForward(xiiMeshImportTransform::Enum transform, bool bFlip);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshImportTransform);
