#include <GraphicsTest/GraphicsTestPCH.h>

XII_CREATE_SIMPLE_TEST_GROUP(Basics);

XII_CREATE_SIMPLE_TEST(Basics, DeviceTest)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Testing Environment")
  {
    xiiStartup::StartupHighLevelSystems();

    auto          pTestingEnvironment = xiiSingletonRegistry::GetSingletonInstance<xiiGPUTestingEnvironmentInterface>();
    xiiGALDevice* pDevice             = pTestingEnvironment->GetDevice();
    XII_TEST_BOOL(pDevice != nullptr);

    xiiWindow* pWindow = pTestingEnvironment->GetWindow();
    XII_TEST_BOOL(pWindow == nullptr);

    xiiGALSwapChainHandle hSwapChain = pTestingEnvironment->GetSwapChainHandle();
    XII_TEST_BOOL(hSwapChain.IsInvalidated());

    pTestingEnvironment->CreateWindow(960, 540).IgnoreResult();

    pWindow = pTestingEnvironment->GetWindow();
    XII_TEST_BOOL(pWindow != nullptr);

    hSwapChain = pTestingEnvironment->GetSwapChainHandle();
    XII_TEST_BOOL(!hSwapChain.IsInvalidated());

    XII_TEST_INT(pWindow->GetClientAreaSize().width, 960U);
    XII_TEST_INT(pWindow->GetClientAreaSize().height, 540U);

    pTestingEnvironment->DestroyWindow();

    pWindow = pTestingEnvironment->GetWindow();
    XII_TEST_BOOL(pWindow == nullptr);

    hSwapChain = pTestingEnvironment->GetSwapChainHandle();
    XII_TEST_BOOL(hSwapChain.IsInvalidated());

    xiiStartup::ShutdownHighLevelSystems();
  }
}
