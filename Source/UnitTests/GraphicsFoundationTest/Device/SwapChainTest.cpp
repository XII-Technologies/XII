/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Device/SwapChain.h>

namespace
{
  class TestSwapChain final : public xiiGALSwapChain
  {
  public:
    TestSwapChain(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALSwapChainCreationDescription& description) :
      xiiGALSwapChain(std::move(pDevice), description)
    {
      m_CurrentSize = xiiSizeU32(320U, 180U);
    }

    virtual void Present() override { ++m_uiPresentCount; }

    virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform) override
    {
      m_CurrentSize             = newSize;
      m_DesiredSurfaceTransform = newTransform;
      return XII_SUCCESS;
    }

    xiiUInt32 m_uiPresentCount = 0U;

  protected:
    virtual xiiResult InitPlatform() override { return XII_SUCCESS; }
  };
}

XII_CREATE_SIMPLE_TEST(Device, SwapChain)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Description defaults and base contract")
  {
    xiiGALSwapChainCreationDescription description;
    XII_TEST_BOOL(description.m_pWindow == nullptr);
    XII_TEST_BOOL(description.m_ColorBufferFormat == xiiGALResourceFormat::RGBA8UNormalizedSRGB);
    XII_TEST_BOOL(description.m_UsageFlags == xiiGALSwapChainUsageFlags::RenderTarget);
    XII_TEST_BOOL(description.m_PreTransform == xiiGALSurfaceTransform::Optimal);
    XII_TEST_INT(description.m_uiBufferCount, 2U);
    XII_TEST_FLOAT(description.m_fDefaultDepthValue, 1.0f, 0.0f);
    XII_TEST_INT(description.m_uiDefaultStencilValue, 0U);

    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      TestSwapChain swapChain(environment.GetDeviceShared(), description);
      XII_TEST_BOOL(swapChain.GetDescription() == description);
      XII_TEST_BOOL(swapChain.GetCurrentSize() == xiiSizeU32(320U, 180U));
      XII_TEST_BOOL(swapChain.GetPresentMode() == xiiGALPresentMode::VSync);
      swapChain.SetPresentMode(xiiGALPresentMode::Immediate);
      XII_TEST_BOOL(swapChain.GetPresentMode() == xiiGALPresentMode::Immediate);
      XII_TEST_BOOL(swapChain.Resize(xiiSizeU32(640U, 360U), xiiGALSurfaceTransform::Identity).Succeeded());
      XII_TEST_BOOL(swapChain.GetCurrentSize() == xiiSizeU32(640U, 360U));
      swapChain.Present();
      XII_TEST_INT(swapChain.m_uiPresentCount, 1U);
    }
  }
}
