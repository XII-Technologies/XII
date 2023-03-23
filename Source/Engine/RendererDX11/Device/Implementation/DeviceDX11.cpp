#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgidebug.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <d3d11_1.h>
#endif

xiiInternal::NewInstance<xiiGALDevice> CreateDX11Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  xiiGraphicsDevice::Default = xiiGraphicsDevice::D3D11;
  return XII_NEW(pAllocator, xiiGALDeviceDX11, Description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererDX11, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  xiiGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "D3D_SM50", "xiiShaderCompiler");
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterCreatorFunc("DX11");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceDX11::xiiGALDeviceDX11(const xiiGALDeviceCreationDescription& Description) :
  xiiGALDevice(Description), m_pDevice(nullptr), m_pDevice3(nullptr), m_pDebug(nullptr), m_pDXGIFactory(nullptr), m_pDXGIAdapter(nullptr), m_pDXGIDevice(nullptr), m_uiFeatureLevel(D3D_FEATURE_LEVEL_9_1), m_uiFrameCounter(0)
{
}

xiiGALDeviceDX11::~xiiGALDeviceDX11() = default;

// Init & shutdown functions

xiiResult xiiGALDeviceDX11::InitPlatform(DWORD dwFlags, IDXGIAdapter* pUsedAdapter)
{
  XII_LOG_BLOCK("xiiGALDeviceDX11::InitPlatform");

retry:

  if (m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
  else
    dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL    FeatureLevels[]   = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3};
  ID3D11DeviceContext* pImmediateContext = nullptr;

  D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
  // driverType = D3D_DRIVER_TYPE_REFERENCE; // enables the Reference Device

  if (pUsedAdapter != nullptr)
  {
    // required by the specification
    driverType = D3D_DRIVER_TYPE_UNKNOWN;
  }

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // initialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < XII_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_uiFeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      xiiLog::Warning("Couldn't initialize D3D11 debug device!");

      m_Description.m_bDebugDevice = false;
      goto retry;
    }

    xiiLog::Error("Couldn't initialize D3D11 device!");
    return XII_FAILURE;
  }
  else
  {
    m_pImmediateContext = pImmediateContext;

    const char* FeatureLevelNames[] = {"11.1", "11.0", "10.1", "10", "9.3"};

    XII_CHECK_AT_COMPILETIME(XII_ARRAY_SIZE(FeatureLevels) == XII_ARRAY_SIZE(FeatureLevelNames));

    xiiLog::Success("Initialized D3D11 device with feature level {0}.", FeatureLevelNames[FeatureLevelIdx]);
  }

  if (m_Description.m_bDebugDevice)
  {
    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_pDebug)))
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // only do this when a debugger is attached, otherwise the app would crash on every DX error
        if (IsDebuggerPresent())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        }

        // Ignore list.
        {
          D3D11_MESSAGE_ID hide[] = {
            // Hide messages about abandoned query results. This can easily happen when a GPUStopwatch is suddenly unused.
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS, D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            // Don't break on invalid input assembly. This can easily happen when using the wrong mesh-material combination.
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_MISSINGELEMENT,
            // Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter;
          xiiMemoryUtils::ZeroFill(&filter, 1);
          filter.DenyList.NumIDs  = _countof(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassDX11, *this);

  if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_pDXGIDevice)))
  {
    xiiLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 "
                  "installed!");
    return XII_FAILURE;
  }

  if (FAILED(m_pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_pDevice3)))
  {
    xiiLog::Info("D3D device doesn't support ID3D11Device3, some features might be unavailable.");
  }

  if (FAILED(m_pDXGIDevice->SetMaximumFrameLatency(1)))
  {
    xiiLog::Warning("Failed to set max frames latency");
  }

  if (FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_pDXGIAdapter)))
  {
    return XII_FAILURE;
  }

  if (FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_pDXGIFactory)))
  {
    return XII_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  // Per frame data & timer data
  D3D11_QUERY_DESC disjointQueryDesc;
  disjointQueryDesc.Query     = D3D11_QUERY_TIMESTAMP_DISJOINT;
  disjointQueryDesc.MiscFlags = 0;

  D3D11_QUERY_DESC timerQueryDesc;
  timerQueryDesc.Query     = D3D11_QUERY_TIMESTAMP;
  timerQueryDesc.MiscFlags = 0;

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.Query     = D3D11_QUERY_EVENT;
    QueryDesc.MiscFlags = 0;
    if (SUCCEEDED(GetDXDevice()->CreateQuery(&QueryDesc, &perFrameData.m_pFence)))

      if (FAILED(m_pDevice->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery)))
      {
        xiiLog::Error("Creation of native DirectX query for disjoint query has failed!");
        return XII_FAILURE;
      }
  }

  //#TODO_DX11 Replace ring buffer with proper pool like in Vulkan to prevent buffer overrun.
  m_Timestamps.SetCountUninitialized(2048);
  for (xiiUInt32 i = 0; i < m_Timestamps.GetCount(); ++i)
  {
    if (FAILED(m_pDevice->CreateQuery(&timerQueryDesc, &m_Timestamps[i])))
    {
      xiiLog::Error("Creation of native DirectX query for timestamp has failed!");
      return XII_FAILURE;
    }
  }

  m_SyncTimeDiff.SetZero();

  xiiGALWindowSwapChain::SetFactoryMethod([this](const xiiGALWindowSwapChainCreationDescription& desc) -> xiiGALSwapChainHandle { return CreateSwapChain([this, &desc](xiiAllocatorBase* pAllocator) -> xiiGALSwapChain* { return XII_NEW(pAllocator, xiiGALSwapChainDX11, desc); }); });

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceDX11::InitPlatform()
{
  return InitPlatform(0, nullptr);
}

