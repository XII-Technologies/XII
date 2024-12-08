#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/SwapChainD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
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
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/PipelineStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

#include <dxgi1_4.h>
#include <dxgidebug.h>
#include <sdkddkver.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDeviceD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceD3D12, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsD3D12, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Direct3D12, .m_sShaderModel = "D3D_SM60", .m_sShaderCompiler = "xiiShaderCompiler" };

  xiiGALDeviceFactory::RegisterImplementation("D3D12", &CreateD3D12Device, implementation);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterImplementation("D3D12");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

#define XII_VERIFY_D3D12(expression, ...)      \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return XII_FAILURE; } \
  } while (false)

xiiGALDeviceD3D12::xiiGALDeviceD3D12(const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(description)
{
}

xiiGALDeviceD3D12::~xiiGALDeviceD3D12() = default;

xiiResult xiiGALDeviceD3D12::InitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceD3D12::InitializePlatform");

  // Load Direct3D 12 dynamic library.
  // XII_SUCCEED_OR_RETURN_LOG(xiiPlugin::LoadPlugin("d3d12.dll"));

  // Enable the D3D12 debug layer.
  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
  {
    ID3D12Debug* pDebugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(pDebugController), reinterpret_cast<void**>(static_cast<ID3D12Debug**>(&pDebugController)))))
    {
      pDebugController->EnableDebugLayer();
      if (m_Description.m_ValidationLevel == xiiGALDeviceValidationLevel::All)
      {
        ID3D12Debug1* pDebugController1 = nullptr;
        if (SUCCEEDED(pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController1))))
        {
          pDebugController1->SetEnableGPUBasedValidation(TRUE);
          pDebugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
        }
        XII_GAL_D3D12_RELEASE(pDebugController1);
      }
    }
    XII_GAL_D3D12_RELEASE(pDebugController);
  }

  XII_VERIFY_D3D12(SUCCEEDED(CreateDXGIFactory1(__uuidof(m_pDXGIFactory), reinterpret_cast<void**>(static_cast<IDXGIFactory4**>(&m_pDXGIFactory)))), "Failed to create DXGI factory. Error code '{}'.", xiiArgErrorCode(GetLastError()));

  // Direct3D12 does not allow feature levels below 11.0 (D3D12CreateDevice fails to create a device).
  const D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

  IDXGIAdapter1* pHardwareAdapter = nullptr;
  if (m_Description.m_uiAdapterID == XII_GAL_DEFAULT_ADAPTER_ID)
  {
    /// \todo GraphicsD3D12: Select best adapter ID by default, based on memory size, number of command queues, and prefer Discrete over Integrated over Software adapters.
    GetHardwareAdapter(m_pDXGIFactory, &pHardwareAdapter, minFeatureLevel);
    XII_VERIFY_D3D12(pHardwareAdapter != nullptr, "No suitable hardware adapter found.");
  }
  else
  {
    xiiDynamicArray<IDXGIAdapter1*> compatibleAdapters = GetCompatibleAdapters(minFeatureLevel);

    XII_VERIFY_D3D12(m_Description.m_uiAdapterID < compatibleAdapters.GetCount(), "{0} is not a valid adapter ID. The total number of compatible adapters on this system is {1}.", m_Description.m_uiAdapterID, compatibleAdapters.GetCount());

    pHardwareAdapter = compatibleAdapters[m_Description.m_uiAdapterID];
    compatibleAdapters.RemoveAtAndSwap(m_Description.m_uiAdapterID);

    XII_GAL_D3D12_RELEASE_ARRAY(compatibleAdapters);
  }
  m_pDXGIAdapter = pHardwareAdapter;

  const D3D_FEATURE_LEVEL targetFeatureLevels[]     = {D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
  const char*             targetFeatureLevelNames[] = {"12.2", "12.1", "12.0", "11.1", "11.0"};
  xiiUInt32               uiFeatureLevelIndex       = 0U;
  HRESULT                 hResult                   = E_FAIL;

  ID3D12Device* pD3D12Device = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12Device));

  for (const auto& featureLevel : targetFeatureLevels)
  {
    hResult = D3D12CreateDevice(m_pDXGIAdapter, featureLevel, __uuidof(pD3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&pD3D12Device)));

    if (SUCCEEDED(hResult))
      break;

    ++uiFeatureLevelIndex;
  }

  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 hardware device. Attempting to create a WARP device.");

    // Try to create a WARP device (a high-performance software device that has the capabilities of a hardware device).
    XII_GAL_D3D12_RELEASE(m_pDXGIAdapter);

    IDXGIAdapter1* pWarpAdapter = nullptr;
    XII_VERIFY_D3D12(SUCCEEDED(m_pDXGIFactory->EnumWarpAdapter(__uuidof(pWarpAdapter), reinterpret_cast<void**>(static_cast<IDXGIAdapter1**>(&pWarpAdapter)))), "Failed to enumerate WARP adapter.");
    m_pDXGIAdapter = pWarpAdapter;

    uiFeatureLevelIndex = 0U;

    for (const auto& featureLevel : targetFeatureLevels)
    {
      hResult = D3D12CreateDevice(m_pDXGIAdapter, featureLevel, __uuidof(pD3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device**>(&pD3D12Device)));

      if (SUCCEEDED(hResult))
      {
        xiiLog::Info("Initialized D3D12 WARP device with feature level {0}.", targetFeatureLevelNames[uiFeatureLevelIndex]);
        break;
      }

      ++uiFeatureLevelIndex;
    }

    XII_VERIFY_D3D12(SUCCEEDED(hResult), "Failed to create D3D12 WARP device.");
  }
  else
  {
    xiiLog::Info("Initialized D3D12 device with feature level {0}.", targetFeatureLevelNames[uiFeatureLevelIndex]);
  }

  if (FAILED(pD3D12Device->QueryInterface(__uuidof(m_pD3D12Device), reinterpret_cast<void**>(static_cast<ID3D12Device1**>(&m_pD3D12Device)))))
  {
    xiiLog::Error("Failed to retrieve ID3D12Device1 from device interface.");
    return XII_FAILURE;
  }

  // Create D3D12 Memory Allocator.
  m_pAllocatorD3D12 = XII_NEW(&m_Allocator, xiiMemoryAllocatorD3D12, m_pDXGIAdapter, pD3D12Device);

  EnumerateDisplayModes(targetFeatureLevels[uiFeatureLevelIndex], m_pDXGIAdapter, 0, xiiGALTextureFormat::RGBA8UNormalizedSRGB, m_DisplayModes);

  if (m_Description.m_ValidationLevel != xiiGALDeviceValidationLevel::Disabled)
  {
    if (SUCCEEDED(m_pD3D12Device->QueryInterface(__uuidof(m_pD3D12Debug), reinterpret_cast<void**>(static_cast<ID3D12Debug1**>(&m_pD3D12Debug)))))
    {
      ID3D12InfoQueue* pD3D12InfoQueue = nullptr;
      XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pD3D12InfoQueue));

      if (SUCCEEDED(m_pD3D12Debug->QueryInterface(&pD3D12InfoQueue)))
      {
        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID denyIDs[] =
          {
            // D3D12 WARNING: ID3D12CommandList::ClearRenderTargetView: The clear values do not match those passed to resource creation.
            // The clear operation is typically slower as a result; but will still clear to the desired value.
            // [ EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,

            // D3D12 WARNING: ID3D12CommandList::ClearDepthStencilView: The clear values do not match those passed to resource creation.
            // The clear operation is typically slower as a result; but will still clear to the desired value.
            // [ EXECUTION WARNING #821: CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE]
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE //
          };

        D3D12_INFO_QUEUE_FILTER queueFilter = {};
        // queueFilter.DenyList.NumCategories = XII_ARRAY_SIZE(categories);
        // queueFilter.DenyList.pCategoryList = categories;
        queueFilter.DenyList.NumSeverities = XII_ARRAY_SIZE(severities);
        queueFilter.DenyList.pSeverityList = severities;
        queueFilter.DenyList.NumIDs        = XII_ARRAY_SIZE(denyIDs);
        queueFilter.DenyList.pIDList       = denyIDs;

        XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->PushStorageFilter(&queueFilter)), "Failed to push storage filter.");

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
        XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE)), "Failed to set break on corruption.");
        XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE)), "Failed to set break on error.");
        XII_VERIFY(SUCCEEDED(pD3D12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE)), "Failed to set break on warning.");
