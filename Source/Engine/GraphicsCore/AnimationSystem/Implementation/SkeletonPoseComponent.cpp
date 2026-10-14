/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkeletonPoseComponent, 2, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Skeleton", xiiDependencyFlags::Package)),
    XII_MAP_WRITE_ACCESSOR_PROPERTY("BoneTransforms", GetBoneTransforms, ReflectionSetBoneTransform, RemoveBoneTransform),
    XII_ACCESSOR_PROPERTY("ApplyToOwnerMesh", GetApplyToOwnerMesh, SetApplyToOwnerMesh)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Graphics/Animation"),
    new xiiBoneManipulatorAttribute("BoneTransforms", "Skeleton"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSkeletonPoseComponent::xiiSkeletonPoseComponent()  = default;
xiiSkeletonPoseComponent::~xiiSkeletonPoseComponent() = default;

void xiiSkeletonPoseComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream << m_hSkeleton;
  stream << m_bApplyToOwnerMesh;
  stream.WriteMap(m_BoneTransforms).AssertSuccess();
  // Preserve unresolved version 1 poses when saving before the skeleton is available.
  stream.WriteArray(m_LegacyLocalPose).AssertSuccess();
}

void xiiSkeletonPoseComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream >> m_hSkeleton;
  stream >> m_bApplyToOwnerMesh;
  if (inout_stream.GetComponentTypeVersion(GetStaticRTTI()) >= 2)
  {
    stream.ReadMap(m_BoneTransforms).AssertSuccess();
    stream.ReadArray(m_LegacyLocalPose).AssertSuccess();
  }
  else
  {
    xiiUInt32 count = 0;
    stream >> count;
    m_LegacyLocalPose.SetCount(count);
    for (auto& transform : m_LegacyLocalPose)
      stream >> transform;
  }
}

void xiiSkeletonPoseComponent::SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton)
{
  if (m_hSkeleton == hSkeleton)
    return;
  m_hSkeleton = hSkeleton;
  m_LegacyLocalPose.Clear();
  m_CurrentPose.Clear();
  m_uiPoseSkeletonRevision = xiiInvalidIndex;
}

const xiiSkeletonResourceHandle& xiiSkeletonPoseComponent::GetSkeleton() const
{
  return m_hSkeleton;
}

xiiResult xiiSkeletonPoseComponent::SetLocalPose(xiiArrayPtr<const xiiTransform> localPose)
{
  if (!m_hSkeleton.IsValid())
    return XII_FAILURE;
  xiiResourceLock<xiiSkeletonResource> skeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!skeleton || skeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  xiiAnimationPose pose;
  pose.m_LocalTransforms = localPose;
  xiiMap<xiiString, xiiTransform> transforms;
  XII_SUCCEED_OR_RETURN(pose.CopyLocalTransforms(*skeleton.GetPointer(), transforms));
  m_BoneTransforms = std::move(transforms);
  m_LegacyLocalPose.Clear();
  m_uiPoseSkeletonRevision = xiiInvalidIndex;
  return XII_SUCCESS;
}

xiiResult xiiSkeletonPoseComponent::SetBoneTransform(xiiStringView sName, const xiiTransform& transform)
{
  if (sName.IsEmpty() || !transform.IsValid())
    return XII_FAILURE;
  m_BoneTransforms.Insert(sName, transform);
  m_uiPoseSkeletonRevision = xiiInvalidIndex;
  return XII_SUCCESS;
}

void xiiSkeletonPoseComponent::ReflectionSetBoneTransform(xiiStringView sName, const xiiTransform& transform)
{
  if (SetBoneTransform(sName, transform).Failed())
    xiiLog::Error("Cannot set invalid skeleton bone transform '{}'.", sName);
}

void xiiSkeletonPoseComponent::RemoveBoneTransform(xiiStringView sName)
{
  if (m_BoneTransforms.Remove(sName))
    m_uiPoseSkeletonRevision = xiiInvalidIndex;
}

const xiiMap<xiiString, xiiTransform>& xiiSkeletonPoseComponent::GetBoneTransforms() const
{
  return m_BoneTransforms;
}

void xiiSkeletonPoseComponent::SetApplyToOwnerMesh(bool bApply)
{
  m_bApplyToOwnerMesh = bApply;
}

bool xiiSkeletonPoseComponent::GetApplyToOwnerMesh() const
{
  return m_bApplyToOwnerMesh;
}

const xiiAnimationPose& xiiSkeletonPoseComponent::GetCurrentPose() const
{
  return m_CurrentPose;
}

xiiResult xiiSkeletonPoseComponent::CopyCurrentLocalPose(xiiMap<xiiString, xiiTransform>& out_transforms) const
{
  out_transforms.Clear();
  if (!m_hSkeleton.IsValid() || m_uiPoseSkeletonRevision == xiiInvalidIndex)
    return XII_FAILURE;
  xiiResourceLock<xiiSkeletonResource> skeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!skeleton || skeleton.GetAcquireResult() != xiiResourceAcquireResult::Final ||
      skeleton->GetCurrentResourceChangeCounter() != m_uiPoseSkeletonRevision)
    return XII_FAILURE;
  return m_CurrentPose.CopyLocalTransforms(*skeleton.GetPointer(), out_transforms);
}

void xiiSkeletonPoseComponent::Update()
{
  m_uiPoseSkeletonRevision = xiiInvalidIndex;
  if (!m_hSkeleton.IsValid())
    return;
  xiiResourceLock<xiiSkeletonResource> skeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!skeleton || skeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  if (!m_LegacyLocalPose.IsEmpty())
  {
    xiiAnimationPose legacy;
    legacy.m_LocalTransforms = m_LegacyLocalPose;
    xiiMap<xiiString, xiiTransform> transforms;
    if (legacy.CopyLocalTransforms(*skeleton.GetPointer(), transforms).Failed())
    {
      xiiLog::Warning("Cannot resolve version 1 skeleton pose against the current skeleton.");
      return;
    }
    for (const auto& transform : transforms)
      if (!m_BoneTransforms.Contains(transform.Key()))
        m_BoneTransforms.Insert(transform.Key(), transform.Value());
    m_LegacyLocalPose.Clear();
  }

  m_CurrentPose.ResetToRestPose(*skeleton.GetPointer());
  for (const auto& transform : m_BoneTransforms)
  {
    if (!transform.Value().IsValid())
      return;
    const auto joint = skeleton->FindJointByName(xiiTempHashedString(transform.Key()));
    if (joint < m_CurrentPose.m_LocalTransforms.GetCount())
      m_CurrentPose.m_LocalTransforms[joint] = transform.Value();
  }
  m_CurrentPose.BuildModelSpacePose(*skeleton.GetPointer());
  m_CurrentPose.BuildSkinningMatrices(*skeleton.GetPointer());
  m_uiPoseSkeletonRevision = skeleton->GetCurrentResourceChangeCounter();
  if (m_bApplyToOwnerMesh)
    ApplyPoseToSkinnedMesh();
}

void xiiSkeletonPoseComponent::ApplyPoseToSkinnedMesh()
{
  xiiSkinnedMeshComponent* mesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(mesh))
    return;
  if (!mesh->GetSkeleton().IsValid() || mesh->GetSkeleton() == m_hSkeleton)
    mesh->SetAnimationPose(m_hSkeleton, m_CurrentPose).IgnoreResult();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonPoseComponent);
