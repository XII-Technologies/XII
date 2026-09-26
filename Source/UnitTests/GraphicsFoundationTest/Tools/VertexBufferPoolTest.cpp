/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/VertexBufferPool.h>

XII_CREATE_SIMPLE_TEST(Tools, VertexBufferPool)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Chunk growth, updates, and reset")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALVertexBufferPool<xiiVec3, xiiMutex> pool(environment.GetDeviceShared(), "Unit Test Vertex Pool", xiiFoundation::GetDefaultAllocator(), 4U, 2U);

      const auto first  = pool.Allocate(3U);
      const auto second = pool.Allocate(2U);
      const auto third  = pool.Allocate(2U);
      XII_TEST_INT(first.m_uiChunkIndex, 0U);
      XII_TEST_INT(first.m_uiOffset, 0U);
      XII_TEST_INT(second.m_uiChunkIndex, 1U);
      XII_TEST_INT(second.m_uiOffset, 0U);
      XII_TEST_INT(third.m_uiChunkIndex, 1U);
      XII_TEST_INT(third.m_uiOffset, 2U);
      XII_TEST_BOOL(pool.GetGPUBuffer(first) != nullptr);
      XII_TEST_BOOL(pool.GetGPUBuffer(second) != nullptr);
      XII_TEST_BOOL(pool.GetGPUBuffer(first) != pool.GetGPUBuffer(second));

      const auto statistics = pool.GetUsageStatistics();
      XII_TEST_INT(statistics.m_uiTotalCapacity, 12U);
      XII_TEST_INT(statistics.m_uiUsageCount, 7U);
      XII_TEST_INT(statistics.m_uiAllocationCount, 3U);

      const xiiVec3 vertices[] = {
        xiiVec3(1.0f, 2.0f, 3.0f),
        xiiVec3(4.0f, 5.0f, 6.0f),
        xiiVec3(7.0f, 8.0f, 9.0f),
      };

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      pool.Update(first, xiiMakeArrayPtr(vertices), pCommandList);
      pCommandList->End();
      XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiUpdateBuffer, 1U);

      xiiGALCommandQueue* pQueue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue != nullptr)
      {
        const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
        pQueue->WaitForFenceValue(uiFenceValue);
      }

      const xiiArrayPtr<const xiiVec3> storedVertices = pool.GetAllocationPointer(first);
      XII_TEST_INT(storedVertices.GetCount(), 3U);
      for (xiiUInt32 i = 0; i < storedVertices.GetCount(); ++i)
      {
        XII_TEST_VEC3(storedVertices[i], vertices[i], 0.0f);
      }

      pool.Reset();
      const auto resetStatistics = pool.GetUsageStatistics();
      XII_TEST_INT(resetStatistics.m_uiTotalCapacity, 4U);
      XII_TEST_INT(resetStatistics.m_uiUsageCount, 0U);
      XII_TEST_INT(resetStatistics.m_uiAllocationCount, 0U);
    }
  }
}
