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

#include <type_traits>

static_assert(!std::is_default_constructible_v<xiiGALDeviceFactory>, "Graphics backend registration must be owned by its subsystem.");

XII_CREATE_SIMPLE_TEST_GROUP(DeviceFactoryLifecycle);

XII_CREATE_SIMPLE_TEST(DeviceFactoryLifecycle, Startup)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subsystem owns graphics backend registration")
  {
    XII_TEST_BOOL(xiiGALDeviceFactory::IsInitialized());
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGALDeviceImplementationDescription>() != nullptr);
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(Device);

XII_CREATE_SIMPLE_TEST(Device, Device)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Initialization and capabilities")
  {
    xiiGALDevice* pDevice = environment.GetDevice();
    XII_TEST_BOOL(pDevice != nullptr);

    XII_TEST_BOOL(pDevice->GetDebugName().StartsWith("GraphicsFoundationTest"));
    XII_TEST_BOOL(!pDevice->GetGraphicsDeviceAdapterProperties().m_sAdapterName.IsEmpty());
    XII_TEST_BOOL(!pDevice->GetGraphicsDeviceAdapterProperties().m_CommandQueueProperties.IsEmpty());
    XII_TEST_BOOL(pDevice->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension > 0U);
    XII_TEST_BOOL(pDevice->GetGraphicsDeviceAdapterProperties().m_BufferProperties.m_uiConstantBufferAlignment > 0U);

    if (pDevice->GetDebugName().IsEqual_NoCase("Vulkan"))
      XII_TEST_BOOL(pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan);
    if (pDevice->GetDebugName().IsEqual_NoCase("D3D12"))
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
