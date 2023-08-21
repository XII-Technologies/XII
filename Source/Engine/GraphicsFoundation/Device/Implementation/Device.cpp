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

xiiGALBlendStateHandle xiiGALDevice::CreateBlendState(const xiiGALBlendStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

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

xiiGALDepthStencilStateHandle xiiGALDevice::CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

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

xiiGALRasterizerStateHandle xiiGALDevice::CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description)
{
  XII_GAL_DEVICE_LOCK_AND_CHECK();

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

#define XII_VERIFY_SHADER(expression, ...) \
  do                                       \
  {                                        \
    if (!(expression))                     \
    {                                      \
      xiiLog::Error(__VA_ARGS__);          \
                                           \
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
                                           \
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

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Device);