#endif
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
// We can prevent the GPU from overclocking or underclocking to get consistent timings.
// m_pD3D12Device->SetStablePowerState(TRUE);
#endif
  }

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceD3D12::PostInitializePlatform()
{
  xiiUInt32 queueCountPerContext[XII_GAL_MAX_ADAPTER_QUEUE_COUNT] = {};

  auto CreateCommandQueue = [&](xiiBitflags<xiiGALCommandQueueType> queueType, xiiStringView sName, xiiUInt32 uiAdapterId) {
    const auto& queues = m_AdapterDescription.m_CommandQueueProperties;

    for (xiiUInt32 i = 0, uiCount = queues.GetCount(); i < uiCount; ++i)
    {
      auto& currentQueue = queues[i];

      if (queueCountPerContext[i] >= currentQueue.m_uiMaxDeviceContexts)
        continue;

      if ((currentQueue.m_Type & queueType) == queueType)
      {
        queueCountPerContext[i] += 1;

        xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueType = queueType};

        xiiGALCommandQueueD3D12* pCommandQueueD3D12 = nullptr;
        if (queueType == xiiGALCommandQueueType::Graphics)
        {
          m_pGraphicsCommandQueue = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription);
          pCommandQueueD3D12      = m_pGraphicsCommandQueue.Borrow();
        }
        else if (queueType == xiiGALCommandQueueType::Compute)
        {
          m_pComputeCommandQueue = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription);
          pCommandQueueD3D12     = m_pComputeCommandQueue.Borrow();
        }
        else if (queueType == xiiGALCommandQueueType::Transfer)
        {
          m_pTransferCommandQueue = XII_NEW(&m_Allocator, xiiGALCommandQueueD3D12, this, queueDescription);
          pCommandQueueD3D12      = m_pTransferCommandQueue.Borrow();
        }

        if (pCommandQueueD3D12 != nullptr)
        {
          pCommandQueueD3D12->InitializePlatform();

          xiiStringBuilder sb;
          sb.SetFormat("Command Queue ({})", sName);
          pCommandQueueD3D12->SetDebugName(sb);

          xiiLog::Info("Created {}", sb);
        }

        return true;
      }
    }
    return false;
  };

  if (!CreateCommandQueue(xiiGALCommandQueueType::Graphics, "Default Graphics", m_Description.m_uiAdapterID))
    return XII_FAILURE;

  CreateCommandQueue(xiiGALCommandQueueType::Transfer, "Default Transfer", m_Description.m_uiAdapterID);
  CreateCommandQueue(xiiGALCommandQueueType::Compute, "Default Compute", m_Description.m_uiAdapterID);

  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::ReportLiveGPUObjects()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  IDXGIDebug1* dxgiDebug = nullptr;
  HRESULT      hResult   = DXGIGetDebugInterface1(0U, IID_PPV_ARGS(&dxgiDebug));
  if (SUCCEEDED(hResult))
  {
    OutputDebugStringW(L" +++++ Live D3D12 Objects: +++++\n");

    // Prints to OutputDebugString
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

    OutputDebugStringW(L" ----- Live D3D12 Objects: -----\n");

    dxgiDebug->Release();
  }
