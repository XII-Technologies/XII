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
    xiiLog::Warning("DestroyBlendState called on invalid handle (double free?).");
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
    xiiLog::Warning("DestroyDepthStencilState called on invalid handle (double free?).");
  }
}
XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Device);
