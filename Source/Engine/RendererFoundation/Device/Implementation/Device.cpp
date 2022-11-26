#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/State.h>

namespace
{
  struct GALObjectType
  {
    enum Enum
    {
      BlendState,
      DepthStencilState,
      RasterizerState,
      SamplerState,
      Shader,
      Buffer,
      Texture,
      ResourceView,
      RenderTargetView,
      UnorderedAccessView,
      SwapChain,
      Query,
      VertexDeclaration
    };
  };

  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBlendStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALDepthStencilStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALRasterizerStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALSamplerStateHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALShaderHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALBufferHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALTextureHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALResourceViewHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALRenderTargetViewHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALUnorderedAccessViewHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALSwapChainHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALQueryHandle) == sizeof(xiiUInt32));
  XII_CHECK_AT_COMPILETIME(sizeof(xiiGALVertexDeclarationHandle) == sizeof(xiiUInt32));
} // namespace

xiiGALDevice* xiiGALDevice::s_pDefaultDevice = nullptr;


xiiGALDevice::xiiGALDevice(const xiiGALDeviceCreationDescription& desc) :
  m_Allocator("GALDevice", xiiFoundation::GetDefaultAllocator()), m_AllocatorWrapper(&m_Allocator), m_Description(desc)
{
}

xiiGALDevice::~xiiGALDevice()
{
  // Check for object leaks
  {
    XII_LOG_BLOCK("xiiGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      xiiLog::Warning("{0} shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      xiiLog::Warning("{0} blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      xiiLog::Warning("{0} depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      xiiLog::Warning("{0} rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      xiiLog::Warning("{0} buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      xiiLog::Warning("{0} textures have not been cleaned up", m_Textures.GetCount());

    if (!m_ResourceViews.IsEmpty())
      xiiLog::Warning("{0} resource views have not been cleaned up", m_ResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      xiiLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_UnorderedAccessViews.IsEmpty())
      xiiLog::Warning("{0} unordered access views have not been cleaned up", m_UnorderedAccessViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      xiiLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_Queries.IsEmpty())
      xiiLog::Warning("{0} queries have not been cleaned up", m_Queries.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      xiiLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());
  }
}

xiiResult xiiGALDevice::Init()
{
  XII_LOG_BLOCK("xiiGALDevice::Init");

  xiiResult PlatformInitResult = InitPlatform();

  if (PlatformInitResult == XII_FAILURE)
  {
    return XII_FAILURE;
  }

  // Fill the capabilities
  FillCapabilitiesPlatform();

  xiiLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, xiiArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
               xiiArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), xiiArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    xiiLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiProfilingSystem::InitializeGPUData();

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::AfterInit;
    m_Events.Broadcast(e);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALDevice::Shutdown()
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  XII_LOG_BLOCK("xiiGALDevice::Shutdown");

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::BeforeShutdown;
    m_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (xiiGALDevice::HasDefaultDevice() && xiiGALDevice::GetDefaultDevice() == this)
  {
    xiiGALDevice::SetDefaultDevice(nullptr);
  }

  return ShutdownPlatform();
}

void xiiGALDevice::BeginPipeline(const char* szName, xiiGALSwapChainHandle hSwapChain)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(!m_bBeginPipelineCalled, "Nested Pipelines are not allowed: You must call xiiGALDevice::EndPipeline before you can call xiiGALDevice::BeginPipeline again");
  m_bBeginPipelineCalled = true;

  xiiGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  BeginPipelinePlatform(szName, pSwapChain);
}

void xiiGALDevice::EndPipeline(xiiGALSwapChainHandle hSwapChain)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(m_bBeginPipelineCalled, "You must have called xiiGALDevice::BeginPipeline before you can call xiiGALDevice::EndPipeline");
  m_bBeginPipelineCalled = false;

  xiiGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  EndPipelinePlatform(pSwapChain);
}

xiiGALPass* xiiGALDevice::BeginPass(const char* szName)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(!m_bBeginPassCalled, "Nested Passes are not allowed: You must call xiiGALDevice::EndPass before you can call xiiGALDevice::BeginPass again");
  m_bBeginPassCalled = true;

  return BeginPassPlatform(szName);
}

