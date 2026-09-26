/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Tools/ImageCapture.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>

xiiGALImageCapture::xiiGALImageCapture(xiiSharedPtr<xiiGALDevice> pDevice) :
  m_pDevice(std::move(pDevice)), m_uiCurrentFenceValue(1U)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Invalid device provided.");

  xiiGALFenceCreationDescription fenceDescription;
  fenceDescription.m_Type = xiiGALFenceType::CpuWaitOnly;
  m_pFence                = m_pDevice->CreateFence(fenceDescription);
  m_pFence->SetDebugName("Image Capture Fence");
}

xiiGALImageCapture::~xiiGALImageCapture() = default;

xiiGALImageCapture::CaptureDescription xiiGALImageCapture::GetCapture()
{
  CaptureDescription captureDescription;

  XII_LOCK(m_PendingTexturesMutex);

  if (!m_PendingTextures.IsEmpty())
  {
    auto&           oldestCapture         = m_PendingTextures.PeekFront();
    const xiiUInt64 uiCompletedFenceValue = m_pFence->GetCompletedValue();

    if (oldestCapture.m_uiFenceValue <= uiCompletedFenceValue)
    {
      // The oldest capture has been completed by the GPU.
      captureDescription.m_pTexture    = std::move(oldestCapture.m_pStagingTexture);
      captureDescription.m_uiTextureID = oldestCapture.m_uiTextureID;

      // Remove the completed capture from the pending list.
      m_PendingTextures.PopFront();
    }
  }
  return captureDescription;
}

bool xiiGALImageCapture::HasCapture()
{
  XII_LOCK(m_PendingTexturesMutex);

  if (!m_PendingTextures.IsEmpty())
  {
    // Check if the oldest capture has been completed by the GPU.
    const auto&     oldestCapture         = m_PendingTextures.PeekFront();
    const xiiUInt64 uiCompletedFenceValue = m_pFence->GetCompletedValue();

    return oldestCapture.m_uiFenceValue <= uiCompletedFenceValue;
  }
  return false;
}

void xiiGALImageCapture::Capture(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt32 uiFrameIndex)
{
  XII_ASSERT_DEV(pSwapChain != nullptr, "Invalid swapchain provided.");
  XII_ASSERT_DEV(pCommandList != nullptr, "Invalid commandlist provided.");

  const xiiGALSwapChainCreationDescription& swapchainDescription = pSwapChain->GetDescription();
  const xiiSizeU32                          swapchainSize        = pSwapChain->GetCurrentSize();

  xiiGALScopedDebugGroup debugGroup(pCommandList, "Image Capture");

  xiiSharedPtr<xiiGALTexture> pStagingTexture;

  // Check if we have a staging texture available that matches the back buffer texture description.
  {
    XII_LOCK(m_AvailableTexturesMutex);

    while (!m_AvailableTextures.IsEmpty() && !pStagingTexture)
    {
      pStagingTexture = std::move(m_AvailableTextures.PeekBack());

      m_AvailableTextures.PopBack();

      const xiiGALTextureCreationDescription& textureDescription = pStagingTexture->GetDescription();

      // Check if the staging texture matches the back buffer texture description.
      if (textureDescription.m_Size != swapchainSize || textureDescription.m_Format != swapchainDescription.m_ColorBufferFormat)
      {
        // The staging texture does not match the back buffer, so we discard it.
        pStagingTexture.Clear();
      }
    }
  }

  // Create a staging texture if we don't have one available.
  if (!pStagingTexture)
  {
    xiiGALTextureCreationDescription stagingTextureDescription;
    stagingTextureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
    stagingTextureDescription.m_Size               = swapchainSize;
    stagingTextureDescription.m_uiArraySizeOrDepth = 1U;
    stagingTextureDescription.m_Format             = swapchainDescription.m_ColorBufferFormat;
    stagingTextureDescription.m_uiMipLevels        = 1U;
    stagingTextureDescription.m_uiSampleCount      = 1U;
    stagingTextureDescription.m_BindFlags          = xiiGALBindFlags::None;
    stagingTextureDescription.m_Usage              = xiiGALResourceUsage::Staging;
    stagingTextureDescription.m_CPUAccessFlags     = xiiGALCPUAccessFlag::Read;
    pStagingTexture                                = m_pDevice->CreateTexture(stagingTextureDescription);

    pStagingTexture->SetDebugName("Image Capture Staging Texture");
  }

  // Copy the back buffer to the staging texture.
  pCommandList->CopyTexture(pSwapChain->GetBackBufferTexture(), pStagingTexture);

  // Signal the fence after the copy operation.
  pCommandList->EnqueueSignal(m_pFence, m_uiCurrentFenceValue);

  // Store the capture information for later retrieval.
  {
    XII_LOCK(m_PendingTexturesMutex);

    m_PendingTextures.PushBack(PendingTextureDescription(std::move(pStagingTexture), uiFrameIndex, m_uiCurrentFenceValue++));
  }
}

void xiiGALImageCapture::WaitForCompletedValue()
{
  if (m_uiCurrentFenceValue > 1U)
  {
    m_pFence->Wait(m_uiCurrentFenceValue - 1U);
  }
}

void xiiGALImageCapture::RecycleStagingTexture(xiiSharedPtr<xiiGALTexture>&& pStagingTexture)
{
  XII_LOCK(m_AvailableTexturesMutex);

  m_AvailableTextures.PushBack(std::move(pStagingTexture));
}
