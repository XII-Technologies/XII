/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/ComponentManager.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiSkeletonComponentManager = xiiComponentManagerSimple<class xiiSkeletonComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::Compact, xiiWorldUpdatePhase::PostTransform>;
using xiiSkeletonPoseComponentManager = xiiComponentManagerSimple<class xiiSkeletonPoseComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::Compact, xiiWorldUpdatePhase::PostTransform>;

class XII_GRAPHICSCORE_DLL xiiSkeletonComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonComponent, xiiComponent, xiiSkeletonComponentManager);

public:
  xiiSkeletonComponent();
  ~xiiSkeletonComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const;         // [ property ]

  void SetAnimationClip(const xiiAnimationClipResourceHandle& hClip); // [ property ]
  const xiiAnimationClipResourceHandle& GetAnimationClip() const;     // [ property ]

  void SetAnimationGraph(const xiiAnimGraphResourceHandle& hGraph); // [ property ]
  const xiiAnimGraphResourceHandle& GetAnimationGraph() const;      // [ property ]

  void  SetPlaybackSpeed(float fSpeed); // [ property ]
  float GetPlaybackSpeed() const;       // [ property ]

  void SetApplyToOwnerMesh(bool bApply); // [ property ]
  bool GetApplyToOwnerMesh() const;      // [ property ]

  void SetFloatParameter(xiiStringView sName, float fValue);
  void SetBoolParameter(xiiStringView sName, bool bValue);

  const xiiAnimationPose& GetCurrentPose() const;
  void Update();

protected:
  void ApplyPoseToSkinnedMesh();

protected:
  xiiSkeletonResourceHandle      m_hSkeleton;
  xiiAnimationClipResourceHandle m_hAnimationClip;
  xiiAnimGraphResourceHandle     m_hAnimationGraph;

  xiiAnimGraphInstance m_AnimGraphInstance;
  xiiAnimationPose     m_CurrentPose;
  xiiTime              m_PlaybackTime;
  float                m_fPlaybackSpeed = 1.0f;
  bool                 m_bApplyToOwnerMesh = true;
};

class XII_GRAPHICSCORE_DLL xiiSkeletonPoseComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonPoseComponent, xiiComponent, xiiSkeletonPoseComponentManager);

public:
  xiiSkeletonPoseComponent();
  ~xiiSkeletonPoseComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const;         // [ property ]

  void SetLocalPose(xiiArrayPtr<const xiiTransform> localPose);
  const xiiAnimationPose& GetCurrentPose() const;

  void Update();

protected:
  void ApplyPoseToSkinnedMesh();

protected:
  xiiSkeletonResourceHandle m_hSkeleton;
  xiiAnimationPose          m_CurrentPose;
  bool                      m_bApplyToOwnerMesh = true;
};
