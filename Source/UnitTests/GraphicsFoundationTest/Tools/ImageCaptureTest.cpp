/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/ImageCapture.h>

namespace
{
  class CaptureSwapChain final : public xiiGALSwapChain
  {
  public:
    CaptureSwapChain(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALSwapChainCreationDescription& description, xiiSizeU32 size) :
      xiiGALSwapChain(pDevice, description)
    {
      m_CurrentSize = size;

      xiiGALTextureCreationDescription textureDescription;
      textureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
      textureDescription.m_Size               = size;
      textureDescription.m_uiArraySizeOrDepth = 1U;
      textureDescription.m_Format             = description.m_ColorBufferFormat;
      textureDescription.m_uiMipLevels        = 1U;
      textureDescription.m_uiSampleCount      = 1U;
      textureDescription.m_BindFlags          = xiiGALBindFlags::RenderTarget;
      textureDescription.m_Usage              = xiiGALResourceUsage::Mutable;
      m_pBackBufferTexture                    = pDevice->CreateTexture(textureDescription);
    }

    virtual void Present() override {}

    virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform>) override
    {
      m_CurrentSize = newSize;
      return XII_SUCCESS;
    }

  protected:
    virtual xiiResult InitPlatform() override { return XII_SUCCESS; }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Tools, ImageCapture)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy, fence completion, retrieval, and recycle")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALImageCapture capture(environment.GetDeviceShared());
      XII_TEST_BOOL(capture.GetDevice().Borrow() == environment.GetDevice());
      XII_TEST_BOOL(!capture.HasCapture());
      XII_TEST_INT(capture.GetPendingCaptureCount(), 0U);
      XII_TEST_BOOL(!static_cast<bool>(capture.GetCapture()));

      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_ColorBufferFormat  = xiiGALResourceFormat::RGBA8UNormalized;
      xiiSharedPtr<CaptureSwapChain> pSwapChain = XII_DEFAULT_NEW(CaptureSwapChain, environment.GetDeviceShared(), swapChainDescription, xiiSizeU32(8U, 8U));
      XII_TEST_BOOL(pSwapChain->GetBackBufferTexture() != nullptr);

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags          = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      capture.Capture(pSwapChain, pCommandList, 17U);
      pCommandList->End();
      XII_TEST_INT(capture.GetPendingCaptureCount(), 1U);
      XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiCopyTexture, 1U);

      xiiGALCommandQueue* pQueue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue == nullptr)
        continue;

      const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
      pQueue->WaitForFenceValue(uiFenceValue);
      capture.WaitForCompletedValue();
      XII_TEST_BOOL(capture.HasCapture());

      xiiGALImageCapture::CaptureDescription result = capture.GetCapture();
      XII_TEST_BOOL(static_cast<bool>(result));
      XII_TEST_INT(result.m_uiTextureID, 17U);
      XII_TEST_INT(capture.GetPendingCaptureCount(), 0U);
      XII_TEST_BOOL(result.m_pTexture->GetDescription().m_Usage == xiiGALResourceUsage::Staging);
      XII_TEST_BOOL(result.m_pTexture->GetDescription().m_CPUAccessFlags == xiiGALCPUAccessFlag::Read);
      XII_TEST_BOOL(result.m_pTexture->GetDescription().m_Size == xiiSizeU32(8U, 8U));

      capture.RecycleStagingTexture(std::move(result.m_pTexture));
      XII_TEST_BOOL(result.m_pTexture == nullptr);
    }
  }
}
