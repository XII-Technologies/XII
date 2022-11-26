#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Tracks/EventTrack.h>

class xiiSkeletonResource;

namespace ozz::animation
{
  class Animation;
}

struct XII_RENDERERCORE_DLL xiiAnimationClipResourceDescriptor
{
public:
  xiiAnimationClipResourceDescriptor();
  xiiAnimationClipResourceDescriptor(xiiAnimationClipResourceDescriptor&& rhs);
  ~xiiAnimationClipResourceDescriptor();

  void operator=(xiiAnimationClipResourceDescriptor&& rhs) noexcept;

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);

  xiiUInt64 GetHeapMemoryUsage() const;

  xiiUInt16 GetNumJoints() const;
  xiiTime   GetDuration() const;
  void      SetDuration(xiiTime duration);

  const ozz::animation::Animation& GetMappedOzzAnimation(const xiiSkeletonResource& skeleton) const;

  struct JointInfo
  {
    xiiUInt32 m_uiPositionIdx   = 0;
    xiiUInt32 m_uiRotationIdx   = 0;
    xiiUInt32 m_uiScaleIdx      = 0;
    xiiUInt16 m_uiPositionCount = 0;
    xiiUInt16 m_uiRotationCount = 0;
    xiiUInt16 m_uiScaleCount    = 0;
  };

  struct KeyframeVec3
  {
    float   m_fTimeInSec;
    xiiVec3 m_Value;
  };

  struct KeyframeQuat
  {
    float   m_fTimeInSec;
    xiiQuat m_Value;
  };

  JointInfo        CreateJoint(const xiiHashedString& sJointName, xiiUInt16 uiNumPositions, xiiUInt16 uiNumRotations, xiiUInt16 uiNumScales);
  const JointInfo* GetJointInfo(const xiiTempHashedString& sJointName) const;
  void             AllocateJointTransforms();

  xiiArrayPtr<KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo);
  xiiArrayPtr<KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo);
  xiiArrayPtr<KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo);

  xiiArrayPtr<const KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo) const;
  xiiArrayPtr<const KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo) const;
  xiiArrayPtr<const KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo) const;

  xiiVec3 m_vConstantRootMotion = xiiVec3::ZeroVector();

  xiiEventTrack m_EventTrack;

  bool m_bAdditive = false;

private:
  xiiArrayMap<xiiHashedString, JointInfo> m_JointInfos;
  xiiDataBuffer                           m_Transforms;
  xiiUInt32                               m_uiNumTotalPositions = 0;
  xiiUInt32                               m_uiNumTotalRotations = 0;
  xiiUInt32                               m_uiNumTotalScales    = 0;
  xiiTime                                 m_Duration;

  struct OzzImpl;
  xiiUniquePtr<OzzImpl> m_pOzzImpl;
};

//////////////////////////////////////////////////////////////////////////

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;

class XII_RENDERERCORE_DLL xiiAnimationClipResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiAnimationClipResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiAnimationClipResource, xiiAnimationClipResourceDescriptor);

public:
  xiiAnimationClipResource();

  const xiiAnimationClipResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUniquePtr<xiiAnimationClipResourceDescriptor> m_pDescriptor;
};
