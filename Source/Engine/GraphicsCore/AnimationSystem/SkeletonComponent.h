/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/ComponentManager.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiSkeletonComponentManager     = xiiComponentManagerSimple<class xiiSkeletonComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::Compact, xiiWorldUpdatePhase::PostTransform>;
using xiiSkeletonPoseComponentManager = xiiComponentManagerSimple<class xiiSkeletonPoseComponent, xiiComponentUpdateType::Always, xiiBlockStorageType::Compact, xiiWorldUpdatePhase::PostTransform>;

class XII_GRAPHICSCORE_DLL xiiSkeletonComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonComponent, xiiComponent, xiiSkeletonComponentManager);

public:
  xiiSkeletonComponent();
  ~xiiSkeletonComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void                             SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const;                                     // [ property ]

  void                                  SetAnimationClip(const xiiAnimationClipResourceHandle& hClip); // [ property ]
  const xiiAnimationClipResourceHandle& GetAnimationClip() const;                                      // [ property ]

  void                              SetAnimationGraph(const xiiAnimGraphResourceHandle& hGraph); // [ property ]
  const xiiAnimGraphResourceHandle& GetAnimationGraph() const;                                   // [ property ]

  void  SetPlaybackSpeed(float fSpeed); // [ property ]
  float GetPlaybackSpeed() const;       // [ property ]

  void SetApplyToOwnerMesh(bool bApply); // [ property ]
  bool GetApplyToOwnerMesh() const;      // [ property ]

  void SetFloatParameter(xiiStringView sName, float fValue);
  void SetBoolParameter(xiiStringView sName, bool bValue);

  const xiiAnimationPose& GetCurrentPose() const;
  /// Copies the evaluated local pose by joint name. Fails if unevaluated or the skeleton has since reloaded.
  xiiResult CopyCurrentLocalPose(xiiMap<xiiString, xiiTransform>& out_transforms) const;
  void      Update();

protected:
  void ApplyPoseToSkinnedMesh();

protected:
  xiiSkeletonResourceHandle      m_hSkeleton;
  xiiAnimationClipResourceHandle m_hAnimationClip;
  xiiAnimGraphResourceHandle     m_hAnimationGraph;

  xiiAnimGraphInstance m_AnimGraphInstance;
  xiiAnimationPose     m_CurrentPose;
  xiiTime              m_PlaybackTime;
  float                m_fPlaybackSpeed         = 1.0f;
  bool                 m_bApplyToOwnerMesh      = true;
  xiiUInt32            m_uiPoseSkeletonRevision = xiiInvalidIndex;
};

class XII_GRAPHICSCORE_DLL xiiSkeletonPoseComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonPoseComponent, xiiComponent, xiiSkeletonPoseComponentManager);

public:
  xiiSkeletonPoseComponent();
  ~xiiSkeletonPoseComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void                             SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const;                                     // [ property ]

  /// Replaces all overrides with the supplied pose in the current skeleton's joint order.
  /// Requires a loaded skeleton, matching joint count and valid transforms. Failure leaves the overrides unchanged.
  xiiResult SetLocalPose(xiiArrayPtr<const xiiTransform> localPose);

  /// Sparse absolute local transforms in runtime skeleton coordinates. Unspecified joints use their rest pose.
  /// Unknown names are retained across resource reloads but have no effect until a matching joint exists.
  xiiResult                              SetBoneTransform(xiiStringView sName, const xiiTransform& transform);
  void                                   RemoveBoneTransform(xiiStringView sName);
  const xiiMap<xiiString, xiiTransform>& GetBoneTransforms() const;

  void SetApplyToOwnerMesh(bool bApply);
  bool GetApplyToOwnerMesh() const;

  const xiiAnimationPose& GetCurrentPose() const;
  xiiResult               CopyCurrentLocalPose(xiiMap<xiiString, xiiTransform>& out_transforms) const;

  void Update();

protected:
  void ApplyPoseToSkinnedMesh();
  void ReflectionSetBoneTransform(xiiStringView sName, const xiiTransform& transform);

protected:
  xiiSkeletonResourceHandle       m_hSkeleton;
  xiiAnimationPose                m_CurrentPose;
  xiiMap<xiiString, xiiTransform> m_BoneTransforms;
  // Version 1 stored joint indices. Resolve them once when the skeleton becomes available.
  xiiDynamicArray<xiiTransform> m_LegacyLocalPose;
  xiiUInt32                     m_uiPoseSkeletonRevision = xiiInvalidIndex;
  bool                          m_bApplyToOwnerMesh      = true;
};