#endif
}

void xiiGALDeviceD3D12::FlushPendingObjects()
{
  FlushDestroyedObjects();
}

xiiResult xiiGALDeviceD3D12::ShutdownPlatform()
{
  if (m_pGraphicsCommandQueue != nullptr)
  {
    m_pGraphicsCommandQueue->DeInitializePlatform();
    m_pGraphicsCommandQueue.Clear();
  }

  if (m_pComputeCommandQueue != nullptr)
  {
    m_pComputeCommandQueue->DeInitializePlatform();
    m_pComputeCommandQueue.Clear();
  }

  if (m_pTransferCommandQueue != nullptr)
  {
    m_pTransferCommandQueue->DeInitializePlatform();
    m_pTransferCommandQueue.Clear();
  }

  XII_GAL_D3D12_RELEASE(m_pD3D12Debug);
  XII_GAL_D3D12_RELEASE(m_pD3D12Device);
  XII_GAL_D3D12_RELEASE(m_pDXGIAdapter);
  XII_GAL_D3D12_RELEASE(m_pDXGIFactory);

  ReportLiveGPUObjects();

  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::BeginFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains, const xiiUInt64 uiRenderFrame)
{
}

void xiiGALDeviceD3D12::EndFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains)
{

  ++m_uiFrameCounter;
}

xiiGALSwapChain* xiiGALDeviceD3D12::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiGALSwapChainD3D12* pSwapChainD3D12 = XII_NEW(&m_Allocator, xiiGALSwapChainD3D12, this, description);

  if (pSwapChainD3D12->InitPlatform().Succeeded())
    return pSwapChainD3D12;

  XII_DELETE(&m_Allocator, pSwapChainD3D12);

  return pSwapChainD3D12;
}

void xiiGALDeviceD3D12::DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain)
{
  xiiGALSwapChainD3D12* pSwapChainD3D12 = static_cast<xiiGALSwapChainD3D12*>(pSwapChain);

  pSwapChainD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSwapChainD3D12);
}

xiiGALBlendState* xiiGALDeviceD3D12::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiGALBlendStateD3D12* pBlendStateD3D12 = XII_NEW(&m_Allocator, xiiGALBlendStateD3D12, this, description);

  if (pBlendStateD3D12->InitPlatform().Succeeded())
    return pBlendStateD3D12;

  XII_DELETE(&m_Allocator, pBlendStateD3D12);

  return pBlendStateD3D12;
}

void xiiGALDeviceD3D12::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateD3D12* pBlendStateD3D12 = static_cast<xiiGALBlendStateD3D12*>(pBlendState);

  pBlendStateD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBlendStateD3D12);
}

xiiGALDepthStencilState* xiiGALDeviceD3D12::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiGALDepthStencilStateD3D12* pDepthStencilStateD3D12 = XII_NEW(&m_Allocator, xiiGALDepthStencilStateD3D12, this, description);

  if (pDepthStencilStateD3D12->InitPlatform().Succeeded())
    return pDepthStencilStateD3D12;

  XII_DELETE(&m_Allocator, pDepthStencilStateD3D12);

  return pDepthStencilStateD3D12;
}

void xiiGALDeviceD3D12::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateD3D12* pDepthStencilStateD3D12 = static_cast<xiiGALDepthStencilStateD3D12*>(pDepthStencilState);

  pDepthStencilStateD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pDepthStencilStateD3D12);
}

xiiGALRasterizerState* xiiGALDeviceD3D12::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiGALRasterizerStateD3D12* pRasterizerStateD3D12 = XII_NEW(&m_Allocator, xiiGALRasterizerStateD3D12, this, description);

  if (pRasterizerStateD3D12->InitPlatform().Succeeded())
    return pRasterizerStateD3D12;

  XII_DELETE(&m_Allocator, pRasterizerStateD3D12);

  return pRasterizerStateD3D12;
}

void xiiGALDeviceD3D12::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateD3D12* pRasterizerStateD3D12 = static_cast<xiiGALRasterizerStateD3D12*>(pRasterizerState);

  pRasterizerStateD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRasterizerStateD3D12);
}

xiiGALShader* xiiGALDeviceD3D12::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiGALShaderD3D12* pShaderD3D12 = XII_NEW(&m_Allocator, xiiGALShaderD3D12, this, description);

  if (pShaderD3D12->InitPlatform().Succeeded())
    return pShaderD3D12;

  XII_DELETE(&m_Allocator, pShaderD3D12);

  return pShaderD3D12;
}

void xiiGALDeviceD3D12::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderD3D12* pShaderD3D12 = static_cast<xiiGALShaderD3D12*>(pShader);

  pShaderD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pShaderD3D12);
}

xiiGALBuffer* xiiGALDeviceD3D12::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData)
{
  xiiGALBufferD3D12* pBufferD3D12 = XII_NEW(&m_Allocator, xiiGALBufferD3D12, this, description);

  if (pBufferD3D12->InitPlatform(pInitialData).Succeeded())
    return pBufferD3D12;

  XII_DELETE(&m_Allocator, pBufferD3D12);

  return pBufferD3D12;
}

void xiiGALDeviceD3D12::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferD3D12* pBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  pBufferD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferD3D12);
}

