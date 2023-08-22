#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/Device.h>

#include <Foundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/BottomLevelAS.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

namespace
{
  struct GALObjectType
  {
    using StorageType = xiiUInt8;

    enum Enum : xiiUInt8
    {
      SwapChain = 0U,
      BottomLevelAS,
      Buffer,
      BufferView,
      Fence,
      Framebuffer,
      Query,
      RenderPass,
      Sampler,
      Texture,
      TextureView,
      TopLevelAS,
      InputLayout,
      Shader,
      BlendState,
      DepthStencilState,
      RasterizerState
    };
  };

  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALSwapChainHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBottomLevelASHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBufferHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBufferViewHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALFenceHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALFramebufferHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALQueryHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALRenderPassHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALSamplerHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALTextureHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALTextureViewHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALTopLevelASHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALInputLayoutHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALShaderHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBlendStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALDepthStencilStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALRasterizerStateHandle) == sizeof(xiiUInt32));
} // namespace

xiiGALDevice::xiiGALDevice(const xiiGALDeviceCreationDescription& creationDescription) :
  xiiGALObject<xiiGALDeviceCreationDescription>(creationDescription), m_Allocator("GALDevice", xiiFoundation::GetDefaultAllocator()), m_AllocatorWrapper(&m_Allocator)
{
}

xiiGALDevice::~xiiGALDevice()
{
  // Check for object leaks.
  {
    XII_LOG_BLOCK("xiiGALDevice object leak report.");

    if (!m_SwapChains.IsEmpty())
      xiiLog::Warning("{0} swap chains have not been cleaned up.", m_SwapChains.GetCount());

    if (!m_BottomLevelAccelerationStructures.IsEmpty())
      xiiLog::Warning("{0} bottom-level acceleration structures have not been cleaned up.", m_BottomLevelAccelerationStructures.GetCount());

    if (!m_Buffers.IsEmpty())
      xiiLog::Warning("{0} buffers have not been cleaned up.", m_Buffers.GetCount());

    if (!m_BufferViews.IsEmpty())
      xiiLog::Warning("{0} buffer views have not been cleaned up.", m_BufferViews.GetCount());

    if (!m_Fences.IsEmpty())
      xiiLog::Warning("{0} fences have not been cleaned up.", m_Fences.GetCount());

    if (!m_Framebuffers.IsEmpty())
      xiiLog::Warning("{0} framebuffers have not been cleaned up.", m_Framebuffers.GetCount());

    if (!m_Queries.IsEmpty())
      xiiLog::Warning("{0} queries have not been cleaned up.", m_Queries.GetCount());

    if (!m_RenderPasses.IsEmpty())
      xiiLog::Warning("{0} render passes have not been cleaned up.", m_RenderPasses.GetCount());

    if (!m_Samplers.IsEmpty())
      xiiLog::Warning("{0} samplers have not been cleaned up.", m_Samplers.GetCount());

    if (!m_Textures.IsEmpty())
      xiiLog::Warning("{0} textures have not been cleaned up.", m_Textures.GetCount());

    if (!m_TopLevelAccelerationStructures.IsEmpty())
      xiiLog::Warning("{0} top-level acceleration structures have not been cleaned up.", m_TopLevelAccelerationStructures.GetCount());

    if (!m_InputLayouts.IsEmpty())
      xiiLog::Warning("{0} input layouts have not been cleaned up.", m_InputLayouts.GetCount());

    if (!m_Shaders.IsEmpty())
      xiiLog::Warning("{0} shaders have not been cleaned up.", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      xiiLog::Warning("{0} blend states have not been cleaned up.", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      xiiLog::Warning("{0} depth stencil states have not been cleaned up.", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      xiiLog::Warning("{0} rasterizer states have not been cleaned up.", m_RasterizerStates.GetCount());
  }
}

xiiResult xiiGALDevice::Initialize()
{
  XII_LOG_BLOCK("xiiGALDevice::Initialize");

  xiiResult platformInitResult = InitializePlatform();

  if (platformInitResult == XII_FAILURE)
  {
    return XII_FAILURE;
  }

  // Fill the device capabilities
  FillCapabilitiesPlatform();

  xiiLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM.", m_AdapterDescription.m_sAdapterName, xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory),
               xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory), xiiArgFileSize(m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory));

  if (m_AdapterDescription.m_Type != xiiGALDeviceAdapterType::Discrete)
  {
    xiiLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiProfilingSystem::InitializeGPUData();

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterInitialization;
    m_Events.Broadcast(e);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALDevice::Shutdown()
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_LOG_BLOCK("xiiGALDevice::Shutdown");

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::BeforeShutdown;
    m_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // Ensure we are not listed as the default device.
  if (xiiGALDevice::HasDefaultDevice() && xiiGALDevice::GetDefaultDevice() == this)
  {
    xiiGALDevice::SetDefaultDevice(nullptr);
  }

  return ShutdownPlatform();
}

void xiiGALDevice::BeginPipeline(xiiStringView sName, xiiGALSwapChainHandle hSwapChain)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(!m_bBeginPipelineCalled, "Nested Pipelines are not allowed: You must call xiiGALDevice::EndPipeline before you can call xiiGALDevice::BeginPipeline again.");
  m_bBeginPipelineCalled = true;

  xiiGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  BeginPipelinePlatform(sName, pSwapChain);
}

void xiiGALDevice::EndPipeline(xiiGALSwapChainHandle hSwapChain)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(m_bBeginPipelineCalled, "You must have called xiiGALDevice::BeginPipeline before you can call xiiGALDevice::EndPipeline.");
  m_bBeginPipelineCalled = false;

  xiiGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  EndPipelinePlatform(pSwapChain);
}

xiiGALPass* xiiGALDevice::BeginPass(xiiStringView sName)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(!m_bBeginPassCalled, "Nested Passes are not allowed: You must call xiiGALDevice::EndPass before you can call xiiGALDevice::BeginPass again.");
  m_bBeginPassCalled = true;

  return BeginPassPlatform(sName);
}

