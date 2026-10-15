/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
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

  struct Entry
  {
    xiiRenderGraphRegistrationDescription m_Description;
    xiiUniquePtr<xiiRenderGraph>          m_pGraph;
    xiiRenderGraphBlackboard              m_Blackboard;
    xiiRenderGraphManager::BuildDelegate  m_BuildDelegate;
    xiiUInt32                             m_uiRegistrationOrder = 0U;
    bool                                  m_bOnDemandRequested  = false;
  };

  xiiDynamicArray<xiiUniquePtr<Entry>>                       m_Entries;
  xiiIdTable<xiiRenderGraphContextId, xiiUniquePtr<Context>> m_Contexts;
  xiiUniquePtr<xiiRenderGraphResourceCache>                  m_pResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler>              m_pProfiler;
  xiiGpuFrameCompletionTracker                               m_FrameCompletionTracker;
  xiiMutex                                                   m_FrameMutex;
  xiiUInt64                                                  m_uiFrameIndex            = 0U;
  bool                                                       m_bFrameActive            = false;
  xiiUInt32                                                  m_uiNextRegistrationOrder = 0U;
  bool                                                       m_bEngineStarted          = false;
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
    xiiUniquePtr<xiiRenderGraphManagerState::Context>* pContext = nullptr;
    return state.m_Contexts.TryGetValue(handle.GetInternalID(), pContext) ? pContext->Borrow() : nullptr;
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

  return xiiRenderGraphContextHandle(s_pState->m_Contexts.Insert(std::move(pContext)));
}

void xiiRenderGraphManager::DestroyContext(xiiRenderGraphContextHandle handle)
{
  if (s_pState == nullptr || GetContext(*s_pState, handle) == nullptr)
    return;

  xiiRenderGraphManagerState::Context* pContext = GetContext(*s_pState, handle);
  pContext->m_pProfiler->Shutdown();
  pContext->m_pResourceCache->Shutdown();
  XII_VERIFY(s_pState->m_Contexts.Remove(handle.GetInternalID()), "The render graph context was validated before removal.");
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

xiiRenderGraphFrame::~xiiRenderGraphFrame() { End(); }
xiiResult xiiRenderGraphFrame::Begin(xiiUInt32 uiFramesInFlight) { return xiiRenderGraphManager::BeginFrame(*this, uiFramesInFlight); }
void      xiiRenderGraphFrame::End()
{
  if (IsActive())
    xiiRenderGraphManager::EndFrame(*this);
}

xiiResult xiiRenderGraphManager::BeginFrame(xiiRenderGraphFrame& frame, xiiUInt32 uiFramesInFlight)
{
  if (!IsInitialized() || frame.IsActive() || uiFramesInFlight == 0U)
    return XII_FAILURE;
  s_pState->m_FrameMutex.Lock();
  // Foundation mutexes are recursive; reject nested frames while other threads wait for End.
  if (s_pState->m_bFrameActive || s_pState->m_uiFrameIndex == xiiMath::MaxValue<xiiUInt64>())
  {
    s_pState->m_FrameMutex.Unlock();
    return XII_FAILURE;
  }
  // All global and context-local slices advance on this clock. Never reuse a shorter
  // ring while its previous occupant is still in flight, even if the caller permits more.
  if (xiiMaterialManager::IsInitialized())
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiMaterialManager::GetGpuStorage().GetFramesInFlight());
  if (xiiGeometryResidencyManager::IsInitialized())
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight);
  if (xiiRayTracingResources::IsInitialized())
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiRayTracingResources::GetConfiguration().m_uiFramesInFlight);
  if (xiiVirtualShadowMapManager::IsInitialized())
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiVirtualShadowMapManager::GetConfiguration().m_uiFramesInFlight);
  const xiiUInt32 uiVisibilityFrames = xiiGpuVisibilityManager::GetMinimumFramesInFlight();
  if (uiVisibilityFrames != 0U)
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, uiVisibilityFrames);

  s_pState->m_bFrameActive     = true;
  const xiiUInt64 uiFrameIndex = ++s_pState->m_uiFrameIndex;
  if (uiFrameIndex > uiFramesInFlight)
    s_pState->m_FrameCompletionTracker.WaitForFrame(uiFrameIndex - uiFramesInFlight);

  const xiiUInt64 uiCompletedFrame = s_pState->m_FrameCompletionTracker.PollCompletedFrames();
  if (xiiGALBindlessResourceTable::IsInitialized())
    xiiGALBindlessResourceTable::Collect(uiCompletedFrame);
  if (xiiMaterialManager::IsInitialized())
    xiiMaterialManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  if (xiiGpuVisibilityManager::IsInitialized())
    xiiGpuVisibilityManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  if (xiiVirtualShadowMapManager::IsInitialized())
    xiiVirtualShadowMapManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  frame.m_uiFrameIndex     = uiFrameIndex;
  frame.m_uiCompletedFrame = uiCompletedFrame;
  return XII_SUCCESS;
}

