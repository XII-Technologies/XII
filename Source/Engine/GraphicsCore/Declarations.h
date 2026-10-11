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
