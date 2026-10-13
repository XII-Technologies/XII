/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

xiiResult xiiEditableSkeleton::MergeImportedHierarchy(xiiEditableSkeleton& newSkeleton)
{
  if (&newSkeleton == this)
    return XII_FAILURE;
  auto* pOldSkeleton = this;
  // Validate the imported hierarchy in the document's coordinate system before replacing any owned data.
  newSkeleton.m_ImportTransform = pOldSkeleton->m_ImportTransform;
  newSkeleton.m_RightDir        = pOldSkeleton->m_RightDir;
  newSkeleton.m_UpDir           = pOldSkeleton->m_UpDir;
  newSkeleton.m_bFlipForwardDir = pOldSkeleton->m_bFlipForwardDir;
  newSkeleton.m_fUniformScaling = pOldSkeleton->m_fUniformScaling;
  xiiSkeletonResourceDescriptor imported;
  if (newSkeleton.FillResourceDescriptor(imported).Failed())
    return XII_FAILURE;
  xiiTransform root;
  XII_SUCCEED_OR_RETURN(pOldSkeleton->GetRootTransform(root));

  xiiHashTable<xiiString, const xiiEditableSkeletonJoint*> previous;
  xiiHashSet<const xiiEditableSkeletonJoint*>              visited;
  xiiDynamicArray<xiiEditableSkeletonJoint*>               pending;
  pending.PushBackRange(pOldSkeleton->m_Children);
  while (!pending.IsEmpty())
  {
    const auto* pJoint = pending.PeekBack();
    pending.PopBack();
    if (pJoint == nullptr || visited.Insert(pJoint))
      return XII_FAILURE;
    if (previous.Insert(pJoint->m_sName.GetString(), pJoint))
      return XII_FAILURE;
    pending.PushBackRange(pJoint->m_Children);
  }

  for (xiiUInt32 i = newSkeleton.m_Children.GetCount(); i > 0; --i)
    pending.PushBack(newSkeleton.m_Children[i - 1]);
  xiiUInt32 index = 0;
  while (!pending.IsEmpty())
  {
    auto* pJoint = pending.PeekBack();
    pending.PopBack();
    const xiiEditableSkeletonJoint* pPrevious = nullptr;
    if (previous.TryGetValue(pJoint->m_sName.GetString(), pPrevious))
      pJoint->CopyPropertiesFrom(pPrevious);
    const auto& native = imported.m_Joints[index++];
    const auto  parent = native.m_uiParentIndex < imported.m_Joints.GetCount() ? imported.m_Joints[native.m_uiParentIndex].m_ModelRestPose : root.GetAsMat4();
    // The manipulator needs an orthonormal parent frame, including reflected import coordinates.
    auto x = parent.GetRotationalPart().GetColumn(0);
    auto y = parent.GetRotationalPart().GetColumn(1);
    if (x.NormalizeIfNotZero().Failed())
      return XII_FAILURE;
    y -= x * x.Dot(y);
    if (y.NormalizeIfNotZero().Failed())
      return XII_FAILURE;
    xiiMat3 rotation;
    rotation.SetColumn(0, x);
    rotation.SetColumn(1, y);
    rotation.SetColumn(2, x.CrossRH(y));
    pJoint->m_qGizmoOffsetRotationRO = xiiQuat::MakeFromMat3(rotation);
    pJoint->m_vGizmoOffsetPositionRO = native.m_ModelRestPose.GetTranslationVector();
    for (xiiUInt32 i = pJoint->m_Children.GetCount(); i > 0; --i)
      pending.PushBack(pJoint->m_Children[i - 1]);
  }
  pOldSkeleton->ClearJoints();
  pOldSkeleton->m_Children = std::move(newSkeleton.m_Children);
  return XII_SUCCESS;
}
