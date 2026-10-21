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
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

//////////////////////////////////////////////////////////////////////////

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
  };

  using ContextTable = xiiIdTable<xiiRenderGraphContextHandle::IdType, Context*, xiiLocalAllocatorWrapper>;

public:
  xiiRenderGraphManagerState() :
    m_Allocator("RenderGraphManager", xiiFoundation::GetDefaultAllocator()), m_AllocatorWrapper(&m_Allocator)
  {
  }

  ~xiiRenderGraphManagerState() = default;

  /// Returns the allocator used by the render graph manager state.
  XII_ALWAYS_INLINE xiiAllocator* GetAllocator() { return &m_Allocator; }

  /// Returns the object associated with the given handle, or nullptr if the handle is invalid.
  template <typename IdTableType, typename ReturnType>
  XII_ALWAYS_INLINE ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
  {
    XII_LOCK(m_Mutex);

    ReturnType* pObject = nullptr;
    IdTable.TryGetValue(hHandle, pObject);
    return pObject;
  }

  /// Returns the context associated with the given handle, or nullptr if the handle is invalid.
  XII_ALWAYS_INLINE Context* GetContext(xiiRenderGraphContextHandle hHandle) const { return Get<ContextTable, Context>(hHandle, m_ContextTable); }

public:
  xiiProxyAllocator        m_Allocator;
  xiiLocalAllocatorWrapper m_AllocatorWrapper;

  mutable xiiMutex m_Mutex;

  ContextTable m_ContextTable;

  xiiDynamicArray<xiiUniquePtr<Entry>>          m_Entries;
  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pProfiler;
  xiiGpuFrameCompletionTracker                  m_FrameCompletionTracker;
  xiiMutex                                      m_FrameMutex;
  xiiUInt64                                     m_uiFrameIndex            = 0U;
  bool                                          m_bFrameActive            = false;
  xiiUInt32                                     m_uiNextRegistrationOrder = 0U;
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

xiiRenderGraphContextHandle xiiRenderGraphManager::CreateContext(xiiStringView sName)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Cannot create a render graph context without a valid GAL device.");

  xiiInternal::NewInstance<xiiRenderGraphManagerState::Context> pRenderGraphContext = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphManagerState::Context);
  pRenderGraphContext->m_pGraph                                                     = XII_NEW(s_pState->GetAllocator(), xiiRenderGraph, sName);
  pRenderGraphContext->m_pResourceCache                                             = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphResourceCache, pDevice);
  pRenderGraphContext->m_pProfiler                                                  = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphTimestampProfiler, pDevice);

  return xiiRenderGraphContextHandle(s_pState->m_ContextTable.Insert(pRenderGraphContext));
}

void xiiRenderGraphManager::DestroyContext(xiiRenderGraphContextHandle hContext)
{
  xiiRenderGraphManagerState::Context* pContext = nullptr;

  XII_VERIFY(s_pState->m_ContextTable.Remove(hContext, &pContext), "Invalid render graph context.");

  XII_DELETE(s_pState->GetAllocator(), pContext);
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphContextHandle hContext)
{
  XII_ASSERT_DEV(!hContext.IsInvalidated(), "The render graph context is invalid.");

  return s_pState->GetContext(hContext)->m_pGraph.Borrow();
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphContextHandle hContext)
{
  XII_ASSERT_DEV(!hContext.IsInvalidated(), "The render graph context is invalid.");

  return &s_pState->GetContext(hContext)->m_Blackboard;
}

xiiRenderGraphResourceCache* xiiRenderGraphManager::GetResourceCache(xiiRenderGraphContextHandle hContext)
{
  XII_ASSERT_DEV(!hContext.IsInvalidated(), "The render graph context is invalid.");

  return s_pState->GetContext(hContext)->m_pResourceCache.Borrow();
}

xiiRenderGraphTimestampProfiler* xiiRenderGraphManager::GetProfiler(xiiRenderGraphContextHandle hContext)
{
  XII_ASSERT_DEV(!hContext.IsInvalidated(), "The render graph context is invalid.");

  return s_pState->GetContext(hContext)->m_pProfiler.Borrow();
}

xiiRenderGraphGraphId xiiRenderGraphManager::RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate)
{
  XII_ASSERT_DEV(!description.m_sName.IsEmpty(), "A render graph registration requires a name.");
  XII_ASSERT_DEV(buildDelegate.IsValid(), "A render graph registration requires a build delegate.");

  xiiUniquePtr<xiiRenderGraphManagerState::Entry> pEntry = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphManagerState::Entry);
  xiiRenderGraphManagerState::Entry&              entry  = *pEntry;
  entry.m_Description                                    = description;
  entry.m_pGraph                                         = XII_NEW(s_pState->GetAllocator(), xiiRenderGraph, description.m_sName);
  entry.m_BuildDelegate                                  = buildDelegate;
  entry.m_uiRegistrationOrder                            = s_pState->m_uiNextRegistrationOrder++;
  const xiiRenderGraphGraphId id                         = entry.m_pGraph->GetId();
  s_pState->m_Entries.PushBack(std::move(pEntry));
  return id;
}

void xiiRenderGraphManager::UnregisterGraph(xiiRenderGraphGraphId id)
{
  XII_ASSERT_DEV(id.IsValid(), "The render graph ID is invalid.");

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "The render graph ID is invalid.");

  s_pState->m_Entries.RemoveAtAndSwap(uiIndex);
}

