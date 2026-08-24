/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
#  include <ozz/animation/runtime/animation.h>
#  include <ozz/animation/runtime/sampling_job.h>
#  include <ozz/animation/runtime/skeleton.h>
#  include <ozz/base/containers/vector.h>
#  include <ozz/base/io/archive.h>
#  include <ozz/base/io/stream.h>
#  include <ozz/base/maths/simd_math.h>
#  include <ozz/base/maths/soa_transform.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipResource, 1, xiiRTTIDefaultAllocator<xiiAnimationClipResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiAnimationClipResource);

namespace
{
  static constexpr xiiUInt32 s_uiAnimationClipResourceVersion = 1U;

  template <typename KeyType, typename ValueType>
  static ValueType SampleStepOrLinear(xiiArrayPtr<const KeyType> pKeys, float fTime, const ValueType& fallback)
  {
    if (pKeys.IsEmpty())
      return fallback;

    if (pKeys.GetCount() == 1U || fTime <= pKeys[0].m_fTime)
      return pKeys[0].m_Value;

    for (xiiUInt32 i = 1; i < pKeys.GetCount(); ++i)
    {
      if (fTime <= pKeys[i].m_fTime)
      {
        const float fRange = pKeys[i].m_fTime - pKeys[i - 1U].m_fTime;
        const float fT     = fRange > 0.0f ? xiiMath::Saturate((fTime - pKeys[i - 1U].m_fTime) / fRange) : 0.0f;
        return pKeys[i - 1U].m_Value + (pKeys[i].m_Value - pKeys[i - 1U].m_Value) * fT;
      }
    }

    return pKeys[pKeys.GetCount() - 1U].m_Value;
  }

  static xiiQuat SampleQuat(xiiArrayPtr<const xiiAnimationKeyQuat> pKeys, float fTime, const xiiQuat& fallback)
  {
    if (pKeys.IsEmpty())
      return fallback;

    if (pKeys.GetCount() == 1U || fTime <= pKeys[0].m_fTime)
      return pKeys[0].m_Value;

    for (xiiUInt32 i = 1; i < pKeys.GetCount(); ++i)
    {
      if (fTime <= pKeys[i].m_fTime)
      {
        const float fRange = pKeys[i].m_fTime - pKeys[i - 1U].m_fTime;
        const float fT     = fRange > 0.0f ? xiiMath::Saturate((fTime - pKeys[i - 1U].m_fTime) / fRange) : 0.0f;
        return xiiQuat::MakeSlerp(pKeys[i - 1U].m_Value, pKeys[i].m_Value, fT);
      }
    }

    return pKeys[pKeys.GetCount() - 1U].m_Value;
  }

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  static void StoreOzzSoaLocals(ozz::span<const ozz::math::SoaTransform> soaLocals, xiiUInt32 uiJointCount, xiiAnimationPose& ref_pose)
  {
    ref_pose.m_LocalTransforms.SetCount(uiJointCount);

    for (xiiUInt32 uiSoaIndex = 0; uiSoaIndex < soaLocals.size(); ++uiSoaIndex)
    {
      const ozz::math::SoaTransform& soaTransform = soaLocals[uiSoaIndex];

      float fTranslationX[4], fTranslationY[4], fTranslationZ[4];
      float fRotationX[4], fRotationY[4], fRotationZ[4], fRotationW[4];
      float fScaleX[4], fScaleY[4], fScaleZ[4];

      ozz::math::StorePtrU(soaTransform.translation.x, fTranslationX);
      ozz::math::StorePtrU(soaTransform.translation.y, fTranslationY);
      ozz::math::StorePtrU(soaTransform.translation.z, fTranslationZ);
      ozz::math::StorePtrU(soaTransform.rotation.x, fRotationX);
      ozz::math::StorePtrU(soaTransform.rotation.y, fRotationY);
      ozz::math::StorePtrU(soaTransform.rotation.z, fRotationZ);
      ozz::math::StorePtrU(soaTransform.rotation.w, fRotationW);
      ozz::math::StorePtrU(soaTransform.scale.x, fScaleX);
      ozz::math::StorePtrU(soaTransform.scale.y, fScaleY);
      ozz::math::StorePtrU(soaTransform.scale.z, fScaleZ);

      for (xiiUInt32 uiLane = 0; uiLane < 4U; ++uiLane)
      {
        const xiiUInt32 uiJointIndex = uiSoaIndex * 4U + uiLane;
        if (uiJointIndex >= uiJointCount)
          break;

        xiiTransform& transform = ref_pose.m_LocalTransforms[uiJointIndex];
        transform.m_vPosition   = xiiVec3(fTranslationX[uiLane], fTranslationY[uiLane], fTranslationZ[uiLane]);
        transform.m_qRotation   = xiiQuat::MakeFromElements(fRotationX[uiLane], fRotationY[uiLane], fRotationZ[uiLane], fRotationW[uiLane]);
        transform.m_vScale      = xiiVec3(fScaleX[uiLane], fScaleY[uiLane], fScaleZ[uiLane]);
      }
    }

    ref_pose.m_ModelTransforms.Clear();
    ref_pose.m_SkinningMatrices.Clear();
  }
#endif
} // namespace

