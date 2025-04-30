#include <GraphicsNull/GraphicsNullPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsNull/CommandEncoder/CommandListNull.h>
#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>
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
#include <GraphicsNull/States/PipelineResourceSignatureNull.h>
#include <GraphicsNull/States/PipelineStateNull.h>
#include <GraphicsNull/States/RasterizerStateNull.h>

xiiInternal::NewInstance<xiiGALDevice> CreateNullDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceNull, pAllocator, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsNull, DeviceFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Null, .m_sShaderModel = "NULL_SM", .m_sShaderCompiler = "" };

    xiiGALDeviceFactory::RegisterImplementation("Null", &CreateNullDevice, implementation);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALDeviceFactory::UnregisterImplementation("Null");
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceNull::xiiGALDeviceNull(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(pAllocator, description)
{
}

xiiGALDeviceNull::~xiiGALDeviceNull() = default;

xiiGALCommandQueue* xiiGALDeviceNull::GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const
{
  XII_IGNORE_UNUSED(queueType);
  XII_IGNORE_UNUSED(bAllowGraphicsCommandQueueFallback);
  return m_pDefaultQueue.Borrow();
}

xiiResult xiiGALDeviceNull::InitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceNull::InitializePlatform");

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceNull::PostInitializePlatform()
{
  xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueType = xiiGALCommandQueueType::Graphics};

  m_pDefaultQueue = XII_NEW(&m_Allocator, xiiGALCommandQueueNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), queueDescription);

  return XII_SUCCESS;
}

void xiiGALDeviceNull::BeginFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains, const xiiUInt64 uiRenderFrame)
{
  XII_IGNORE_UNUSED(swapchains);
  XII_IGNORE_UNUSED(uiRenderFrame);
}

void xiiGALDeviceNull::EndFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains)
{
  for (auto pSwapChain : swapchains)
  {
    pSwapChain->Present();
  }

  ++m_uiFrameNumber;
}

xiiInternal::NewInstance<xiiGALSwapChain> xiiGALDeviceNull::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSwapChainNull> pSwapChainNull = XII_NEW(&m_Allocator, xiiGALSwapChainNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pSwapChainNull->InitPlatform().Succeeded())
    return pSwapChainNull;

  XII_DELETE(&m_Allocator, pSwapChainNull.m_pInstance);

  return pSwapChainNull;
}

xiiInternal::NewInstance<xiiGALBlendState> xiiGALDeviceNull::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBlendStateNull> pBlendStateNull = XII_NEW(&m_Allocator, xiiGALBlendStateNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pBlendStateNull->InitPlatform().Succeeded())
    return pBlendStateNull;

  XII_DELETE(&m_Allocator, pBlendStateNull.m_pInstance);

  return pBlendStateNull;
}

xiiInternal::NewInstance<xiiGALDepthStencilState> xiiGALDeviceNull::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALDepthStencilStateNull> pDepthStencilStateNull = XII_NEW(&m_Allocator, xiiGALDepthStencilStateNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pDepthStencilStateNull->InitPlatform().Succeeded())
    return pDepthStencilStateNull;

  XII_DELETE(&m_Allocator, pDepthStencilStateNull.m_pInstance);

  return pDepthStencilStateNull;
}

xiiInternal::NewInstance<xiiGALRasterizerState> xiiGALDeviceNull::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRasterizerStateNull> pRasterizerStateNull = XII_NEW(&m_Allocator, xiiGALRasterizerStateNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pRasterizerStateNull->InitPlatform().Succeeded())
    return pRasterizerStateNull;

  XII_DELETE(&m_Allocator, pRasterizerStateNull.m_pInstance);

  return pRasterizerStateNull;
}

xiiInternal::NewInstance<xiiGALShader> xiiGALDeviceNull::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALShaderNull> pShaderNull = XII_NEW(&m_Allocator, xiiGALShaderNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pShaderNull->InitPlatform().Succeeded())
    return pShaderNull;

  XII_DELETE(&m_Allocator, pShaderNull.m_pInstance);

  return pShaderNull;
}

