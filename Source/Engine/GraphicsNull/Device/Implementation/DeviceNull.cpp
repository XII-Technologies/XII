#include <GraphicsNull/GraphicsNullPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsNull/CommandEncoder/CommandEncoderNull.h>
#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Device/PassNull.h>
#include <GraphicsNull/Device/SwapChainNull.h>
#include <GraphicsNull/Resources/BottomLevelASNull.h>
#include <GraphicsNull/Resources/BufferNull.h>
#include <GraphicsNull/Resources/BufferViewNull.h>
#include <GraphicsNull/Resources/FenceNull.h>
#include <GraphicsNull/Resources/FramebufferNull.h>
#include <GraphicsNull/Resources/QueryNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>
#include <GraphicsNull/Resources/SamplerNull.h>
#include <GraphicsNull/Resources/TextureNull.h>
#include <GraphicsNull/Resources/TextureViewNull.h>
#include <GraphicsNull/Resources/TopLevelASNull.h>
#include <GraphicsNull/Shader/InputLayoutNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>
#include <GraphicsNull/States/BlendStateNull.h>
#include <GraphicsNull/States/DepthStencilStateNull.h>
#include <GraphicsNull/States/RasterizerStateNull.h>

xiiInternal::NewInstance<xiiGALDevice> CreateNullDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceNull, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsNull, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Null, .m_sShaderModel = "NULL_SM60", .m_sShaderCompiler = "xiiShaderCompiler" };

  xiiGALDeviceFactory::RegisterImplementation("Null", &CreateNullDevice, implementation);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterImplementation("Null");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceNull::xiiGALDeviceNull(const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(description)
{
}

xiiGALDeviceNull::~xiiGALDeviceNull() = default;

xiiResult xiiGALDeviceNull::InitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceNull::InitializePlatform");

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassNull, *this);

  return XII_SUCCESS;
}

void xiiGALDeviceNull::ReportLiveGPUObjects()
{
}

void xiiGALDeviceNull::FlushPendingObjects()
{
  DestroyDeadObjects();
}

xiiResult xiiGALDeviceNull::ShutdownPlatform()
{
  m_pDefaultPass.Clear();

  return XII_SUCCESS;
}

void xiiGALDeviceNull::BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStringBuilder sb;
  sb.Format("{} - Frame {}", !sName.IsEmpty() ? sName : "Unavailable", m_uiFrameNumber);
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), sb);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void xiiGALDeviceNull::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif

  if (pSwapChain)
  {
    pSwapChain->Present(this);
  }
}

xiiGALPass* xiiGALDeviceNull::BeginPassPlatform(xiiStringView sName)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), sName);
#endif

  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceNull::EndPassPlatform(xiiGALPass* pPass)
{
  XII_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass.");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

void xiiGALDeviceNull::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
}

void xiiGALDeviceNull::EndFramePlatform()
{
  ++m_uiFrameNumber;
}

xiiGALSwapChain* xiiGALDeviceNull::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiGALSwapChainNull* pSwapChainNull = XII_NEW(&m_Allocator, xiiGALSwapChainNull, description);

  if (pSwapChainNull->InitPlatform(this).Succeeded())
    return pSwapChainNull;

  XII_DELETE(&m_Allocator, pSwapChainNull);

  return pSwapChainNull;
}

void xiiGALDeviceNull::DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain)
{
  xiiGALSwapChainNull* pSwapChainNull = static_cast<xiiGALSwapChainNull*>(pSwapChain);

  pSwapChainNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pSwapChainNull);
}

xiiGALBlendState* xiiGALDeviceNull::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiGALBlendStateNull* pBlendStateNull = XII_NEW(&m_Allocator, xiiGALBlendStateNull, description);

  if (pBlendStateNull->InitPlatform(this).Succeeded())
    return pBlendStateNull;

  XII_DELETE(&m_Allocator, pBlendStateNull);

  return pBlendStateNull;
}

void xiiGALDeviceNull::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateNull* pBlendStateNull = static_cast<xiiGALBlendStateNull*>(pBlendState);

  pBlendStateNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateNull);
}

xiiGALDepthStencilState* xiiGALDeviceNull::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateNull* pDepthStencilStateNull = XII_NEW(&m_Allocator, xiiGALDepthStencilStateNull, description);

  if (pDepthStencilStateNull->InitPlatform(this).Succeeded())
    return pDepthStencilStateNull;

  XII_DELETE(&m_Allocator, pDepthStencilStateNull);

  return pDepthStencilStateNull;
}

