#include <RendererDiligent/RendererDiligentPCH.h>

#include <Core/System/Window.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/SwapChainDiligent.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#if D3D11_SUPPORTED
#  include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#endif

#if D3D12_SUPPORTED
#  include <Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
#  include <Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#endif

#if VULKAN_SUPPORTED
#  include <Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#endif

#if METAL_SUPPORTED
#  include <Graphics/GraphicsEngineMetal/interface/EngineFactoryMtl.h>
#endif


void xiiGALSwapChainDiligent::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  m_hBackbufferTexture.Invalidate();
  m_hBackbufferTextureView.Invalidate();

  if (CreateBackBufferInternal(pDeviceDiligent) != XII_SUCCESS)
  {
    xiiLog::Error("Failed to get internal backbuffer textures");
  }

  Diligent::ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
  Diligent::ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
  pDeviceDiligent->GetImmediateContext()->SetRenderTargets(1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  const float Zero[4] = {0, 0, 0, 0};
  pDeviceDiligent->GetImmediateContext()->ClearRenderTarget(pRTV, &Zero[0], Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  pDeviceDiligent->GetImmediateContext()->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALSwapChainDiligent::PresentRenderTarget(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  m_pSwapChain->Present(m_CurrentPresentMode == xiiGALPresentMode::VSync ? 1 : 0);
}

xiiResult xiiGALSwapChainDiligent::UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  m_CurrentPresentMode = newPresentMode;

  m_pSwapChain->Resize(m_WindowDesc.m_pWindow->GetClientAreaSize().width, m_WindowDesc.m_pWindow->GetClientAreaSize().height);

  m_hBackbufferTexture.Invalidate();
  m_hBackbufferTextureView.Invalidate();

  return CreateBackBufferInternal(pDeviceDiligent);
}

xiiGALSwapChainDiligent::xiiGALSwapChainDiligent(const xiiGALWindowSwapChainCreationDescription& Description) :
  xiiGALWindowSwapChain(Description)
{
}

xiiGALSwapChainDiligent::~xiiGALSwapChainDiligent() {}


xiiResult xiiGALSwapChainDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  Diligent::Win32NativeWindow Window{xiiMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle())};
#else
#  error Not Implemented on platform!
#endif

  Diligent::SwapChainDesc SCDesc;
  SCDesc.IsPrimary         = m_WindowDesc.m_bIsPrimarySwapchain;
  SCDesc.Width             = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SCDesc.Height            = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SCDesc.ColorBufferFormat = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  // Enforce double buffering on the UWP Platform
  SCDesc.BufferCount = 2;
#elif XII_ENABLED(XII_PLATFORM_OSX)
  // There needs to be at least 3 buffers in Metal to avoid massive performance degradation
  // in fullscreen mode.
  // https://github.com/KhronosGroup/MoltenVK/issues/808
  SCDesc.BufferCount = 3;
#else
  SCDesc.BufferCount = m_WindowDesc.m_bDoubleBuffered ? 2 : 1;