void xiiGALDevice::EndPass(xiiGALPass* pPass)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(m_bBeginPassCalled, "You must have called xiiGALDevice::BeginPass before you can call xiiGALDevice::EndPass.");
  m_bBeginPassCalled = false;

  EndPassPlatform(pPass);
}

void xiiGALDevice::BeginFrame(const xiiUInt64 uiRenderFrame)
{
  {
    XII_PROFILE_SCOPE("BeforeBeginFrame");

    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::BeforeBeginFrame;
    m_Events.Broadcast(e);
  }

  {
    XII_GAL_DEVICE_LOCK_AND_CHECK();

    XII_ASSERT_DEV(!m_bBeginFrameCalled, "You must call xiiGALDevice::EndFrame before you can call xiiGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;

    BeginFramePlatform(uiRenderFrame);
  }

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterBeginFrame;
    m_Events.Broadcast(e);
  }
}

void xiiGALDevice::EndFrame()
{
  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::BeforeEndFrame;
    m_Events.Broadcast(e);
  }

  {
    XII_GAL_DEVICE_LOCK_AND_CHECK();

    XII_ASSERT_DEV(m_bBeginFrameCalled, "You must have called xiiGALDevice::Begin before you can call xiiGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform();

    m_bBeginFrameCalled = false;
  }

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEventType::AfterEndFrame;
    m_Events.Broadcast(e);
  }
}

#define XII_VERIFY_BLEND_STATE(expression, ...) \
  do                                            \
  {                                             \
    if (!(expression))                          \
    {                                           \
      xiiLog::Error(__VA_ARGS__);               \
      return xiiGALBlendStateHandle();          \
    }                                           \
  } while (false);

xiiGALBlendStateHandle xiiGALDevice::CreateBlendState(const xiiGALBlendStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  for (xiiUInt32 i = 0U; i < description.m_RenderTargets.GetCount(); ++i)
  {
    const auto& rtDescription = description.m_RenderTargets[i];

    const bool bBlendEnable = rtDescription.m_bBlendEnable && (i == 0U || (description.m_bIndependentBlend && i > 0U));

    if (bBlendEnable)
    {
      XII_VERIFY_BLEND_STATE(rtDescription.m_SourceBlend != xiiGALBlendFactor::Undefined, "The source blend must not be xiiGALBlendFactor::Undefined.");
      XII_VERIFY_BLEND_STATE(rtDescription.m_DestinationBlend != xiiGALBlendFactor::Undefined, "The destination blend must not be xiiGALBlendFactor::Undefined.");
      XII_VERIFY_BLEND_STATE(rtDescription.m_BlendOperation != xiiGALBlendOperation::Undefined, "The blend operation must not be xiiGALBlendOperation::Undefined.");

      XII_VERIFY_BLEND_STATE(rtDescription.m_SourceBlendAlpha != xiiGALBlendFactor::Undefined, "The alpha source blend must not be xiiGALBlendFactor::Undefined.");
      XII_VERIFY_BLEND_STATE(rtDescription.m_DestinationBlendAlpha != xiiGALBlendFactor::Undefined, "The alpha destination blend must not be xiiGALBlendFactor::Undefined.");
      XII_VERIFY_BLEND_STATE(rtDescription.m_BlendOperationAlpha != xiiGALBlendOperation::Undefined, "The alpha blend operation must not be xiiGALBlendOperation::Undefined.");
    }
  }

  // Hash description and return any existing one (including increasing the refcount).
  xiiUInt32 uiHash = description.CalculateHash();

  {
    xiiGALBlendStateHandle hBlendState;
    if (m_BlendStateTable.TryGetValue(uiHash, hBlendState))
    {
      xiiGALBlendState* pBlendState = m_BlendStates[hBlendState];
      if (pBlendState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::BlendState, hBlendState);
      }

      pBlendState->AddRef();
      return hBlendState;
    }
  }

  xiiGALBlendState* pBlendState = CreateBlendStatePlatform(description);

  if (pBlendState != nullptr)
  {
    XII_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash does not match.");

    pBlendState->AddRef();

    xiiGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
  }

  return xiiGALBlendStateHandle();
}

void xiiGALDevice::DestroyBlendState(xiiGALBlendStateHandle hBlendState)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALBlendState* pBlendState = nullptr;

  if (m_BlendStates.TryGetValue(hBlendState, pBlendState))
  {
    pBlendState->ReleaseRef();

    if (pBlendState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::BlendState, hBlendState);
    }
  }
  else
  {
    xiiLog::Warning("DestroyBlendState called on an invalid handle (double free?).");
  }
}

#undef XII_VERIFY_BLEND_STATE

#define XII_VERIFY_DEPTH_STENCIL_STATE(expression, ...) \
  do                                                    \
  {                                                     \
    if (!(expression))                                  \
    {                                                   \
      xiiLog::Error(__VA_ARGS__);                       \
      return xiiGALDepthStencilStateHandle();           \
    }                                                   \
  } while (false);

xiiGALDepthStencilStateHandle xiiGALDevice::CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_VERIFY_DEPTH_STENCIL_STATE(!description.m_bDepthEnable && description.m_ComparisonDepthFunction == xiiGALComparisonFunction::Unknown, "The depth comparison function must not be xiiGALComparisonFunction::Unknown when depth is enabled.");

  if (description.m_bStencilEnable)
  {
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_FrontFace.m_StencilFailOperation != xiiGALStencilOperation::Undefined, "The front face stencil fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_FrontFace.m_StencilDepthFailOperation != xiiGALStencilOperation::Undefined, "The front face stencil depth fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_FrontFace.m_StencilPassOperation != xiiGALStencilOperation::Undefined, "The front face stencil pass operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_FrontFace.m_ComparisonFunction != xiiGALComparisonFunction::Unknown, "The front face stencil comparison function must not be xiiGALComparisonFunction::Unknown when stencil is enabled.");

    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_BackFace.m_StencilFailOperation != xiiGALStencilOperation::Undefined, "The back face stencil fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_BackFace.m_StencilDepthFailOperation != xiiGALStencilOperation::Undefined, "The back face stencil depth fail operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_BackFace.m_StencilPassOperation != xiiGALStencilOperation::Undefined, "The back face stencil pass operation must not be xiiGALStencilOperation::Undefined when stencil is enabled.");
    XII_VERIFY_DEPTH_STENCIL_STATE(description.m_BackFace.m_ComparisonFunction != xiiGALComparisonFunction::Unknown, "The back face stencil comparison function must not be xiiGALComparisonFunction::Unknown when stencil is enabled.");
  }

  // Hash description and return any existing one (including increasing the refcount).
  xiiUInt32 uiHash = description.CalculateHash();

  {
    xiiGALDepthStencilStateHandle hDepthStencilState;
    if (m_DepthStencilStateTable.TryGetValue(uiHash, hDepthStencilState))
    {
      xiiGALDepthStencilState* pDepthStencilState = m_DepthStencilStates[hDepthStencilState];
      if (pDepthStencilState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
      }

      pDepthStencilState->AddRef();
      return hDepthStencilState;
    }
  }

  xiiGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(description);

  if (pDepthStencilState != nullptr)
  {
    XII_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash does not match.");

    pDepthStencilState->AddRef();

    xiiGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return xiiGALDepthStencilStateHandle();
}

