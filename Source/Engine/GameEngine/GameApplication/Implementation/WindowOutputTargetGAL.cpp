#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Tools/ImageCapture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Image.h>

xiiWindowOutputTargetGAL::xiiWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged) :
  m_OnSwapChainChanged(onSwapChainChanged)
{
  m_pImageCapture = XII_DEFAULT_NEW(xiiGALImageCapture, xiiGALDevice::GetDefaultDevice());
}

xiiWindowOutputTargetGAL::~xiiWindowOutputTargetGAL()
{
  m_pImageCapture.Clear();
  m_pSwapChain.Clear();

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

  if (m_pSwapChain)
  {
    m_pSwapChain->SetPresentMode(m_PresentMode);
    m_pSwapChain->Resize(m_Size).IgnoreResult();

    if (m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      m_OnSwapChainChanged(m_pSwapChain, m_pSwapChain->GetCurrentSize());
    }
  }
  else
  {
    m_pSwapChain = pDevice->CreateSwapChain(m_CurrentDesc);

    if (m_pSwapChain)
    {
      m_PresentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

      m_pSwapChain->SetPresentMode(m_PresentMode);
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

    // The actual present call is done by setting the swapchain to a xiiView.
    // This call is only used to recreate the swapchain at a safe location.
    if (m_Size != m_CurrentDesc.m_pWindow->GetClientAreaSize() || m_PresentMode != presentMode)
    {
      CreateSwapchain(m_CurrentDesc);
    }
  }
}

void xiiWindowOutputTargetGAL::PresentImage(bool bEnableVSync)
{
  if (!m_pSwapChain)
    return;

  m_pSwapChain->SetPresentMode(bEnableVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
  m_pSwapChain->Present();
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  if (!m_pSwapChain)
  {
    xiiLog::Error("No swapchain available for image capture.");
    return XII_FAILURE;
  }

  auto pGraphicsQueue = xiiGALDevice::GetDefaultDevice()->GetDefaultCommandQueue();

  if (auto pCommandList = pGraphicsQueue->BeginCommandList())
  {
    m_pImageCapture->Capture(m_pSwapChain, pCommandList, m_uiCurrentFrame);

    ++m_uiCurrentFrame;

    pCommandList->Submit();
  }

  if (m_pImageCapture)
  {
    while (auto capture = m_pImageCapture->GetCapture())
    {
      const auto& textureDescription = capture.m_pTexture->GetDescription();

      xiiDynamicArray<xiiUInt8> backbufferData;
      backbufferData.SetCountUninitialized(textureDescription.m_Size.width * textureDescription.m_Size.height * 4);

      const xiiUInt32 uiStride      = 4 * textureDescription.m_Size.width;
      const xiiUInt32 uiDepthStride = 4 * textureDescription.m_Size.width * textureDescription.m_Size.height;

      auto pGraphicsQueue = xiiGALDevice::GetDefaultDevice()->GetDefaultCommandQueue();

      if (auto pCommandList = pGraphicsQueue->BeginCommandList())
      {
        xiiGALTextureMipLevelData      sourceSubResource;
        xiiGALMappedTextureSubresource mappedSubResource;
        pCommandList->MapTextureSubresource(capture.m_pTexture, sourceSubResource, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).IgnoreResult();

        const auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

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

        pCommandList->UnmapTextureSubresource(capture.m_pTexture, sourceSubResource).IgnoreResult();
        pCommandList->Submit();
      }

      m_pImageCapture->RecycleStagingTexture(std::move(capture.m_pTexture));
      xiiImageHeader header;
      header.SetWidth(textureDescription.m_Size.width);
      header.SetHeight(textureDescription.m_Size.height);
      header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(textureDescription.m_Format, true));
      out_image.ResetAndAlloc(header);
      xiiUInt8* pData = out_image.GetPixelPointer<xiiUInt8>();

      xiiMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
    }

#ifdef CORE_ENABLE
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

    auto pCommandQueue   = pDevice->GetDefaultCommandQueue();
    auto pGALCommandList = pCommandQueue->BeginCommandList();

    pGALCommandList->BeginDebugGroup("CaptureImage");

    xiiGALTextureHandle hBackbuffer = pSwapChain->GetBackBufferTexture();

    pGALCommandList->CopyTexture(hBackbuffer, m_hBackbufferStagingTexture);

    const xiiGALTexture*                pBackbuffer = xiiGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
    const xiiUInt32                     uiWidth     = pBackbuffer->GetDescription().m_Size.width;
    const xiiUInt32                     uiHeight    = pBackbuffer->GetDescription().m_Size.height;
    const xiiEnum<xiiGALResourceFormat> format      = pBackbuffer->GetDescription().m_Format;

    xiiDynamicArray<xiiUInt8> backbufferData;
    backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

    const xiiUInt32 uiStride      = 4 * uiWidth;
    const xiiUInt32 uiDepthStride = 4 * uiWidth * uiHeight;

    /// \todo Make this more efficient
    xiiGALTextureMipLevelData      sourceSubResource;
    xiiGALMappedTextureSubresource mappedSubResource;
    XII_SUCCEED_OR_RETURN(pGALCommandList->MapTextureSubresource(m_hBackbufferStagingTexture, sourceSubResource, xiiGALMapType::Read, xiiGALMapFlags::None, nullptr, mappedSubResource));

    const auto& textureDescription = pDevice->GetTexture(m_hBackbufferStagingTexture)->GetDescription();
    const auto& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

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