xiiInternal::NewInstance<xiiGALBuffer> xiiGALDeviceNull::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiInternal::NewInstance<xiiGALBufferNull> pBufferNull = XII_NEW(&m_Allocator, xiiGALBufferNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pBufferNull->InitPlatform(pInitialData).Succeeded())
    return pBufferNull;

  XII_DELETE(&m_Allocator, pBufferNull.m_pInstance);

  return pBufferNull;
}

xiiInternal::NewInstance<xiiGALTexture> xiiGALDeviceNull::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiInternal::NewInstance<xiiGALTextureNull> pTextureNull = XII_NEW(&m_Allocator, xiiGALTextureNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pTextureNull->InitPlatform(pInitialData).Succeeded())
    return pTextureNull;

  XII_DELETE(&m_Allocator, pTextureNull.m_pInstance);

  return pTextureNull;
}

xiiInternal::NewInstance<xiiGALSampler> xiiGALDeviceNull::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSamplerNull> pSamplerNull = XII_NEW(&m_Allocator, xiiGALSamplerNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pSamplerNull->InitPlatform().Succeeded())
    return pSamplerNull;

  XII_DELETE(&m_Allocator, pSamplerNull.m_pInstance);

  return pSamplerNull;
}

xiiInternal::NewInstance<xiiGALQuery> xiiGALDeviceNull::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALQueryNull> pQueryNull = XII_NEW(&m_Allocator, xiiGALQueryNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pQueryNull->InitPlatform().Succeeded())
    return pQueryNull;

  XII_DELETE(&m_Allocator, pQueryNull.m_pInstance);

  return pQueryNull;
}

xiiInternal::NewInstance<xiiGALFence> xiiGALDeviceNull::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFenceNull> pFenceNull = XII_NEW(&m_Allocator, xiiGALFenceNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pFenceNull->InitPlatform().Succeeded())
    return pFenceNull;

  XII_DELETE(&m_Allocator, pFenceNull.m_pInstance);

  return pFenceNull;
}

xiiInternal::NewInstance<xiiGALRenderPass> xiiGALDeviceNull::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRenderPassNull> pRenderPassNull = XII_NEW(&m_Allocator, xiiGALRenderPassNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pRenderPassNull->InitPlatform().Succeeded())
    return pRenderPassNull;

  XII_DELETE(&m_Allocator, pRenderPassNull.m_pInstance);

  return pRenderPassNull;
}

xiiInternal::NewInstance<xiiGALFramebuffer> xiiGALDeviceNull::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFramebufferNull> pFramebufferNull = XII_NEW(&m_Allocator, xiiGALFramebufferNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pFramebufferNull->InitPlatform().Succeeded())
    return pFramebufferNull;

  XII_DELETE(&m_Allocator, pFramebufferNull.m_pInstance);

  return pFramebufferNull;
}

xiiInternal::NewInstance<xiiGALBottomLevelAS> xiiGALDeviceNull::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBottomLevelASNull> pBottomLevelASNull = XII_NEW(&m_Allocator, xiiGALBottomLevelASNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pBottomLevelASNull->InitPlatform().Succeeded())
    return pBottomLevelASNull;

  XII_DELETE(&m_Allocator, pBottomLevelASNull.m_pInstance);

  return pBottomLevelASNull;
}

xiiInternal::NewInstance<xiiGALTopLevelAS> xiiGALDeviceNull::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALTopLevelASNull> pTopLevelASNull = XII_NEW(&m_Allocator, xiiGALTopLevelASNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pTopLevelASNull->InitPlatform().Succeeded())
    return pTopLevelASNull;

  XII_DELETE(&m_Allocator, pTopLevelASNull.m_pInstance);

  return pTopLevelASNull;
}

xiiInternal::NewInstance<xiiGALPipelineResourceSignature> xiiGALDeviceNull::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALPipelineResourceSignatureNull> pPipelineResourceSignatureNull = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pPipelineResourceSignatureNull->InitPlatform().Succeeded())
    return pPipelineResourceSignatureNull;

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureNull.m_pInstance);

  return pPipelineResourceSignatureNull;
}

