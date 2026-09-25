/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Scene/SceneSpatialHierarchy.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneSpatialQuery, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiSceneSpatialQuery>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_BITFLAGS_MEMBER_PROPERTY("RequiredFlags", xiiSceneObjectFlags, m_RequiredFlags),
      XII_BITFLAGS_MEMBER_PROPERTY("ExcludedFlags", xiiSceneObjectFlags, m_ExcludedFlags),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneSpatialStats, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiSceneSpatialStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("LeafCount", m_uiLeafCount),
      XII_MEMBER_PROPERTY("NodeCount", m_uiNodeCount),
      XII_MEMBER_PROPERTY("TreeHeight", m_uiTreeHeight),
      XII_MEMBER_PROPERTY("Reinsertions", m_uiReinsertions),
      XII_MEMBER_PROPERTY("Rotations", m_uiRotations),
      XII_MEMBER_PROPERTY("AreaRatio", m_fAreaRatio),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiSceneSpatialHierarchy::xiiSceneSpatialHierarchy(float fFatBoundsMargin) :
  m_fFatBoundsMargin(xiiMath::Max(0.0f, fFatBoundsMargin))
{
}

void xiiSceneSpatialHierarchy::Clear()
{
  m_Nodes.Clear();
  m_ObjectToNode.Clear();
  m_iRoot          = -1;
  m_iFreeList      = -1;
  m_uiLeafCount    = 0U;
  m_uiReinsertions = 0U;
  m_uiRotations    = 0U;
}

void xiiSceneSpatialHierarchy::Reserve(xiiUInt32 uiObjectCapacity)
{
  m_Nodes.Reserve(uiObjectCapacity * 2U);
  m_ObjectToNode.Reserve(uiObjectCapacity);
}

xiiInt32 xiiSceneSpatialHierarchy::AllocateNode()
{
  if (m_iFreeList != -1)
  {
    const xiiInt32 iNode = m_iFreeList;
    m_iFreeList          = m_Nodes[iNode].m_iNextFree;
    m_Nodes[iNode]       = Node{};
    return iNode;
  }

  const xiiInt32 iNode      = static_cast<xiiInt32>(m_Nodes.GetCount());
  m_Nodes.ExpandAndGetRef() = Node{};
  return iNode;
}

void xiiSceneSpatialHierarchy::FreeNode(xiiInt32 iNode)
{
  Node& node       = m_Nodes[iNode];
  node             = Node{};
  node.m_iNextFree = m_iFreeList;
  m_iFreeList      = iNode;
}

float xiiSceneSpatialHierarchy::SurfaceArea(const xiiBoundingBox& bounds)
{
  const xiiVec3 v = bounds.GetExtents();
  return 2.0f * (v.x * v.y + v.y * v.z + v.z * v.x);
}

xiiBoundingBox xiiSceneSpatialHierarchy::MakeFatBounds(const xiiBoundingBox& bounds, const xiiVec3& vDisplacement) const
{
  xiiBoundingBox fat = bounds;
  fat.Grow(xiiVec3(m_fFatBoundsMargin));

  // Predictive expansion avoids a remove/insert every frame for coherently moving bodies.
  if (vDisplacement.x < 0.0f) fat.m_vMin.x += vDisplacement.x * 2.0f;
  else
    fat.m_vMax.x += vDisplacement.x * 2.0f;
  if (vDisplacement.y < 0.0f) fat.m_vMin.y += vDisplacement.y * 2.0f;
  else
    fat.m_vMax.y += vDisplacement.y * 2.0f;
  if (vDisplacement.z < 0.0f) fat.m_vMin.z += vDisplacement.z * 2.0f;
  else
    fat.m_vMax.z += vDisplacement.z * 2.0f;
  return fat;
}

