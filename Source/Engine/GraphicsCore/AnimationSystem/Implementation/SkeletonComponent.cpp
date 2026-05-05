/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkeletonComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Skeleton", xiiDependencyFlags::Package)),
    XII_RESOURCE_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClip, SetAnimationClip)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Animation_Clip", xiiDependencyFlags::Package)),
    XII_RESOURCE_ACCESSOR_PROPERTY("AnimationGraph", GetAnimationGraph, SetAnimationGraph)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Animation_Graph", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("PlaybackSpeed", GetPlaybackSpeed, SetPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("ApplyToOwnerMesh", GetApplyToOwnerMesh, SetApplyToOwnerMesh)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE

XII_BEGIN_COMPONENT_TYPE(xiiSkeletonPoseComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Skeleton", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSkeletonComponent::xiiSkeletonComponent() = default;
xiiSkeletonComponent::~xiiSkeletonComponent() = default;

void xiiSkeletonComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_hAnimationClip;
  s << m_hAnimationGraph;
  s << m_fPlaybackSpeed;
  s << m_bApplyToOwnerMesh;
}

void xiiSkeletonComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_hAnimationClip;
  s >> m_hAnimationGraph;
  s >> m_fPlaybackSpeed;
  s >> m_bApplyToOwnerMesh;

  m_AnimGraphInstance.SetGraph(m_hAnimationGraph);
}

void xiiSkeletonComponent::SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton)
{
  if (m_hSkeleton == hSkeleton)
    return;

  m_hSkeleton = hSkeleton;
  m_CurrentPose.Clear();
}

const xiiSkeletonResourceHandle& xiiSkeletonComponent::GetSkeleton() const
{
  return m_hSkeleton;
}

void xiiSkeletonComponent::SetAnimationClip(const xiiAnimationClipResourceHandle& hClip)
{
  if (m_hAnimationClip == hClip)
    return;

  m_hAnimationClip = hClip;
  m_PlaybackTime = xiiTime::MakeZero();
}

const xiiAnimationClipResourceHandle& xiiSkeletonComponent::GetAnimationClip() const
{
  return m_hAnimationClip;
}

void xiiSkeletonComponent::SetAnimationGraph(const xiiAnimGraphResourceHandle& hGraph)
{
  if (m_hAnimationGraph == hGraph)
    return;

  m_hAnimationGraph = hGraph;
  m_AnimGraphInstance.SetGraph(hGraph);
}

const xiiAnimGraphResourceHandle& xiiSkeletonComponent::GetAnimationGraph() const
{
  return m_hAnimationGraph;
}

void xiiSkeletonComponent::SetPlaybackSpeed(float fSpeed)
{
  m_fPlaybackSpeed = fSpeed;
}

float xiiSkeletonComponent::GetPlaybackSpeed() const
{
  return m_fPlaybackSpeed;
}

void xiiSkeletonComponent::SetApplyToOwnerMesh(bool bApply)
{
  m_bApplyToOwnerMesh = bApply;
}

bool xiiSkeletonComponent::GetApplyToOwnerMesh() const
{
  return m_bApplyToOwnerMesh;
}

void xiiSkeletonComponent::SetFloatParameter(xiiStringView sName, float fValue)
{
  m_AnimGraphInstance.SetFloat(sName, fValue);
}

void xiiSkeletonComponent::SetBoolParameter(xiiStringView sName, bool bValue)
{
  m_AnimGraphInstance.SetBool(sName, bValue);
}

const xiiAnimationPose& xiiSkeletonComponent::GetCurrentPose() const
{
  return m_CurrentPose;
}

