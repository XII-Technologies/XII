#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogAnimNode, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new xiiDefaultValueAttribute("Values: {0}/{1}-{3}/{4}")),

      XII_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("NumberCount", m_uiNumberCount)->AddAttributes(new xiiNoTemporaryTransactionsAttribute(), new xiiDynamicPinAttribute(), new xiiDefaultValueAttribute(1)),
      XII_ARRAY_MEMBER_PROPERTY("InNumbers", m_InNumbers)->AddAttributes(new xiiHiddenAttribute(), new xiiDynamicPinAttribute("NumberCount")),
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
  stream << m_uiNumberCount;

  XII_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InNumbers));

  return XII_SUCCESS;
}

xiiResult xiiLogAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  XII_IGNORE_UNUSED(version);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;
  stream >> m_uiNumberCount;

  XII_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InNumbers));

  return XII_SUCCESS;
}

static xiiStringView BuildFormattedText(xiiStringView sText, const xiiVariantArray& params, xiiStringBuilder& ref_sStorage)
{
  xiiHybridArray<xiiString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<xiiString>());
  }

  xiiHybridArray<xiiStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  xiiFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogInfoAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogInfoAnimNode>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Log Info: '{Text}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLogInfoAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  xiiVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  xiiStringBuilder sStorage;
  xiiLog::Info(BuildFormattedText(m_sText, params, sStorage));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogErrorAnimNode, 1, xiiRTTIDefaultAllocator<xiiLogErrorAnimNode>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Log Error: '{Text}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiLogErrorAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  xiiVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  xiiStringBuilder sStorage;
  xiiLog::Error(BuildFormattedText(m_sText, params, sStorage));
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