xiiGALBufferView* xiiGALDeviceD3D12::CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = XII_NEW(&m_Allocator, xiiGALBufferViewD3D12, this, pBuffer, description);

  if (pBufferViewD3D12->InitPlatform().Succeeded())
    return pBufferViewD3D12;

  XII_DELETE(&m_Allocator, pBufferViewD3D12);

  return pBufferViewD3D12;
}

void xiiGALDeviceD3D12::DestroyBufferViewPlatform(xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = static_cast<xiiGALBufferViewD3D12*>(pBufferView);

  pBufferViewD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBufferViewD3D12);
}

xiiGALTexture* xiiGALDeviceD3D12::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData)
{
  xiiGALTextureD3D12* pTextureD3D12 = XII_NEW(&m_Allocator, xiiGALTextureD3D12, this, description);

  if (pTextureD3D12->InitPlatform(pInitialData).Succeeded())
    return pTextureD3D12;

  XII_DELETE(&m_Allocator, pTextureD3D12);

  return pTextureD3D12;
}

void xiiGALDeviceD3D12::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureD3D12* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pTexture);

  pTextureD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureD3D12);
}

xiiGALTextureView* xiiGALDeviceD3D12::CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = XII_NEW(&m_Allocator, xiiGALTextureViewD3D12, this, pTexture, description);

  if (pTextureViewD3D12->InitPlatform().Succeeded())
    return pTextureViewD3D12;

  XII_DELETE(&m_Allocator, pTextureViewD3D12);

  return pTextureViewD3D12;
}

void xiiGALDeviceD3D12::DestroyTextureViewPlatform(xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pTextureView);

  pTextureViewD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTextureViewD3D12);
}

xiiGALSampler* xiiGALDeviceD3D12::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiGALSamplerD3D12* pSamplerD3D12 = XII_NEW(&m_Allocator, xiiGALSamplerD3D12, this, description);

  if (pSamplerD3D12->InitPlatform().Succeeded())
    return pSamplerD3D12;

  XII_DELETE(&m_Allocator, pSamplerD3D12);

  return pSamplerD3D12;
}

void xiiGALDeviceD3D12::DestroySamplerPlatform(xiiGALSampler* pSampler)
{
  xiiGALSamplerD3D12* pSamplerD3D12 = static_cast<xiiGALSamplerD3D12*>(pSampler);

  pSamplerD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pSamplerD3D12);
}

xiiGALInputLayout* xiiGALDeviceD3D12::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiGALInputLayoutD3D12* pInputLayoutD3D12 = XII_NEW(&m_Allocator, xiiGALInputLayoutD3D12, this, description);

  if (pInputLayoutD3D12->InitPlatform().Succeeded())
    return pInputLayoutD3D12;

  XII_DELETE(&m_Allocator, pInputLayoutD3D12);

  return pInputLayoutD3D12;
}

void xiiGALDeviceD3D12::DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  xiiGALInputLayoutD3D12* pInputLayoutD3D12 = static_cast<xiiGALInputLayoutD3D12*>(pInputLayout);

  pInputLayoutD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pInputLayoutD3D12);
}

xiiGALQuery* xiiGALDeviceD3D12::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiGALQueryD3D12* pQueryD3D12 = XII_NEW(&m_Allocator, xiiGALQueryD3D12, this, description);

  if (pQueryD3D12->InitPlatform().Succeeded())
    return pQueryD3D12;

  XII_DELETE(&m_Allocator, pQueryD3D12);

  return pQueryD3D12;
}

void xiiGALDeviceD3D12::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryD3D12* pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  pQueryD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pQueryD3D12);
}

xiiGALFence* xiiGALDeviceD3D12::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiGALFenceD3D12* pFenceD3D12 = XII_NEW(&m_Allocator, xiiGALFenceD3D12, this, description);

  if (pFenceD3D12->InitPlatform().Succeeded())
    return pFenceD3D12;

  XII_DELETE(&m_Allocator, pFenceD3D12);

  return pFenceD3D12;
}

void xiiGALDeviceD3D12::DestroyFencePlatform(xiiGALFence* pFence)
{
  xiiGALFenceD3D12* pFenceD3D12 = static_cast<xiiGALFenceD3D12*>(pFence);

  pFenceD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFenceD3D12);
}

xiiGALRenderPass* xiiGALDeviceD3D12::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassD3D12* pRenderPassD3D12 = XII_NEW(&m_Allocator, xiiGALRenderPassD3D12, this, description);

  if (pRenderPassD3D12->InitPlatform().Succeeded())
    return pRenderPassD3D12;

  XII_DELETE(&m_Allocator, pRenderPassD3D12);

  return pRenderPassD3D12;
}

void xiiGALDeviceD3D12::DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass)
{
  xiiGALRenderPassD3D12* pRenderPassD3D12 = static_cast<xiiGALRenderPassD3D12*>(pRenderPass);

  pRenderPassD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pRenderPassD3D12);
}

xiiGALFramebuffer* xiiGALDeviceD3D12::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiGALFramebufferD3D12* pFramebufferD3D12 = XII_NEW(&m_Allocator, xiiGALFramebufferD3D12, this, description);

  if (pFramebufferD3D12->InitPlatform().Succeeded())
    return pFramebufferD3D12;

  XII_DELETE(&m_Allocator, pFramebufferD3D12);

  return pFramebufferD3D12;
}

void xiiGALDeviceD3D12::DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer)
{
  xiiGALFramebufferD3D12* pFramebufferD3D12 = static_cast<xiiGALFramebufferD3D12*>(pFramebuffer);

  pFramebufferD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pFramebufferD3D12);
}

