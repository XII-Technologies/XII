/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

XII_CREATE_SIMPLE_TEST_GROUP(CommandEncoder);

XII_CREATE_SIMPLE_TEST(CommandEncoder, CommandQueue)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Submission fence progression")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALCommandQueue* pQueue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue == nullptr)
        continue;

      const xiiUInt64 uiNextBeforeSubmit      = pQueue->GetNextFenceValue();
      const xiiUInt64 uiCompletedBeforeSubmit = pQueue->GetCompletedFenceValue();
      XII_TEST_BOOL(uiNextBeforeSubmit > uiCompletedBeforeSubmit);

      xiiGALCommandListCreationDescription description;
      description.m_QueueFlags                     = xiiGALCommandQueueFlags::Graphics;
      description.m_Flags                          = xiiGALCommandListFlags::MultiSubmit;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(description);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      pCommandList->InsertDebugLabel("Queue fence test");
      pCommandList->End();

      const xiiUInt64 uiFirstFence  = pQueue->Submit(pCommandList.Borrow());
      const xiiUInt64 uiSecondFence = pQueue->Submit(pCommandList.Borrow());
      XII_TEST_BOOL(uiFirstFence >= uiNextBeforeSubmit);
      XII_TEST_BOOL(uiSecondFence > uiFirstFence);
      XII_TEST_INT(static_cast<xiiUInt32>(pCommandList->GetRecordingState()), static_cast<xiiUInt32>(xiiGALCommandList::RecordingState::Ended));

      pQueue->WaitForFenceValue(uiSecondFence);
      XII_TEST_BOOL(pQueue->GetCompletedFenceValue() >= uiSecondFence);
      XII_TEST_BOOL(pQueue->GetNextFenceValue() > uiSecondFence);
      pCommandList->Reset();
    }
  }
}