void xiiGALDevice::DestroyDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALDepthStencilState* pDepthStencilState = nullptr;

  if (m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState))
  {
    pDepthStencilState->ReleaseRef();

    if (pDepthStencilState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
    }
  }
  else
  {
    xiiLog::Warning("DestroyDepthStencilState called on an invalid handle (double free?).");
  }
}

#undef XII_VERIFY_DEPTH_STENCIL_STATE

#define XII_VERIFY_RASTERIZER_STATE(expression, ...) \
  do                                                 \
  {                                                  \
    if (!(expression))                               \
    {                                                \
      xiiLog::Error(__VA_ARGS__);                    \
      return xiiGALRasterizerStateHandle();          \
    }                                                \
  } while (false);

xiiGALRasterizerStateHandle xiiGALDevice::CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  XII_VERIFY_RASTERIZER_STATE(description.m_FillMode != xiiGALFillMode::Undefined, "The fill mode cannot be xiiGALFillMode::Undefined.");
  XII_VERIFY_RASTERIZER_STATE(description.m_CullMode != xiiGALCullMode::Undefined, "The cull mode cannot be xiiGALCullMode::Undefined.");

  // Hash description and return any existing one (including increasing the refcount).
  xiiUInt32 uiHash = description.CalculateHash();

  {
    xiiGALRasterizerStateHandle hRasterizerState;
    if (m_RasterizerStateTable.TryGetValue(uiHash, hRasterizerState))
    {
      xiiGALRasterizerState* pRasterizerState = m_RasterizerStates[hRasterizerState];
      if (pRasterizerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::RasterizerState, hRasterizerState);
      }

      pRasterizerState->AddRef();
      return hRasterizerState;
    }
  }

  xiiGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(description);

  if (pRasterizerState != nullptr)
  {
    XII_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash does not match.");

    pRasterizerState->AddRef();

    xiiGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return xiiGALRasterizerStateHandle();
}

void xiiGALDevice::DestroyRasterizerState(xiiGALRasterizerStateHandle hRasterizerState)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALRasterizerState* pRasterizerState = nullptr;

  if (m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState))
  {
    pRasterizerState->ReleaseRef();

    if (pRasterizerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::RasterizerState, hRasterizerState);
    }
  }
  else
  {
    xiiLog::Warning("DestroyRasterizerState called on an invalid handle (double free?).");
  }
}

#undef XII_VERIFY_RASTERIZER_STATE

#define XII_VERIFY_SHADER(expression, ...) \
  do                                       \
  {                                        \
    if (!(expression))                     \
    {                                      \
      xiiLog::Error(__VA_ARGS__);          \
      return xiiGALShaderHandle();         \
    }                                      \
  } while (false);

