/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Fence.h>

XII_CREATE_SIMPLE_TEST(Resources, Fence)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Creation and host signaling")
  {
    xiiGALFenceCreationDescription cpuFenceDescription;
    xiiSharedPtr<xiiGALFence>      pCpuFence = environment.GetDevice()->CreateFence(cpuFenceDescription);
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
      generalFenceDescription.m_Type          = xiiGALFenceType::General;
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

XII_CREATE_SIMPLE_TEST_GROUP(FenceSynchronization);
XII_CREATE_SIMPLE_TEST(FenceSynchronization, IndependentCallerAndQueueValues)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto fence = environment.GetDevice()->CreateFence({});
  if (!XII_TEST_BOOL(fence != nullptr))
    return;
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    const auto flags    = (i & 1U) ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics;
    auto*      queue    = environment.GetDevice()->GetCommandQueue(flags);
    auto       commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = flags});
    if (!XII_TEST_BOOL(queue && commands))
      return;
    // Private values exceed both queues' unrelated submission clocks.
    const xiiUInt64 value = 10000 + i;
    commands->Begin();
    commands->EnqueueSignal(fence.Borrow(), value);
    commands->End();
    queue->Submit(commands);
    fence->Wait(value);
    XII_TEST_INT(fence->GetCompletedValue(), value);
  }
}