void xiiRenderGraphManager::EndFrame(xiiRenderGraphFrame& frame)
{
  XII_ASSERT_DEV(s_pState != nullptr && s_pState->m_bFrameActive && frame.m_uiFrameIndex == s_pState->m_uiFrameIndex, "A GPU frame must end before renderer shutdown.");
  s_pState->m_FrameCompletionTracker.CaptureSubmittedFrame(frame.m_uiFrameIndex);
  frame.m_uiFrameIndex     = 0U;
  frame.m_uiCompletedFrame = 0U;
  s_pState->m_bFrameActive = false;
  s_pState->m_FrameMutex.Unlock();
}

xiiResult xiiRenderGraphManager::ExecuteFrame(const xiiView* pView, xiiUInt32 uiFramesInFlight, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  xiiRenderGraphFrame frame;
  XII_SUCCEED_OR_RETURN(frame.Begin(uiFramesInFlight));
  return ExecuteFrame(frame, pView, settings, out_pError);
}

xiiResult xiiRenderGraphManager::ExecuteFrame(const xiiRenderGraphFrame& frame, const xiiView* pView, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  if (!IsInitialized() || !frame.IsActive() || frame.GetFrameIndex() != s_pState->m_uiFrameIndex)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || s_pState->m_pResourceCache == nullptr)
    return XII_FAILURE;

  const auto uiFrameIndex     = frame.GetFrameIndex();
  const auto uiCompletedFrame = frame.GetCompletedFrame();
  if (xiiGeometryResidencyManager::IsInitialized())
  {
    const xiiGeometryResidencyDescription& geometryDescription = xiiGeometryResidencyManager::GetConfiguration();
    xiiGeometryResidencyManager::ProcessStreaming(uiFrameIndex, uiCompletedFrame, geometryDescription.m_uiUploadBudgetPerFrameBytes);
  }

  s_pState->m_pResourceCache->BeginFrame(uiFrameIndex, uiCompletedFrame);
  const xiiResult result = ExecuteFrame(uiFrameIndex, pDevice.Borrow(), pView, s_pState->m_pResourceCache.Borrow(), s_pState->m_pProfiler.Borrow(), settings, out_pError);
  s_pState->m_pResourceCache->EndFrame();
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

  XII_ASSERT_DEV(!s_pState->m_bFrameActive, "End GPU frames before renderer shutdown.");

  s_pState->m_Entries.Clear();
  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pProfiler->Shutdown();
    it.Value()->m_pResourceCache->Shutdown();
  }
  s_pState->m_Contexts.Clear();
  s_pState->m_FrameCompletionTracker.Reset();
  s_pState->m_uiFrameIndex = 0U;
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
  return m_Handle.IsInvalidated() ? XII_FAILURE : XII_SUCCESS;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderGraphManager);