xiiInternal::NewInstance<xiiGALPipelineState> xiiGALDeviceNull::CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALPipelineStateNull> pPipelineStateNull = XII_NEW(&m_Allocator, xiiGALPipelineStateNull, xiiSharedPtr<xiiGALDeviceNull>(this, m_Allocator.GetParent()), description);

  if (pPipelineStateNull->InitPlatform().Succeeded())
    return pPipelineStateNull;

  XII_DELETE(&m_Allocator, pPipelineStateNull.m_pInstance);

  return pPipelineStateNull;
}

void xiiGALDeviceNull::WaitIdlePlatform()
{
}

xiiResult xiiGALDeviceNull::FillCapabilitiesPlatform()
{
  m_Description.m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Null;

  m_AdapterDescription.m_sAdapterName = "XII Null Graphics Adapter";
  m_AdapterDescription.m_Type         = xiiGALDeviceAdapterType::Software;
  m_AdapterDescription.m_Vendor       = xiiGALGraphicsAdapterVendor::Unknown;

  m_AdapterDescription.m_uiVendorID         = 12;
  m_AdapterDescription.m_uiDeviceID         = 22;
  m_AdapterDescription.m_uiVideoOutputCount = 0;

  // Memory properties

  m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory               = 8423211008;
  m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory         = 17001539584;
  m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory             = 0;
  m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation       = 0;
  m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags = xiiGALCPUAccessFlag::None;
  m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags  = xiiGALBindFlags::None;

  // Raytracing properties

  m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = 31;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = 1073741824;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = 16777216;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = 536870912;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = 16777216;
  m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = 1;
  m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = 1;
  m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = 16;
  m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = 8;
  m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = 256;
  m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = 16;
  m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = 32;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = 4096;
  m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = 64;
  m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags            = xiiGALRayTracingCapabilityFlags::StandaloneShaders | xiiGALRayTracingCapabilityFlags::InlineRayTracing | xiiGALRayTracingCapabilityFlags::IndirectRayTracing;

  // Wave operation properties

  m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize             = 32;
  m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize             = 32;
  m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures          = xiiGALWaveFeature::Basic | xiiGALWaveFeature::Vote | xiiGALWaveFeature::Arithmetic | xiiGALWaveFeature::Ballot | xiiGALWaveFeature::Quad;
  m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages = xiiGALShaderType::Pixel | xiiGALShaderType::Compute | xiiGALShaderType::Amplification | xiiGALShaderType::Mesh;

  // Buffer properties

  m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = 256;
  m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = 16;

  // Texture properties

  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = 16384;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = 2048;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = 16384;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = 2048;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = 2048;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = 16384;
  m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = true;
  m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = true;
  m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = true;
  m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = true;
  m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = true;

  // Sampler properties

  m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported = true;
  m_AdapterDescription.m_SamplerProperties.m_uiMaxAnisotropy              = 1U;
  m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported            = true;

  // Mesh shader properties

  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = 65536;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = 65536;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = 65536;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = 4194304;

  // Shading rate properties

  // Null graphics device does not handle shading rates yet.

  // Compute shader properties

  m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = 32768;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = 1024;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = 1024;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = 1024;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = 64;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = 65535;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = 65535;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = 65535;

  // Draw command properties

  m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::DrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance | xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;
  m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = 4294967295;
  m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = 4294967295;

  // Sparse resource properties

  // Null graphics device does not handle sparse resources yet.

  // Device features support.

  m_AdapterDescription.m_Features.m_SeparablePrograms             = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_WireframeFill                 = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_MultithreadedResourceCreation = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_GeometryShaders               = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_Tessellation                  = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_MeshShaders                   = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_RayTracing                    = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_OcclusionQueries              = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_BinaryOcclusionQueries        = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_TimestampQueries              = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_PipelineStatisticsQueries     = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_DurationQueries               = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_DepthBiasClamp                = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_DepthClamp                    = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_IndependentBlend              = xiiGALDeviceFeatureState::Enabled;
  m_AdapterDescription.m_Features.m_NativeFence                   = xiiGALDeviceFeatureState::Enabled;

  // Command queue properties

  // Null graphics device does not handle command queues yet.

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Device_Implementation_DeviceNull);
