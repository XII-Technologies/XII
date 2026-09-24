/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Scene/SceneDatabase.h>

namespace
{
  template <typename T>
  void EnsureCount(xiiDynamicArray<T>& array, xiiUInt32 uiCount, const T& initialValue)
  {
    const xiiUInt32 uiOldCount = array.GetCount();
    array.SetCount(uiCount);
    for (xiiUInt32 i = uiOldCount; i < uiCount; ++i)
      array[i] = initialValue;
  }
}

xiiSceneDatabase::xiiSceneDatabase()  = default;
xiiSceneDatabase::~xiiSceneDatabase() = default;

void xiiSceneDatabase::Reserve(xiiUInt32 uiObjectCapacity)
{
  m_Generations.Reserve(uiObjectCapacity);
  m_FreeIndices.Reserve(uiObjectCapacity);
  m_Alive.Reserve(uiObjectCapacity);
  m_TransformDirty.Reserve(uiObjectCapacity);
  m_GpuDirty.Reserve(uiObjectCapacity);
  m_Parent.Reserve(uiObjectCapacity);
  m_FirstChild.Reserve(uiObjectCapacity);
  m_NextSibling.Reserve(uiObjectCapacity);
  m_LocalTransforms.Reserve(uiObjectCapacity);
  m_GlobalTransforms.Reserve(uiObjectCapacity);
  m_PreviousGlobalTransforms.Reserve(uiObjectCapacity);
  m_LocalBounds.Reserve(uiObjectCapacity);
  m_GlobalBounds.Reserve(uiObjectCapacity);
  m_GeometryIndices.Reserve(uiObjectCapacity);
  m_MaterialIndices.Reserve(uiObjectCapacity);
  m_VisibilityMasks.Reserve(uiObjectCapacity);
  m_UserData.Reserve(uiObjectCapacity);
  m_Flags.Reserve(uiObjectCapacity);
  m_ObjectToGpuIndex.Reserve(uiObjectCapacity);
  m_GpuToObjectIndex.Reserve(uiObjectCapacity);
  m_GpuInstances.Reserve(uiObjectCapacity);
  m_WorkStack.Reserve(uiObjectCapacity);
}

xiiSceneObjectHandle xiiSceneDatabase::CreateObject(const xiiSceneObjectDesc& desc)
{
  xiiUInt32 uiIndex;
  if (!m_FreeIndices.IsEmpty())
  {
    uiIndex = m_FreeIndices.PeekBack();
    m_FreeIndices.PopBack();
  }
  else
  {
    uiIndex = m_Generations.GetCount();
    const xiiUInt32 uiCount = uiIndex + 1U;
    EnsureCount(m_Generations, uiCount, 1U);
    EnsureCount(m_Alive, uiCount, static_cast<xiiUInt8>(0U));
    EnsureCount(m_TransformDirty, uiCount, static_cast<xiiUInt8>(0U));
    EnsureCount(m_GpuDirty, uiCount, static_cast<xiiUInt8>(0U));
    EnsureCount(m_Parent, uiCount, s_uiInvalidIndex);
    EnsureCount(m_FirstChild, uiCount, s_uiInvalidIndex);
    EnsureCount(m_NextSibling, uiCount, s_uiInvalidIndex);
    EnsureCount(m_LocalTransforms, uiCount, xiiMat4::MakeIdentity());
    EnsureCount(m_GlobalTransforms, uiCount, xiiMat4::MakeIdentity());
    EnsureCount(m_PreviousGlobalTransforms, uiCount, xiiMat4::MakeIdentity());
    EnsureCount(m_LocalBounds, uiCount, xiiBoundingBoxSphere::MakeZero());
    EnsureCount(m_GlobalBounds, uiCount, xiiBoundingBoxSphere::MakeZero());
    EnsureCount(m_GeometryIndices, uiCount, xiiInvalidIndex);
    EnsureCount(m_MaterialIndices, uiCount, xiiInvalidIndex);
    EnsureCount(m_VisibilityMasks, uiCount, 0xFFFFFFFFU);
    EnsureCount(m_UserData, uiCount, 0U);
    EnsureCount(m_Flags, uiCount, xiiBitflags<xiiSceneObjectFlags>(xiiSceneObjectFlags::Default));
    EnsureCount(m_ObjectToGpuIndex, uiCount, s_uiInvalidIndex);
  }

  m_Alive[uiIndex]                    = 1U;
  m_TransformDirty[uiIndex]           = 1U;
  m_GpuDirty[uiIndex]                 = 1U;
  m_Parent[uiIndex]                   = s_uiInvalidIndex;
  m_FirstChild[uiIndex]               = s_uiInvalidIndex;
  m_NextSibling[uiIndex]              = s_uiInvalidIndex;
  m_LocalTransforms[uiIndex]          = desc.m_LocalTransform;
  m_GlobalTransforms[uiIndex]         = desc.m_LocalTransform;
  m_PreviousGlobalTransforms[uiIndex] = desc.m_LocalTransform;
  m_LocalBounds[uiIndex]              = desc.m_LocalBounds;
  m_GlobalBounds[uiIndex]             = desc.m_LocalBounds;
  m_GeometryIndices[uiIndex]          = desc.m_uiGeometryIndex;
  m_MaterialIndices[uiIndex]          = desc.m_uiMaterialIndex;
  m_VisibilityMasks[uiIndex]          = desc.m_uiVisibilityMask;
  m_UserData[uiIndex]                 = desc.m_uiUserData;
  m_Flags[uiIndex]                    = desc.m_Flags;

  ++m_uiObjectCount;
  ++m_uiRevision;

  xiiSceneObjectHandle hObject;
  hObject.m_uiIndex      = uiIndex;
  hObject.m_uiGeneration = m_Generations[uiIndex];
  if (desc.m_hParent.IsValid())
    SetParent(hObject, desc.m_hParent);

  return hObject;
}