void xiiGALDevice::EndPass(xiiGALPass* pPass)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  XII_ASSERT_DEV(m_bBeginPassCalled, "You must have called xiiGALDevice::BeginPass before you can call xiiGALDevice::EndPass");
  m_bBeginPassCalled = false;

  EndPassPlatform(pPass);
}

xiiGALBlendStateHandle xiiGALDevice::CreateBlendState(const xiiGALBlendStateCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  xiiUInt32 uiHash = desc.CalculateHash();

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

  xiiGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    XII_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash doesn't match");

    pBlendState->AddRef();

    xiiGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
  }

  return xiiGALBlendStateHandle();
}

void xiiGALDevice::DestroyBlendState(xiiGALBlendStateHandle hBlendState)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

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
    xiiLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

xiiGALDepthStencilStateHandle xiiGALDevice::CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  xiiUInt32 uiHash = desc.CalculateHash();

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

  xiiGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    XII_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash doesn't match");

    pDepthStencilState->AddRef();

    xiiGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return xiiGALDepthStencilStateHandle();
}

void xiiGALDevice::DestroyDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

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
    xiiLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

xiiGALRasterizerStateHandle xiiGALDevice::CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  xiiUInt32 uiHash = desc.CalculateHash();

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

  xiiGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    XII_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash doesn't match");

    pRasterizerState->AddRef();

    xiiGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return xiiGALRasterizerStateHandle();
}

void xiiGALDevice::DestroyRasterizerState(xiiGALRasterizerStateHandle hRasterizerState)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

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
    xiiLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

xiiGALSamplerStateHandle xiiGALDevice::CreateSamplerState(const xiiGALSamplerStateCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  xiiUInt32 uiHash = desc.CalculateHash();

  {
    xiiGALSamplerStateHandle hSamplerState;
    if (m_SamplerStateTable.TryGetValue(uiHash, hSamplerState))
    {
      xiiGALSamplerState* pSamplerState = m_SamplerStates[hSamplerState];
      if (pSamplerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::SamplerState, hSamplerState);
      }

      pSamplerState->AddRef();
      return hSamplerState;
    }
  }

  xiiGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    XII_ASSERT_DEBUG(pSamplerState->GetDescription().CalculateHash() == uiHash, "SamplerState hash doesn't match");

    pSamplerState->AddRef();

    xiiGALSamplerStateHandle hSamplerState(m_SamplerStates.Insert(pSamplerState));
    m_SamplerStateTable.Insert(uiHash, hSamplerState);

    return hSamplerState;
  }

  return xiiGALSamplerStateHandle();
}