xiiGALShaderHandle xiiGALDevice::CreateShader(const xiiGALShaderCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  bool bHasByteCodes = false;

  for (xiiUInt32 uiStage = 0; uiStage < xiiGALShaderStage::ENUM_COUNT; ++uiStage)
  {
    if (description.HasByteCodeForStage((xiiGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    xiiLog::Error("A shader cannot be created with no shader bytecode.");

    return xiiGALShaderHandle();
  }

  if (description.m_ShaderStage.IsSet(xiiGALShaderStage::Geometry) && m_AdapterDescription.m_Features.m_GeometryShaders != xiiGALDeviceFeatureState::Enabled)
  {
    XII_VERIFY_SHADER(false, "Geometry shaders are not supported by this device.");
  }
  if (description.m_ShaderStage.IsAnySet(xiiGALShaderStage::Domain | xiiGALShaderStage::Hull) && m_AdapterDescription.m_Features.m_Tessellation != xiiGALDeviceFeatureState::Enabled)
  {
    XII_VERIFY_SHADER(false, "Tessellation shaders are not supported by this device.");
  }
  if (description.m_ShaderStage.IsSet(xiiGALShaderStage::Compute) && m_AdapterDescription.m_Features.m_ComputeShaders != xiiGALDeviceFeatureState::Enabled)
  {
    XII_VERIFY_SHADER(false, "Compute shaders are not supported by this device.");
  }
  if (description.m_ShaderStage.IsAnySet(xiiGALShaderStage::Amplification | xiiGALShaderStage::Mesh) && m_AdapterDescription.m_Features.m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
  {
    XII_VERIFY_SHADER(false, "Mesh shaders are not supported by this device.");
  }
  if (description.m_ShaderStage.IsAnySet(xiiGALShaderStage::AllRayTracing))
  {
    if (m_AdapterDescription.m_Features.m_RayTracing != xiiGALDeviceFeatureState::Enabled || m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags.AreNoneSet(xiiGALRayTracingCapabilityFlags::StandaloneShaders))
    {
      XII_VERIFY_SHADER(false, "Standalone ray tracing shaders are not supported by this device.");
    }
  }
  if (description.m_ShaderStage.IsSet(xiiGALShaderStage::Tile) && m_AdapterDescription.m_Features.m_TileShaders != xiiGALDeviceFeatureState::Enabled)
  {
    XII_VERIFY_SHADER(false, "Tile shaders are not supported by this device.");
  }

  xiiGALShader* pShader = CreateShaderPlatform(description);

  if (pShader == nullptr)
  {
    return xiiGALShaderHandle();
  }
  else
  {
    return xiiGALShaderHandle(m_Shaders.Insert(pShader));
  }
}

void xiiGALDevice::DestroyShader(xiiGALShaderHandle hShader)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    AddDeadObject(GALObjectType::Shader, hShader);
  }
  else
  {
    xiiLog::Warning("DestroyShader called on an invalid handle (double free?).");
  }
}

#undef XII_VERIFY_SHADER

#define XII_VERIFY_BUFFER(expression, ...) \
  do                                       \
  {                                        \
    if (!(expression))                     \
    {                                      \
      xiiLog::Error(__VA_ARGS__);          \
      return xiiGALBufferHandle();         \
    }                                      \
  } while (false);

xiiGALBufferHandle xiiGALDevice::CreateBuffer(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData /* = nullptr*/)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  // Validate buffer description.

  auto allowedBindFlags = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::StreamOutput | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;

  XII_VERIFY_BUFFER(description.m_BindFlags.IsStrictlyAnySet(allowedBindFlags), "The buffer description bind flags contain unsupported bind flags.");

  if (description.m_BindFlags.IsAnySet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess))
  {
    XII_VERIFY_BUFFER(description.m_Mode.GetValue() > xiiGALBufferMode::Undefined && description.m_Mode.GetValue() < xiiGALBufferMode::ENUM_COUNT, "The given buffer mode is not a valid buffer mode for a buffer created with xiiGALBindFlags::ShaderResource or xiiGALBindFlags::UnorderedAccess.");

    if (description.m_Mode == xiiGALBufferMode::Structured || description.m_Mode == xiiGALBufferMode::Formatted)
    {
      XII_VERIFY_BUFFER(description.m_uiElementByteStride != 0U, "The element stride must not be zero for structured and formatted buffers.");
    }
    else if (description.m_Mode == xiiGALBufferMode::Raw)
    {
      // Nothing to do.
    }
  }

  if (description.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing))
  {
    XII_VERIFY_BUFFER(m_AdapterDescription.m_Features.m_RayTracing == xiiGALDeviceFeatureState::Enabled, "xiiGALBindFlags::RayTracing flag cannot be used when the Ray Tracing feature is disabled.");
  }
  if (description.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments))
  {
    XII_VERIFY_BUFFER(m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirect), "xiiGALBindFlags::IndirectDrawArguments flag cannot be used when the xiiGALDrawCommandCapabilityFlags::DrawIndirect capability is not supported.");
  }

  switch (description.m_ResourceUsage)
  {
    case xiiGALResourceUsage::Immutable:
    case xiiGALResourceUsage::Default:
    {
      XII_VERIFY_BUFFER(description.m_CPUAccessFlags.IsNoFlagSet(), "Static and default buffers cannot have any CPU flags set.");
    }
    break;
    case xiiGALResourceUsage::Dynamic:
    {
      XII_VERIFY_BUFFER(description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Write, "Dynamic buffers require the xiiGALCPUAccessFlag::Write flag.");
    }
    break;
    case xiiGALResourceUsage::Staging:
    {
      XII_VERIFY_BUFFER(description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Write || description.m_CPUAccessFlags == xiiGALCPUAccessFlag::Read, "Exactly one of xiiGALCPUAccessFlag::Write or xiiGALCPUAccessFlag::Read must be specified for a staging buffer.");
      XII_VERIFY_BUFFER(description.m_BindFlags.IsNoFlagSet(), "Staging buffers cannot be bound to any part of the graphics pipeline and cannot have any bind flags set.");
    }
    break;
    case xiiGALResourceUsage::Unified:
    {
      XII_VERIFY_BUFFER(m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory != 0U, "Unified memory is not present in this device. Check the amount of unified memory in the device adapter information before creating unified buffers.");
      XII_VERIFY_BUFFER(description.m_CPUAccessFlags.IsAnyFlagSet(), "At least one of xiiGALCPUAccessFlag::Write or xiiGALCPUAccessFlag::Read must be specified for a unified buffer.");

      if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
      {
        XII_VERIFY_BUFFER(m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags.IsAnySet(xiiGALCPUAccessFlag::Write), "Unified memory on this device does not support write access. Check for available access flags in the device properties before creating unified buffers.");
      }
      if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
      {
        XII_VERIFY_BUFFER(m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags.IsAnySet(xiiGALCPUAccessFlag::Read), "Unified memory on this device does not support read access. Check for available access flags in the device properties before creating unified buffers.");
      }
    }
    break;
    case xiiGALResourceUsage::Sparse:
    {
      XII_VERIFY_BUFFER(m_AdapterDescription.m_Features.m_SparseResources == xiiGALDeviceFeatureState::Enabled, "Sparse buffer requires the Sparse Resources device feature to be enabled.");
      XII_VERIFY_BUFFER(description.m_CPUAccessFlags.IsNoFlagSet(), "Sparse buffers cannot have any CPU access flags set.");
      XII_VERIFY_BUFFER(description.m_uiSize <= m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize, "Sparse buffer size ({0}), must not exceed the Resource Space Size ({1}).", description.m_uiSize, m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize);
      XII_VERIFY_BUFFER(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Buffer), "Sparse buffer requires the xiiGALSparseResourceCapabilityFlags::Buffer capability.");

      if (description.m_MiscFlags.IsSet(xiiGALMiscBufferFlags::SparseAlias))
      {
        XII_VERIFY_BUFFER(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Aliased), "xiiGALMiscBufferFlags::SparseAlias flag requires the xiiGALSparseResourceCapabilityFlags::Aliased capability.");
      }
      XII_VERIFY_BUFFER(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_SparseResourceProperties.m_BindFlags), "The buffer description bind flags contain unsupported bind flags.");
    }
    break;

    default:
      xiiLog::Error("Unknown resource usage given.");
      return xiiGALBufferHandle();
  }

  if (description.m_ResourceUsage == xiiGALResourceUsage::Dynamic && xiiMath::CountBits(description.m_uiImmediateContextMask) > 1U)
  {
    const bool bNeedsBackingResource = (description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess) || description.m_Mode == xiiGALBufferMode::Formatted);
    XII_VERIFY_BUFFER(!bNeedsBackingResource, "xiiGALResourceUsage::Dynamic buffers that use the Unordered Access flag or Formatted mode requires an internal backing resource. "
                                              "This resource is implicitly transitioned by the device context and thus cannot be relied upon to be safely used in multiple contexts. Create a xiiGALResourceUsage::Dynamic buffer "
                                              "without the xiiGALResourceUsage::UnorderedAccess flag and use xiiGALResourceUsage::Undefined mode and copy the contents to a xiiGALResourceUsage::Default buffer with required flags, "
                                              "which can be shared between device contexts.");
  }

  if (description.m_ResourceUsage != xiiGALResourceUsage::Sparse)
  {
    XII_VERIFY_BUFFER(m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation == 0U || description.m_uiSize <= m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation, "Non-sparse buffer size ({0}) must not exceed the maximum allocation size ({1}).", description.m_uiSize, m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation);
    XII_VERIFY_BUFFER(description.m_MiscFlags.AreNoneSet(xiiGALMiscBufferFlags::SparseAlias), "Miscellaneous flags must not have xiiGALMiscBufferFlags::SparseAlias if the buffer usage is not xiiGALResourceUsage::Sparse.");
  }

  // Validate buffer initial data.

  const bool bHasInitialData = (pInitialData != nullptr && pInitialData->m_pData != nullptr);

  if (description.m_ResourceUsage == xiiGALResourceUsage::Immutable && !bHasInitialData)
  {
    XII_VERIFY_BUFFER(false, "The initial data must not be nullptr, as immutable buffers must be initialized at creation.");
  }
  if (description.m_ResourceUsage == xiiGALResourceUsage::Dynamic && bHasInitialData)
  {
    XII_VERIFY_BUFFER(false, "The initial data must be nullptr for dynamic buffers.");
  }
  if (description.m_ResourceUsage == xiiGALResourceUsage::Sparse && bHasInitialData)
  {
    XII_VERIFY_BUFFER(false, "The initial data must be nullptr for sparse buffers.");
  }

  if (description.m_ResourceUsage == xiiGALResourceUsage::Staging)
  {
    if (description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
    {
      XII_VERIFY_BUFFER(bHasInitialData, "Staging buffers with CPU write access must be updated via map.");
    }
  }
  else if (description.m_ResourceUsage == xiiGALResourceUsage::Unified)
  {
    XII_VERIFY_BUFFER(description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write) && bHasInitialData, "xiiGALCPUAccessFlag::Write is required to initialize a unified buffer.");
  }

  if (pInitialData != nullptr /* && pInitialData->m_pCommandEncoder != nullptr */)
  {
    /// \todo GraphicsFoundation: Assert that the command encoder used to initialize the resource is not a deferred context, as those cannot be used to initialize resources.
  }

  if (bHasInitialData)
  {
    XII_VERIFY_BUFFER(pInitialData->m_uiDataSize >= description.m_uiSize, "The buffer initial data size ({0}) must be larger than the buffer size ({1]).", pInitialData->m_uiDataSize, description.m_uiSize);
  }

  xiiGALBuffer* pBuffer = CreateBufferPlatform(description, pInitialData);

  return FinalizeBufferInternal(description, pBuffer);
}

