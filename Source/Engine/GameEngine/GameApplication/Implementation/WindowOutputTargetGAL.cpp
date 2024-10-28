#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Image.h>

xiiWindowOutputTargetGAL::xiiWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged) :
  m_OnSwapChainChanged(onSwapChainChanged)
{
}

xiiWindowOutputTargetGAL::~xiiWindowOutputTargetGAL()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_hBackbufferStagingTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hBackbufferStagingTexture);
    m_hBackbufferStagingTexture.Invalidate();
  }

  if (!m_hSwapChain.IsInvalidated())
  {
    pDevice->DestroySwapChain(m_hSwapChain);
    m_hSwapChain.Invalidate();
  }

  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  pDevice->WaitIdle();
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
    pSwapchain->Resize(m_Size).IgnoreResult();

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

void xiiWindowOutputTargetGAL::AcquireImage()
{
  // For now, the actual acquire call is done during xiiGALDevice::BeginFrame by calling xiiGALDevice::EnqueueFrameSwapChain before the render loop.
  // This call is only used to recreate the swapchain at a safe location.

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

void xiiWindowOutputTargetGAL::PresentImage(bool bEnableVSync)
{
  // For now, the actual present call is done during xiiGALDevice::EndFrame by calling xiiGALDevice::EnqueueFrameSwapChain before the render loop.
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  xiiGALDevice*          pDevice        = xiiGALDevice::GetDefaultDevice();
  const xiiGALSwapChain* pSwapChain     = pDevice->GetSwapChain(m_hSwapChain);
  const auto&            backbufferDesc = pDevice->GetTexture(pSwapChain->GetBackBufferTexture())->GetDescription();

  // Create/Re-create staging texture to match the resolution and format of the backbuffer.
  {
    bool bRecreateStagingTexture = true;
    if (xiiGALTexture* pExistingStagingTexture = pDevice->GetTexture(m_hBackbufferStagingTexture))
    {
      const auto& stagingTextureDescription = pExistingStagingTexture->GetDescription();

      if (stagingTextureDescription.m_Size == backbufferDesc.m_Size && stagingTextureDescription.m_Format == backbufferDesc.m_Format)
      {
        bRecreateStagingTexture = false;
      }
    }

    if (bRecreateStagingTexture)
    {
      if (!m_hBackbufferStagingTexture.IsInvalidated())
      {
        pDevice->DestroyTexture(m_hBackbufferStagingTexture);
        m_hBackbufferStagingTexture.Invalidate();
      }

      xiiGALTextureCreationDescription swapChainStagingTextureDescription;
      swapChainStagingTextureDescription.m_Type           = xiiGALResourceDimension::Texture2D;
      swapChainStagingTextureDescription.m_Size           = backbufferDesc.m_Size;
      swapChainStagingTextureDescription.m_Format         = backbufferDesc.m_Format;
      swapChainStagingTextureDescription.m_Usage          = xiiGALResourceUsage::Staging;
      swapChainStagingTextureDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;

      m_hBackbufferStagingTexture = pDevice->CreateTexture(swapChainStagingTextureDescription);

      pDevice->GetTexture(m_hBackbufferStagingTexture)->SetDebugName("Image Capture Staging Texture");
    }
  }

  auto pCommandQueue   = pDevice->GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics);
  auto pGALCommandList = pCommandQueue->BeginCommandList();

  pGALCommandList->BeginDebugGroup("CaptureImage");

  xiiGALTextureHandle hBackbuffer = pSwapChain->GetBackBufferTexture();

  pGALCommandList->CopyTexture(hBackbuffer, m_hBackbufferStagingTexture);

  // Since we are reading data from the backbuffer, we need to ensure that the copy command has completed before mapping the staging texture for reading.
  // This is mainly a D3D11 deferred context limitation, we will need to update/branch this code path on modern api's like D3D12 and Vulkan.
  pGALCommandList->Submit(false);
  pCommandQueue->WaitForIdle();

  pGALCommandList->Begin();

  const xiiGALTexture*               pBackbuffer = xiiGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const xiiUInt32                    uiWidth     = pBackbuffer->GetDescription().m_Size.width;
  const xiiUInt32                    uiHeight    = pBackbuffer->GetDescription().m_Size.height;
  const xiiEnum<xiiGALTextureFormat> format      = pBackbuffer->GetDescription().m_Format;

  xiiDynamicArray<xiiUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  const xiiUInt32 uiStride      = 4 * uiWidth;
  const xiiUInt32 uiDepthStride = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  xiiGALTextureMipLevelData      sourceSubResource;
  xiiGALMappedTextureSubresource mappedSubResource;
  XII_SUCCEED_OR_RETURN(pGALCommandList->MapTextureSubresource(m_hBackbufferStagingTexture, sourceSubResource, xiiGALMapType::Read, xiiGALMapFlags::None, nullptr, mappedSubResource));

  const auto& textureDescription = pDevice->GetTexture(m_hBackbufferStagingTexture)->GetDescription();
  const auto& formatProperties   = xiiGALTextureUtilities::GetTextureFormatProperties(textureDescription.m_Format);

  if (mappedSubResource.m_pData)
  {
    /// \todo Support depth pitch.
    if (mappedSubResource.m_uiStride == uiStride)
    {
      const xiiUInt32 uiMemorySize = formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.width, sourceSubResource.m_uiMipLevel) * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.height, sourceSubResource.m_uiMipLevel);

      memcpy(backbufferData.GetData(), mappedSubResource.m_pData, uiMemorySize);
    }
    else
    {
      // Copy row by row.
      const xiiUInt32 uiHeight = xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.height, sourceSubResource.m_uiMipLevel);

      for (xiiUInt32 y = 0; y < uiHeight; ++y)
      {
        const void* pSource      = xiiMemoryUtils::AddByteOffset(mappedSubResource.m_pData, y * mappedSubResource.m_uiStride);
        void*       pDestination = xiiMemoryUtils::AddByteOffset(backbufferData.GetData(), y * uiStride);

        memcpy(pDestination, pSource, formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.width, sourceSubResource.m_uiMipLevel));
      }
    }
  }
  else
  {
    xiiLog::Error("Failed to map texture subresource for reading backbuffer data.");
  }

  pGALCommandList->UnmapTextureSubresource(m_hBackbufferStagingTexture, sourceSubResource).IgnoreResult();

  pGALCommandList->EndDebugGroup();
  pGALCommandList->Submit();

  pCommandQueue->WaitForIdle();

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
