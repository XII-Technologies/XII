#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceVulkan, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsVulkan, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Vulkan, .m_sShaderModel = "VK_SM60", .m_sShaderCompiler = "xiiShaderCompiler" };

  xiiGALDeviceFactory::RegisterImplementation("Vulkan", &CreateVulkanDevice, implementation);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterImplementation("Vulkan");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceVulkan::xiiGALDeviceVulkan(const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(description)
{
}

xiiGALDeviceVulkan::~xiiGALDeviceVulkan() = default;

xiiResult xiiGALDeviceVulkan::InitializePlatform()
{

  return XII_FAILURE;
}

void xiiGALDeviceVulkan::ReportLiveGPUObjects()
{
}

void xiiGALDeviceVulkan::FlushPendingObjects()
{
  FlushDestroyedObjects();
}

xiiResult xiiGALDeviceVulkan::ShutdownPlatform()
{
  ReportLiveGPUObjects();

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceVulkan::CreateCommandQueuesPlatform()
{
  return XII_FAILURE;
}

void xiiGALDeviceVulkan::BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget();
  }
}

void xiiGALDeviceVulkan::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->Present();
  }
}

void xiiGALDeviceVulkan::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
}

void xiiGALDeviceVulkan::EndFramePlatform()
{
  // Call FinishFrame() to release references to Swapchain resources
}

xiiGALSwapChain* xiiGALDeviceVulkan::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = XII_NEW(&m_Allocator, xiiGALSwapChainVulkan, this, description);

  if (pSwapChainVulkan->InitPlatform().Succeeded())
    return pSwapChainVulkan;

  XII_DELETE(&m_Allocator, pSwapChainVulkan);

  return pSwapChainVulkan;
}

void xiiGALDeviceVulkan::DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = static_cast<xiiGALSwapChainVulkan*>(pSwapChain);

  pSwapChainVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSwapChainVulkan);
}

xiiGALCommandQueue* xiiGALDeviceVulkan::CreateCommandQueuePlatform(const xiiGALCommandQueueCreationDescription& description)
{
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = XII_NEW(&m_Allocator, xiiGALCommandQueueVulkan, this, description);

  if (pCommandQueueVulkan->InitPlatform().Succeeded())
    return pCommandQueueVulkan;

  XII_DELETE(&m_Allocator, pCommandQueueVulkan);

  return pCommandQueueVulkan;
}

void xiiGALDeviceVulkan::DestroyCommandQueuePlatform(xiiGALCommandQueue* pCommandQueue)
{
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(pCommandQueue);

  pCommandQueueVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pCommandQueueVulkan);
}

xiiGALBlendState* xiiGALDeviceVulkan::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = XII_NEW(&m_Allocator, xiiGALBlendStateVulkan, this, description);

  if (pBlendStateVulkan->InitPlatform().Succeeded())
    return pBlendStateVulkan;

  XII_DELETE(&m_Allocator, pBlendStateVulkan);

  return pBlendStateVulkan;
}

void xiiGALDeviceVulkan::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = static_cast<xiiGALBlendStateVulkan*>(pBlendState);

  pBlendStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateVulkan);
}

xiiGALDepthStencilState* xiiGALDeviceVulkan::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = XII_NEW(&m_Allocator, xiiGALDepthStencilStateVulkan, this, description);

  if (pDepthStencilStateVulkan->InitPlatform().Succeeded())
    return pDepthStencilStateVulkan;

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);

  return pDepthStencilStateVulkan;
}

void xiiGALDeviceVulkan::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = static_cast<xiiGALDepthStencilStateVulkan*>(pDepthStencilState);

  pDepthStencilStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);
}

xiiGALRasterizerState* xiiGALDeviceVulkan::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = XII_NEW(&m_Allocator, xiiGALRasterizerStateVulkan, this, description);

  if (pRasterizerStateVulkan->InitPlatform().Succeeded())
    return pRasterizerStateVulkan;

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);

  return pRasterizerStateVulkan;
}

void xiiGALDeviceVulkan::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = static_cast<xiiGALRasterizerStateVulkan*>(pRasterizerState);

  pRasterizerStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);
}