xiiGALBufferHandle xiiGALDevice::FinalizeBufferInternal(const xiiGALBufferCreationDescription& description, xiiGALBuffer* pBuffer)
{
  if (pBuffer != nullptr)
  {
    xiiGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view.

    if (!description.m_BindFlags.IsAnySet(xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::ShaderResource))
    {
      xiiGALBufferViewCreationDescription viewDescription;
      viewDescription.m_hBuffer      = hBuffer;
      viewDescription.m_uiByteOffset = 0U;
      viewDescription.m_uiByteWidth  = (description.m_uiElementByteStride != 0U) ? (description.m_uiSize / description.m_uiElementByteStride) : description.m_uiSize;

      pBuffer->m_hDefaultBufferView = pBuffer->CreateView(viewDescription);
    }

    return hBuffer;
  }

  return xiiGALBufferHandle();
}

void xiiGALDevice::DestroyBuffer(xiiGALBufferHandle hBuffer)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALBuffer* pBuffer = nullptr;

  if (m_Buffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, hBuffer);
  }
  else
  {
    xiiLog::Warning("DestroyBuffer called on an invalid handle (double free?).");
  }
}

#undef XII_VERIFY_BUFFER

#define XII_VERIFY_TEXTURE(expression, ...) \
  do                                        \
  {                                         \
    if (!(expression))                      \
    {                                       \
      xiiLog::Error(__VA_ARGS__);           \
      return xiiGALTextureHandle();         \
    }                                       \
  } while (false);

