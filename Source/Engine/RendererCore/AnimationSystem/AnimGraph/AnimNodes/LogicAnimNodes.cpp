#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicAndAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicAndAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new xiiHiddenAttribute),
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

  stream << m_bNegateResult;
  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicAndAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicAndAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  bool res = m_ActivePin.AreAllTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicOrAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicOrAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new xiiHiddenAttribute),
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

  stream << m_bNegateResult;
  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicOrAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicOrAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  bool res = m_ActivePin.IsTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogicNotAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogicNotAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new xiiHiddenAttribute),
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

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLogicNotAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLogicNotAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  bool res = !m_ActivePin.IsTriggered(graph);

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCompareNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiCompareNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ENUM_MEMBER_PROPERTY("Comparison", xiiComparisonOperator, m_Comparison),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Check: Number {Comparison} {ReferenceValue}"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiCompareNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCompareNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiCompareNumberAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (xiiComparisonOperator::Compare(m_Comparison, m_NumberPin.GetNumber(graph), m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