xiiGALBottomLevelAS* xiiGALDeviceD3D12::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALBottomLevelASD3D12, this, description);

  if (pBottomLevelASD3D12->InitPlatform().Succeeded())
    return pBottomLevelASD3D12;

  XII_DELETE(&m_Allocator, pBottomLevelASD3D12);

  return pBottomLevelASD3D12;
}

void xiiGALDeviceD3D12::DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS)
{
  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12 = static_cast<xiiGALBottomLevelASD3D12*>(pBottomLevelAS);

  pBottomLevelASD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pBottomLevelASD3D12);
}

xiiGALTopLevelAS* xiiGALDeviceD3D12::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiGALTopLevelASD3D12* pTopLevelASD3D12 = XII_NEW(&m_Allocator, xiiGALTopLevelASD3D12, this, description);

  if (pTopLevelASD3D12->InitPlatform().Succeeded())
    return pTopLevelASD3D12;

  XII_DELETE(&m_Allocator, pTopLevelASD3D12);

  return pTopLevelASD3D12;
}

void xiiGALDeviceD3D12::DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASD3D12* pTopLevelASD3D12 = static_cast<xiiGALTopLevelASD3D12*>(pTopLevelAS);

  pTopLevelASD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pTopLevelASD3D12);
}

xiiGALPipelineResourceSignature* xiiGALDeviceD3D12::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiGALPipelineResourceSignatureD3D12* pPipelineResourceSignatureD3D12 = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureD3D12, this, description);

  if (pPipelineResourceSignatureD3D12->InitPlatform().Succeeded())
    return pPipelineResourceSignatureD3D12;

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureD3D12);

  return pPipelineResourceSignatureD3D12;
}

void xiiGALDeviceD3D12::DestroyPipelineResourceSignaturePlatform(xiiGALPipelineResourceSignature* pPipelineResourceSignature)
{
  xiiGALPipelineResourceSignatureD3D12* pPipelineResourceSignatureD3D12 = static_cast<xiiGALPipelineResourceSignatureD3D12*>(pPipelineResourceSignature);

  pPipelineResourceSignatureD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineResourceSignatureD3D12);
}

xiiGALPipelineState* xiiGALDeviceD3D12::CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description)
{
  xiiGALPipelineStateD3D12* pPipelineStateD3D12 = XII_NEW(&m_Allocator, xiiGALPipelineStateD3D12, this, description);

  if (pPipelineStateD3D12->InitPlatform().Succeeded())
    return pPipelineStateD3D12;

  XII_DELETE(&m_Allocator, pPipelineStateD3D12);

  return pPipelineStateD3D12;
}

void xiiGALDeviceD3D12::DestroyPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  xiiGALPipelineStateD3D12* pPipelineStateD3D12 = static_cast<xiiGALPipelineStateD3D12*>(pPipelineState);

  pPipelineStateD3D12->DeInitPlatform().IgnoreResult();

  XII_DELETE(&m_Allocator, pPipelineStateD3D12);
}

void xiiGALDeviceD3D12::WaitIdlePlatform()
{
  ///\todo Idle all command queues.

  FlushPendingObjects();

  ///\todo Release stale resources.
}

xiiResult xiiGALDeviceD3D12::FillCapabilitiesPlatform()
{
  m_Description.m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Direct3D12;

  /// \todo GraphicsD3D12: Assert that structure sizes has not been modified.

  // Set graphics adapter properties.
  {
    DXGI_ADAPTER_DESC1 dxgiAdapterDescription = {};
    m_pDXGIAdapter->GetDesc1(&dxgiAdapterDescription);
    m_AdapterDescription.m_sAdapterName = xiiStringUtf8(dxgiAdapterDescription.Description).GetData();

    if (dxgiAdapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Software;
    else if (dxgiAdapterDescription.DedicatedVideoMemory != 0U)
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Discrete;
    else
      m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;

    {
      D3D12_FEATURE_DATA_ARCHITECTURE dataArchitecture = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &dataArchitecture, sizeof(dataArchitecture))))
      {
        if (m_AdapterDescription.m_Type != xiiGALDeviceAdapterType::Software && (dataArchitecture.UMA || dataArchitecture.CacheCoherentUMA))
          m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;
      }
    }

    m_AdapterDescription.m_Vendor             = xiiGALDeviceUtilities::GetVendorFromID(dxgiAdapterDescription.VendorId);
    m_AdapterDescription.m_uiVendorID         = dxgiAdapterDescription.VendorId;
    m_AdapterDescription.m_uiDeviceID         = dxgiAdapterDescription.DeviceId;
    m_AdapterDescription.m_uiVideoOutputCount = 0U;

    // Enable features.
    m_AdapterDescription.m_Features.m_SeparablePrograms             = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_ShaderResourceQueries         = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_WireframeFill                 = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_MultithreadedResourceCreation = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_GeometryShaders               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_Tessellation                  = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_OcclusionQueries              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_BinaryOcclusionQueries        = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TimestampQueries              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_PipelineStatisticsQueries     = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DurationQueries               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DepthBiasClamp                = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DepthClamp                    = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_IndependentBlend              = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_DualSourceBlend               = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_MultiViewport                 = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TextureCompressionBC          = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_PixelUAVWritesAndAtomics      = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TextureUAVExtendedFormats     = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_InstanceDataStepRate          = xiiGALDeviceFeatureState::Enabled;
    m_AdapterDescription.m_Features.m_TileShaders                   = xiiGALDeviceFeatureState::Disabled;
    m_AdapterDescription.m_Features.m_SubpassFramebufferFetch       = xiiGALDeviceFeatureState::Disabled;
    m_AdapterDescription.m_Features.m_TextureComponentSwizzle       = xiiGALDeviceFeatureState::Disabled;

    // Set memory properties.
    m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory         = dxgiAdapterDescription.DedicatedVideoMemory;
    m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory   = dxgiAdapterDescription.SharedSystemMemory;
    m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory       = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation = 0U; // Unable to query.

    // Set draw command properties.
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = 0U;
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = ~0U;
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::DrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance;

    // Set queue information.
    xiiGALCommandQueueType::Enum queueIndexType[] = {xiiGALCommandQueueType::Graphics, xiiGALCommandQueueType::Compute, xiiGALCommandQueueType::Transfer};
    m_AdapterDescription.m_CommandQueueProperties.SetCount(XII_ARRAY_SIZE(queueIndexType));

    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto& queueProperty                 = m_AdapterDescription.m_CommandQueueProperties.ExpandAndGetRef();
      queueProperty.m_Type                = queueIndexType[i];
      queueProperty.m_uiMaxDeviceContexts = 0xFFU;

      queueProperty.m_TextureCopyGranularity[0] = 1U;
      queueProperty.m_TextureCopyGranularity[1] = 1U;
      queueProperty.m_TextureCopyGranularity[2] = 1U;
    }
  }

  // Enable features and set properties.
  {
    auto& deviceFeatures = m_AdapterDescription.m_Features;

    // Direct3D12 supports shader model 5.1 on all feature levels (even on 11.0), so bindless resources are always available.
    // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
    deviceFeatures.m_BindlessResources = xiiGALDeviceFeatureState::Enabled;

    deviceFeatures.m_VertexPipelineUAVWritesAndAtomics = xiiGALDeviceFeatureState::Enabled;
    deviceFeatures.m_NativeFence                       = xiiGALDeviceFeatureState::Optional; // This can be disabled.
    deviceFeatures.m_TextureComponentSwizzle           = xiiGALDeviceFeatureState::Enabled;

    // Check if mesh shader is supported.
    bool bMeshShadersSupported = false;
#ifdef D3D12_H_HAS_MESH_SHADER
    {
      D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {static_cast<D3D_SHADER_MODEL>(0x65)};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
      {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
        bMeshShadersSupported                         = SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData))) && featureData.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
      }
    }
