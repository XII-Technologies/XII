/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
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
    XII_TEST_BOOL(xiiGetStaticRTTI<xiiGALDeviceImplementationDescription>() != nullptr);
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(Device);

XII_CREATE_SIMPLE_TEST(Device, MemoryHeapStatistics)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_BOOL(environment.Initialize().Succeeded()))
    return;

  auto* pDevice = environment.GetDevice();
  if (pDevice->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan)
    return;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Memory is accounted until allocator reclamation")
  {
    auto getAllocatedBytes = [pDevice]() {
      xiiHybridArray<xiiGALMemoryHeapStatistics, 16> heaps;
      pDevice->GetMemoryHeapStatistics(heaps);
      XII_TEST_BOOL(!heaps.IsEmpty());
      xiiUInt64 uiAllocatedBytes = 0U;
      for (const auto& heap : heaps)
      {
        XII_TEST_BOOL(heap.m_uiBudgetBytes > 0U && heap.m_uiBudgetBytes <= heap.m_uiCapacityBytes);
        XII_TEST_BOOL(heap.m_uiAllocatedBytes <= heap.m_uiCommittedBytes);
        uiAllocatedBytes += heap.m_uiAllocatedBytes;
      }
      return uiAllocatedBytes;
    };

    pDevice->BeginFrame();
    const xiiUInt64                 uiBaseline = getAllocatedBytes();
    xiiGALBufferCreationDescription description;
    description.m_uiSize    = 1024U * 1024U;
    description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
    auto pBuffer            = pDevice->CreateBuffer(description);
    if (XII_TEST_BOOL(pBuffer != nullptr))
    {
      const xiiUInt64 uiAllocated = getAllocatedBytes();
      XII_TEST_BOOL(uiAllocated >= uiBaseline + description.m_uiSize);
      pBuffer.Clear();
      XII_TEST_BOOL(getAllocatedBytes() == uiAllocated);
    }
    pDevice->EndFrame();
    pDevice->WaitIdle();
    XII_TEST_BOOL(getAllocatedBytes() == uiBaseline);
  }
}

XII_CREATE_SIMPLE_TEST(Device, DeferredDestructionWithoutFutureSubmissions)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;

  auto* pDevice = environment.GetDevice();
  if (pDevice->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan)
    return;

  xiiGALMemoryCategoryStatistics baseline;
  if (!XII_TEST_BOOL(pDevice->GetMemoryCategoryStatistics(xiiGALMemoryCategory::Readback, baseline)))
    return;

  // First retire an unused allocation without submitting to any queue. Then exercise
  // completed graphics work while the compute and transfer queues remain unused.
  for (xiiUInt32 uiFrame = 0U; uiFrame < 8U; ++uiFrame)
  {
    pDevice->BeginFrame();
    xiiGALBufferCreationDescription description;
    description.m_uiSize         = 1024U * 1024U;
    description.m_BindFlags      = xiiGALBindFlags::VertexBuffer;
    description.m_MemoryCategory = xiiGALMemoryCategory::Readback;
    auto pBuffer                 = pDevice->CreateBuffer(description);
    XII_TEST_BOOL(pBuffer != nullptr);

    if (pBuffer && uiFrame != 0U)
    {
      auto pCommands = pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      pCommands->Begin();
      xiiGALStateTransitionDescription transition;
      transition.m_pResource       = pBuffer;
      transition.m_NewState        = xiiGALResourceStateFlags::VertexBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      pCommands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
      pCommands->End();
      auto* pQueue = pDevice->GetCommandQueue();
      pQueue->WaitForFenceValue(pQueue->Submit(pCommands));
    }

    xiiGALMemoryCategoryStatistics allocated;
    XII_TEST_BOOL(pDevice->GetMemoryCategoryStatistics(xiiGALMemoryCategory::Readback, allocated));
    pBuffer.Clear();
    xiiGALMemoryCategoryStatistics retiring;
    XII_TEST_BOOL(pDevice->GetMemoryCategoryStatistics(xiiGALMemoryCategory::Readback, retiring));
    XII_TEST_BOOL(retiring.m_uiAllocatedBytes == allocated.m_uiAllocatedBytes);
    XII_TEST_BOOL(retiring.m_uiRetiringBytes >= baseline.m_uiRetiringBytes + description.m_uiSize);

    // Reclamation must not require another submission or a device-wide idle wait.
    pDevice->EndFrame();
    xiiGALMemoryCategoryStatistics reclaimed;
    XII_TEST_BOOL(pDevice->GetMemoryCategoryStatistics(xiiGALMemoryCategory::Readback, reclaimed));
    XII_TEST_BOOL(reclaimed.m_uiAllocatedBytes == baseline.m_uiAllocatedBytes);
    XII_TEST_BOOL(reclaimed.m_uiRetiringBytes == baseline.m_uiRetiringBytes);
    XII_TEST_INT(reclaimed.m_uiAllocationCount, baseline.m_uiAllocationCount);
  }
}

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
    {
      XII_TEST_BOOL(pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan);
    }
    if (pDevice->GetDebugName().IsEqual_NoCase("D3D12"))
    {
      XII_TEST_BOOL(pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Direct3D12);
    }

    xiiGALCommandQueue* pGraphicsQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    XII_TEST_BOOL(pGraphicsQueue != nullptr);
    XII_TEST_BOOL(pGraphicsQueue->GetDevice() == pDevice);
    XII_TEST_BOOL(pGraphicsQueue->GetDescription().m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics));
    XII_TEST_BOOL(pGraphicsQueue->GetNextFenceValue() > 0U);

    pDevice->BeginFrame();
    pDevice->EndFrame();
    pDevice->WaitIdle();
  }
}
