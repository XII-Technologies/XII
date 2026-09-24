/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderGraphManager.h>

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderGraphCategory, 1)
  XII_ENUM_CONSTANT(xiiRenderGraphCategory::ScenePreparation),
    XII_ENUM_CONSTANT(xiiRenderGraphCategory::SceneRendering),
    XII_ENUM_CONSTANT(xiiRenderGraphCategory::ScenePostProcess),
    XII_ENUM_CONSTANT(xiiRenderGraphCategory::Output),
    XII_ENUM_CONSTANT(xiiRenderGraphCategory::Background),
    XII_ENUM_CONSTANT(xiiRenderGraphCategory::AsyncCompute),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderGraphFrequency, 1)
  XII_ENUM_CONSTANT(xiiRenderGraphFrequency::EveryFrame),
    XII_ENUM_CONSTANT(xiiRenderGraphFrequency::EveryNFrames),
    XII_ENUM_CONSTANT(xiiRenderGraphFrequency::OnDemand),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderGraphRegistrationDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderGraphRegistrationDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Name", m_sName),
      XII_ENUM_MEMBER_PROPERTY("Category", xiiRenderGraphCategory, m_Category),
      XII_ENUM_MEMBER_PROPERTY("Frequency", xiiRenderGraphFrequency, m_Frequency),
      XII_MEMBER_PROPERTY("Priority", m_iPriority),
      XII_MEMBER_PROPERTY("EveryNFrames", m_uiEveryNFrames)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, 100000U)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiRenderGraphGraphId xiiRenderGraphManager::RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate)
{
  XII_ASSERT_DEV(!description.m_sName.IsEmpty(), "A render graph registration requires a name.");
  XII_ASSERT_DEV(buildDelegate.IsValid(), "A render graph registration requires a build delegate.");

  xiiUniquePtr<Entry> pEntry           = XII_DEFAULT_NEW(Entry);
  Entry&              entry            = *pEntry;
  entry.m_Description                  = description;
  entry.m_Description.m_uiEveryNFrames = xiiMath::Max(1U, description.m_uiEveryNFrames);
  entry.m_pGraph                       = XII_DEFAULT_NEW(xiiRenderGraph, description.m_sName);
  entry.m_BuildDelegate                = buildDelegate;
  entry.m_uiRegistrationOrder          = m_uiNextRegistrationOrder++;
  const xiiRenderGraphGraphId id       = entry.m_pGraph->GetId();
  m_Entries.PushBack(std::move(pEntry));
  return id;
}

bool xiiRenderGraphManager::UnregisterGraph(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  m_Entries.RemoveAtAndSwap(uiIndex);
  return true;
}

bool xiiRenderGraphManager::RequestExecution(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  m_Entries[uiIndex]->m_bOnDemandRequested = true;
  return true;
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(id);
  return uiIndex == xiiInvalidIndex ? nullptr : m_Entries[uiIndex]->m_pGraph.Borrow();
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(id);
  return uiIndex == xiiInvalidIndex ? nullptr : &m_Entries[uiIndex]->m_Blackboard;
}

xiiResult xiiRenderGraphManager::ExecuteFrame(xiiUInt64 uiFrameIndex, xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(pDevice != nullptr && pResourceCache != nullptr, "Render graph manager requires a device and resource cache.");

  xiiDynamicArray<xiiUInt32> executionOrder;
  for (xiiUInt32 i = 0U; i < m_Entries.GetCount(); ++i)
  {
    if (ShouldExecute(*m_Entries[i], uiFrameIndex))
      executionOrder.PushBack(i);
  }

  executionOrder.Sort([this](xiiUInt32 lhs, xiiUInt32 rhs) {
    const Entry& a = *m_Entries[lhs];
    const Entry& b = *m_Entries[rhs];
    if (a.m_Description.m_Category != b.m_Description.m_Category)
      return a.m_Description.m_Category < b.m_Description.m_Category;
    if (a.m_Description.m_iPriority != b.m_Description.m_iPriority)
      return a.m_Description.m_iPriority < b.m_Description.m_iPriority;
    return a.m_uiRegistrationOrder < b.m_uiRegistrationOrder;
  });

  for (xiiUInt32 uiEntryIndex : executionOrder)
  {
    Entry& entry = *m_Entries[uiEntryIndex];
    entry.m_Blackboard.ClearFrame();
    entry.m_pGraph->BeginSetup(uiFrameIndex);
    entry.m_BuildDelegate(*entry.m_pGraph, entry.m_Blackboard);
    entry.m_pGraph->EndSetup();

    if (entry.m_pGraph->Compile(settings, out_pError).Failed())
      return XII_FAILURE;
    if (entry.m_pGraph->Execute(pDevice, pView, &entry.m_Blackboard, pResourceCache, pProfiler, out_pError).Failed())
      return XII_FAILURE;

    entry.m_bOnDemandRequested = false;
  }
  return XII_SUCCESS;
}

xiiUInt32 xiiRenderGraphManager::FindEntry(xiiRenderGraphGraphId id) const
{
  for (xiiUInt32 i = 0U; i < m_Entries.GetCount(); ++i)
  {
    if (m_Entries[i]->m_pGraph->GetId() == id)
      return i;
  }
  return xiiInvalidIndex;
}

bool xiiRenderGraphManager::ShouldExecute(const Entry& entry, xiiUInt64 uiFrameIndex)
{
  switch (entry.m_Description.m_Frequency.GetValue())
  {
    case xiiRenderGraphFrequency::EveryFrame:
      return true;
    case xiiRenderGraphFrequency::EveryNFrames:
      return (uiFrameIndex % entry.m_Description.m_uiEveryNFrames) == 0ULL;
    case xiiRenderGraphFrequency::OnDemand:
      return entry.m_bOnDemandRequested;
    default:
      return false;
  }
}
