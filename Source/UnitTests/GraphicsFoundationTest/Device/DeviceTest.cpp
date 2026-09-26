/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Framework/TestFramework.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/RasterizerState.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_CREATE_SIMPLE_TEST_GROUP(Device);

XII_CREATE_SIMPLE_TEST(Device, Device)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selected implementation matrix")
  {
    XII_TEST_BOOL(xiiGetGPUTestingEnvironmentCount() > 0U);

    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      const xiiStringView sName = xiiGetGPUTestingEnvironmentName(uiImplementation);
      XII_TEST_BOOL(!sName.IsEmpty());
      for (xiiUInt32 uiPrevious = 0; uiPrevious < uiImplementation; ++uiPrevious)
      {
        XII_TEST_BOOL(!sName.IsEqual_NoCase(xiiGetGPUTestingEnvironmentName(uiPrevious)));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Initialization and capabilities")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      const xiiStringView      sName = xiiGetGPUTestingEnvironmentName(uiImplementation);
      xiiGPUTestingEnvironment environment(sName);
      XII_TEST_BOOL(environment.Initialize().Succeeded());

      xiiGALDevice* pDevice = environment.GetDevice();
      XII_TEST_BOOL(pDevice != nullptr);
      if (pDevice == nullptr)
        continue;

      XII_TEST_BOOL(pDevice->GetDebugName().StartsWith("GraphicsFoundationTest"));
      XII_TEST_BOOL(!pDevice->GetGraphicsDeviceAdapterProperties().m_sAdapterName.IsEmpty());
      XII_TEST_BOOL(!pDevice->GetGraphicsDeviceAdapterProperties().m_CommandQueueProperties.IsEmpty());
      XII_TEST_BOOL(pDevice->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension > 0U);
      XII_TEST_BOOL(pDevice->GetGraphicsDeviceAdapterProperties().m_BufferProperties.m_uiConstantBufferAlignment > 0U);

      if (sName.IsEqual_NoCase("Vulkan"))
        XII_TEST_BOOL(pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan);
      if (sName.IsEqual_NoCase("D3D12"))
        XII_TEST_BOOL(pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Direct3D12);

      xiiGALCommandQueue* pGraphicsQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pGraphicsQueue != nullptr);
      if (pGraphicsQueue != nullptr)
      {
        XII_TEST_BOOL(pGraphicsQueue->GetDevice() == pDevice);
        XII_TEST_BOOL(pGraphicsQueue->GetDescription().m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics));
        XII_TEST_BOOL(pGraphicsQueue->GetNextFenceValue() > 0U);
      }

      pDevice->BeginFrame();
      pDevice->EndFrame();
      pDevice->WaitIdle();
    }
  }
}