#endif

    if (bMeshShadersSupported)
    {
      deviceFeatures.m_MeshShaders = xiiGALDeviceFeatureState::Enabled;

      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = 65536U; // From specification: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#dispatchmesh-api
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = 65536U;
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = 65536U;
      m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = XII_BIT(22U);
    }

    deviceFeatures.m_ShaderResourceRuntimeArray = xiiGALDeviceFeatureState::Enabled;

    {
      D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(featureDataOptions))))
      {
        if (featureDataOptions.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
        {
          deviceFeatures.m_ShaderFloat16 = xiiGALDeviceFeatureState::Enabled;
        }

        if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_1)
        {
          deviceFeatures.m_SparseResources = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_SparseResourceProperties.m_uiStandardBlockSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

          D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT featureDataGPUVirtualAddress = {};
          if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &featureDataGPUVirtualAddress, sizeof(featureDataGPUVirtualAddress))))
          {
            m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = XII_BIT(featureDataGPUVirtualAddress.MaxGPUVirtualAddressBitsPerProcess);
            m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = XII_BIT(featureDataGPUVirtualAddress.MaxGPUVirtualAddressBitsPerResource);
          }
          else
          {
            m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = XII_BIT(featureDataOptions.MaxGPUVirtualAddressBitsPerResource);
            m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = XII_BIT(featureDataOptions.MaxGPUVirtualAddressBitsPerResource);
          }

          m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags = xiiGALSparseResourceCapabilityFlags::Buffer | xiiGALSparseResourceCapabilityFlags::BufferStandardBlock | xiiGALSparseResourceCapabilityFlags::Texture2D |
            xiiGALSparseResourceCapabilityFlags::Standard2DTileShape | xiiGALSparseResourceCapabilityFlags::Aliased | xiiGALSparseResourceCapabilityFlags::NonResidentSafe;

          // No 2, 8 or 16 sample multisample antialiasing (MSAA) support. Only 4x is required, except no 128 bpp formats.
          m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture4Samples | xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape;

          if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency | xiiGALSparseResourceCapabilityFlags::NonResidentStrict;
          }
          if (featureDataOptions.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_3)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture3D | xiiGALSparseResourceCapabilityFlags::Standard3DTileShape;
          }
#if 0 // We currently do not use NVAPI on Nvidia graphics cards.
          if (pNVAPI)
          {
              m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail;
          }
