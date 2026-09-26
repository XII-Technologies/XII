/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

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
}
