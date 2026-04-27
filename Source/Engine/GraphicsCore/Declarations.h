#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>

class xiiShaderStageBinary;
struct xiiInputLayoutInfo;

using xiiTexture2DResourceHandle         = xiiTypedResourceHandle<class xiiTexture2DResource>;
using xiiRenderToTexture2DResourceHandle = xiiTypedResourceHandle<class xiiRenderToTexture2DResource>;
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

// ---- New High-Performance Mesh Pipeline ----
/// Handle to a GPU-side meshlet geometry resource (cluster decomposition + packed vertex/index data).
using xiiMeshletResourceHandle          = xiiTypedResourceHandle<class xiiMeshletResource>;
/// Handle to a CPU-writable, GPU-resident dynamic mesh resource (ring-buffered, per-frame upload).
using xiiDynamicMeshResourceHandle      = xiiTypedResourceHandle<class xiiDynamicMeshResource>;
/// Handle to a virtual geometry DAG resource (Nanite-style hierarchical cluster stream).
using xiiVirtualGeometryResourceHandle  = xiiTypedResourceHandle<class xiiVirtualGeometryResource>;
/// Handle to a GPU simulation interop buffer (CUDA/compute → renderer zero-copy).
using xiiGPUSimulationBufferHandle      = xiiTypedResourceHandle<class xiiGPUSimulationBuffer>;

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
