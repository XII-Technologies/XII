/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

class xiiLightingManagerState
{
public:
  struct Slot
  {
    xiiUniquePtr<xiiLightingSystem> m_pSystem;
    xiiUInt32                       m_uiGeneration = 1U;
  };

  xiiDynamicArray<Slot>       m_Slots;
  xiiDynamicArray<xiiUInt32>  m_FreeSlots;
  xiiSharedPtr<xiiGALTexture> m_pBRDFLUT;
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLightingContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLightingContextHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

bool xiiLightingManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiLightingContextHandle xiiLightingManager::CreateContext(const xiiLightingSystemSettings& settings)
{
  XII_ASSERT_DEV(IsInitialized(), "The lighting manager is not initialized.");
  if (!IsInitialized())
    return {};

  xiiUniquePtr<xiiLightingSystem> pSystem = XII_DEFAULT_NEW(xiiLightingSystem);
  pSystem->Initialize(xiiGALDevice::GetDefaultDevice(), settings);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  if (!s_pState->m_FreeSlots.IsEmpty())
  {
    uiIndex = s_pState->m_FreeSlots.PeekBack();
    s_pState->m_FreeSlots.PopBack();
  }
  else
  {
    uiIndex = s_pState->m_Slots.GetCount();
    s_pState->m_Slots.ExpandAndGetRef();
  }

  xiiLightingManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
  slot.m_pSystem                       = std::move(pSystem);

  xiiLightingContextHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiLightingManager::DestroyContext(xiiLightingContextHandle handle)
{
  xiiLightingSystem* pSystem = GetContext(handle);
  if (pSystem == nullptr)
    return;

  xiiLightingManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  pSystem->Shutdown();
  slot.m_pSystem.Clear();
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeSlots.PushBack(handle.m_uiIndex);
}

bool xiiLightingManager::IsValid(xiiLightingContextHandle handle)
{
  return GetContext(handle) != nullptr;
}

xiiLightingSystem* xiiLightingManager::GetContext(xiiLightingContextHandle handle)
{
  if (s_pState == nullptr || !handle.IsValid() || handle.m_uiIndex >= s_pState->m_Slots.GetCount())
    return nullptr;

  xiiLightingManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  return slot.m_uiGeneration == handle.m_uiGeneration ? slot.m_pSystem.Borrow() : nullptr;
}

const xiiLightingSystem* xiiLightingManager::GetContextConst(xiiLightingContextHandle handle)
{
  return GetContext(handle);
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

  pBRDFLUT->SetDebugName("Split-Sum BRDF LUT");
  s_pState->m_pBRDFLUT          = std::move(pBRDFLUT);
  s_pState->m_bBRDFLUTGenerated = false;
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALTexture> xiiLightingManager::GetBRDFLUT()
{
  return s_pState != nullptr ? s_pState->m_pBRDFLUT : nullptr;
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
    EnsureBRDFLUTResources().IgnoreResult();
  }
}

void xiiLightingManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_pBRDFLUT.Clear();
  s_pState->m_bBRDFLUTGenerated = false;

  for (xiiUInt32 uiIndex = 0U; uiIndex < s_pState->m_Slots.GetCount(); ++uiIndex)
  {
    xiiLightingManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
    if (slot.m_pSystem == nullptr)
      continue;

    slot.m_pSystem->Shutdown();
    slot.m_pSystem.Clear();
    ++slot.m_uiGeneration;
    if (slot.m_uiGeneration == 0U)
      slot.m_uiGeneration = 1U;
    s_pState->m_FreeSlots.PushBack(uiIndex);
  }
  s_pState->m_bEngineStarted = false;
}

void xiiLightingManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiLightingContext::~xiiLightingContext()
{
  Shutdown();
}

xiiResult xiiLightingContext::Initialize(const xiiLightingSystemSettings& settings)
{
  Shutdown();
  m_Handle = xiiLightingManager::CreateContext(settings);
  return m_Handle.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiLightingContext::Shutdown()
{
  xiiLightingManager::DestroyContext(m_Handle);
  m_Handle = {};
}

bool xiiLightingContext::IsInitialized() const
{
  return xiiLightingManager::IsValid(m_Handle);
}

void xiiLightingContext::BuildFrameData(const xiiView& view, const xiiExtractedRenderData& extractedData, xiiUInt32 uiFrameIndex)
{
  GetSystem()->BuildFrameData(view, extractedData, uiFrameIndex);
}

void xiiLightingContext::UploadFrameData(xiiGALCommandList& ref_commandList)
{
  GetSystem()->UploadFrameData(ref_commandList);
}

void xiiLightingContext::BindFrameConstants(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  GetSystem()->BindFrameConstants(ref_commandList, shaderStages);
}

void xiiLightingContext::BindLightData(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  GetSystem()->BindLightData(ref_commandList, shaderStages);
}

void xiiLightingContext::BindIESProfiles(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  GetSystem()->BindIESProfiles(ref_commandList, shaderStages);
}

void xiiLightingContext::BindLightingResources(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const
{
  GetSystem()->BindLightingResources(ref_commandList, shaderStages);
}

void xiiLightingContext::WriteBlackboard(xiiRenderGraphBlackboard& ref_blackboard) const
{
  GetSystem()->WriteBlackboard(ref_blackboard);
}

const xiiLightingSystemSettings& xiiLightingContext::GetSettings() const
{
  return GetSystem()->GetSettings();
}

const xiiLightingSystem::FrameStatistics& xiiLightingContext::GetFrameStatistics() const
{
  return GetSystem()->GetFrameStatistics();
}

xiiUInt32 xiiLightingContext::GetActiveLightCount() const
{
  return GetSystem()->GetActiveLightCount();
}

xiiUInt32 xiiLightingContext::GetClusterCountX() const
{
  return GetSystem()->GetClusterCountX();
}

xiiUInt32 xiiLightingContext::GetClusterCountY() const
{
  return GetSystem()->GetClusterCountY();
}

xiiUInt32 xiiLightingContext::GetClusterCountZ() const
{
  return GetSystem()->GetClusterCountZ();
}

xiiUInt32 xiiLightingContext::GetTotalClusterCount() const
{
  return GetSystem()->GetTotalClusterCount();
}

xiiGALBuffer* xiiLightingContext::GetLightDataBuffer() const
{
  return GetSystem()->GetLightDataBuffer();
}

xiiLightingSystem* xiiLightingContext::BorrowSystem() const
{
  return GetSystem();
}

xiiLightingSystem* xiiLightingContext::GetSystem() const
{
  xiiLightingSystem* pSystem = xiiLightingManager::GetContext(m_Handle);
  XII_ASSERT_DEV(pSystem != nullptr, "The lighting context is not initialized or was invalidated by subsystem shutdown.");
  return pSystem;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LightingManager);
