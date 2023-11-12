#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>

#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

#include <Diligent/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>

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

xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceD3D12, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsD3D12, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Direct3D12, .m_sShaderModel = "D3D12_SM60", .m_sShaderCompiler = "xiiShaderCompiler" };

  xiiGALDeviceFactory::RegisterImplementation("D3D12", &CreateD3D12Device, implementation);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterImplementation("D3D12");
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
  using namespace Diligent;

  XII_LOG_BLOCK("xiiGALDeviceD3D12::InitializePlatform");

  auto FindAdapter = [this](Diligent::IEngineFactoryD3D12* pFactory, Diligent::Version apiVersion, Diligent::GraphicsAdapterInfo& adapterInfo, xiiUInt32& out_AdatapterID) -> xiiResult {
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

        XII_CHECK_AT_COMPILETIME_MSG((Diligent::ADAPTER_TYPE_DISCRETE > Diligent::ADAPTER_TYPE_INTEGRATED && Diligent::ADAPTER_TYPE_INTEGRATED > Diligent::ADAPTER_TYPE_SOFTWARE && Diligent::ADAPTER_TYPE_SOFTWARE > Diligent::ADAPTER_TYPE_UNKNOWN), "xiiGraphicsD3D12: Unexpected ADAPTER_TYPE enum ordering");

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


  xiiHybridArray<Diligent::IDeviceContext*, 1U> deviceContexts;
  xiiUInt32                                     uiImmediateContextCount = 0U;

#if ENGINE_DLL
  auto GetEngineFactoryD3D12 = Diligent::LoadGraphicsEngineD3D12();
#endif

  auto* pFactoryD3D12 = GetEngineFactoryD3D12();
  if (pFactoryD3D12->LoadD3D12() != Diligent::True)
  {
    xiiLog::Error("Failed to load Direct3D12 Library.");
    return XII_FAILURE;
  }
  m_pEngineFactory = pFactoryD3D12;

  // Register custom message callback.
  m_pEngineFactory->SetMessageCallback(xiiLogDiligent);

  Diligent::EngineD3D12CreateInfo d3d12CreateInfo;
  d3d12CreateInfo.GraphicsAPIVersion = {12, 0};
  d3d12CreateInfo.pRawMemAllocator   = &m_AllocatorDiligent;
  d3d12CreateInfo.EnableValidation   = m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled;

  d3d12CreateInfo.Features.SeparablePrograms                 = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SeparablePrograms);
  d3d12CreateInfo.Features.ShaderResourceQueries             = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderResourceQueries);
  d3d12CreateInfo.Features.WireframeFill                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_WireframeFill);
  d3d12CreateInfo.Features.MultithreadedResourceCreation     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MultithreadedResourceCreation);
  d3d12CreateInfo.Features.ComputeShaders                    = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ComputeShaders);
  d3d12CreateInfo.Features.GeometryShaders                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_GeometryShaders);
  d3d12CreateInfo.Features.Tessellation                      = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_Tessellation);
  d3d12CreateInfo.Features.MeshShaders                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MeshShaders);
  d3d12CreateInfo.Features.RayTracing                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_RayTracing);
  d3d12CreateInfo.Features.BindlessResources                 = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_BindlessResources);
  d3d12CreateInfo.Features.OcclusionQueries                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_OcclusionQueries);
  d3d12CreateInfo.Features.BinaryOcclusionQueries            = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_BinaryOcclusionQueries);
  d3d12CreateInfo.Features.TimestampQueries                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TimestampQueries);
  d3d12CreateInfo.Features.PipelineStatisticsQueries         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_PipelineStatisticsQueries);
  d3d12CreateInfo.Features.DurationQueries                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DurationQueries);
  d3d12CreateInfo.Features.DepthBiasClamp                    = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DepthBiasClamp);
  d3d12CreateInfo.Features.DepthClamp                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DepthClamp);
  d3d12CreateInfo.Features.IndependentBlend                  = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_IndependentBlend);
  d3d12CreateInfo.Features.DualSourceBlend                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_DualSourceBlend);
  d3d12CreateInfo.Features.MultiViewport                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_MultiViewport);
  d3d12CreateInfo.Features.TextureCompressionBC              = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureCompressionBC);
  d3d12CreateInfo.Features.VertexPipelineUAVWritesAndAtomics = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics);
  d3d12CreateInfo.Features.PixelUAVWritesAndAtomics          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_PixelUAVWritesAndAtomics);
  d3d12CreateInfo.Features.TextureUAVExtendedFormats         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureUAVExtendedFormats);
  d3d12CreateInfo.Features.ShaderFloat16                     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderFloat16);
  d3d12CreateInfo.Features.ResourceBuffer16BitAccess         = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ResourceBuffer16BitAccess);
  d3d12CreateInfo.Features.UniformBuffer16BitAccess          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_UniformBuffer16BitAccess);
  d3d12CreateInfo.Features.ShaderInputOutput16               = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderInputOutput16);
  d3d12CreateInfo.Features.ShaderInt8                        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderInt8);
  d3d12CreateInfo.Features.ResourceBuffer8BitAccess          = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ResourceBuffer8BitAccess);
  d3d12CreateInfo.Features.UniformBuffer8BitAccess           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_UniformBuffer8BitAccess);
  d3d12CreateInfo.Features.ShaderResourceRuntimeArray        = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_ShaderResourceRuntimeArray);
  d3d12CreateInfo.Features.WaveOp                            = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_WaveOp);
  d3d12CreateInfo.Features.InstanceDataStepRate              = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_InstanceDataStepRate);
  d3d12CreateInfo.Features.NativeFence                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_NativeFence);
  d3d12CreateInfo.Features.TileShaders                       = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TileShaders);
  d3d12CreateInfo.Features.TransferQueueTimestampQueries     = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TransferQueueTimestampQueries);
  d3d12CreateInfo.Features.VariableRateShading               = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_VariableRateShading);
  d3d12CreateInfo.Features.SparseResources                   = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SparseResources);
  d3d12CreateInfo.Features.SubpassFramebufferFetch           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_SubpassFramebufferFetch);
  d3d12CreateInfo.Features.TextureComponentSwizzle           = xiiDiligentTypeConversions::GetDeviceFeatureState(m_Description.m_DeviceFeatures.m_TextureComponentSwizzle);

  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
    d3d12CreateInfo.SetValidationLevel(xiiDiligentTypeConversions::GetDeviceValidationLevel(m_Description.m_ValidationLevel));

  try
  {
    xiiUInt32 uiAdapterID = xiiInvalidIndex;

    XII_SUCCEED_OR_RETURN_LOG(FindAdapter(pFactoryD3D12, d3d12CreateInfo.GraphicsAPIVersion, m_AdapterAttribs, uiAdapterID));

    d3d12CreateInfo.AdapterId = uiAdapterID;
  }
  catch (...)
  {
    xiiLog::Error("Failed to locate DirectX12 compatible hardware adapters.");
  }

  if (m_Description.m_AdapterType != xiiGALDeviceAdapterType::Software and m_Description.m_uiAdapterID != XII_GAL_DEFAULT_ADAPTER_ID)
  {
    // Display mode enumeration fails with error for software adapter.
    xiiUInt32 uiDisplayModeCount = 0U;
    pFactoryD3D12->EnumerateDisplayModes(d3d12CreateInfo.GraphicsAPIVersion, d3d12CreateInfo.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, uiDisplayModeCount, nullptr);
    m_DisplayModes.SetCount(uiDisplayModeCount);
    pFactoryD3D12->EnumerateDisplayModes(d3d12CreateInfo.GraphicsAPIVersion, d3d12CreateInfo.AdapterId, 0, Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB, uiDisplayModeCount, m_DisplayModes.GetData());
  }

  uiImmediateContextCount = xiiMath::Max(1U, d3d12CreateInfo.NumImmediateContexts);
  deviceContexts.SetCount(uiImmediateContextCount + d3d12CreateInfo.NumDeferredContexts);
  pFactoryD3D12->CreateDeviceAndContextsD3D12(d3d12CreateInfo, &m_pDevice, deviceContexts.GetData());

  if (m_pDevice == nullptr)
  {
    xiiLog::Error("Unable to load Diligent Engine in Direct3D12 mode. The API may not be available, or required features may not be supported by this GPU/Driver/OS version.");
    return XII_FAILURE;
  }

  m_uiImmediateContextsCount = uiImmediateContextCount;
  m_pDeviceContexts.SetCount(deviceContexts.GetCount());
  for (xiiUInt32 i = 0; i < deviceContexts.GetCount(); ++i)
    m_pDeviceContexts[i].Attach(deviceContexts[i]);

  FillFormatLookupTable();

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

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
  if (!m_pDeviceContexts.IsEmpty())
  {
    for (xiiUInt32 uiContext = 0; uiContext < m_uiImmediateContextsCount; ++uiContext)
    {
      m_pDeviceContexts[uiContext]->Flush();
      m_pDeviceContexts[uiContext]->FinishFrame();
      m_pDeviceContexts[uiContext]->InvalidateState();

      XII_GAL_DILIGENT_REF_RELEASE(m_pDeviceContexts[uiContext]);
    }

    m_pDeviceContexts.Clear();
  }

  XII_GAL_DILIGENT_REF_RELEASE(m_pDevice);
  XII_GAL_DILIGENT_REF_RELEASE(m_pEngineFactory);

  ReportLiveGPUObjects();

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
  xiiGALBlendStateD3D12* pBlendStateD3D12 = XII_NEW(&m_Allocator, xiiGALBlendStateD3D12, description);

  if (pBlendStateD3D12->InitPlatform(this).Succeeded())
    return pBlendStateD3D12;

  XII_DELETE(&m_Allocator, pBlendStateD3D12);

  return pBlendStateD3D12;
}

