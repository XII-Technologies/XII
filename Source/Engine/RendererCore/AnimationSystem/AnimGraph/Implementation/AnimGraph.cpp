#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton.h>

xiiMutex                                                             xiiAnimGraph::s_SharedDataMutex;
xiiHashTable<xiiString, xiiSharedPtr<xiiAnimGraphSharedBoneWeights>> xiiAnimGraph::s_SharedBoneWeights;

xiiAnimGraph::xiiAnimGraph()  = default;
xiiAnimGraph::~xiiAnimGraph() = default;

void xiiAnimGraph::Configure(const xiiSkeletonResourceHandle& hSkeleton, xiiAnimPoseGenerator& ref_poseGenerator, const xiiSharedPtr<xiiBlackboard>& pBlackboard /*= nullptr*/)
{
  m_hSkeleton      = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard    = pBlackboard;
}

void xiiAnimGraph::Update(xiiTime diff, xiiGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  if (!m_bInitialized)
  {
    m_bInitialized = true;

    XII_LOG_BLOCK("Initializing animation controller graph");

    for (const auto& pNode : m_Nodes)
    {
      pNode->Initialize(*this, pSkeleton.GetPointer());
    }
  }

  m_pCurrentModelTransforms = nullptr;

  m_pPoseGenerator->Reset(pSkeleton.GetPointer());

  // reset all pin states
  {
    m_PinDataBoneWeights.Clear();
    m_PinDataLocalTransforms.Clear();
    m_PinDataModelTransforms.Clear();

    for (auto& pin : m_TriggerInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_NumberInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_BoneWeightInputPinStates)
    {
      pin = 0xFFFF;
    }
    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
    for (auto& pin : m_ModelPoseInputPinStates)
    {
      pin = 0xFFFF;
    }
  }

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(*this, diff, pSkeleton.GetPointer(), pTarget);
  }

  if (auto newPose = GetPoseGenerator().GeneratePose(pTarget); !newPose.IsEmpty())
  {
    xiiMsgAnimationPoseUpdated msg;
    msg.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    // Recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments.
    pTarget->SendMessageRecursive(msg);
  }
}

void xiiAnimGraph::GetRootMotion(xiiVec3& ref_vTranslation, xiiAngle& ref_rotationX, xiiAngle& ref_rotationY, xiiAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX    = m_RootRotationX;
  ref_rotationY    = m_RootRotationY;
  ref_rotationZ    = m_RootRotationZ;
}

xiiResult xiiAnimGraph::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(5);

  const xiiUInt32 uiNumNodes = m_Nodes.GetCount();
  ref_stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    ref_stream << node->GetDynamicRTTI()->GetTypeName();

    XII_SUCCEED_OR_RETURN(node->SerializeNode(ref_stream));
  }

  ref_stream << m_hSkeleton;

  {
    XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_TriggerInputPinStates));

    ref_stream << m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(ar));
    }
  }
  {
    XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(m_NumberInputPinStates));

    ref_stream << m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(ar));
    }
  }
  {
    ref_stream << m_BoneWeightInputPinStates.GetCount();

    ref_stream << m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(ar));
    }
  }
  {
    ref_stream << m_LocalPoseInputPinStates.GetCount();

    ref_stream << m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(ar));
    }
  }
  {
    ref_stream << m_ModelPoseInputPinStates.GetCount();

    ref_stream << m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.WriteArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return XII_SUCCESS;
}

xiiResult xiiAnimGraph::Deserialize(xiiStreamReader& ref_stream)
{
  const auto uiVersion = ref_stream.ReadVersion(5);

  xiiUInt32 uiNumNodes = 0;
  ref_stream >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  xiiStringBuilder sTypeName;

  for (auto& node : m_Nodes)
  {
    ref_stream >> sTypeName;
    node = std::move(xiiRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<xiiAnimGraphNode>());

    XII_SUCCEED_OR_RETURN(node->DeserializeNode(ref_stream));
  }

  ref_stream >> m_hSkeleton;

  if (uiVersion >= 2)
  {
    XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_TriggerInputPinStates));

    xiiUInt32 sar = 0;
    ref_stream >> sar;
    m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 3)
  {
    XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(m_NumberInputPinStates));

    xiiUInt32 sar = 0;
    ref_stream >> sar;
    m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 4)
  {
    xiiUInt32 sar = 0;

    ref_stream >> sar;
    m_BoneWeightInputPinStates.SetCount(sar);

    ref_stream >> sar;
    m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    xiiUInt32 sar = 0;

    ref_stream >> sar;
    m_LocalPoseInputPinStates.SetCount(sar);

    ref_stream >> sar;
    m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    xiiUInt32 sar = 0;

    ref_stream >> sar;
    m_ModelPoseInputPinStates.SetCount(sar);

    ref_stream >> sar;
    m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose])
    {
      XII_SUCCEED_OR_RETURN(ref_stream.ReadArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return XII_SUCCESS;
}

xiiAnimGraphPinDataBoneWeights* xiiAnimGraph::AddPinDataBoneWeights()
{
  xiiAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex                   = static_cast<xiiUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

xiiAnimGraphPinDataLocalTransforms* xiiAnimGraph::AddPinDataLocalTransforms()
{
  xiiAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex                       = static_cast<xiiUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

xiiAnimGraphPinDataModelTransforms* xiiAnimGraph::AddPinDataModelTransforms()
{
  xiiAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex                       = static_cast<xiiUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void xiiAnimGraph::SetOutputModelTransform(xiiAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void xiiAnimGraph::SetRootMotion(const xiiVec3& vTranslation, xiiAngle rotationX, xiiAngle rotationY, xiiAngle rotationZ)
{
  m_vRootMotion   = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

xiiSharedPtr<xiiAnimGraphSharedBoneWeights> xiiAnimGraph::CreateBoneWeights(const char* szUniqueName, const xiiSkeletonResource& skeleton, xiiDelegate<void(xiiAnimGraphSharedBoneWeights&)> fill)
{
  XII_LOCK(s_SharedDataMutex);

  xiiSharedPtr<xiiAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = XII_DEFAULT_NEW(xiiAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    xiiMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
