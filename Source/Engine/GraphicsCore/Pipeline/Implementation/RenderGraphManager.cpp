/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
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
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

#include <utility>

//////////////////////////////////////////////////////////////////////////

class xiiRenderGraphManagerState
{
public:
  struct Entry
  {
    xiiUniquePtr<xiiRenderGraph>         m_pGraph;
    xiiRenderGraphBlackboard             m_Blackboard;
    xiiRenderGraphManager::BuildDelegate m_BuildDelegate;
  };

  using GraphTable = xiiIdTable<xiiRenderGraphHandle::IdType, xiiUniquePtr<Entry>>;

public:
  xiiRenderGraphManagerState() :
    m_Allocator("RenderGraphManager", xiiFoundation::GetDefaultAllocator()), m_GraphTable(&m_Allocator), m_RetiredEntries(&m_Allocator)
  {
  }

  ~xiiRenderGraphManagerState() = default;

  Entry* FindEntry(xiiRenderGraphHandle hGraph)
  {
    xiiUniquePtr<Entry>* pEntry = nullptr;
    return m_GraphTable.TryGetValue(hGraph.GetInternalID(), pEntry) ? pEntry->Borrow() : nullptr;
  }
  /// Returns the allocator used by the render graph manager state.
  XII_ALWAYS_INLINE xiiAllocator* GetAllocator() { return &m_Allocator; }

public:
  xiiProxyAllocator m_Allocator;

  // The frame mutex protects graph lifetime against execution. Graph table operations also use this mutex.
  xiiMutex m_FrameMutex;

  GraphTable m_GraphTable;

  // Removed handles cannot be resolved, but their resources may still be referenced by the active frame.
  xiiDynamicArray<xiiUniquePtr<Entry>> m_RetiredEntries;

  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  xiiGpuFrameCompletionTracker                  m_FrameCompletionTracker;

  xiiUInt64 m_uiFrameIndex = 0U;
  bool      m_bFrameActive = false;
};

xiiUniquePtr<xiiRenderGraphManagerState> xiiRenderGraphManager::s_pState;

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

//////////////////////////////////////////////////////////////////////////

xiiRenderGraphFrame::xiiRenderGraphFrame() = default;

xiiRenderGraphFrame::~xiiRenderGraphFrame()
{
  End();
}

xiiResult xiiRenderGraphFrame::Begin(xiiUInt32 uiFramesInFlight)
{
  return xiiRenderGraphManager::BeginFrame(*this, uiFramesInFlight);
}