void xiiGALDeviceD3D12::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateD3D12* pBlendStateD3D12 = static_cast<xiiGALBlendStateD3D12*>(pBlendState);

  pBlendStateD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateD3D12);
}

xiiGALDepthStencilState* xiiGALDeviceD3D12::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateD3D12* pDepthStencilStateD3D12 = XII_NEW(&m_Allocator, xiiGALDepthStencilStateD3D12, description);

  if (pDepthStencilStateD3D12->InitPlatform(this).Succeeded())
    return pDepthStencilStateD3D12;

  XII_DELETE(&m_Allocator, pDepthStencilStateD3D12);

  return pDepthStencilStateD3D12;
}

void xiiGALDeviceD3D12::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateD3D12* pDepthStencilStateD3D12 = static_cast<xiiGALDepthStencilStateD3D12*>(pDepthStencilState);

  pDepthStencilStateD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateD3D12);
}

xiiGALRasterizerState* xiiGALDeviceD3D12::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateD3D12* pRasterizerStateD3D12 = XII_NEW(&m_Allocator, xiiGALRasterizerStateD3D12, description);

  if (pRasterizerStateD3D12->InitPlatform(this).Succeeded())
    return pRasterizerStateD3D12;

  XII_DELETE(&m_Allocator, pRasterizerStateD3D12);

  return pRasterizerStateD3D12;
}

