#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/CombinePosesAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCombinePosesAnimNode, 1, xiiRTTIDefaultAllocator<xiiCombinePosesAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxPoses", m_uiMaxPoses)->AddAttributes(new xiiDefaultValueAttribute(8)),
    XII_MEMBER_PROPERTY("LocalPoses", m_LocalPosesPin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Pose Processing"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Indigo)),
    new xiiTitleAttribute("Combine Poses"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCombinePosesAnimNode::xiiCombinePosesAnimNode()  = default;
xiiCombinePosesAnimNode::~xiiCombinePosesAnimNode() = default;

xiiResult xiiCombinePosesAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiMaxPoses;

  XII_SUCCEED_OR_RETURN(m_LocalPosesPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCombinePosesAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiMaxPoses;

  XII_SUCCEED_OR_RETURN(m_LocalPosesPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiCombinePosesAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LocalPosesPin.IsConnected())
    return;

  xiiHybridArray<xiiAnimGraphPinDataLocalTransforms*, 16> pIn;

  m_LocalPosesPin.GetPoses(graph, pIn);

  if (pIn.IsEmpty())
    return;

  xiiAnimGraphPinDataLocalTransforms* pPinData = graph.AddPinDataLocalTransforms();

  pPinData->m_vRootMotion.SetZero();

  float fSummedRootMotionWeight = 0.0f;

  // TODO: skip blending, if only a single animation is played
  // unless the weight is below 1.0 and the bind pose should be faded in

  auto& cmd = graph.GetPoseGenerator().AllocCommandCombinePoses();

  struct PinWeight
  {
    xiiUInt32 m_uiPinIdx;
    float     m_fPinWeight = 0.0f;
  };

  xiiHybridArray<PinWeight, 16> pw;
  pw.SetCount(pIn.GetCount());

  for (xiiUInt32 i = 0; i < pIn.GetCount(); ++i)
  {
    pw[i].m_uiPinIdx = i;

    if (pIn[i] != nullptr)
    {
      pw[i].m_fPinWeight = pIn[i]->m_fOverallWeight;

      if (pIn[i]->m_pWeights)
      {
        pw[i].m_fPinWeight *= pIn[i]->m_pWeights->m_fOverallWeight;
      }
    }
  }

  if (pw.GetCount() > m_uiMaxPoses)
  {
    pw.Sort([](const PinWeight& lhs, const PinWeight& rhs) { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
    pw.SetCount(m_uiMaxPoses);
  }

  xiiArrayPtr<const ozz::math::SimdFloat4> invWeights;

  for (const auto& in : pw)
  {
    if (in.m_fPinWeight > 0 && pIn[in.m_uiPinIdx]->m_pWeights)
    {
      // only initialize and use the inverse mask, when it is actually needed
      if (invWeights.IsEmpty())
      {
        m_BlendMask.SetCountUninitialized(pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

        for (auto& sj : m_BlendMask)
        {
          sj = ozz::math::simd_float4::one();
        }

        invWeights = m_BlendMask;
      }

      const ozz::math::SimdFloat4 factor = ozz::math::simd_float4::Load1(in.m_fPinWeight);

      const xiiArrayPtr<const ozz::math::SimdFloat4> weights = pIn[in.m_uiPinIdx]->m_pWeights->m_pSharedBoneWeights->m_Weights;

      for (xiiUInt32 i = 0; i < m_BlendMask.GetCount(); ++i)
      {
        const auto& weight = weights[i];
        auto&       mask   = m_BlendMask[i];

        const auto oneMinusWeight = ozz::math::NMAdd(factor, weight, ozz::math::simd_float4::one());

        mask = ozz::math::Min(mask, oneMinusWeight);
      }
    }
  }

  for (const auto& in : pw)
  {
    if (in.m_fPinWeight > 0)
    {
      if (pIn[in.m_uiPinIdx]->m_pWeights)
      {
        const xiiArrayPtr<const ozz::math::SimdFloat4> weights = pIn[in.m_uiPinIdx]->m_pWeights->m_pSharedBoneWeights->m_Weights;

        cmd.m_InputBoneWeights.PushBack(weights);
      }
      else
      {
        cmd.m_InputBoneWeights.PushBack(invWeights);
      }

      if (pIn[in.m_uiPinIdx]->m_bUseRootMotion)
      {
        fSummedRootMotionWeight += in.m_fPinWeight;
        pPinData->m_vRootMotion += pIn[in.m_uiPinIdx]->m_vRootMotion * in.m_fPinWeight;

        // TODO: combining quaternions is mathematically tricky
        // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

        pPinData->m_bUseRootMotion = true;
      }

      cmd.m_Inputs.PushBack(pIn[in.m_uiPinIdx]->m_CommandID);
      cmd.m_InputWeights.PushBack(in.m_fPinWeight);
    }
  }

  if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
  {
    pPinData->m_vRootMotion /= fSummedRootMotionWeight;
  }

  pPinData->m_CommandID = cmd.GetCommandID();

  m_LocalPosePin.SetPose(graph, pPinData);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_CombinePosesAnimNode);