void xiiGALDevice::DestroySamplerState(xiiGALSamplerStateHandle hSamplerState)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALSamplerState* pSamplerState = nullptr;

  if (m_SamplerStates.TryGetValue(hSamplerState, pSamplerState))
  {
    pSamplerState->ReleaseRef();

    if (pSamplerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::SamplerState, hSamplerState);
    }
  }
  else
  {
    xiiLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



xiiGALShaderHandle xiiGALDevice::CreateShader(const xiiGALShaderCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  bool bHasByteCodes = false;

  for (xiiUInt32 uiStage = 0; uiStage < xiiGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (desc.HasByteCodeForStage((xiiGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    xiiLog::Error("Can't create a shader which supplies no bytecodes at all!");
    return xiiGALShaderHandle();
  }

  xiiGALShader* pShader = CreateShaderPlatform(desc);

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
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    AddDeadObject(GALObjectType::Shader, hShader);
  }
  else
  {
    xiiLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


xiiGALBufferHandle xiiGALDevice::CreateBuffer(const xiiGALBufferCreationDescription& desc, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    xiiLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return xiiGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (pInitialData.IsEmpty())
    {
      xiiLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
      return xiiGALBufferHandle();
    }

    xiiUInt32 uiBufferSize = desc.m_uiTotalSize;
    if (uiBufferSize != pInitialData.GetCount())
    {
      xiiLog::Error("Trying to create a buffer with invalid initial data!");
      return xiiGALBufferHandle();
    }
  }

  /// \todo Platform independent validation (buffer type supported)

  xiiGALBuffer* pBuffer = CreateBufferPlatform(desc, pInitialData);

  return FinalizeBufferInternal(desc, pBuffer);
}

xiiGALBufferHandle xiiGALDevice::FinalizeBufferInternal(const xiiGALBufferCreationDescription& desc, xiiGALBuffer* pBuffer)
{
  if (pBuffer != nullptr)
  {
    xiiGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView && desc.m_BufferType == xiiGALBufferType::Generic)
    {
      xiiGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hBuffer        = hBuffer;
      viewDesc.m_uiFirstElement = 0;
      viewDesc.m_uiNumElements  = (desc.m_uiStructSize != 0) ? (desc.m_uiTotalSize / desc.m_uiStructSize) : desc.m_uiTotalSize;

      pBuffer->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    return hBuffer;
  }

  return xiiGALBufferHandle();
}

void xiiGALDevice::DestroyBuffer(xiiGALBufferHandle hBuffer)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALBuffer* pBuffer = nullptr;

  if (m_Buffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, hBuffer);
  }
  else
  {
    xiiLog::Warning("DestroyBuffer called on invalid handle (double free?)");
  }
}

// Helper functions for buffers (for common, simple use cases)
xiiGALBufferHandle xiiGALDevice::CreateVertexBuffer(xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, xiiArrayPtr<const xiiUInt8> pInitialData, bool bDataIsMutable /*= false */)
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = uiVertexSize;
  desc.m_uiTotalSize                 = uiVertexSize * uiVertexCount;
  desc.m_BufferType                  = xiiGALBufferType::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !pInitialData.IsEmpty() && !bDataIsMutable;

  return CreateBuffer(desc, pInitialData);
}

xiiGALBufferHandle xiiGALDevice::CreateIndexBuffer(xiiGALIndexType::Enum IndexType, xiiUInt32 uiIndexCount, xiiArrayPtr<const xiiUInt8> pInitialData, bool bDataIsMutable /*= false*/)
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = xiiGALIndexType::GetSize(IndexType);
  desc.m_uiTotalSize                 = desc.m_uiStructSize * uiIndexCount;
  desc.m_BufferType                  = xiiGALBufferType::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !bDataIsMutable && !pInitialData.IsEmpty();

  return CreateBuffer(desc, pInitialData);
}

xiiGALBufferHandle xiiGALDevice::CreateConstantBuffer(xiiUInt32 uiBufferSize)
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = 0;
  desc.m_uiTotalSize                 = uiBufferSize;
  desc.m_BufferType                  = xiiGALBufferType::ConstantBuffer;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


xiiGALTextureHandle xiiGALDevice::CreateTexture(const xiiGALTextureCreationDescription& desc, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (pInitialData.IsEmpty() || pInitialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bCreateRenderTarget)
  {
    xiiLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return xiiGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    xiiLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return xiiGALTextureHandle();
  }

  xiiGALTexture* pTexture = CreateTexturePlatform(desc, pInitialData);

  return FinalizeTextureInternal(desc, pTexture);
}

