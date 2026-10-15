/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
#include <GraphicsFoundation/Device/Device.h>

class xiiGpuVisibilityManagerState
{
public:
  xiiIdTable<xiiGpuVisibilityContextId, xiiUniquePtr<xiiGpuVisibilitySystem>> m_Contexts;
  xiiUInt64                                                                   m_uiLastBeginFrame = xiiMath::MaxValue<xiiUInt64>();
  bool                                                                        m_bEngineStarted   = false;
};

xiiUniquePtr<xiiGpuVisibilityManagerState> xiiGpuVisibilityManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GpuVisibilityManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "GeometryResidencyManager",
    "PipelineCache",
    "ShaderPermutationUtilities"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGpuVisibilityManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGpuVisibilityManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGpuVisibilityManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGpuVisibilityManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityContextHandle>)
XII_END_STATIC_REFLECTED_TYPE;

bool xiiGpuVisibilityManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGpuVisibilityManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

void xiiGpuVisibilityManager::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  if (!IsInitialized() || s_pState->m_uiLastBeginFrame == uiFrameIndex)
    return;

  s_pState->m_uiLastBeginFrame = uiFrameIndex;
  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
    it.Value()->BeginFrame(uiFrameIndex, uiCompletedFrame);
}

xiiUInt32 xiiGpuVisibilityManager::GetMinimumFramesInFlight()
{
  if (!IsInitialized() || s_pState->m_Contexts.IsEmpty())
    return 0U;

  xiiUInt32 uiMinimum = xiiMath::MaxValue<xiiUInt32>();
  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
    uiMinimum = xiiMath::Min(uiMinimum, it.Value()->m_Description.m_uiFramesInFlight);
  return uiMinimum;
}

xiiGpuVisibilityContextHandle xiiGpuVisibilityManager::CreateContext(const xiiGpuVisibilityDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The GPU visibility manager is not initialized.");
  if (!IsInitialized())
    return {};

  const xiiSharedPtr<xiiGALDevice>     pDevice = xiiGALDevice::GetDefaultDevice();
  xiiUniquePtr<xiiGpuVisibilitySystem> pSystem = XII_DEFAULT_NEW(xiiGpuVisibilitySystem);
  if (pSystem->Initialize(pDevice.Borrow(), description).Failed())
    return {};

  return xiiGpuVisibilityContextHandle(s_pState->m_Contexts.Insert(std::move(pSystem)));
}

void xiiGpuVisibilityManager::DestroyContext(xiiGpuVisibilityContextHandle handle)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  if (pSystem == nullptr)
    return;

  pSystem->Shutdown();
  XII_VERIFY(s_pState->m_Contexts.Remove(handle.GetInternalID()), "The GPU visibility context was validated before removal.");
}

bool xiiGpuVisibilityManager::IsValid(xiiGpuVisibilityContextHandle handle)
{
  return GetSystem(handle) != nullptr;
}

xiiGpuVisibilityOutputs xiiGpuVisibilityManager::AddPasses(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiGpuInstanceData& instances, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  XII_ASSERT_DEV(pSystem != nullptr, "GPU visibility context is invalid or has already been destroyed.");
  return pSystem != nullptr ? pSystem->AddPasses(graph, uiFrameIndex, instances, view, geometry, description, hHiZ) : xiiGpuVisibilityOutputs{};
}

xiiResult xiiGpuVisibilityManager::PrepareHiZHistory(xiiGpuVisibilityContextHandle handle, xiiUInt32 uiWidth, xiiUInt32 uiHeight)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->m_HiZHistory.Resize(uiWidth, uiHeight) : XII_FAILURE;
}

void xiiGpuVisibilityManager::InvalidateHiZHistory(xiiGpuVisibilityContextHandle handle)
{
  if (xiiGpuVisibilitySystem* pSystem = GetSystem(handle))
    pSystem->m_HiZHistory.Invalidate();
}

xiiRenderGraphTextureHandle xiiGpuVisibilityManager::ImportPreviousHiZ(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->m_HiZHistory.ImportPrevious(graph, uiFrameIndex) : xiiRenderGraphTextureHandle{};
}

void xiiGpuVisibilityManager::AddHiZBuildPass(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRenderGraphTextureHandle hSceneDepth, bool bAsyncCompute)
{
  if (xiiGpuVisibilitySystem* pSystem = GetSystem(handle))
    pSystem->m_HiZHistory.AddBuildPass(graph, uiFrameIndex, hSceneDepth, bAsyncCompute);
}

xiiUInt32 xiiGpuVisibilityManager::GetHiZMipLevelCount(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->m_HiZHistory.GetMipLevelCount() : 0U;
}

xiiUInt32 xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->GetMeshDispatchGroupCountX() : 0U;
}

xiiUInt32 xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->GetMeshDispatchGroupCountY() : 0U;
}

xiiGpuVisibilityStreamingStats xiiGpuVisibilityManager::GetStreamingStats(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->m_StreamingStats : xiiGpuVisibilityStreamingStats{};
}

void xiiGpuVisibilityManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "GPU visibility manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiGpuVisibilityManagerState);
}

void xiiGpuVisibilityManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede GPU visibility manager engine startup.");
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiGpuVisibilityManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
    it.Value()->Shutdown();
  s_pState->m_Contexts.Clear();
  s_pState->m_uiLastBeginFrame = xiiMath::MaxValue<xiiUInt64>();
  s_pState->m_bEngineStarted   = false;
}

void xiiGpuVisibilityManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiGpuVisibilitySystem* xiiGpuVisibilityManager::GetSystem(xiiGpuVisibilityContextHandle handle)
{
  if (s_pState == nullptr)
    return nullptr;

  xiiUniquePtr<xiiGpuVisibilitySystem>* pSystem = nullptr;
  return s_pState->m_Contexts.TryGetValue(handle.GetInternalID(), pSystem) ? pSystem->Borrow() : nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuVisibilityManager);
