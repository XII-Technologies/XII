#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>

#include <GraphicsVulkan/CommandEncoder/CommandEncoderVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/DiligentCore.h>
#include <GraphicsVulkan/Device/PassVulkan.h>
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
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

#include <Diligent/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>

/// Reroutes Diligent Logs To XII.
void xiiLogDiligent(enum Diligent::DEBUG_MESSAGE_SEVERITY Severity, const Diligent::Char* Message, const Diligent::Char* Function, const Diligent::Char* File, xiiInt32 Line)
{
  // Format Diligent string as it is in printf format.
  switch (Severity)
  {
    case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
      xiiLog::Info("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
      xiiLog::Warning("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
      xiiLog::SeriousWarning("{}", Message);
      break;
    case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
      xiiLog::Error("{}", Message);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

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
  using namespace Diligent;

  XII_LOG_BLOCK("xiiGALDeviceVulkan::InitializePlatform");

  auto FindAdapter = [this](Diligent::IEngineFactoryVk* pFactory, Diligent::Version apiVersion, Diligent::GraphicsAdapterInfo& adapterInfo, xiiUInt32& out_AdatapterID) -> xiiResult {
    xiiUInt32 uiAdapterCount = 0U;
    pFactory->EnumerateAdapters(apiVersion, uiAdapterCount, nullptr);

    xiiHybridArray<Diligent::GraphicsAdapterInfo, 2U> graphicsAdapters;
    graphicsAdapters.Reserve(uiAdapterCount);

    if (uiAdapterCount > 0U)
    {
      pFactory->EnumerateAdapters(apiVersion, uiAdapterCount, graphicsAdapters.GetData());
    }
    else
    {
      xiiLog::Error("Failed to find compatible hardware adapters.");
      return XII_FAILURE;
    }

    xiiUInt32 uiAdapterID = m_Description.m_uiAdapterID;
    if (uiAdapterID != XII_GAL_DEFAULT_ADAPTER_ID)
    {
      if (uiAdapterID < graphicsAdapters.GetCount())
      {
        m_Description.m_AdapterType = xiiDiligentTypeConversions::GetGALAdapterType(graphicsAdapters[uiAdapterID].Type);
      }
      else
      {
        xiiLog::Error("Adapter ID ('{0}') is invalid. Only {1} compatible adapter (s) present in the system.", uiAdapterID, graphicsAdapters.GetCount());

        uiAdapterID = XII_GAL_DEFAULT_ADAPTER_ID;
      }
    }

    if (uiAdapterID == XII_GAL_DEFAULT_ADAPTER_ID && m_Description.m_AdapterType != xiiGALDeviceAdapterType::Unknown)
    {
      for (xiiUInt32 i = 0; i < graphicsAdapters.GetCount(); ++i)
      {
        if (graphicsAdapters[i].Type == xiiDiligentTypeConversions::GetAdapterType(m_Description.m_AdapterType))
        {
          uiAdapterID = i;
          break;
        }
      }

      if (uiAdapterID == XII_GAL_DEFAULT_ADAPTER_ID)
      {
        xiiLog::Warning("Unable to find the requested adapter type. Using default adapter.");
      }
    }

    if (uiAdapterID == XII_GAL_DEFAULT_ADAPTER_ID)
    {
      m_Description.m_AdapterType = xiiGALDeviceAdapterType::Unknown;

      for (xiiUInt32 i = 0; i < graphicsAdapters.GetCount(); ++i)
      {
        const Diligent::GraphicsAdapterInfo& AdapterInfo = graphicsAdapters[i];
        const Diligent::ADAPTER_TYPE         AdapterType = adapterInfo.Type;

        XII_CHECK_AT_COMPILETIME_MSG((Diligent::ADAPTER_TYPE_DISCRETE > Diligent::ADAPTER_TYPE_INTEGRATED && Diligent::ADAPTER_TYPE_INTEGRATED > Diligent::ADAPTER_TYPE_SOFTWARE && Diligent::ADAPTER_TYPE_SOFTWARE > Diligent::ADAPTER_TYPE_UNKNOWN), "xiiGraphicsVulkan: Unexpected ADAPTER_TYPE enum ordering");

        if (AdapterType > xiiDiligentTypeConversions::GetAdapterType(m_Description.m_AdapterType))
        {
          // Prefer Discrete over Integrated over Software adapters.
          m_Description.m_AdapterType = xiiDiligentTypeConversions::GetGALAdapterType(AdapterType);
          m_Description.m_uiAdapterID = i;
        }
        else if (AdapterType == xiiDiligentTypeConversions::GetAdapterType(m_Description.m_AdapterType))
        {
          // Select adapter with more memory.
          const Diligent::AdapterMemoryInfo& newAdapterMemory     = adapterInfo.Memory;
          const xiiUInt64                    uiNewTotalMemory     = newAdapterMemory.LocalMemory + newAdapterMemory.HostVisibleMemory + newAdapterMemory.UnifiedMemory;
          const Diligent::AdapterMemoryInfo& currentAdapterMemory = graphicsAdapters[uiAdapterID].Memory;
          const xiiUInt64                    uiCurrentTotalMemory = currentAdapterMemory.LocalMemory + currentAdapterMemory.HostVisibleMemory + currentAdapterMemory.UnifiedMemory;

          if (uiNewTotalMemory > uiCurrentTotalMemory)
          {
            uiAdapterID = i;
          }
        }
      }
    }

    if (uiAdapterID != XII_GAL_DEFAULT_ADAPTER_ID)
    {
      adapterInfo = graphicsAdapters[uiAdapterID];

      xiiLog::Info("Using Adapter {0}: '{1}'", uiAdapterID, adapterInfo.Description);
    }

    out_AdatapterID = uiAdapterID;

    return XII_SUCCESS;
  };

#if ENGINE_DLL
  auto GetEngineFactoryVulkan = Diligent::LoadGraphicsEngineVk();
#endif

  auto* pFactoryVulkan = GetEngineFactoryVulkan();
  if (pFactoryVulkan == nullptr)
  {
    xiiLog::Error("Failed to load Vulkan Library.");
    return XII_FAILURE;
  }
  m_pEngineFactory = pFactoryVulkan;

  // Register custom message callback.
  m_pEngineFactory->SetMessageCallback(xiiLogDiligent);

  Diligent::EngineVkCreateInfo vkCreateInfo;
  vkCreateInfo.pRawMemAllocator = xiiDiligentCore::GetDiligentMemoryAllocator();
  vkCreateInfo.EnableValidation = m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled;

  vkCreateInfo.Features.SeparablePrograms                 = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SeparablePrograms);
  vkCreateInfo.Features.ShaderResourceQueries             = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderResourceQueries);
  vkCreateInfo.Features.WireframeFill                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_WireframeFill);
  vkCreateInfo.Features.MultithreadedResourceCreation     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MultithreadedResourceCreation);
  vkCreateInfo.Features.ComputeShaders                    = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ComputeShaders);
  vkCreateInfo.Features.GeometryShaders                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_GeometryShaders);
  vkCreateInfo.Features.Tessellation                      = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_Tessellation);
  vkCreateInfo.Features.MeshShaders                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MeshShaders);
  vkCreateInfo.Features.RayTracing                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_RayTracing);
  vkCreateInfo.Features.BindlessResources                 = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_BindlessResources);
  vkCreateInfo.Features.OcclusionQueries                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_OcclusionQueries);
  vkCreateInfo.Features.BinaryOcclusionQueries            = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_BinaryOcclusionQueries);
  vkCreateInfo.Features.TimestampQueries                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TimestampQueries);
  vkCreateInfo.Features.PipelineStatisticsQueries         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_PipelineStatisticsQueries);
  vkCreateInfo.Features.DurationQueries                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DurationQueries);
  vkCreateInfo.Features.DepthBiasClamp                    = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DepthBiasClamp);
  vkCreateInfo.Features.DepthClamp                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DepthClamp);
  vkCreateInfo.Features.IndependentBlend                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_IndependentBlend);
  vkCreateInfo.Features.DualSourceBlend                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DualSourceBlend);
  vkCreateInfo.Features.MultiViewport                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MultiViewport);
  vkCreateInfo.Features.TextureCompressionBC              = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureCompressionBC);
  vkCreateInfo.Features.VertexPipelineUAVWritesAndAtomics = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics);
  vkCreateInfo.Features.PixelUAVWritesAndAtomics          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_PixelUAVWritesAndAtomics);
  vkCreateInfo.Features.TextureUAVExtendedFormats         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureUAVExtendedFormats);
  vkCreateInfo.Features.ShaderFloat16                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderFloat16);
  vkCreateInfo.Features.ResourceBuffer16BitAccess         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ResourceBuffer16BitAccess);
  vkCreateInfo.Features.UniformBuffer16BitAccess          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_UniformBuffer16BitAccess);
  vkCreateInfo.Features.ShaderInputOutput16               = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderInputOutput16);
  vkCreateInfo.Features.ShaderInt8                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderInt8);
  vkCreateInfo.Features.ResourceBuffer8BitAccess          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ResourceBuffer8BitAccess);
  vkCreateInfo.Features.UniformBuffer8BitAccess           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_UniformBuffer8BitAccess);
  vkCreateInfo.Features.ShaderResourceRuntimeArray        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderResourceRuntimeArray);
  vkCreateInfo.Features.WaveOp                            = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_WaveOp);
  vkCreateInfo.Features.InstanceDataStepRate              = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_InstanceDataStepRate);
  vkCreateInfo.Features.NativeFence                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_NativeFence);
  vkCreateInfo.Features.TileShaders                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TileShaders);
  vkCreateInfo.Features.TransferQueueTimestampQueries     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TransferQueueTimestampQueries);
  vkCreateInfo.Features.VariableRateShading               = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_VariableRateShading);
  vkCreateInfo.Features.SparseResources                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SparseResources);
  vkCreateInfo.Features.SubpassFramebufferFetch           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SubpassFramebufferFetch);
  vkCreateInfo.Features.TextureComponentSwizzle           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureComponentSwizzle);

  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
    vkCreateInfo.SetValidationLevel(xiiDiligentTypeConversions::GetDeviceValidationLevel(m_Description.m_ValidationLevel));

  const char* const ppIgnoreDebugMessages[] = //
    {
      // Validation Performance Warning: [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ]
      // vertex shader writes to output location 1.0 which is not consumed by fragment shader
      "UNASSIGNED-CoreValidation-Shader-OutputNotConsumed" //
    };

  vkCreateInfo.ppIgnoreDebugMessageNames = ppIgnoreDebugMessages;
  vkCreateInfo.IgnoreDebugMessageCount   = XII_ARRAY_SIZE(ppIgnoreDebugMessages);

  xiiUInt32 uiAdapterID = xiiInvalidIndex;

  Diligent::GraphicsAdapterInfo adapterInfo;

  XII_SUCCEED_OR_RETURN_LOG(FindAdapter(pFactoryVulkan, vkCreateInfo.GraphicsAPIVersion, adapterInfo, uiAdapterID));

  vkCreateInfo.AdapterId = uiAdapterID;

  xiiUInt32 uiImmediateContextCount = xiiMath::Max(1U, vkCreateInfo.NumImmediateContexts);
  m_pDeviceContexts.SetCount(uiImmediateContextCount + vkCreateInfo.NumDeferredContexts);
  pFactoryVulkan->CreateDeviceAndContextsVk(vkCreateInfo, &m_pDevice, m_pDeviceContexts.GetData());

  if (m_pDevice == nullptr)
  {
    xiiLog::Error("Unable to load Diligent Engine in Vulkan mode. The API may not be available, or required features may not be supported by this GPU/Driver/OS version.");
    return XII_FAILURE;
  }

  FillFormatLookupTable();

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassVulkan, *this);

  return XII_SUCCESS;
}