xiiGALTextureHandle xiiGALDevice::CreateTexture(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData /* = nullptr*/)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  const auto& formatProperties = GetTextureFormatProperties(description.m_Format);

  // Validate texture description.

  XII_VERIFY_TEXTURE(description.m_Type != xiiGALResourceDimension::Undefined, "The texture resource dimension is undefined.");
  XII_VERIFY_TEXTURE(description.m_Type > xiiGALResourceDimension::Undefined && description.m_Type < xiiGALResourceDimension::ENUM_COUNT, "The texture resource dimension is invalid.");
  XII_VERIFY_TEXTURE(description.m_Size.width != 0U, "The texture width cannot be zero.");

  if (description.m_Type == xiiGALResourceDimension::Texture1D || description.m_Type == xiiGALResourceDimension::Texture1DArray)
  {
    if (description.m_Size.height != formatProperties.m_uiBlockHeight)
    {
      if (formatProperties.m_uiBlockHeight == 1U)
      {
        XII_VERIFY_TEXTURE(false, "The texture height ({0}) of a Texture1D or Texture1DArray must be equal to 1.");
      }
      else
      {
        XII_VERIFY_TEXTURE(false, "For block-compressed formats, the height ({0}) of a Texture1D or Texture1DArray must be equal to the compressed block height ({1}).", description.m_Size.height, formatProperties.m_uiBlockHeight);
      }
    }
  }
  else
  {
    XII_VERIFY_TEXTURE(description.m_Size.height != 0U, "The texture height cannot be zero.");
  }

  XII_VERIFY_TEXTURE(description.m_Type != xiiGALResourceDimension::Texture3D && description.m_uiArraySizeOrDepth != 0U, "A 3D texture depth cannot be zero.");

  if (description.m_Type == xiiGALResourceDimension::Texture1D || description.m_Type == xiiGALResourceDimension::Texture2D)
  {
    XII_VERIFY_TEXTURE(description.m_uiArraySizeOrDepth == 1U, "A Texture1D or Texture2D must have 1 array slice, ({0}) provided. Use Texture1DArray or Texture2DArray if more than one slice is needed.", description.m_uiArraySizeOrDepth);
  }

  if (description.m_Type == xiiGALResourceDimension::TextureCube || description.m_Type == xiiGALResourceDimension::TextureCubeArray)
  {
    XII_VERIFY_TEXTURE(description.m_Size.width == description.m_Size.height, "For TextureCube or TextureCubeArray textures, the width ({0} provided) must match the height ({1} provided).", description.m_Size.width, description.m_Size.height);
    XII_VERIFY_TEXTURE(description.m_uiArraySizeOrDepth >= 6U, "For TextureCube or TextureCubeArray textures, a minimum of 6 slices must be given ({0} provided).", description.m_uiArraySizeOrDepth);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    xiiUInt32 uiMaxDimension = 0;
    if (description.Is1D())
      uiMaxDimension = description.m_Size.width;
    else if (description.Is3D())
      uiMaxDimension = xiiMath::Max(description.m_Size.width, description.m_Size.height);
    else if (description.Is3D())
      uiMaxDimension = xiiMath::Max(xiiMath::Max(description.m_Size.width, description.m_Size.height), description.m_uiArraySizeOrDepth);

    XII_VERIFY_TEXTURE((uiMaxDimension >= XII_BIT(description.m_uiMipLevels - 1)), "Texture '{0}' has an incorrect number of Mip levels ({1}).", description.m_sName, description.m_uiMipLevels);
  }