void xiiGALDeviceDX11::ReportLiveGpuObjects()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Not implemented
  return;
#else

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  typedef HRESULT(WINAPI * FnGetDebugInterfacePtr)(REFIID, void**);
  FnGetDebugInterfacePtr GetDebugInterfacePtr = (FnGetDebugInterfacePtr)GetProcAddress(hDxgiDebugDLL, "DXGIGetDebugInterface");

  if (GetDebugInterfacePtr == nullptr)
    return;

  IDXGIDebug* dxgiDebug = nullptr;
  GetDebugInterfacePtr(IID_PPV_ARGS(&dxgiDebug));

  if (dxgiDebug == nullptr)
    return;

  OutputDebugStringW(L" +++++ Live DX11 Objects: +++++\n");

  // Prints to OutputDebugString
  dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

  OutputDebugStringW(L" ----- Live DX11 Objects: -----\n");

  dxgiDebug->Release();

#endif
}

void xiiGALDeviceDX11::FlushDeadObjects()
{
  DestroyDeadObjects();
}

xiiResult xiiGALDeviceDX11::ShutdownPlatform()
{
  xiiGALWindowSwapChain::SetFactoryMethod({});
  for (xiiUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      xiiDynamicArray<ID3D11Resource*>& resources = it.Value();
      for (auto pResource : resources)
      {
        XII_GAL_DX11_RELEASE(pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      XII_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  for (auto& timestamp : m_Timestamps)
  {
    XII_GAL_DX11_RELEASE(timestamp);
  }
  m_Timestamps.Clear();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    XII_GAL_DX11_RELEASE(perFrameData.m_pFence);
    perFrameData.m_pFence = nullptr;

    XII_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Force immediate destruction of all objects destroyed so far.
  // This is necessary if we want to create a new primary swap chain/device right after this.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
  // Strictly speaking we should do this right after we destroy the swap chain and flush all contexts that are affected.
  // However, the particular usecase where this problem comes up is usually a restart scenario.
  if (m_pImmediateContext != nullptr)
  {
    m_pImmediateContext->ClearState();
    m_pImmediateContext->Flush();
  }
#endif

  m_pDefaultPass = nullptr;

  XII_GAL_DX11_RELEASE(m_pImmediateContext);
  XII_GAL_DX11_RELEASE(m_pDevice3);
  XII_GAL_DX11_RELEASE(m_pDevice);
  XII_GAL_DX11_RELEASE(m_pDebug);
  XII_GAL_DX11_RELEASE(m_pDXGIFactory);
  XII_GAL_DX11_RELEASE(m_pDXGIAdapter);
  XII_GAL_DX11_RELEASE(m_pDXGIDevice);

  ReportLiveGpuObjects();

  return XII_SUCCESS;
}

// Pipeline & Pass functions

void xiiGALDeviceDX11::BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void xiiGALDeviceDX11::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
}

xiiGALPass* xiiGALDeviceDX11::BeginPassPlatform(const char* szName)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  m_pDefaultPass->BeginPass(szName);

  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceDX11::EndPassPlatform(xiiGALPass* pPass)
{
  XII_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif

  m_pDefaultPass->EndPass();
}

// State creation functions

xiiGALBlendState* xiiGALDeviceDX11::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description)
{
  xiiGALBlendStateDX11* pState = XII_NEW(&m_Allocator, xiiGALBlendStateDX11, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void xiiGALDeviceDX11::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  xiiGALBlendStateDX11* pState = static_cast<xiiGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pState);
}

xiiGALDepthStencilState* xiiGALDeviceDX11::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description)
{
  xiiGALDepthStencilStateDX11* pDX11DepthStencilState = XII_NEW(&m_Allocator, xiiGALDepthStencilStateDX11, Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDX11DepthStencilState);
    return nullptr;
  }
}