void xiiGALDeviceVulkan::ReportLiveGPUObjects()
{
}

void xiiGALDeviceVulkan::FlushPendingObjects()
{
  DestroyDeadObjects();
}

xiiResult xiiGALDeviceVulkan::ShutdownPlatform()
{
  m_pDefaultPass.Clear();

  if (!m_pDeviceContexts.IsEmpty())
  {
    for (xiiUInt32 uiContext = 0; uiContext < m_pDeviceContexts.GetCount(); ++uiContext)
    {
      m_pDeviceContexts[uiContext]->Flush();
      m_pDeviceContexts[uiContext]->FinishFrame();
      m_pDeviceContexts[uiContext]->InvalidateState();

      XII_GAL_DILIGENT_PTR_RELEASE(m_pDeviceContexts[uiContext]);
    }
    m_pDeviceContexts.Clear();
  }

  XII_GAL_DILIGENT_PTR_RELEASE(m_pDevice);
  XII_GAL_DILIGENT_PTR_RELEASE(m_pEngineFactory);

  ReportLiveGPUObjects();

  return XII_SUCCESS;
}

void xiiGALDeviceVulkan::BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStringBuilder sb;
  sb.Format("{} - Frame {}", !sName.IsEmpty() ? sName : "Unavailable", GetImmediateContext()->GetFrameNumber());
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), sb);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void xiiGALDeviceVulkan::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif

  if (pSwapChain)
  {
    pSwapChain->Present(this);
  }
}

xiiGALPass* xiiGALDeviceVulkan::BeginPassPlatform(xiiStringView sName)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), sName);
#endif

  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceVulkan::EndPassPlatform(xiiGALPass* pPass)
{
  XII_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass.");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pGraphicsCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

void xiiGALDeviceVulkan::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
}

void xiiGALDeviceVulkan::EndFramePlatform()
{
  // Call FinishFrame() to release references to Swapchain resources
  {
    for (auto& pContext : m_pDeviceContexts)
    {
      pContext->Flush();
      pContext->FinishFrame();
    }
    m_pDevice->ReleaseStaleResources();
  }
}

