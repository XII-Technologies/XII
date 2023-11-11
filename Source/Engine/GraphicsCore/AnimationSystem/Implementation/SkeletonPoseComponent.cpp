#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSkeletonPoseMode, 1)
  XII_ENUM_CONSTANTS(xiiSkeletonPoseMode::CustomPose, xiiSkeletonPoseMode::RestPose, xiiSkeletonPoseMode::Disabled)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiSkeletonPoseComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    XII_ENUM_ACCESSOR_PROPERTY("Mode", xiiSkeletonPoseMode, GetPoseMode, SetPoseMode),
    XII_MEMBER_PROPERTY("EditBones", m_fDummy),
    XII_MAP_ACCESSOR_PROPERTY("Bones", GetBones, GetBone, SetBone, RemoveBone)->AddAttributes(new xiiExposedParametersAttribute("Skeleton"), new xiiContainerAttribute(false, true, false)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
    new xiiBoneManipulatorAttribute("Bones", "EditBones"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkeletonPoseComponent::xiiSkeletonPoseComponent()  = default;
xiiSkeletonPoseComponent::~xiiSkeletonPoseComponent() = default;

void xiiSkeletonPoseComponent::Update()
{
  if (m_uiResendPose == 0)
    return;

  if (--m_uiResendPose > 0)
  {
    static_cast<xiiSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
  }

  if (m_PoseMode == xiiSkeletonPoseMode::RestPose)
  {
    SendRestPose();
    return;
  }

  if (m_PoseMode == xiiSkeletonPoseMode::CustomPose)
  {
    SendCustomPose();
    return;
  }
}

void xiiSkeletonPoseComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_PoseMode;

  m_Bones.Sort();
  xiiUInt16 numBones = static_cast<xiiUInt16>(m_Bones.GetCount());
  s << numBones;

  for (xiiUInt16 i = 0; i < numBones; ++i)
  {
    s << m_Bones.GetKey(i);
    s << m_Bones.GetValue(i).m_sName;
    s << m_Bones.GetValue(i).m_sParent;
    s << m_Bones.GetValue(i).m_Transform;
  }
}

void xiiSkeletonPoseComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_PoseMode;

  xiiHashedString sKey;
  xiiExposedBone  bone;

  xiiUInt16 numBones = 0;
  s >> numBones;
  m_Bones.Reserve(numBones);

  for (xiiUInt16 i = 0; i < numBones; ++i)
  {
    s >> sKey;
    s >> bone.m_sName;
    s >> bone.m_sParent;
    s >> bone.m_Transform;

    m_Bones[sKey] = bone;
  }
  ResendPose();
}

void xiiSkeletonPoseComponent::OnActivated()
{
  SUPER::OnActivated();

  ResendPose();
}

void xiiSkeletonPoseComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ResendPose();
}

void xiiSkeletonPoseComponent::SetSkeletonFile(const char* szFile)
{
  xiiSkeletonResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* xiiSkeletonPoseComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}

void xiiSkeletonPoseComponent::SetSkeleton(const xiiSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    ResendPose();
  }
}

void xiiSkeletonPoseComponent::SetPoseMode(xiiEnum<xiiSkeletonPoseMode> mode)
{
  m_PoseMode = mode;
  ResendPose();
}

void xiiSkeletonPoseComponent::ResendPose()
{
  if (m_uiResendPose == 2)
    return;

  m_uiResendPose = 2;
  static_cast<xiiSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
}

const xiiRangeView<const char*, xiiUInt32> xiiSkeletonPoseComponent::GetBones() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Bones.GetCount(); },
                                              [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                              [this](const xiiUInt32& uiIt) -> const char* { return m_Bones.GetKey(uiIt).GetString().GetData(); });
}

void xiiSkeletonPoseComponent::SetBone(const char* szKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(szKey);

  if (value.GetReflectedType() == xiiGetStaticRTTI<xiiExposedBone>())
  {
    m_Bones[hs] = *reinterpret_cast<const xiiExposedBone*>(value.GetData());
  }

  // TODO
  // if (IsActiveAndInitialized())
  //{
  //  // only add to update list, if not yet activated,
  //  // since OnActivate will do the instantiation anyway
  //  GetWorld()->GetComponentManager<xiiPrefabReferenceComponentManager>()->AddToUpdateList(this);
  //}
  ResendPose();
}

