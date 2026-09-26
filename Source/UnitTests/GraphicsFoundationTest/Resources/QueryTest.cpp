/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Query.h>

XII_CREATE_SIMPLE_TEST(Resources, Query)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Supported query objects")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      struct QueryFeature
      {
        xiiGALQueryType::Enum m_Type;
        xiiEnum<xiiGALDeviceFeatureState> xiiGALDeviceFeatures::*m_pFeature;
      };

      const QueryFeature queryFeatures[] = {
        {xiiGALQueryType::Occlusion, &xiiGALDeviceFeatures::m_OcclusionQueries},
        {xiiGALQueryType::BinaryOcclusion, &xiiGALDeviceFeatures::m_BinaryOcclusionQueries},
        {xiiGALQueryType::Timestamp, &xiiGALDeviceFeatures::m_TimestampQueries},
        {xiiGALQueryType::PipelineStatistics, &xiiGALDeviceFeatures::m_PipelineStatisticsQueries},
        {xiiGALQueryType::Duration, &xiiGALDeviceFeatures::m_DurationQueries},
      };

      for (const QueryFeature& queryFeature : queryFeatures)
      {
        if (environment.GetDevice()->GetFeatures().*queryFeature.m_pFeature != xiiGALDeviceFeatureState::Enabled)
          continue;

        xiiGALQueryCreationDescription description;
        description.m_Type = queryFeature.m_Type;
        xiiSharedPtr<xiiGALQuery> pQuery = environment.GetDevice()->CreateQuery(description);
        XII_TEST_BOOL(pQuery != nullptr);
        if (pQuery != nullptr)
        {
          XII_TEST_BOOL(pQuery->GetDescription() == description);
          XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Inactive));
          pQuery->Invalidate();
          XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Inactive));
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Record, submit, retrieve, and automatically invalidate queries")
  {
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

      struct QueryFeature
      {
        xiiGALQueryType::Enum m_Type;
        xiiEnum<xiiGALDeviceFeatureState> xiiGALDeviceFeatures::*m_pFeature;
      };

      const QueryFeature queryFeatures[] = {
        {xiiGALQueryType::Occlusion, &xiiGALDeviceFeatures::m_OcclusionQueries},
        {xiiGALQueryType::BinaryOcclusion, &xiiGALDeviceFeatures::m_BinaryOcclusionQueries},
        {xiiGALQueryType::Timestamp, &xiiGALDeviceFeatures::m_TimestampQueries},
        {xiiGALQueryType::PipelineStatistics, &xiiGALDeviceFeatures::m_PipelineStatisticsQueries},
        {xiiGALQueryType::Duration, &xiiGALDeviceFeatures::m_DurationQueries},
      };

      xiiDynamicArray<xiiSharedPtr<xiiGALQuery>> queries;
      for (const QueryFeature& queryFeature : queryFeatures)
      {
        if (pDevice->GetFeatures().*queryFeature.m_pFeature != xiiGALDeviceFeatureState::Enabled)
          continue;

        xiiGALQueryCreationDescription queryDescription;
        queryDescription.m_Type = queryFeature.m_Type;
        xiiSharedPtr<xiiGALQuery> pQuery = pDevice->CreateQuery(queryDescription);
        XII_TEST_BOOL(pQuery != nullptr);
        if (pQuery != nullptr)
          queries.PushBack(std::move(pQuery));
      }

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      for (const xiiSharedPtr<xiiGALQuery>& pQuery : queries)
      {
        if (pQuery->GetDescription().m_Type != xiiGALQueryType::Timestamp)
        {
          pCommandList->BeginQuery(pQuery.Borrow());
          XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Querying));
        }

        pCommandList->InsertDebugLabel("Query workload boundary");
        pCommandList->EndQuery(pQuery.Borrow());
        XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Ended));
      }
      pCommandList->End();

      xiiUInt32 uiExpectedBeginCount = 0U;
      for (const xiiSharedPtr<xiiGALQuery>& pQuery : queries)
        uiExpectedBeginCount += pQuery->GetDescription().m_Type != xiiGALQueryType::Timestamp ? 1U : 0U;
      XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiBeginQuery, uiExpectedBeginCount);

      const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
      XII_TEST_BOOL(uiFenceValue > 0U);
      pQueue->WaitForFenceValue(uiFenceValue);

      for (const xiiSharedPtr<xiiGALQuery>& pQuery : queries)
      {
        XII_TEST_BOOL(pQuery->GetData(nullptr, 0U, false));
        XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Ended));

        bool bResultAvailable = false;
        switch (pQuery->GetDescription().m_Type)
        {
          case xiiGALQueryType::Occlusion:
          {
            xiiGALQueryDataOcclusion data;
            bResultAvailable = pQuery->GetData(&data, sizeof(data));
          }
          break;
          case xiiGALQueryType::BinaryOcclusion:
          {
            xiiGALQueryDataBinaryOcclusion data;
            bResultAvailable = pQuery->GetData(&data, sizeof(data));
          }
          break;
          case xiiGALQueryType::Timestamp:
          {
            xiiGALQueryDataTimestamp data;
            bResultAvailable = pQuery->GetData(&data, sizeof(data));
            XII_TEST_BOOL(data.m_uiFrequency > 0U);
          }
          break;
          case xiiGALQueryType::PipelineStatistics:
          {
            xiiGALQueryDataPipelineStatistics data;
            bResultAvailable = pQuery->GetData(&data, sizeof(data));
          }
          break;
          case xiiGALQueryType::Duration:
          {
            xiiGALQueryDataDuration data;
            bResultAvailable = pQuery->GetData(&data, sizeof(data));
            XII_TEST_BOOL(data.m_uiFrequency > 0U);
          }
          break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }

        XII_TEST_BOOL(bResultAvailable);
        XII_TEST_INT(static_cast<xiiUInt32>(pQuery->GetQueryState()), static_cast<xiiUInt32>(xiiGALQuery::QueryState::Inactive));
      }
    }
  }
}