xiiGALSwapChain* xiiGALDeviceVulkan::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = XII_NEW(&m_Allocator, xiiGALSwapChainVulkan, description);

  if (pSwapChainVulkan->InitPlatform(this).Succeeded())
    return pSwapChainVulkan;

  XII_DELETE(&m_Allocator, pSwapChainVulkan);

  return pSwapChainVulkan;
}

void xiiGALDeviceVulkan::DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain)
{
  xiiGALSwapChainVulkan* pSwapChainVulkan = static_cast<xiiGALSwapChainVulkan*>(pSwapChain);

  pSwapChainVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pSwapChainVulkan);
}

xiiGALBlendState* xiiGALDeviceVulkan::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = XII_NEW(&m_Allocator, xiiGALBlendStateVulkan, description);

  if (pBlendStateVulkan->InitPlatform(this).Succeeded())
    return pBlendStateVulkan;

  XII_DELETE(&m_Allocator, pBlendStateVulkan);

  return pBlendStateVulkan;
}

void xiiGALDeviceVulkan::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateVulkan* pBlendStateVulkan = static_cast<xiiGALBlendStateVulkan*>(pBlendState);

  pBlendStateVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateVulkan);
}

xiiGALDepthStencilState* xiiGALDeviceVulkan::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = XII_NEW(&m_Allocator, xiiGALDepthStencilStateVulkan, description);

  if (pDepthStencilStateVulkan->InitPlatform(this).Succeeded())
    return pDepthStencilStateVulkan;

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);

  return pDepthStencilStateVulkan;
}

void xiiGALDeviceVulkan::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateVulkan* pDepthStencilStateVulkan = static_cast<xiiGALDepthStencilStateVulkan*>(pDepthStencilState);

  pDepthStencilStateVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateVulkan);
}

xiiGALRasterizerState* xiiGALDeviceVulkan::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = XII_NEW(&m_Allocator, xiiGALRasterizerStateVulkan, description);

  if (pRasterizerStateVulkan->InitPlatform(this).Succeeded())
    return pRasterizerStateVulkan;

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);

  return pRasterizerStateVulkan;
}

void xiiGALDeviceVulkan::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateVulkan* pRasterizerStateVulkan = static_cast<xiiGALRasterizerStateVulkan*>(pRasterizerState);

  pRasterizerStateVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateVulkan);
}

xiiGALShader* xiiGALDeviceVulkan::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderVulkan* pShaderVulkan = XII_NEW(&m_Allocator, xiiGALShaderVulkan, description);

  if (pShaderVulkan->InitPlatform(this).Succeeded())
    return pShaderVulkan;

  XII_DELETE(&m_Allocator, pShaderVulkan);

  return pShaderVulkan;
}

void xiiGALDeviceVulkan::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderVulkan* pShaderVulkan = static_cast<xiiGALShaderVulkan*>(pShader);

  pShaderVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderVulkan);
}

xiiGALBuffer* xiiGALDeviceVulkan::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferVulkan* pBufferVulkan = XII_NEW(&m_Allocator, xiiGALBufferVulkan, description);

  if (pBufferVulkan->InitPlatform(this, pInitialData).Succeeded())
    return pBufferVulkan;

  XII_DELETE(&m_Allocator, pBufferVulkan);

  return pBufferVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  pBufferVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferVulkan);
}

xiiGALBufferView* xiiGALDeviceVulkan::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = XII_NEW(&m_Allocator, xiiGALBufferViewVulkan, pBuffer, description);

  if (pBufferViewVulkan->InitPlatform(this).Succeeded())
    return pBufferViewVulkan;

  XII_DELETE(&m_Allocator, pBufferViewVulkan);

  return pBufferViewVulkan;
}

void xiiGALDeviceVulkan::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewVulkan* pBufferViewVulkan = static_cast<xiiGALBufferViewVulkan*>(pBufferView);

  pBufferViewVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewVulkan);
}

xiiGALTexture* xiiGALDeviceVulkan::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureVulkan* pTextureVulkan = XII_NEW(&m_Allocator, xiiGALTextureVulkan, description);

  if (pTextureVulkan->InitPlatform(this, pInitialData).Succeeded())
    return pTextureVulkan;

  XII_DELETE(&m_Allocator, pTextureVulkan);

  return pTextureVulkan;
}

void xiiGALDeviceVulkan::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureVulkan* pTextureVulkan = static_cast<xiiGALTextureVulkan*>(pTexture);

  pTextureVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureVulkan);
}

xiiGALTextureView* xiiGALDeviceVulkan::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = XII_NEW(&m_Allocator, xiiGALTextureViewVulkan, pTexture, description);

  if (pTextureViewVulkan->InitPlatform(this).Succeeded())
    return pTextureViewVulkan;

  XII_DELETE(&m_Allocator, pTextureViewVulkan);

  return pTextureViewVulkan;
}

void xiiGALDeviceVulkan::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewVulkan* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pTextureView);

  pTextureViewVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewVulkan);
}

xiiGALSampler* xiiGALDeviceVulkan::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerVulkan* pSamplerVulkan = XII_NEW(&m_Allocator, xiiGALSamplerVulkan, description);

  if (pSamplerVulkan->InitPlatform(this).Succeeded())
    return pSamplerVulkan;

  XII_DELETE(&m_Allocator, pSamplerVulkan);

  return pSamplerVulkan;
}

void xiiGALDeviceVulkan::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerVulkan* pSamplerVulkan = static_cast<xiiGALSamplerVulkan*>(pSampler);

  pSamplerVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerVulkan);
}

xiiGALInputLayout* xiiGALDeviceVulkan::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = XII_NEW(&m_Allocator, xiiGALInputLayoutVulkan, description);

  if (pInputLayoutVulkan->InitPlatform(this).Succeeded())
    return pInputLayoutVulkan;

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);

  return pInputLayoutVulkan;
}

void xiiGALDeviceVulkan::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutVulkan* pInputLayoutVulkan = static_cast<xiiGALInputLayoutVulkan*>(pInputLayout);

  pInputLayoutVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutVulkan);
}

xiiGALQuery* xiiGALDeviceVulkan::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryVulkan* pQueryVulkan = XII_NEW(&m_Allocator, xiiGALQueryVulkan, description);

  if (pQueryVulkan->InitPlatform(this).Succeeded())
    return pQueryVulkan;

  XII_DELETE(&m_Allocator, pQueryVulkan);

  return pQueryVulkan;
}

void xiiGALDeviceVulkan::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryVulkan* pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);

  pQueryVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryVulkan);
}

xiiGALFence* xiiGALDeviceVulkan::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceVulkan* pFenceVulkan = XII_NEW(&m_Allocator, xiiGALFenceVulkan, description);

  if (pFenceVulkan->InitPlatform(this).Succeeded())
    return pFenceVulkan;

  XII_DELETE(&m_Allocator, pFenceVulkan);

  return pFenceVulkan;
}

void xiiGALDeviceVulkan::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceVulkan* pFenceVulkan = static_cast<xiiGALFenceVulkan*>(pFence);

  pFenceVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceVulkan);
}

