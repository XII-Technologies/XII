/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Algorithm/HashStream.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
#  include <ozz/animation/runtime/skeleton.h>
#  include <ozz/base/io/archive.h>
#  include <ozz/base/io/stream.h>
#  include <ozz/base/maths/simd_math.h>
#  include <ozz/base/maths/soa_transform.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonResource, 1, xiiRTTIDefaultAllocator<xiiSkeletonResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiSkeletonResource);

namespace
{
  static constexpr xiiUInt32 s_uiSkeletonResourceVersion = 1U;

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  static xiiTransform GetOzzRestPoseTransform(ozz::span<const ozz::math::SoaTransform> restPose, xiiUInt32 uiJointIndex)
  {
    const ozz::math::SoaTransform& soaTransform = restPose[uiJointIndex / 4U];
    const xiiUInt32                uiLane       = uiJointIndex & 3U;

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

    xiiTransform transform;
    transform.m_vPosition = xiiVec3(fTranslationX[uiLane], fTranslationY[uiLane], fTranslationZ[uiLane]);
    transform.m_qRotation = xiiQuat::MakeFromElements(fRotationX[uiLane], fRotationY[uiLane], fRotationZ[uiLane], fRotationW[uiLane]);
    transform.m_vScale    = xiiVec3(fScaleX[uiLane], fScaleY[uiLane], fScaleZ[uiLane]);
    return transform;
  }
#endif
} // namespace

xiiResult xiiSkeletonJoint::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_uiParentIndex;
  inout_stream << m_LocalRestPose;
  inout_stream << m_ModelRestPose;
  inout_stream << m_InverseBindPose;
  inout_stream << m_LocalBounds;
  return XII_SUCCESS;
}

xiiResult xiiSkeletonJoint::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_uiParentIndex;
  inout_stream >> m_LocalRestPose;
  inout_stream >> m_ModelRestPose;
  inout_stream >> m_InverseBindPose;
  inout_stream >> m_LocalBounds;
  return XII_SUCCESS;
}

void xiiSkeletonResourceDescriptor::Clear()
{
  m_Joints.Clear();
  m_Bounds        = xiiBoundingBoxSphere::MakeInvalid();
  m_uiRootJoint   = xiiMath::MaxValue<xiiUInt16>();
  m_uiRuntimeHash = 0U;
  m_OzzSkeletonData.Clear();
}

xiiUInt16 xiiSkeletonResourceDescriptor::AddJoint(xiiStringView sName, xiiUInt16 uiParentIndex, const xiiTransform& localRestPose)
{
  XII_ASSERT_DEV(m_Joints.GetCount() < xiiMath::MaxValue<xiiUInt16>(), "Skeletons support up to 65535 joints in the engine descriptor.");

  xiiSkeletonJoint& joint = m_Joints.ExpandAndGetRef();
  joint.m_sName.Assign(sName);
  joint.m_uiParentIndex = uiParentIndex;
  joint.m_LocalRestPose = localRestPose;

  const xiiUInt16 uiJointIndex = static_cast<xiiUInt16>(m_Joints.GetCount() - 1U);
  if (uiParentIndex == xiiMath::MaxValue<xiiUInt16>())
  {
    m_uiRootJoint = uiJointIndex;
  }

  return uiJointIndex;
}

xiiUInt16 xiiSkeletonResourceDescriptor::FindJointByName(const xiiTempHashedString& sName) const
{
  for (xiiUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].m_sName == sName)
      return static_cast<xiiUInt16>(i);
  }

  return xiiMath::MaxValue<xiiUInt16>();
}

void xiiSkeletonResourceDescriptor::BuildModelSpaceRestPose()
{
  xiiHybridArray<xiiVec3, 128> jointPositions;
  jointPositions.Reserve(m_Joints.GetCount());

  for (xiiUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    xiiSkeletonJoint& joint       = m_Joints[i];
    const xiiMat4     localMatrix = joint.m_LocalRestPose.GetAsMat4();

    if (joint.m_uiParentIndex < i)
    {
      joint.m_ModelRestPose = m_Joints[joint.m_uiParentIndex].m_ModelRestPose * localMatrix;
    }
    else
    {
      joint.m_ModelRestPose = localMatrix;
      if (m_uiRootJoint == xiiMath::MaxValue<xiiUInt16>())
      {
        m_uiRootJoint = static_cast<xiiUInt16>(i);
      }
    }

    joint.m_InverseBindPose = joint.m_ModelRestPose.GetInverse();
    jointPositions.PushBack(joint.m_ModelRestPose.GetTranslationVector());
  }

  m_Bounds = jointPositions.IsEmpty() ? xiiBoundingBoxSphere::MakeZero() : xiiBoundingBoxSphere::MakeFromPoints(jointPositions.GetData(), jointPositions.GetCount());
  ComputeRuntimeHash();
}

