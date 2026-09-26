/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>

class xiiRenderGraphManagerState
{
public:
  struct Entry
  {
    xiiRenderGraphRegistrationDescription m_Description;
    xiiUniquePtr<xiiRenderGraph>           m_pGraph;
    xiiRenderGraphBlackboard               m_Blackboard;
    xiiRenderGraphManager::BuildDelegate   m_BuildDelegate;
    xiiUInt32                              m_uiRegistrationOrder = 0U;
    bool                                   m_bOnDemandRequested  = false;
  };

  xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;
  xiiUniquePtr<xiiRenderGraphResourceCache> m_pResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  xiiGpuFrameCompletionTracker m_FrameCompletionTracker;
  xiiUInt32 m_uiNextRegistrationOrder = 0U;
  bool m_bEngineStarted = false;
};

xiiUniquePtr<xiiRenderGraphManagerState> xiiRenderGraphManager::s_pState;

namespace
{
  [[nodiscard]] xiiUInt32 FindEntry(const xiiRenderGraphManagerState& state, xiiRenderGraphGraphId id)
  {
    for (xiiUInt32 i = 0U; i < state.m_Entries.GetCount(); ++i)
    {
      if (state.m_Entries[i]->m_pGraph->GetId() == id)
        return i;
    }
    return xiiInvalidIndex;
  }

  [[nodiscard]] bool ShouldExecute(const xiiRenderGraphManagerState::Entry& entry, xiiUInt64 uiFrameIndex)
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
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderGraphManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderPassCache",
    "PipelineCache"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiRenderGraphManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRenderGraphManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRenderGraphManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRenderGraphManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

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
  XII_ASSERT_DEV(s_pState != nullptr, "Render graph manager is not started.");
  XII_ASSERT_DEV(!description.m_sName.IsEmpty(), "A render graph registration requires a name.");
  XII_ASSERT_DEV(buildDelegate.IsValid(), "A render graph registration requires a build delegate.");

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry = XII_DEFAULT_NEW(xiiRenderGraphManagerState::Entry);
  xiiRenderGraphManagerState::Entry& entry = *pEntry;
  entry.m_Description                  = description;
  entry.m_Description.m_uiEveryNFrames = xiiMath::Max(1U, description.m_uiEveryNFrames);
  entry.m_pGraph                       = XII_DEFAULT_NEW(xiiRenderGraph, description.m_sName);
  entry.m_BuildDelegate                = buildDelegate;
  entry.m_uiRegistrationOrder          = s_pState->m_uiNextRegistrationOrder++;
  const xiiRenderGraphGraphId id       = entry.m_pGraph->GetId();
  s_pState->m_Entries.PushBack(std::move(pEntry));
  return id;
}

bool xiiRenderGraphManager::UnregisterGraph(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  s_pState->m_Entries.RemoveAtAndSwap(uiIndex);
  return true;
}

bool xiiRenderGraphManager::RequestExecution(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  s_pState->m_Entries[uiIndex]->m_bOnDemandRequested = true;
  return true;
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  return uiIndex == xiiInvalidIndex ? nullptr : s_pState->m_Entries[uiIndex]->m_pGraph.Borrow();
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphGraphId id)
{
  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  return uiIndex == xiiInvalidIndex ? nullptr : &s_pState->m_Entries[uiIndex]->m_Blackboard;
}

xiiRenderGraphResourceCache* xiiRenderGraphManager::GetResourceCache()
{
  return s_pState != nullptr ? s_pState->m_pResourceCache.Borrow() : nullptr;
}

xiiRenderGraphTimestampProfiler* xiiRenderGraphManager::GetProfiler()
{
  return s_pState != nullptr ? s_pState->m_pProfiler.Borrow() : nullptr;
}