#endif

  if (description.m_uiSampleCount > 1U)
  {
    XII_VERIFY_TEXTURE(xiiMath::IsPowerOf2(description.m_uiSampleCount), "The texture sample count must be a power of two.");
    XII_VERIFY_TEXTURE(description.m_Type == xiiGALResourceDimension::Texture2D || description.m_Type == xiiGALResourceDimension::Texture2DArray, "Only Texture2D and Texture2DArray can be multi-sampled.");
    XII_VERIFY_TEXTURE(description.m_uiMipLevels == 1U, "Multi-sampled textures must have one mip level ({0} levels specified).", description.m_uiMipLevels);
    XII_VERIFY_TEXTURE(description.m_BindFlags.AreNoneSet(xiiGALBindFlags::UnorderedAccess), "xiiGALBindFlags::UnorderedAccess is not allowed for multi-sampled resources.");
  }

  if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless))
  {
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags.IsAnyFlagSet(), "Memoryless textures are not supported by this device.");
    XII_VERIFY_TEXTURE(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags), "Unsupported bind flags given for memoryless textures.");
    XII_VERIFY_TEXTURE(description.m_Usage == xiiGALResourceUsage::Default, "Memoryless attachment requires xiiGALResourceUsage::Default.");
    XII_VERIFY_TEXTURE(description.m_CPUAccessFlags.IsNoFlagSet(), "Memoryless attachment requires xiiGALCPUAccessFlags::None.");
    XII_VERIFY_TEXTURE(description.m_MiscFlags.AreNoneSet(xiiGALMiscTextureFlags::GenerateMips), "Memoryless attachment is not compatible with mip map generation.");
  }

  if (description.m_Usage == xiiGALResourceUsage::Staging)
  {
    XII_VERIFY_TEXTURE(description.m_BindFlags.IsNoFlagSet(), "Staging textures cannot be bound to any GPU pipeline stage.");
    XII_VERIFY_TEXTURE(description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips), "Mipmaps cannot be automatically generated for staging textures.");
    XII_VERIFY_TEXTURE(description.m_CPUAccessFlags.IsAnyFlagSet(), "Staging textures must specify the CPU access flags.");
    XII_VERIFY_TEXTURE(description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Read) || description.m_CPUAccessFlags.IsStrictlyAnySet(xiiGALCPUAccessFlag::Write), "Staging textures must use exactly one of xiiGALCPUAccessFlags::Read or xiiGALCPUAccessFlags::Write.");
  }
  else if (description.m_Usage == xiiGALResourceUsage::Unified)
  {
    XII_VERIFY_TEXTURE(false, "xiiGALResourceUsage::Unified textures are currently not supported.");
  }

  if (description.m_Usage == xiiGALResourceUsage::Dynamic && xiiMath::CountBits(description.m_uiImmediateContextMask) > 1U)
  {
    // Dynamic textures always use a backing resource that requires implicit state transitions in map/unmap operations, which is not safe in multiple device contexts.
    XII_VERIFY_TEXTURE(false, "xiiGALResourceUsage::Dynamic textures may only be used in one immediate device context.");
  }

  if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Subsampled))
  {
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "xiiGALMiscTextureFlags::Subsampled requires the Variable Shading Rate device feature.");
    XII_VERIFY_TEXTURE(m_Description.m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Metal, "xiiGALMiscTextureFlags::Subsampled is unsupported in Metal. Use IRasterizationRateMapMtl to implement Variable Rate Shading in Metal.");
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget), "xiiGALMiscTextureFlags::Subsampled requires the xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capability.");
    XII_VERIFY_TEXTURE(!description.m_BindFlags.AreAllSet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil), "Subsampled textures must use one of xiiGALBindFlags::RenderTarget or xiiGALBindFlags::DepthStencil bind flags.");
    XII_VERIFY_TEXTURE(!description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate), "xiiGALMiscTextureFlags::Subsampled is not compatible with xiiGALBindFlags::ShadingRate.");
  }

  if (description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate))
  {
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "xiiGALBindFlags::ShadingRate requires the Variable Shading Rate device feature.");
    XII_VERIFY_TEXTURE(m_Description.m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Metal, "xiiGALBindFlags::ShadingRate is unsupported in Metal. Use IRasterizationRateMapMtl to implement Variable Rate Shading in Metal.");
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased), "xiiGALBindFlags::ShadingRate requires the xiiGALShadingRateCapabilityFlags::TextureBased capability.");
    XII_VERIFY_TEXTURE(description.m_uiSampleCount == 1U, "xiiGALBindFlags::ShadingRate is not allowed for multi-sampled textures.");

    if (description.m_Type == xiiGALResourceDimension::Texture2DArray && description.m_uiArraySizeOrDepth > 1U)
    {
      XII_VERIFY_TEXTURE(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureArray), "Shading rate texture arrays require the xiiGALShadingRateCapabilityFlags::TextureArray capability.");
    }

    XII_VERIFY_TEXTURE(description.m_Usage == xiiGALResourceUsage::Default || description.m_Usage == xiiGALResourceUsage::Immutable, "Shading rate textures only allow xiiGALResourceUsage::Default or xiiGALResourceUsage::Immutable.");

    XII_VERIFY_TEXTURE(description.m_uiMipLevels == 1U, "Shading rate textures must have a single mip level.");
    XII_VERIFY_TEXTURE(description.m_uiMipLevels != 1U, "Shading rate textures must have a single mip level."); // For Direct3D12 and Vulkan with VK_EXT_fragment_density_map
    XII_VERIFY_TEXTURE(description.m_BindFlags.IsStrictlyAnySet(m_AdapterDescription.m_ShadingRateProperties.m_BindFlags), "Unsupported bind flags are specified for the shading rate texture.");

    /// \todo GraphicsFoundation: Vulkan allows the creation of 2D texture arrays, and using a single slice for view even if xiiGALShadingRateCapabilityFlags::TextureArray is not supported by the device.
    if (description.m_Type != xiiGALResourceDimension::Texture2D && !(description.m_Type == xiiGALResourceDimension::Texture2DArray && m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureArray)))
    {
      XII_VERIFY_TEXTURE(false, "Shading rate texture must be Texture2D or Texture2DArray with the xiiGALShadingRateCapabilityFlags::TextureArray capability.");
    }

    switch (m_AdapterDescription.m_ShadingRateProperties.m_Format)
    {
      case xiiGALShadingRateFormat::Palette:
      {
        XII_VERIFY_TEXTURE(description.m_Format == xiiGALTextureFormat::R8UInt, "The shading rate texture format must be xiiGALTextureFormat::R8UInt.");
      }
      break;
      case xiiGALShadingRateFormat::RG8UNormalized:
      {
        XII_VERIFY_TEXTURE(description.m_Format == xiiGALTextureFormat::R8UNormalized, "The shading rate texture format must be xiiGALTextureFormat::R8UNormalized.");
      }
      break;

      case xiiGALShadingRateFormat::ColumnRowFloat32:
      default:
        XII_VERIFY_TEXTURE(false, "The shading rate texture is not supported.");
    }
  }

  if (description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    XII_VERIFY_TEXTURE(m_AdapterDescription.m_Features.m_SparseResources == xiiGALDeviceFeatureState::Enabled, "Sparse texture requires the Sparse Resources device feature.");

    if (description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::SparseAlias))
    {
      XII_VERIFY_TEXTURE(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.AreNoneSet(xiiGALSparseResourceCapabilityFlags::Aliased), "xiiGALMiscTextureFlags::SparseAlias flag requires the xiiGALSparseResourceCapabilityFlags::Aliased capability.");
    }

    switch (description.m_Type)
    {
      case xiiGALResourceDimension::Texture2D:
      {
        XII_VERIFY_TEXTURE(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture2D), "Texture2D requires the xiiGALSparseResourceCapabilityFlags::Texture2D capability.");
      }
      break;
      case xiiGALResourceDimension::Texture2DArray:
      case xiiGALResourceDimension::TextureCube:
      case xiiGALResourceDimension::TextureCubeArray:
      {
        XII_VERIFY_TEXTURE(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture2D), "Texture2DArray, Texture2DCube and TextureCubeArray requires the xiiGALSparseResourceCapabilityFlags::Texture2D capability.");

        if (m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.AreNoneSet(xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail))
        {
          /// \todo GraphicsFoundation: Validate texture mip size to the tile size property.
        }
      }
      break;
      case xiiGALResourceDimension::Texture3D:
      {
        XII_VERIFY_TEXTURE(m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.IsSet(xiiGALSparseResourceCapabilityFlags::Texture3D), "Texture3D requires the xiiGALSparseResourceCapabilityFlags::Texture3D capability.");
      }
      break;
      case xiiGALResourceDimension::Texture1D:
      case xiiGALResourceDimension::Texture1DArray:
      {
        XII_VERIFY_TEXTURE(false, "Texture1D and Texture1DArray sparse textures are not supported.");
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
  else
  {
    XII_VERIFY_TEXTURE(description.m_MiscFlags.AreNoneSet(xiiGALMiscTextureFlags::SparseAlias), "The miscellaneous flags must not have xiiGALMiscTextureFlags::SparseAlias if the usage is not xiiGALResourceUsage::Sparse.");
  }

  xiiGALTexture* pTexture = CreateTexturePlatform(description, pInitialData);

  return FinalizeTextureInternal(description, pTexture);
}

