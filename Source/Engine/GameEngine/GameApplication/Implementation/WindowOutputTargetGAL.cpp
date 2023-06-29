#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

xiiWindowOutputTargetGAL::xiiWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged) :
  m_OnSwapChainChanged(onSwapChainChanged)
{
}

xiiWindowOutputTargetGAL::~xiiWindowOutputTargetGAL()
{
  xiiGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  xiiGALDevice::GetDefaultDevice()->WaitIdle();
}

void xiiWindowOutputTargetGAL::CreateSwapchain(const xiiGALWindowSwapChainCreationDescription& desc)
{
  m_currentDesc = desc;
  // xiiWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_Size                             = desc.m_pWindow->GetClientAreaSize();
  m_currentDesc.m_InitialPresentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    xiiGALDevice* pDevice    = xiiGALDevice::GetDefaultDevice();
    auto*         pSwapchain = pDevice->GetSwapChain<xiiGALWindowSwapChain>(m_hSwapChain);
    pDevice->UpdateSwapChain(m_hSwapChain, xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate).AssertSuccess("");
    if (bSwapChainExisted && m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      xiiSizeU32 currentSize = pSwapchain->GetCurrentSize();
      m_OnSwapChainChanged(m_hSwapChain, currentSize);
    }
  }
  else
  {
    m_hSwapChain = xiiGALWindowSwapChain::Create(m_currentDesc);
  }
}

void xiiWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  // Only re-create the swapchain if somebody is listening to changes.
  if (m_OnSwapChainChanged.IsValid())
  {
    xiiEnum<xiiGALPresentMode> presentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

    // The actual present call is done by setting the swapchain to a xiiView.
    // This call is only used to recreate the swapchain at a safe location.
    if (m_Size != m_currentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_currentDesc.m_InitialPresentMode)
    {
      CreateSwapchain(m_currentDesc);
    }
  }
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("CaptureImage");
  XII_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  auto pGALCommandEncoder = pGALPass->BeginRendering(xiiGALRenderingSetup());
  XII_SCOPE_EXIT(pGALPass->EndRendering(pGALCommandEncoder));

  const xiiGALSwapChain* pSwapChain  = pDevice->GetSwapChain(m_hSwapChain);
  xiiGALTextureHandle    hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : xiiGALTextureHandle();

  pGALCommandEncoder->ReadbackTexture(hBackbuffer);

  const xiiGALTexture*                pBackbuffer = xiiGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const xiiUInt32                     uiWidth     = pBackbuffer->GetDescription().m_uiWidth;
  const xiiUInt32                     uiHeight    = pBackbuffer->GetDescription().m_uiHeight;
  const xiiEnum<xiiGALResourceFormat> format      = pBackbuffer->GetDescription().m_Format;

  xiiDynamicArray<xiiUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  xiiGALSystemMemoryDescription MemDesc;
  MemDesc.m_uiRowPitch   = 4 * uiWidth;
  MemDesc.m_uiSlicePitch = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  MemDesc.m_pData = backbufferData.GetData();
  xiiArrayPtr<xiiGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
  xiiGALTextureSubresource                   sourceSubResource;
  xiiArrayPtr<xiiGALTextureSubresource>      sourceSubResources(&sourceSubResource, 1);
  pGALCommandEncoder->CopyTextureReadbackResult(hBackbuffer, sourceSubResources, SysMemDescsDepth);

  xiiImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(format, true));
  out_image.ResetAndAlloc(header);
  xiiUInt8* pData = out_image.GetPixelPointer<xiiUInt8>();

  xiiMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
