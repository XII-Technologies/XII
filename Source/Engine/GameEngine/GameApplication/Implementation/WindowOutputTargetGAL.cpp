/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Tools/ImageCapture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Image.h>

xiiWindowOutputTargetGAL::xiiWindowOutputTargetGAL(const xiiGALSwapChainCreationDescription& description, OnSwapChainChanged onSwapChainChanged) :
  m_OnSwapChainChanged(onSwapChainChanged)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pSwapChain = pDevice->CreateSwapChain(description);
  XII_ASSERT_DEV(m_pSwapChain != nullptr, "Failed to create swap chain.");

  if (const auto* pApplication = xiiGameApplication::GetGameApplicationInstance())
    SetVSyncEnabled(pApplication->GetSettings().m_bVSync);

  m_pImageCapture = XII_DEFAULT_NEW(xiiGALImageCapture, pDevice);
}

xiiWindowOutputTargetGAL::~xiiWindowOutputTargetGAL()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pImageCapture.Clear();
  m_pSwapChain.Clear();

  // After the Swap Chain is destroyed it can still be used in the renderer.
  // As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  pDevice->WaitIdle();
}

bool xiiWindowOutputTargetGAL::GetVSyncEnabled() const
{
  if (!m_pSwapChain)
    return false;

  return m_pSwapChain->GetPresentMode() == xiiGALPresentMode::VSync;
}

void xiiWindowOutputTargetGAL::SetVSyncEnabled(bool bEnableVSync)
{
  if (!m_pSwapChain)
    return;

  m_pSwapChain->SetPresentMode(bEnableVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
}

void xiiWindowOutputTargetGAL::PresentImage()
{
  if (!m_pSwapChain)
    return;

  if (!m_ScreenshotRequests.IsEmpty())
  {
    xiiImage image;
    if (CaptureImage(image).Succeeded())
    {
      for (const auto& path : m_ScreenshotRequests)
        if (image.SaveTo(path).Failed())
          xiiLog::Error("Failed to save screenshot to '{}'.", path);
    }
    else
      xiiLog::Error("Failed to capture the rendered window image.");
    m_ScreenshotRequests.Clear();
  }

  m_pSwapChain->Present();
}

void xiiWindowOutputTargetGAL::RequestScreenshot(xiiStringView sOutputFile)
{
  m_ScreenshotRequests.PushBack(xiiString(sOutputFile));
}

void xiiWindowOutputTargetGAL::Resize(const xiiSizeU32& newSize)
{
  if (!m_pSwapChain)
    return;

  const xiiGALSwapChainCreationDescription& description = m_pSwapChain->GetDescription();

  m_pSwapChain->Resize(newSize, description.m_PreTransform).IgnoreResult();

  if (m_OnSwapChainChanged.IsValid())
  {
    m_OnSwapChainChanged(m_pSwapChain, newSize);
  }
}

xiiResult xiiWindowOutputTargetGAL::CaptureImage(xiiImage& out_image)
{
  if (m_pSwapChain == nullptr)
  {
    xiiLog::Error("No Swap Chain available for image capture.");
    return XII_FAILURE;
  }

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice || !m_pImageCapture)
    return XII_FAILURE;
  auto pGraphicsQueue = pDevice->GetCommandQueue();

  xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!pCommandList || !pGraphicsQueue)
    return XII_FAILURE;

  pCommandList->Begin();
  {
    m_pImageCapture->Capture(m_pSwapChain, pCommandList, m_uiCurrentFrame++);
  }
  pCommandList->End();

  pGraphicsQueue->Submit(pCommandList);

  m_pImageCapture->WaitForCompletedValue();

  bool captured = false;
  while (auto capture = m_pImageCapture->GetCapture())
  {
    const xiiGALTextureCreationDescription& textureDescription = capture.m_pTexture->GetDescription();

    out_image.ResetAndAlloc(textureDescription);

    pCommandList->Begin();
    {
      xiiGALTextureMipLevelData      mipLevelData;
      xiiGALMappedTextureSubresource mappedSubResource;
      if (pCommandList->MapTextureSubresource(capture.m_pTexture, mipLevelData, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).Succeeded() && mappedSubResource.m_pData)
      {
        const auto bytes = out_image.GetByteBlobPtr();
        xiiGALTextureUtilities::CopySubresourceToMemory(textureDescription, mappedSubResource, mipLevelData, xiiMakeArrayPtr(bytes.GetPtr(), static_cast<xiiUInt32>(bytes.GetCount())), static_cast<xiiUInt32>(out_image.GetRowPitch()));
        captured = pCommandList->UnmapTextureSubresource(capture.m_pTexture, mipLevelData).Succeeded();
      }
      else
      {
        xiiLog::Error("Failed to map texture sub-resource for reading back-buffer data.");
      }
    }
    pCommandList->End();

    pGraphicsQueue->Submit(pCommandList);

    m_pImageCapture->RecycleStagingTexture(std::move(capture.m_pTexture));
  }

  return captured ? XII_SUCCESS : XII_FAILURE;
}
