/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Fence.h>

XII_CREATE_SIMPLE_TEST(Resources, Fence)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Creation and host signaling")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALFenceCreationDescription cpuFenceDescription;
      xiiSharedPtr<xiiGALFence> pCpuFence = environment.GetDevice()->CreateFence(cpuFenceDescription);
      XII_TEST_BOOL(pCpuFence != nullptr);
      if (pCpuFence != nullptr)
      {
        XII_TEST_BOOL(pCpuFence->GetDescription() == cpuFenceDescription);
        XII_TEST_INT(pCpuFence->GetCompletedValue(), 0U);
        XII_TEST_BOOL(pCpuFence->GetDevice().Borrow() == environment.GetDevice());
      }

      if (environment.GetDevice()->GetFeatures().m_NativeFence == xiiGALDeviceFeatureState::Enabled)
      {
        xiiGALFenceCreationDescription generalFenceDescription;
        generalFenceDescription.m_Type = xiiGALFenceType::General;
        xiiSharedPtr<xiiGALFence> pGeneralFence = environment.GetDevice()->CreateFence(generalFenceDescription);
        XII_TEST_BOOL(pGeneralFence != nullptr);
        if (pGeneralFence != nullptr)
        {
          pGeneralFence->Signal(3U);
          pGeneralFence->Wait(3U);
          XII_TEST_INT(pGeneralFence->GetCompletedValue(), 3U);
        }
      }
    }
  }
}