void xiiGALDeviceNull::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateNull* pDepthStencilStateNull = static_cast<xiiGALDepthStencilStateNull*>(pDepthStencilState);

  pDepthStencilStateNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateNull);
}

xiiGALRasterizerState* xiiGALDeviceNull::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateNull* pRasterizerStateNull = XII_NEW(&m_Allocator, xiiGALRasterizerStateNull, description);

  if (pRasterizerStateNull->InitPlatform(this).Succeeded())
    return pRasterizerStateNull;

  XII_DELETE(&m_Allocator, pRasterizerStateNull);

  return pRasterizerStateNull;
}

void xiiGALDeviceNull::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateNull* pRasterizerStateNull = static_cast<xiiGALRasterizerStateNull*>(pRasterizerState);

  pRasterizerStateNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateNull);
}

xiiGALShader* xiiGALDeviceNull::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderNull* pShaderNull = XII_NEW(&m_Allocator, xiiGALShaderNull, description);

  if (pShaderNull->InitPlatform(this).Succeeded())
    return pShaderNull;

  XII_DELETE(&m_Allocator, pShaderNull);

  return pShaderNull;
}

void xiiGALDeviceNull::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderNull* pShaderNull = static_cast<xiiGALShaderNull*>(pShader);

  pShaderNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderNull);
}

xiiGALBuffer* xiiGALDeviceNull::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferNull* pBufferNull = XII_NEW(&m_Allocator, xiiGALBufferNull, description);

  if (pBufferNull->InitPlatform(this, pInitialData).Succeeded())
    return pBufferNull;

  XII_DELETE(&m_Allocator, pBufferNull);

  return pBufferNull;
}

void xiiGALDeviceNull::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferNull* pBufferNull = static_cast<xiiGALBufferNull*>(pBuffer);

  pBufferNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferNull);
}

xiiGALBufferView* xiiGALDeviceNull::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewNull* pBufferViewNull = XII_NEW(&m_Allocator, xiiGALBufferViewNull, pBuffer, description);

  if (pBufferViewNull->InitPlatform(this).Succeeded())
    return pBufferViewNull;

  XII_DELETE(&m_Allocator, pBufferViewNull);

  return pBufferViewNull;
}

void xiiGALDeviceNull::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewNull* pBufferViewNull = static_cast<xiiGALBufferViewNull*>(pBufferView);

  pBufferViewNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewNull);
}

xiiGALTexture* xiiGALDeviceNull::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureNull* pTextureNull = XII_NEW(&m_Allocator, xiiGALTextureNull, description);

  if (pTextureNull->InitPlatform(this, pInitialData).Succeeded())
    return pTextureNull;

  XII_DELETE(&m_Allocator, pTextureNull);

  return pTextureNull;
}

void xiiGALDeviceNull::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureNull* pTextureNull = static_cast<xiiGALTextureNull*>(pTexture);

  pTextureNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureNull);
}

xiiGALTextureView* xiiGALDeviceNull::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewNull* pTextureViewNull = XII_NEW(&m_Allocator, xiiGALTextureViewNull, pTexture, description);

  if (pTextureViewNull->InitPlatform(this).Succeeded())
    return pTextureViewNull;

  XII_DELETE(&m_Allocator, pTextureViewNull);

  return pTextureViewNull;
}

void xiiGALDeviceNull::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewNull* pTextureViewNull = static_cast<xiiGALTextureViewNull*>(pTextureView);

  pTextureViewNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewNull);
}

xiiGALSampler* xiiGALDeviceNull::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerNull* pSamplerNull = XII_NEW(&m_Allocator, xiiGALSamplerNull, description);

  if (pSamplerNull->InitPlatform(this).Succeeded())
    return pSamplerNull;

  XII_DELETE(&m_Allocator, pSamplerNull);

  return pSamplerNull;
}

void xiiGALDeviceNull::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerNull* pSamplerNull = static_cast<xiiGALSamplerNull*>(pSampler);

  pSamplerNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerNull);
}

