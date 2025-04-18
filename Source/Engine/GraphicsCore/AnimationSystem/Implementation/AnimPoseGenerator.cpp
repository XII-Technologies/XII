#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/base/span.h>

void xiiAnimPoseGenerator::Reset(const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  m_pSkeleton         = pSkeleton;
  m_pTargetGameObject = pTarget;
  m_LocalPoseCounter  = 0;
  m_ModelPoseCounter  = 0;
  m_FinalCommand      = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsRestPose.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsSampleEventTrack.Clear();
  m_CommandsAimIK.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  // m_UsedModelTransforms.Clear();
  // m_SamplingCaches.Clear();
}

static XII_ALWAYS_INLINE xiiAnimPoseGeneratorCommandID CreateCommandID(xiiAnimPoseGeneratorCommandType type, xiiUInt32 uiIndex)
{
  return (static_cast<xiiUInt32>(type) << 24u) | uiIndex;
}

static XII_ALWAYS_INLINE xiiUInt32 GetCommandIndex(xiiAnimPoseGeneratorCommandID id)
{
  return static_cast<xiiUInt32>(id) & 0x00FFFFFFu;
}

static XII_ALWAYS_INLINE xiiAnimPoseGeneratorCommandType GetCommandType(xiiAnimPoseGeneratorCommandID id)
{
  return static_cast<xiiAnimPoseGeneratorCommandType>(static_cast<xiiUInt32>(id) >> 24u);
}

xiiAnimPoseGeneratorCommandSampleTrack& xiiAnimPoseGenerator::AllocCommandSampleTrack(xiiUInt32 uiDeterministicID)
{
  auto& cmd             = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.m_Type            = xiiAnimPoseGeneratorCommandType::SampleTrack;
  cmd.m_CommandID       = CreateCommandID(cmd.m_Type, m_CommandsSampleTrack.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;
  cmd.m_uiUniqueID      = uiDeterministicID;

  return cmd;
}

xiiAnimPoseGeneratorCommandRestPose& xiiAnimPoseGenerator::AllocCommandRestPose()
{
  auto& cmd             = m_CommandsRestPose.ExpandAndGetRef();
  cmd.m_Type            = xiiAnimPoseGeneratorCommandType::RestPose;
  cmd.m_CommandID       = CreateCommandID(cmd.m_Type, m_CommandsRestPose.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

xiiAnimPoseGeneratorCommandCombinePoses& xiiAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd             = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.m_Type            = xiiAnimPoseGeneratorCommandType::CombinePoses;
  cmd.m_CommandID       = CreateCommandID(cmd.m_Type, m_CommandsCombinePoses.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

xiiAnimPoseGeneratorCommandLocalToModelPose& xiiAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd             = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.m_Type            = xiiAnimPoseGeneratorCommandType::LocalToModelPose;
  cmd.m_CommandID       = CreateCommandID(cmd.m_Type, m_CommandsLocalToModelPose.GetCount() - 1);
  cmd.m_ModelPoseOutput = m_ModelPoseCounter++;

  return cmd;
}

xiiAnimPoseGeneratorCommandSampleEventTrack& xiiAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd       = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type      = xiiAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

  return cmd;
}

xiiAnimPoseGeneratorCommandAimIK& xiiAnimPoseGenerator::AllocCommandAimIK()
{
  auto& cmd       = m_CommandsAimIK.ExpandAndGetRef();
  cmd.m_Type      = xiiAnimPoseGeneratorCommandType::AimIK;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsAimIK.GetCount() - 1);

  return cmd;
}

xiiAnimPoseGeneratorCommandTwoBoneIK& xiiAnimPoseGenerator::AllocCommandTwoBoneIK()
{
  auto& cmd       = m_CommandsTwoBoneIK.ExpandAndGetRef();
  cmd.m_Type      = xiiAnimPoseGeneratorCommandType::TwoBoneIK;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsTwoBoneIK.GetCount() - 1);

  return cmd;
}

xiiAnimPoseGenerator::xiiAnimPoseGenerator() = default;

