#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/LodAnimatedMeshComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLodAnimatedMeshLod, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiLodAnimatedMeshLod>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    XII_MEMBER_PROPERTY("Threshold", m_fThreshold)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiLodAnimatedMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new xiiDefaultValueAttribute(xiiVec4(0, 1, 0, 1))),
    XII_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
    XII_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    XII_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.01f, 100.0f)),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ARRAY_MEMBER_PROPERTY("Meshes", m_Meshes),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
    new xiiSphereVisualizerAttribute("BoundsRadius", xiiColor::MediumVioletRed, nullptr, xiiVisualizerAnchor::Center, xiiVec3(1.0f), "BoundsOffset"),
    new xiiTransformManipulatorAttribute("BoundsOffset"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnMsgSetColor),
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

struct LodAnimatedMeshCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

xiiLodAnimatedMeshComponent::xiiLodAnimatedMeshComponent()  = default;
xiiLodAnimatedMeshComponent::~xiiLodAnimatedMeshComponent() = default;

void xiiLodAnimatedMeshComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo, bShow);
}

bool xiiLodAnimatedMeshComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo);
}

void xiiLodAnimatedMeshComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges, bShow);
}

bool xiiLodAnimatedMeshComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges);
}

void xiiLodAnimatedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_Meshes.GetCount();
  for (const auto& mesh : m_Meshes)
  {
    s << mesh.m_hMesh;
    s << mesh.m_fThreshold;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s << m_vCustomData;
}

void xiiLodAnimatedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  xiiUInt32 uiMeshes = 0;
  s >> uiMeshes;

  m_Meshes.SetCount(uiMeshes);

  for (auto& mesh : m_Meshes)
  {
    s >> mesh.m_hMesh;
    s >> mesh.m_fThreshold;
  }

  s >> m_Color;
  s >> m_fSortingDepthOffset;

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  if (uiVersion >= 2)
  {
    s >> m_vCustomData;
  }
}

xiiResult xiiLodAnimatedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  out_bounds         = xiiBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return XII_SUCCESS;
}

void xiiLodAnimatedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (m_Meshes.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::EditorView || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::MainView)
  {
    UpdateSelectedLod(*msg.m_pView);
  }

  if (m_iCurLod >= (xiiInt32)m_Meshes.GetCount())
    return;

  auto hMesh = m_Meshes[m_iCurLod].m_hMesh;

  if (!hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource>                      pMesh(hMesh, xiiResourceAcquireMode::AllowLoadingFallback);
  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (xiiUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const xiiUInt32           uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    xiiMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    xiiMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform     = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds        = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_hMesh               = hMesh;
      pRenderData->m_hMaterial           = hMaterial;
      pRenderData->m_Color               = m_Color;
      pRenderData->m_uiSubMeshIndex      = uiPartIndex;
      pRenderData->m_uiUniqueID          = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillSortingKey();
    }

    // Determine render data category.
    xiiRenderData::Category category = xiiDefaultRenderDataCategories::LitOpaque;
    if (hMaterial.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> pMaterial(hMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, xiiRenderData::Caching::Never);
  }
}

void xiiLodAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones, const xiiSkeleton& skeleton, xiiArrayPtr<const xiiMat4> modelSpaceTransforms, xiiBoundingBox* bounds)
{
  m_SkinningState.m_Transforms.SetCountUninitialized(bones.GetCount());

  if (bounds)
  {
    for (auto itBone : bones)
    {
      const xiiUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == xiiInvalidJointIndex)
        continue;

      bounds->ExpandToInclude(modelSpaceTransforms[uiJointIdx].GetTranslationVector());
      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  else
  {
    for (auto itBone : bones)
    {
      const xiiUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == xiiInvalidJointIndex)
        continue;

      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
}

void xiiLodAnimatedMeshComponent::SetColor(const xiiColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const xiiColor& xiiLodAnimatedMeshComponent::GetColor() const
{
  return m_Color;
}

void xiiLodAnimatedMeshComponent::SetCustomData(const xiiVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const xiiVec4& xiiLodAnimatedMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void xiiLodAnimatedMeshComponent::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float xiiLodAnimatedMeshComponent::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void xiiLodAnimatedMeshComponent::OnMsgSetColor(xiiMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void xiiLodAnimatedMeshComponent::RetrievePose(xiiDynamicArray<xiiMat4>& out_modelTransforms, xiiTransform& out_rootTransform, const xiiSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (m_Meshes.IsEmpty())
    return;

  auto hMesh = m_Meshes[0].m_hMesh;

  if (!hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  xiiResourceLock<xiiMeshResource> pMesh(hMesh, xiiResourceAcquireMode::BlockTillLoaded);

  const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones = pMesh->m_Bones;

  out_modelTransforms.SetCount(skeleton.GetJointCount(), xiiMat4::MakeIdentity());

  for (auto itBone : bones)
  {
    const xiiUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == xiiInvalidJointIndex)
      continue;

    out_modelTransforms[uiJointIdx] = m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex].GetAsMat4() * itBone.Value().m_GlobalInverseRestPoseMatrix.GetInverse();
  }
}

xiiMeshRenderData* xiiLodAnimatedMeshComponent::CreateRenderData() const
{
  auto pRenderData               = xiiCreateRenderDataForThisFrame<xiiSkinnedMeshRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = m_RootTransform;

  pRenderData->m_pSkinningTransforms = m_SkinningState.m_pGpuBuffer;

  return pRenderData;
}

static float CalculateSphereScreenSpaceCoverage(const xiiBoundingSphere& sphere, const xiiCamera& camera)
{
  if (camera.IsPerspective())
  {
    return xiiGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return xiiGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

void xiiLodAnimatedMeshComponent::UpdateSelectedLod(const xiiView& view) const
{
  const xiiInt32 iNumLods = (xiiInt32)m_Meshes.GetCount();

  const xiiVec3 vScale  = GetOwner()->GetGlobalScaling();
  const float   fScale  = xiiMath::Max(vScale.x, vScale.y, vScale.z);
  const xiiVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(xiiBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *view.GetLodCamera());

  // clamp the input value, this is to prevent issues while editing the threshold array
  xiiInt32 iNewLod = xiiMath::Clamp<xiiInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_Meshes[iNewLod - 1].m_fThreshold;
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_Meshes[iNewLod].m_fThreshold;
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_Meshes[iNewLod + 1].m_fThreshold);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod   = xiiMath::Clamp(iNewLod, 0, iNumLods);
  m_iCurLod = iNewLod;

  if (GetShowDebugInfo())
  {
    xiiStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", xiiArgF(fCoverage, 3), iNewLod, xiiArgF(fCoverageP, 3), xiiArgF(fCoverageN, 3));
    xiiDebugRenderer::Draw3DText(view.GetHandle(), sb, GetOwner()->GetGlobalPosition(), xiiColor::White);
  }
}

void xiiLodAnimatedMeshComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  xiiResourceLock<xiiMeshResource> pMesh(m_Meshes[0].m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);

  xiiBoundingBox poseBounds;
  poseBounds = xiiBoundingBox::MakeInvalid();
  MapModelSpacePoseToSkinningSpace(pMesh->m_Bones, *msg.m_pSkeleton, msg.m_ModelTransforms, &poseBounds);

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((xiiRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (XII_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }

  m_SkinningState.TransformsChanged();
}

void xiiLodAnimatedMeshComponent::OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  if (!msg.m_hSkeleton.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the xiiSkeletonComponent)

    xiiResourceLock<xiiMeshResource> pMesh(m_Meshes[0].m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

void xiiLodAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void xiiLodAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void xiiLodAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = xiiBoundingBox::MakeInvalid();

  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource> pMesh(m_Meshes[0].m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  m_hDefaultSkeleton   = pMesh->m_hDefaultSkeleton;
  const auto hSkeleton = m_hDefaultSkeleton;

  if (!hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  if (pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  {
    const ozz::animation::Skeleton* pOzzSkeleton        = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    const xiiUInt32                 uiNumSkeletonJoints = pOzzSkeleton->num_joints();

    xiiArrayPtr<ozz::math::Float4x4> pPoseMatrices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), ozz::math::Float4x4, uiNumSkeletonJoints);

    XII_ASSERT_DEBUG(xiiMemoryUtils::IsAligned(pPoseMatrices.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    {
      ozz::animation::LocalToModelJob job;
      job.input    = pOzzSkeleton->joint_rest_poses();
      job.output   = ozz::span<ozz::math::Float4x4>(pPoseMatrices.GetPtr(), pPoseMatrices.GetEndPtr());
      job.skeleton = pOzzSkeleton;
      job.Run();
    }

    xiiMsgAnimationPoseUpdated msg;
    msg.m_ModelTransforms = xiiMakeArrayPtr(reinterpret_cast<const xiiMat4*>(pPoseMatrices.GetPtr()), pPoseMatrices.GetCount());
    msg.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;

    OnAnimationPoseUpdated(msg);
  }

  TriggerLocalBoundsUpdate();
}

//////////////////////////////////////////////////////////////////////////


xiiLodAnimatedMeshComponentManager::xiiLodAnimatedMeshComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::FreeList>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

xiiLodAnimatedMeshComponentManager::~xiiLodAnimatedMeshComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void xiiLodAnimatedMeshComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiLodAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiLodAnimatedMeshComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading)
  {
    if (xiiMeshResource* pResource = xiiDynamicCast<xiiMeshResource*>(e.m_pResource))
    {
      xiiMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        for (auto& am : it->m_Meshes)
        {
          if (am.m_hMesh == hMesh)
          {
            AddToUpdateList(it);
          }
        }
      }
    }

    if (xiiSkeletonResource* pResource = xiiDynamicCast<xiiSkeletonResource*>(e.m_pResource))
    {
      xiiSkeletonResourceHandle hSkeleton(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hDefaultSkeleton == hSkeleton)
        {
          AddToUpdateList(it);
        }
      }
    }
  }
}

void xiiLodAnimatedMeshComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    xiiLodAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void xiiLodAnimatedMeshComponentManager::AddToUpdateList(xiiLodAnimatedMeshComponent* pComponent)
{
  xiiComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == xiiInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_LodAnimatedMeshComponent);