xiiResult xiiAnimationKeyVec3::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_fTime;
  inout_stream << m_Value;
  return XII_SUCCESS;
}

xiiResult xiiAnimationKeyVec3::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_fTime;
  inout_stream >> m_Value;
  return XII_SUCCESS;
}

xiiResult xiiAnimationKeyQuat::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_fTime;
  inout_stream << m_Value;
  return XII_SUCCESS;
}

xiiResult xiiAnimationKeyQuat::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_fTime;
  inout_stream >> m_Value;
  return XII_SUCCESS;
}

xiiResult xiiAnimationClipJointTrack::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sJointName;
  inout_stream << m_uiJointIndex;
  inout_stream.WriteArray(m_PositionKeys).IgnoreResult();
  inout_stream.WriteArray(m_RotationKeys).IgnoreResult();
  inout_stream.WriteArray(m_ScaleKeys).IgnoreResult();
  return XII_SUCCESS;
}

xiiResult xiiAnimationClipJointTrack::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sJointName;
  inout_stream >> m_uiJointIndex;
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_PositionKeys));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_RotationKeys));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ScaleKeys));
  return XII_SUCCESS;
}

xiiResult xiiAnimationClipEvent::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_fTime;
  inout_stream << m_sEvent;
  return XII_SUCCESS;
}

xiiResult xiiAnimationClipEvent::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_fTime;
  inout_stream >> m_sEvent;
  return XII_SUCCESS;
}

void xiiAnimationClipResourceDescriptor::Clear()
{
  m_Duration          = xiiTime::MakeZero();
  m_fSampleRate       = 30.0f;
  m_bLooping          = true;
  m_bAdditive         = false;
  m_uiRootMotionJoint = xiiMath::MaxValue<xiiUInt16>();
  m_JointTracks.Clear();
  m_Events.Clear();
  m_OzzAnimationData.Clear();
  m_uiRuntimeHash = 0U;
}

xiiAnimationClipJointTrack& xiiAnimationClipResourceDescriptor::AddJointTrack(xiiStringView sJointName, xiiUInt16 uiJointIndex)
{
  xiiAnimationClipJointTrack& track = m_JointTracks.ExpandAndGetRef();
  track.m_sJointName.Assign(sJointName);
  track.m_uiJointIndex = uiJointIndex;
  return track;
}

void xiiAnimationClipResourceDescriptor::ComputeRuntimeHash()
{
  xiiHashStreamWriter32 hashWriter;
  hashWriter << m_Duration.GetSeconds();
  hashWriter << m_fSampleRate;
  hashWriter << m_bLooping;
  hashWriter << m_bAdditive;
  hashWriter << m_uiRootMotionJoint;
  hashWriter << m_JointTracks.GetCount();

  for (const xiiAnimationClipJointTrack& track : m_JointTracks)
  {
    hashWriter << track.m_sJointName.GetHash();
    hashWriter << track.m_uiJointIndex;
    hashWriter << track.m_PositionKeys.GetCount();
    hashWriter << track.m_RotationKeys.GetCount();
    hashWriter << track.m_ScaleKeys.GetCount();
  }

  hashWriter << m_OzzAnimationData.GetCount();
  if (!m_OzzAnimationData.IsEmpty())
  {
    hashWriter.WriteBytes(m_OzzAnimationData.GetData(), m_OzzAnimationData.GetCount()).IgnoreResult();
  }

  m_uiRuntimeHash = hashWriter.GetHashValue();
}

