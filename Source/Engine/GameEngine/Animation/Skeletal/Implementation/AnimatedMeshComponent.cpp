#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsFoundation/Device/Device.h>

#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAnimatedMeshComponent, 13, xiiComponentMode::Dynamic); // TODO: why dynamic ? (I guess because the overridden CreateRenderData() has to be called every frame)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiExposeColorAlphaAttribute()),
    XII_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    XII_MESSAGE_HANDLER(xiiMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRootMotionMode, 1)
  XII_ENUM_CONSTANTS(xiiRootMotionMode::Ignore, xiiRootMotionMode::ApplyToOwner, xiiRootMotionMode::SendMoveCharacterMsg)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiAnimatedMeshComponent::xiiAnimatedMeshComponent()  = default;
xiiAnimatedMeshComponent::~xiiAnimatedMeshComponent() = default;

void xiiAnimatedMeshComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void xiiAnimatedMeshComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  XII_ASSERT_DEV(uiVersion >= 13, "Unsupported version, delete the file and reexport it");
}

void xiiAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void xiiAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void xiiAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds .SetInvalid();

  if (!m_hMesh.IsValid())
    return;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
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

    xiiArrayPtr<xiiMat4> pPoseMatrices = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), xiiMat4, uiNumSkeletonJoints);

    {
      ozz::animation::LocalToModelJob job;
      job.input    = pOzzSkeleton->joint_rest_poses();
      job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(pPoseMatrices.GetPtr()), reinterpret_cast<ozz::math::Float4x4*>(pPoseMatrices.GetEndPtr()));
      job.skeleton = pOzzSkeleton;
      job.Run();
    }

    xiiMsgAnimationPoseUpdated msg;
    msg.m_ModelTransforms = pPoseMatrices;
    msg.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;

    OnAnimationPoseUpdated(msg);
  }

  TriggerLocalBoundsUpdate();
}


void xiiAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones, const xiiSkeleton& skeleton, xiiArrayPtr<const xiiMat4> modelSpaceTransforms, xiiBoundingBox* bounds)
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

xiiMeshRenderData* xiiAnimatedMeshComponent::CreateRenderData() const
{
  auto pRenderData               = xiiCreateRenderDataForThisFrame<xiiSkinnedMeshRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = m_RootTransform;

  m_SkinningState.FillSkinnedMeshRenderData(*pRenderData);

  return pRenderData;
}

void xiiAnimatedMeshComponent::RetrievePose(xiiDynamicArray<xiiMat4>& out_modelTransforms, xiiTransform& out_rootTransform, const xiiSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (!m_hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);

  const xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData>& bones = pMesh->m_Bones;

  out_modelTransforms.SetCount(skeleton.GetJointCount(), xiiMat4::IdentityMatrix());

  for (auto itBone : bones)
  {
    const xiiUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == xiiInvalidJointIndex)
      continue;

    out_modelTransforms[uiJointIdx] = m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex].GetAsMat4() * itBone.Value().m_GlobalInverseRestPoseMatrix.GetInverse();
  }
}

void xiiAnimatedMeshComponent::OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg)
{
  if (!m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);

  xiiBoundingBox poseBounds;
  poseBounds.SetInvalid();
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

void xiiAnimatedMeshComponent::OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg)
{
  if (!msg.m_hSkeleton.IsValid() && m_hMesh.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the xiiSkeletonComponent)

    xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

xiiResult xiiAnimatedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (!m_MaxBounds.IsValid() || !m_hMesh.IsValid())
    return XII_FAILURE;

  xiiResourceLock<xiiMeshResource> pMesh(m_hMesh, xiiResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;

  xiiBoundingBox bbox = m_MaxBounds;
  bbox.Grow(xiiVec3(pMesh->m_fMaxBoneVertexOffset));
  bounds = xiiBoundingBoxSphere(bbox);
  bounds.Transform(m_RootTransform.GetAsMat4());
  return XII_SUCCESS;
}

void xiiRootMotionMode::Apply(xiiRootMotionMode::Enum mode, xiiGameObject* pObject, const xiiVec3& vTranslation, xiiAngle rotationX, xiiAngle rotationY, xiiAngle rotationZ)
{
  switch (mode)
  {
    case xiiRootMotionMode::Ignore:
      return;

    case xiiRootMotionMode::ApplyToOwner:
    {
      xiiVec3 vNewPos = pObject->GetLocalPosition();
      vNewPos += pObject->GetLocalRotation() * vTranslation;
      pObject->SetLocalPosition(vNewPos);

      // not tested whether this is actually correct
      xiiQuat rotation;
      rotation.SetFromEulerAngles(rotationX, rotationY, rotationZ);

      pObject->SetLocalRotation(rotation * pObject->GetLocalRotation());

      return;
    }

    case xiiRootMotionMode::SendMoveCharacterMsg:
    {
      xiiMsgApplyRootMotion msg;
      msg.m_vTranslation = vTranslation;
      msg.m_RotationX    = rotationX;
      msg.m_RotationY    = rotationY;
      msg.m_RotationZ    = rotationZ;

      while (pObject != nullptr)
      {
        pObject->SendMessage(msg);
        pObject = pObject->GetParent();
      }

      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////


xiiAnimatedMeshComponentManager::xiiAnimatedMeshComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::FreeList>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiAnimatedMeshComponentManager::ResourceEventHandler, this));
}

xiiAnimatedMeshComponentManager::~xiiAnimatedMeshComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void xiiAnimatedMeshComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiAnimatedMeshComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading)
  {
    if (xiiMeshResource* pResource = xiiDynamicCast<xiiMeshResource*>(e.m_pResource))
    {
      xiiMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hMesh == hMesh)
        {
          AddToUpdateList(it);
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

void xiiAnimatedMeshComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    xiiAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void xiiAnimatedMeshComponentManager::AddToUpdateList(xiiAnimatedMeshComponent* pComponent)
{
  xiiComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == xiiInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);
