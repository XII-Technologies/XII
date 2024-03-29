
#if 0
XII_ALWAYS_INLINE bool xiiGALQueryD3D11::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  switch (m_Description.m_Type)
  {
    case xiiGALQueryType::Occlusion:
    {
      Diligent::QueryDataOcclusion yieldOcclusionData;

      if (m_pQuery->GetData(&yieldOcclusionData, sizeof(yieldOcclusionData), bAutoInvalidate))
      {
        xiiGALQueryDataOcclusion& occlusionData = *reinterpret_cast<xiiGALQueryDataOcclusion*>(pData);
        occlusionData.m_Type                    = xiiGALQueryType::Occlusion;
        occlusionData.m_uiNumSamples            = yieldOcclusionData.NumSamples;

        return true;
      }
    }
    break;
    case xiiGALQueryType::BinaryOcclusion:
    {
      Diligent::QueryDataBinaryOcclusion yieldBinaryOcclusionData;

      if (m_pQuery->GetData(&yieldBinaryOcclusionData, sizeof(yieldBinaryOcclusionData), bAutoInvalidate))
      {
        xiiGALQueryDataBinaryOcclusion& binaryOcclusionData = *reinterpret_cast<xiiGALQueryDataBinaryOcclusion*>(pData);
        binaryOcclusionData.m_Type                          = xiiGALQueryType::BinaryOcclusion;
        binaryOcclusionData.m_bAnySamplesPassed             = yieldBinaryOcclusionData.AnySamplePassed;

        return true;
      }
    }
    break;
    case xiiGALQueryType::Timestamp:
    {
      Diligent::QueryDataTimestamp yieldTimestamp;

      if (m_pQuery->GetData(&yieldTimestamp, sizeof(yieldTimestamp), bAutoInvalidate))
      {
        xiiGALQueryDataTimestamp& timestampData = *reinterpret_cast<xiiGALQueryDataTimestamp*>(pData);
        timestampData.m_Type                    = xiiGALQueryType::Timestamp;
        timestampData.m_uiCounter               = yieldTimestamp.Counter;
        timestampData.m_uiFrequency             = yieldTimestamp.Frequency;

        return true;
      }
    }
    break;
    case xiiGALQueryType::PipelineStatistics:
    {
      Diligent::QueryDataPipelineStatistics yieldPipelineStatistics;

      if (m_pQuery->GetData(&yieldPipelineStatistics, sizeof(yieldPipelineStatistics), bAutoInvalidate))
      {
        xiiGALQueryDataPipelineStatistics& pipelineStatistics = *reinterpret_cast<xiiGALQueryDataPipelineStatistics*>(pData);
        pipelineStatistics.m_Type                             = xiiGALQueryType::PipelineStatistics;
        pipelineStatistics.m_uiInputVertices                  = yieldPipelineStatistics.InputVertices;
        pipelineStatistics.m_uiInputPrimitives                = yieldPipelineStatistics.InputPrimitives;
        pipelineStatistics.m_uiGSPrimitives                   = yieldPipelineStatistics.GSPrimitives;
        pipelineStatistics.m_uiClippingInvocations            = yieldPipelineStatistics.ClippingInvocations;
        pipelineStatistics.m_uiClippingPrimitives             = yieldPipelineStatistics.ClippingPrimitives;
        pipelineStatistics.m_uiVSInvocations                  = yieldPipelineStatistics.VSInvocations;
        pipelineStatistics.m_uiGSInvocations                  = yieldPipelineStatistics.GSInvocations;
        pipelineStatistics.m_uiPSInvocations                  = yieldPipelineStatistics.PSInvocations;
        pipelineStatistics.m_uiHSInvocations                  = yieldPipelineStatistics.HSInvocations;
        pipelineStatistics.m_uiDSInvocations                  = yieldPipelineStatistics.DSInvocations;
        pipelineStatistics.m_uiCSInvocations                  = yieldPipelineStatistics.CSInvocations;

        return true;
      }
    }
    break;
    case xiiGALQueryType::Duration:
    {
      Diligent::QueryDataDuration yieldDurationData;

      if (m_pQuery->GetData(&yieldDurationData, sizeof(yieldDurationData), bAutoInvalidate))
      {
        xiiGALQueryDataDuration& durationData = *reinterpret_cast<xiiGALQueryDataDuration*>(pData);
        durationData.m_Type                   = xiiGALQueryType::Duration;
        durationData.m_uiDuration             = yieldDurationData.Duration;
        durationData.m_uiFrequency            = yieldDurationData.Frequency;

        return true;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return false;
}
#endif

XII_ALWAYS_INLINE void xiiGALQueryD3D11::Invalidate()
{
  m_DisjointQuery.Clear();

  m_QueryState = QueryState::Inactive;
}

XII_ALWAYS_INLINE ID3D11Query* xiiGALQueryD3D11::GetQuery(xiiUInt32 uiQueryID) const
{
  XII_ASSERT_DEV(uiQueryID == 0 || (m_Description.m_Type == xiiGALQueryType::Duration && uiQueryID == 1), "");

  return m_pQueryD3D11[uiQueryID];
}

XII_ALWAYS_INLINE void xiiGALQueryD3D11::SetDisjointQuery(xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> disjointQuery)
{
  m_DisjointQuery = disjointQuery;
}