xiiGALShader* xiiGALDeviceVulkan::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderVulkan* pShaderVulkan = XII_NEW(&m_Allocator, xiiGALShaderVulkan, this, description);

  if (pShaderVulkan->InitPlatform().Succeeded())
    return pShaderVulkan;

  XII_DELETE(&m_Allocator, pShaderVulkan);

  return pShaderVulkan;
}

void xiiGALDeviceVulkan::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pShader);

  pShaderVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderVulkan);
}

xiiGALBuffer* xiiGALDeviceVulkan::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferVulkan* pBufferVulkan = XII_NEW(&m_Allocator, xiiGALBufferVulkan, this, description);

  if (pBufferVulkan->InitPlatform(pInitialData).Succeeded())
    return pBufferVulkan;

  XII_DELETE(&m_Allocator, pBufferVulkan);

  return pBufferVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  pBufferVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferVulkan);
}

xiiGALBufferView* xiiGALDeviceVulkan::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = XII_NEW(&m_Allocator, xiiGALBufferViewVulkan, this, pBuffer, description);

  if (pBufferViewVulkan->InitPlatform().Succeeded())
    return pBufferViewVulkan;

  XII_DELETE(&m_Allocator, pBufferViewVulkan);

  return pBufferViewVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = static_cast<xiiGALBufferViewVulkan*>(pBufferView);

  pBufferViewVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewVulkan);
}

xiiGALTexture* xiiGALDeviceVulkan::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureVulkan* pTextureVulkan = XII_NEW(&m_Allocator, xiiGALTextureVulkan, this, description);

  if (pTextureVulkan->InitPlatform(pInitialData).Succeeded())
    return pTextureVulkan;

  XII_DELETE(&m_Allocator, pTextureVulkan);

  return pTextureVulkan;
}

void xiiGALDeviceVulkan::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureVulkan* pTextureVulkan = static_cast<xiiGALTextureVulkan*>(pTexture);

  pTextureVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureVulkan);
}

xiiGALTextureView* xiiGALDeviceVulkan::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = XII_NEW(&m_Allocator, xiiGALTextureViewVulkan, this, pTexture, description);

  if (pTextureViewVulkan->InitPlatform().Succeeded())
    return pTextureViewVulkan;

  XII_DELETE(&m_Allocator, pTextureViewVulkan);

  return pTextureViewVulkan;
}

void xiiGALDeviceVulkan::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pTextureView);

  pTextureViewVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewVulkan);
}

xiiGALSampler* xiiGALDeviceVulkan::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerVulkan* pSamplerVulkan = XII_NEW(&m_Allocator, xiiGALSamplerVulkan, this, description);

  if (pSamplerVulkan->InitPlatform().Succeeded())
    return pSamplerVulkan;

  XII_DELETE(&m_Allocator, pSamplerVulkan);

  return pSamplerVulkan;
}

void xiiGALDeviceVulkan::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerVulkan* pSamplerVulkan = static_cast<xiiGALSamplerVulkan*>(pSampler);

  pSamplerVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerVulkan);
}

xiiGALInputLayout* xiiGALDeviceVulkan::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = XII_NEW(&m_Allocator, xiiGALInputLayoutVulkan, this, description);

  if (pInputLayoutVulkan->InitPlatform().Succeeded())
    return pInputLayoutVulkan;

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);

  return pInputLayoutVulkan;
}

void xiiGALDeviceVulkan::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = static_cast<xiiGALInputLayoutVulkan*>(pInputLayout);

  pInputLayoutVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);
}

xiiGALQuery* xiiGALDeviceVulkan::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryVulkan* pQueryVulkan = XII_NEW(&m_Allocator, xiiGALQueryVulkan, this, description);

  if (pQueryVulkan->InitPlatform().Succeeded())
    return pQueryVulkan;

  XII_DELETE(&m_Allocator, pQueryVulkan);

  return pQueryVulkan;
}

void xiiGALDeviceVulkan::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryVulkan* pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);

  pQueryVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryVulkan);
}

