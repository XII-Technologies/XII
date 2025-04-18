#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/MathAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMathExpressionAnimNode, 1, xiiRTTIDefaultAllocator<xiiMathExpressionAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression)->AddAttributes(new xiiDefaultValueAttribute("a*a + (b-c) / abs(d)")),
    XII_MEMBER_PROPERTY("a", m_ValueAPin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("b", m_ValueBPin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("c", m_ValueCPin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("d", m_ValueDPin)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("Result", m_ResultPin)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
    new xiiTitleAttribute("= {Expression}"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMathExpressionAnimNode::xiiMathExpressionAnimNode()  = default;
xiiMathExpressionAnimNode::~xiiMathExpressionAnimNode() = default;

void xiiMathExpressionAnimNode::SetExpression(xiiString sExpr)
{
  m_sExpression = sExpr;
}

xiiString xiiMathExpressionAnimNode::GetExpression() const
{
  return m_sExpression;
}

xiiResult xiiMathExpressionAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sExpression;
  XII_SUCCEED_OR_RETURN(m_ValueAPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueBPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueCPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueDPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ResultPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiMathExpressionAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sExpression;
  XII_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return XII_SUCCESS;
}

static xiiHashedString s_sA = xiiMakeHashedString("a");
static xiiHashedString s_sB = xiiMakeHashedString("b");
static xiiHashedString s_sC = xiiMakeHashedString("c");
static xiiHashedString s_sD = xiiMakeHashedString("d");

void xiiMathExpressionAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_mExpression.GetExpressionString().IsEmpty())
  {
    pInstance->m_mExpression.Reset(m_sExpression);
  }

  if (!pInstance->m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(ref_graph, 0);
    return;
  }

  xiiMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(ref_graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(ref_graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(ref_graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(ref_graph))},
    };

  float result = pInstance->m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(ref_graph, result);
}

bool xiiMathExpressionAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCompareNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiCompareNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    XII_ENUM_MEMBER_PROPERTY("Comparison", xiiComparisonOperator, m_Comparison),

    XII_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("InReference", m_InReference)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Compare: Number {Comparison} {ReferenceValue}"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiCompareNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(2);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  XII_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InReference.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCompareNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  auto version = stream.ReadVersion(2);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  XII_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InReference.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  if (version >= 2)
  {
    XII_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));
  }

  return XII_SUCCESS;
}

void xiiCompareNumberAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  const bool bIsTrue = xiiComparisonOperator::Compare<double>(m_Comparison, m_InNumber.GetNumber(ref_graph), m_InReference.GetNumber(ref_graph, m_fReferenceValue));

  m_OutIsTrue.SetBool(ref_graph, bIsTrue);
  m_OutIsFalse.SetBool(ref_graph, !bIsTrue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoolToNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiBoolToNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FalseValue", m_fFalseValue)->AddAttributes(new xiiDefaultValueAttribute(0.0)),
    XII_MEMBER_PROPERTY("TrueValue", m_fTrueValue)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    XII_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Bool To Number"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoolToNumberAnimNode::xiiBoolToNumberAnimNode()  = default;
xiiBoolToNumberAnimNode::~xiiBoolToNumberAnimNode() = default;

xiiResult xiiBoolToNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fFalseValue;
  stream << m_fTrueValue;

  XII_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiBoolToNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fFalseValue;
  stream >> m_fTrueValue;

  XII_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiBoolToNumberAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  m_OutNumber.SetNumber(ref_graph, m_InValue.GetBool(ref_graph) ? m_fTrueValue : m_fFalseValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoolToTriggerAnimNode, 1, xiiRTTIDefaultAllocator<xiiBoolToTriggerAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Bool To Event"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoolToTriggerAnimNode::xiiBoolToTriggerAnimNode()  = default;
xiiBoolToTriggerAnimNode::~xiiBoolToTriggerAnimNode() = default;

xiiResult xiiBoolToTriggerAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiBoolToTriggerAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));

  return XII_SUCCESS;
}

bool xiiBoolToTriggerAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void xiiBoolToTriggerAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool    bIsTrueNow = m_InValue.GetBool(ref_graph);
  const xiiInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bIsTrueNow)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
