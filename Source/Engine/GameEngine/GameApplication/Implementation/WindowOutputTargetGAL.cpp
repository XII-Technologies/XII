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

  m_pSwapChain->Present();
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

  xiiSharedPtr<xiiGALDevice> pDevice        = xiiGALDevice::GetDefaultDevice();
  auto                       pGraphicsQueue = pDevice->GetCommandQueue();

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
    const xiiGALTextureCreationDescription& textureDescription = capture.m_pTexture->GetDescription();

    xiiTemporaryArray<xiiUInt8> backbufferData;
    backbufferData.SetCountUninitialized(textureDescription.m_Size.width * textureDescription.m_Size.height * 4);

    pCommandList->Begin();
    {
      xiiGALTextureMipLevelData      mipLevelData;
      xiiGALMappedTextureSubresource mappedSubResource;
      pCommandList->MapTextureSubresource(capture.m_pTexture, mipLevelData, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).IgnoreResult();

      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

      if (mappedSubResource.m_pData)
      {
        const xiiUInt32 uiStride = 4 * textureDescription.m_Size.width;

        xiiGALTextureUtilities::CopySubresourceToMemory(textureDescription, mappedSubResource, mipLevelData, backbufferData, uiStride);
      }
      else
      {
        xiiLog::Error("Failed to map texture sub-resource for reading back-buffer data.");
      }

      pCommandList->UnmapTextureSubresource(capture.m_pTexture, mipLevelData).IgnoreResult();
    }
    pCommandList->End();

    pGraphicsQueue->Submit(pCommandList);

    m_pImageCapture->RecycleStagingTexture(std::move(capture.m_pTexture));

    out_image.ResetAndAlloc(textureDescription);
    xiiUInt8* pData = out_image.GetPixelPointer<xiiUInt8>();

    xiiMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
