#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>

#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceD3D12, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsD3D12, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  xiiGALDeviceFactory::RegisterCreatorFunc("D3D12", &CreateD3D12Device, "D3D12_SM60", "xiiShaderCompiler");
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterCreatorFunc("D3D12");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceD3D12::xiiGALDeviceD3D12(const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(description)
{
}

xiiGALDeviceD3D12::~xiiGALDeviceD3D12() = default;

xiiResult xiiGALDeviceD3D12::InitializePlatform()
{
  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::ReportLiveGPUObjects()
{
}

void xiiGALDeviceD3D12::FlushPendingObjects()
{
}

xiiResult xiiGALDeviceD3D12::ShutdownPlatform()
{
  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::BeginPipelinePlatform(xiiStringView Name, xiiGALSwapChain* pSwapChain)
{
}

void xiiGALDeviceD3D12::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
}

xiiGALPass* xiiGALDeviceD3D12::BeginPassPlatform(xiiStringView Name)
{
  return nullptr;
}

void xiiGALDeviceD3D12::EndPassPlatform(xiiGALPass* pPass)
{
}

void xiiGALDeviceD3D12::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
}

void xiiGALDeviceD3D12::EndFramePlatform()
{
}

xiiGALBlendState* xiiGALDeviceD3D12::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
}

xiiGALDepthStencilState* xiiGALDeviceD3D12::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
}

xiiGALRasterizerState* xiiGALDeviceD3D12::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
}

xiiGALShader* xiiGALDeviceD3D12::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyShaderPlatform(xiiGALShader* pShader)
{
}

xiiGALBuffer* xiiGALDeviceD3D12::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferD3D12* pBufferD3D12 = XII_NEW(&m_Allocator, xiiGALBufferD3D12, description);

  if (pBufferD3D12->InitPlatform(this, pInitialData).Succeeded())
    return pBufferD3D12;

  XII_DELETE(&m_Allocator, pBufferD3D12);

  return pBufferD3D12;
}

void xiiGALDeviceD3D12::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferD3D12* pBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  pBufferD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferD3D12);
}

xiiGALBufferView* xiiGALDeviceD3D12::CreateBufferViewPlatform(const xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
}

xiiGALTexture* xiiGALDeviceD3D12::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureD3D12* pTextureD3D12 = XII_NEW(&m_Allocator, xiiGALTextureD3D12, description);

  if (pTextureD3D12->InitPlatform(this, pInitialData).Succeeded())
    return pTextureD3D12;

  XII_DELETE(&m_Allocator, pTextureD3D12);

  return pTextureD3D12;
}

void xiiGALDeviceD3D12::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureD3D12* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pTexture);

  pTextureD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureD3D12);
}

xiiGALTextureView* xiiGALDeviceD3D12::CreateTextureViewPlatform(const xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
}

xiiGALSampler* xiiGALDeviceD3D12::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerD3D12* pSamplerD3D12 = XII_NEW(&m_Allocator, xiiGALSamplerD3D12, description);

  if (pSamplerD3D12->InitPlatform(this).Succeeded())
    return pSamplerD3D12;

  XII_DELETE(&m_Allocator, pSamplerD3D12);

  return pSamplerD3D12;
}

void xiiGALDeviceD3D12::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerD3D12* pSamplerD3D12 = static_cast<xiiGALSamplerD3D12*>(pSampler);

  pSamplerD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerD3D12);
}

xiiGALInputLayout* xiiGALDeviceD3D12::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
}

xiiGALQuery* xiiGALDeviceD3D12::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
}

xiiGALFence* xiiGALDeviceD3D12::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceD3D12* pFenceD3D12 = XII_NEW(&m_Allocator, xiiGALFenceD3D12, description);

  if (pFenceD3D12->InitPlatform(this).Succeeded())
    return pFenceD3D12;

  XII_DELETE(&m_Allocator, pFenceD3D12);

  return pFenceD3D12;
}

void xiiGALDeviceD3D12::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceD3D12* pFenceD3D12 = static_cast<xiiGALFenceD3D12*>(pFence);

  pFenceD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceD3D12);
}

xiiGALRenderPass* xiiGALDeviceD3D12::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
}

xiiGALFramebuffer* xiiGALDeviceD3D12::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
}

xiiGALBottomLevelAS* xiiGALDeviceD3D12::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
}

xiiGALTopLevelAS* xiiGALDeviceD3D12::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  return nullptr;
}

void xiiGALDeviceD3D12::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
}

void xiiGALDeviceD3D12::WaitIdlePlatform()
{
}

void xiiGALDeviceD3D12::FillCapabilitiesPlatform()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_DeviceD3D12);