xiiGALTextureHandle xiiGALDevice::FinalizeTextureInternal(const xiiGALTextureCreationDescription& description, xiiGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    xiiGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view.
    if (description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_hTexture                  = hTexture;
      viewDescription.m_uiArrayOrDepthSlicesCount = description.m_uiArraySizeOrDepth;
      pTexture->m_hDefaultTextureView             = pTexture->CreateView(viewDescription);
    }

    // Create default render target view.
    if (description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_hTexture                  = hTexture;
      viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
      viewDescription.m_uiArrayOrDepthSlicesCount = description.m_uiArraySizeOrDepth;

      pTexture->m_hDefaultRenderTargetView = pTexture->CreateView(viewDescription);
    }

    return hTexture;
  }

  return xiiGALTextureHandle();
}

void xiiGALDevice::DestroyTexture(xiiGALTextureHandle hTexture)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, hTexture);
  }
  else
  {
    xiiLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

#undef XII_VERIFY_TEXTURE

#define XII_VERIFY_SAMPLER(expression, ...) \
  do                                        \
  {                                         \
    if (!(expression))                      \
    {                                       \
      xiiLog::Error(__VA_ARGS__);           \
      return xiiGALSamplerHandle();         \
    }                                       \
  } while (false);

xiiGALSamplerHandle xiiGALDevice::CreateSampler(const xiiGALSamplerCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  if (description.m_Flags.AreAllSet(xiiGALSamplerFlags::Subsampled | xiiGALSamplerFlags::SubsampledCoarseReconstruction))
  {
    XII_VERIFY_SAMPLER(m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget), "Subsampled sampler requires the xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capability.");
  }
  if (description.m_bUnormalizedCoords)
  {
    XII_VERIFY_SAMPLER(m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Vulkan || m_Description.m_GraphicsDeviceType == xiiGALGraphicsDeviceType::Metal, "Unnormalized coordinates are only supported in Vulkan and Metal.");
    XII_VERIFY_SAMPLER(description.m_MinFilter == description.m_MagFilter, "When unnormalized coordinates are enabled, the MipFilter must equal the MagFilter.");
    XII_VERIFY_SAMPLER(description.m_MipFilter == xiiGALFilterType::Point, "When unnormalized coordinates are enabled, the MipFilter must be xiiGALFilterType::Point.");
    XII_VERIFY_SAMPLER(description.m_AddressU == xiiGALTextureAddressMode::Clamp || description.m_AddressU == xiiGALTextureAddressMode::Border, "When unnormalized coordinates are enabled, the AddressU must be xiiGALTextureAddressMode::Clamp or xiiGALTextureAddressMode::Border.");
    XII_VERIFY_SAMPLER(description.m_AddressV == xiiGALTextureAddressMode::Clamp || description.m_AddressV == xiiGALTextureAddressMode::Border, "When unnormalized coordinates are enabled, the AddressV must be xiiGALTextureAddressMode::Clamp or xiiGALTextureAddressMode::Border.");
    XII_VERIFY_SAMPLER(!xiiGALFilterType::IsComparisonFilter(description.m_MinFilter), "When unnormalized coordinates are enabled, the MinFilter and MagFilter must not be of the comparison type.");
    XII_VERIFY_SAMPLER(!xiiGALFilterType::IsAnisotropicFilter(description.m_MinFilter), "When unnormalized coordinates are enabled, the MinFilter and MagFilter must not be of the anisotropic type.");
  }

  // Hash description and return any existing one (including increasing the refcount).
  xiiUInt32 uiHash = description.CalculateHash();

  {
    xiiGALSamplerHandle hSampler;
    if (m_SamplerTable.TryGetValue(uiHash, hSampler))
    {
      xiiGALSampler* pSampler = m_Samplers[hSampler];
      if (pSampler->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::Sampler, hSampler);
      }

      pSampler->AddRef();
      return hSampler;
    }
  }

  xiiGALSampler* pSampler = CreateSamplerPlatform(description);

  if (pSampler != nullptr)
  {
    XII_ASSERT_DEBUG(pSampler->GetDescription().CalculateHash() == uiHash, "Sampler hash does not match");

    pSampler->AddRef();

    xiiGALSamplerHandle hSampler(m_Samplers.Insert(pSampler));
    m_SamplerTable.Insert(uiHash, hSampler);

    return hSampler;
  }

  return xiiGALSamplerHandle();
}

void xiiGALDevice::DestroySampler(xiiGALSamplerHandle hSampler)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALSampler* pSampler = nullptr;

  if (m_Samplers.TryGetValue(hSampler, pSampler))
  {
    pSampler->ReleaseRef();

    if (pSampler->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::Sampler, hSampler);
    }
  }
  else
  {
    xiiLog::Warning("DestroySampler called on an invalid handle (double free?). ");
  }
}

#undef XII_VERIFY_SAMPLER

xiiGALInputLayoutHandle xiiGALDevice::CreateInputLayout(const xiiGALInputLayoutCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation.

  // Hash description and return any existing one (including increasing the refcount).
  xiiUInt32 uiHash = description.CalculateHash();

  {
    xiiGALInputLayoutHandle hInputLayout;
    if (m_InputLayoutTable.TryGetValue(uiHash, hInputLayout))
    {
      xiiGALInputLayout* pInputLayout = m_InputLayouts[hInputLayout];
      if (pInputLayout->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::InputLayout, hInputLayout);
      }

      pInputLayout->AddRef();
      return hInputLayout;
    }
  }

  xiiGALInputLayout* pInputLayout = CreateInputLayoutPlatform(description);

  if (pInputLayout != nullptr)
  {
    pInputLayout->AddRef();

    xiiGALInputLayoutHandle hInputLayout(m_InputLayouts.Insert(pInputLayout));
    m_InputLayoutTable.Insert(uiHash, hInputLayout);

    return hInputLayout;
  }

  return xiiGALInputLayoutHandle();
}

void xiiGALDevice::DestroyInputLayout(xiiGALInputLayoutHandle hInputLayout)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

  xiiGALInputLayout* pInputLayout = nullptr;

  if (m_InputLayouts.TryGetValue(hInputLayout, pInputLayout))
  {
    pInputLayout->ReleaseRef();

    if (pInputLayout->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::InputLayout, hInputLayout);
    }
  }
  else
  {
    xiiLog::Warning("DestroyInputLayout called on an invalid handle (double free?).");
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Device);