#endif
          if (featureDataOptions.ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2)
          {
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags |= xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport;
          }

          // Some features are not correctly working in software renderer.
          if (m_AdapterDescription.m_Type == xiiGALDeviceAdapterType::Software)
          {
            // Reading from null-mapped tile does not return zero.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::NonResidentStrict);
            // CheckAccessFullyMapped() in shader does not work.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency);
            // Mip tails are not supported at all.
            m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags.Remove(xiiGALSparseResourceCapabilityFlags::AlignedMipSize);
          }

          m_AdapterDescription.m_SparseResourceProperties.m_BindFlags = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer |
            xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;

          for (xiiUInt32 i = 0; i < m_AdapterDescription.m_CommandQueueProperties.GetCount(); ++i)
            m_AdapterDescription.m_CommandQueueProperties[i].m_Type |= xiiGALCommandQueueType::SparseBinding;
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS1 featureDataOptions1 = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &featureDataOptions1, sizeof(featureDataOptions1))))
      {
        if (featureDataOptions1.WaveOps != FALSE)
        {
          deviceFeatures.m_WaveOperation = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize             = featureDataOptions1.WaveLaneCountMin;
          m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize             = featureDataOptions1.WaveLaneCountMax;
          m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages = xiiGALShaderType::Pixel | xiiGALShaderType::Compute;
          m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures          = xiiGALWaveFeature::Basic | xiiGALWaveFeature::Vote | xiiGALWaveFeature::Arithmetic | xiiGALWaveFeature::Ballot | xiiGALWaveFeature::Quad;
          if (bMeshShadersSupported)
            m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages |= xiiGALShaderType::Amplification | xiiGALShaderType::Mesh;
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS3 featureDataOptions3 = {};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &featureDataOptions3, sizeof(featureDataOptions3))))
      {
        if (featureDataOptions3.CopyQueueTimestampQueriesSupported)
          deviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Enabled;
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS4 featureDataOptions4{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &featureDataOptions4, sizeof(featureDataOptions4))))
      {
        if (featureDataOptions4.Native16BitShaderOpsSupported)
        {
          deviceFeatures.m_ResourceBuffer16BitAccess = xiiGALDeviceFeatureState::Enabled;
          deviceFeatures.m_UniformBuffer16BitAccess  = xiiGALDeviceFeatureState::Enabled;
          deviceFeatures.m_ShaderInputOutput16       = xiiGALDeviceFeatureState::Enabled;
        }
      }

      D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureDataOptions5{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureDataOptions5, sizeof(featureDataOptions5))))
      {
        if (featureDataOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
        {
          deviceFeatures.m_RayTracing = xiiGALDeviceFeatureState::Enabled;

          m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
          m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
          m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = D3D12_RAYTRACING_MAX_RAY_GENERATION_SHADER_THREADS;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = D3D12_RAYTRACING_MAX_INSTANCES_PER_TOP_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
          m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = 1U;
          m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = 1U;
          m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT;
          m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::StandaloneShaders;
        }
        if (featureDataOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
        {
          m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::InlineRayTracing | xiiGALRayTracingCapabilityFlags::IndirectRayTracing;
        }
      }

#if defined(NTDDI_WIN10_19H1) || defined(FORCE_NTDDI_WIN10_19H1)
      D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureDataOptions6{};
      if (SUCCEEDED(m_pD3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureDataOptions6, sizeof(featureDataOptions6))))
      {
        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
        auto& shadingRateProperties = m_AdapterDescription.m_ShadingRateProperties;
        auto  AddShadingRate        = [&shadingRateProperties](xiiBitflags<xiiGALShadingRateFlags> shadingRate, xiiBitflags<xiiGALSampleCount> sampleBits) -> void {
          XII_ASSERT_DEV(shadingRateProperties.m_Modes.GetCount() < XII_GAL_MAX_SHADING_RATE, "Shading rate properties exeeds the GAL maximum shaing rate count.");

          auto& mode         = shadingRateProperties.m_Modes.ExpandAndGetRef();
          mode.m_ShadingRate = shadingRate;
          mode.m_SampleBits  = sampleBits;
        };

        if (featureDataOptions6.AdditionalShadingRatesSupported != FALSE)
        {
          AddShadingRate(xiiGALShadingRateFlags::_4X4, xiiGALSampleCount::OneSample);
          AddShadingRate(xiiGALShadingRateFlags::_4X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples);
          AddShadingRate(xiiGALShadingRateFlags::_2X4, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples);
        }
        if (featureDataOptions6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
        {
          deviceFeatures.m_VariableRateShading = xiiGALDeviceFeatureState::Enabled;

          shadingRateProperties.m_Format = xiiGALShadingRateFormat::Palette;
          shadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::PassThrough;
          shadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::PerDraw;

          // 1x1, 1x2, 2x1, 2x2 are always supported
          AddShadingRate(xiiGALShadingRateFlags::_2X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_2X1, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_1X2, xiiGALSampleCount::OneSample | xiiGALSampleCount::TwoSamples | xiiGALSampleCount::FourSamples);
          AddShadingRate(xiiGALShadingRateFlags::_1X1, xiiGALSampleCount::AllSamples);
        }
        if (featureDataOptions6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
        {
          shadingRateProperties.m_CapabilityFlags = xiiGALShadingRateCapabilityFlags::PerPrimitive | xiiGALShadingRateCapabilityFlags::TextureBased | xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget | xiiGALShadingRateCapabilityFlags::SampleMask |
            xiiGALShadingRateCapabilityFlags::ShaderSampleMask | xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput;

          shadingRateProperties.m_MinTileSize = xiiSizeU32(featureDataOptions6.ShadingRateImageTileSize, featureDataOptions6.ShadingRateImageTileSize);
          shadingRateProperties.m_MaxTileSize = xiiSizeU32(featureDataOptions6.ShadingRateImageTileSize, featureDataOptions6.ShadingRateImageTileSize);
          shadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::CombinerOverride | xiiGALShadingRateCombinerFlags::CombinerMin | xiiGALShadingRateCombinerFlags::CombinerMax | xiiGALShadingRateCombinerFlags::CombinerSum;
          shadingRateProperties.m_BindFlags |= xiiGALBindFlags::ShadingRate | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShadingRate;
          shadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnGPU;
        }
        if (featureDataOptions6.PerPrimitiveShadingRateSupportedWithViewportIndexing != FALSE)
        {
          shadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports;
        }
        // Export of depth and stencil is not supported
        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#export-of-depth-and-stencil

        // Perhaps add support for D3D12_FEATURE_DATA_D3D12_OPTIONS10?
      }
#endif // NTDDI_WIN10_19H1
    }

    // Buffer properties.
    {
      m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
      m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
    }
  }

  // Texture properties.
  {
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = D3D12_REQ_TEXTURECUBE_DIMENSION;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = true;
  }

  // Sampler properties.
  {
    m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported   = true;
    m_AdapterDescription.m_SamplerProperties.m_uiMaxAnisotropy                = D3D12_DEFAULT_MAX_ANISOTROPY;
    m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported              = true;
  }

  // Compute shader properties.
  {
    m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = 32U << 10U;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = D3D12_CS_THREAD_GROUP_MAX_X;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = D3D12_CS_THREAD_GROUP_MAX_Y;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = D3D12_CS_THREAD_GROUP_MAX_Z;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
  }

  // Draw command properties.
  {
#if D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP >= 32
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue = ~0U;
#else
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue = 1U << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP;
#endif
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::BaseVertex | xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect | xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;
  }

  return XII_SUCCESS;
}