xiiRenderGraph* xiiRenderGraphManager::GetGraph(xiiRenderGraphGraphId id)
{
  XII_ASSERT_DEV(id.IsValid(), "The render graph ID is invalid.");

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "The render graph ID is invalid.");

  return s_pState->m_Entries[uiIndex]->m_pGraph.Borrow();
}

xiiRenderGraphBlackboard* xiiRenderGraphManager::GetBlackboard(xiiRenderGraphGraphId id)
{
  XII_ASSERT_DEV(id.IsValid(), "The render graph ID is invalid.");

  const xiiUInt32 uiIndex = FindEntry(*s_pState, id);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "The render graph ID is invalid.");

  return &s_pState->m_Entries[uiIndex]->m_Blackboard;
}

xiiRenderGraphResourceCache* xiiRenderGraphManager::GetResourceCache()
{
  return s_pState->m_pResourceCache.Borrow();
}

xiiRenderGraphTimestampProfiler* xiiRenderGraphManager::GetProfiler()
{
  return s_pState->m_pProfiler.Borrow();
}

xiiResult xiiRenderGraphManager::GetFrameCompletion(xiiUInt64 uiFrameIndex, xiiUInt64& out_uiCompletedFrame)
{
  out_uiCompletedFrame = 0;

  XII_LOCK(s_pState->m_FrameMutex);

  if (!s_pState->m_bFrameActive || uiFrameIndex == 0 || uiFrameIndex != s_pState->m_uiFrameIndex)
    return XII_FAILURE;

  out_uiCompletedFrame = s_pState->m_FrameCompletionTracker.GetLastCompletedFrame();

  return XII_SUCCESS;
}

xiiResult xiiRenderGraphManager::BeginFrame(xiiRenderGraphFrame& frame, xiiUInt32 uiFramesInFlight)
{
  if (frame.IsActive() || uiFramesInFlight == 0U)
    return XII_FAILURE;

  s_pState->m_FrameMutex.Lock();

  // Foundation mutexes are recursive; reject nested frames while other threads wait for End.
  if (s_pState->m_bFrameActive || s_pState->m_uiFrameIndex == xiiMath::MaxValue<xiiUInt64>())
  {
    s_pState->m_FrameMutex.Unlock();
    return XII_FAILURE;
  }
  // All global and context-local slices advance on this clock.
  // Never reuse a shorter ring while its previous occupant is still in flight, even if the caller permits more.
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
  if (!frame.IsActive() || frame.GetFrameIndex() != s_pState->m_uiFrameIndex)
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
  XII_ASSERT_DEV(pResourceCache != nullptr, "The render graph resource cache is invalid.");

  XII_LOCK(s_pState->m_FrameMutex);

  s_pState->m_Entries.Sort([](xiiUInt32 lhs, xiiUInt32 rhs) -> bool {
    const xiiRenderGraphManagerState::Entry& a = *s_pState->m_Entries[lhs];
    const xiiRenderGraphManagerState::Entry& b = *s_pState->m_Entries[rhs];

    if (a.m_Description.m_iPriority != b.m_Description.m_iPriority)
    {
      return a.m_Description.m_iPriority < b.m_Description.m_iPriority;
    }
    return a.m_uiRegistrationOrder < b.m_uiRegistrationOrder;
  });

  for (auto& pEntry : s_pState->m_Entries)
  {
    pEntry->m_Blackboard.ClearFrame();
    pEntry->m_pGraph->BeginSetup(uiFrameIndex);
    pEntry->m_BuildDelegate(*pEntry->m_pGraph, pEntry->m_Blackboard);
    pEntry->m_pGraph->EndSetup();

    if (pEntry->m_pGraph->Compile(settings, out_pError).Failed())
      return XII_FAILURE;

    if (pEntry->m_pGraph->Execute(pDevice, pView, &pEntry->m_Blackboard, pResourceCache, pProfiler, out_pError).Failed())
      return XII_FAILURE;
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
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  XII_ASSERT_DEV(pDevice != nullptr, "Cannot initialize render graph manager without a valid GAL device.");

  s_pState->m_pResourceCache = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphResourceCache, pDevice);
  s_pState->m_pProfiler      = XII_NEW(s_pState->GetAllocator(), xiiRenderGraphTimestampProfiler, pDevice);

  s_pState->m_FrameCompletionTracker.Initialize(pDevice.Borrow());
}

void xiiRenderGraphManager::EngineShutdown()
{
  XII_ASSERT_DEV(!s_pState->m_bFrameActive, "End GPU frames before renderer shutdown.");

  // Check for object leaks.
  {
    if (!s_pState->m_ContextTable.IsEmpty())
    {
      xiiLog::Warning("Render graph manager context leak detected: {0} contexts were not destroyed.", s_pState->m_ContextTable.GetCount());

      for (auto it = s_pState->m_ContextTable.GetIterator(); it.IsValid(); ++it)
      {
        auto pContext = it.Value();

        xiiLog::Warning("  Context '{0}' was not destroyed.", pContext->m_pGraph->GetName());
      }
    }
  }

  s_pState->m_Entries.Clear();
  s_pState->m_ContextTable.Clear();
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
