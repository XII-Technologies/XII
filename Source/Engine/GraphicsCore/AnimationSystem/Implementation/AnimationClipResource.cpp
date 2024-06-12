#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipResource, 1, xiiRTTIDefaultAllocator<xiiAnimationClipResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiAnimationClipResource);
// clang-format on

xiiAnimationClipResource::xiiAnimationClipResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiAnimationClipResource, xiiAnimationClipResourceDescriptor)
{
  m_pDescriptor  = XII_DEFAULT_NEW(xiiAnimationClipResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDesc xiiAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiAnimationClipResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiAnimationClipResource::UpdateContent", GetResourceDescription().GetData());

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = XII_DEFAULT_NEW(xiiAnimationClipResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiAnimationClipResource);

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<xiiUInt32>(m_pDescriptor->GetHeapMemoryUsage());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct xiiAnimationClipResourceDescriptor::OzzImpl
{
  struct CachedAnim
  {
    xiiUInt32                                  m_uiResourceChangeCounter = 0;
    ozz::unique_ptr<ozz::animation::Animation> m_pAnim;
  };

  xiiMap<const xiiSkeletonResource*, CachedAnim> m_MappedOzzAnimations;
};

xiiAnimationClipResourceDescriptor::xiiAnimationClipResourceDescriptor()
{
  m_pOzzImpl = XII_DEFAULT_NEW(OzzImpl);
}

xiiAnimationClipResourceDescriptor::xiiAnimationClipResourceDescriptor(xiiAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

xiiAnimationClipResourceDescriptor::~xiiAnimationClipResourceDescriptor() = default;

void xiiAnimationClipResourceDescriptor::operator=(xiiAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_pOzzImpl = std::move(rhs.m_pOzzImpl);

  m_JointInfos          = std::move(rhs.m_JointInfos);
  m_Transforms          = std::move(rhs.m_Transforms);
  m_uiNumTotalPositions = rhs.m_uiNumTotalPositions;
  m_uiNumTotalRotations = rhs.m_uiNumTotalRotations;
  m_uiNumTotalScales    = rhs.m_uiNumTotalScales;
  m_Duration            = rhs.m_Duration;
}

xiiResult xiiAnimationClipResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(9);

  const xiiUInt16 uiNumJoints = static_cast<xiiUInt16>(m_JointInfos.GetCount());
  inout_stream << uiNumJoints;
  for (xiiUInt32 i = 0; i < m_JointInfos.GetCount(); ++i)
  {
    const auto& val = m_JointInfos.GetValue(i);

    inout_stream << m_JointInfos.GetKey(i);
    inout_stream << val.m_uiPositionIdx;
    inout_stream << val.m_uiPositionCount;
    inout_stream << val.m_uiRotationIdx;
    inout_stream << val.m_uiRotationCount;
    inout_stream << val.m_uiScaleIdx;
    inout_stream << val.m_uiScaleCount;
  }

  inout_stream << m_Duration;
  inout_stream << m_uiNumTotalPositions;
  inout_stream << m_uiNumTotalRotations;
  inout_stream << m_uiNumTotalScales;

  XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Transforms));

  inout_stream << m_vConstantRootMotion;

  m_EventTrack.Save(inout_stream);

  inout_stream << m_bAdditive;

  return XII_SUCCESS;
}

xiiResult xiiAnimationClipResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiTypeVersion uiVersion = inout_stream.ReadVersion(9);

  if (uiVersion < 6)
    return XII_FAILURE;

  xiiUInt16 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_JointInfos.Reserve(uiNumJoints);

  xiiHashedString hs;

  for (xiiUInt16 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream >> hs;

    JointInfo ji;
    inout_stream >> ji.m_uiPositionIdx;
    inout_stream >> ji.m_uiPositionCount;
    inout_stream >> ji.m_uiRotationIdx;
    inout_stream >> ji.m_uiRotationCount;
    inout_stream >> ji.m_uiScaleIdx;
    inout_stream >> ji.m_uiScaleCount;

    m_JointInfos.Insert(hs, ji);
  }

  m_JointInfos.Sort();

  inout_stream >> m_Duration;
  inout_stream >> m_uiNumTotalPositions;
  inout_stream >> m_uiNumTotalRotations;
  inout_stream >> m_uiNumTotalScales;

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Transforms));

  if (uiVersion >= 7)
  {
    inout_stream >> m_vConstantRootMotion;
  }

  if (uiVersion >= 8)
  {
    m_EventTrack.Load(inout_stream);
  }

  if (uiVersion >= 9)
  {
    inout_stream >> m_bAdditive;
  }

  return XII_SUCCESS;
}

