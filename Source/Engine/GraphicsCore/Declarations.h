#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiShaderStageBinary;
struct xiiVertexDeclarationInfo;

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

struct XII_RENDERERCORE_DLL xiiPermutationVar
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  xiiHashedString m_sName;
  xiiHashedString m_sValue;

  XII_ALWAYS_INLINE bool operator==(const xiiPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};
