/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Material/MaterialManager.h>

class xiiMaterialManagerState
{
public:
  xiiUniquePtr<xiiMaterialSystem> m_pSystem;
  xiiMaterialGpuStorageDescription m_Description;
  bool m_bEngineStarted = false;
  bool m_bInitialized = false;
};

xiiUniquePtr<xiiMaterialManagerState> xiiMaterialManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, MaterialManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiMaterialManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiMaterialManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiMaterialManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiMaterialManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiResult xiiMaterialManager::Configure(const xiiMaterialGpuStorageDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Material manager is not started.");
  if (s_pState == nullptr || description.m_uiMaxMaterials == 0U || description.m_uiMaxParameterBytes == 0U || description.m_uiFramesInFlight == 0U)
    return XII_FAILURE;

  if (s_pState->m_bInitialized && s_pState->m_pSystem->GetGpuStorage().GetStatistics().m_uiActiveMaterials != 0U)
  {
    XII_ASSERT_DEV(false, "The material manager cannot be reconfigured while material slots are active.");
    return XII_FAILURE;
  }

  s_pState->m_Description = description;
  return s_pState->m_bEngineStarted ? ApplyConfiguration() : XII_SUCCESS;
}

bool xiiMaterialManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

void xiiMaterialManager::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before BeginFrame().");
  s_pState->m_pSystem->BeginFrame(uiFrameIndex, uiCompletedFrame);
}

xiiMaterialGpuHandle xiiMaterialManager::RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before registering materials.");
  return IsInitialized() ? s_pState->m_pSystem->RegisterMaterial(std::move(pInstance)) : xiiMaterialGpuHandle{};
}

void xiiMaterialManager::UnregisterMaterial(xiiMaterialGpuHandle handle)
{
  if (IsInitialized())
    s_pState->m_pSystem->UnregisterMaterial(handle);
}

xiiRenderGraphBufferHandle xiiMaterialManager::AddUploadPass(xiiRenderGraph& graph)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before adding upload passes.");
  return IsInitialized() ? s_pState->m_pSystem->AddUploadPass(graph) : xiiRenderGraphBufferHandle{};
}

xiiResult xiiMaterialManager::ExtractRenderData(xiiMaterialGpuHandle handle, xiiMaterialRenderData& out_renderData)
{
  return IsInitialized() ? s_pState->m_pSystem->ExtractRenderData(handle, out_renderData) : XII_FAILURE;
}

xiiMaterialGpuStorage& xiiMaterialManager::GetGpuStorage()
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager GPU storage is not initialized.");
  return s_pState->m_pSystem->GetGpuStorage();
}

xiiResult xiiMaterialManager::CreateRuntimeMaterial(const xiiMaterialSchemaDescription& description, const xiiMaterialRuntimeState& runtimeState, xiiSharedPtr<xiiMaterialSchema>& out_pSchema, xiiSharedPtr<xiiMaterialInstance>& out_pInstance, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Material manager is not started.");
  return xiiMaterialSystem::CreateRuntimeMaterial(description, runtimeState, out_pSchema, out_pInstance, out_pError);
}

void xiiMaterialManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Material manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiMaterialManagerState);
}

void xiiMaterialManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede material manager engine startup.");
  s_pState->m_bEngineStarted = true;
  ApplyConfiguration().IgnoreResult();
}

void xiiMaterialManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  if (s_pState->m_pSystem != nullptr)
    s_pState->m_pSystem->Shutdown();
  s_pState->m_pSystem.Clear();
  s_pState->m_bInitialized = false;
  s_pState->m_bEngineStarted = false;
}

void xiiMaterialManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiMaterialManager::ApplyConfiguration()
{
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return XII_FAILURE;

  if (s_pState->m_pSystem == nullptr)
    s_pState->m_pSystem = XII_DEFAULT_NEW(xiiMaterialSystem);
  else if (s_pState->m_bInitialized)
    s_pState->m_pSystem->Shutdown();

  s_pState->m_bInitialized = s_pState->m_pSystem->Initialize(pDevice.Borrow(), s_pState->m_Description).Succeeded();
  return s_pState->m_bInitialized ? XII_SUCCESS : XII_FAILURE;
}