void xiiRenderGraphFrame::End()
{
  if (IsActive())
  {
    xiiRenderGraphManager::EndFrame(*this);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiRenderGraphHandle xiiRenderGraphManager::CreateGraph(xiiStringView sName, BuildDelegate buildDelegate)
{
  if (s_pState == nullptr || !buildDelegate.IsValid())
    return {};

  XII_LOCK(s_pState->m_FrameMutex);

  if (s_pState->m_pResourceCache == nullptr)
    return {};

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphManagerState::Entry);
  pEntry->m_pGraph                                       = XII_NEW(s_pState->GetAllocator(), xiiRenderGraph, sName);
  pEntry->m_BuildDelegate                                = buildDelegate;

  return xiiRenderGraphHandle(s_pState->m_GraphTable.Insert(std::move(pEntry)));
}

xiiResult xiiRenderGraphManager::DestroyGraph(xiiRenderGraphHandle hGraph)
{
  if (s_pState == nullptr || hGraph.IsInvalidated())
    return XII_FAILURE;

  XII_LOCK(s_pState->m_FrameMutex);

  if (!s_pState->m_GraphTable.Contains(hGraph))
    return XII_FAILURE;

  if (!s_pState->m_bFrameActive && s_pState->m_uiFrameIndex != 0U)
    s_pState->m_FrameCompletionTracker.WaitForFrame(s_pState->m_uiFrameIndex);

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry;
  XII_VERIFY(s_pState->m_GraphTable.Remove(hGraph, &pEntry), "Failed to remove a managed render graph.");
  if (s_pState->m_bFrameActive)
    s_pState->m_RetiredEntries.PushBack(std::move(pEntry));
  return XII_SUCCESS;
}

xiiResult xiiRenderGraphManager::ExecuteGraph(const xiiRenderGraphFrame& frame, xiiRenderGraphHandle hGraph, const xiiView* pView, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  if (s_pState == nullptr || hGraph.IsInvalidated() || !frame.IsActive())
    return XII_FAILURE;

  XII_LOCK(s_pState->m_FrameMutex);

  if (!s_pState->m_bFrameActive || frame.GetFrameIndex() != s_pState->m_uiFrameIndex)
    return XII_FAILURE;

  xiiRenderGraphManagerState::Entry* pEntry = s_pState->FindEntry(hGraph);
  if (pEntry == nullptr)
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || s_pState->m_pResourceCache == nullptr)
    return XII_FAILURE;

  pEntry->m_Blackboard.ClearFrame();
  pEntry->m_pGraph->BeginSetup(frame.GetFrameIndex());
  pEntry->m_pGraph->SetSetupBlackboard(&pEntry->m_Blackboard);
  const xiiResult buildResult = pEntry->m_BuildDelegate(*pEntry->m_pGraph, pEntry->m_Blackboard, pView);
  pEntry->m_pGraph->EndSetup();
  if (buildResult.Failed())
  {
    if (out_pError)
      *out_pError = "The graph build delegate failed; see the preceding operation diagnostics.";
    return XII_FAILURE;
  }

  if (pEntry->m_pGraph->Compile(settings, out_pError).Failed())
    return XII_FAILURE;

  if (pEntry->m_pGraph->Execute(pDevice.Borrow(), pView, &pEntry->m_Blackboard, s_pState->m_pResourceCache.Borrow(), s_pState->m_pProfiler.Borrow(), out_pError).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphHandle hGraph)
{
  if (s_pState == nullptr)
    return nullptr;

  XII_LOCK(s_pState->m_FrameMutex);
  xiiRenderGraphManagerState::Entry* pEntry = s_pState->FindEntry(hGraph);
  return pEntry != nullptr ? pEntry->m_pGraph.Borrow() : nullptr;
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphHandle hGraph)
{
  if (s_pState == nullptr)
    return nullptr;

  XII_LOCK(s_pState->m_FrameMutex);
  xiiRenderGraphManagerState::Entry* pEntry = s_pState->FindEntry(hGraph);
  return pEntry != nullptr ? &pEntry->m_Blackboard : nullptr;
}

xiiRenderGraphResourceCache* xiiRenderGraphManager::GetResourceCache()
{
  return s_pState != nullptr ? s_pState->m_pResourceCache.Borrow() : nullptr;
}

xiiRenderGraphTimestampProfiler* xiiRenderGraphManager::GetProfiler()
{
  return s_pState != nullptr ? s_pState->m_pProfiler.Borrow() : nullptr;
}

xiiResult xiiRenderGraphManager::GetFrameCompletion(xiiUInt64 uiFrameIndex, xiiUInt64& out_uiCompletedFrame)
{
  out_uiCompletedFrame = 0U;

  if (s_pState == nullptr)
    return XII_FAILURE;

  XII_LOCK(s_pState->m_FrameMutex);

  if (!s_pState->m_bFrameActive || uiFrameIndex == 0U || uiFrameIndex != s_pState->m_uiFrameIndex)
    return XII_FAILURE;

  out_uiCompletedFrame = s_pState->m_FrameCompletionTracker.GetLastCompletedFrame();
  return XII_SUCCESS;
}

xiiResult xiiRenderGraphManager::BeginFrame(xiiRenderGraphFrame& frame, xiiUInt32 uiFramesInFlight)
{
  if (s_pState == nullptr || frame.IsActive() || uiFramesInFlight == 0U)
    return XII_FAILURE;

  s_pState->m_FrameMutex.Lock();

  // Foundation mutexes are recursive; reject nested frames while other threads wait for End.
  if (s_pState->m_bFrameActive || s_pState->m_uiFrameIndex == xiiMath::MaxValue<xiiUInt64>() || s_pState->m_pResourceCache == nullptr)
  {
    s_pState->m_FrameMutex.Unlock();
    return XII_FAILURE;
  }

  // All device-wide buffer rings advance on this clock.
  // Never reuse a shorter ring while its previous occupant is still in flight.
  if (xiiMaterialManager::IsInitialized())
  {
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiMaterialManager::GetGpuStorage().GetFramesInFlight());
  }
  if (xiiGeometryResidencyManager::IsInitialized())
  {
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight);
  }
  if (xiiRayTracingResources::IsInitialized())
  {
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiRayTracingResources::GetConfiguration().m_uiFramesInFlight);
  }
  if (xiiVirtualShadowMapManager::IsInitialized())
  {
    uiFramesInFlight = xiiMath::Min(uiFramesInFlight, xiiVirtualShadowMapManager::GetConfiguration().m_uiFramesInFlight);
  }

  if (uiFramesInFlight == 0U)
  {
    s_pState->m_FrameMutex.Unlock();
    return XII_FAILURE;
  }

  s_pState->m_bFrameActive     = true;
  const xiiUInt64 uiFrameIndex = ++s_pState->m_uiFrameIndex;

  if (uiFrameIndex > uiFramesInFlight)
  {
    s_pState->m_FrameCompletionTracker.WaitForFrame(uiFrameIndex - uiFramesInFlight);
  }

  const xiiUInt64 uiCompletedFrame = s_pState->m_FrameCompletionTracker.PollCompletedFrames();
  if (xiiGALBindlessResourceTable::IsInitialized())
  {
    xiiGALBindlessResourceTable::Collect(uiCompletedFrame);
  }
  if (xiiMaterialManager::IsInitialized())
  {
    xiiMaterialManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  }
  if (xiiVirtualShadowMapManager::IsInitialized())
  {
    xiiVirtualShadowMapManager::BeginFrame(uiFrameIndex, uiCompletedFrame);
  }
  if (xiiGeometryResidencyManager::IsInitialized())
  {
    const xiiGeometryResidencyDescription& geometryDescription = xiiGeometryResidencyManager::GetConfiguration();

    xiiGeometryResidencyManager::ProcessStreaming(uiFrameIndex, uiCompletedFrame, geometryDescription.m_uiUploadBudgetPerFrameBytes);
  }

  // Resource-cache frame scope matches the shared GPU frame, not an individual view or graph.
  s_pState->m_pResourceCache->BeginFrame(uiFrameIndex, uiCompletedFrame);

  frame.m_uiFrameIndex     = uiFrameIndex;
  frame.m_uiCompletedFrame = uiCompletedFrame;

  return XII_SUCCESS;
}