xiiUInt64 xiiAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Transforms.GetHeapMemoryUsage() + m_JointInfos.GetHeapMemoryUsage() + m_pOzzImpl->m_MappedOzzAnimations.GetHeapMemoryUsage();
}

xiiUInt16 xiiAnimationClipResourceDescriptor::GetNumJoints() const
{
  return static_cast<xiiUInt16>(m_JointInfos.GetCount());
}

xiiTime xiiAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}

void xiiAnimationClipResourceDescriptor::SetDuration(xiiTime duration)
{
  m_Duration = duration;
}

XII_FORCE_INLINE void xii2ozz(const xiiVec3& vIn, ozz::math::Float3& ref_out)
{
  ref_out.x = vIn.x;
  ref_out.y = vIn.y;
  ref_out.z = vIn.z;
}

XII_FORCE_INLINE void xii2ozz(const xiiQuat& qIn, ozz::math::Quaternion& ref_out)
{
  ref_out.x = qIn.x;
  ref_out.y = qIn.y;
  ref_out.z = qIn.z;
  ref_out.w = qIn.w;
}

const ozz::animation::Animation& xiiAnimationClipResourceDescriptor::GetMappedOzzAnimation(const xiiSkeletonResource& skeleton) const
{
  auto it = m_pOzzImpl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
  {
    if (it.Value().m_uiResourceChangeCounter == skeleton.GetCurrentResourceChangeCounter())
    {
      return *it.Value().m_pAnim.get();
    }
  }

  auto            pOzzSkeleton = &skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton();
  const xiiUInt32 uiNumJoints  = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = xiiMath::Max(1.0f / 60.0f, m_Duration.AsFloatInSeconds());
  rawAnim.tracks.resize(uiNumJoints);

  for (xiiUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];

    const xiiTempHashedString sJointName = xiiTempHashedString(pOzzSkeleton->joint_names()[j]);

    const JointInfo* pJointInfo = GetJointInfo(sJointName);

    if (pJointInfo == nullptr)
    {
      dstTrack.translations.resize(1);
      dstTrack.rotations.resize(1);
      dstTrack.scales.resize(1);

      const xiiUInt16 uiFallbackIdx = skeleton.GetDescriptor().m_Skeleton.FindJointByName(sJointName);

      XII_ASSERT_DEV(uiFallbackIdx != xiiInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetDescriptor().m_Skeleton.GetJointByIndex(uiFallbackIdx);

      const xiiTransform& fallbackTransform = fallbackJoint.GetRestPoseLocalTransform();

      auto& dstT = dstTrack.translations[0];
      auto& dstR = dstTrack.rotations[0];
      auto& dstS = dstTrack.scales[0];

      dstT.time = 0.0f;
      dstR.time = 0.0f;
      dstS.time = 0.0f;

      xii2ozz(fallbackTransform.m_vPosition, dstT.value);
      xii2ozz(fallbackTransform.m_qRotation, dstR.value);
      xii2ozz(fallbackTransform.m_vScale, dstS.value);
    }
    else
    {
      // positions
      {
        dstTrack.translations.resize(pJointInfo->m_uiPositionCount);
        const xiiArrayPtr<const KeyframeVec3> keyframes = GetPositionKeyframes(*pJointInfo);

        for (xiiUInt32 i = 0; i < pJointInfo->m_uiPositionCount; ++i)
        {
          auto& dst = dstTrack.translations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          xii2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // rotations
      {
        dstTrack.rotations.resize(pJointInfo->m_uiRotationCount);
        const xiiArrayPtr<const KeyframeQuat> keyframes = GetRotationKeyframes(*pJointInfo);

        for (xiiUInt32 i = 0; i < pJointInfo->m_uiRotationCount; ++i)
        {
          auto& dst = dstTrack.rotations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          xii2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // scales
      {
        dstTrack.scales.resize(pJointInfo->m_uiScaleCount);
        const xiiArrayPtr<const KeyframeVec3> keyframes = GetScaleKeyframes(*pJointInfo);

        for (xiiUInt32 i = 0; i < pJointInfo->m_uiScaleCount; ++i)
        {
          auto& dst = dstTrack.scales[i];

          dst.time = keyframes[i].m_fTimeInSec;
          xii2ozz(keyframes[i].m_Value, dst.value);
        }
      }
    }
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  XII_ASSERT_DEBUG(rawAnim.Validate(), "Invalid animation data");

  auto& cached                     = m_pOzzImpl->m_MappedOzzAnimations[&skeleton];
  cached.m_pAnim                   = std::move(animBuilder(rawAnim));
  cached.m_uiResourceChangeCounter = skeleton.GetCurrentResourceChangeCounter();

  return *cached.m_pAnim.get();
}

xiiAnimationClipResourceDescriptor::JointInfo xiiAnimationClipResourceDescriptor::CreateJoint(const xiiHashedString& sJointName, xiiUInt16 uiNumPositions, xiiUInt16 uiNumRotations, xiiUInt16 uiNumScales)
{
  JointInfo ji;
  ji.m_uiPositionIdx = m_uiNumTotalPositions;
  ji.m_uiRotationIdx = m_uiNumTotalRotations;
  ji.m_uiScaleIdx    = m_uiNumTotalScales;

  ji.m_uiPositionCount = uiNumPositions;
  ji.m_uiRotationCount = uiNumRotations;
  ji.m_uiScaleCount    = uiNumScales;

  m_uiNumTotalPositions += uiNumPositions;
  m_uiNumTotalRotations += uiNumRotations;
  m_uiNumTotalScales += uiNumScales;

  m_JointInfos.Insert(sJointName, ji);

  return ji;
}

const xiiAnimationClipResourceDescriptor::JointInfo* xiiAnimationClipResourceDescriptor::GetJointInfo(const xiiTempHashedString& sJointName) const
{
  xiiUInt32 uiIndex = m_JointInfos.Find(sJointName);

  if (uiIndex == xiiInvalidIndex)
    return nullptr;

  return &m_JointInfos.GetValue(uiIndex);
}

void xiiAnimationClipResourceDescriptor::AllocateJointTransforms()
{
  const xiiUInt32 uiNumBytes = m_uiNumTotalPositions * sizeof(KeyframeVec3) + m_uiNumTotalRotations * sizeof(KeyframeQuat) + m_uiNumTotalScales * sizeof(KeyframeVec3);

  m_Transforms.SetCountUninitialized(uiNumBytes);
}

xiiArrayPtr<xiiAnimationClipResourceDescriptor::KeyframeVec3> xiiAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo)
{
  XII_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return xiiArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

xiiArrayPtr<xiiAnimationClipResourceDescriptor::KeyframeQuat> xiiAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo)
{
  XII_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return xiiArrayPtr<KeyframeQuat>(reinterpret_cast<KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

xiiArrayPtr<xiiAnimationClipResourceDescriptor::KeyframeVec3> xiiAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo)
{
  XII_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return xiiArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

xiiArrayPtr<const xiiAnimationClipResourceDescriptor::KeyframeVec3> xiiAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo) const
{
  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return xiiArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

xiiArrayPtr<const xiiAnimationClipResourceDescriptor::KeyframeQuat> xiiAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo) const
{
  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return xiiArrayPtr<const KeyframeQuat>(reinterpret_cast<const KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

xiiArrayPtr<const xiiAnimationClipResourceDescriptor::KeyframeVec3> xiiAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo) const
{
  xiiUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return xiiArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

// bool xiiAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(xiiTempHashedString("xiiRootMotionTransform"));
//}
//
// xiiUInt16 xiiAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  xiiUInt16 jointIdx = 0;
//
//#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
//
//  const xiiUInt32 idx = m_JointNameToIndex.Find("xiiRootMotionTransform");
//  XII_ASSERT_DEBUG(idx != xiiInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  XII_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
//#endif
//
//  return jointIdx;
//}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_AnimationClipResource);