void xiiGALDeviceDX11::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDepthStencilStateDX11* pDX11DepthStencilState = static_cast<xiiGALDepthStencilStateDX11*>(pDepthStencilState);
  pDX11DepthStencilState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11DepthStencilState);
}

xiiGALRasterizerState* xiiGALDeviceDX11::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description)
{
  xiiGALRasterizerStateDX11* pDX11RasterizerState = XII_NEW(&m_Allocator, xiiGALRasterizerStateDX11, Description);

  if (pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDX11RasterizerState);
    return nullptr;
  }
}

void xiiGALDeviceDX11::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALRasterizerStateDX11* pDX11RasterizerState = static_cast<xiiGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11RasterizerState);
}

xiiGALSamplerState* xiiGALDeviceDX11::CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description)
{
  xiiGALSamplerStateDX11* pDX11SamplerState = XII_NEW(&m_Allocator, xiiGALSamplerStateDX11, Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    XII_DELETE(&m_Allocator, pDX11SamplerState);
    return nullptr;
  }
}

void xiiGALDeviceDX11::DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState)
{
  xiiGALSamplerStateDX11* pDX11SamplerState = static_cast<xiiGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11SamplerState);
}


// Resource creation functions

xiiGALShader* xiiGALDeviceDX11::CreateShaderPlatform(const xiiGALShaderCreationDescription& Description)
{
  xiiGALShaderDX11* pShader = XII_NEW(&m_Allocator, xiiGALShaderDX11, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void xiiGALDeviceDX11::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALShaderDX11* pDX11Shader = static_cast<xiiGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11Shader);
}

xiiGALBuffer* xiiGALDeviceDX11::CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  xiiGALBufferDX11* pBuffer = XII_NEW(&m_Allocator, xiiGALBufferDX11, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void xiiGALDeviceDX11::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALBufferDX11* pDX11Buffer = static_cast<xiiGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11Buffer);
}

xiiGALTexture* xiiGALDeviceDX11::CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  xiiGALTextureDX11* pTexture = XII_NEW(&m_Allocator, xiiGALTextureDX11, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    XII_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void xiiGALDeviceDX11::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALTextureDX11* pDX11Texture = static_cast<xiiGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11Texture);
}

xiiGALResourceView* xiiGALDeviceDX11::CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description)
{
  xiiGALResourceViewDX11* pResourceView = XII_NEW(&m_Allocator, xiiGALResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void xiiGALDeviceDX11::DestroyResourceViewPlatform(xiiGALResourceView* pResourceView)
{
  xiiGALResourceViewDX11* pDX11ResourceView = static_cast<xiiGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11ResourceView);
}

xiiGALRenderTargetView* xiiGALDeviceDX11::CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description)
{
  xiiGALRenderTargetViewDX11* pRTView = XII_NEW(&m_Allocator, xiiGALRenderTargetViewDX11, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void xiiGALDeviceDX11::DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView)
{
  xiiGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<xiiGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pDX11RenderTargetView);
}

