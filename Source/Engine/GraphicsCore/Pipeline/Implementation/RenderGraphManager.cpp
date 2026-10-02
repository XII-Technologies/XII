/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

class xiiRenderGraphManagerState
{
public:
  struct Context
  {
    xiiUniquePtr<xiiRenderGraph>                  m_pGraph;
    xiiRenderGraphBlackboard                      m_Blackboard;
    xiiUniquePtr<xiiRenderGraphResourceCache>     m_pResourceCache;
    xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  };

  struct ContextSlot
  {
    xiiUniquePtr<Context> m_pContext;
    xiiUInt32             m_uiGeneration = 1U;
  };

  struct Entry
  {
    xiiRenderGraphRegistrationDescription m_Description;
    xiiUniquePtr<xiiRenderGraph>          m_pGraph;
    xiiRenderGraphBlackboard              m_Blackboard;
    xiiRenderGraphManager::BuildDelegate  m_BuildDelegate;
    xiiUInt32                             m_uiRegistrationOrder = 0U;
    bool                                  m_bOnDemandRequested  = false;
  };

  xiiDynamicArray<xiiUniquePtr<Entry>>          m_Entries;
  xiiDynamicArray<ContextSlot>                  m_ContextSlots;
  xiiDynamicArray<xiiUInt32>                    m_FreeContextSlots;
  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  xiiGpuFrameCompletionTracker                  m_FrameCompletionTracker;
  xiiUInt32                                     m_uiNextRegistrationOrder = 0U;
  bool                                          m_bEngineStarted          = false;
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

  [[nodiscard]] xiiRenderGraphManagerState::Context* GetContext(xiiRenderGraphManagerState& state, xiiRenderGraphContextHandle handle)
  {
    if (!handle.IsValid() || handle.m_uiIndex >= state.m_ContextSlots.GetCount())
      return nullptr;

    xiiRenderGraphManagerState::ContextSlot& slot = state.m_ContextSlots[handle.m_uiIndex];
    return slot.m_uiGeneration == handle.m_uiGeneration ? slot.m_pContext.Borrow() : nullptr;
  }
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderGraphManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "GeometryResidencyManager",
    "MaterialManager",
    "VirtualShadowMapManager",
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderGraphContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRenderGraphContextHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

bool xiiRenderGraphManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiRenderGraphManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiRenderGraphContextHandle xiiRenderGraphManager::CreateContext(xiiStringView sName)
{
  XII_ASSERT_DEV(IsInitialized(), "The render graph manager is not initialized.");
  if (!IsInitialized())
    return {};

  xiiSharedPtr<xiiGALDevice>                        pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiUniquePtr<xiiRenderGraphManagerState::Context> pContext = XII_DEFAULT_NEW(xiiRenderGraphManagerState::Context);
  pContext->m_pGraph                                         = XII_DEFAULT_NEW(xiiRenderGraph, sName);
  pContext->m_pResourceCache                                 = XII_DEFAULT_NEW(xiiRenderGraphResourceCache);
  pContext->m_pProfiler                                      = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler);
  pContext->m_pResourceCache->Initialize(pDevice);
  pContext->m_pProfiler->Initialize(pDevice);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  if (!s_pState->m_FreeContextSlots.IsEmpty())
  {
    uiIndex = s_pState->m_FreeContextSlots.PeekBack();
    s_pState->m_FreeContextSlots.PopBack();
  }
  else
  {
    uiIndex = s_pState->m_ContextSlots.GetCount();
    s_pState->m_ContextSlots.ExpandAndGetRef();
  }

  xiiRenderGraphManagerState::ContextSlot& slot = s_pState->m_ContextSlots[uiIndex];
  slot.m_pContext                               = std::move(pContext);

  xiiRenderGraphContextHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiRenderGraphManager::DestroyContext(xiiRenderGraphContextHandle handle)
{
  if (s_pState == nullptr || GetContext(*s_pState, handle) == nullptr)
    return;

  xiiRenderGraphManagerState::ContextSlot& slot = s_pState->m_ContextSlots[handle.m_uiIndex];
  slot.m_pContext->m_pProfiler->Shutdown();
  slot.m_pContext->m_pResourceCache->Shutdown();
  slot.m_pContext.Clear();
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeContextSlots.PushBack(handle.m_uiIndex);
}

bool xiiRenderGraphManager::IsValid(xiiRenderGraphContextHandle handle)
{
  return s_pState != nullptr && GetContext(*s_pState, handle) != nullptr;
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphContextHandle handle)
{
  xiiRenderGraphManagerState::Context* pContext = s_pState != nullptr ? GetContext(*s_pState, handle) : nullptr;
  return pContext != nullptr ? pContext->m_pGraph.Borrow() : nullptr;
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphContextHandle handle)
{
  xiiRenderGraphManagerState::Context* pContext = s_pState != nullptr ? GetContext(*s_pState, handle) : nullptr;
  return pContext != nullptr ? &pContext->m_Blackboard : nullptr;
}

xiiRenderGraphResourceCache* xiiRenderGraphManager::GetResourceCache(xiiRenderGraphContextHandle handle)
{
  xiiRenderGraphManagerState::Context* pContext = s_pState != nullptr ? GetContext(*s_pState, handle) : nullptr;
  return pContext != nullptr ? pContext->m_pResourceCache.Borrow() : nullptr;
}

xiiRenderGraphTimestampProfiler* xiiRenderGraphManager::GetProfiler(xiiRenderGraphContextHandle handle)
{
  xiiRenderGraphManagerState::Context* pContext = s_pState != nullptr ? GetContext(*s_pState, handle) : nullptr;
  return pContext != nullptr ? pContext->m_pProfiler.Borrow() : nullptr;
}

xiiRenderGraphGraphId xiiRenderGraphManager::RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Render graph manager is not started.");
  XII_ASSERT_DEV(!description.m_sName.IsEmpty(), "A render graph registration requires a name.");
  XII_ASSERT_DEV(buildDelegate.IsValid(), "A render graph registration requires a build delegate.");
  if (s_pState == nullptr || description.m_sName.IsEmpty() || !buildDelegate.IsValid())
    return {};

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry = XII_DEFAULT_NEW(xiiRenderGraphManagerState::Entry);
  xiiRenderGraphManagerState::Entry&              entry  = *pEntry;
  entry.m_Description                                    = description;
  entry.m_Description.m_uiEveryNFrames                   = xiiMath::Max(1U, description.m_uiEveryNFrames);
  entry.m_pGraph                                         = XII_DEFAULT_NEW(xiiRenderGraph, description.m_sName);
  entry.m_BuildDelegate                                  = buildDelegate;
  entry.m_uiRegistrationOrder                            = s_pState->m_uiNextRegistrationOrder++;
  const xiiRenderGraphGraphId id                         = entry.m_pGraph->GetId();
  s_pState->m_Entries.PushBack(std::move(pEntry));
  return id;
}

bool xiiRenderGraphManager::UnregisterGraph(xiiRenderGraphGraphId id)
{
  if (s_pState == nullptr || !id.IsValid())
    return false;

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  s_pState->m_Entries.RemoveAtAndSwap(uiIndex);
  return true;
}

bool xiiRenderGraphManager::RequestExecution(xiiRenderGraphGraphId id)
{
  if (s_pState == nullptr || !id.IsValid())
    return false;

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  if (uiIndex == xiiInvalidIndex)
    return false;
  s_pState->m_Entries[uiIndex]->m_bOnDemandRequested = true;
  return true;
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphGraphId id)
{
  if (s_pState == nullptr || !id.IsValid())
    return nullptr;

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  return uiIndex == xiiInvalidIndex ? nullptr : s_pState->m_Entries[uiIndex]->m_pGraph.Borrow();
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphGraphId id)
{
  if (s_pState == nullptr || !id.IsValid())
    return nullptr;

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

  XII_ASSERT_DEV(uiFrameIndex > 0U, "Frame zero is reserved as the no-completed-frame sentinel.");
  if (uiFrameIndex == 0U)
    return 0ULL;

  if (uiFrameIndex > uiFramesInFlight)
    s_pState->m_FrameCompletionTracker.WaitForFrame(uiFrameIndex - uiFramesInFlight);

  const xiiUInt64 uiCompletedFrame = s_pState->m_FrameCompletionTracker.PollCompletedFrames();
  if (xiiGALBindlessResourceTable::IsInitialized())
    xiiGALBindlessResourceTable::Collect(uiCompletedFrame);
  if (xiiMaterialManager::IsInitialized())
    xiiMaterialManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  if (xiiVirtualShadowMapManager::IsInitialized())
    xiiVirtualShadowMapManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  return uiCompletedFrame;
}