bool xiiSceneSpatialHierarchy::Insert(xiiSceneObjectHandle hObject, const xiiBoundingBox& bounds, xiiUInt32 uiVisibilityMask, xiiBitflags<xiiSceneObjectFlags> flags)
{
  if (!hObject.IsValid() || !bounds.IsValid())
    return false;

  if (hObject.m_uiIndex >= m_ObjectToNode.GetCount())
  {
    const xiiUInt32 uiOldCount = m_ObjectToNode.GetCount();
    m_ObjectToNode.SetCount(hObject.m_uiIndex + 1U);
    for (xiiUInt32 i = uiOldCount; i < m_ObjectToNode.GetCount(); ++i)
      m_ObjectToNode[i] = -1;
  }
  if (m_ObjectToNode[hObject.m_uiIndex] != -1)
    return false;

  const xiiInt32 iLeaf              = AllocateNode();
  Node&          leaf               = m_Nodes[iLeaf];
  leaf.m_ExactBounds                = bounds;
  leaf.m_FatBounds                  = MakeFatBounds(bounds, xiiVec3::MakeZero());
  leaf.m_hObject                    = hObject;
  leaf.m_uiVisibilityMask           = uiVisibilityMask;
  leaf.m_Flags                      = flags;
  leaf.m_iHeight                    = 0;
  m_ObjectToNode[hObject.m_uiIndex] = iLeaf;
  InsertLeaf(iLeaf);
  ++m_uiLeafCount;
  return true;
}

bool xiiSceneSpatialHierarchy::Remove(xiiSceneObjectHandle hObject)
{
  if (!hObject.IsValid() || hObject.m_uiIndex >= m_ObjectToNode.GetCount())
    return false;
  const xiiInt32 iLeaf = m_ObjectToNode[hObject.m_uiIndex];
  if (iLeaf == -1 || m_Nodes[iLeaf].m_hObject != hObject)
    return false;

  RemoveLeaf(iLeaf);
  FreeNode(iLeaf);
  m_ObjectToNode[hObject.m_uiIndex] = -1;
  --m_uiLeafCount;
  return true;
}

bool xiiSceneSpatialHierarchy::Update(xiiSceneObjectHandle hObject, const xiiBoundingBox& bounds, const xiiVec3& vDisplacement, xiiUInt32 uiVisibilityMask, xiiBitflags<xiiSceneObjectFlags> flags)
{
  if (!hObject.IsValid() || !bounds.IsValid() || hObject.m_uiIndex >= m_ObjectToNode.GetCount())
    return false;
  const xiiInt32 iLeaf = m_ObjectToNode[hObject.m_uiIndex];
  if (iLeaf == -1 || m_Nodes[iLeaf].m_hObject != hObject)
    return false;

  Node& leaf              = m_Nodes[iLeaf];
  leaf.m_ExactBounds      = bounds;
  leaf.m_uiVisibilityMask = uiVisibilityMask;
  leaf.m_Flags            = flags;
  if (leaf.m_FatBounds.Contains(bounds))
    return true;

  RemoveLeaf(iLeaf);
  leaf.m_FatBounds = MakeFatBounds(bounds, vDisplacement);
  InsertLeaf(iLeaf);
  ++m_uiReinsertions;
  return true;
}

xiiInt32 xiiSceneSpatialHierarchy::FindBestSibling(const xiiBoundingBox& bounds) const
{
  xiiInt32 iNode = m_iRoot;
  while (!m_Nodes[iNode].IsLeaf())
  {
    const Node&    node     = m_Nodes[iNode];
    const float    fArea    = SurfaceArea(node.m_FatBounds);
    xiiBoundingBox combined = node.m_FatBounds;
    combined.ExpandToInclude(bounds);
    const float fCombinedArea    = SurfaceArea(combined);
    const float fInheritanceCost = 2.0f * (fCombinedArea - fArea);

    auto ChildCost = [&](xiiInt32 iChild) {
      xiiBoundingBox childCombined = m_Nodes[iChild].m_FatBounds;
      childCombined.ExpandToInclude(bounds);
      const float fNewArea = SurfaceArea(childCombined);
      return m_Nodes[iChild].IsLeaf() ? fNewArea + fInheritanceCost : fNewArea - SurfaceArea(m_Nodes[iChild].m_FatBounds) + fInheritanceCost;
    };

    const float fLeftCost  = ChildCost(node.m_iLeft);
    const float fRightCost = ChildCost(node.m_iRight);
    if (2.0f * fCombinedArea < fLeftCost && 2.0f * fCombinedArea < fRightCost)
      break;
    iNode = fLeftCost < fRightCost ? node.m_iLeft : node.m_iRight;
  }
  return iNode;
}