xiiGALRenderPass* xiiGALDeviceVulkan::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = XII_NEW(&m_Allocator, xiiGALRenderPassVulkan, description);

  if (pRenderPassVulkan->InitPlatform(this).Succeeded())
    return pRenderPassVulkan;

  XII_DELETE(&m_Allocator, pRenderPassVulkan);

  return pRenderPassVulkan;
}

void xiiGALDeviceVulkan::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassVulkan* pRenderPassVulkan = static_cast<xiiGALRenderPassVulkan*>(pRenderPass);

  pRenderPassVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassVulkan);
}

xiiGALFramebuffer* xiiGALDeviceVulkan::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = XII_NEW(&m_Allocator, xiiGALFramebufferVulkan, description);

  if (pFramebufferVulkan->InitPlatform(this).Succeeded())
    return pFramebufferVulkan;

  XII_DELETE(&m_Allocator, pFramebufferVulkan);

  return pFramebufferVulkan;
}

void xiiGALDeviceVulkan::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferVulkan* pFramebufferVulkan = static_cast<xiiGALFramebufferVulkan*>(pFramebuffer);

  pFramebufferVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferVulkan);
}

xiiGALBottomLevelAS* xiiGALDeviceVulkan::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = XII_NEW(&m_Allocator, xiiGALBottomLevelASVulkan, description);

  if (pBottomLevelASVulkan->InitPlatform(this).Succeeded())
    return pBottomLevelASVulkan;

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);

  return pBottomLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASVulkan* pBottomLevelASVulkan = static_cast<xiiGALBottomLevelASVulkan*>(pBottomLevelAS);

  pBottomLevelASVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASVulkan);
}

xiiGALTopLevelAS* xiiGALDeviceVulkan::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = XII_NEW(&m_Allocator, xiiGALTopLevelASVulkan, description);

  if (pTopLevelASVulkan->InitPlatform(this).Succeeded())
    return pTopLevelASVulkan;

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);

  return pTopLevelASVulkan;
}

void xiiGALDeviceVulkan::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASVulkan* pTopLevelASVulkan = static_cast<xiiGALTopLevelASVulkan*>(pTopLevelAS);

  pTopLevelASVulkan->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASVulkan);
}

void xiiGALDeviceVulkan::WaitIdlePlatform()
{
  m_pDevice->IdleGPU();

  FlushPendingObjects();

  m_pDevice->ReleaseStaleResources(true);
}

void xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
  const Diligent::GraphicsAdapterInfo& adapterInformation = m_pDevice->GetAdapterInfo();

  m_AdapterDescription.m_sAdapterName = adapterInformation.Description;
  m_AdapterDescription.m_Type         = xiiDiligentTypeConversions::GetGALAdapterType(adapterInformation.Type);

  switch (adapterInformation.Vendor)
  {
    case Diligent::ADAPTER_VENDOR_UNKNOWN:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Unknown;
      break;
    case Diligent::ADAPTER_VENDOR_NVIDIA:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Nvidia;
      break;
    case Diligent::ADAPTER_VENDOR_AMD:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::AMD;
      break;
    case Diligent::ADAPTER_VENDOR_INTEL:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Intel;
      break;
    case Diligent::ADAPTER_VENDOR_ARM:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::ARM;
      break;
    case Diligent::ADAPTER_VENDOR_QUALCOMM:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Qualcomm;
      break;
    case Diligent::ADAPTER_VENDOR_IMGTECH:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::ImaginationTechnologies;
      break;
    case Diligent::ADAPTER_VENDOR_MSFT:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Microsoft;
      break;
    case Diligent::ADAPTER_VENDOR_APPLE:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Apple;
      break;
    case Diligent::ADAPTER_VENDOR_MESA:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Mesa;
      break;
    case Diligent::ADAPTER_VENDOR_BROADCOM:
      m_AdapterDescription.m_Vendor = xiiGALGraphicsAdapterVendor::Broadcom;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_AdapterDescription.m_uiVendorID         = adapterInformation.VendorId;
  m_AdapterDescription.m_uiDeviceID         = adapterInformation.DeviceId;
  m_AdapterDescription.m_uiVideoOutputCount = adapterInformation.NumOutputs;

  // Memory properties

  m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory         = adapterInformation.Memory.LocalMemory;
  m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory   = adapterInformation.Memory.HostVisibleMemory;
  m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory       = adapterInformation.Memory.UnifiedMemory;
  m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation = adapterInformation.Memory.MaxMemoryAllocation;

  if (adapterInformation.Memory.UnifiedMemoryCPUAccess & Diligent::CPU_ACCESS_READ)
    m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Read;
  if (adapterInformation.Memory.UnifiedMemoryCPUAccess & Diligent::CPU_ACCESS_WRITE)
    m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Write;

  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_VERTEX_BUFFER)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::VertexBuffer;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_INDEX_BUFFER)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::IndexBuffer;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_UNIFORM_BUFFER)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::UniformBuffer;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_SHADER_RESOURCE)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::ShaderResource;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_STREAM_OUTPUT)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::StreamOutput;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_RENDER_TARGET)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::RenderTarget;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_DEPTH_STENCIL)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::DepthStencil;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_INDIRECT_DRAW_ARGS)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::IndirectDrawArguments;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_INPUT_ATTACHMENT)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::InputAttachment;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_RAY_TRACING)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::RayTracing;
  if (adapterInformation.Memory.MemorylessTextureBindFlags & Diligent::BIND_SHADING_RATE)
    m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags |= xiiGALBindFlags::ShadingRate;

  // Raytracing properties

  m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = adapterInformation.RayTracing.MaxRecursionDepth;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = adapterInformation.RayTracing.MaxRayGenThreads;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = adapterInformation.RayTracing.MaxInstancesPerTLAS;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = adapterInformation.RayTracing.MaxPrimitivesPerBLAS;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = adapterInformation.RayTracing.MaxGeometriesPerBLAS;
  m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = adapterInformation.RayTracing.VertexBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = adapterInformation.RayTracing.IndexBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = adapterInformation.RayTracing.TransformBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = adapterInformation.RayTracing.BoxBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = adapterInformation.RayTracing.ScratchBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = adapterInformation.RayTracing.InstanceBufferAlignment;
  m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = adapterInformation.RayTracing.ShaderGroupHandleSize;
  m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = adapterInformation.RayTracing.MaxShaderRecordStride;
  m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = adapterInformation.RayTracing.ShaderGroupBaseAlignment;

  if (adapterInformation.RayTracing.CapFlags & Diligent::RAY_TRACING_CAP_FLAG_STANDALONE_SHADERS)
    m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::StandaloneShaders;
  if (adapterInformation.RayTracing.CapFlags & Diligent::RAY_TRACING_CAP_FLAG_INLINE_RAY_TRACING)
    m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::InlineRayTracing;
  if (adapterInformation.RayTracing.CapFlags & Diligent::RAY_TRACING_CAP_FLAG_INDIRECT_RAY_TRACING)
    m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::IndirectRayTracing;

  // Wave operation properties

  m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize = adapterInformation.WaveOp.MinSize;
  m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize = adapterInformation.WaveOp.MaxSize;

  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_VERTEX)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Vertex;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_PIXEL)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Pixel;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_GEOMETRY)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Geometry;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_HULL)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Hull;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_DOMAIN)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Domain;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_COMPUTE)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Compute;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_AMPLIFICATION)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Amplification;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_MESH)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Mesh;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_RAY_GEN)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::RayGeneration;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_RAY_CLOSEST_HIT)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::RayClosestHit;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_RAY_ANY_HIT)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::RayAnyHit;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_RAY_INTERSECTION)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::RayIntersection;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_CALLABLE)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Callable;
  if (adapterInformation.WaveOp.SupportedStages & Diligent::SHADER_TYPE_TILE)
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderStage::Tile;

  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_BASIC)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Basic;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_VOTE)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Vote;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_ARITHMETIC)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Arithmetic;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_BALLOUT)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::BallOut;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_SHUFFLE)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Shuffle;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_SHUFFLE_RELATIVE)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::ShuffleRelative;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_CLUSTERED)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Clustered;
  if (adapterInformation.WaveOp.Features & Diligent::WAVE_FEATURE_QUAD)
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Quad;

  // Buffer properties

  m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = adapterInformation.Buffer.ConstantBufferOffsetAlignment;
  m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = adapterInformation.Buffer.StructuredBufferOffsetAlignment;

  // Texture properties

  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = adapterInformation.Texture.MaxTexture1DDimension;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = adapterInformation.Texture.MaxTexture1DArraySlices;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = adapterInformation.Texture.MaxTexture2DDimension;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = adapterInformation.Texture.MaxTexture2DArraySlices;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = adapterInformation.Texture.MaxTexture3DDimension;
  m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = adapterInformation.Texture.MaxTextureCubeDimension;
  m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = adapterInformation.Texture.Texture2DMSSupported;
  m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = adapterInformation.Texture.Texture2DMSArraySupported;
  m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = adapterInformation.Texture.TextureViewSupported;
  m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = adapterInformation.Texture.CubemapArraysSupported;
  m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = adapterInformation.Texture.TextureView2DOn3DSupported;

  // Sampler properties

  m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported   = adapterInformation.Sampler.BorderSamplingModeSupported;
  m_AdapterDescription.m_SamplerProperties.m_bAnisotropicFilteringSupported = adapterInformation.Sampler.AnisotropicFilteringSupported;
  m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported              = adapterInformation.Sampler.LODBiasSupported;

  // Mesh shader properties

  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = adapterInformation.MeshShader.MaxThreadGroupCountX;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = adapterInformation.MeshShader.MaxThreadGroupCountY;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = adapterInformation.MeshShader.MaxThreadGroupCountZ;
  m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = adapterInformation.MeshShader.MaxThreadGroupTotalCount;

  // Shading rate properties

  for (xiiUInt32 i = 0; i < adapterInformation.ShadingRate.NumShadingRates; ++i)
  {
    const auto& refMode = adapterInformation.ShadingRate.ShadingRates[i];
    auto&       mode    = m_AdapterDescription.m_ShadingRateProperties.m_Mode.ExpandAndGetRef();

    if (refMode.Rate & Diligent::SHADING_RATE_1X1)
      mode.m_ShadingRate |= xiiGALShadingRate::_1X1;
    if (refMode.Rate & Diligent::SHADING_RATE_1X2)
      mode.m_ShadingRate |= xiiGALShadingRate::_1X2;
    if (refMode.Rate & Diligent::SHADING_RATE_1X4)
      mode.m_ShadingRate |= xiiGALShadingRate::_1X4;
    if (refMode.Rate & Diligent::SHADING_RATE_2X1)
      mode.m_ShadingRate |= xiiGALShadingRate::_2X1;
    if (refMode.Rate & Diligent::SHADING_RATE_2X2)
      mode.m_ShadingRate |= xiiGALShadingRate::_2X2;
    if (refMode.Rate & Diligent::SHADING_RATE_2X4)
      mode.m_ShadingRate |= xiiGALShadingRate::_2X4;
    if (refMode.Rate & Diligent::SHADING_RATE_4X1)
      mode.m_ShadingRate |= xiiGALShadingRate::_4X1;
    if (refMode.Rate & Diligent::SHADING_RATE_4X2)
      mode.m_ShadingRate |= xiiGALShadingRate::_4X2;
    if (refMode.Rate & Diligent::SHADING_RATE_4X4)
      mode.m_ShadingRate |= xiiGALShadingRate::_4X4;

    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_1)
      mode.m_SampleBits = xiiGALSampleCount::OneSample;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_2)
      mode.m_SampleBits = xiiGALSampleCount::TwoSamples;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_4)
      mode.m_SampleBits = xiiGALSampleCount::FourSamples;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_8)
      mode.m_SampleBits = xiiGALSampleCount::EightSamples;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_16)
      mode.m_SampleBits = xiiGALSampleCount::SixteenSamples;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_32)
      mode.m_SampleBits = xiiGALSampleCount::ThirtyTwoSamples;
    if (refMode.SampleBits & Diligent::SAMPLE_COUNT_64)
      mode.m_SampleBits = xiiGALSampleCount::SixtyFourSamples;
  }

  // Compute shader properties

  m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = adapterInformation.ComputeShader.SharedMemorySize;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = adapterInformation.ComputeShader.MaxThreadGroupInvocations;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = adapterInformation.ComputeShader.MaxThreadGroupSizeX;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = adapterInformation.ComputeShader.MaxThreadGroupSizeY;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = adapterInformation.ComputeShader.MaxThreadGroupSizeZ;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = adapterInformation.ComputeShader.MaxThreadGroupCountX;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = adapterInformation.ComputeShader.MaxThreadGroupCountY;
  m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = adapterInformation.ComputeShader.MaxThreadGroupCountZ;

  // Draw command properties

  if (adapterInformation.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_BASE_VERTEX)
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::BaseVertex;
  if (adapterInformation.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT)
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirect;
  if (adapterInformation.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_FIRST_INSTANCE)
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance;
  if (adapterInformation.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_NATIVE_MULTI_DRAW_INDIRECT)
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect;
  if (adapterInformation.DrawCommand.CapFlags & Diligent::DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_COUNTER_BUFFER)
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;

  m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = adapterInformation.DrawCommand.MaxIndexValue;
  m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = adapterInformation.DrawCommand.MaxDrawIndirectCount;

  // Sparse resource properties

  m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = adapterInformation.SparseResources.AddressSpaceSize;
  m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = adapterInformation.SparseResources.ResourceSpaceSize;

  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_SHADER_RESOURCE_RESIDENCY)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_BUFFER)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Buffer;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture2D;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture3D;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2_SAMPLES)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture2Samples;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_4_SAMPLES)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture4Samples;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_8_SAMPLES)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture8Samples;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_16_SAMPLES)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture16Samples;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_ALIASED)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Aliased;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_STANDARD_2D_TILE_SHAPE)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Standard2DTileShape;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_STANDARD_2DMS_TILE_SHAPE)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_ALIGNED_MIP_SIZE)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::AlignedMipSize;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_NON_RESIDENT_STRICT)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::NonResidentStrict;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D_ARRAY_MIP_TAIL)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_BUFFER_STANDARD_BLOCK)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::BufferStandardBlock;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_NON_RESIDENT_SAFE)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::NonResidentSafe;
  if (adapterInformation.SparseResources.CapFlags & Diligent::SPARSE_RESOURCE_CAP_FLAG_MIXED_RESOURCE_TYPE_SUPPORT)
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport;

  m_AdapterDescription.m_SparseResourceProperties.m_uiStandardBlockSize = adapterInformation.SparseResources.StandardBlockSize;

  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_VERTEX_BUFFER)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::VertexBuffer;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_INDEX_BUFFER)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::IndexBuffer;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_UNIFORM_BUFFER)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::UniformBuffer;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_SHADER_RESOURCE)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::ShaderResource;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_STREAM_OUTPUT)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::StreamOutput;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_RENDER_TARGET)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::RenderTarget;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_DEPTH_STENCIL)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::DepthStencil;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_UNORDERED_ACCESS)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::UnorderedAccess;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_INDIRECT_DRAW_ARGS)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::IndirectDrawArguments;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_INPUT_ATTACHMENT)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::InputAttachment;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_RAY_TRACING)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::RayTracing;
  if (adapterInformation.SparseResources.BufferBindFlags & Diligent::BIND_SHADING_RATE)
    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags |= xiiGALBindFlags::ShadingRate;

  // Device features support

  m_AdapterDescription.m_Features.m_SeparablePrograms                 = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.SeparablePrograms);
  m_AdapterDescription.m_Features.m_ShaderResourceQueries             = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ShaderResourceQueries);
  m_AdapterDescription.m_Features.m_WireframeFill                     = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.WireframeFill);
  m_AdapterDescription.m_Features.m_MultithreadedResourceCreation     = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.MultithreadedResourceCreation);
  m_AdapterDescription.m_Features.m_ComputeShaders                    = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ComputeShaders);
  m_AdapterDescription.m_Features.m_GeometryShaders                   = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.GeometryShaders);
  m_AdapterDescription.m_Features.m_Tessellation                      = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.Tessellation);
  m_AdapterDescription.m_Features.m_MeshShaders                       = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.MeshShaders);
  m_AdapterDescription.m_Features.m_RayTracing                        = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.RayTracing);
  m_AdapterDescription.m_Features.m_BindlessResources                 = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.BindlessResources);
  m_AdapterDescription.m_Features.m_OcclusionQueries                  = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.OcclusionQueries);
  m_AdapterDescription.m_Features.m_BinaryOcclusionQueries            = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.BinaryOcclusionQueries);
  m_AdapterDescription.m_Features.m_TimestampQueries                  = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TimestampQueries);
  m_AdapterDescription.m_Features.m_PipelineStatisticsQueries         = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.PipelineStatisticsQueries);
  m_AdapterDescription.m_Features.m_DurationQueries                   = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.DurationQueries);
  m_AdapterDescription.m_Features.m_DepthBiasClamp                    = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.DepthBiasClamp);
  m_AdapterDescription.m_Features.m_DepthClamp                        = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.DepthClamp);
  m_AdapterDescription.m_Features.m_IndependentBlend                  = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.IndependentBlend);
  m_AdapterDescription.m_Features.m_DualSourceBlend                   = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.DualSourceBlend);
  m_AdapterDescription.m_Features.m_MultiViewport                     = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.MultiViewport);
  m_AdapterDescription.m_Features.m_TextureCompressionBC              = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TextureCompressionBC);
  m_AdapterDescription.m_Features.m_VertexPipelineUAVWritesAndAtomics = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.VertexPipelineUAVWritesAndAtomics);
  m_AdapterDescription.m_Features.m_PixelUAVWritesAndAtomics          = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.PixelUAVWritesAndAtomics);
  m_AdapterDescription.m_Features.m_TextureUAVExtendedFormats         = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TextureUAVExtendedFormats);
  m_AdapterDescription.m_Features.m_ShaderFloat16                     = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ShaderFloat16);
  m_AdapterDescription.m_Features.m_ResourceBuffer16BitAccess         = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ResourceBuffer16BitAccess);
  m_AdapterDescription.m_Features.m_UniformBuffer16BitAccess          = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.UniformBuffer16BitAccess);
  m_AdapterDescription.m_Features.m_ShaderInputOutput16               = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ShaderInputOutput16);
  m_AdapterDescription.m_Features.m_ShaderInt8                        = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ShaderInt8);
  m_AdapterDescription.m_Features.m_ResourceBuffer8BitAccess          = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ResourceBuffer8BitAccess);
  m_AdapterDescription.m_Features.m_UniformBuffer8BitAccess           = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.UniformBuffer8BitAccess);
  m_AdapterDescription.m_Features.m_ShaderResourceRuntimeArray        = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.ShaderResourceRuntimeArray);
  m_AdapterDescription.m_Features.m_WaveOp                            = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.WaveOp);
  m_AdapterDescription.m_Features.m_InstanceDataStepRate              = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.InstanceDataStepRate);
  m_AdapterDescription.m_Features.m_NativeFence                       = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.NativeFence);
  m_AdapterDescription.m_Features.m_TileShaders                       = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TileShaders);
  m_AdapterDescription.m_Features.m_TransferQueueTimestampQueries     = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TransferQueueTimestampQueries);
  m_AdapterDescription.m_Features.m_VariableRateShading               = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.VariableRateShading);
  m_AdapterDescription.m_Features.m_SparseResources                   = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.SparseResources);
  m_AdapterDescription.m_Features.m_SubpassFramebufferFetch           = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.SubpassFramebufferFetch);
  m_AdapterDescription.m_Features.m_TextureComponentSwizzle           = xiiDiligentTypeConversions::GetGALDeviceFeatureState(adapterInformation.Features.TextureComponentSwizzle);

  // Command queue properties

  for (xiiUInt32 i = 0; i < adapterInformation.NumQueues; ++i)
  {
    const auto& refQueue = adapterInformation.Queues[i];
    auto&       queue    = m_AdapterDescription.m_CommandQueueProperties.ExpandAndGetRef();

    if (refQueue.QueueType & Diligent::COMMAND_QUEUE_TYPE_TRANSFER)
      queue.m_Type |= xiiGALCommandQueueType::Transfer;
    if (refQueue.QueueType & Diligent::COMMAND_QUEUE_TYPE_COMPUTE)
      queue.m_Type |= xiiGALCommandQueueType::Compute;
    if (refQueue.QueueType & Diligent::COMMAND_QUEUE_TYPE_GRAPHICS)
      queue.m_Type |= xiiGALCommandQueueType::Graphics;
    if (refQueue.QueueType & Diligent::COMMAND_QUEUE_TYPE_SPARSE_BINDING)
      queue.m_Type |= xiiGALCommandQueueType::SparseBinding;

    queue.m_MaxDeviceContexts      = refQueue.MaxDeviceContexts;
    queue.m_TextureCopyGranularity = refQueue.TextureCopyGranularity;
  }
}