xiiResult xiiRenderGraphManager::ExecuteFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, const xiiView* pView, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  if (s_pState == nullptr || !s_pState->m_bEngineStarted)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || s_pState->m_pResourceCache == nullptr)
    return XII_FAILURE;

  if (xiiGeometryResidencyManager::IsInitialized())
  {
    const xiiGeometryResidencyDescription& geometryDescription = xiiGeometryResidencyManager::GetConfiguration();
    xiiGeometryResidencyManager::ProcessStreaming(uiFrameIndex, uiCompletedFrame, geometryDescription.m_uiUploadBudgetPerFrameBytes);
  }

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
  XII_ASSERT_DEV(s_pState != nullptr && s_pState->m_bEngineStarted, "Render graph manager is not initialized.");
  if (s_pState == nullptr || !s_pState->m_bEngineStarted || pDevice == nullptr || pResourceCache == nullptr)
    return XII_FAILURE;

  xiiDynamicArray<xiiUInt32> executionOrder;
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
  s_pState->m_pProfiler      = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler);
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
  for (xiiUInt32 uiIndex = 0U; uiIndex < s_pState->m_ContextSlots.GetCount(); ++uiIndex)
  {
    xiiRenderGraphManagerState::ContextSlot& slot = s_pState->m_ContextSlots[uiIndex];
    if (slot.m_pContext == nullptr)
      continue;

    slot.m_pContext->m_pProfiler->Shutdown();
    slot.m_pContext->m_pResourceCache->Shutdown();
    slot.m_pContext.Clear();
    ++slot.m_uiGeneration;
    if (slot.m_uiGeneration == 0U)
      slot.m_uiGeneration = 1U;
    s_pState->m_FreeContextSlots.PushBack(uiIndex);
  }
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

xiiRenderGraphContext::~xiiRenderGraphContext()
{
  Shutdown();
}

xiiResult xiiRenderGraphContext::Initialize(xiiStringView sName)
{
  Shutdown();
  m_Handle = xiiRenderGraphManager::CreateContext(sName);
  return m_Handle.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiRenderGraphContext::Shutdown()
{
  xiiRenderGraphManager::DestroyContext(m_Handle);
  m_Handle = {};
}

bool xiiRenderGraphContext::IsInitialized() const
{
  return xiiRenderGraphManager::IsValid(m_Handle);
}

xiiRenderGraph& xiiRenderGraphContext::GetGraph()
{
  xiiRenderGraph* pGraph = xiiRenderGraphManager::GetGraph(m_Handle);
  XII_ASSERT_DEV(pGraph != nullptr, "The render graph context is not initialized or was invalidated by subsystem shutdown.");
  return *pGraph;
}

const xiiRenderGraph& xiiRenderGraphContext::GetGraph() const
{
  return const_cast<xiiRenderGraphContext*>(this)->GetGraph();
}

xiiRenderGraphBlackboard& xiiRenderGraphContext::GetBlackboard()
{
  xiiRenderGraphBlackboard* pBlackboard = xiiRenderGraphManager::GetBlackboard(m_Handle);
  XII_ASSERT_DEV(pBlackboard != nullptr, "The render graph context is not initialized or was invalidated by subsystem shutdown.");
  return *pBlackboard;
}

const xiiRenderGraphBlackboard& xiiRenderGraphContext::GetBlackboard() const
{
  return const_cast<xiiRenderGraphContext*>(this)->GetBlackboard();
}

xiiRenderGraphResourceCache& xiiRenderGraphContext::GetResourceCache()
{
  xiiRenderGraphResourceCache* pCache = xiiRenderGraphManager::GetResourceCache(m_Handle);
  XII_ASSERT_DEV(pCache != nullptr, "The render graph context is not initialized or was invalidated by subsystem shutdown.");
  return *pCache;
}

const xiiRenderGraphResourceCache& xiiRenderGraphContext::GetResourceCache() const
{
  return const_cast<xiiRenderGraphContext*>(this)->GetResourceCache();
}

xiiRenderGraphTimestampProfiler& xiiRenderGraphContext::GetProfiler()
{
  xiiRenderGraphTimestampProfiler* pProfiler = xiiRenderGraphManager::GetProfiler(m_Handle);
  XII_ASSERT_DEV(pProfiler != nullptr, "The render graph context is not initialized or was invalidated by subsystem shutdown.");
  return *pProfiler;
}