xiiResult xiiAnimationClipResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << s_uiAnimationClipResourceVersion;
  inout_stream << m_Duration;
  inout_stream << m_fSampleRate;
  inout_stream << m_bLooping;
  inout_stream << m_bAdditive;
  inout_stream << m_uiRootMotionJoint;
  inout_stream.WriteArray(m_JointTracks).IgnoreResult();
  inout_stream.WriteArray(m_Events).IgnoreResult();

  inout_stream << m_OzzAnimationData.GetCount();
  if (!m_OzzAnimationData.IsEmpty())
  {
    inout_stream.WriteBytes(m_OzzAnimationData.GetData(), m_OzzAnimationData.GetCount()).IgnoreResult();
  }

  inout_stream << m_uiRuntimeHash;
  return XII_SUCCESS;
}

xiiResult xiiAnimationClipResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  xiiUInt32 uiVersion = 0U;
  inout_stream >> uiVersion;
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Duration;
  inout_stream >> m_fSampleRate;
  inout_stream >> m_bLooping;
  inout_stream >> m_bAdditive;
  inout_stream >> m_uiRootMotionJoint;
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_JointTracks));
  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Events));

  xiiUInt32 uiOzzDataSize = 0U;
  inout_stream >> uiOzzDataSize;
  if (uiOzzDataSize > 0U)
  {
    m_OzzAnimationData.SetCount(uiOzzDataSize);

    inout_stream.ReadBytes(m_OzzAnimationData.GetData(), uiOzzDataSize);
  }

  inout_stream >> m_uiRuntimeHash;
  return XII_SUCCESS;
}

xiiAnimationClipResource::xiiAnimationClipResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiAnimationClipResource::~xiiAnimationClipResource() = default;

xiiTime xiiAnimationClipResource::GetDuration() const
{
  return m_Descriptor.m_Duration;
}

bool xiiAnimationClipResource::IsLooping() const
{
  return m_Descriptor.m_bLooping;
}

bool xiiAnimationClipResource::IsAdditive() const
{
  return m_Descriptor.m_bAdditive;
}

xiiUInt32 xiiAnimationClipResource::GetRuntimeHash() const
{
  return m_Descriptor.m_uiRuntimeHash;
}

const xiiAnimationClipResourceDescriptor& xiiAnimationClipResource::GetDescriptor() const
{
  return m_Descriptor;
}

