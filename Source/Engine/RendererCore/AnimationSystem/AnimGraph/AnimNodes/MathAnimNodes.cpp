#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MathAnimNodes.h>

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

void xiiMathExpressionAnimNode::SetExpression(const char* szSz)
{
  m_mExpression.Reset(szSz);
}

const char* xiiMathExpressionAnimNode::GetExpression() const
{
  return m_mExpression.GetExpressionString();
}

xiiResult xiiMathExpressionAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_mExpression.GetExpressionString();
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

  xiiStringBuilder tmp;
  stream >> tmp;
  m_mExpression.Reset(tmp);
  XII_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiMathExpressionAnimNode::Initialize(xiiAnimGraph& graph, const xiiSkeletonResource* pSkeleton)
{
  if (!m_mExpression.IsValid() && m_ResultPin.IsConnected())
  {
    xiiLog::Error("Math expression '{}' is invalid.", m_mExpression.GetExpressionString());
  }
}

static xiiHashedString s_sA = xiiMakeHashedString("a");
static xiiHashedString s_sB = xiiMakeHashedString("b");
static xiiHashedString s_sC = xiiMakeHashedString("c");
static xiiHashedString s_sD = xiiMakeHashedString("d");

void xiiMathExpressionAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(graph, 0);
    return;
  }

  xiiMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(graph))},
    };

  float result = m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(graph, result);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
