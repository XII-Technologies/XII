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

  using GraphTable = xiiIdTable<xiiRenderGraphHandle::IdType, Entry*, xiiLocalAllocatorWrapper>;

public:
  xiiRenderGraphManagerState() :
    m_Allocator("RenderGraphManager", xiiFoundation::GetDefaultAllocator()), m_AllocatorWrapper(&m_Allocator), m_Entries(&m_Allocator)
  {
  }

  ~xiiRenderGraphManagerState() = default;

  /// Returns the allocator used by the render graph manager state.
  XII_ALWAYS_INLINE xiiAllocator* GetAllocator() { return &m_Allocator; }

public:
  xiiProxyAllocator        m_Allocator;
  xiiLocalAllocatorWrapper m_AllocatorWrapper;

  // The frame mutex protects graph lifetime against execution. Graph table operations also use this mutex.
  xiiMutex m_FrameMutex;

  GraphTable m_GraphTable;

  // Stable Entry allocations: moving these unique pointers does not invalidate graph-table pointers.
  xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;

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

  if (s_pState->m_bFrameActive || s_pState->m_pResourceCache == nullptr)
    return {};

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphManagerState::Entry);
  pEntry->m_pGraph                                       = XII_NEW(s_pState->GetAllocator(), xiiRenderGraph, sName);
  pEntry->m_BuildDelegate                                = buildDelegate;

  const xiiRenderGraphHandle hGraph(s_pState->m_GraphTable.Insert(pEntry.Borrow()));
  s_pState->m_Entries.PushBack(std::move(pEntry));

  return hGraph;
}

xiiResult xiiRenderGraphManager::DestroyGraph(xiiRenderGraphHandle hGraph)
{
  if (s_pState == nullptr || hGraph.IsInvalidated())
    return XII_FAILURE;

  XII_LOCK(s_pState->m_FrameMutex);

  if (s_pState->m_bFrameActive)
    return XII_FAILURE;

  xiiRenderGraphManagerState::Entry* pEntry = nullptr;
  if (!s_pState->m_GraphTable.TryGetValue(hGraph, pEntry))
    return XII_FAILURE;

  xiiUInt32 uiEntryIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0U; i < s_pState->m_Entries.GetCount(); ++i)
  {
    if (s_pState->m_Entries[i].Borrow() == pEntry)
    {
      uiEntryIndex = i;
      break;
    }
  }

  if (uiEntryIndex == xiiInvalidIndex)
    return XII_FAILURE;

  // A graph can own persistent pass resources in addition to pooled transient resources.
  // Conservatively wait until all previously captured submissions have completed.
  if (s_pState->m_uiFrameIndex != 0U)
  {
    s_pState->m_FrameCompletionTracker.WaitForFrame(s_pState->m_uiFrameIndex);
  }

  xiiRenderGraphManagerState::Entry* pRemovedEntry = nullptr;
  XII_VERIFY(s_pState->m_GraphTable.Remove(hGraph, &pRemovedEntry), "Failed to remove a managed render graph.");
  s_pState->m_Entries.RemoveAtAndSwap(uiEntryIndex);

  return XII_SUCCESS;
}

xiiResult xiiRenderGraphManager::ExecuteGraph(const xiiRenderGraphFrame& frame, xiiRenderGraphHandle hGraph, const xiiView* pView, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError)
{
  if (s_pState == nullptr || hGraph.IsInvalidated() || !frame.IsActive())
    return XII_FAILURE;

  XII_LOCK(s_pState->m_FrameMutex);

  if (!s_pState->m_bFrameActive || frame.GetFrameIndex() != s_pState->m_uiFrameIndex)
    return XII_FAILURE;

  xiiRenderGraphManagerState::Entry* pEntry = nullptr;
  if (!s_pState->m_GraphTable.TryGetValue(hGraph, pEntry))
    return XII_FAILURE;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || s_pState->m_pResourceCache == nullptr)
    return XII_FAILURE;

  pEntry->m_Blackboard.ClearFrame();
  pEntry->m_pGraph->BeginSetup(frame.GetFrameIndex());
  pEntry->m_BuildDelegate(*pEntry->m_pGraph, pEntry->m_Blackboard, pView);
  pEntry->m_pGraph->EndSetup();

  if (pEntry->m_pGraph->Compile(settings, out_pError).Failed())
    return XII_FAILURE;

  if (pEntry->m_pGraph->Execute(pDevice.Borrow(), pView, &pEntry->m_Blackboard, s_pState->m_pResourceCache.Borrow(), s_pState->m_pProfiler.Borrow(), out_pError).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
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

    for (auto& pEntry : s_pState->m_Entries)
    {
      xiiLog::Warning("  Graph '{0}' was not destroyed.", pEntry->m_pGraph->GetName());
    }
  }

  s_pState->m_GraphTable.Clear();
  s_pState->m_Entries.Clear();
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