xiiAnimPoseGenerator::~xiiAnimPoseGenerator()
{
  for (xiiUInt32 i = 0; i < m_SamplingCaches.GetCount(); ++i)
  {
    XII_DEFAULT_DELETE(m_SamplingCaches.GetValue(i));
  }
  m_SamplingCaches.Clear();
}

void xiiAnimPoseGenerator::Validate() const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)

  for (auto& cmd : m_CommandsSampleTrack)
  {
    XII_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    // XII_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    XII_ASSERT_DEV(cmd.m_LocalPoseOutput != xiiInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    // XII_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    XII_ASSERT_DEV(cmd.m_LocalPoseOutput != xiiInvalidIndex, "Output pose not allocated.");
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::SampleTrack || type == xiiAnimPoseGeneratorCommandType::CombinePoses || type == xiiAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    XII_ASSERT_DEV(cmd.m_ModelPoseOutput != xiiInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::SampleTrack || type == xiiAnimPoseGeneratorCommandType::CombinePoses || type == xiiAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsAimIK)
  {
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::LocalToModelPose || type == xiiAnimPoseGeneratorCommandType::AimIK || type == xiiAnimPoseGeneratorCommandType::TwoBoneIK, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsTwoBoneIK)
  {
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::LocalToModelPose || type == xiiAnimPoseGeneratorCommandType::AimIK || type == xiiAnimPoseGeneratorCommandType::TwoBoneIK, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    XII_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  }

#endif
}

const xiiAnimPoseGeneratorCommand& xiiAnimPoseGenerator::GetCommand(xiiAnimPoseGeneratorCommandID id) const
{
  return const_cast<xiiAnimPoseGenerator*>(this)->GetCommand(id);
}