void xiiRenderGraphManager::EndFrame(xiiRenderGraphFrame& frame)
{
  XII_ASSERT_DEV(s_pState != nullptr && s_pState->m_bFrameActive && frame.m_uiFrameIndex == s_pState->m_uiFrameIndex, "A GPU frame must end before renderer shutdown.");

  s_pState->m_pResourceCache->EndFrame();
  s_pState->m_FrameCompletionTracker.CaptureSubmittedFrame(frame.m_uiFrameIndex);

  if (!s_pState->m_RetiredEntries.IsEmpty())
  {
    s_pState->m_FrameCompletionTracker.WaitForFrame(frame.m_uiFrameIndex);
    s_pState->m_RetiredEntries.Clear();
  }

  frame.m_uiFrameIndex     = 0U;
  frame.m_uiCompletedFrame = 0U;
  s_pState->m_bFrameActive = false;
  s_pState->m_FrameMutex.Unlock();
}

void xiiRenderGraphManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Render graph manager started twice.");

  s_pState = XII_DEFAULT_NEW(xiiRenderGraphManagerState);
}

void xiiRenderGraphManager::EngineStartup()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Cannot initialize render graph manager without a valid GAL device.");

  s_pState->m_pResourceCache = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphResourceCache, pDevice);
  s_pState->m_pProfiler      = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphTimestampProfiler, pDevice);

  s_pState->m_FrameCompletionTracker.Initialize(pDevice.Borrow());
}

void xiiRenderGraphManager::EngineShutdown()
{
  XII_ASSERT_DEV(!s_pState->m_bFrameActive, "End GPU frames before renderer shutdown.");

  if (s_pState->m_uiFrameIndex != 0U)
  {
    s_pState->m_FrameCompletionTracker.WaitForFrame(s_pState->m_uiFrameIndex);
  }

  if (!s_pState->m_GraphTable.IsEmpty())
  {
    xiiLog::Warning("Render graph manager leak detected: {0} graphs were not destroyed.", s_pState->m_GraphTable.GetCount());

    for (auto it = s_pState->m_GraphTable.GetIterator(); it.IsValid(); ++it)
    {
      xiiLog::Warning("  Graph '{0}' was not destroyed.", it.Value()->m_pGraph->GetName());
    }
  }

  s_pState->m_GraphTable.Clear();
  s_pState->m_RetiredEntries.Clear();
  s_pState->m_FrameCompletionTracker.Reset();
  s_pState->m_pProfiler.Clear();
  s_pState->m_pResourceCache.Clear();

  s_pState->m_uiFrameIndex = 0U;
}

void xiiRenderGraphManager::Shutdown()
{
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderGraphManager);