void xiiSceneSpatialHierarchy::InsertLeaf(xiiInt32 iLeaf)
{
  if (m_iRoot == -1)
  {
    m_iRoot                  = iLeaf;
    m_Nodes[iLeaf].m_iParent = -1;
    return;
  }

  const xiiInt32 iSibling   = FindBestSibling(m_Nodes[iLeaf].m_FatBounds);
  const xiiInt32 iOldParent = m_Nodes[iSibling].m_iParent;
  const xiiInt32 iNewParent = AllocateNode();
  Node&          parent     = m_Nodes[iNewParent];
  parent.m_iParent          = iOldParent;
  parent.m_iLeft            = iSibling;
  parent.m_iRight           = iLeaf;
  parent.m_iHeight          = m_Nodes[iSibling].m_iHeight + 1;
  parent.m_FatBounds        = m_Nodes[iSibling].m_FatBounds;
  parent.m_FatBounds.ExpandToInclude(m_Nodes[iLeaf].m_FatBounds);

  m_Nodes[iSibling].m_iParent = iNewParent;
  m_Nodes[iLeaf].m_iParent    = iNewParent;
  if (iOldParent == -1)
    m_iRoot = iNewParent;
  else if (m_Nodes[iOldParent].m_iLeft == iSibling)
    m_Nodes[iOldParent].m_iLeft = iNewParent;
  else
    m_Nodes[iOldParent].m_iRight = iNewParent;

  RefitAncestors(iNewParent);
}

void xiiSceneSpatialHierarchy::RemoveLeaf(xiiInt32 iLeaf)
{
  if (iLeaf == m_iRoot)
  {
    m_iRoot = -1;
    return;
  }

  const xiiInt32 iParent      = m_Nodes[iLeaf].m_iParent;
  const xiiInt32 iGrandParent = m_Nodes[iParent].m_iParent;
  const xiiInt32 iSibling     = m_Nodes[iParent].m_iLeft == iLeaf ? m_Nodes[iParent].m_iRight : m_Nodes[iParent].m_iLeft;
  if (iGrandParent == -1)
  {
    m_iRoot                     = iSibling;
    m_Nodes[iSibling].m_iParent = -1;
  }
  else
  {
    if (m_Nodes[iGrandParent].m_iLeft == iParent)
      m_Nodes[iGrandParent].m_iLeft = iSibling;
    else
      m_Nodes[iGrandParent].m_iRight = iSibling;
    m_Nodes[iSibling].m_iParent = iGrandParent;
    RefitAncestors(iGrandParent);
  }
  m_Nodes[iLeaf].m_iParent = -1;
  FreeNode(iParent);
}

void xiiSceneSpatialHierarchy::RefitAncestors(xiiInt32 iNode)
{
  while (iNode != -1)
  {
    iNode      = Balance(iNode);
    Node& node = m_Nodes[iNode];
    if (!node.IsLeaf())
    {
      node.m_iHeight   = 1 + xiiMath::Max(m_Nodes[node.m_iLeft].m_iHeight, m_Nodes[node.m_iRight].m_iHeight);
      node.m_FatBounds = m_Nodes[node.m_iLeft].m_FatBounds;
      node.m_FatBounds.ExpandToInclude(m_Nodes[node.m_iRight].m_FatBounds);
    }
    iNode = node.m_iParent;
  }
}

