#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>

class xiiShaderStageBinary;
struct xiiInputLayoutInfo;

using xiiTexture2DResourceHandle         = xiiTypedResourceHandle<class xiiTexture2DResource>;
using xiiRenderToTexture2DResourceHandle = xiiTypedResourceHandle<class xiiRenderToTexture2DResource>;
using xiiTextureCubeResourceHandle       = xiiTypedResourceHandle<class xiiTextureCubeResource>;
using xiiMeshBufferResourceHandle        = xiiTypedResourceHandle<class xiiMeshBufferResource>;
using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;
using xiiMeshResourceHandle              = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiMaterialResourceHandle          = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiShaderResourceHandle            = xiiTypedResourceHandle<class xiiShaderResource>;
using xiiShaderPermutationResourceHandle = xiiTypedResourceHandle<class xiiShaderPermutationResource>;
using xiiRenderPipelineResourceHandle    = xiiTypedResourceHandle<class xiiRenderPipelineResource>;
using xiiDecalResourceHandle             = xiiTypedResourceHandle<class xiiDecalResource>;
using xiiDecalAtlasResourceHandle        = xiiTypedResourceHandle<class xiiDecalAtlasResource>;

struct XII_GRAPHICSCORE_DLL xiiPermutationVar
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  xiiHashedString m_sName;
  xiiHashedString m_sValue;

  XII_ALWAYS_INLINE bool operator==(const xiiPermutationVar& other) const
  {
    return m_sName == other.m_sName && m_sValue == other.m_sValue;
  }
};

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
