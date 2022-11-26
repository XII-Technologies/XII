#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ModelPoseOutputAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiModelPoseOutputAnimNode, 1, xiiRTTIDefaultAllocator<xiiModelPoseOutputAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("RotateZ", m_RotateZPin)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Output"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Grape)),
    new xiiTitleAttribute("Output"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiModelPoseOutputAnimNode::xiiModelPoseOutputAnimNode()  = default;
xiiModelPoseOutputAnimNode::~xiiModelPoseOutputAnimNode() = default;

xiiResult xiiModelPoseOutputAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_RotateZPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiModelPoseOutputAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_RotateZPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiModelPoseOutputAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  xiiVec3  rootMotion = xiiVec3::ZeroVector();
  xiiAngle rootRotationX;
  xiiAngle rootRotationY;
  xiiAngle rootRotationZ;

  if (m_ModelPosePin.IsConnected())
  {
    if (auto pCurrentModelTransforms = m_ModelPosePin.GetPose(graph))
    {
      if (pCurrentModelTransforms->m_CommandID != xiiInvalidIndex)
      {
        auto& cmd = graph.GetPoseGenerator().AllocCommandModelPoseToOutput();
        cmd.m_Inputs.PushBack(m_ModelPosePin.GetPose(graph)->m_CommandID);
      }

      if (pCurrentModelTransforms->m_bUseRootMotion)
      {
        rootMotion    = pCurrentModelTransforms->m_vRootMotion;
        rootRotationX = pCurrentModelTransforms->m_RootRotationX;
        rootRotationY = pCurrentModelTransforms->m_RootRotationY;
        rootRotationZ = pCurrentModelTransforms->m_RootRotationZ;
      }

      graph.SetOutputModelTransform(pCurrentModelTransforms);
    }
  }

  if (m_RotateZPin.IsConnected())
  {
    const float rotZ = static_cast<float>(m_RotateZPin.GetNumber(graph));
    rootRotationZ += xiiAngle::Degree(rotZ);
  }

  graph.SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
}