void xiiSkeletonResourceDescriptor::ComputeRuntimeHash()
{
  xiiHashStreamWriter32 hashWriter;
  hashWriter << m_Joints.GetCount();
  hashWriter << m_uiRootJoint;
  hashWriter << m_Bounds.m_vCenter;
  hashWriter << m_Bounds.m_fSphereRadius;

  for (const xiiSkeletonJoint& joint : m_Joints)
  {
    hashWriter << joint.m_sName.GetHash();
    hashWriter << joint.m_uiParentIndex;
    hashWriter << joint.m_LocalRestPose.m_vPosition;
    hashWriter << joint.m_LocalRestPose.m_qRotation;
    hashWriter << joint.m_LocalRestPose.m_vScale;
  }

  hashWriter << m_OzzSkeletonData.GetCount();
  if (!m_OzzSkeletonData.IsEmpty())
  {
    hashWriter.WriteBytes(m_OzzSkeletonData.GetData(), m_OzzSkeletonData.GetCount()).IgnoreResult();
  }

  m_uiRuntimeHash = hashWriter.GetHashValue();
}

xiiResult xiiSkeletonResourceDescriptor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << s_uiSkeletonResourceVersion;

  inout_stream.WriteArray(m_Joints).IgnoreResult();

  inout_stream << m_Bounds;
  inout_stream << m_uiRootJoint;
  inout_stream << m_uiRuntimeHash;

  inout_stream << m_OzzSkeletonData.GetCount();
  if (!m_OzzSkeletonData.IsEmpty())
  {
    inout_stream.WriteBytes(m_OzzSkeletonData.GetData(), m_OzzSkeletonData.GetCount()).IgnoreResult();
  }

  return XII_SUCCESS;
}

xiiResult xiiSkeletonResourceDescriptor::Deserialize(xiiStreamReader& inout_stream)
{
  xiiUInt32 uiVersion = 0U;
  inout_stream >> uiVersion;
  XII_IGNORE_UNUSED(uiVersion);

  XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Joints));

  inout_stream >> m_Bounds;
  inout_stream >> m_uiRootJoint;
  inout_stream >> m_uiRuntimeHash;

  xiiUInt32 uiOzzDataSize = 0U;
  inout_stream >> uiOzzDataSize;
  if (uiOzzDataSize > 0U)
  {
    m_OzzSkeletonData.SetCount(uiOzzDataSize);

    inout_stream.ReadBytes(m_OzzSkeletonData.GetData(), uiOzzDataSize);
  }

  return XII_SUCCESS;
}

xiiSkeletonResource::xiiSkeletonResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiSkeletonResource::~xiiSkeletonResource() = default;

xiiUInt32 xiiSkeletonResource::GetJointCount() const
{
  return m_Descriptor.m_Joints.GetCount();
}

xiiUInt16 xiiSkeletonResource::FindJointByName(const xiiTempHashedString& sName) const
{
  xiiUInt16 uiJointIndex = xiiMath::MaxValue<xiiUInt16>();
  m_JointLookup.TryGetValue(sName, uiJointIndex);
  return uiJointIndex;
}

const xiiSkeletonJoint& xiiSkeletonResource::GetJoint(xiiUInt32 uiIndex) const
{
  return m_Descriptor.m_Joints[uiIndex];
}

xiiArrayPtr<const xiiSkeletonJoint> xiiSkeletonResource::GetJoints() const
{
  return m_Descriptor.m_Joints;
}

const xiiBoundingBoxSphere& xiiSkeletonResource::GetBounds() const
{
  return m_Descriptor.m_Bounds;
}

