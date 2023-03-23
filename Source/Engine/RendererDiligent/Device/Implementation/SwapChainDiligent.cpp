#include <RendererDiligent/RendererDiligentPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/SwapChainDiligent.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/IncludeX11.h>
#endif

#if D3D11_SUPPORTED
#  include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#endif

#if D3D12_SUPPORTED
#  include <Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#endif

#if VULKAN_SUPPORTED
#  include <Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#endif

template <>
struct xiiHashHelper<xiiGALSwapChainDiligent::RenderTargetInfo>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiGALSwapChainDiligent::RenderTargetInfo& value)
  {
    const xiiUInt32 hashA = xiiHashHelper<const void*>::Hash(value.m_pTexture);
    const xiiUInt32 hashB = xiiHashHelper<const void*>::Hash(value.m_pTextureView);
    return xiiHashingUtils::CombineHashValues32(hashA, hashB);
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiGALSwapChainDiligent::RenderTargetInfo& a, const xiiGALSwapChainDiligent::RenderTargetInfo& b)
  {
    return a.m_pTexture == b.m_pTexture && a.m_pTextureView == b.m_pTextureView;
  }
};

xiiGALResourceFormat::Enum ToGALRenderTargetFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
      return xiiGALResourceFormat::RGBAUByteNormalized;
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
      return xiiGALResourceFormat::BGRAUByteNormalized;
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
      return xiiGALResourceFormat::RGBAUByteNormalizedsRGB;
    case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRAUByteNormalizedsRGB;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Invalid;
}

void xiiGALSwapChainDiligent::AcquireNextRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("AcquireNextRenderTarget");

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  Diligent::ITexture*     pCurrentTexture     = m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture();
  Diligent::ITextureView* pCurrentTextureView = m_pSwapChain->GetCurrentBackBufferRTV();
  RenderTargetInfo        rendertargetInfo{pCurrentTexture, pCurrentTextureView};

  xiiGALTextureHandle hBackbufferTexture;
  if (!m_BackbufferTextures.TryGetValue(rendertargetInfo, hBackbufferTexture))
  {
    if (CreateBackBufferInternal(pDeviceDiligent, false).Failed())
    {
      xiiLog::Error("Failed to acquire next render target");
    }
  }

  XII_ASSERT_DEV(m_BackbufferTextures.TryGetValue(rendertargetInfo, hBackbufferTexture), "Failed to retrieve backbuffer texture handle. This should have succeeded.");

  m_RenderTargets.m_hRTs[0] = hBackbufferTexture;
}

void xiiGALSwapChainDiligent::PresentRenderTarget(xiiGALDevice* pDevice)
{
  XII_PROFILE_SCOPE("PresentRenderTarget");

  // xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  m_pSwapChain->Present(m_CurrentPresentMode == xiiGALPresentMode::VSync ? 1u : 0u);
}

xiiResult xiiGALSwapChainDiligent::UpdateSwapChain(xiiGALDevice* pDevice, xiiEnum<xiiGALPresentMode> newPresentMode)
{
  XII_PROFILE_SCOPE("UpdateSwapChain");

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  m_CurrentPresentMode = newPresentMode;

  DestroyBackBufferInternal(pDeviceDiligent);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDeviceDiligent->FlushDeadObjects();

  m_pSwapChain->Resize(m_WindowDesc.m_pWindow->GetClientAreaSize().width, m_WindowDesc.m_pWindow->GetClientAreaSize().height);

  return CreateBackBufferInternal(pDeviceDiligent, true);
}

xiiGALSwapChainDiligent::xiiGALSwapChainDiligent(const xiiGALWindowSwapChainCreationDescription& Description) :
  xiiGALWindowSwapChain(Description), m_pSwapChain(nullptr)
{
}

xiiGALSwapChainDiligent::~xiiGALSwapChainDiligent() {}

xiiResult xiiGALSwapChainDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  Diligent::Win32NativeWindow Window{xiiMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle())};
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  xiiWindowHandle             xcbWindowHandle = m_WindowDesc.m_pWindow->GetNativeWindowHandle();
  Diligent::LinuxNativeWindow Window          = {};
  Window.WindowId                             = xiiMinX11::ToNative(xcbWindowHandle.xcbWindow.m_hWindow);
  Window.pXCBConnection                       = xcbWindowHandle.xcbWindow.m_pConnection;