void xiiGALDeviceD3D12::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateD3D12* pRasterizerStateD3D12 = static_cast<xiiGALRasterizerStateD3D12*>(pRasterizerState);

  pRasterizerStateD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateD3D12);
}

xiiGALShader* xiiGALDeviceD3D12::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderD3D12* pShaderD3D12 = XII_NEW(&m_Allocator, xiiGALShaderD3D12, description);

  if (pShaderD3D12->InitPlatform(this).Succeeded())
    return pShaderD3D12;

  XII_DELETE(&m_Allocator, pShaderD3D12);

  return pShaderD3D12;
}

void xiiGALDeviceD3D12::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderD3D12* pShaderD3D12 = static_cast<xiiGALShaderD3D12*>(pShader);

  pShaderD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderD3D12);
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

xiiGALBufferView* xiiGALDeviceD3D12::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = XII_NEW(&m_Allocator, xiiGALBufferViewD3D12, pBuffer, description);

  if (pBufferViewD3D12->InitPlatform(this).Succeeded())
    return pBufferViewD3D12;

  XII_DELETE(&m_Allocator, pBufferViewD3D12);

  return pBufferViewD3D12;
}

void xiiGALDeviceD3D12::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = static_cast<xiiGALBufferViewD3D12*>(pBufferView);

  pBufferViewD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewD3D12);
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