xiiGALUnorderedAccessView* xiiGALDeviceDX11::CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pTextureOfBuffer, const xiiGALUnorderedAccessViewCreationDescription& Description)
{
  xiiGALUnorderedAccessViewDX11* pUnorderedAccessView = XII_NEW(&m_Allocator, xiiGALUnorderedAccessViewDX11, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void xiiGALDeviceDX11::DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<xiiGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}



// Other rendering creation functions

xiiGALQuery* xiiGALDeviceDX11::CreateQueryPlatform(const xiiGALQueryCreationDescription& Description)
{
  xiiGALQueryDX11* pQuery = XII_NEW(&m_Allocator, xiiGALQueryDX11, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    XII_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void xiiGALDeviceDX11::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALQueryDX11* pQueryDX11 = static_cast<xiiGALQueryDX11*>(pQuery);
  pQueryDX11->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pQueryDX11);
}

xiiGALVertexDeclaration* xiiGALDeviceDX11::CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description)
{
  xiiGALVertexDeclarationDX11* pVertexDeclaration = XII_NEW(&m_Allocator, xiiGALVertexDeclarationDX11, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    XII_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void xiiGALDeviceDX11::DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration)
{
  xiiGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<xiiGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this).IgnoreResult();
  XII_DELETE(&m_Allocator, pVertexDeclarationDX11);
}

xiiGALTimestampHandle xiiGALDeviceDX11::GetTimestampPlatform()
{
  xiiUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Timestamps.GetCount();
  return {uiIndex, m_uiFrameCounter};
}

xiiResult xiiGALDeviceDX11::GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result)
{
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  xiiUInt64 uiFrameCounter = hTimestamp.m_uiFrameCounter;

  PerFrameData* pPerFrameData = nullptr;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    if (m_PerFrameData[i].m_uiFrame == uiFrameCounter && m_PerFrameData[i].m_fInvTicksPerSecond >= 0.0)
    {
      pPerFrameData = &m_PerFrameData[i];
      break;
    }
  }

  if (pPerFrameData == nullptr)
  {
    return XII_FAILURE;
  }

  ID3D11Query* pQuery = GetTimestamp(hTimestamp);

  xiiUInt64 uiTimestamp;
  if (FAILED(m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), D3D11_ASYNC_GETDATA_DONOTFLUSH)))
  {
    return XII_FAILURE;
  }

  if (pPerFrameData->m_fInvTicksPerSecond == 0.0)
  {
    result.SetZero();
  }
  else
  {
    result = xiiTime::Seconds(double(uiTimestamp) * pPerFrameData->m_fInvTicksPerSecond) + m_SyncTimeDiff;
  }
  return XII_SUCCESS;
}

// Swap chain functions

void xiiGALDeviceDX11::PresentPlatform(const xiiGALSwapChain* pSwapChain, bool bVSync)
{
}

// Misc functions

void xiiGALDeviceDX11::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  xiiStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);

#if XII_ENABLED(XII_USE_PROFILING)
  m_pFrameTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif

  // check if fence is reached and wait if the disjoint timer is about to be re-used
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((xiiUInt64)-1))
    {

      bool bFenceReached = IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      if (!bFenceReached && m_uiNextPerFrameData == m_uiCurrentPerFrameData)
      {
        WaitForFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->Begin(perFrameData.m_pDisjointTimerQuery);

    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }
}

void xiiGALDeviceDX11::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  // end disjoint query
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->End(perFrameData.m_pDisjointTimerQuery);
  }

  // check if fence is reached and update per frame data
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((xiiUInt64)-1))
    {
      if (IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence))
      {
        FreeTempResources(perFrameData.m_uiFrame);

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
        if (FAILED(m_pImmediateContext->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH)) || data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0f;
        }
        else
        {
          perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

          if (m_bSyncTimeNeeded)
          {
            xiiGALTimestampHandle hTimestamp = m_pDefaultPass->m_pRenderCommandEncoder->InsertTimestamp();
            ID3D11Query*          pQuery     = GetTimestamp(hTimestamp);

            xiiUInt64 uiTimestamp;
            while (m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
            {
              xiiThreadUtils::YieldTimeSlice();
            }

            m_SyncTimeDiff    = xiiTime::Now() - xiiTime::Seconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
            m_bSyncTimeNeeded = false;
          }
        }

        m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % XII_ARRAY_SIZE(m_PerFrameData);
      }
    }
  }

  {
    auto& perFrameData     = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_uiFrame = m_uiFrameCounter;

    // insert fence
    InsertFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);

    m_uiNextPerFrameData = (m_uiNextPerFrameData + 1) % XII_ARRAY_SIZE(m_PerFrameData);
  }

  ++m_uiFrameCounter;
}

