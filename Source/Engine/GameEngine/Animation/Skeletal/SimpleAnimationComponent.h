#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

class xiiEventTrack;
struct xiiMsgGenericEvent;

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;
using xiiSkeletonResourceHandle      = xiiTypedResourceHandle<class xiiSkeletonResource>;

using xiiSimpleAnimationComponentManager = xiiComponentManagerSimple<class xiiSimpleAnimationComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::FreeList>;

/// \brief Plays a single animation clip on an animated mesh.
///
/// \see xiiAnimatedMeshComponent
class XII_GAMEENGINE_DLL xiiSimpleAnimationComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSimpleAnimationComponent, xiiComponent, xiiSimpleAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJointAttachmentComponent

public:
  xiiSimpleAnimationComponent();
  ~xiiSimpleAnimationComponent();

  xiiAnimationClipResourceHandle m_hAnimationClip;

  // adds SetAnimationClipFile() and GetAnimationClipFile() for convenience
  XII_ADD_RESOURCEHANDLE_ACCESSORS(AnimationClip, m_hAnimationClip);

  /// \brief How to play the animation.
  xiiEnum<xiiPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How quickly or slowly to play the animation.
  float m_fSpeed = 1.0f; // [ property ]

  /// \brief Sets the current sample position of the animation clip in 0 (start) to 1 (end) range.
  void SetNormalizedPlaybackPosition(float fPosition);

  /// \brief Returns the normalized [0;1] sample position of the animation clip.
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

  /// \brief How often to update the animation while the animated mesh is invisible.
  xiiEnum<xiiAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();
  bool UpdatePlaybackTime(xiiTime tDiff, const xiiEventTrack& eventTrack, xiiAnimPoseEventTrackSampleMode& out_trackSampling);

  xiiEnum<xiiRootMotionMode> m_RootMotionMode;
  float                      m_fNormalizedPlaybackPosition = 0.0f;
  xiiTime                    m_Duration;
  xiiSkeletonResourceHandle  m_hSkeleton;
  xiiTime                    m_ElapsedTimeSinceUpdate = xiiTime::MakeZero();
  bool                       m_bEnableIK              = false;

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};
