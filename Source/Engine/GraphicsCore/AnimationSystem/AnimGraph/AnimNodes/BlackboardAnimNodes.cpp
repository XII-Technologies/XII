#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetBlackboardNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiSetBlackboardNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    XII_MEMBER_PROPERTY("Number", m_fNumber),

    XII_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Set Number: '{BlackboardEntry}' to {Number}"),
    new xiiCategoryAttribute("Blackboard"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSetBlackboardNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fNumber;

  XII_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSetBlackboardNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fNumber;

  XII_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiSetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiSetBlackboardNumberAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InNumber.GetNumber(ref_graph, m_fNumber));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGetBlackboardNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiGetBlackboardNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    XII_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("Get Number: '{BlackboardEntry}'"),
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

  XII_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiGetBlackboardNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

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

void xiiGetBlackboardNumberAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    xiiLog::Warning("AnimController::GetBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  m_OutNumber.SetNumber(ref_graph, value.ConvertTo<double>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCompareBlackboardNumberAnimNode, 1, xiiRTTIDefaultAllocator<xiiCompareBlackboardNumberAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    XII_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    XII_ENUM_MEMBER_PROPERTY("Comparison", xiiComparisonOperator, m_Comparison),

    XII_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new xiiHiddenAttribute()),
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

xiiResult xiiCompareBlackboardNumberAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(2);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCompareBlackboardNumberAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiCompareBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiCompareBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiCompareBlackboardNumberAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    xiiLog::Warning("AnimController::CompareBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const double  fValue     = value.ConvertTo<double>();
  const bool    bIsTrueNow = xiiComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue);
  const xiiInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  m_OutIsTrue.SetBool(ref_graph, bIsTrueNow);

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

bool xiiCompareBlackboardNumberAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCheckBlackboardBoolAnimNode, 1, xiiRTTIDefaultAllocator<xiiCheckBlackboardBoolAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    XII_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("Check Bool: '{BlackboardEntry}'"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiCheckBlackboardBoolAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiCheckBlackboardBoolAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiCheckBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiCheckBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiCheckBlackboardBoolAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    xiiLog::Warning("AnimController::CheckBlackboardBool: '{}' doesn't exist or isn't a bool type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool    bValue     = value.ConvertTo<bool>();
  const xiiInt8 iIsTrueNow = bValue ? 1 : 0;

  m_OutBool.SetBool(ref_graph, bValue);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bValue)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool xiiCheckBlackboardBoolAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetBlackboardBoolAnimNode, 1, xiiRTTIDefaultAllocator<xiiSetBlackboardBoolAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    XII_MEMBER_PROPERTY("Bool", m_bBool),

    XII_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTitleAttribute("Set Bool: '{BlackboardEntry}' to {Bool}"),
    new xiiCategoryAttribute("Blackboard"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSetBlackboardBoolAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_bBool;

  XII_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSetBlackboardBoolAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_bBool;

  XII_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiSetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiSetBlackboardBoolAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InBool.GetBool(ref_graph, m_bBool));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGetBlackboardBoolAnimNode, 1, xiiRTTIDefaultAllocator<xiiGetBlackboardBoolAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    XII_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("Get Bool: '{BlackboardEntry}'"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGetBlackboardBoolAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiGetBlackboardBoolAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiGetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiGetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiGetBlackboardBoolAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  xiiVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    xiiLog::Warning("AnimController::GetBlackboardBool: '{}' doesn't exist or can't be converted to bool.", m_sBlackboardEntry);
    return;
  }

  m_OutBool.SetBool(ref_graph, value.ConvertTo<bool>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOnBlackboardValueChangedAnimNode, 1, xiiRTTIDefaultAllocator<xiiOnBlackboardValueChangedAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    XII_MEMBER_PROPERTY("OutOnValueChanged", m_OutOnValueChanged)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Blackboard"),
    new xiiTitleAttribute("OnChanged: '{BlackboardEntry}'"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiOnBlackboardValueChangedAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutOnValueChanged.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiOnBlackboardValueChangedAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  XII_SUCCEED_OR_RETURN(m_OutOnValueChanged.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiOnBlackboardValueChangedAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* xiiOnBlackboardValueChangedAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void xiiOnBlackboardValueChangedAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const xiiBlackboard::Entry* pEntry = pBlackboard->GetEntry(m_sBlackboardEntry);

  if (pEntry == nullptr)
  {
    xiiLog::Warning("AnimController::OnBlackboardValueChanged: '{}' doesn't exist.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_uiChangeCounter == pEntry->m_uiChangeCounter)
    return;

  if (pInstance->m_uiChangeCounter != xiiInvalidIndex)
  {
    m_OutOnValueChanged.SetTriggered(ref_graph);
  }

  pInstance->m_uiChangeCounter = pEntry->m_uiChangeCounter;
}

bool xiiOnBlackboardValueChangedAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