void xiiGALDeviceDX11::FillCapabilitiesPlatform()
{
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_pDXGIAdapter->GetDesc1(&adapterDesc);

    m_Capabilities.m_sAdapterName         = xiiStringUtf8(adapterDesc.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM      = static_cast<xiiUInt64>(adapterDesc.DedicatedVideoMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<xiiUInt64>(adapterDesc.DedicatedSystemMemory);
    m_Capabilities.m_uiSharedSystemRAM    = static_cast<xiiUInt64>(adapterDesc.SharedSystemMemory);
    m_Capabilities.m_bHardwareAccelerated = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  switch (m_uiFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bB5G6R5Textures          = true;
      m_Capabilities.m_bNoOverwriteBufferUpdate = true;

    case D3D_FEATURE_LEVEL_11_0:
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::VertexShader]   = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::HullShader]     = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::DomainShader]   = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::PixelShader]    = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::ComputeShader]  = true;
      m_Capabilities.m_bInstancing                                              = true;
      m_Capabilities.m_b32BitIndices                                            = true;
      m_Capabilities.m_bIndirectDraw                                            = true;
      m_Capabilities.m_bStreamOut                                               = true;
      m_Capabilities.m_uiMaxConstantBuffers                                     = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays                                           = true;
      m_Capabilities.m_bCubemapArrays                                           = true;
      m_Capabilities.m_uiMaxTextureDimension                                    = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension                                    = D3D11_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension                                  = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy                                          = D3D11_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets                                       = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount                                               = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 64 : 8);
      m_Capabilities.m_bAlphaToCoverage                                         = true;
      break;

    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_10_0:
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::VertexShader]   = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::HullShader]     = false;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::DomainShader]   = false;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::PixelShader]    = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::ComputeShader]  = false;
      m_Capabilities.m_bInstancing                                              = true;
      m_Capabilities.m_b32BitIndices                                            = true;
      m_Capabilities.m_bIndirectDraw                                            = false;
      m_Capabilities.m_bStreamOut                                               = true;
      m_Capabilities.m_uiMaxConstantBuffers                                     = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays                                           = true;
      m_Capabilities.m_bCubemapArrays                                           = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
      m_Capabilities.m_uiMaxTextureDimension                                    = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension                                    = D3D10_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension                                  = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy                                          = D3D10_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets                                       = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount                                               = 0;
      m_Capabilities.m_bAlphaToCoverage                                         = true;
      break;

    case D3D_FEATURE_LEVEL_9_3:
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::VertexShader]   = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::HullShader]     = false;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::DomainShader]   = false;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::GeometryShader] = false;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::PixelShader]    = true;
      m_Capabilities.m_bShaderStageSupported[xiiGALShaderStage::ComputeShader]  = false;
      m_Capabilities.m_bInstancing                                              = true;
      m_Capabilities.m_b32BitIndices                                            = true;
      m_Capabilities.m_bIndirectDraw                                            = false;
      m_Capabilities.m_bStreamOut                                               = false;
      m_Capabilities.m_uiMaxConstantBuffers                                     = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays                                           = false;
      m_Capabilities.m_bCubemapArrays                                           = false;
      m_Capabilities.m_uiMaxTextureDimension                                    = D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension                                    = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension                                  = 0;
      m_Capabilities.m_uiMaxAnisotropy                                          = 16;
      m_Capabilities.m_uiMaxRendertargets                                       = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount                                               = 0;
      m_Capabilities.m_bAlphaToCoverage                                         = false;
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOpts3;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOpts3, sizeof(featureOpts3))))
    {
      m_Capabilities.m_bVertexShaderRenderTargetArrayIndex = featureOpts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer != 0;
    }
  }
}