void xiiSkeletonComponent::Update()
{
  if (!m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!pSkeleton)
    return;

  const xiiTime deltaTime = GetWorld()->GetClock().GetTimeDiff() * static_cast<double>(m_fPlaybackSpeed);

  if (m_hAnimationGraph.IsValid())
  {
    m_AnimGraphInstance.SetGraph(m_hAnimationGraph);
    m_AnimGraphInstance.Update(*pSkeleton, deltaTime, m_CurrentPose);
  }
  else if (m_hAnimationClip.IsValid())
  {
    m_PlaybackTime += deltaTime;
    xiiResourceLock<xiiAnimationClipResource> pClip(m_hAnimationClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pClip)
    {
      pClip->SampleLocalPose(*pSkeleton, m_PlaybackTime, m_CurrentPose);
    }
  }
  else
  {
    m_CurrentPose.ResetToRestPose(*pSkeleton);
    m_CurrentPose.BuildModelSpacePose(*pSkeleton);
    m_CurrentPose.BuildSkinningMatrices(*pSkeleton);
  }

  if (m_bApplyToOwnerMesh)
  {
    ApplyPoseToSkinnedMesh();
  }
}

void xiiSkeletonComponent::ApplyPoseToSkinnedMesh()
{
  xiiSkinnedMeshComponent* pSkinnedMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(pSkinnedMesh))
    return;

  if (!pSkinnedMesh->GetSkeleton().IsValid() && m_hSkeleton.IsValid())
  {
    pSkinnedMesh->SetSkeleton(m_hSkeleton);
  }

  pSkinnedMesh->SetSkinningMatrices(m_CurrentPose.m_SkinningMatrices);
}

xiiSkeletonPoseComponent::xiiSkeletonPoseComponent() = default;
xiiSkeletonPoseComponent::~xiiSkeletonPoseComponent() = default;

void xiiSkeletonPoseComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bApplyToOwnerMesh;
  s << m_CurrentPose.m_LocalTransforms.GetCount();
  for (const xiiTransform& transform : m_CurrentPose.m_LocalTransforms)
  {
    s << transform;
  }
}

void xiiSkeletonPoseComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bApplyToOwnerMesh;

  xiiUInt32 uiPoseCount = 0U;
  s >> uiPoseCount;
  m_CurrentPose.m_LocalTransforms.SetCount(uiPoseCount);
  for (xiiTransform& transform : m_CurrentPose.m_LocalTransforms)
  {
    s >> transform;
  }
}

void xiiSkeletonPoseComponent::SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton)
{
  if (m_hSkeleton == hSkeleton)
    return;

  m_hSkeleton = hSkeleton;
  m_CurrentPose.Clear();
}

const xiiSkeletonResourceHandle& xiiSkeletonPoseComponent::GetSkeleton() const
{
  return m_hSkeleton;
}

void xiiSkeletonPoseComponent::SetLocalPose(xiiArrayPtr<const xiiTransform> localPose)
{
  m_CurrentPose.m_LocalTransforms.SetCount(localPose.GetCount());
  for (xiiUInt32 i = 0; i < localPose.GetCount(); ++i)
  {
    m_CurrentPose.m_LocalTransforms[i] = localPose[i];
  }
}

const xiiAnimationPose& xiiSkeletonPoseComponent::GetCurrentPose() const
{
  return m_CurrentPose;
}

void xiiSkeletonPoseComponent::Update()
{
  if (!m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!pSkeleton)
    return;

  if (m_CurrentPose.m_LocalTransforms.GetCount() != pSkeleton->GetJointCount())
  {
    m_CurrentPose.ResetToRestPose(*pSkeleton);
  }

  m_CurrentPose.BuildModelSpacePose(*pSkeleton);
  m_CurrentPose.BuildSkinningMatrices(*pSkeleton);

  if (m_bApplyToOwnerMesh)
  {
    ApplyPoseToSkinnedMesh();
  }
}

void xiiSkeletonPoseComponent::ApplyPoseToSkinnedMesh()
{
  xiiSkinnedMeshComponent* pSkinnedMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(pSkinnedMesh))
    return;

  if (!pSkinnedMesh->GetSkeleton().IsValid() && m_hSkeleton.IsValid())
  {
    pSkinnedMesh->SetSkeleton(m_hSkeleton);
  }

  pSkinnedMesh->SetSkinningMatrices(m_CurrentPose.m_SkinningMatrices);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonComponent);
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonPoseComponent);