xiiGALTextureHandle xiiGALDevice::FinalizeTextureInternal(const xiiGALTextureCreationDescription& desc, xiiGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    xiiGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView)
    {
      xiiGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hTexture              = hTexture;
      viewDesc.m_uiArraySize           = desc.m_uiArraySize;
      pTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    // Create default render target view
    if (desc.m_bCreateRenderTarget)
    {
      xiiGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture     = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;

      pTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return xiiGALTextureHandle();
}

void xiiGALDevice::DestroyTexture(xiiGALTextureHandle hTexture)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

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

xiiGALTextureHandle xiiGALDevice::CreateProxyTexture(xiiGALTextureHandle hParentTexture, xiiUInt32 uiSlice)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALTexture* pParentTexture = nullptr;

  if (!hParentTexture.IsInvalidated())
  {
    pParentTexture = Get<TextureTable, xiiGALTexture>(hParentTexture, m_Textures);
  }

  if (pParentTexture == nullptr)
  {
    xiiLog::Error("No valid texture handle given for proxy texture creation!");
    return xiiGALTextureHandle();
  }

  const auto& parentDesc = pParentTexture->GetDescription();
  XII_ASSERT_DEV(parentDesc.m_Type != xiiGALTextureType::Texture2DProxy, "Can't create a proxy texture of a proxy texture.");
  XII_ASSERT_DEV(parentDesc.m_Type == xiiGALTextureType::TextureCube || parentDesc.m_uiArraySize > 1,
                 "Proxy textures can only be created for cubemaps or array textures.");

  xiiGALProxyTexture* pProxyTexture = XII_NEW(&m_Allocator, xiiGALProxyTexture, *pParentTexture);
  xiiGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  const auto& desc = pProxyTexture->GetDescription();

  // Create default resource view
  if (desc.m_bAllowShaderResourceView)
  {
    xiiGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture          = hProxyTexture;
    viewDesc.m_uiFirstArraySlice = uiSlice;
    viewDesc.m_uiArraySize       = 1;

    pProxyTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
  }

  // Create default render target view
  if (desc.m_bCreateRenderTarget)
  {
    xiiGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture     = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void xiiGALDevice::DestroyProxyTexture(xiiGALTextureHandle hProxyTexture)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hProxyTexture, pTexture))
  {
    XII_ASSERT_DEV(pTexture->GetDescription().m_Type == xiiGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, hProxyTexture);
  }
  else
  {
    xiiLog::Warning("DestroyProxyTexture called on invalid handle (double free?)");
  }
}

xiiGALResourceViewHandle xiiGALDevice::GetDefaultResourceView(xiiGALTextureHandle hTexture)
{
  if (const xiiGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultResourceView;
  }

  return xiiGALResourceViewHandle();
}

xiiGALResourceViewHandle xiiGALDevice::GetDefaultResourceView(xiiGALBufferHandle hBuffer)
{
  if (const xiiGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    return pBuffer->m_hDefaultResourceView;
  }

  return xiiGALResourceViewHandle();
}

xiiGALResourceViewHandle xiiGALDevice::CreateResourceView(const xiiGALResourceViewCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALResourceBase* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, xiiGALTexture>(desc.m_hTexture, m_Textures);

  if (!desc.m_hBuffer.IsInvalidated())
    pResource = Get<BufferTable, xiiGALBuffer>(desc.m_hBuffer, m_Buffers);

  if (pResource == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return xiiGALResourceViewHandle();
  }

  // Hash desc and return potential existing one
  xiiUInt32 uiHash = desc.CalculateHash();

  {
    xiiGALResourceViewHandle hResourceView;
    if (pResource->m_ResourceViews.TryGetValue(uiHash, hResourceView))
    {
      return hResourceView;
    }
  }

  xiiGALResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);

  if (pResourceView != nullptr)
  {
    xiiGALResourceViewHandle hResourceView(m_ResourceViews.Insert(pResourceView));
    pResource->m_ResourceViews.Insert(uiHash, hResourceView);

    return hResourceView;
  }

  return xiiGALResourceViewHandle();
}

void xiiGALDevice::DestroyResourceView(xiiGALResourceViewHandle hResourceView)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALResourceView* pResourceView = nullptr;

  if (m_ResourceViews.TryGetValue(hResourceView, pResourceView))
  {
    AddDeadObject(GALObjectType::ResourceView, hResourceView);
  }
  else
  {
    xiiLog::Warning("DestroyResourceView called on invalid handle (double free?)");
  }
}

xiiGALRenderTargetViewHandle xiiGALDevice::GetDefaultRenderTargetView(xiiGALTextureHandle hTexture)
{
  if (const xiiGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultRenderTargetView;
  }

  return xiiGALRenderTargetViewHandle();
}

xiiGALRenderTargetViewHandle xiiGALDevice::CreateRenderTargetView(const xiiGALRenderTargetViewCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALTexture* pTexture = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, xiiGALTexture>(desc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    xiiLog::Error("No valid texture handle given for render target view creation!");
    return xiiGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one
  xiiUInt32 uiHash = desc.CalculateHash();

  {
    xiiGALRenderTargetViewHandle hRenderTargetView;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTargetView))
    {
      return hRenderTargetView;
    }
  }

  xiiGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, desc);

  if (pRenderTargetView != nullptr)
  {
    xiiGALRenderTargetViewHandle hRenderTargetView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hRenderTargetView);

    return hRenderTargetView;
  }

  return xiiGALRenderTargetViewHandle();
}