xiiGALFence* xiiGALDeviceVulkan::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceVulkan* pFenceVulkan = XII_NEW(&m_Allocator, xiiGALFenceVulkan, this, description);

  if (pFenceVulkan->InitPlatform().Succeeded())
    return pFenceVulkan;

  XII_DELETE(&m_Allocator, pFenceVulkan);

  return pFenceVulkan;
}

void xiiGALDeviceVulkan::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceVulkan* pFenceVulkan = static_cast<xiiGALFenceVulkan*>(pFence);

  pFenceVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceVulkan);
}

xiiGALRenderPass* xiiGALDeviceVulkan::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = XII_NEW(&m_Allocator, xiiGALRenderPassVulkan, this, description);

  if (pRenderPassVulkan->InitPlatform().Succeeded())
    return pRenderPassVulkan;

  XII_DELETE(&m_Allocator, pRenderPassVulkan);

  return pRenderPassVulkan;
}

void xiiGALDeviceVulkan::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = static_cast<xiiGALRenderPassVulkan*>(pRenderPass);

  pRenderPassVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassVulkan);
}

xiiGALFramebuffer* xiiGALDeviceVulkan::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = XII_NEW(&m_Allocator, xiiGALFramebufferVulkan, this, description);

  if (pFramebufferVulkan->InitPlatform().Succeeded())
    return pFramebufferVulkan;

  XII_DELETE(&m_Allocator, pFramebufferVulkan);

  return pFramebufferVulkan;
}

void xiiGALDeviceVulkan::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = static_cast<xiiGALFramebufferVulkan*>(pFramebuffer);

  pFramebufferVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferVulkan);
}

xiiGALBottomLevelAS* xiiGALDeviceVulkan::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = XII_NEW(&m_Allocator, xiiGALBottomLevelASVulkan, this, description);

  if (pBottomLevelASVulkan->InitPlatform().Succeeded())
    return pBottomLevelASVulkan;

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);

  return pBottomLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = static_cast<xiiGALBottomLevelASVulkan*>(pBottomLevelAS);

  pBottomLevelASVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);
}

xiiGALTopLevelAS* xiiGALDeviceVulkan::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = XII_NEW(&m_Allocator, xiiGALTopLevelASVulkan, this, description);

  if (pTopLevelASVulkan->InitPlatform().Succeeded())
    return pTopLevelASVulkan;

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);

  return pTopLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = static_cast<xiiGALTopLevelASVulkan*>(pTopLevelAS);

  pTopLevelASVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);
}

xiiGALPipelineResourceSignature* xiiGALDeviceVulkan::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureVulkan, this, description);

  if (pPipelineResourceSignatureVulkan->InitPlatform().Succeeded())
    return pPipelineResourceSignatureVulkan;

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureVulkan);

  return pPipelineResourceSignatureVulkan;
}

void xiiGALDeviceVulkan::DestroyPipelineResourceSignaturePlatform(xiiGALPipelineResourceSignature* pPipelineResourceSignature)
{
  xiiGALPipelineResourceSignatureVulkan* pPipelineResourceSignatureVulkan = static_cast<xiiGALPipelineResourceSignatureVulkan*>(pPipelineResourceSignature);

  pPipelineResourceSignatureVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureVulkan);
}

xiiGALPipelineState* xiiGALDeviceVulkan::CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description)
{
  xiiGALPipelineStateVulkan* pPipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALPipelineStateVulkan, this, description);

  if (pPipelineStateVulkan->InitPlatform().Succeeded())
    return pPipelineStateVulkan;

  XII_DELETE(&m_Allocator, pPipelineStateVulkan);

  return pPipelineStateVulkan;
}

void xiiGALDeviceVulkan::DestroyPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  xiiGALPipelineStateVulkan* pPipelineStateVulkan = static_cast<xiiGALPipelineStateVulkan*>(pPipelineState);

  pPipelineStateVulkan->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineStateVulkan);
}

void xiiGALDeviceVulkan::WaitIdlePlatform()
{
  FlushPendingObjects();
}

void xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
}

void xiiGALDeviceVulkan::CreateCommandQueues()
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_DeviceVulkan);
