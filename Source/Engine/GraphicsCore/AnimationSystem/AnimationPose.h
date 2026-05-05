/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
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

struct XII_GRAPHICSCORE_DLL xiiAnimationPose
{
  void Clear();
  void ResetToRestPose(const xiiSkeletonResource& skeleton);
  void BuildModelSpacePose(const xiiSkeletonResource& skeleton);
  void BuildSkinningMatrices(const xiiSkeletonResource& skeleton);

  void Blend(const xiiAnimationPose& a, const xiiAnimationPose& b, float fWeight);
  void AdditiveBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& additivePose, float fWeight);
  void LayeredBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& layerPose, const xiiSkeletonResource& skeleton, xiiUInt16 uiRootJoint, float fWeight);

  xiiDynamicArray<xiiTransform> m_LocalTransforms;
  xiiDynamicArray<xiiMat4>      m_ModelTransforms;
  xiiDynamicArray<xiiMat4>      m_SkinningMatrices;
  xiiAnimationRootMotion        m_RootMotion;
};