void xiiGALDevice::DestroyRenderTargetView(xiiGALRenderTargetViewHandle hRenderTargetView)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALRenderTargetView* pRenderTargetView = nullptr;

  if (m_RenderTargetViews.TryGetValue(hRenderTargetView, pRenderTargetView))
  {
    AddDeadObject(GALObjectType::RenderTargetView, hRenderTargetView);
  }
  else
  {
    xiiLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}

xiiGALUnorderedAccessViewHandle xiiGALDevice::CreateUnorderedAccessView(const xiiGALUnorderedAccessViewCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  if (!desc.m_hTexture.IsInvalidated() && !desc.m_hBuffer.IsInvalidated())
  {
    xiiLog::Error("Can't pass both a texture and buffer to a xiiGALUnorderedAccessViewCreationDescription.");
    return xiiGALUnorderedAccessViewHandle();
  }

  xiiGALResourceBase* pResource = nullptr;
  xiiGALTexture*      pTexture  = nullptr;
  xiiGALBuffer*       pBuffer   = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
  {
    pResource = pTexture = Get<TextureTable, xiiGALTexture>(desc.m_hTexture, m_Textures);
  }
  else if (!desc.m_hBuffer.IsInvalidated())
  {
    pResource = pBuffer = Get<BufferTable, xiiGALBuffer>(desc.m_hBuffer, m_Buffers);
  }

  if (pResource == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return xiiGALUnorderedAccessViewHandle();
  }

  // Some platform independent validation.
  {
    if (pTexture)
    {
      // Is this really platform independent?
      if (pTexture->GetDescription().m_SampleCount != xiiGALMSAASampleCount::None)
      {
        xiiLog::Error("Can't create unordered access view on textures with multisampling.");
        return xiiGALUnorderedAccessViewHandle();
      }
    }
    else
    {
      if (desc.m_OverrideViewFormat == xiiGALResourceFormat::Invalid)
      {
        xiiLog::Error("Invalid resource format is not allowed for buffer unordered access views!");
        return xiiGALUnorderedAccessViewHandle();
      }

      if (!pBuffer->GetDescription().m_bAllowRawViews && desc.m_bRawView)
      {
        xiiLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
        return xiiGALUnorderedAccessViewHandle();
      }
    }
  }

  // Hash desc and return potential existing one
  xiiUInt32 uiHash = desc.CalculateHash();

  {
    xiiGALUnorderedAccessViewHandle hUnorderedAccessView;
    if (pResource->m_UnorderedAccessViews.TryGetValue(uiHash, hUnorderedAccessView))
    {
      return hUnorderedAccessView;
    }
  }

  xiiGALUnorderedAccessView* pUnorderedAccessViewView = CreateUnorderedAccessViewPlatform(pResource, desc);

  if (pUnorderedAccessViewView != nullptr)
  {
    xiiGALUnorderedAccessViewHandle hUnorderedAccessView(m_UnorderedAccessViews.Insert(pUnorderedAccessViewView));
    pResource->m_UnorderedAccessViews.Insert(uiHash, hUnorderedAccessView);

    return hUnorderedAccessView;
  }

  return xiiGALUnorderedAccessViewHandle();
}

void xiiGALDevice::DestroyUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

  if (m_UnorderedAccessViews.TryGetValue(hUnorderedAccessViewHandle, pUnorderedAccesssView))
  {
    AddDeadObject(GALObjectType::UnorderedAccessView, hUnorderedAccessViewHandle);
  }
  else
  {
    xiiLog::Warning("DestroyUnorderedAccessView called on invalid handle (double free?)");
  }
}

xiiGALSwapChainHandle xiiGALDevice::CreateSwapChain(const SwapChainFactoryFunction& func)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  ///// \todo Platform independent validation
  //if (desc.m_pWindow == nullptr)
  //{
  //  xiiLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
  //  return xiiGALSwapChainHandle();
  //}

  xiiGALSwapChain* pSwapChain = func(&m_Allocator);
  //xiiGALSwapChainDX11* pSwapChain = XII_NEW(&m_Allocator, xiiGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pSwapChain);
    return xiiGALSwapChainHandle();
  }

  return xiiGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
}