void xiiGALDeviceDX11::WaitIdlePlatform()
{
  m_pImmediateContext->Flush();
  DestroyDeadObjects();
}

ID3D11Resource* xiiGALDeviceDX11::FindTempBuffer(xiiUInt32 uiSize)
{
  const xiiUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = xiiMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = xiiMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = xiiMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  ID3D11Resource* pResource = nullptr;
  auto            it        = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    xiiDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = uiSize;
    desc.Usage               = D3D11_USAGE_STAGING;
    desc.BindFlags           = 0;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, &pBuffer)))
    {
      return nullptr;
    }

    pResource = pBuffer;
  }

  auto& tempResource       = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame   = m_uiFrameCounter;
  tempResource.m_uiHash    = uiSize;

  return pResource;
}


ID3D11Resource* xiiGALDeviceDX11::FindTempTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiGALResourceFormat::Enum format)
{
  xiiUInt32 data[] = {uiWidth, uiHeight, uiDepth, (xiiUInt32)format};
  xiiUInt32 uiHash = xiiHashingUtils::xxHash32(data, sizeof(data));

  ID3D11Resource* pResource = nullptr;
  auto            it        = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    xiiDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width              = uiWidth;
      desc.Height             = uiHeight;
      desc.MipLevels          = 1;
      desc.ArraySize          = 1;
      desc.Format             = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count   = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage              = D3D11_USAGE_STAGING;
      desc.BindFlags          = 0;
      desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags          = 0;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, nullptr, &pTexture)))
      {
        return nullptr;
      }

      pResource = pTexture;
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource       = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame   = m_uiFrameCounter;
  tempResource.m_uiHash    = uiHash;

  return pResource;
}