#else
#  error Not Implemented on platform!
#endif

  Diligent::SwapChainDesc SCDesc;
  SCDesc.IsPrimary         = m_WindowDesc.m_bIsPrimarySwapchain;
  SCDesc.Width             = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SCDesc.Height            = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SCDesc.ColorBufferFormat = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
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

  const Diligent::RENDER_DEVICE_TYPE& deviceType = pDeviceDiligent->GetDeviceType();
  switch (deviceType)
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      Diligent::FullScreenModeDesc FSMDesc;

      auto* pFactoryD3D11 = static_cast<Diligent::IEngineFactoryD3D11*>(pDeviceDiligent->GetFactory());
      pFactoryD3D11->CreateSwapChainD3D11(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
      auto* pFactoryD3D12 = static_cast<Diligent::IEngineFactoryD3D12*>(pDeviceDiligent->GetFactory());

      Diligent::FullScreenModeDesc FSMDesc;
      pFactoryD3D12->CreateSwapChainD3D12(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, FSMDesc, Window, &m_pSwapChain);
    }
    break;
#endif

#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
    {
      auto* pFactoryVk = static_cast<Diligent::IEngineFactoryVk*>(pDeviceDiligent->GetFactory());

      pFactoryVk->CreateSwapChainVk(pDeviceDiligent->GetDevice(), pDeviceDiligent->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
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

  return CreateBackBufferInternal(pDeviceDiligent, true);
}

xiiResult xiiGALSwapChainDiligent::CreateBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent, bool bInitPlatform)
{
  Diligent::ITextureView* pRTV     = m_pSwapChain->GetCurrentBackBufferRTV();
  Diligent::ITexture*     pTexture = pRTV->GetTexture();

  if (pRTV == nullptr)
  {
    xiiLog::Error("Couldn't access backbuffer texture of swapchain");
    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pSwapChain);

    return XII_FAILURE;
  }

  const Diligent::TextureDesc&     textureDesc = pTexture->GetDesc();
  const Diligent::TextureViewDesc& rtvDesc     = pRTV->GetDesc();

  xiiGALTextureCreationDescription TexDesc;
  TexDesc.m_Type                        = xiiGALTextureType ::Texture2D;
  TexDesc.m_szName                      = textureDesc.Name;
  TexDesc.m_uiWidth                     = textureDesc.Width;
  TexDesc.m_uiHeight                    = textureDesc.Height;
  TexDesc.m_SampleCount                 = xiiDiligentUtils::ToGALMSAASampleCount(textureDesc.SampleCount);
  TexDesc.m_uiDepth                     = textureDesc.Depth;
  TexDesc.m_pExisitingNativeObject      = pTexture;
  TexDesc.m_bAllowShaderResourceView    = false;
  TexDesc.m_bCreateRenderTarget         = true;
  TexDesc.m_ResourceAccess.m_bImmutable = true;
  TexDesc.m_ResourceAccess.m_bReadBack  = false;
  TexDesc.m_Format                      = ToGALRenderTargetFormat(rtvDesc.Format);

  xiiGALTextureHandle hBackbufferTexture = pDeviceDiligent->CreateTexture(TexDesc);

  XII_ASSERT_RELEASE(!hBackbufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  RenderTargetInfo rendertargetInfo{pTexture, pRTV};
  m_BackbufferTextures.Insert(rendertargetInfo, hBackbufferTexture);

  if (bInitPlatform)
  {
    m_RenderTargets.m_hRTs[0] = hBackbufferTexture;
  }

  return XII_SUCCESS;
}

void xiiGALSwapChainDiligent::DestroyBackBufferInternal(xiiGALDeviceDiligent* pDeviceDiligent)
{
  for (auto iter : m_BackbufferTextures)
  {
    pDeviceDiligent->DestroyTexture(iter.Value());

    iter.Value().Invalidate();
  }

  m_RenderTargets.m_hRTs[0].Invalidate();

  m_BackbufferTextures.Clear();
  m_BackbufferTextures.Compact();
}

xiiResult xiiGALSwapChainDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  DestroyBackBufferInternal(pDeviceDiligent);

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
