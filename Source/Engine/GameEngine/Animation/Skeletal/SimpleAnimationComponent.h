#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

class xiiEventTrack;
struct xiiMsgAnimationReachedEnd;
struct xiiMsgGenericEvent;

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;
using xiiSkeletonResourceHandle      = xiiTypedResourceHandle<class xiiSkeletonResource>;

using xiiSimpleAnimationComponentManager = xiiComponentManagerSimple<class xiiSimpleAnimationComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::FreeList>;

class XII_GAMEENGINE_DLL xiiSimpleAnimationComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSimpleAnimationComponent, xiiComponent, xiiSimpleAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJointAttachmentComponent

public:
  xiiSimpleAnimationComponent();
  ~xiiSimpleAnimationComponent();

  void                                  SetAnimationClip(const xiiAnimationClipResourceHandle& hResource);
  const xiiAnimationClipResourceHandle& GetAnimationClip() const;

  void        SetAnimationClipFile(const char* szFile); // [ property ]
  const char* GetAnimationClipFile() const;             // [ property ]

  xiiEnum<xiiPropertyAnimMode> m_AnimationMode; // [ property ]
  float                        m_fSpeed = 1.0f; // [ property ]

  void  SetNormalizedPlaybackPosition(float fPosition);
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

protected:
  void Update();
  bool UpdatePlaybackTime(xiiTime tDiff, const xiiEventTrack& eventTrack, xiiAnimPoseEventTrackSampleMode& out_trackSampling);

  xiiEnum<xiiRootMotionMode>     m_RootMotionMode;
  float                          m_fNormalizedPlaybackPosition = 0.0f;
  xiiTime                        m_Duration;
  xiiAnimationClipResourceHandle m_hAnimationClip;
  xiiSkeletonResourceHandle      m_hSkeleton;

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};
