#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/RootMotionAnimNodes.h>

// clang-format off
 XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRootRotationAnimNode, 1, xiiRTTIDefaultAllocator<xiiRootRotationAnimNode>)
{
   XII_BEGIN_PROPERTIES
   {
     XII_MEMBER_PROPERTY("InRotateX", m_InRotateX)->AddAttributes(new xiiHiddenAttribute),
     XII_MEMBER_PROPERTY("InRotateY", m_InRotateY)->AddAttributes(new xiiHiddenAttribute),
     XII_MEMBER_PROPERTY("InRotateZ", m_InRotateZ)->AddAttributes(new xiiHiddenAttribute),
   }
   XII_END_PROPERTIES;
   XII_BEGIN_ATTRIBUTES
   {
     new xiiCategoryAttribute("Output"),
     new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Grape)),
     new xiiTitleAttribute("Root Rotation"),
   }
   XII_END_ATTRIBUTES;
 }
 XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRootRotationAnimNode::xiiRootRotationAnimNode()  = default;
xiiRootRotationAnimNode::~xiiRootRotationAnimNode() = default;

xiiResult xiiRootRotationAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InRotateX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InRotateY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InRotateZ.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiRootRotationAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InRotateX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InRotateY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InRotateZ.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiRootRotationAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  xiiVec3  vRootMotion = xiiVec3::ZeroVector();
  xiiAngle rootRotationX;
  xiiAngle rootRotationY;
  xiiAngle rootRotationZ;

  ref_controller.GetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);

  if (m_InRotateX.IsConnected())
  {
    rootRotationX += xiiAngle::MakeFromDegree(static_cast<float>(m_InRotateX.GetNumber(ref_graph)));
  }
  if (m_InRotateY.IsConnected())
  {
    rootRotationY += xiiAngle::MakeFromDegree(static_cast<float>(m_InRotateY.GetNumber(ref_graph)));
  }
  if (m_InRotateZ.IsConnected())
  {
    rootRotationZ += xiiAngle::MakeFromDegree(static_cast<float>(m_InRotateZ.GetNumber(ref_graph)));
  }

  ref_controller.SetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
