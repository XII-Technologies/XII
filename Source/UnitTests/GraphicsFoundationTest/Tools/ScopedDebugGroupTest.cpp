/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>

XII_CREATE_SIMPLE_TEST(Tools, ScopedDebugGroup)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RAII, null construction, moves, and macros")
  {
    xiiGALScopedDebugGroup emptyGroup;
    xiiGALScopedDebugGroup nullGroup(nullptr, "Ignored null command list");

    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALDevice*       pDevice = environment.GetDevice();
      xiiGALCommandQueue* pQueue  = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue == nullptr)
        continue;

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags          = xiiGALCommandQueueFlags::Graphics;
      xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      {
        xiiGALScopedDebugGroup outer(*pCommandList, "Outer", xiiColor::CornflowerBlue);
        xiiGALScopedDebugGroup moved(std::move(outer));

        xiiGALScopedDebugGroup first(pCommandList.Borrow(), "Move assignment destination");
        xiiGALScopedDebugGroup second(pCommandList.Borrow(), "Move assignment source");
        first = std::move(second);
        first = std::move(first);

        XII_COMMANDLIST_SCOPE(pCommandList.Borrow(), "Macro scope");
        XII_COMMANDLIST_SCOPE_COLOR(pCommandList.Borrow(), "Colored macro scope", xiiColor::Orange);
      }
      pCommandList->End();

      const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
      XII_TEST_BOOL(uiFenceValue > 0U);
      pQueue->WaitForFenceValue(uiFenceValue);
    }
  }
}