xiiResult xiiGALDevice::UpdateSwapChain(xiiGALSwapChainHandle hSwapChain, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->UpdateSwapChain(this, newPresentMode);
  }
  else
  {
    xiiLog::Warning("UpdateSwapChain called on invalid handle.");
    return XII_FAILURE;
  }
}

void xiiGALDevice::DestroySwapChain(xiiGALSwapChainHandle hSwapChain)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, hSwapChain);
  }
  else
  {
    xiiLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

xiiGALQueryHandle xiiGALDevice::CreateQuery(const xiiGALQueryCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALQuery* pQuery = CreateQueryPlatform(desc);

  if (pQuery == nullptr)
  {
    return xiiGALQueryHandle();
  }
  else
  {
    return xiiGALQueryHandle(m_Queries.Insert(pQuery));
  }
}

void xiiGALDevice::DestroyQuery(xiiGALQueryHandle hQuery)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALQuery* pQuery = nullptr;

  if (m_Queries.TryGetValue(hQuery, pQuery))
  {
    AddDeadObject(GALObjectType::Query, hQuery);
  }
  else
  {
    xiiLog::Warning("DestroyQuery called on invalid handle (double free?)");
  }
}

xiiGALVertexDeclarationHandle xiiGALDevice::CreateVertexDeclaration(const xiiGALVertexDeclarationCreationDescription& desc)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  xiiUInt32 uiHash = desc.CalculateHash();

  {
    xiiGALVertexDeclarationHandle hVertexDeclaration;
    if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
    {
      xiiGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
      if (pVertexDeclaration->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
      }

      pVertexDeclaration->AddRef();
      return hVertexDeclaration;
    }
  }

  xiiGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();

    xiiGALVertexDeclarationHandle hVertexDeclaration(m_VertexDeclarations.Insert(pVertexDeclaration));
    m_VertexDeclarationTable.Insert(uiHash, hVertexDeclaration);

    return hVertexDeclaration;
  }

  return xiiGALVertexDeclarationHandle();
}

void xiiGALDevice::DestroyVertexDeclaration(xiiGALVertexDeclarationHandle hVertexDeclaration)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  xiiGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
    }
  }
  else
  {
    xiiLog::Warning("DestroyVertexDeclaration called on invalid handle (double free?)");
  }
}

xiiGALTextureHandle xiiGALDevice::GetBackBufferTextureFromSwapChain(xiiGALSwapChainHandle hSwapChain)
{
  xiiGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->GetBackBufferTexture();
  }
  else
  {
    XII_REPORT_FAILURE("Swap chain handle invalid");
    return xiiGALTextureHandle();
  }
}



// Misc functions

void xiiGALDevice::BeginFrame(const xiiUInt64 uiRenderFrame)
{
  {
    XII_PROFILE_SCOPE("BeforeBeginFrame");
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::BeforeBeginFrame;
    m_Events.Broadcast(e);
  }

  {
    XII_GALDEVICE_LOCK_AND_CHECK();
    XII_ASSERT_DEV(!m_bBeginFrameCalled, "You must call xiiGALDevice::EndFrame before you can call xiiGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;

    BeginFramePlatform(uiRenderFrame);
  }

  // TODO: move to beginrendering/compute calls
  //m_pPrimaryContext->ClearStatisticsCounters();

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::AfterBeginFrame;
    m_Events.Broadcast(e);
  }
}

void xiiGALDevice::EndFrame()
{
  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::BeforeEndFrame;
    m_Events.Broadcast(e);
  }

  {
    XII_GALDEVICE_LOCK_AND_CHECK();
    XII_ASSERT_DEV(m_bBeginFrameCalled, "You must have called xiiGALDevice::Begin before you can call xiiGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform();

    m_bBeginFrameCalled = false;
  }

  {
    xiiGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type    = xiiGALDeviceEvent::AfterEndFrame;
    m_Events.Broadcast(e);
  }
}