#endif

  const Diligent::RENDER_DEVICE_TYPE& deviceType = pDeviceDiligent->GetDeviceType();
  switch (deviceType)
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      Diligent::FullScreenModeDesc FSMDesc;

      auto* pFactoryD3D11 = static_cast<Diligent::IEngineFactoryD3D11*>(pDeviceDiligent->GetFactory().RawPtr());
      pFactoryD3D11->CreateSwapChainD3D11(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
      auto* pFactoryD3D12 = static_cast<Diligent::IEngineFactoryD3D12*>(pDeviceDiligent->GetFactory().RawPtr());

      Diligent::FullScreenModeDesc FSMDesc;
      pFactoryD3D12->CreateSwapChainD3D12(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_GL:
    case Diligent::RENDER_DEVICE_TYPE_GLES:
    {
      auto* pFactoryOpenGL = static_cast<Diligent::IEngineFactoryOpenGL*>(pDeviceDiligent->GetFactory().RawPtr());

      Diligent::EngineGLCreateInfo EngineCI;
      EngineCI.Window = Window;

      if (pDeviceDiligent->GetValidationLevel() >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(pDeviceDiligent->GetValidationLevel()));

      bool bForceNonSeprblProgs = false;
      if (bForceNonSeprblProgs)
        EngineCI.Features.SeparablePrograms = Diligent::DEVICE_FEATURE_STATE_DISABLED;

      if (EngineCI.NumDeferredContexts != 0)
      {
        xiiLog::Warning("Deferred contexts are not supported in OpenGL mode");
        EngineCI.NumDeferredContexts = 0;
      }

      EngineCI.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.BinaryOcclusionQueries    = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
      EngineCI.Features.DurationQueries           = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

      EngineCI.Features.MultithreadedResourceCreation = Diligent::DEVICE_FEATURE_STATE_ENABLED;
      EngineCI.Features.WireframeFill                 = Diligent::DEVICE_FEATURE_STATE_ENABLED;

      Diligent::RefCntAutoPtr<Diligent::IDeviceContext>& pContext = pDeviceDiligent->GetImmediateContext();

      pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext().RawDblPtr(), SCDesc, &m_pSwapChain);
      if (!pDeviceDiligent->GetDevice())
      {
        xiiLog::Error("Unable to initialize Diligent Engine in OpenGL mode. The API may not be available, "
                      "or required features may not be supported by this GPU/driver/OS version.");
      }
    }
    break;
#endif

#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
    {
      auto* pFactoryVk = static_cast<Diligent::IEngineFactoryVk*>(pDeviceDiligent->GetFactory().RawPtr());

      pFactoryVk->CreateSwapChainVk(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if METAL_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_METAL:
    {
      auto* pFactoryMtl = static_cast<Diligent::IEngineFactoryMtl>(pDeviceDiligent->GetFactory().RawPtr());

      pFactoryMtl->CreateSwapChainMtl(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!m_pSwapChain)
  {
    xiiLog::Error("Failed to create device SwapChain");
    return XII_FAILURE;
  }

  return CreateBackBufferInternal(pDeviceDiligent);
}

xiiResult xiiGALSwapChainDiligent::CreateBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent)
{
  Diligent::ITexture* pRTV = m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture();
  Diligent::ITexture* pDSV = m_pSwapChain->GetDepthBufferDSV()->GetTexture();

  if (pRTV == nullptr || pDSV == nullptr)
  {
    xiiLog::Error("Couldn't access backbuffer texture of swapchain");
    XII_GAL_DILIGENT_RELEASE(m_pSwapChain);

    return XII_FAILURE;
  }

  const Diligent::TextureDesc& rtvDesc = pRTV->GetDesc();

  xiiGALTextureCreationDescription TexDesc;
  TexDesc.m_Type                        = xiiGALTextureType ::Texture2D;
  TexDesc.m_szName                      = rtvDesc.Name;
  TexDesc.m_uiWidth                     = rtvDesc.Width;
  TexDesc.m_uiHeight                    = rtvDesc.Height;
  TexDesc.m_SampleCount                 = xiiDiligentUtils::ToGALMSAASampleCount(rtvDesc.SampleCount);
  TexDesc.m_uiDepth                     = rtvDesc.Depth;
  TexDesc.m_pExisitingNativeObject      = pRTV;
  TexDesc.m_bAllowShaderResourceView    = false;
  TexDesc.m_bCreateRenderTarget         = false;
  TexDesc.m_ResourceAccess.m_bImmutable = true;
  TexDesc.m_ResourceAccess.m_bReadBack  = false;
  TexDesc.m_Format                      = xiiGALResourceFormat::RGBAUByteNormalizedsRGB;

  xiiGALSystemMemoryDescription temp[1];
  m_hBackbufferTexture = pDeviceDiligent->CreateTexture(TexDesc, xiiArrayPtr<xiiGALSystemMemoryDescription>(temp));

  XII_ASSERT_RELEASE(!m_hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");
  m_RenderTargets.m_hRTs[0] = m_hBackbufferTexture;

  xiiGALRenderTargetViewCreationDescription RTViewDesc;
  RTViewDesc.m_bReadOnly              = true;
  RTViewDesc.m_hTexture               = m_hBackbufferTexture;
  RTViewDesc.m_uiSliceCount           = rtvDesc.GetArraySize();
  RTViewDesc.m_pExisitingNativeObject = m_pSwapChain->GetCurrentBackBufferRTV();

  m_hBackbufferTextureView = pDeviceDiligent->CreateRenderTargetView(RTViewDesc);

  XII_ASSERT_RELEASE(!m_hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  return XII_SUCCESS;
}

xiiResult xiiGALSwapChainDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  m_RenderTargets.m_hRTs[0].Invalidate();

  if (!m_hBackbufferTexture.IsInvalidated())
    m_hBackbufferTexture.Invalidate();

  if (!m_hBackbufferTextureView.IsInvalidated())
    m_hBackbufferTextureView.Invalidate();

  // Full screen swap chains must be switched to windowed mode before destruction.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
  m_pSwapChain->SetWindowedMode();

  XII_GAL_DILIGENT_RELEASE(m_pSwapChain);

  return XII_FAILURE;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_SwapChainDiligent);