xiiInt32 xiiSceneSpatialHierarchy::Balance(xiiInt32 iNode)
{
  Node& root = m_Nodes[iNode];
  if (root.IsLeaf() || root.m_iHeight < 2)
    return iNode;

  const xiiInt32 iLeft    = root.m_iLeft;
  const xiiInt32 iRight   = root.m_iRight;
  Node&          left     = m_Nodes[iLeft];
  Node&          right    = m_Nodes[iRight];
  const xiiInt32 iBalance = right.m_iHeight - left.m_iHeight;

  // Rotate the right child above this node. Choosing the taller grandchild to remain attached to
  // the promoted node minimizes the new surface area and preserves logarithmic tree height.
  if (iBalance > 1)
  {
    const xiiInt32 iRightLeft  = right.m_iLeft;
    const xiiInt32 iRightRight = right.m_iRight;
    Node&          rightLeft   = m_Nodes[iRightLeft];
    Node&          rightRight  = m_Nodes[iRightRight];

    right.m_iLeft   = iNode;
    right.m_iParent = root.m_iParent;
    root.m_iParent  = iRight;
    if (right.m_iParent == -1)
      m_iRoot = iRight;
    else if (m_Nodes[right.m_iParent].m_iLeft == iNode)
      m_Nodes[right.m_iParent].m_iLeft = iRight;
    else
      m_Nodes[right.m_iParent].m_iRight = iRight;

    if (rightLeft.m_iHeight > rightRight.m_iHeight)
    {
      right.m_iRight       = iRightLeft;
      root.m_iRight        = iRightRight;
      rightLeft.m_iParent  = iRight;
      rightRight.m_iParent = iNode;
    }
    else
    {
      right.m_iRight       = iRightRight;
      root.m_iRight        = iRightLeft;
      rightRight.m_iParent = iRight;
      rightLeft.m_iParent  = iNode;
    }

    root.m_FatBounds = left.m_FatBounds;
    root.m_FatBounds.ExpandToInclude(m_Nodes[root.m_iRight].m_FatBounds);
    right.m_FatBounds = root.m_FatBounds;
    right.m_FatBounds.ExpandToInclude(m_Nodes[right.m_iRight].m_FatBounds);
    root.m_iHeight  = 1 + xiiMath::Max(left.m_iHeight, m_Nodes[root.m_iRight].m_iHeight);
    right.m_iHeight = 1 + xiiMath::Max(root.m_iHeight, m_Nodes[right.m_iRight].m_iHeight);
    ++m_uiRotations;
    return iRight;
  }

  // Symmetric left-heavy rotation.
  if (iBalance < -1)
  {
    const xiiInt32 iLeftLeft  = left.m_iLeft;
    const xiiInt32 iLeftRight = left.m_iRight;
    Node&          leftLeft   = m_Nodes[iLeftLeft];
    Node&          leftRight  = m_Nodes[iLeftRight];

    left.m_iLeft   = iNode;
    left.m_iParent = root.m_iParent;
    root.m_iParent = iLeft;
    if (left.m_iParent == -1)
      m_iRoot = iLeft;
    else if (m_Nodes[left.m_iParent].m_iLeft == iNode)
      m_Nodes[left.m_iParent].m_iLeft = iLeft;
    else
      m_Nodes[left.m_iParent].m_iRight = iLeft;

    if (leftLeft.m_iHeight > leftRight.m_iHeight)
    {
      left.m_iRight       = iLeftLeft;
      root.m_iLeft        = iLeftRight;
      leftLeft.m_iParent  = iLeft;
      leftRight.m_iParent = iNode;
    }
    else
    {
      left.m_iRight       = iLeftRight;
      root.m_iLeft        = iLeftLeft;
      leftRight.m_iParent = iLeft;
      leftLeft.m_iParent  = iNode;
    }

    root.m_FatBounds = right.m_FatBounds;
    root.m_FatBounds.ExpandToInclude(m_Nodes[root.m_iLeft].m_FatBounds);
    left.m_FatBounds = root.m_FatBounds;
    left.m_FatBounds.ExpandToInclude(m_Nodes[left.m_iRight].m_FatBounds);
    root.m_iHeight = 1 + xiiMath::Max(right.m_iHeight, m_Nodes[root.m_iLeft].m_iHeight);
    left.m_iHeight = 1 + xiiMath::Max(root.m_iHeight, m_Nodes[left.m_iRight].m_iHeight);
    ++m_uiRotations;
    return iLeft;
  }

  return iNode;
}

