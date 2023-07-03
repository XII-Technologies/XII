#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoneWeightsAnimNode, 1, xiiRTTIDefaultAllocator<xiiBoneWeightsAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    XII_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Weights"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Cyan)),
    new xiiTitleAttribute("Bone Weights '{RootBones[0]}' '{RootBones[1]}' '{RootBones[2]}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoneWeightsAnimNode::xiiBoneWeightsAnimNode()  = default;
xiiBoneWeightsAnimNode::~xiiBoneWeightsAnimNode() = default;

xiiUInt32 xiiBoneWeightsAnimNode::RootBones_GetCount() const
{
  return m_RootBones.GetCount();
}

const char* xiiBoneWeightsAnimNode::RootBones_GetValue(xiiUInt32 uiIndex) const
{
  return m_RootBones[uiIndex].GetString();
}

void xiiBoneWeightsAnimNode::RootBones_SetValue(xiiUInt32 uiIndex, const char* value)
{
  m_RootBones[uiIndex].Assign(value);
}

void xiiBoneWeightsAnimNode::RootBones_Insert(xiiUInt32 uiIndex, const char* value)
{
  xiiHashedString tmp;
  tmp.Assign(value);
  m_RootBones.Insert(tmp, uiIndex);
}

void xiiBoneWeightsAnimNode::RootBones_Remove(xiiUInt32 uiIndex)
{
  m_RootBones.RemoveAtAndCopy(uiIndex);
}

xiiResult xiiBoneWeightsAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_RootBones));

  stream << m_fWeight;

  XII_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InverseWeightsPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiBoneWeightsAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_RootBones));

  stream >> m_fWeight;

  XII_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InverseWeightsPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiBoneWeightsAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected())
    return;

  if (m_RootBones.IsEmpty())
  {
    xiiLog::Warning("No root-bones added to bone weight node in animation controller.");
    return;
  }

  if (m_pSharedBoneWeights == nullptr && m_pSharedInverseBoneWeights == nullptr)
  {
    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    xiiStringBuilder name;
    name.Format("{}", pSkeleton->GetResourceIDHash());

    for (const auto& rootBone : m_RootBones)
    {
      name.AppendFormat("-{}", rootBone);
    }

    m_pSharedBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this, pOzzSkeleton](xiiAnimGraphSharedBoneWeights& ref_bw) {
      for (const auto& rootBone : m_RootBones)
      {
        int iRootBone = -1;
        for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
        {
          if (xiiStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], rootBone.GetData()))
          {
            iRootBone = iBone;
            break;
          }
        }

        const float fBoneWeight = 1.0f;

        auto setBoneWeight = [&](int iCurrentBone, int) {
          const int iJointIdx0 = iCurrentBone / 4;
          const int iJointIdx1 = iCurrentBone % 4;

          ozz::math::SimdFloat4& soa_weight = ref_bw.m_Weights[iJointIdx0];
          soa_weight                        = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
        };

        ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
      }
    });

    if (m_InverseWeightsPin.IsConnected())
    {
      name.Append("-inv");

      m_pSharedInverseBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this](xiiAnimGraphSharedBoneWeights& ref_bw) {
        const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

        for (xiiUInt32 b = 0; b < ref_bw.m_Weights.GetCount(); ++b)
        {
          ref_bw.m_Weights[b] = ozz::math::MSub(oneBone, oneBone, m_pSharedBoneWeights->m_Weights[b]);
        }
      });
    }

    if (!m_WeightsPin.IsConnected())
    {
      m_pSharedBoneWeights.Clear();
    }
  }

  if (m_WeightsPin.IsConnected())
  {
    xiiAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight               = m_fWeight;
    pPinData->m_pSharedBoneWeights           = m_pSharedBoneWeights.Borrow();

    m_WeightsPin.SetWeights(graph, pPinData);
  }

  if (m_InverseWeightsPin.IsConnected())
  {
    xiiAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight               = m_fWeight;
    pPinData->m_pSharedBoneWeights           = m_pSharedInverseBoneWeights.Borrow();

    m_InverseWeightsPin.SetWeights(graph, pPinData);
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BoneWeightsAnimNode);