xiiUInt32 xiiSkeletonResource::GetRuntimeHash() const
{
  return m_Descriptor.m_uiRuntimeHash;
}

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
const ozz::animation::Skeleton* xiiSkeletonResource::GetOzzSkeleton() const
{
  return m_pOzzSkeleton.Borrow();
}
#endif

xiiResourceLoadDescription xiiSkeletonResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);

  m_Descriptor.Clear();
  m_JointLookup.Clear();

#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  m_pOzzSkeleton.Clear();
#endif

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

xiiResourceLoadDescription xiiSkeletonResource::UpdateContent(xiiStreamReader* pStream)
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

  xiiSkeletonResourceDescriptor descriptor;
  if (descriptor.Deserialize(*pStream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(descriptor));
  return res;
}

void xiiSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiSkeletonResource) + static_cast<xiiUInt32>(m_Descriptor.m_Joints.GetHeapMemoryUsage() + m_Descriptor.m_OzzSkeletonData.GetHeapMemoryUsage() + m_JointLookup.GetHeapMemoryUsage());
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

void xiiSkeletonResource::BuildJointLookup()
{
  m_JointLookup.Clear();
  m_JointLookup.Reserve(m_Descriptor.m_Joints.GetCount());

  for (xiiUInt32 i = 0; i < m_Descriptor.m_Joints.GetCount(); ++i)
  {
    m_JointLookup.Insert(m_Descriptor.m_Joints[i].m_sName, static_cast<xiiUInt16>(i));
  }
}

void xiiSkeletonResource::LoadOzzSkeleton(const xiiDynamicArray<xiiUInt8>& ozzData)
{
#if defined(BUILDSYSTEM_ENABLE_OZZ_SUPPORT)
  m_pOzzSkeleton.Clear();

  if (ozzData.IsEmpty())
    return;

  ozz::io::MemoryStream stream;
  stream.Write(ozzData.GetData(), ozzData.GetCount());
  stream.Seek(0, ozz::io::Stream::kSet);

  xiiUniquePtr<ozz::animation::Skeleton> pSkeleton = XII_DEFAULT_NEW(ozz::animation::Skeleton);
  ozz::io::IArchive                      archive(&stream);
  archive >> *pSkeleton.Borrow();

  m_pOzzSkeleton = std::move(pSkeleton);

  if (m_Descriptor.m_Joints.IsEmpty())
  {
    const ozz::animation::Skeleton*                pLoadedSkeleton = m_pOzzSkeleton.Borrow();
    const xiiUInt32                                uiJointCount    = static_cast<xiiUInt32>(pLoadedSkeleton->num_joints());
    const ozz::span<const ozz::math::SoaTransform> restPose        = pLoadedSkeleton->joint_rest_poses();
    const ozz::span<const int16_t>                 parents         = pLoadedSkeleton->joint_parents();
    const ozz::span<const char* const>             names           = pLoadedSkeleton->joint_names();

    m_Descriptor.m_Bounds      = xiiBoundingBoxSphere::MakeInvalid();
    m_Descriptor.m_uiRootJoint = xiiMath::MaxValue<xiiUInt16>();
    m_Descriptor.m_Joints.Reserve(uiJointCount);

    for (xiiUInt32 i = 0; i < uiJointCount; ++i)
    {
      const xiiUInt16 uiParentIndex = parents[i] < 0 ? xiiMath::MaxValue<xiiUInt16>() : static_cast<xiiUInt16>(parents[i]);
      const char*     szName        = (!names.empty() && names[i] != nullptr) ? names[i] : "";
      m_Descriptor.AddJoint(szName, uiParentIndex, GetOzzRestPoseTransform(restPose, i));
    }

    m_Descriptor.BuildModelSpaceRestPose();
    m_Descriptor.ComputeRuntimeHash();
  }
#else
  XII_IGNORE_UNUSED(ozzData);
#endif
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiSkeletonResource, xiiSkeletonResourceDescriptor)
{
  descriptor.BuildModelSpaceRestPose();
  descriptor.ComputeRuntimeHash();

  m_Descriptor = std::move(descriptor);
  LoadOzzSkeleton(m_Descriptor.m_OzzSkeletonData);
  BuildJointLookup();

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0U;
  res.m_uiQualityLevelsLoadable    = 0U;
  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonResource);
