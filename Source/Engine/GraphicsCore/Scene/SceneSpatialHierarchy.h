/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Frustum.h>
#include <GraphicsCore/Scene/SceneTypes.h>

struct XII_GRAPHICSCORE_DLL xiiSceneSpatialQuery
{
  xiiUInt32                        m_uiVisibilityMask = 0xFFFFFFFFU;
  xiiBitflags<xiiSceneObjectFlags> m_RequiredFlags;
  xiiBitflags<xiiSceneObjectFlags> m_ExcludedFlags;
};

struct XII_GRAPHICSCORE_DLL xiiSceneSpatialStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiLeafCount   = 0U;
  xiiUInt32 m_uiNodeCount   = 0U;
  xiiUInt32 m_uiTreeHeight  = 0U;
  xiiUInt32 m_uiReinsertions = 0U;
  float     m_fAreaRatio    = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneSpatialStats);

/// Incrementally updated dynamic BVH used as the CPU broad phase for GPU visibility.
///
/// Leaves use motion-expanded "fat" bounds. Small movements only refit the exact leaf bounds and
/// avoid structural changes; objects leaving their fat volume are reinserted using a surface-area
/// heuristic. This keeps update cost bounded for dense simulation scenes while producing a much
/// smaller candidate list for GPU frustum and Hi-Z culling.
class XII_GRAPHICSCORE_DLL xiiSceneSpatialHierarchy
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneSpatialHierarchy);

public:
  explicit xiiSceneSpatialHierarchy(float fFatBoundsMargin = 0.25f);

  void Clear();
  void Reserve(xiiUInt32 uiObjectCapacity);

  bool Insert(xiiSceneObjectHandle hObject, const xiiBoundingBox& bounds, xiiUInt32 uiVisibilityMask, xiiBitflags<xiiSceneObjectFlags> flags);
  bool Remove(xiiSceneObjectHandle hObject);
  bool Update(xiiSceneObjectHandle hObject, const xiiBoundingBox& bounds, const xiiVec3& vDisplacement, xiiUInt32 uiVisibilityMask, xiiBitflags<xiiSceneObjectFlags> flags);

  void QueryFrustum(const xiiFrustum& frustum, const xiiSceneSpatialQuery& query, xiiDynamicArray<xiiSceneObjectHandle>& out_objects) const;
  void QueryBox(const xiiBoundingBox& bounds, const xiiSceneSpatialQuery& query, xiiDynamicArray<xiiSceneObjectHandle>& out_objects) const;

  [[nodiscard]] xiiSceneSpatialStats GetStats() const;

private:
  struct Node
  {
    xiiBoundingBox                  m_FatBounds = xiiBoundingBox::MakeInvalid();
    xiiBoundingBox                  m_ExactBounds = xiiBoundingBox::MakeInvalid();
    xiiSceneObjectHandle            m_hObject;
    xiiBitflags<xiiSceneObjectFlags> m_Flags;
    xiiUInt32                       m_uiVisibilityMask = 0U;
    xiiInt32                        m_iParent = -1;
    xiiInt32                        m_iLeft = -1;
    xiiInt32                        m_iRight = -1;
    xiiInt32                        m_iHeight = -1;
    xiiInt32                        m_iNextFree = -1;

    [[nodiscard]] bool IsLeaf() const { return m_iLeft == -1; }
  };

  [[nodiscard]] xiiInt32 AllocateNode();
  void                       FreeNode(xiiInt32 iNode);
  void                       InsertLeaf(xiiInt32 iLeaf);
  void                       RemoveLeaf(xiiInt32 iLeaf);
  void                       RefitAncestors(xiiInt32 iNode);
  [[nodiscard]] xiiInt32     FindBestSibling(const xiiBoundingBox& bounds) const;
  [[nodiscard]] bool         PassesFilter(const Node& node, const xiiSceneSpatialQuery& query) const;
  [[nodiscard]] xiiBoundingBox MakeFatBounds(const xiiBoundingBox& bounds, const xiiVec3& vDisplacement) const;
  [[nodiscard]] static float SurfaceArea(const xiiBoundingBox& bounds);

  xiiDynamicArray<Node>     m_Nodes;
  xiiDynamicArray<xiiInt32> m_ObjectToNode;
  mutable xiiDynamicArray<xiiInt32> m_QueryStack;
  xiiInt32                  m_iRoot = -1;
  xiiInt32                  m_iFreeList = -1;
  xiiUInt32                 m_uiLeafCount = 0U;
  xiiUInt32                 m_uiReinsertions = 0U;
  float                     m_fFatBoundsMargin = 0.25f;
};