xiiAnimPoseGeneratorCommand& xiiAnimPoseGenerator::GetCommand(xiiAnimPoseGeneratorCommandID id)
{
  XII_ASSERT_DEV(id != xiiInvalidIndex, "Invalid command ID");

  switch (GetCommandType(id))
  {
    case xiiAnimPoseGeneratorCommandType::SampleTrack:
      return m_CommandsSampleTrack[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::RestPose:
      return m_CommandsRestPose[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::AimIK:
      return m_CommandsAimIK[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::TwoBoneIK:
      return m_CommandsTwoBoneIK[GetCommandIndex(id)];

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  XII_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

void xiiAnimPoseGenerator::UpdatePose(bool bRequestExternalPoseGeneration)
{
  if (m_FinalCommand == 0)
    return;

  Validate();

  Execute(GetCommand(m_FinalCommand));

  if (bRequestExternalPoseGeneration && m_pTargetGameObject)
  {
    xiiMsgAnimationPoseGeneration poseGenMsg;
    poseGenMsg.m_pGenerator = this;
    m_pTargetGameObject->SendMessageRecursive(poseGenMsg);

    // update the pose once again afterwards
    Execute(GetCommand(m_FinalCommand));
  }
}

void xiiAnimPoseGenerator::Execute(xiiAnimPoseGeneratorCommand& cmd)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id));
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandSampleTrack&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::RestPose:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandRestPose&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandLocalToModelPose&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandSampleEventTrack&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::AimIK:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandAimIK&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::TwoBoneIK:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandTwoBoneIK&>(cmd));
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandSampleTrack& cmd)
{
  xiiResourceLock<xiiAnimationClipResource> pResource(cmd.m_hAnimationClip, xiiResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  cmd.m_bAdditive = pResource->GetDescriptor().m_bAdditive;

  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  auto& pSampler = m_SamplingCaches[cmd.m_uiUniqueID];

  if (pSampler == nullptr)
  {
    pSampler = XII_DEFAULT_NEW(ozz::animation::SamplingJob::Context);
  }

  if (pSampler->max_tracks() != ozzAnim.num_tracks())
  {
    pSampler->Resize(ozzAnim.num_tracks());
  }

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.context   = pSampler;
  job.ratio     = cmd.m_fNormalizedSamplePos;
  job.output    = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());

  if (!job.Validate())
    return;

  XII_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandRestPose& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  const auto restPose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();

  transforms.CopyFrom(xiiArrayPtr<const ozz::math::SoaTransform>(restPose.begin(), (xiiUInt32)restPose.size()));
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  xiiHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;
  xiiHybridArray<ozz::animation::BlendingJob::Layer, 8> blAdd;

  for (xiiUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    if (cmdIn.GetType() == xiiAnimPoseGeneratorCommandType::SampleEventTrack)
      continue;

    ozz::animation::BlendingJob::Layer* layer = nullptr;

    switch (cmdIn.GetType())
    {
      case xiiAnimPoseGeneratorCommandType::SampleTrack:
      {
        if (static_cast<const xiiAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_bAdditive)
        {
          layer = &blAdd.ExpandAndGetRef();
        }
        else
        {
          layer = &bl.ExpandAndGetRef();
        }

        auto transform   = AcquireLocalPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case xiiAnimPoseGeneratorCommandType::RestPose:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform   = AcquireLocalPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case xiiAnimPoseGeneratorCommandType::CombinePoses:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform   = AcquireLocalPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    layer->weight = cmd.m_InputWeights[i];

    if (cmd.m_InputBoneWeights.GetCount() > i && !cmd.m_InputBoneWeights[i].IsEmpty())
    {
      layer->joint_weights = ozz::span(cmd.m_InputBoneWeights[i].GetPtr(), cmd.m_InputBoneWeights[i].GetEndPtr());
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold       = 1.0f;
  job.layers          = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.additive_layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(blAdd), end(blAdd));
  job.rest_pose       = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
  job.output          = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());
  XII_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandLocalToModelPose& cmd)
{
  ozz::animation::LocalToModelJob job;

  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::SampleTrack:
    {
      cmd.m_LocalPoseOutput = static_cast<const xiiAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput;
    }
    break;

    case xiiAnimPoseGeneratorCommandType::RestPose:
    {
      cmd.m_LocalPoseOutput = static_cast<const xiiAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput;
    }
    break;

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
    {
      cmd.m_LocalPoseOutput = static_cast<const xiiAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput;
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);
  job.input      = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());

  if (cmd.m_pSendLocalPoseMsgTo || m_pTargetGameObject)
  {
    xiiMsgAnimationPosePreparing msg;
    msg.m_pSkeleton       = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = xiiMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (xiiUInt32)job.input.size());

    if (m_pTargetGameObject)
      m_pTargetGameObject->SendMessageRecursive(msg);
    else
      cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  m_OutputPose = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
  // This cast is safe because m_OutputPose points to m_UsedModelTransforms which is 16 byte aligned.
  XII_ASSERT_DEBUG(xiiMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
  job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  XII_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandSampleEventTrack& cmd)
{
  xiiResourceLock<xiiAnimationClipResource> pResource(cmd.m_hAnimationClip, xiiResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void MultiplySoATransformQuaternion(xiiUInt32 uiIndex, const ozz::math::SimdQuaternion& quat, xiiArrayPtr<ozz::math::SoaTransform>& ref_transforms)
{
  XII_ASSERT_DEBUG(uiIndex < ref_transforms.GetCount() * 4, "Joint index out of bound.");

  // Convert SOA to AOS in order to perform quaternion multiplication, and get back to SOA.
  ozz::math::SoaTransform&  soa_transform_ref = ref_transforms[uiIndex / 4];
  ozz::math::SimdQuaternion aos_quats[4];
  ozz::math::Transpose4x4(&soa_transform_ref.rotation.x, &aos_quats->xyzw);

  ozz::math::SimdQuaternion& aos_quat_ref = aos_quats[uiIndex & 3];
  aos_quat_ref                            = aos_quat_ref * quat;

  ozz::math::Transpose4x4(&aos_quats->xyzw, &soa_transform_ref.rotation.x);
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandAimIK& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
    {
      const xiiAnimPoseGeneratorCommandLocalToModelPose& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn);
      cmd.m_LocalPoseOutput                                     = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                                     = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                              = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case xiiAnimPoseGeneratorCommandType::AimIK:
    {
      const xiiAnimPoseGeneratorCommandAimIK& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandAimIK&>(cmdIn);
      cmd.m_LocalPoseOutput                          = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                          = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                   = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case xiiAnimPoseGeneratorCommandType::TwoBoneIK:
    {
      const xiiAnimPoseGeneratorCommandTwoBoneIK& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandTwoBoneIK&>(cmdIn);
      cmd.m_LocalPoseOutput                              = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                              = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                       = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  XII_ASSERT_DEBUG(cmd.m_uiJointIdx < m_OutputPose.GetCount(), "Invalid joint index");

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  const xiiMat4* pJoint = &m_OutputPose[cmd.m_uiJointIdx];

  ozz::math::SimdQuaternion correction;
  bool                      bReached = false;

  // patch the local poses
  {
    ozz::animation::IKAimJob job;
    job.weight           = cmd.m_fWeight;
    job.target           = ozz::math::simd_float4::Load3PtrU(cmd.m_vTargetPosition.GetData());
    job.forward          = ozz::math::simd_float4::Load3PtrU(cmd.m_vForwardVector.GetData());
    job.up               = ozz::math::simd_float4::Load3PtrU(cmd.m_vUpVector.GetData());
    job.pole_vector      = ozz::math::simd_float4::Load3PtrU(cmd.m_vPoleVector.GetData());
    job.joint_correction = &correction;
    job.joint            = reinterpret_cast<const ozz::math::Float4x4*>(pJoint);
    job.reached          = &bReached;
    XII_ASSERT_DEBUG(job.Validate(), "");
    job.Run();

    MultiplySoATransformQuaternion(cmd.m_uiJointIdx, correction, transform);
  }

  // rebuild the model poses
  {
    ozz::animation::LocalToModelJob job;
    job.from  = (int)cmd.m_uiJointIdx;
    job.to    = (int)cmd.m_uiRecalcModelPoseToJointIdx;
    job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    XII_ASSERT_DEBUG(xiiMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
    job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    XII_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandTwoBoneIK& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
    {
      const xiiAnimPoseGeneratorCommandLocalToModelPose& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn);
      cmd.m_LocalPoseOutput                                     = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                                     = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                              = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case xiiAnimPoseGeneratorCommandType::AimIK:
    {
      const xiiAnimPoseGeneratorCommandAimIK& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandAimIK&>(cmdIn);
      cmd.m_LocalPoseOutput                          = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                          = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                   = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

    case xiiAnimPoseGeneratorCommandType::TwoBoneIK:
    {
      const xiiAnimPoseGeneratorCommandTwoBoneIK& cmdIn2 = static_cast<const xiiAnimPoseGeneratorCommandTwoBoneIK&>(cmdIn);
      cmd.m_LocalPoseOutput                              = cmdIn2.m_LocalPoseOutput;
      cmd.m_ModelPoseOutput                              = cmdIn2.m_ModelPoseOutput;
      m_OutputPose                                       = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);
      break;
    }

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  auto transform = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  XII_ASSERT_DEBUG(cmd.m_uiJointIdxStart < m_OutputPose.GetCount(), "Invalid joint index");
  XII_ASSERT_DEBUG(cmd.m_uiJointIdxMiddle < m_OutputPose.GetCount(), "Invalid joint index");
  XII_ASSERT_DEBUG(cmd.m_uiJointIdxEnd < m_OutputPose.GetCount(), "Invalid joint index");

  const xiiMat4* pJointStart  = &m_OutputPose[cmd.m_uiJointIdxStart];
  const xiiMat4* pJointMiddle = &m_OutputPose[cmd.m_uiJointIdxMiddle];
  const xiiMat4* pJointEnd    = &m_OutputPose[cmd.m_uiJointIdxEnd];

  ozz::math::SimdQuaternion correctionStart, correctionMiddle;
  bool                      bReached = false;

  // patch the local poses
  {
    ozz::animation::IKTwoBoneJob job;
    job.reached                = &bReached;
    job.weight                 = cmd.m_fWeight;
    job.target                 = ozz::math::simd_float4::Load3PtrU(cmd.m_vTargetPosition.GetData());
    job.start_joint            = reinterpret_cast<const ozz::math::Float4x4*>(pJointStart);
    job.mid_joint              = reinterpret_cast<const ozz::math::Float4x4*>(pJointMiddle);
    job.end_joint              = reinterpret_cast<const ozz::math::Float4x4*>(pJointEnd);
    job.start_joint_correction = &correctionStart;
    job.mid_joint_correction   = &correctionMiddle;
    job.mid_axis               = ozz::math::simd_float4::Load3PtrU(cmd.m_vMidAxis.GetData());
    job.pole_vector            = ozz::math::simd_float4::Load3PtrU(cmd.m_vPoleVector.GetData());
    job.soften                 = cmd.m_fSoften;
    job.twist_angle            = cmd.m_TwistAngle.GetRadian();
    XII_ASSERT_DEBUG(job.Validate(), "");
    job.Run();

    MultiplySoATransformQuaternion(cmd.m_uiJointIdxStart, correctionStart, transform);
    MultiplySoATransformQuaternion(cmd.m_uiJointIdxMiddle, correctionMiddle, transform);
  }

  // rebuild the model poses
  {
    ozz::animation::LocalToModelJob job;
    job.from  = (int)cmd.m_uiJointIdxStart;
    job.to    = (int)cmd.m_uiRecalcModelPoseToJointIdx;
    job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    XII_ASSERT_DEBUG(xiiMemoryUtils::IsAligned(m_OutputPose.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(m_OutputPose.GetPtr()), m_OutputPose.GetCount());
    job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    XII_ASSERT_DEBUG(job.Validate(), "");
    job.Run();
  }
}

void xiiAnimPoseGenerator::SampleEventTrack(const xiiAnimationClipResource* pResource, xiiAnimPoseEventTrackSampleMode mode, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == xiiAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const xiiTime duration = pResource->GetDescriptor().GetDuration();

  const xiiTime tPrev  = fPrevPos * duration;
  const xiiTime tNow   = fCurPos * duration;
  const xiiTime tStart = xiiTime::MakeZero();
  const xiiTime tEnd   = duration + xiiTime::MakeFromSeconds(1.0); // sampling position is EXCLUSIVE

  xiiHybridArray<xiiHashedString, 16> events;

  switch (mode)
  {
    case xiiAnimPoseEventTrackSampleMode::OnlyBetween:
      et.Sample(tPrev, tNow, events);
      break;

    case xiiAnimPoseEventTrackSampleMode::LoopAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tStart, tNow, events);
      break;

    case xiiAnimPoseEventTrackSampleMode::LoopAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

    case xiiAnimPoseEventTrackSampleMode::BounceAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tEnd, tNow, events);
      break;

    case xiiAnimPoseEventTrackSampleMode::BounceAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  xiiMsgGenericEvent msg;

  for (const auto& hs : events)
  {
    msg.m_sMessage = hs;

    m_pTargetGameObject->SendEventMessage(msg, nullptr);
  }
}

xiiArrayPtr<ozz::math::SoaTransform> xiiAnimPoseGenerator::AcquireLocalPoseTransforms(xiiAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id].IsEmpty())
  {
    using T                   = ozz::math::SoaTransform;
    const xiiUInt32 num       = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints();
    m_UsedLocalTransforms[id] = XII_NEW_ARRAY(xiiFrameAllocator::GetCurrentAllocator(), T, num);
  }

  return m_UsedLocalTransforms[id];
}

xiiArrayPtr<xiiMat4> xiiAnimPoseGenerator::AcquireModelPoseTransforms(xiiAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  m_UsedModelTransforms[id].SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return m_UsedModelTransforms[id];
}