void xiiGALDeviceD3D12::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel)
{
  IDXGIAdapter1* pDXGIAdapter = nullptr;
  *ppAdapter                  = nullptr;

  for (xiiUInt32 uiAdapterIndex = 0; pFactory->EnumAdapters1(uiAdapterIndex, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++uiAdapterIndex)
  {
    DXGI_ADAPTER_DESC1 adapterDescription;
    pDXGIAdapter->GetDesc1(&adapterDescription);

    if (adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
    {
      // Skip software adapters.
      XII_GAL_D3D12_RELEASE(pDXGIAdapter);

      continue;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, featureLevel, __uuidof(ID3D12Device), nullptr)))
    {
      break;
    }
    else
    {
      XII_GAL_D3D12_RELEASE(pDXGIAdapter);
    }
  }

  *ppAdapter = pDXGIAdapter;
}

xiiDynamicArray<IDXGIAdapter1*> xiiGALDeviceD3D12::GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel)
{
  xiiDynamicArray<IDXGIAdapter1*> DXGIAdapters;

  IDXGIFactory2* pDXGIFactory = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&pDXGIFactory)))
  {
    xiiLog::Error("Failed to create DXGI factory.");
    return DXGIAdapters;
  }

  IDXGIAdapter1* pDXGIAdapter = nullptr;
  for (xiiUInt32 uiAdapterIndex = 0; pDXGIFactory->EnumAdapters1(uiAdapterIndex, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++uiAdapterIndex)
  {
    DXGI_ADAPTER_DESC1 adapterDescription;
    pDXGIAdapter->GetDesc1(&adapterDescription);

    if (SUCCEEDED(D3D12CreateDevice(pDXGIAdapter, minFeatureLevel, __uuidof(ID3D12Device), nullptr)))
    {
      DXGIAdapters.PushBack(pDXGIAdapter);
    }
    else
    {
      XII_GAL_D3D12_RELEASE(pDXGIAdapter);
    }
  }

  return DXGIAdapters;
}

void xiiGALDeviceD3D12::EnumerateDisplayModes(D3D_FEATURE_LEVEL featureLevel, IDXGIAdapter1* pDXGIAdapter, xiiUInt32 uiOutputID, xiiEnum<xiiGALTextureFormat> format, xiiDynamicArray<xiiGALDisplayModeDescription>& displayModes)
{
  auto DXGIAdapters = GetCompatibleAdapters(featureLevel);

  DXGI_FORMAT  dxgiFormat = xiiD3D12TypeConversions::GetFormat(format);
  IDXGIOutput* pOutput    = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D12_RELEASE(pOutput));

  if (pDXGIAdapter->EnumOutputs(uiOutputID, &pOutput) == DXGI_ERROR_NOT_FOUND)
  {
    DXGI_ADAPTER_DESC1 adapterDescription;
    pDXGIAdapter->GetDesc1(&adapterDescription);

    xiiLog::Error("Failed to enumerate output {0} of adapter {1} ({2}).", uiOutputID, adapterDescription.DeviceId, xiiStringUtf8(adapterDescription.Description).GetData());
    return;
  }

  // Retrieve the display mode count.
  xiiUInt32 uiModeCount = 0;
  if (SUCCEEDED(pOutput->GetDisplayModeList(dxgiFormat, 0U, &uiModeCount, NULL)))
  {
    // Retireve the display mode descriptions.
    xiiDynamicArray<DXGI_MODE_DESC> dxgiDisplayModes;
    dxgiDisplayModes.SetCount(uiModeCount);

    if (SUCCEEDED(pOutput->GetDisplayModeList(dxgiFormat, 0U, &uiModeCount, dxgiDisplayModes.GetData())))
    {
      displayModes.Clear();
      for (xiiUInt32 i = 0; i < uiModeCount; ++i)
      {
        const auto& dxgiDisplayMode = dxgiDisplayModes[i];
        auto&       galDisplayMode  = displayModes.ExpandAndGetRef();

        galDisplayMode.m_Resolution               = xiiSizeU32(dxgiDisplayMode.Width, dxgiDisplayMode.Height);
        galDisplayMode.m_TextureFormat            = xiiD3D12TypeConversions::GetGALFormat(dxgiDisplayMode.Format);
        galDisplayMode.m_uiRefreshRateNumerator   = dxgiDisplayMode.RefreshRate.Numerator;
        galDisplayMode.m_uiRefreshRateDenominator = dxgiDisplayMode.RefreshRate.Denominator;
        galDisplayMode.m_ScalingMode              = xiiD3D12TypeConversions::GetGALScalingMode(dxgiDisplayMode.Scaling);
        galDisplayMode.m_ScanLineOrder            = xiiD3D12TypeConversions::GetGALScanLineOrder(dxgiDisplayMode.ScanlineOrdering);
      }
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_DeviceD3D12);