void xiiGALDeviceVulkan::FillFormatLookupTable()
{
  // The list below is in the same order as the xiiGALTextureFormat enumeration, no format should be missing.

  // clang-format off

  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32Typeless,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32Float,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_FLOAT).IL(Diligent::TEX_FORMAT_RGBA32_FLOAT).RV(Diligent::TEX_FORMAT_RGBA32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32UInt,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_UINT).IL(Diligent::TEX_FORMAT_RGBA32_UINT).RV(Diligent::TEX_FORMAT_RGBA32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32SInt,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_SINT).IL(Diligent::TEX_FORMAT_RGBA32_SINT).RV(Diligent::TEX_FORMAT_RGBA32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32Typeless,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_TYPELESS).IL(Diligent::TEX_FORMAT_RGB32_TYPELESS).RV(Diligent::TEX_FORMAT_RGB32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32Float,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_FLOAT).IL(Diligent::TEX_FORMAT_RGB32_FLOAT).RV(Diligent::TEX_FORMAT_RGB32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32UInt,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_UINT).IL(Diligent::TEX_FORMAT_RGB32_UINT).RV(Diligent::TEX_FORMAT_RGB32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32SInt,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_SINT).IL(Diligent::TEX_FORMAT_RGB32_SINT).RV(Diligent::TEX_FORMAT_RGB32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16Typeless,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16Float,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_FLOAT).IL(Diligent::TEX_FORMAT_RGBA16_FLOAT).RV(Diligent::TEX_FORMAT_RGBA16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16UNormalized,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UNORM).IL(Diligent::TEX_FORMAT_RGBA16_UNORM).RV(Diligent::TEX_FORMAT_RGBA16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16UInt,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UINT).IL(Diligent::TEX_FORMAT_RGBA16_UINT).RV(Diligent::TEX_FORMAT_RGBA16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16SNormalized,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SNORM).IL(Diligent::TEX_FORMAT_RGBA16_SNORM).RV(Diligent::TEX_FORMAT_RGBA16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16SInt,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SINT).IL(Diligent::TEX_FORMAT_RGBA16_SINT).RV(Diligent::TEX_FORMAT_RGBA16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32Typeless,                 xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_TYPELESS).IL(Diligent::TEX_FORMAT_RG32_TYPELESS).RV(Diligent::TEX_FORMAT_RG32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32Float,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_FLOAT).IL(Diligent::TEX_FORMAT_RG32_FLOAT).RV(Diligent::TEX_FORMAT_RG32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32UInt,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_UINT).IL(Diligent::TEX_FORMAT_RG32_UINT).RV(Diligent::TEX_FORMAT_RG32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32SInt,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_SINT).IL(Diligent::TEX_FORMAT_RG32_SINT).RV(Diligent::TEX_FORMAT_RG32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32G8X24Typeless,             xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).RT(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).IL(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).RV(Diligent::TEX_FORMAT_R32G8X24_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D32FloatS8X24UInt,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).D(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).DS(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).S(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32FloatX8X24Typeless,        xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).RT(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).IL(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).RV(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::X32TypelessG8X24UInt,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).RT(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).IL(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).RV(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2Typeless,              xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).IL(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RV(Diligent::TEX_FORMAT_RGB10A2_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2UNormalized,           xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UNORM).IL(Diligent::TEX_FORMAT_RGB10A2_UNORM).RV(Diligent::TEX_FORMAT_RGB10A2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2UInt,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UINT).IL(Diligent::TEX_FORMAT_RGB10A2_UINT).RV(Diligent::TEX_FORMAT_RGB10A2_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG11B10Float,                 xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RT(Diligent::TEX_FORMAT_R11G11B10_FLOAT).IL(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RV(Diligent::TEX_FORMAT_R11G11B10_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8Typeless,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UNormalized,             xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM).IL(Diligent::TEX_FORMAT_RGBA8_UNORM).RV(Diligent::TEX_FORMAT_RGBA8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UNormalizedSRGB,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UInt,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UINT).IL(Diligent::TEX_FORMAT_RGBA8_UINT).RV(Diligent::TEX_FORMAT_RGBA8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8SNormalized,             xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SNORM).IL(Diligent::TEX_FORMAT_RGBA8_SNORM).RV(Diligent::TEX_FORMAT_RGBA8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8SInt,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SINT).IL(Diligent::TEX_FORMAT_RGBA8_SINT).RV(Diligent::TEX_FORMAT_RGBA8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16Typeless,                 xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_TYPELESS).IL(Diligent::TEX_FORMAT_RG16_TYPELESS).RV(Diligent::TEX_FORMAT_RG16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16Float,                    xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_FLOAT).RT(Diligent::TEX_FORMAT_RG16_FLOAT).IL(Diligent::TEX_FORMAT_RG16_FLOAT).RV(Diligent::TEX_FORMAT_RG16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16UNormalized,              xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UNORM).IL(Diligent::TEX_FORMAT_RG16_UNORM).RV(Diligent::TEX_FORMAT_RG16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16UInt,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UINT).IL(Diligent::TEX_FORMAT_RG16_UINT).RV(Diligent::TEX_FORMAT_RG16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16SNormalized,              xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SNORM).IL(Diligent::TEX_FORMAT_RG16_SNORM).RV(Diligent::TEX_FORMAT_RG16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16SInt,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SINT).IL(Diligent::TEX_FORMAT_RG16_SINT).RV(Diligent::TEX_FORMAT_RG16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_TYPELESS).IL(Diligent::TEX_FORMAT_R32_TYPELESS).RV(Diligent::TEX_FORMAT_R32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D32Float,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_TYPELESS).D(Diligent::TEX_FORMAT_D32_FLOAT).DS(Diligent::TEX_FORMAT_D32_FLOAT).S(Diligent::TEX_FORMAT_D32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32Float,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_FLOAT).IL(Diligent::TEX_FORMAT_R32_FLOAT).RV(Diligent::TEX_FORMAT_R32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32UInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_UINT).IL(Diligent::TEX_FORMAT_R32_UINT).RV(Diligent::TEX_FORMAT_R32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32SInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_SINT).IL(Diligent::TEX_FORMAT_R32_SINT).RV(Diligent::TEX_FORMAT_R32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R24G8Typeless,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R24G8_TYPELESS).RT(Diligent::TEX_FORMAT_R24G8_TYPELESS).IL(Diligent::TEX_FORMAT_R24G8_TYPELESS).RV(Diligent::TEX_FORMAT_R24G8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D24UNormalizedS8UInt,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).D(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).DS(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).S(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R24UNormalizedX8Typeless,     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).RT(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).IL(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).RV(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::X24TypelessG8UInt,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).RT(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).IL(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).RV(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_TYPELESS).IL(Diligent::TEX_FORMAT_RG8_TYPELESS).RV(Diligent::TEX_FORMAT_RG8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UNORM).IL(Diligent::TEX_FORMAT_RG8_UNORM).RV(Diligent::TEX_FORMAT_RG8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8UInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UINT).IL(Diligent::TEX_FORMAT_RG8_UINT).RV(Diligent::TEX_FORMAT_RG8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8SNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SNORM).IL(Diligent::TEX_FORMAT_RG8_SNORM).RV(Diligent::TEX_FORMAT_RG8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8SInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SINT).IL(Diligent::TEX_FORMAT_RG8_SINT).RV(Diligent::TEX_FORMAT_RG8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_TYPELESS).IL(Diligent::TEX_FORMAT_R16_TYPELESS).RV(Diligent::TEX_FORMAT_R16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16Float,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_FLOAT).IL(Diligent::TEX_FORMAT_R16_FLOAT).RV(Diligent::TEX_FORMAT_R16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D16UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_D16_UNORM).IL(Diligent::TEX_FORMAT_D16_UNORM).RV(Diligent::TEX_FORMAT_D16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).D(Diligent::TEX_FORMAT_R16_UNORM).DS(Diligent::TEX_FORMAT_R16_UNORM).S(Diligent::TEX_FORMAT_R16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16UInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_UINT).IL(Diligent::TEX_FORMAT_R16_UINT).RV(Diligent::TEX_FORMAT_R16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16SNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SNORM).IL(Diligent::TEX_FORMAT_R16_SNORM).RV(Diligent::TEX_FORMAT_R16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16SInt,                      xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SINT).IL(Diligent::TEX_FORMAT_R16_SINT).RV(Diligent::TEX_FORMAT_R16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8Typeless,                   xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_TYPELESS).IL(Diligent::TEX_FORMAT_R8_TYPELESS).RV(Diligent::TEX_FORMAT_R8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8UNormalized,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UNORM).IL(Diligent::TEX_FORMAT_R8_UNORM).RV(Diligent::TEX_FORMAT_R8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8UInt,                       xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UINT).IL(Diligent::TEX_FORMAT_R8_UINT).RV(Diligent::TEX_FORMAT_R8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8SNormalized,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SNORM).IL(Diligent::TEX_FORMAT_R8_SNORM).RV(Diligent::TEX_FORMAT_R8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8SInt,                       xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SINT).IL(Diligent::TEX_FORMAT_R8_SINT).RV(Diligent::TEX_FORMAT_R8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::A8UNormalized,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_A8_UNORM).IL(Diligent::TEX_FORMAT_A8_UNORM).RV(Diligent::TEX_FORMAT_A8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R1UNormalized,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R1_UNORM).RT(Diligent::TEX_FORMAT_R1_UNORM).IL(Diligent::TEX_FORMAT_R1_UNORM).RV(Diligent::TEX_FORMAT_R1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB9E5SharedExponent,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).RT(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).IL(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).RV(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8BG8UNormalized,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).RT(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).IL(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).RV(Diligent::TEX_FORMAT_RG8_B8G8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::GR8GB8UNormalized,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).RT(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).IL(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).RV(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_TYPELESS).IL(Diligent::TEX_FORMAT_BC1_TYPELESS).RV(Diligent::TEX_FORMAT_BC1_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_UNORM).IL(Diligent::TEX_FORMAT_BC1_UNORM).RV(Diligent::TEX_FORMAT_BC1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1UNormalizedSRGB,           xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC1_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC1_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_TYPELESS).IL(Diligent::TEX_FORMAT_BC2_TYPELESS).RV(Diligent::TEX_FORMAT_BC2_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_UNORM).IL(Diligent::TEX_FORMAT_BC2_UNORM).RV(Diligent::TEX_FORMAT_BC2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2UNormalizedSRGB,           xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC2_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC2_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_TYPELESS).IL(Diligent::TEX_FORMAT_BC3_TYPELESS).RV(Diligent::TEX_FORMAT_BC3_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_UNORM).IL(Diligent::TEX_FORMAT_BC3_UNORM).RV(Diligent::TEX_FORMAT_BC3_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3UNormalizedSRGB,           xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC3_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC3_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_TYPELESS).IL(Diligent::TEX_FORMAT_BC4_TYPELESS).RV(Diligent::TEX_FORMAT_BC4_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_UNORM).IL(Diligent::TEX_FORMAT_BC4_UNORM).RV(Diligent::TEX_FORMAT_BC4_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4SNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_SNORM).IL(Diligent::TEX_FORMAT_BC4_SNORM).RV(Diligent::TEX_FORMAT_BC4_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_TYPELESS).IL(Diligent::TEX_FORMAT_BC5_TYPELESS).RV(Diligent::TEX_FORMAT_BC5_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_UNORM).IL(Diligent::TEX_FORMAT_BC5_UNORM).RV(Diligent::TEX_FORMAT_BC5_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5SNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_SNORM).IL(Diligent::TEX_FORMAT_BC5_SNORM).RV(Diligent::TEX_FORMAT_BC5_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::B5G6R5UNormalized,            xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_B5G6R5_UNORM).RT(Diligent::TEX_FORMAT_B5G6R5_UNORM).IL(Diligent::TEX_FORMAT_B5G6R5_UNORM).RV(Diligent::TEX_FORMAT_B5G6R5_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::B5G5R5A1UNormalized,          xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).RT(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).IL(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).RV(Diligent::TEX_FORMAT_B5G5R5A1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8UNormalized,             xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM).IL(Diligent::TEX_FORMAT_BGRA8_UNORM).RV(Diligent::TEX_FORMAT_BGRA8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8UNormalized,             xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_UNORM).IL(Diligent::TEX_FORMAT_BGRX8_UNORM).RV(Diligent::TEX_FORMAT_BGRX8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized, xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).RT(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).IL(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).RV(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8Typeless,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_TYPELESS).IL(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RV(Diligent::TEX_FORMAT_BGRA8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8UNormalizedSRGB,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8Typeless,                xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_TYPELESS).IL(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RV(Diligent::TEX_FORMAT_BGRX8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8UNormalizedSRGB,         xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HTypeless,                 xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_TYPELESS).IL(Diligent::TEX_FORMAT_BC6H_TYPELESS).RV(Diligent::TEX_FORMAT_BC6H_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HUF16,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_UF16).IL(Diligent::TEX_FORMAT_BC6H_UF16).RV(Diligent::TEX_FORMAT_BC6H_UF16));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HSF16,                     xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_SF16).IL(Diligent::TEX_FORMAT_BC6H_SF16).RV(Diligent::TEX_FORMAT_BC6H_SF16));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7Typeless,                  xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_TYPELESS).IL(Diligent::TEX_FORMAT_BC7_TYPELESS).RV(Diligent::TEX_FORMAT_BC7_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7UNormalized,               xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_UNORM).IL(Diligent::TEX_FORMAT_BC7_UNORM).RV(Diligent::TEX_FORMAT_BC7_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7UNormalizedSRGB,           xiiGALFormatLookupEntryVulkan(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC7_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC7_UNORM_SRGB));

  // clang-format on
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_DeviceVulkan);
