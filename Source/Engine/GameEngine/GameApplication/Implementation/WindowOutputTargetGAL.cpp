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

  xiiSharedPtr<xiiGALDevice> pDevice           = xiiGALDevice::GetDefaultDevice();
  const bool                 bSwapChainExisted = m_pSwapChain != nullptr;

  if (bSwapChainExisted)
  {
    m_pSwapChain->SetPresentMode(m_PresentMode);
    m_pSwapChain->Resize(m_Size).AssertSuccess("Failed to resize swap chain!");

    if (m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      xiiSizeU32 currentSize = m_pSwapChain->GetCurrentSize();

      m_OnSwapChainChanged(m_pSwapChain, currentSize);
    }
  }
  else
  {
    m_pSwapChain = pDevice->CreateSwapChain(m_CurrentDesc);

    m_pSwapChain->SetPresentMode(m_PresentMode);
  }
}

void xiiWindowOutputTargetGAL::AcquireImage()
{
  if (!m_OnSwapChainChanged.IsValid())
    return;

  xiiEnum<xiiGALPresentMode> presentMode = xiiGameApplication::cvar_AppVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate;

  // Detect window size or vsync mode changes.
  if (m_Size != m_CurrentDesc.m_pWindow->GetClientAreaSize() || m_PresentMode != presentMode)
  {
    CreateSwapchain(m_CurrentDesc);
  }

  // Detect swapchain size changes that happen outside of window events.
  CheckForSwapChainResize();
}

void xiiWindowOutputTargetGAL::PresentImage(bool bEnableVSync)
{
  if (m_pSwapChain == nullptr)
    return;

  m_pSwapChain->SetPresentMode(bEnableVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
  m_pSwapChain->Present();
}

void xiiWindowOutputTargetGAL::CheckForSwapChainResize()
{
  if (!m_pSwapChain)
    return;

  // Query the actual swapchain size
  xiiSizeU32 actualSize = m_pSwapChain->GetCurrentSize();

  // If the swapchain size has changed (e.g., due to OS/driver adjustments)
  if (actualSize != m_Size)
  {
    m_Size = actualSize;

    // Resize the swapchain to match the new size
    m_pSwapChain->Resize(m_Size).AssertSuccess("Failed to resize swap chain!");

    if (m_OnSwapChainChanged.IsValid())
    {
      m_OnSwapChainChanged(m_pSwapChain, m_Size);
    }
  }
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  if (m_pSwapChain == nullptr)
  {
    xiiLog::Error("No swapchain available for image capture.");
    return XII_FAILURE;
  }

  xiiSharedPtr<xiiGALDevice> pDevice        = xiiGALDevice::GetDefaultDevice();
  auto                       pGraphicsQueue = xiiGALDevice::GetDefaultDevice()->GetCommandQueue();

  xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  XII_ASSERT_DEV(pCommandList != nullptr, "Failed to create command list!");

  pCommandList->Begin();
  {
    m_pImageCapture->Capture(m_pSwapChain, pCommandList, m_uiCurrentFrame++);
  }
  pCommandList->End();

  pGraphicsQueue->Submit(pCommandList);

  m_pImageCapture->WaitForCompletedValue();

  while (auto capture = m_pImageCapture->GetCapture())
  {
    const auto& textureDescription = capture.m_pTexture->GetDescription();

    xiiDynamicArray<xiiUInt8> backbufferData;
    backbufferData.SetCountUninitialized(textureDescription.m_Size.width * textureDescription.m_Size.height * 4);

    pCommandList->Begin();
    {
      xiiGALTextureMipLevelData      mipLevelData;
      xiiGALMappedTextureSubresource mappedSubResource;
      pCommandList->MapTextureSubresource(capture.m_pTexture, mipLevelData, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).IgnoreResult();

      const auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

      if (mappedSubResource.m_pData)
      {
        const xiiUInt32 uiStride = 4 * textureDescription.m_Size.width;

        xiiGALTextureUtilities::CopySubresourceToMemory(textureDescription, mappedSubResource, mipLevelData, backbufferData, uiStride);
      }
      else
      {
        xiiLog::Error("Failed to map texture subresource for reading backbuffer data.");
      }

      pCommandList->UnmapTextureSubresource(capture.m_pTexture, mipLevelData).IgnoreResult();
    }
    pCommandList->End();

    pGraphicsQueue->Submit(pCommandList);

    m_pImageCapture->RecycleStagingTexture(std::move(capture.m_pTexture));

    xiiImageHeader header;
    header.SetWidth(textureDescription.m_Size.width);
    header.SetHeight(textureDescription.m_Size.height);
    header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(textureDescription.m_Format, true));
    out_image.ResetAndAlloc(header);
    xiiUInt8* pData = out_image.GetPixelPointer<xiiUInt8>();

    xiiMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
