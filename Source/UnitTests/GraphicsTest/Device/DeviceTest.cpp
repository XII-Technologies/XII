#include <GraphicsTest/GraphicsTestPCH.h>

#include <GraphicsTest/TestingEnvironment/TestingEnvironment.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <TestFramework/Framework/TestFramework.h>

XII_CREATE_SIMPLE_TEST_GROUP(Device);

XII_CREATE_SIMPLE_TEST(Device, Basics)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create and initialize device")
  {
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill = xiiGALDeviceFeatureState::Optional;

    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");

    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_TEST_BOOL(pDevice != nullptr);
    if (pDevice)
    {
      XII_TEST_RESULT(pDevice->Initialize());
      pDevice.Clear();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create window and swapchain via testing environment")
  {
    xiiUniquePtr<xiiGPUTestingEnvironmentVulkan> pEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);

    XII_TEST_RESULT(pEnvironment->Initialize());

    XII_TEST_RESULT(pEnvironment->CreateWindow(128, 128));

    XII_TEST_RESULT(pEnvironment->CreateSwapChainForWindow(128, 128));

    XII_TEST_BOOL(pEnvironment->GetSwapChain() != nullptr);
    XII_TEST_BOOL(pEnvironment->GetDepthStencilTexture() != nullptr);

    // Basic frame operations
    pEnvironment->BeginFrame();
    pEnvironment->EndFrame();
    pEnvironment->Present();

    pEnvironment->DestroySwapChain();
    pEnvironment->DestroyWindow();
    pEnvironment->Shutdown();

    pEnvironment.Clear();
  }
}
