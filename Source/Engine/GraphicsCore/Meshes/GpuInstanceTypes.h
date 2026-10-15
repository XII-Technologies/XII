/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Runtime classification used by scene queries and GPU visibility passes.
struct XII_GRAPHICSCORE_DLL xiiGpuInstanceFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None              = 0,
    Enabled           = XII_BIT(0),
    Static            = XII_BIT(1),
    CastShadows       = XII_BIT(2),
    ReceiveShadows    = XII_BIT(3),
    AlwaysVisible     = XII_BIT(4),
    Occluder          = XII_BIT(5),
    Transparent       = XII_BIT(6),
    SensorVisible     = XII_BIT(7),
    RayTracingVisible = XII_BIT(8),

    Default = Enabled | CastShadows | ReceiveShadows
  };

  struct Bits
  {
    StorageType Enabled : 1;
    StorageType Static : 1;
    StorageType CastShadows : 1;
    StorageType ReceiveShadows : 1;
    StorageType AlwaysVisible : 1;
    StorageType Occluder : 1;
    StorageType Transparent : 1;
    StorageType SensorVisible : 1;
    StorageType RayTracingVisible : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGpuInstanceFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuInstanceFlags);

/// Canonical, backend independent GPU record. It deliberately contains both current and previous
/// transforms so motion vectors never need a second scene lookup.
struct XII_GRAPHICSCORE_DLL xiiGpuSceneInstance
{
  XII_DECLARE_POD_TYPE();

  xiiMat4 m_GlobalTransform         = xiiMat4::MakeIdentity();
  xiiMat4 m_PreviousGlobalTransform = xiiMat4::MakeIdentity();
  /// Rows of the inverse-transpose upper 3x3. Keeping these in the instance record avoids
  /// recomputing an inverse per mesh vertex and produces correct normals under non-uniform scale.
  xiiVec4   m_NormalTransformRow0 = xiiVec4(1.0f, 0.0f, 0.0f, 0.0f);
  xiiVec4   m_NormalTransformRow1 = xiiVec4(0.0f, 1.0f, 0.0f, 0.0f);
  xiiVec4   m_NormalTransformRow2 = xiiVec4(0.0f, 0.0f, 1.0f, 0.0f);
  xiiVec4   m_BoundsCenterRadius  = xiiVec4::MakeZero();
  xiiVec4   m_BoundsExtents       = xiiVec4::MakeZero();
  xiiUInt32 m_uiGeometryIndex     = xiiInvalidIndex;
  xiiUInt32 m_uiMaterialIndex     = xiiInvalidIndex;
  /// Non-reused rendering identity. Zero denotes background. Resolve against the producing
  /// world extraction and validate its authoritative object/component handles.
  xiiUInt32 m_uiObjectId       = 0U;
  xiiUInt32 m_uiFlags          = 0U;
  xiiUInt32 m_uiVisibilityMask = 0xFFFFFFFFU;
  xiiUInt32 m_uiUserData       = 0U;
  xiiUInt32 m_uiFirstPrimitive = 0U;
  xiiUInt32 m_uiPrimitiveCount = 0U; ///< Zero means the complete selected LOD.
  xiiUInt32 m_uiForcedLod      = xiiInvalidIndex;
  float     m_fLodBias         = 0.0f;
  xiiUInt32 m_uiDrawLod        = xiiInvalidIndex;
  /// Dense index of the object supplying geometry-level LOD controls and history. Invalid means self.
  xiiUInt32 m_uiLodSelectionIndex = xiiInvalidIndex;
  /// Matrix indices in the scene's frame snapshot. Counts of zero select bind pose.
  xiiUInt32 m_uiSkinningOffset         = 0U;
  xiiUInt32 m_uiSkinningCount          = 0U;
  xiiUInt32 m_uiPreviousSkinningOffset = 0U;
  xiiUInt32 m_uiPreviousSkinningCount  = 0U;
  xiiUInt32 m_uiBoneIndicesOffset      = 0U;
  xiiUInt32 m_uiBoneWeightsOffset      = 0U;
  xiiUInt32 m_uiSkinningVertexStride   = 0U;
  xiiUInt32 m_uiSkinningPadding        = 0U;
};

static_assert(sizeof(xiiGpuSceneInstance) == 288U && offsetof(xiiGpuSceneInstance, m_uiSkinningOffset) == 256U, "GPU scene records must match GpuSceneCommon.h.");
static_assert(offsetof(xiiGpuSceneInstance, m_uiForcedLod) == 240U && offsetof(xiiGpuSceneInstance, m_fLodBias) == 244U && offsetof(xiiGpuSceneInstance, m_uiDrawLod) == 248U && offsetof(xiiGpuSceneInstance, m_uiLodSelectionIndex) == 252U,
              "GPU LOD controls must preserve the structured-buffer layout.");

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuSceneInstance);

/// Contiguous dirty interval used for partial GPU uploads.
struct XII_GRAPHICSCORE_DLL xiiGpuInstanceUploadRange
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiFirstInstance = 0U;
  xiiUInt32 m_uiInstanceCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuInstanceUploadRange);


/// Non-owning provenance for a derived GPU draw candidate, never a renderer-owned object.
struct XII_GRAPHICSCORE_DLL xiiGpuInstanceSource
{
  xiiGameObjectHandle m_hOwnerObject;
  xiiComponentHandle  m_hOwnerComponent;
  xiiUInt32           m_uiLodIndex      = xiiInvalidIndex;
  xiiUInt32           m_uiSectionIndex  = xiiInvalidIndex;
  xiiUInt32           m_uiMaterialIndex = 0;
  xiiUInt32           m_uiRenderDataId  = 0;
};
