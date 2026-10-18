/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>

class xiiLightingManagerState
{
public:
  xiiSharedPtr<xiiGALTexture> m_pBRDFLUT;
  xiiSharedPtr<xiiGALFence>   m_pBRDFReadyFence;
  bool                        m_bBRDFLUTGenerated = false;
  bool                        m_bEngineStarted    = false;
};

xiiUniquePtr<xiiLightingManagerState> xiiLightingManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, LightingManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiLightingManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiLightingManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiLightingManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiLightingManager::Shutdown();
  }

  // Keep this terminator on a unique source line because subsystem symbols use __LINE__ in unity builds.
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiLightingManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiLightingManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiResult xiiLightingManager::EnsureBRDFLUTResources()
{
  if (!IsInitialized())
    return XII_FAILURE;

  if (s_pState->m_pBRDFLUT != nullptr)
    return XII_SUCCESS;

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = 256U;
  description.m_Size.height = 256U;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;

  xiiSharedPtr<xiiGALTexture> pBRDFLUT = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  if (pBRDFLUT == nullptr)
    return XII_FAILURE;
  xiiGALFenceCreationDescription fenceDescription;
  fenceDescription.m_Type = xiiGALFenceType::General;
  auto ready              = xiiGALDevice::GetDefaultDevice()->CreateFence(fenceDescription);
  if (!ready)
    return XII_FAILURE;

  pBRDFLUT->SetDebugName("Split-Sum BRDF LUT");
  s_pState->m_pBRDFLUT          = std::move(pBRDFLUT);
  s_pState->m_pBRDFReadyFence   = std::move(ready);
  s_pState->m_bBRDFLUTGenerated = false;
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALTexture> xiiLightingManager::GetBRDFLUT()
{
  return s_pState != nullptr ? s_pState->m_pBRDFLUT : nullptr;
}

xiiSharedPtr<xiiGALFence> xiiLightingManager::GetBRDFLUTReadyFence()
{
  return s_pState != nullptr ? s_pState->m_pBRDFReadyFence : nullptr;
}

bool xiiLightingManager::IsBRDFLUTGenerationPending()
{
  return s_pState == nullptr || !s_pState->m_bBRDFLUTGenerated;
}

void xiiLightingManager::MarkBRDFLUTGenerated()
{
  if (s_pState != nullptr && s_pState->m_pBRDFLUT != nullptr)
    s_pState->m_bBRDFLUTGenerated = true;
}

void xiiLightingManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Lighting manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiLightingManagerState);
}

void xiiLightingManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede lighting manager engine startup.");
  if (s_pState != nullptr)
  {
    s_pState->m_bEngineStarted = true;
    // Lookup allocation is demand-driven by the reflected generation pass.
  }
}

void xiiLightingManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_pBRDFLUT.Clear();
  s_pState->m_pBRDFReadyFence.Clear();
  s_pState->m_bBRDFLUTGenerated = false;

  s_pState->m_bEngineStarted = false;
}

void xiiLightingManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LightingManager);
