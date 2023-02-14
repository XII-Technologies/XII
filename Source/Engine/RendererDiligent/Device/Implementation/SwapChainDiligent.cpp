#include <RendererDiligent/RendererDiligentPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
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
  XII_PROFILE_SCOPE("AcquireNextRenderTarget");

  Diligent::ITextureView* pCurrentBackbuffer = m_pSwapChain->GetCurrentBackBufferRTV();

  if (pCurrentBackbuffer != m_pCurrentBackbufferRTV)
  {
    DestroyBackBufferInternal(m_pDeviceDiligent);

    if (CreateBackBufferInternal(m_pDeviceDiligent).Failed())
    {
      xiiLog::Error("Failed to acquire next render target");
    }
  }
}

void xiiGALSwapChainDiligent::PresentRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  m_pSwapChain->Present(m_CurrentPresentMode == xiiGALPresentMode::VSync ? 1u : 0u);
}

xiiResult xiiGALSwapChainDiligent::UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  XII_PROFILE_SCOPE("UpdateSwapChain");

  m_CurrentPresentMode = newPresentMode;

  DestroyBackBufferInternal(m_pDeviceDiligent);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  m_pDeviceDiligent->FlushDeadObjects();

  m_pSwapChain->Resize(m_WindowDesc.m_pWindow->GetClientAreaSize().width, m_WindowDesc.m_pWindow->GetClientAreaSize().height);

  return CreateBackBufferInternal(m_pDeviceDiligent);
}

xiiGALSwapChainDiligent::xiiGALSwapChainDiligent(const xiiGALWindowSwapChainCreationDescription& Description) :
  xiiGALWindowSwapChain(Description), m_pSwapChain(nullptr), m_pDeviceDiligent(nullptr), m_pCurrentBackbufferRTV(nullptr)
{
}

xiiGALSwapChainDiligent::~xiiGALSwapChainDiligent() {}

xiiResult xiiGALSwapChainDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  Diligent::Win32NativeWindow Window{xiiMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle())};
#else
#  error Not Implemented on platform!
#endif

  Diligent::SwapChainDesc SCDesc;
  SCDesc.IsPrimary         = m_WindowDesc.m_bIsPrimarySwapchain;
  SCDesc.Width             = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SCDesc.Height            = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SCDesc.ColorBufferFormat = m_pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  // Do not set the depth format so a default one will not be created.
  SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_UNKNOWN;

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

  const Diligent::RENDER_DEVICE_TYPE& deviceType = m_pDeviceDiligent->GetDeviceType();
  switch (deviceType)
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      Diligent::FullScreenModeDesc FSMDesc;

      auto* pFactoryD3D11 = static_cast<Diligent::IEngineFactoryD3D11*>(m_pDeviceDiligent->GetFactory());
      pFactoryD3D11->CreateSwapChainD3D11(m_pDeviceDiligent->GetDevice(), m_pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
      auto* pFactoryD3D12 = static_cast<Diligent::IEngineFactoryD3D12*>(m_pDeviceDiligent->GetFactory());

      Diligent::FullScreenModeDesc FSMDesc;
      pFactoryD3D12->CreateSwapChainD3D12(m_pDeviceDiligent->GetDevice(), m_pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_GL:
    case Diligent::RENDER_DEVICE_TYPE_GLES:
    {
      auto* pFactoryOpenGL = static_cast<Diligent::IEngineFactoryOpenGL*>(m_pDeviceDiligent->GetFactory());

      Diligent::EngineGLCreateInfo EngineCI;
      EngineCI.Window = Window;

      if (m_pDeviceDiligent->GetValidationLevel() >= 0)
        EngineCI.SetValidationLevel(static_cast<Diligent::VALIDATION_LEVEL>(m_pDeviceDiligent->GetValidationLevel()));

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

      Diligent::IRenderDevice*  pDevice  = m_pDeviceDiligent->GetDevice();
      Diligent::IDeviceContext* pContext = m_pDeviceDiligent->GetImmediateContext();

      pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &pDevice, &pContext, SCDesc, &m_pSwapChain);
      if (!m_pDeviceDiligent->GetDevice())
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
      auto* pFactoryVk = static_cast<Diligent::IEngineFactoryVk*>(m_pDeviceDiligent->GetFactory());

      pFactoryVk->CreateSwapChainVk(m_pDeviceDiligent->GetDevice(), m_pDeviceDiligent->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if METAL_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_METAL:
    {
      auto* pFactoryMtl = static_cast<Diligent::IEngineFactoryMtl>(m_pDeviceDiligent->GetFactory());

      pFactoryMtl->CreateSwapChainMtl(m_pDeviceDiligent->GetDevice(), m_pDeviceDiligent->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (m_pSwapChain == nullptr)
  {
    return XII_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();

  return CreateBackBufferInternal(m_pDeviceDiligent);
}

xiiResult xiiGALSwapChainDiligent::CreateBackBufferInternal(xiiGALDeviceDiligent* m_pDeviceDiligent)
{
  Diligent::ITextureView* pRTV     = m_pSwapChain->GetCurrentBackBufferRTV();
  Diligent::ITexture*     pTexture = pRTV->GetTexture();

  if (pRTV == nullptr)
  {
    xiiLog::Error("Couldn't access backbuffer texture of swapchain");
    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pSwapChain);

    return XII_FAILURE;
  }

  const Diligent::TextureDesc& rtvDesc = pTexture->GetDesc();

  xiiGALTextureCreationDescription TexDesc;
  TexDesc.m_Type                        = xiiGALTextureType ::Texture2D;
  TexDesc.m_szName                      = rtvDesc.Name;
  TexDesc.m_uiWidth                     = rtvDesc.Width;
  TexDesc.m_uiHeight                    = rtvDesc.Height;
  TexDesc.m_SampleCount                 = xiiDiligentUtils::ToGALMSAASampleCount(rtvDesc.SampleCount);
  TexDesc.m_uiDepth                     = rtvDesc.Depth;
  TexDesc.m_pExisitingNativeObject      = pTexture;
  TexDesc.m_bAllowShaderResourceView    = false;
  TexDesc.m_bCreateRenderTarget         = true;
  TexDesc.m_ResourceAccess.m_bImmutable = true;
  TexDesc.m_ResourceAccess.m_bReadBack  = false;
  TexDesc.m_Format                      = xiiGALResourceFormat::RGBAUByteNormalizedsRGB;

  m_hBackbufferTexture = m_pDeviceDiligent->CreateTexture(TexDesc);

  XII_ASSERT_RELEASE(!m_hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");
  m_RenderTargets.m_hRTs[0] = m_hBackbufferTexture;

  m_pCurrentBackbufferRTV = pRTV;

  return XII_SUCCESS;
}

void xiiGALSwapChainDiligent::DestroyBackBufferInternal(xiiGALDeviceDiligent* m_pDeviceDiligent)
{
  m_pDeviceDiligent->DestroyTexture(m_hBackbufferTexture);

  m_hBackbufferTexture.Invalidate();

  m_RenderTargets.m_hRTs[0].Invalidate();
}

xiiResult xiiGALSwapChainDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  DestroyBackBufferInternal(m_pDeviceDiligent);

  if (m_pSwapChain)
  {
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    m_pSwapChain->SetWindowedMode();

    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pSwapChain);

    m_WindowDesc.m_pWindow->RemoveReference();
  }

  return XII_FAILURE;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_SwapChainDiligent);
