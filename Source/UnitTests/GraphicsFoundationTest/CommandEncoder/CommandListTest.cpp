/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

XII_CREATE_SIMPLE_TEST(CommandEncoder, CommandList)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Record, submit, and read back buffer commands")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALDevice* pDevice = environment.GetDevice();
      xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue == nullptr)
        continue;

      xiiGALBufferCreationDescription sourceDescription;
      sourceDescription.m_uiSize    = 64U;
      sourceDescription.m_Usage     = xiiGALResourceUsage::Mutable;
      sourceDescription.m_BindFlags = xiiGALBindFlags::VertexBuffer;
      xiiSharedPtr<xiiGALBuffer> pSource = pDevice->CreateBuffer(sourceDescription);

      xiiGALBufferCreationDescription readbackDescription;
      readbackDescription.m_uiSize         = 64U;
      readbackDescription.m_Usage          = xiiGALResourceUsage::Staging;
      readbackDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
      xiiSharedPtr<xiiGALBuffer> pReadback = pDevice->CreateBuffer(readbackDescription);
      XII_TEST_BOOL(pSource != nullptr);
      XII_TEST_BOOL(pReadback != nullptr);
      if (pSource == nullptr || pReadback == nullptr)
        continue;

      xiiUInt8 sourceData[64];
      for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(sourceData); ++i)
      {
        sourceData[i] = static_cast<xiiUInt8>((i * 37U + 11U) & 0xFFU);
      }

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      XII_TEST_BOOL(pCommandList->GetDescription() == commandListDescription);
      XII_TEST_INT(static_cast<xiiUInt32>(pCommandList->GetRecordingState()), static_cast<xiiUInt32>(xiiGALCommandList::RecordingState::Reset));
      pCommandList->Begin();
      XII_TEST_INT(static_cast<xiiUInt32>(pCommandList->GetRecordingState()), static_cast<xiiUInt32>(xiiGALCommandList::RecordingState::Recording));
      pCommandList->BeginDebugGroup("GraphicsFoundationTest command recording", xiiColor::CornflowerBlue);
      pCommandList->InsertDebugLabel("Buffer upload and readback");
      pCommandList->UpdateBuffer(pSource.Borrow(), 0U, xiiMakeArrayPtr(sourceData));
      pCommandList->CopyBuffer(pSource.Borrow(), pReadback.Borrow());
      pCommandList->EndDebugGroup();
      pCommandList->End();

      XII_TEST_INT(static_cast<xiiUInt32>(pCommandList->GetRecordingState()), static_cast<xiiUInt32>(xiiGALCommandList::RecordingState::Ended));
      XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiUpdateBuffer, 1U);
      XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiCopyBuffer, 1U);

      const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
      XII_TEST_BOOL(uiFenceValue > 0U);
      pQueue->WaitForFenceValue(uiFenceValue);
      XII_TEST_BOOL(pQueue->GetCompletedFenceValue() >= uiFenceValue);
      XII_TEST_INT(static_cast<xiiUInt32>(pCommandList->GetRecordingState()), static_cast<xiiUInt32>(xiiGALCommandList::RecordingState::Reset));

      xiiSharedPtr<xiiGALCommandList> pMapCommandList = pDevice->CreateCommandList(commandListDescription);
      pMapCommandList->Begin();
      void* pMappedData = nullptr;
      const xiiResult mapResult = pMapCommandList->MapBuffer(pReadback.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pMappedData);
      XII_TEST_BOOL(mapResult.Succeeded());
      XII_TEST_BOOL(pMappedData != nullptr);
      if (pMappedData != nullptr)
      {
        XII_TEST_BOOL(xiiMemoryUtils::IsEqual(static_cast<const xiiUInt8*>(pMappedData), sourceData, XII_ARRAY_SIZE(sourceData)));
      }
      if (mapResult.Succeeded())
      {
        XII_TEST_BOOL(pMapCommandList->UnmapBuffer(pReadback.Borrow(), xiiGALMapType::Read).Succeeded());
      }
      pMapCommandList->End();
    }
  }
}