void xiiSkeletonPoseComponent::RemoveBone(const char* szKey)
{
  if (m_Bones.RemoveAndCopy(xiiTempHashedString(szKey)))
  {
    // TODO
    // if (IsActiveAndInitialized())
    //{
    //  // only add to update list, if not yet activated,
    //  // since OnActivate will do the instantiation anyway
    //  GetWorld()->GetComponentManager<xiiPrefabReferenceComponentManager>()->AddToUpdateList(this);
    //}

    ResendPose();
  }
}

bool xiiSkeletonPoseComponent::GetBone(const char* szKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Bones.Find(szKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value.CopyTypedObject(&m_Bones.GetValue(it), xiiGetStaticRTTI<xiiExposedBone>());
  return true;
}

void xiiSkeletonPoseComponent::SendRestPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  if (skel.GetJointCount() == 0)
    return;

  xiiHybridArray<xiiMat4, 32> finalTransforms(xiiFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  {
    ozz::animation::LocalToModelJob job;
    job.input    = skel.GetOzzSkeleton().joint_rest_poses();
    job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
    job.skeleton = &skel.GetOzzSkeleton();
    job.Run();
  }

  xiiMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform  = &desc.m_RootTransform;
  msg.m_pSkeleton       = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = xiiSkeletonPoseMode::Disabled;
}

void xiiSkeletonPoseComponent::SendCustomPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                          desc = pSkeleton->GetDescriptor();
  const auto&                          skel = desc.m_Skeleton;

  xiiHybridArray<xiiMat4, 32> finalTransforms(xiiFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  for (xiiUInt32 i = 0; i < finalTransforms.GetCount(); ++i)
  {
    finalTransforms[i].SetIdentity();
  }

  ozz::vector<ozz::math::SoaTransform> ozzLocalTransforms;
  ozzLocalTransforms.resize((skel.GetJointCount() + 3) / 4);

  auto restPoses = skel.GetOzzSkeleton().joint_rest_poses();

  // initialize the skeleton with the rest pose
  for (xiiUInt32 i = 0; i < ozzLocalTransforms.size(); ++i)
  {
    ozzLocalTransforms[i] = restPoses[i];
  }

  for (const auto& boneIt : m_Bones)
  {
    const xiiUInt16 uiBone = skel.FindJointByName(boneIt.key);
    if (uiBone == xiiInvalidJointIndex)
      continue;

    const xiiExposedBone& thisBone = boneIt.value;

    // this can happen when the property was reverted
    if (thisBone.m_sName.IsEmpty() || thisBone.m_sParent.IsEmpty())
      continue;

    XII_ASSERT_DEBUG(!thisBone.m_Transform.m_qRotation.IsNaN(), "Invalid bone transform in pose component");

    const xiiQuat& boneRot = thisBone.m_Transform.m_qRotation;

    const xiiUInt32 idx0 = uiBone / 4;
    const xiiUInt32 idx1 = uiBone % 4;

    ozz::math::SoaQuaternion& q          = ozzLocalTransforms[idx0].rotation;
    reinterpret_cast<float*>(&q.x)[idx1] = boneRot.x;
    reinterpret_cast<float*>(&q.y)[idx1] = boneRot.y;
    reinterpret_cast<float*>(&q.z)[idx1] = boneRot.z;
    reinterpret_cast<float*>(&q.w)[idx1] = boneRot.w;
  }

  ozz::animation::LocalToModelJob job;
  job.input    = ozz::span<const ozz::math::SoaTransform>(ozzLocalTransforms.data(), ozzLocalTransforms.size());
  job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
  job.skeleton = &skel.GetOzzSkeleton();
  XII_ASSERT_DEBUG(job.Validate(), "");
  job.Run();


  xiiMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform  = &desc.m_RootTransform;
  msg.m_pSkeleton       = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = xiiSkeletonPoseMode::Disabled;
}

//////////////////////////////////////////////////////////////////////////

void xiiSkeletonPoseComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  xiiDeque<xiiComponentHandle> requireUpdate;

  {
    XII_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    xiiSkeletonPoseComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->Update();
  }
}

void xiiSkeletonPoseComponentManager::EnqueueUpdate(xiiComponentHandle hComponent)
{
  XII_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != xiiInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void xiiSkeletonPoseComponentManager::Initialize()
{
  SUPER::Initialize();

  xiiWorldModule::UpdateFunctionDesc desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiSkeletonPoseComponentManager::Update, this);
  desc.m_Phase                            = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonPoseComponent);
