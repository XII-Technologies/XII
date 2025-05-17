#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Query.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQuery, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALQuery::xiiGALQuery(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALDeviceObject(pDevice), m_Description(creationDescription)
{
}

xiiGALQuery::~xiiGALQuery()
{
  if (m_QueryState == QueryState::Querying)
  {
    xiiLog::Error("Destroying query '{}' that is in querying state. A query must be ended before releasing it.", GetDebugName());
  }
}

void xiiGALQuery::Invalidate()
{
  m_QueryState = QueryState::Inactive;
}

void xiiGALQuery::OnBeginQuery(xiiGALCommandList* pCommandList)
{
  XII_ASSERT_DEV(m_Description.m_Type != xiiGALQueryType::Timestamp, "BeginQuery cannot be called on timestamp query '{}'. Call EndQuery to set the timestamp.", GetDebugName());
  XII_ASSERT_DEV(m_QueryState != xiiGALQuery::QueryState::Querying, "Attempting to begin query '{}' twice. A query must be ended before it can be begun again.", GetDebugName());

  if (m_pCommandList != nullptr && m_pCommandList != pCommandList)
  {
    Invalidate();
  }

  m_pCommandList = pCommandList;
  m_QueryState   = xiiGALQuery::QueryState::Querying;
}

void xiiGALQuery::OnEndQuery(xiiGALCommandList* pCommandList)
{
  if (m_Description.m_Type != xiiGALQueryType::Timestamp)
  {
    XII_ASSERT_DEV(m_QueryState == xiiGALQuery::QueryState::Querying && m_pCommandList != nullptr, "Attempting to end query '{}' that has not been begun.", GetDebugName());
    XII_ASSERT_DEV(m_pCommandList == pCommandList, "Query '{}' has been begun by another command list.", GetDebugName());
  }
  else
  {
    // Timestamp queries are never begun.
    if (m_pCommandList != nullptr && m_pCommandList != pCommandList)
    {
      Invalidate();
    }

    m_pCommandList = pCommandList;
  }

  m_QueryState = xiiGALQuery::QueryState::Ended;
}

void xiiGALQuery::CheckQueryDataPtr(void* pData, xiiUInt32 uiDataSize)
{
  XII_ASSERT_DEV(m_QueryState == QueryState::Ended, "Attempting to get data of query '{}' that has not been ended.", GetDebugName());

  if (pData != nullptr)
  {
    switch (m_Description.m_Type)
    {
      case xiiGALQueryType::Occlusion:
      {
        XII_ASSERT_DEV(uiDataSize == sizeof(xiiGALQueryDataOcclusion), "The size of query data ({0}) is incorrect: ({1}) is expected.", GetDebugName(), sizeof(xiiGALQueryDataOcclusion));
      };
      break;
      case xiiGALQueryType::BinaryOcclusion:
      {
        XII_ASSERT_DEV(uiDataSize == sizeof(xiiGALQueryDataBinaryOcclusion), "The size of query data ({0}) is incorrect: ({1}) is expected.", GetDebugName(), sizeof(xiiGALQueryDataBinaryOcclusion));
      };
      break;
      case xiiGALQueryType::Timestamp:
      {
        XII_ASSERT_DEV(uiDataSize == sizeof(xiiGALQueryDataTimestamp), "The size of query data ({0}) is incorrect: ({1}) is expected.", GetDebugName(), sizeof(xiiGALQueryDataTimestamp));
      };
      break;
      case xiiGALQueryType::PipelineStatistics:
      {
        XII_ASSERT_DEV(uiDataSize == sizeof(xiiGALQueryDataPipelineStatistics), "The size of query data ({0}) is incorrect: ({1}) is expected.", GetDebugName(), sizeof(xiiGALQueryDataPipelineStatistics));
      };
      break;
      case xiiGALQueryType::Duration:
      {
        XII_ASSERT_DEV(uiDataSize == sizeof(xiiGALQueryDataDuration), "The size of query data ({0}) is incorrect: ({1}) is expected.", GetDebugName(), sizeof(xiiGALQueryDataDuration));
      };
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Query);
