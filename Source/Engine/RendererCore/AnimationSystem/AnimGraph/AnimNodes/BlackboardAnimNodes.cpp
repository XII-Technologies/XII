#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetBlackboardValueAnimNode, 1, xiiRTTIDefaultAllocator<xiiSetBlackboardValueAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    XII_MEMBER_PROPERTY("SetOnActivation", m_bSetOnActivation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ActivationValue", m_fOnActivatedValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("SetOnHold", m_bSetOnHold)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("HoldValue", m_fOnHoldValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("SetOnDeactivation", m_bSetOnDeactivation)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("DeactivationValue", m_fOnDeactivatedValue)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Set: '{BlackboardEntry}' '{ActivationValue}''"),
    new xiiCategoryAttribute("Blackboard"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSetBlackboardValueAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fOnActivatedValue;
  stream << m_fOnHoldValue;
  stream << m_fOnDeactivatedValue;
  stream << m_bSetOnActivation;
  stream << m_bSetOnHold;
  stream << m_bSetOnDeactivation;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSetBlackboardValueAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fOnActivatedValue;
  stream >> m_fOnHoldValue;
  stream >> m_fOnDeactivatedValue;
  stream >> m_bSetOnActivation;
  stream >> m_bSetOnHold;
  stream >> m_bSetOnDeactivation;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSetBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiSetBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiSetBlackboardValueAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    xiiLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  const bool bIsActiveNow = m_ActivePin.IsTriggered(graph);

  if (bIsActiveNow != m_bLastActiveState)
  {
    m_bLastActiveState = bIsActiveNow;

    if (bIsActiveNow)
    {
      if (m_bSetOnActivation)
      {
        pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnActivatedValue);
      }
    }
    else
    {
      if (m_bSetOnDeactivation)
      {
        pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnDeactivatedValue);
      }
    }
  }
  else if (bIsActiveNow && m_bSetOnHold)
  {
    pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnHoldValue);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCheckBlackboardValueAnimNode, 1, xiiRTTIDefaultAllocator<xiiCheckBlackboardValueAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    XII_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ENUM_MEMBER_PROPERTY("Comparison", xiiComparisonOperator, m_Comparison),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("Check: '{BlackboardEntry}' {Comparison} {ReferenceValue}"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiCheckBlackboardValueAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCheckBlackboardValueAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiCheckBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiCheckBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiCheckBlackboardValueAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    xiiLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  float fValue = 0.0f;
  if (!m_sBlackboardEntry.IsEmpty())
  {
    xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<float>();
    }
    else
    {
      xiiLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  if (xiiComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}

//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGetBlackboardNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiGetBlackboardNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    XII_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("Get: '{BlackboardEntry}'"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGetBlackboardNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiGetBlackboardNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiGetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiGetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiGetBlackboardNumberAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    xiiLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  double fValue = 0.0f;

  if (!m_sBlackboardEntry.IsEmpty())
  {
    xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<double>();
    }
    else
    {
      xiiLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  m_NumberPin.SetNumber(graph, fValue);
}
