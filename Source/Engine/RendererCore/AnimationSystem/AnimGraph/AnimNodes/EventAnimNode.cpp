#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/EventAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEventAnimNode, 1, xiiRTTIDefaultAllocator<xiiEventAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Events"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Orange)),
    new xiiTitleAttribute("Event: '{EventName}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiEventAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiEventAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiEventAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_ActivePin.IsConnected() || m_sEventName.IsEmpty())
    return;

  if (m_ActivePin.IsTriggered(graph))
  {
    xiiMsgGenericEvent msg;
    msg.m_sMessage = m_sEventName;
    msg.m_Value    = xiiVariant();

    pTarget->SendEventMessage(msg, nullptr);
  }
}
