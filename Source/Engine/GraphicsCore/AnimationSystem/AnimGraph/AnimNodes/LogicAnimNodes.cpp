#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicAndAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicAndAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new xiiNoTemporaryTransactionsAttribute(), new xiiDynamicPinAttribute(), new xiiDefaultValueAttribute(2)),
    XII_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new xiiHiddenAttribute(), new xiiDynamicPinAttribute("BoolCount")),
    XII_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("AND"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLogicAndAnimNode::xiiLogicAndAnimNode()  = default;
xiiLogicAndAnimNode::~xiiLogicAndAnimNode() = default;

xiiResult xiiLogicAndAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicAndAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicAndAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  bool res = true;

  for (const auto& pin : m_InBool)
  {
    if (!pin.GetBool(ref_graph, true))
    {
      res = false;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicEventAndAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicEventAndAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutOnActivated", m_OutOnActivated)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Event AND"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLogicEventAndAnimNode::xiiLogicEventAndAnimNode()  = default;
xiiLogicEventAndAnimNode::~xiiLogicEventAndAnimNode() = default;

xiiResult xiiLogicEventAndAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnActivated.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicEventAndAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnActivated.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicEventAndAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (m_InActivate.IsTriggered(ref_graph) && m_InBool.GetBool(ref_graph))
  {
    m_OutOnActivated.SetTriggered(ref_graph);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicOrAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicOrAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new xiiNoTemporaryTransactionsAttribute(), new xiiDynamicPinAttribute(), new xiiDefaultValueAttribute(2)),
    XII_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new xiiHiddenAttribute(), new xiiDynamicPinAttribute("BoolCount")),
    XII_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("OR"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLogicOrAnimNode::xiiLogicOrAnimNode()  = default;
xiiLogicOrAnimNode::~xiiLogicOrAnimNode() = default;

xiiResult xiiLogicOrAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicOrAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicOrAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  bool res = false;

  for (const auto& pin : m_InBool)
  {
    if (!pin.GetBool(ref_graph, true))
    {
      res = true;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicNotAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicNotAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("NOT"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLogicNotAnimNode::xiiLogicNotAnimNode()  = default;
xiiLogicNotAnimNode::~xiiLogicNotAnimNode() = default;

xiiResult xiiLogicNotAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicNotAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicNotAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  const bool value = !m_InBool.GetBool(ref_graph);

  m_OutBool.SetBool(ref_graph, !value);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