xiiGALInputLayout* xiiGALDeviceNull::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiGALInputLayoutNull* pInputLayoutNull = XII_NEW(&m_Allocator, xiiGALInputLayoutNull, description);

  if (pInputLayoutNull->InitPlatform(this).Succeeded())
    return pInputLayoutNull;

  XII_DELETE(&m_Allocator, pInputLayoutNull);

  return pInputLayoutNull;
}

void xiiGALDeviceNull::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutNull* pInputLayoutNull = static_cast<xiiGALInputLayoutNull*>(pInputLayout);

  pInputLayoutNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutNull);
}

xiiGALQuery* xiiGALDeviceNull::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryNull* pQueryNull = XII_NEW(&m_Allocator, xiiGALQueryNull, description);

  if (pQueryNull->InitPlatform(this).Succeeded())
    return pQueryNull;

  XII_DELETE(&m_Allocator, pQueryNull);

  return pQueryNull;
}

void xiiGALDeviceNull::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryNull* pQueryNull = static_cast<xiiGALQueryNull*>(pQuery);

  pQueryNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryNull);
}

xiiGALFence* xiiGALDeviceNull::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceNull* pFenceNull = XII_NEW(&m_Allocator, xiiGALFenceNull, description);

  if (pFenceNull->InitPlatform(this).Succeeded())
    return pFenceNull;

  XII_DELETE(&m_Allocator, pFenceNull);

  return pFenceNull;
}

void xiiGALDeviceNull::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceNull* pFenceNull = static_cast<xiiGALFenceNull*>(pFence);

  pFenceNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceNull);
}

xiiGALRenderPass* xiiGALDeviceNull::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassNull* pRenderPassNull = XII_NEW(&m_Allocator, xiiGALRenderPassNull, description);

  if (pRenderPassNull->InitPlatform(this).Succeeded())
    return pRenderPassNull;

  XII_DELETE(&m_Allocator, pRenderPassNull);

  return pRenderPassNull;
}

void xiiGALDeviceNull::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassNull* pRenderPassNull = static_cast<xiiGALRenderPassNull*>(pRenderPass);

  pRenderPassNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassNull);
}

xiiGALFramebuffer* xiiGALDeviceNull::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferNull* pFramebufferNull = XII_NEW(&m_Allocator, xiiGALFramebufferNull, description);

  if (pFramebufferNull->InitPlatform(this).Succeeded())
    return pFramebufferNull;

  XII_DELETE(&m_Allocator, pFramebufferNull);

  return pFramebufferNull;
}

void xiiGALDeviceNull::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferNull* pFramebufferNull = static_cast<xiiGALFramebufferNull*>(pFramebuffer);

  pFramebufferNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferNull);
}

xiiGALBottomLevelAS* xiiGALDeviceNull::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASNull* pBottomLevelASNull = XII_NEW(&m_Allocator, xiiGALBottomLevelASNull, description);

  if (pBottomLevelASNull->InitPlatform(this).Succeeded())
    return pBottomLevelASNull;

  XII_DELETE(&m_Allocator, pBottomLevelASNull);

  return pBottomLevelASNull;
}

void xiiGALDeviceNull::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASNull* pBottomLevelASNull = static_cast<xiiGALBottomLevelASNull*>(pBottomLevelAS);

  pBottomLevelASNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASNull);
}

xiiGALTopLevelAS* xiiGALDeviceNull::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASNull* pTopLevelASNull = XII_NEW(&m_Allocator, xiiGALTopLevelASNull, description);

  if (pTopLevelASNull->InitPlatform(this).Succeeded())
    return pTopLevelASNull;

  XII_DELETE(&m_Allocator, pTopLevelASNull);

  return pTopLevelASNull;
}

void xiiGALDeviceNull::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASNull* pTopLevelASNull = static_cast<xiiGALTopLevelASNull*>(pTopLevelAS);

  pTopLevelASNull->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASNull);
}

void xiiGALDeviceNull::WaitIdlePlatform()
{
  FlushPendingObjects();
}

void xiiGALDeviceNull::FillCapabilitiesPlatform()
{
  m_Type                                                          = xiiGALGraphicsDeviceType::Null;
  m_AdapterDescription.m_Type                                     = xiiGALDeviceAdapterType::Software;
  m_AdapterDescription.m_sAdapterName                             = "XII Null Graphics Adapter";
  m_AdapterDescription.m_Features.m_MultithreadedResourceCreation = xiiGALDeviceFeatureState::Enabled;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Device_Implementation_DeviceNull);