const xiiGALDeviceCapabilities& xiiGALDevice::GetCapabilities() const
{
  return m_Capabilities;
}

xiiUInt64 xiiGALDevice::GetMemoryConsumptionForTexture(const xiiGALTextureCreationDescription& desc) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  xiiUInt64 uiMemory = xiiUInt64(desc.m_uiWidth) * xiiUInt64(desc.m_uiHeight) * xiiUInt64(desc.m_uiDepth);
  uiMemory *= desc.m_uiArraySize;
  uiMemory *= xiiGALResourceFormat::GetBitsPerElement(desc.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= desc.m_SampleCount;

  // Also account for mip maps
  if (desc.m_uiMipLevelCount > 1)
  {
    uiMemory += static_cast<xiiUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


xiiUInt64 xiiGALDevice::GetMemoryConsumptionForBuffer(const xiiGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}


void xiiGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

void xiiGALDevice::DestroyViews(xiiGALResourceBase* pResource)
{
  XII_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    xiiGALResourceViewHandle hResourceView = it.Value();
    xiiGALResourceView*      pResourceView = m_ResourceViews[hResourceView];

    m_ResourceViews.Remove(hResourceView);

    DestroyResourceViewPlatform(pResourceView);
  }
  pResource->m_ResourceViews.Clear();
  pResource->m_hDefaultResourceView.Invalidate();

  for (auto it = pResource->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
  {
    xiiGALRenderTargetViewHandle hRenderTargetView = it.Value();
    xiiGALRenderTargetView*      pRenderTargetView = m_RenderTargetViews[hRenderTargetView];

    m_RenderTargetViews.Remove(hRenderTargetView);

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  pResource->m_RenderTargetViews.Clear();
  pResource->m_hDefaultRenderTargetView.Invalidate();

  for (auto it = pResource->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
  {
    xiiGALUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    xiiGALUnorderedAccessView*      pUnorderedAccessView = m_UnorderedAccessViews[hUnorderedAccessView];

    m_UnorderedAccessViews.Remove(hUnorderedAccessView);

    DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
  }
  pResource->m_UnorderedAccessViews.Clear();
}

void xiiGALDevice::DestroyDeadObjects()
{
  // Can't use range based for here since new objects might be added during iteration
  for (xiiUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    switch (deadObject.m_uiType)
    {
      case GALObjectType::BlendState:
      {
        xiiGALBlendStateHandle hBlendState(xiiGAL::xii16_16Id(deadObject.m_uiHandle));
        xiiGALBlendState*      pBlendState = nullptr;

        XII_VERIFY(m_BlendStates.Remove(hBlendState, &pBlendState), "BlendState not found in idTable");
        XII_VERIFY(m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash()), "BlendState not found in de-duplication table");

        DestroyBlendStatePlatform(pBlendState);

        break;
      }
      case GALObjectType::DepthStencilState:
      {
        xiiGALDepthStencilStateHandle hDepthStencilState(xiiGAL::xii16_16Id(deadObject.m_uiHandle));
        xiiGALDepthStencilState*      pDepthStencilState = nullptr;

        XII_VERIFY(m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState), "DepthStencilState not found in idTable");
        XII_VERIFY(m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash()),
                   "DepthStencilState not found in de-duplication table");

        DestroyDepthStencilStatePlatform(pDepthStencilState);

        break;
      }
      case GALObjectType::RasterizerState:
      {
        xiiGALRasterizerStateHandle hRasterizerState(xiiGAL::xii16_16Id(deadObject.m_uiHandle));
        xiiGALRasterizerState*      pRasterizerState = nullptr;

        XII_VERIFY(m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState), "RasterizerState not found in idTable");
        XII_VERIFY(
          m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash()), "RasterizerState not found in de-duplication table");

        DestroyRasterizerStatePlatform(pRasterizerState);

        break;
      }
      case GALObjectType::SamplerState:
      {
        xiiGALSamplerStateHandle hSamplerState(xiiGAL::xii16_16Id(deadObject.m_uiHandle));
        xiiGALSamplerState*      pSamplerState = nullptr;

        XII_VERIFY(m_SamplerStates.Remove(hSamplerState, &pSamplerState), "SamplerState not found in idTable");
        XII_VERIFY(m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash()), "SamplerState not found in de-duplication table");

        DestroySamplerStatePlatform(pSamplerState);

        break;
      }
      case GALObjectType::Shader:
      {
        xiiGALShaderHandle hShader(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALShader*      pShader = nullptr;

        m_Shaders.Remove(hShader, &pShader);

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        xiiGALBufferHandle hBuffer(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALBuffer*      pBuffer = nullptr;

        m_Buffers.Remove(hBuffer, &pBuffer);

        DestroyViews(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        xiiGALTextureHandle hTexture(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALTexture*      pTexture = nullptr;

        m_Textures.Remove(hTexture, &pTexture);

        DestroyViews(pTexture);
        DestroyTexturePlatform(pTexture);

        break;
      }
      case GALObjectType::ResourceView:
      {
        xiiGALResourceViewHandle hResourceView(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALResourceView*      pResourceView = nullptr;

        m_ResourceViews.Remove(hResourceView, &pResourceView);

        xiiGALResourceBase* pResource = pResourceView->m_pResource;
        XII_ASSERT_DEBUG(pResource != nullptr, "");

        XII_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
        pResourceView->m_pResource = nullptr;

        DestroyResourceViewPlatform(pResourceView);

        break;
      }
      case GALObjectType::RenderTargetView:
      {
        xiiGALRenderTargetViewHandle hRenderTargetView(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALRenderTargetView*      pRenderTargetView = nullptr;

        m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView);

        xiiGALTexture* pTexture = pRenderTargetView->m_pTexture;
        XII_ASSERT_DEBUG(pTexture != nullptr, "");
        XII_VERIFY(pTexture->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
        pRenderTargetView->m_pTexture = nullptr;

        DestroyRenderTargetViewPlatform(pRenderTargetView);

        break;
      }
      case GALObjectType::UnorderedAccessView:
      {
        xiiGALUnorderedAccessViewHandle hUnorderedAccessViewHandle(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALUnorderedAccessView*      pUnorderedAccesssView = nullptr;

        m_UnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView);

        xiiGALResourceBase* pResource = pUnorderedAccesssView->m_pResource;
        XII_ASSERT_DEBUG(pResource != nullptr, "");

        XII_VERIFY(pResource->m_UnorderedAccessViews.Remove(pUnorderedAccesssView->GetDescription().CalculateHash()), "");
        pUnorderedAccesssView->m_pResource = nullptr;

        DestroyUnorderedAccessViewPlatform(pUnorderedAccesssView);

        break;
      }
      case GALObjectType::SwapChain:
      {
        xiiGALSwapChainHandle hSwapChain(xiiGAL::xii16_16Id(deadObject.m_uiHandle));
        xiiGALSwapChain*      pSwapChain = nullptr;

        m_SwapChains.Remove(hSwapChain, &pSwapChain);

        if (pSwapChain != nullptr)
        {
          pSwapChain->DeInitPlatform(this).IgnoreResult();
          XII_DELETE(&m_Allocator, pSwapChain);
        }

        break;
      }
      case GALObjectType::Query:
      {
        xiiGALQueryHandle hQuery(xiiGAL::xii20_12Id(deadObject.m_uiHandle));
        xiiGALQuery*      pQuery = nullptr;

        m_Queries.Remove(hQuery, &pQuery);

        DestroyQueryPlatform(pQuery);

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        xiiGALVertexDeclarationHandle hVertexDeclaration(xiiGAL::xii18_14Id(deadObject.m_uiHandle));
        xiiGALVertexDeclaration*      pVertexDeclaration = nullptr;

        m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration);
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

const xiiGALSwapChain* xiiGALDevice::GetSwapChainInternal(xiiGALSwapChainHandle hSwapChain, const xiiRTTI* pRequestedType) const
{
  const xiiGALSwapChain* pSwapChain = GetSwapChain(hSwapChain);
  if (pSwapChain)
  {
    if (!pSwapChain->GetDescription().m_pSwapChainType->IsDerivedFrom(pRequestedType))
      return nullptr;
  }
  return pSwapChain;
}

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);
