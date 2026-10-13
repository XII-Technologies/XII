/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Transform.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct XII_GRAPHICSCORE_DLL xiiAnimationRootMotion
{
  xiiVec3   m_vTranslation = xiiVec3::MakeZero();
  xiiQuat   m_qRotation    = xiiQuat::MakeIdentity();
  xiiUInt16 m_uiJointIndex = xiiMath::MaxValue<xiiUInt16>();
};

class xiiMeshResourceDescriptor;

struct XII_GRAPHICSCORE_DLL xiiAnimationPose
{
  void Clear();
  void ResetToRestPose(const xiiSkeletonResource& skeleton);
  void BuildModelSpacePose(const xiiSkeletonResource& skeleton);
  void BuildSkinningMatrices(const xiiSkeletonResource& skeleton);

  /// Resolves mesh bone names to joints and writes mesh-indexed matrices using the mesh's inverse bind poses.
  /// Empty bone maps use the skeleton palette. Sparse unused slots are identity. Invalid mappings/matrices fail and clear output.
  /// Requires model transforms for this skeleton revision. Output and scratch capacity are reused between calls.
  /// The output must not alias any of this pose's arrays.
  xiiResult BuildMeshSkinningPalette(const xiiSkeletonResource& skeleton, const xiiMeshResourceDescriptor& mesh,
                                     xiiDynamicArray<xiiMat4>& out_palette, xiiDynamicBitfield& inout_usedSlots) const;

  /// Copies local joint transforms by name. Rejects mismatched counts, duplicate/empty names and invalid transforms.
  /// Clears the output on failure. The caller must ensure the pose belongs to this skeleton revision.
  xiiResult CopyLocalTransforms(const xiiSkeletonResource& skeleton, xiiMap<xiiString, xiiTransform>& out_transforms) const;

  void Blend(const xiiAnimationPose& a, const xiiAnimationPose& b, float fWeight);
  void AdditiveBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& additivePose, float fWeight);
  void LayeredBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& layerPose, const xiiSkeletonResource& skeleton, xiiUInt16 uiRootJoint, float fWeight);

  xiiDynamicArray<xiiTransform> m_LocalTransforms;
  xiiDynamicArray<xiiMat4>      m_ModelTransforms;
  xiiDynamicArray<xiiMat4>      m_SkinningMatrices;
  xiiAnimationRootMotion        m_RootMotion;
};