xiiGALTextureView* xiiGALDeviceD3D12::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = XII_NEW(&m_Allocator, xiiGALTextureViewD3D12, pTexture, description);

  if (pTextureViewD3D12->InitPlatform(this).Succeeded())
    return pTextureViewD3D12;

  XII_DELETE(&m_Allocator, pTextureViewD3D12);

  return pTextureViewD3D12;
}

void xiiGALDeviceD3D12::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pTextureView);

  pTextureViewD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewD3D12);
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
  xiiGALInputLayoutD3D12* pInputLayoutD3D12 = XII_NEW(&m_Allocator, xiiGALInputLayoutD3D12, description);

  if (pInputLayoutD3D12->InitPlatform(this).Succeeded())
    return pInputLayoutD3D12;

  XII_DELETE(&m_Allocator, pInputLayoutD3D12);

  return pInputLayoutD3D12;
}

void xiiGALDeviceD3D12::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutD3D12* pInputLayoutD3D12 = static_cast<xiiGALInputLayoutD3D12*>(pInputLayout);

  pInputLayoutD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutD3D12);
}

xiiGALQuery* xiiGALDeviceD3D12::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryD3D12* pQueryD3D12 = XII_NEW(&m_Allocator, xiiGALQueryD3D12, description);

  if (pQueryD3D12->InitPlatform(this).Succeeded())
    return pQueryD3D12;

  XII_DELETE(&m_Allocator, pQueryD3D12);

  return pQueryD3D12;
}

void xiiGALDeviceD3D12::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryD3D12* pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  pQueryD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryD3D12);
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
  xiiGALRenderPassD3D12* pRenderPassD3D12 = XII_NEW(&m_Allocator, xiiGALRenderPassD3D12, description);

  if (pRenderPassD3D12->InitPlatform(this).Succeeded())
    return pRenderPassD3D12;

  XII_DELETE(&m_Allocator, pRenderPassD3D12);

  return pRenderPassD3D12;
}

void xiiGALDeviceD3D12::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassD3D12* pRenderPassD3D12 = static_cast<xiiGALRenderPassD3D12*>(pRenderPass);

  pRenderPassD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassD3D12);
}

xiiGALFramebuffer* xiiGALDeviceD3D12::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferD3D12* pFramebufferD3D12 = XII_NEW(&m_Allocator, xiiGALFramebufferD3D12, description);

  if (pFramebufferD3D12->InitPlatform(this).Succeeded())
    return pFramebufferD3D12;

  XII_DELETE(&m_Allocator, pFramebufferD3D12);

  return pFramebufferD3D12;
}

void xiiGALDeviceD3D12::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferD3D12* pFramebufferD3D12 = static_cast<xiiGALFramebufferD3D12*>(pFramebuffer);

  pFramebufferD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferD3D12);
}

xiiGALBottomLevelAS* xiiGALDeviceD3D12::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALBottomLevelASD3D12, description);

  if (pBottomLevelASD3D12->InitPlatform(this).Succeeded())
    return pBottomLevelASD3D12;

  XII_DELETE(&m_Allocator, pBottomLevelASD3D12);

  return pBottomLevelASD3D12;
}

void xiiGALDeviceD3D12::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12 = static_cast<xiiGALBottomLevelASD3D12*>(pBottomLevelAS);

  pBottomLevelASD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASD3D12);
}

xiiGALTopLevelAS* xiiGALDeviceD3D12::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASD3D12* pTopLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALTopLevelASD3D12, description);

  if (pTopLevelASD3D12->InitPlatform(this).Succeeded())
    return pTopLevelASD3D12;

  XII_DELETE(&m_Allocator, pTopLevelASD3D12);

  return pTopLevelASD3D12;
}

void xiiGALDeviceD3D12::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASD3D12* pTopLevelASD3D12 = static_cast<xiiGALTopLevelASD3D12*>(pTopLevelAS);

  pTopLevelASD3D12->DeInitPlatform(this).IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASD3D12);
}

void xiiGALDeviceD3D12::WaitIdlePlatform()
{
  m_pDevice->IdleGPU();
}

