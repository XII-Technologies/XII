#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Text", m_sText),

      XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Debug"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Pink)),
      new xiiTitleAttribute("Log: '{Text}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiLogAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input0.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input1.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input2.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input3.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input0.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input1.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input2.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_Input3.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_ActivePin.IsTriggered(graph))
    return;

  xiiLog::Dev(m_sText, m_Input0.IsTriggered(graph), m_Input1.IsTriggered(graph), m_Input2.GetNumber(graph), m_Input3.GetNumber(graph));
}
