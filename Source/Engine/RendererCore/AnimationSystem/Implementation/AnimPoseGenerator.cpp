#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/span.h>

void xiiAnimPoseGenerator::Reset(const xiiSkeletonResource* pSkeleton)
{
  m_pSkeleton        = pSkeleton;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsModelPoseToOutput.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  //m_UsedModelTransforms.Clear();
  //m_SamplingCaches.Clear();
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

xiiAnimPoseGeneratorCommandModelPoseToOutput& xiiAnimPoseGenerator::AllocCommandModelPoseToOutput()
{
  auto& cmd       = m_CommandsModelPoseToOutput.ExpandAndGetRef();
  cmd.m_Type      = xiiAnimPoseGeneratorCommandType::ModelPoseToOutput;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsModelPoseToOutput.GetCount() - 1);

  return cmd;
}

xiiAnimPoseGeneratorCommandSampleEventTrack& xiiAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd       = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type      = xiiAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

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
  XII_ASSERT_DEV(m_CommandsModelPoseToOutput.GetCount() <= 1, "Only one output node may exist");

  for (auto& cmd : m_CommandsSampleTrack)
  {
    XII_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    //XII_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    XII_ASSERT_DEV(cmd.m_LocalPoseOutput != xiiInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    //XII_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    XII_ASSERT_DEV(cmd.m_LocalPoseOutput != xiiInvalidIndex, "Output pose not allocated.");
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::SampleTrack || type == xiiAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    XII_ASSERT_DEV(cmd.m_ModelPoseOutput != xiiInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::SampleTrack || type == xiiAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    XII_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      XII_ASSERT_DEV(type == xiiAnimPoseGeneratorCommandType::LocalToModelPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    XII_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  }
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

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::ModelPoseToOutput:
      return m_CommandsModelPoseToOutput[GetCommandIndex(id)];

    case xiiAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  XII_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

xiiArrayPtr<xiiMat4> xiiAnimPoseGenerator::GeneratePose(const xiiGameObject* pSendAnimationEventsTo /*= nullptr*/)
{
  Validate();

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    Execute(cmd, pSendAnimationEventsTo);
  }

  auto pPose = m_OutputPose;

  // TODO: clear temp data

  return pPose;
}

void xiiAnimPoseGenerator::Execute(xiiAnimPoseGeneratorCommand& cmd, const xiiGameObject* pSendAnimationEventsTo)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id), pSendAnimationEventsTo);
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandSampleTrack&>(cmd), pSendAnimationEventsTo);
      break;

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandLocalToModelPose&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::ModelPoseToOutput:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandModelPoseToOutput&>(cmd));
      break;

    case xiiAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<xiiAnimPoseGeneratorCommandSampleEventTrack&>(cmd), pSendAnimationEventsTo);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandSampleTrack& cmd, const xiiGameObject* pSendAnimationEventsTo)
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

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
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
      auto transform = AcquireLocalPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
      job.input      = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

    case xiiAnimPoseGeneratorCommandType::CombinePoses:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
      job.input      = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (cmd.m_pSendLocalPoseMsgTo)
  {
    xiiMsgAnimationPosePreparing msg;
    msg.m_pSkeleton       = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = xiiMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (xiiUInt32)job.input.size());

    cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  auto transforms = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);

  job.output   = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(transforms.GetPtr()), transforms.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  XII_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandModelPoseToOutput& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case xiiAnimPoseGeneratorCommandType::LocalToModelPose:
      m_OutputPose = AcquireModelPoseTransforms(static_cast<const xiiAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn).m_ModelPoseOutput);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiAnimPoseGenerator::ExecuteCmd(xiiAnimPoseGeneratorCommandSampleEventTrack& cmd, const xiiGameObject* pSendAnimationEventsTo)
{
  xiiResourceLock<xiiAnimationClipResource> pResource(cmd.m_hAnimationClip, xiiResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void xiiAnimPoseGenerator::SampleEventTrack(const xiiAnimationClipResource* pResource, xiiAnimPoseEventTrackSampleMode mode, const xiiGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == xiiAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const xiiTime duration = pResource->GetDescriptor().GetDuration();

  const xiiTime tPrev  = fPrevPos * duration;
  const xiiTime tNow   = fCurPos * duration;
  const xiiTime tStart = xiiTime::Zero();
  const xiiTime tEnd   = duration + xiiTime::Seconds(1.0); // sampling position is EXCLUSIVE

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

    pSendAnimationEventsTo->SendEventMessage(msg, nullptr);
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