bool xiiSceneDatabase::DestroyObject(xiiSceneObjectHandle hObject)
{
  if (!IsAlive(hObject))
    return false;

  const xiiUInt32 uiIndex = hObject.m_uiIndex;
  DetachFromParent(uiIndex);

  // Orphans retain their world transform, preventing destruction from moving an entire assembly.
  xiiUInt32 uiChild = m_FirstChild[uiIndex];
  while (uiChild != s_uiInvalidIndex)
  {
    const xiiUInt32 uiNext = m_NextSibling[uiChild];
    m_Parent[uiChild]         = s_uiInvalidIndex;
    m_NextSibling[uiChild]    = s_uiInvalidIndex;
    m_LocalTransforms[uiChild] = m_GlobalTransforms[uiChild];
    MarkSubtreeDirty(uiChild);
    uiChild = uiNext;
  }

  m_FirstChild[uiIndex] = s_uiInvalidIndex;
  m_Alive[uiIndex]      = 0U;
  m_TransformDirty[uiIndex] = 0U;
  m_GpuDirty[uiIndex]   = 1U;
  m_ObjectToGpuIndex[uiIndex] = s_uiInvalidIndex;
  ++m_Generations[uiIndex];
  if (m_Generations[uiIndex] == 0U)
    m_Generations[uiIndex] = 1U;
  m_FreeIndices.PushBack(uiIndex);

  --m_uiObjectCount;
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::IsAlive(xiiSceneObjectHandle hObject) const
{
  return hObject.IsValid() && hObject.m_uiIndex < m_Generations.GetCount() && m_Alive[hObject.m_uiIndex] != 0U && m_Generations[hObject.m_uiIndex] == hObject.m_uiGeneration;
}

bool xiiSceneDatabase::WouldCreateCycle(xiiUInt32 uiObject, xiiUInt32 uiParent) const
{
  for (xiiUInt32 uiCurrent = uiParent; uiCurrent != s_uiInvalidIndex; uiCurrent = m_Parent[uiCurrent])
  {
    if (uiCurrent == uiObject)
      return true;
  }
  return false;
}

void xiiSceneDatabase::DetachFromParent(xiiUInt32 uiObject)
{
  const xiiUInt32 uiParent = m_Parent[uiObject];
  if (uiParent == s_uiInvalidIndex)
    return;

  xiiUInt32* pLink = &m_FirstChild[uiParent];
  while (*pLink != s_uiInvalidIndex && *pLink != uiObject)
    pLink = &m_NextSibling[*pLink];
  if (*pLink == uiObject)
    *pLink = m_NextSibling[uiObject];

  m_Parent[uiObject]      = s_uiInvalidIndex;
  m_NextSibling[uiObject] = s_uiInvalidIndex;
}

void xiiSceneDatabase::AttachToParent(xiiUInt32 uiObject, xiiUInt32 uiParent)
{
  m_Parent[uiObject]      = uiParent;
  m_NextSibling[uiObject] = m_FirstChild[uiParent];
  m_FirstChild[uiParent]  = uiObject;
}

bool xiiSceneDatabase::SetParent(xiiSceneObjectHandle hObject, xiiSceneObjectHandle hParent)
{
  if (!IsAlive(hObject) || (hParent.IsValid() && !IsAlive(hParent)))
    return false;

  const xiiUInt32 uiParent = hParent.IsValid() ? hParent.m_uiIndex : s_uiInvalidIndex;
  if (uiParent == hObject.m_uiIndex || (uiParent != s_uiInvalidIndex && WouldCreateCycle(hObject.m_uiIndex, uiParent)))
    return false;
  if (m_Parent[hObject.m_uiIndex] == uiParent)
    return true;

  DetachFromParent(hObject.m_uiIndex);
  if (uiParent != s_uiInvalidIndex)
    AttachToParent(hObject.m_uiIndex, uiParent);
  MarkSubtreeDirty(hObject.m_uiIndex);
  ++m_uiRevision;
  return true;
}

void xiiSceneDatabase::MarkSubtreeDirty(xiiUInt32 uiObject)
{
  m_WorkStack.Clear();
  m_WorkStack.PushBack(uiObject);
  while (!m_WorkStack.IsEmpty())
  {
    const xiiUInt32 uiCurrent = m_WorkStack.PeekBack();
    m_WorkStack.PopBack();
    m_TransformDirty[uiCurrent] = 1U;
    m_GpuDirty[uiCurrent]       = 1U;
    for (xiiUInt32 uiChild = m_FirstChild[uiCurrent]; uiChild != s_uiInvalidIndex; uiChild = m_NextSibling[uiChild])
      m_WorkStack.PushBack(uiChild);
  }
}

bool xiiSceneDatabase::SetLocalTransform(xiiSceneObjectHandle hObject, const xiiMat4& localTransform)
{
  if (!IsAlive(hObject) || !localTransform.IsValid())
    return false;
  m_LocalTransforms[hObject.m_uiIndex] = localTransform;
  MarkSubtreeDirty(hObject.m_uiIndex);
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::SetLocalBounds(xiiSceneObjectHandle hObject, const xiiBoundingBoxSphere& localBounds)
{
  if (!IsAlive(hObject) || !localBounds.IsValid())
    return false;
  m_LocalBounds[hObject.m_uiIndex] = localBounds;
  MarkSubtreeDirty(hObject.m_uiIndex);
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::SetGeometry(xiiSceneObjectHandle hObject, xiiUInt32 uiGeometryIndex)
{
  if (!IsAlive(hObject)) return false;
  m_GeometryIndices[hObject.m_uiIndex] = uiGeometryIndex;
  m_GpuDirty[hObject.m_uiIndex] = 1U;
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::SetMaterial(xiiSceneObjectHandle hObject, xiiUInt32 uiMaterialIndex)
{
  if (!IsAlive(hObject)) return false;
  m_MaterialIndices[hObject.m_uiIndex] = uiMaterialIndex;
  m_GpuDirty[hObject.m_uiIndex] = 1U;
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::SetFlags(xiiSceneObjectHandle hObject, xiiBitflags<xiiSceneObjectFlags> flags)
{
  if (!IsAlive(hObject)) return false;
  m_Flags[hObject.m_uiIndex] = flags;
  m_GpuDirty[hObject.m_uiIndex] = 1U;
  ++m_uiRevision;
  return true;
}

bool xiiSceneDatabase::SetVisibilityMask(xiiSceneObjectHandle hObject, xiiUInt32 uiVisibilityMask)
{
  if (!IsAlive(hObject)) return false;
  m_VisibilityMasks[hObject.m_uiIndex] = uiVisibilityMask;
  m_GpuDirty[hObject.m_uiIndex] = 1U;
  ++m_uiRevision;
  return true;
}

const xiiMat4& xiiSceneDatabase::GetGlobalTransform(xiiSceneObjectHandle hObject) const
{
  XII_ASSERT_DEV(IsAlive(hObject), "Invalid scene object handle.");
  return m_GlobalTransforms[hObject.m_uiIndex];
}

const xiiBoundingBoxSphere& xiiSceneDatabase::GetGlobalBounds(xiiSceneObjectHandle hObject) const
{
  XII_ASSERT_DEV(IsAlive(hObject), "Invalid scene object handle.");
  return m_GlobalBounds[hObject.m_uiIndex];
}

void xiiSceneDatabase::UpdateDirtyTransforms()
{
  m_Stats.m_uiHierarchyDepth = 0U;
  m_WorkStack.Clear();

  // Roots are processed first; descendants are then guaranteed to observe an updated parent.
  for (xiiUInt32 i = 0; i < m_Alive.GetCount(); ++i)
  {
    if (m_Alive[i] != 0U && m_Parent[i] == s_uiInvalidIndex)
    {
      m_WorkStack.PushBack(i);
      m_WorkStack.PushBack(0U);
    }
  }

  while (!m_WorkStack.IsEmpty())
  {
    const xiiUInt32 uiDepth = m_WorkStack.PeekBack(); m_WorkStack.PopBack();
    const xiiUInt32 uiObject = m_WorkStack.PeekBack(); m_WorkStack.PopBack();
    m_Stats.m_uiHierarchyDepth = xiiMath::Max(m_Stats.m_uiHierarchyDepth, uiDepth);

    if (m_TransformDirty[uiObject] != 0U)
    {
      m_PreviousGlobalTransforms[uiObject] = m_GlobalTransforms[uiObject];
      const xiiUInt32 uiParent = m_Parent[uiObject];
      m_GlobalTransforms[uiObject] = uiParent == s_uiInvalidIndex ? m_LocalTransforms[uiObject] : m_GlobalTransforms[uiParent] * m_LocalTransforms[uiObject];
      m_GlobalBounds[uiObject] = m_LocalBounds[uiObject];
      m_GlobalBounds[uiObject].Transform(m_GlobalTransforms[uiObject]);
      m_TransformDirty[uiObject] = 0U;
      m_GpuDirty[uiObject]       = 1U;
    }

    for (xiiUInt32 uiChild = m_FirstChild[uiObject]; uiChild != s_uiInvalidIndex; uiChild = m_NextSibling[uiChild])
    {
      m_WorkStack.PushBack(uiChild);
      m_WorkStack.PushBack(uiDepth + 1U);
    }
  }
}

void xiiSceneDatabase::RebuildGpuInstances()
{
  const xiiUInt32 uiPreviousCount = m_GpuInstances.GetCount();
  m_GpuToObjectIndex.Clear();
  m_GpuInstances.Clear();
  m_GpuToObjectIndex.Reserve(m_uiObjectCount);
  m_GpuInstances.Reserve(m_uiObjectCount);
  m_UploadRanges.Clear();

  bool      bRangeOpen = false;
  xiiUInt32 uiRangeStart = 0U;
  for (xiiUInt32 uiObject = 0; uiObject < m_Alive.GetCount(); ++uiObject)
  {
    if (m_Alive[uiObject] == 0U)
      continue;

    const xiiUInt32 uiGpuIndex = m_GpuInstances.GetCount();
    const bool bMoved = m_ObjectToGpuIndex[uiObject] != uiGpuIndex;
    m_ObjectToGpuIndex[uiObject] = uiGpuIndex;
    m_GpuToObjectIndex.PushBack(uiObject);

    xiiGpuSceneInstance& instance = m_GpuInstances.ExpandAndGetRef();
    instance.m_GlobalTransform         = m_GlobalTransforms[uiObject];
    instance.m_PreviousGlobalTransform = m_PreviousGlobalTransforms[uiObject];
    const xiiBoundingBoxSphere& bounds = m_GlobalBounds[uiObject];
    instance.m_BoundsCenterRadius      = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fSphereRadius);
    instance.m_BoundsExtents           = xiiVec4(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z, 0.0f);
    instance.m_uiGeometryIndex         = m_GeometryIndices[uiObject];
    instance.m_uiMaterialIndex         = m_MaterialIndices[uiObject];
    instance.m_uiObjectIndex           = uiObject;
    instance.m_uiFlags                 = m_Flags[uiObject].GetValue();
    instance.m_uiVisibilityMask        = m_VisibilityMasks[uiObject];
    instance.m_uiUserData              = m_UserData[uiObject];

    const bool bDirty = m_GpuDirty[uiObject] != 0U || bMoved || uiPreviousCount != m_uiObjectCount;
    if (bDirty && !bRangeOpen)
    {
      bRangeOpen   = true;
      uiRangeStart = uiGpuIndex;
    }
    else if (!bDirty && bRangeOpen)
    {
      m_UploadRanges.PushBack({uiRangeStart, uiGpuIndex - uiRangeStart});
      bRangeOpen = false;
    }
    m_GpuDirty[uiObject] = 0U;
  }

  if (bRangeOpen)
    m_UploadRanges.PushBack({uiRangeStart, m_GpuInstances.GetCount() - uiRangeStart});
}

void xiiSceneDatabase::CommitFrame(xiiUInt64 uiFrameIndex)
{
  if (m_uiLastCommittedFrame == uiFrameIndex)
    return;

  xiiUInt32 uiDirtyCount = 0U;
  for (xiiUInt8 value : m_GpuDirty)
    uiDirtyCount += value != 0U ? 1U : 0U;

  UpdateDirtyTransforms();
  RebuildGpuInstances();

  m_Stats.m_uiObjectCount      = m_uiObjectCount;
  m_Stats.m_uiObjectCapacity   = m_Alive.GetCount();
  m_Stats.m_uiDirtyObjectCount = uiDirtyCount;
  m_Stats.m_uiRevision         = m_uiRevision;
  m_uiLastCommittedFrame       = uiFrameIndex;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneDatabase);