void xiiGALDeviceD3D12::FillCapabilitiesPlatform()
{
}

void xiiGALDeviceD3D12::FillFormatLookupTable()
{
  // The list below is in the same order as the xiiGALTextureFormat enumeration, no format should be missing.

  // clang-format off

  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32Typeless,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32Float,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_FLOAT).IL(Diligent::TEX_FORMAT_RGBA32_FLOAT).RV(Diligent::TEX_FORMAT_RGBA32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32UInt,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_UINT).IL(Diligent::TEX_FORMAT_RGBA32_UINT).RV(Diligent::TEX_FORMAT_RGBA32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA32SInt,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA32_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA32_SINT).IL(Diligent::TEX_FORMAT_RGBA32_SINT).RV(Diligent::TEX_FORMAT_RGBA32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32Typeless,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_TYPELESS).IL(Diligent::TEX_FORMAT_RGB32_TYPELESS).RV(Diligent::TEX_FORMAT_RGB32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32Float,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_FLOAT).IL(Diligent::TEX_FORMAT_RGB32_FLOAT).RV(Diligent::TEX_FORMAT_RGB32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32UInt,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_UINT).IL(Diligent::TEX_FORMAT_RGB32_UINT).RV(Diligent::TEX_FORMAT_RGB32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB32SInt,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB32_TYPELESS).RT(Diligent::TEX_FORMAT_RGB32_SINT).IL(Diligent::TEX_FORMAT_RGB32_SINT).RV(Diligent::TEX_FORMAT_RGB32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16Typeless,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16Float,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_FLOAT).IL(Diligent::TEX_FORMAT_RGBA16_FLOAT).RV(Diligent::TEX_FORMAT_RGBA16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16UNormalized,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UNORM).IL(Diligent::TEX_FORMAT_RGBA16_UNORM).RV(Diligent::TEX_FORMAT_RGBA16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16UInt,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_UINT).IL(Diligent::TEX_FORMAT_RGBA16_UINT).RV(Diligent::TEX_FORMAT_RGBA16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16SNormalized,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SNORM).IL(Diligent::TEX_FORMAT_RGBA16_SNORM).RV(Diligent::TEX_FORMAT_RGBA16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA16SInt,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA16_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA16_SINT).IL(Diligent::TEX_FORMAT_RGBA16_SINT).RV(Diligent::TEX_FORMAT_RGBA16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32Typeless,                 xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_TYPELESS).IL(Diligent::TEX_FORMAT_RG32_TYPELESS).RV(Diligent::TEX_FORMAT_RG32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32Float,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_FLOAT).IL(Diligent::TEX_FORMAT_RG32_FLOAT).RV(Diligent::TEX_FORMAT_RG32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32UInt,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_UINT).IL(Diligent::TEX_FORMAT_RG32_UINT).RV(Diligent::TEX_FORMAT_RG32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG32SInt,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG32_TYPELESS).RT(Diligent::TEX_FORMAT_RG32_SINT).IL(Diligent::TEX_FORMAT_RG32_SINT).RV(Diligent::TEX_FORMAT_RG32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32G8X24Typeless,             xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).RT(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).IL(Diligent::TEX_FORMAT_R32G8X24_TYPELESS).RV(Diligent::TEX_FORMAT_R32G8X24_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D32FloatS8X24UInt,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).D(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).DS(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT).S(Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32FloatX8X24Typeless,        xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).RT(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).IL(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS).RV(Diligent::TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::X32TypelessG8X24UInt,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).RT(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).IL(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT).RV(Diligent::TEX_FORMAT_X32_TYPELESS_G8X24_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2Typeless,              xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).IL(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RV(Diligent::TEX_FORMAT_RGB10A2_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2UNormalized,           xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UNORM).IL(Diligent::TEX_FORMAT_RGB10A2_UNORM).RV(Diligent::TEX_FORMAT_RGB10A2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB10A2UInt,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB10A2_TYPELESS).RT(Diligent::TEX_FORMAT_RGB10A2_UINT).IL(Diligent::TEX_FORMAT_RGB10A2_UINT).RV(Diligent::TEX_FORMAT_RGB10A2_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG11B10Float,                 xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RT(Diligent::TEX_FORMAT_R11G11B10_FLOAT).IL(Diligent::TEX_FORMAT_R11G11B10_FLOAT).RV(Diligent::TEX_FORMAT_R11G11B10_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8Typeless,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_TYPELESS).IL(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RV(Diligent::TEX_FORMAT_RGBA8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UNormalized,             xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM).IL(Diligent::TEX_FORMAT_RGBA8_UNORM).RV(Diligent::TEX_FORMAT_RGBA8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UNormalizedSRGB,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8UInt,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_UINT).IL(Diligent::TEX_FORMAT_RGBA8_UINT).RV(Diligent::TEX_FORMAT_RGBA8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8SNormalized,             xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SNORM).IL(Diligent::TEX_FORMAT_RGBA8_SNORM).RV(Diligent::TEX_FORMAT_RGBA8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGBA8SInt,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGBA8_TYPELESS).RT(Diligent::TEX_FORMAT_RGBA8_SINT).IL(Diligent::TEX_FORMAT_RGBA8_SINT).RV(Diligent::TEX_FORMAT_RGBA8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16Typeless,                 xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_TYPELESS).IL(Diligent::TEX_FORMAT_RG16_TYPELESS).RV(Diligent::TEX_FORMAT_RG16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16Float,                    xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_FLOAT).RT(Diligent::TEX_FORMAT_RG16_FLOAT).IL(Diligent::TEX_FORMAT_RG16_FLOAT).RV(Diligent::TEX_FORMAT_RG16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16UNormalized,              xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UNORM).IL(Diligent::TEX_FORMAT_RG16_UNORM).RV(Diligent::TEX_FORMAT_RG16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16UInt,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_UINT).IL(Diligent::TEX_FORMAT_RG16_UINT).RV(Diligent::TEX_FORMAT_RG16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16SNormalized,              xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SNORM).IL(Diligent::TEX_FORMAT_RG16_SNORM).RV(Diligent::TEX_FORMAT_RG16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG16SInt,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG16_TYPELESS).RT(Diligent::TEX_FORMAT_RG16_SINT).IL(Diligent::TEX_FORMAT_RG16_SINT).RV(Diligent::TEX_FORMAT_RG16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_TYPELESS).IL(Diligent::TEX_FORMAT_R32_TYPELESS).RV(Diligent::TEX_FORMAT_R32_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D32Float,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_TYPELESS).D(Diligent::TEX_FORMAT_D32_FLOAT).DS(Diligent::TEX_FORMAT_D32_FLOAT).S(Diligent::TEX_FORMAT_D32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32Float,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_FLOAT).IL(Diligent::TEX_FORMAT_R32_FLOAT).RV(Diligent::TEX_FORMAT_R32_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32UInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_UINT).IL(Diligent::TEX_FORMAT_R32_UINT).RV(Diligent::TEX_FORMAT_R32_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R32SInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R32_TYPELESS).RT(Diligent::TEX_FORMAT_R32_SINT).IL(Diligent::TEX_FORMAT_R32_SINT).RV(Diligent::TEX_FORMAT_R32_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R24G8Typeless,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R24G8_TYPELESS).RT(Diligent::TEX_FORMAT_R24G8_TYPELESS).IL(Diligent::TEX_FORMAT_R24G8_TYPELESS).RV(Diligent::TEX_FORMAT_R24G8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D24UNormalizedS8UInt,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).D(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).DS(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT).S(Diligent::TEX_FORMAT_D24_UNORM_S8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R24UNormalizedX8Typeless,     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).RT(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).IL(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS).RV(Diligent::TEX_FORMAT_R24_UNORM_X8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::X24TypelessG8UInt,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).RT(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).IL(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT).RV(Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_TYPELESS).IL(Diligent::TEX_FORMAT_RG8_TYPELESS).RV(Diligent::TEX_FORMAT_RG8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UNORM).IL(Diligent::TEX_FORMAT_RG8_UNORM).RV(Diligent::TEX_FORMAT_RG8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8UInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_UINT).IL(Diligent::TEX_FORMAT_RG8_UINT).RV(Diligent::TEX_FORMAT_RG8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8SNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SNORM).IL(Diligent::TEX_FORMAT_RG8_SNORM).RV(Diligent::TEX_FORMAT_RG8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8SInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_TYPELESS).RT(Diligent::TEX_FORMAT_RG8_SINT).IL(Diligent::TEX_FORMAT_RG8_SINT).RV(Diligent::TEX_FORMAT_RG8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_TYPELESS).IL(Diligent::TEX_FORMAT_R16_TYPELESS).RV(Diligent::TEX_FORMAT_R16_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16Float,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_FLOAT).IL(Diligent::TEX_FORMAT_R16_FLOAT).RV(Diligent::TEX_FORMAT_R16_FLOAT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::D16UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_D16_UNORM).IL(Diligent::TEX_FORMAT_D16_UNORM).RV(Diligent::TEX_FORMAT_D16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).D(Diligent::TEX_FORMAT_R16_UNORM).DS(Diligent::TEX_FORMAT_R16_UNORM).S(Diligent::TEX_FORMAT_R16_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16UInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_UINT).IL(Diligent::TEX_FORMAT_R16_UINT).RV(Diligent::TEX_FORMAT_R16_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16SNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SNORM).IL(Diligent::TEX_FORMAT_R16_SNORM).RV(Diligent::TEX_FORMAT_R16_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R16SInt,                      xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R16_TYPELESS).RT(Diligent::TEX_FORMAT_R16_SINT).IL(Diligent::TEX_FORMAT_R16_SINT).RV(Diligent::TEX_FORMAT_R16_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8Typeless,                   xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_TYPELESS).IL(Diligent::TEX_FORMAT_R8_TYPELESS).RV(Diligent::TEX_FORMAT_R8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8UNormalized,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UNORM).IL(Diligent::TEX_FORMAT_R8_UNORM).RV(Diligent::TEX_FORMAT_R8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8UInt,                       xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_UINT).IL(Diligent::TEX_FORMAT_R8_UINT).RV(Diligent::TEX_FORMAT_R8_UINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8SNormalized,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SNORM).IL(Diligent::TEX_FORMAT_R8_SNORM).RV(Diligent::TEX_FORMAT_R8_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R8SInt,                       xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_R8_SINT).IL(Diligent::TEX_FORMAT_R8_SINT).RV(Diligent::TEX_FORMAT_R8_SINT));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::A8UNormalized,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R8_TYPELESS).RT(Diligent::TEX_FORMAT_A8_UNORM).IL(Diligent::TEX_FORMAT_A8_UNORM).RV(Diligent::TEX_FORMAT_A8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R1UNormalized,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R1_UNORM).RT(Diligent::TEX_FORMAT_R1_UNORM).IL(Diligent::TEX_FORMAT_R1_UNORM).RV(Diligent::TEX_FORMAT_R1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RGB9E5SharedExponent,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).RT(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).IL(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP).RV(Diligent::TEX_FORMAT_RGB9E5_SHAREDEXP));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::RG8BG8UNormalized,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).RT(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).IL(Diligent::TEX_FORMAT_RG8_B8G8_UNORM).RV(Diligent::TEX_FORMAT_RG8_B8G8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::GR8GB8UNormalized,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).RT(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).IL(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM).RV(Diligent::TEX_FORMAT_G8R8_G8B8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_TYPELESS).IL(Diligent::TEX_FORMAT_BC1_TYPELESS).RV(Diligent::TEX_FORMAT_BC1_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_UNORM).IL(Diligent::TEX_FORMAT_BC1_UNORM).RV(Diligent::TEX_FORMAT_BC1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC1UNormalizedSRGB,           xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC1_TYPELESS).RT(Diligent::TEX_FORMAT_BC1_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC1_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC1_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_TYPELESS).IL(Diligent::TEX_FORMAT_BC2_TYPELESS).RV(Diligent::TEX_FORMAT_BC2_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_UNORM).IL(Diligent::TEX_FORMAT_BC2_UNORM).RV(Diligent::TEX_FORMAT_BC2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC2UNormalizedSRGB,           xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC2_TYPELESS).RT(Diligent::TEX_FORMAT_BC2_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC2_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC2_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_TYPELESS).IL(Diligent::TEX_FORMAT_BC3_TYPELESS).RV(Diligent::TEX_FORMAT_BC3_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_UNORM).IL(Diligent::TEX_FORMAT_BC3_UNORM).RV(Diligent::TEX_FORMAT_BC3_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC3UNormalizedSRGB,           xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC3_TYPELESS).RT(Diligent::TEX_FORMAT_BC3_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC3_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC3_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_TYPELESS).IL(Diligent::TEX_FORMAT_BC4_TYPELESS).RV(Diligent::TEX_FORMAT_BC4_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_UNORM).IL(Diligent::TEX_FORMAT_BC4_UNORM).RV(Diligent::TEX_FORMAT_BC4_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC4SNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC4_TYPELESS).RT(Diligent::TEX_FORMAT_BC4_SNORM).IL(Diligent::TEX_FORMAT_BC4_SNORM).RV(Diligent::TEX_FORMAT_BC4_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_TYPELESS).IL(Diligent::TEX_FORMAT_BC5_TYPELESS).RV(Diligent::TEX_FORMAT_BC5_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_UNORM).IL(Diligent::TEX_FORMAT_BC5_UNORM).RV(Diligent::TEX_FORMAT_BC5_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC5SNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC5_TYPELESS).RT(Diligent::TEX_FORMAT_BC5_SNORM).IL(Diligent::TEX_FORMAT_BC5_SNORM).RV(Diligent::TEX_FORMAT_BC5_SNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::B5G6R5UNormalized,            xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_B5G6R5_UNORM).RT(Diligent::TEX_FORMAT_B5G6R5_UNORM).IL(Diligent::TEX_FORMAT_B5G6R5_UNORM).RV(Diligent::TEX_FORMAT_B5G6R5_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::B5G5R5A1UNormalized,          xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).RT(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).IL(Diligent::TEX_FORMAT_B5G5R5A1_UNORM).RV(Diligent::TEX_FORMAT_B5G5R5A1_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8UNormalized,             xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM).IL(Diligent::TEX_FORMAT_BGRA8_UNORM).RV(Diligent::TEX_FORMAT_BGRA8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8UNormalized,             xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_UNORM).IL(Diligent::TEX_FORMAT_BGRX8_UNORM).RV(Diligent::TEX_FORMAT_BGRX8_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized, xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).RT(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).IL(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM).RV(Diligent::TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8Typeless,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_TYPELESS).IL(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RV(Diligent::TEX_FORMAT_BGRA8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRA8UNormalizedSRGB,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRA8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8Typeless,                xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_TYPELESS).IL(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RV(Diligent::TEX_FORMAT_BGRX8_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BGRX8UNormalizedSRGB,         xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BGRX8_TYPELESS).RT(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BGRX8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HTypeless,                 xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_TYPELESS).IL(Diligent::TEX_FORMAT_BC6H_TYPELESS).RV(Diligent::TEX_FORMAT_BC6H_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HUF16,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_UF16).IL(Diligent::TEX_FORMAT_BC6H_UF16).RV(Diligent::TEX_FORMAT_BC6H_UF16));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC6HSF16,                     xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC6H_TYPELESS).RT(Diligent::TEX_FORMAT_BC6H_SF16).IL(Diligent::TEX_FORMAT_BC6H_SF16).RV(Diligent::TEX_FORMAT_BC6H_SF16));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7Typeless,                  xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_TYPELESS).IL(Diligent::TEX_FORMAT_BC7_TYPELESS).RV(Diligent::TEX_FORMAT_BC7_TYPELESS));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7UNormalized,               xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_UNORM).IL(Diligent::TEX_FORMAT_BC7_UNORM).RV(Diligent::TEX_FORMAT_BC7_UNORM));
  m_FormatLookupTable.SetFormatInfo(xiiGALTextureFormat::BC7UNormalizedSRGB,           xiiGALFormatLookupEntryD3D12(Diligent::TEX_FORMAT_BC7_TYPELESS).RT(Diligent::TEX_FORMAT_BC7_UNORM_SRGB).IL(Diligent::TEX_FORMAT_BC7_UNORM_SRGB).RV(Diligent::TEX_FORMAT_BC7_UNORM_SRGB));

  // clang-format on
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_DeviceD3D12);