void xiiAnimationClipResource::SampleLocalPose(const xiiSkeletonResource& skeleton, xiiTime sampleTime, xiiAnimationPose& ref_pose) const
{
  ref_pose.ResetToRestPose(skeleton);

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  const ozz::animation::Animation* pOzzAnimation = GetOzzAnimation();
  const ozz::animation::Skeleton*  pOzzSkeleton  = skeleton.GetOzzSkeleton();
  if (pOzzAnimation != nullptr && pOzzSkeleton != nullptr && pOzzAnimation->num_tracks() <= pOzzSkeleton->num_joints() && static_cast<xiiUInt32>(pOzzSkeleton->num_joints()) <= skeleton.GetJointCount())
  {
    const float fOzzDuration = pOzzAnimation->duration();
    if (fOzzDuration > 0.0f)
    {
      float fSampleSeconds = sampleTime.AsFloatInSeconds();
      if (m_Descriptor.m_bLooping)
      {
        fSampleSeconds = xiiMath::Mod(fSampleSeconds, fOzzDuration);
        if (fSampleSeconds < 0.0f)
          fSampleSeconds += fOzzDuration;
      }
      else
      {
        fSampleSeconds = xiiMath::Clamp(fSampleSeconds, 0.0f, fOzzDuration);
      }

      ozz::vector<ozz::math::SoaTransform>           localTransforms(static_cast<size_t>(pOzzSkeleton->num_soa_joints()));
      const ozz::span<const ozz::math::SoaTransform> restPose = pOzzSkeleton->joint_rest_poses();
      for (size_t i = 0; i < localTransforms.size(); ++i)
      {
        localTransforms[i] = restPose[i];
      }

      ozz::animation::SamplingJob::Context context(pOzzAnimation->num_tracks());
      ozz::animation::SamplingJob          samplingJob;
      samplingJob.animation = pOzzAnimation;
      samplingJob.context   = &context;
      samplingJob.ratio     = xiiMath::Saturate(fSampleSeconds / fOzzDuration);
      samplingJob.output    = ozz::span<ozz::math::SoaTransform>(localTransforms.data(), localTransforms.size());

      if (samplingJob.Run())
      {
        StoreOzzSoaLocals(ozz::span<const ozz::math::SoaTransform>(localTransforms.data(), localTransforms.size()), skeleton.GetJointCount(), ref_pose);
        ref_pose.BuildModelSpacePose(skeleton);
        ref_pose.BuildSkinningMatrices(skeleton);
        return;
      }
    }
  }
#endif

  const double fDuration = m_Descriptor.m_Duration.GetSeconds();
  float        fTime     = sampleTime.AsFloatInSeconds();
  if (fDuration > 0.0)
  {
    fTime = m_Descriptor.m_bLooping ? static_cast<float>(xiiMath::Mod(sampleTime.GetSeconds(), fDuration)) : xiiMath::Clamp(fTime, 0.0f, static_cast<float>(fDuration));
  }

  for (const xiiAnimationClipJointTrack& track : m_Descriptor.m_JointTracks)
  {
    xiiUInt16 uiJointIndex = track.m_uiJointIndex;
    if (uiJointIndex == xiiMath::MaxValue<xiiUInt16>())
    {
      uiJointIndex = skeleton.FindJointByName(track.m_sJointName);
    }

    if (uiJointIndex >= ref_pose.m_LocalTransforms.GetCount())
      continue;

    xiiTransform& localTransform = ref_pose.m_LocalTransforms[uiJointIndex];
    localTransform.m_vPosition   = SampleStepOrLinear<xiiAnimationKeyVec3, xiiVec3>(track.m_PositionKeys, fTime, localTransform.m_vPosition);
    localTransform.m_qRotation   = SampleQuat(track.m_RotationKeys, fTime, localTransform.m_qRotation);
    localTransform.m_vScale      = SampleStepOrLinear<xiiAnimationKeyVec3, xiiVec3>(track.m_ScaleKeys, fTime, localTransform.m_vScale);
  }

  ref_pose.BuildModelSpacePose(skeleton);
  ref_pose.BuildSkinningMatrices(skeleton);
}

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
const ozz::animation::Animation* xiiAnimationClipResource::GetOzzAnimation() const
{
  return m_pOzzAnimation.Borrow();
}
#endif

xiiResourceLoadDescription xiiAnimationClipResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);

  m_Descriptor.Clear();
#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  m_pOzzAnimation.Clear();
#endif

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

xiiResourceLoadDescription xiiAnimationClipResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiAnimationClipResourceDescriptor descriptor;
  if (descriptor.Deserialize(*pStream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(descriptor));
  return res;
}

void xiiAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiAnimationClipResource) + static_cast<xiiUInt32>(m_Descriptor.m_JointTracks.GetHeapMemoryUsage() + m_Descriptor.m_Events.GetHeapMemoryUsage() + m_Descriptor.m_OzzAnimationData.GetHeapMemoryUsage());
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

void xiiAnimationClipResource::LoadOzzAnimation(const xiiDynamicArray<xiiUInt8>& ozzData)
{
#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  m_pOzzAnimation.Clear();

  if (ozzData.IsEmpty())
    return;

  ozz::io::MemoryStream stream;
  stream.Write(ozzData.GetData(), ozzData.GetCount());
  stream.Seek(0, ozz::io::Stream::kSet);

  xiiUniquePtr<ozz::animation::Animation> pAnimation = XII_DEFAULT_NEW(ozz::animation::Animation);
  ozz::io::IArchive                       archive(&stream);
  archive >> *pAnimation.Borrow();

  m_pOzzAnimation = std::move(pAnimation);
  if (m_Descriptor.m_Duration.IsZeroOrNegative() && m_pOzzAnimation->duration() > 0.0f)
  {
    m_Descriptor.m_Duration = xiiTime::MakeFromSeconds(m_pOzzAnimation->duration());
    m_Descriptor.ComputeRuntimeHash();
  }
#else
  XII_IGNORE_UNUSED(ozzData);
#endif
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiAnimationClipResource, xiiAnimationClipResourceDescriptor)
{
  descriptor.ComputeRuntimeHash();

  m_Descriptor = std::move(descriptor);
  LoadOzzAnimation(m_Descriptor.m_OzzAnimationData);

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_AnimationClipResource);
