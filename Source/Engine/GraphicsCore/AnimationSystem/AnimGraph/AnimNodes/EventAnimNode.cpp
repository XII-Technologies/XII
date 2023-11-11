#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/EventAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSendEventAnimNode, 1, xiiRTTIDefaultAllocator<xiiSendEventAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    XII_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Events"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Orange)),
    new xiiTitleAttribute("Send Event: '{EventName}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSendEventAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  XII_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSendEventAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  XII_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSendEventAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (m_sEventName.IsEmpty())
    return;

  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  xiiMsgGenericEvent msg;
  msg.m_sMessage = m_sEventName;

  pTarget->SendEventMessage(msg, nullptr);
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