xiiUInt64 xiiRenderGraphManager::PrepareFrame(xiiUInt64 uiFrameIndex, xiiUInt32 uiFramesInFlight)
{
  XII_ASSERT_DEV(s_pState != nullptr && s_pState->m_bEngineStarted, "Render graph manager is not ready to prepare GPU frames.");
  XII_ASSERT_DEV(uiFramesInFlight > 0U, "At least one frame in flight is required.");
  if (s_pState == nullptr || !s_pState->m_bEngineStarted || uiFramesInFlight == 0U)
    return 0ULL;

  if (uiFrameIndex > uiFramesInFlight)
    s_pState->m_FrameCompletionTracker.WaitForFrame(uiFrameIndex - uiFramesInFlight);
  return s_pState->m_FrameCompletionTracker.PollCompletedFrames();
}

xiiResult xiiRenderGraphManager::ExecuteFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, const xiiView* pView, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  if (s_pState == nullptr || !s_pState->m_bEngineStarted)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || s_pState->m_pResourceCache == nullptr)
    return XII_FAILURE;

  s_pState->m_pResourceCache->BeginFrame(uiFrameIndex, uiCompletedFrame);
  const xiiResult result = ExecuteFrame(uiFrameIndex, pDevice.Borrow(), pView, s_pState->m_pResourceCache.Borrow(), s_pState->m_pProfiler.Borrow(), settings, out_pError);
  s_pState->m_pResourceCache->EndFrame();
  // Capture every attempted frame so ring-slot waiting remains sequential even if graph
  // compilation fails before submitting new work. In that case the previous queue values make
  // the frame immediately complete without introducing a hole in the tracker timeline.
  s_pState->m_FrameCompletionTracker.CaptureSubmittedFrame(uiFrameIndex);
  return result;
}

xiiResult xiiRenderGraphManager::ExecuteFrame(xiiUInt64 uiFrameIndex, xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(pDevice != nullptr && pResourceCache != nullptr, "Render graph manager requires a device and resource cache.");

  xiiDynamicArray<xiiUInt32> executionOrder;
  XII_ASSERT_DEV(s_pState != nullptr, "Render graph manager is not started.");
  for (xiiUInt32 i = 0U; i < s_pState->m_Entries.GetCount(); ++i)
  {
    if (ShouldExecute(*s_pState->m_Entries[i], uiFrameIndex))
      executionOrder.PushBack(i);
  }

  executionOrder.Sort([](xiiUInt32 lhs, xiiUInt32 rhs) {
    const xiiRenderGraphManagerState::Entry& a = *s_pState->m_Entries[lhs];
    const xiiRenderGraphManagerState::Entry& b = *s_pState->m_Entries[rhs];
    if (a.m_Description.m_Category != b.m_Description.m_Category)
      return a.m_Description.m_Category < b.m_Description.m_Category;
    if (a.m_Description.m_iPriority != b.m_Description.m_iPriority)
      return a.m_Description.m_iPriority < b.m_Description.m_iPriority;
    return a.m_uiRegistrationOrder < b.m_uiRegistrationOrder;
  });

  for (xiiUInt32 uiEntryIndex : executionOrder)
  {
    xiiRenderGraphManagerState::Entry& entry = *s_pState->m_Entries[uiEntryIndex];
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

void xiiRenderGraphManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Render graph manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiRenderGraphManagerState);
}

void xiiRenderGraphManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede render graph engine startup.");
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return;

  s_pState->m_pResourceCache = XII_DEFAULT_NEW(xiiRenderGraphResourceCache);
  s_pState->m_pProfiler = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler);
  s_pState->m_pResourceCache->Initialize(pDevice);
  s_pState->m_pProfiler->Initialize(pDevice);
  s_pState->m_FrameCompletionTracker.Initialize(pDevice.Borrow());
  s_pState->m_bEngineStarted = true;
}

void xiiRenderGraphManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_Entries.Clear();
  s_pState->m_FrameCompletionTracker.Reset();
  if (s_pState->m_pProfiler != nullptr)
    s_pState->m_pProfiler->Shutdown();
  if (s_pState->m_pResourceCache != nullptr)
    s_pState->m_pResourceCache->Shutdown();
  s_pState->m_pProfiler.Clear();
  s_pState->m_pResourceCache.Clear();
  s_pState->m_bEngineStarted = false;
}

void xiiRenderGraphManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}
