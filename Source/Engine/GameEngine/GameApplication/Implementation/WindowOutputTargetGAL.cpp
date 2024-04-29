#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
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

void xiiWindowOutputTargetGAL::CreateSwapchain(const xiiGALSwapChainCreationDescription& desc)
{
  m_CurrentDesc = desc;
  // xiiWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_Size        = desc.m_pWindow->GetClientAreaSize();
  m_PresentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    auto* pSwapchain = pDevice->GetSwapChain(m_hSwapChain);

    pSwapchain->SetPresentMode(m_PresentMode);
    pSwapchain->Resize(pDevice, m_Size).IgnoreResult();

    if (bSwapChainExisted && m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      xiiSizeU32 currentSize = pSwapchain->GetCurrentSize();
      m_OnSwapChainChanged(m_hSwapChain, currentSize);
    }
  }
  else
  {
    m_hSwapChain = pDevice->CreateSwapChain(m_CurrentDesc);

    if (xiiGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain))
    {
      m_PresentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

      pSwapChain->SetPresentMode(m_PresentMode);
    }
  }
}

void xiiWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  // Only re-create the swapchain if somebody is listening to changes.
  if (m_OnSwapChainChanged.IsValid())
  {
    xiiEnum<xiiGALPresentMode> presentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

    // The actual present call is done by setting the swapchain to an xiiView.
    // This call is only used to recreate the swapchain at a safe location.
    if (m_Size != m_CurrentDesc.m_pWindow->GetClientAreaSize() || m_PresentMode != presentMode)
    {
      CreateSwapchain(m_CurrentDesc);
    }
  }
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  /// \todo Implement screen capture.
#if 0
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("CaptureImage");
  XII_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  auto pGALCommandEncoder = pGALPass->BeginRendering(xiiGALRenderingSetup());
  XII_SCOPE_EXIT(pGALPass->EndRendering(pGALCommandEncoder));

  const xiiGALSwapChain* pSwapChain  = pDevice->GetSwapChain(m_hSwapChain);
  xiiGALTextureHandle    hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : xiiGALTextureHandle();

  pGALCommandEncoder->ReadbackTexture(hBackbuffer);

  const xiiGALTexture*               pBackbuffer = xiiGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const xiiUInt32                    uiWidth     = pBackbuffer->GetDescription().m_Size.width;
  const xiiUInt32                    uiHeight    = pBackbuffer->GetDescription().m_Size.height;
  const xiiEnum<xiiGALTextureFormat> format      = pBackbuffer->GetDescription().m_Format;

  xiiDynamicArray<xiiUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  xiiGALTextureSubResourceData MemDesc;
  MemDesc.m_uiStride      = 4 * uiWidth;
  MemDesc.m_uiDepthStride = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  MemDesc.m_pData = backbufferData.GetData();
  xiiArrayPtr<xiiGALTextureSubResourceData> SysMemDescsDepth(&MemDesc, 1);
  xiiGALTextureMipLevelData                 sourceSubResource;
  xiiArrayPtr<xiiGALTextureMipLevelData>    sourceSubResources(&sourceSubResource, 1);
  pGALCommandEncoder->CopyTextureReadbackResult(hBackbuffer, xiiGALTextureHandle(), sourceSubResources, SysMemDescsDepth);

  xiiImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(format, true));
  out_image.ResetAndAlloc(header);
  xiiUInt8* pData = out_image.GetPixelPointer<xiiUInt8>();

  xiiMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
#endif

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
