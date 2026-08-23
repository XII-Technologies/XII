/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Time/Time.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

struct XII_GRAPHICSCORE_DLL xiiAnimationKeyVec3
{
  float   m_fTime = 0.0f;
  xiiVec3 m_Value = xiiVec3::MakeZero();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimationKeyQuat
{
  float   m_fTime = 0.0f;
  xiiQuat m_Value = xiiQuat::MakeIdentity();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimationClipJointTrack
{
  xiiHashedString m_sJointName;
  xiiUInt16       m_uiJointIndex = xiiMath::MaxValue<xiiUInt16>();

  xiiHybridArray<xiiAnimationKeyVec3, 4> m_PositionKeys;
  xiiHybridArray<xiiAnimationKeyQuat, 4> m_RotationKeys;
  xiiHybridArray<xiiAnimationKeyVec3, 4> m_ScaleKeys;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimationClipEvent
{
  float           m_fTime = 0.0f;
  xiiHashedString m_sEvent;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimationClipResourceDescriptor
{
  void Clear();
  void ComputeRuntimeHash();

  xiiAnimationClipJointTrack& AddJointTrack(xiiStringView sJointName, xiiUInt16 uiJointIndex = xiiMath::MaxValue<xiiUInt16>());

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiTime   m_Duration          = xiiTime::MakeZero();
  float     m_fSampleRate       = 30.0f;
  bool      m_bLooping          = true;
  bool      m_bAdditive         = false;
  xiiUInt16 m_uiRootMotionJoint = xiiMath::MaxValue<xiiUInt16>();

  xiiDynamicArray<xiiAnimationClipJointTrack> m_JointTracks;
  xiiDynamicArray<xiiAnimationClipEvent>      m_Events;
  xiiDynamicArray<xiiUInt8>                   m_OzzAnimationData;
  xiiUInt32                                   m_uiRuntimeHash = 0U;
};

class XII_GRAPHICSCORE_DLL xiiAnimationClipResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiAnimationClipResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiAnimationClipResource, xiiAnimationClipResourceDescriptor);

public:
  xiiAnimationClipResource();
  ~xiiAnimationClipResource();

  xiiTime                                   GetDuration() const;
  bool                                      IsLooping() const;
  bool                                      IsAdditive() const;
  xiiUInt32                                 GetRuntimeHash() const;
  const xiiAnimationClipResourceDescriptor& GetDescriptor() const;

  void SampleLocalPose(const xiiSkeletonResource& skeleton, xiiTime sampleTime, xiiAnimationPose& ref_pose) const;

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  const ozz::animation::Animation* GetOzzAnimation() const;
#endif

private:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void LoadOzzAnimation(const xiiDynamicArray<xiiUInt8>& ozzData);

private:
  xiiAnimationClipResourceDescriptor m_Descriptor;

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  xiiUniquePtr<ozz::animation::Animation> m_pOzzAnimation;
#endif
};
