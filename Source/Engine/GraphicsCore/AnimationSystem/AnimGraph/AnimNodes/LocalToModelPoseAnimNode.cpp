#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

//// clang-format off
// XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLocalToModelPoseAnimNode, 1, xiiRTTIDefaultAllocator<xiiLocalToModelPoseAnimNode>)
//{
//   XII_BEGIN_PROPERTIES
//   {
//     XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute),
//     XII_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new xiiHiddenAttribute),
//   }
//   XII_END_PROPERTIES;
//   XII_BEGIN_ATTRIBUTES
//   {
//     new xiiCategoryAttribute("Pose Processing"),
//     new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
//     new xiiTitleAttribute("Local To Model Space"),
//   }
//   XII_END_ATTRIBUTES;
// }
// XII_END_DYNAMIC_REFLECTED_TYPE;
//// clang-format on
//
// xiiLocalToModelPoseAnimNode::xiiLocalToModelPoseAnimNode() = default;
// xiiLocalToModelPoseAnimNode::~xiiLocalToModelPoseAnimNode() = default;
//
// xiiResult xiiLocalToModelPoseAnimNode::SerializeNode(xiiStreamWriter& stream) const
//{
//  stream.WriteVersion(1);
//
//  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));
//
//  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
//  XII_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
//
//  return XII_SUCCESS;
//}
//
// xiiResult xiiLocalToModelPoseAnimNode::DeserializeNode(xiiStreamReader& stream)
//{
//  stream.ReadVersion(1);
//
//  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));
//
//  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
//  XII_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
//
//  return XII_SUCCESS;
//}
//
// void xiiLocalToModelPoseAnimNode::Step(xiiAnimGraphExecutor& executor, xiiAnimGraphInstance& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
//{
//  if (!m_LocalPosePin.IsConnected() || !m_ModelPosePin.IsConnected())
//    return;
//
//  auto pLocalPose = m_LocalPosePin.GetPose(graph);
//  if (pLocalPose == nullptr)
//    return;
//
//  xiiAnimGraphPinDataModelTransforms* pModelTransform = graph.AddPinDataModelTransforms();
//
//  if (pLocalPose->m_bUseRootMotion)
//  {
//    pModelTransform->m_bUseRootMotion = true;
//    pModelTransform->m_vRootMotion = pLocalPose->m_vRootMotion;
//  }
//
//  auto& cmd = graph.GetPoseGenerator().AllocCommandLocalToModelPose();
//  cmd.m_Inputs.PushBack(m_LocalPosePin.GetPose(graph)->m_CommandID);
//
//  pModelTransform->m_CommandID = cmd.GetCommandID();
//
//  m_ModelPosePin.SetPose(graph, pModelTransform);
//}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