void xiiGALDeviceDX11::FreeTempResources(xiiUInt64 uiFrame)
{
  for (xiiUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, xiiDynamicArray<ID3D11Resource*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}

void xiiGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the xiiGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_FLOAT).VA(DXGI_FORMAT_R32G32B32A32_FLOAT).RV(DXGI_FORMAT_R32G32B32A32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_UINT).VA(DXGI_FORMAT_R32G32B32A32_UINT).RV(DXGI_FORMAT_R32G32B32A32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_SINT).VA(DXGI_FORMAT_R32G32B32A32_SINT).RV(DXGI_FORMAT_R32G32B32A32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT).RV(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBUInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_UINT).VA(DXGI_FORMAT_R32G32B32_UINT).RV(DXGI_FORMAT_R32G32B32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_SINT).VA(DXGI_FORMAT_R32G32B32_SINT).RV(DXGI_FORMAT_R32G32B32_SINT));

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::B5G6R5UNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).RT(DXGI_FORMAT_B5G6R5_UNORM).VA(DXGI_FORMAT_B5G6R5_UNORM).RV(DXGI_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM).VA(DXGI_FORMAT_B8G8R8A8_UNORM).RV(DXGI_FORMAT_B8G8R8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BGRAUByteNormalizedsRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAHalf, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UINT).VA(DXGI_FORMAT_R16G16B16A16_UINT).RV(DXGI_FORMAT_R16G16B16A16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UNORM).VA(DXGI_FORMAT_R16G16B16A16_UNORM).RV(DXGI_FORMAT_R16G16B16A16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SINT).VA(DXGI_FORMAT_R16G16B16A16_SINT).RV(DXGI_FORMAT_R16G16B16A16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SNORM).VA(DXGI_FORMAT_R16G16B16A16_SNORM).RV(DXGI_FORMAT_R16G16B16A16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT).RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_UINT).VA(DXGI_FORMAT_R32G32_UINT).RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_SINT).VA(DXGI_FORMAT_R32G32_SINT).RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UINT).VA(DXGI_FORMAT_R10G10B10A2_UINT).RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGB10A2UIntNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UNORM).VA(DXGI_FORMAT_R10G10B10A2_UNORM).RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RG11B10Float, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).RT(DXGI_FORMAT_R11G11B10_FLOAT).VA(DXGI_FORMAT_R11G11B10_FLOAT).RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).VA(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByteNormalizedsRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAUByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UINT).VA(DXGI_FORMAT_R8G8B8A8_UINT).RV(DXGI_FORMAT_R8G8B8A8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SNORM).VA(DXGI_FORMAT_R8G8B8A8_SNORM).RV(DXGI_FORMAT_R8G8B8A8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGBAByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SINT).VA(DXGI_FORMAT_R8G8B8A8_SINT).RV(DXGI_FORMAT_R8G8B8A8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGHalf, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_FLOAT).VA(DXGI_FORMAT_R16G16_FLOAT).RV(DXGI_FORMAT_R16G16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UINT).VA(DXGI_FORMAT_R16G16_UINT).RV(DXGI_FORMAT_R16G16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UNORM).VA(DXGI_FORMAT_R16G16_UNORM).RV(DXGI_FORMAT_R16G16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SINT).VA(DXGI_FORMAT_R16G16_SINT).RV(DXGI_FORMAT_R16G16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SNORM).VA(DXGI_FORMAT_R16G16_SNORM).RV(DXGI_FORMAT_R16G16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UINT).VA(DXGI_FORMAT_R8G8_UINT).RV(DXGI_FORMAT_R8G8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGUByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UNORM).VA(DXGI_FORMAT_R8G8_UNORM).RV(DXGI_FORMAT_R8G8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SINT).VA(DXGI_FORMAT_R8G8_SINT).RV(DXGI_FORMAT_R8G8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RGByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SNORM).VA(DXGI_FORMAT_R8G8_SNORM).RV(DXGI_FORMAT_R8G8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::DFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RV(DXGI_FORMAT_R32_FLOAT).D(DXGI_FORMAT_R32_FLOAT).DS(DXGI_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_UINT).VA(DXGI_FORMAT_R32_UINT).RV(DXGI_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RInt, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_SINT).VA(DXGI_FORMAT_R32_SINT).RV(DXGI_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RHalf, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_FLOAT).VA(DXGI_FORMAT_R16_FLOAT).RV(DXGI_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UINT).VA(DXGI_FORMAT_R16_UINT).RV(DXGI_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UNORM).VA(DXGI_FORMAT_R16_UNORM).RV(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShort, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SINT).VA(DXGI_FORMAT_R16_SINT).RV(DXGI_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RShortNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SNORM).VA(DXGI_FORMAT_R16_SNORM).RV(DXGI_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UINT).VA(DXGI_FORMAT_R8_UINT).RV(DXGI_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RUByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UNORM).VA(DXGI_FORMAT_R8_UNORM).RV(DXGI_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByte, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SINT).VA(DXGI_FORMAT_R8_SINT).RV(DXGI_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::RByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SNORM).VA(DXGI_FORMAT_R8_SNORM).RV(DXGI_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::AUByteNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_A8_UNORM).RT(DXGI_FORMAT_A8_UNORM).VA(DXGI_FORMAT_A8_UNORM).RV(DXGI_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D16, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RV(DXGI_FORMAT_R16_UNORM).DS(DXGI_FORMAT_D16_UNORM).D(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::D24S8, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC1, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC1sRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC2, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC2sRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC3, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC3sRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC4UNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC4Normalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC5UNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC5Normalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC6UFloat, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC6Float, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC7UNormalized, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(xiiGALResourceFormat::BC7UNormalizedsRGB, xiiGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM_SRGB));
}

xiiGALRenderCommandEncoder* xiiGALDeviceDX11::GetRenderCommandEncoder() const
{
  return m_pDefaultPass->m_pRenderCommandEncoder.Borrow();
}

void xiiGALDeviceDX11::InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  pContext->End(pFence);
}

bool xiiGALDeviceDX11::IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  if (pContext->GetData(pFence, &data, sizeof(data), 0) == S_OK)
  {
    XII_ASSERT_DEV(data == TRUE, "Implementation error");
    return true;
  }

  return false;
}

void xiiGALDeviceDX11::WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  while (pContext->GetData(pFence, &data, sizeof(data), 0) != S_OK)
  {
    xiiThreadUtils::YieldTimeSlice();
  }

  XII_ASSERT_DEV(data == TRUE, "Implementation error");
}

XII_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