bool xiiSceneSpatialHierarchy::PassesFilter(const Node& node, const xiiSceneSpatialQuery& query) const
{
  return (node.m_uiVisibilityMask & query.m_uiVisibilityMask) != 0U && node.m_Flags.AreAllSet(query.m_RequiredFlags) && !node.m_Flags.IsAnySet(query.m_ExcludedFlags);
}

void xiiSceneSpatialHierarchy::QueryFrustum(const xiiFrustum& frustum, const xiiSceneSpatialQuery& query, xiiDynamicArray<xiiSceneObjectHandle>& out_objects) const
{
  if (m_iRoot == -1)
    return;
  xiiHybridArray<xiiInt32, 64U> queryStack;
  queryStack.PushBack(m_iRoot);
  while (!queryStack.IsEmpty())
  {
    const xiiInt32 iNode = queryStack.PeekBack();
    queryStack.PopBack();
    const Node& node = m_Nodes[iNode];
    if (frustum.GetObjectPosition(node.m_FatBounds) == xiiVolumePosition::Outside)
      continue;
    if (node.IsLeaf())
    {
      if (PassesFilter(node, query) && frustum.GetObjectPosition(node.m_ExactBounds) != xiiVolumePosition::Outside)
        out_objects.PushBack(node.m_hObject);
    }
    else
    {
      queryStack.PushBack(node.m_iLeft);
      queryStack.PushBack(node.m_iRight);
    }
  }
}

void xiiSceneSpatialHierarchy::QueryBox(const xiiBoundingBox& bounds, const xiiSceneSpatialQuery& query, xiiDynamicArray<xiiSceneObjectHandle>& out_objects) const
{
  if (m_iRoot == -1)
    return;
  xiiHybridArray<xiiInt32, 64U> queryStack;
  queryStack.PushBack(m_iRoot);
  while (!queryStack.IsEmpty())
  {
    const xiiInt32 iNode = queryStack.PeekBack();
    queryStack.PopBack();
    const Node& node = m_Nodes[iNode];
    if (!bounds.Overlaps(node.m_FatBounds))
      continue;
    if (node.IsLeaf())
    {
      if (PassesFilter(node, query) && bounds.Overlaps(node.m_ExactBounds))
        out_objects.PushBack(node.m_hObject);
    }
    else
    {
      queryStack.PushBack(node.m_iLeft);
      queryStack.PushBack(node.m_iRight);
    }
  }
}

xiiSceneSpatialStats xiiSceneSpatialHierarchy::GetStats() const
{
  xiiSceneSpatialStats stats;
  stats.m_uiLeafCount    = m_uiLeafCount;
  stats.m_uiReinsertions = m_uiReinsertions;
  stats.m_uiRotations    = m_uiRotations;
  stats.m_uiTreeHeight   = m_iRoot == -1 ? 0U : static_cast<xiiUInt32>(m_Nodes[m_iRoot].m_iHeight);

  float fTotalArea = 0.0f;
  for (const Node& node : m_Nodes)
  {
    if (node.m_iHeight >= 0)
    {
      ++stats.m_uiNodeCount;
      fTotalArea += SurfaceArea(node.m_FatBounds);
    }
  }
  if (m_iRoot != -1 && SurfaceArea(m_Nodes[m_iRoot].m_FatBounds) > 0.0f)
    stats.m_fAreaRatio = fTotalArea / SurfaceArea(m_Nodes[m_iRoot].m_FatBounds);
  return stats;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneSpatialHierarchy);
