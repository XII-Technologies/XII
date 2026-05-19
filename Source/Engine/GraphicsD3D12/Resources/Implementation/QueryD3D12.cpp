/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Pools/QueryPoolD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQueryD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] XII_ALWAYS_INLINE xiiArgEnum GetQueryTypeLogValue(xiiGALQueryType::Enum queryType)
  {
    return xiiArgEnum(xiiEnum<xiiGALQueryType>(queryType));
  }
} // namespace

xiiGALQueryD3D12::xiiGALQueryD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(std::move(pDeviceD3D12), creationDescription)
{
  m_QueryPoolIndices.SetCount(2U, xiiInvalidIndex);
}

xiiGALQueryD3D12::~xiiGALQueryD3D12()
{
  DiscardQueries();
}

xiiResult xiiGALQueryD3D12::InitPlatform()
{
  if (m_Description.m_Type == xiiGALQueryType::Undefined)
  {
    xiiLog::Error("Failed to create D3D12 query '{}' because the query type is undefined.", GetDebugName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALQueryD3D12::Invalidate()
{
  DiscardQueries();

  xiiGALQuery::Invalidate();
}

bool xiiGALQueryD3D12::OnBeginQuery(xiiGALCommandListD3D12* pCommandListD3D12)
{
  xiiGALQuery::OnBeginQuery(pCommandListD3D12);

  if (!AllocateQueries())
  {
    Invalidate();
    return false;
  }

  return true;
}

bool xiiGALQueryD3D12::OnEndQuery(xiiGALCommandListD3D12* pCommandListD3D12)
{
  xiiGALQuery::OnEndQuery(pCommandListD3D12);

  if (m_Description.m_Type == xiiGALQueryType::Timestamp && !AllocateQueries())
  {
    return false;
  }

  if ((m_QueryPoolIndices[0] == xiiInvalidIndex) || (m_Description.m_Type == xiiGALQueryType::Duration && m_QueryPoolIndices[1] == xiiInvalidIndex))
  {
    xiiLog::Error("Query '{}' is invalid. D3D12 query allocation failed.", GetQueryTypeLogValue(m_Description.m_Type));
    return false;
  }

  XII_ASSERT_DEV(m_pQueryPoolD3D12 != nullptr, "");

  xiiSharedPtr<xiiGALDeviceD3D12>            pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  const xiiGALCommandListCreationDescription commandListDescription = pCommandListD3D12->GetDescription();
  xiiGALCommandQueueD3D12*                   pCommandQueueD3D12     = xiiDynamicCast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetCommandQueue(commandListDescription.m_QueueFlags));
  if (pCommandQueueD3D12 == nullptr)
  {
    xiiLog::Error("Failed to finalize D3D12 query '{}' because the command queue could not be resolved.", GetDebugName());
    return false;
  }

  m_uiQueryEndFenceValue = pCommandQueueD3D12->GetNextFenceValue();
  return true;
}

bool xiiGALQueryD3D12::AllocateQueries()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  DiscardQueries();

  XII_ASSERT_DEV(m_pQueryPoolD3D12 == nullptr, "");
  XII_ASSERT_DEV(m_pCommandList != nullptr, "");

  const xiiGALCommandListCreationDescription commandListDescription = m_pCommandList->GetDescription();
  m_QueryQueueFlags                                               = commandListDescription.m_QueueFlags;
  m_pQueryPoolD3D12                                               = pDeviceD3D12->GetCommandQueueQueryPool(m_QueryQueueFlags);
  if (m_pQueryPoolD3D12 == nullptr)
  {
    xiiLog::Error("Failed to allocate D3D12 query of type '{}': query pool is unavailable for queue flags {}.", GetQueryTypeLogValue(m_Description.m_Type), xiiArgEnum(m_QueryQueueFlags));
    m_QueryQueueFlags = xiiGALCommandQueueFlags::None;
    return false;
  }

  const xiiUInt32 uiQueryCount = (m_Description.m_Type == xiiGALQueryType::Duration ? 2U : 1U);
  for (xiiUInt32 uiQueryID = 0U; uiQueryID < uiQueryCount; ++uiQueryID)
  {
    xiiUInt32& uiQueryPoolIndex = m_QueryPoolIndices[uiQueryID];

    XII_ASSERT_DEV(uiQueryPoolIndex == xiiInvalidIndex, "");

    uiQueryPoolIndex = m_pQueryPoolD3D12->AllocateQuery(m_Description.m_Type);
    if (uiQueryPoolIndex == xiiInvalidIndex)
    {
      xiiLog::Error("Failed to allocate D3D12 query for type '{}'. Increase the query pool size.", GetQueryTypeLogValue(m_Description.m_Type));

      DiscardQueries();
      return false;
    }
  }

  return true;
}

void xiiGALQueryD3D12::DiscardQueries()
{
  xiiUInt64 uiFenceValue = m_uiQueryEndFenceValue;
  if (uiFenceValue == xiiInvalidIndex && m_QueryQueueFlags != xiiGALCommandQueueFlags::None)
  {
    xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
    if (xiiGALCommandQueueD3D12* pCommandQueueD3D12 = xiiDynamicCast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetCommandQueue(m_QueryQueueFlags)))
    {
      uiFenceValue = pCommandQueueD3D12->GetNextFenceValue();
    }
  }

  for (xiiUInt32& uiQueryPoolIndex : m_QueryPoolIndices)
  {
    if (uiQueryPoolIndex == xiiInvalidIndex)
      continue;

    XII_ASSERT_DEV(m_pQueryPoolD3D12 != nullptr, "");

    m_pQueryPoolD3D12->DiscardQuery(m_Description.m_Type, uiQueryPoolIndex, uiFenceValue);
    uiQueryPoolIndex = xiiInvalidIndex;
  }

  m_pQueryPoolD3D12     = nullptr;
  m_uiQueryEndFenceValue = xiiInvalidIndex;
  m_QueryQueueFlags      = xiiGALCommandQueueFlags::None;
}

bool xiiGALQueryD3D12::ReadbackQueryData(xiiUInt32 uiQueryID, void* pDestinationData, xiiUInt32 uiDataSize) const
{
  XII_ASSERT_DEV(uiQueryID == 0U || (m_Description.m_Type == xiiGALQueryType::Duration && uiQueryID == 1U), "Invalid D3D12 query slot {} for type '{}'.", uiQueryID, GetQueryTypeLogValue(m_Description.m_Type));
  XII_ASSERT_DEV(m_pQueryPoolD3D12 != nullptr, "D3D12 query pool must be valid before reading query data.");

  const xiiUInt32 uiQueryPoolIndex = m_QueryPoolIndices[uiQueryID];
  if (uiQueryPoolIndex == xiiInvalidIndex)
    return false;

  ID3D12Resource* pReadbackBuffer = m_pQueryPoolD3D12->GetReadbackBuffer(m_Description.m_Type);
  if (pReadbackBuffer == nullptr)
  {
    xiiLog::Error("Cannot read D3D12 query data for '{}' because the readback buffer is invalid.", GetDebugName());
    return false;
  }

  const xiiUInt64 uiReadbackOffset = m_pQueryPoolD3D12->GetQueryReadbackOffset(m_Description.m_Type, uiQueryPoolIndex);

  void*       pMappedData = nullptr;
  D3D12_RANGE readRange   = {static_cast<SIZE_T>(uiReadbackOffset), static_cast<SIZE_T>(uiReadbackOffset + uiDataSize)};

  const HRESULT hResult = pReadbackBuffer->Map(0U, &readRange, &pMappedData);
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to map D3D12 query readback buffer for query '{}': {}.", GetDebugName(), xiiHRESULTtoString(hResult));
    return false;
  }

  xiiMemoryUtils::RawByteCopy(pDestinationData, static_cast<const xiiUInt8*>(pMappedData) + uiReadbackOffset, uiDataSize);

  D3D12_RANGE writeRange = {0U, 0U};
  pReadbackBuffer->Unmap(0U, &writeRange);

  return true;
}

bool xiiGALQueryD3D12::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  CheckQueryDataPtr(pData, uiDataSize);

  if (m_pQueryPoolD3D12 == nullptr || m_uiQueryEndFenceValue == xiiInvalidIndex)
    return false;

  const xiiUInt64 uiCompletedFenceValue = m_pQueryPoolD3D12->GetCommandQueue()->GetCompletedFenceValue();
  if (uiCompletedFenceValue < m_uiQueryEndFenceValue)
    return false;

  if (pData == nullptr)
    return true;

  bool bIsDataAvailable = false;
  switch (m_Description.m_Type)
  {
    case xiiGALQueryType::Occlusion:
    {
      xiiUInt64 uiSampleCount = 0ULL;
      if (ReadbackQueryData(0U, &uiSampleCount, sizeof(uiSampleCount)))
      {
        auto& queryData          = *reinterpret_cast<xiiGALQueryDataOcclusion*>(pData);
        queryData.m_uiSampleCount = uiSampleCount;
        bIsDataAvailable          = true;
      }
    }
    break;

    case xiiGALQueryType::BinaryOcclusion:
    {
      xiiUInt64 uiAnySamplesPassed = 0ULL;
      if (ReadbackQueryData(0U, &uiAnySamplesPassed, sizeof(uiAnySamplesPassed)))
      {
        auto& queryData              = *reinterpret_cast<xiiGALQueryDataBinaryOcclusion*>(pData);
        queryData.m_bAnySamplesPassed = uiAnySamplesPassed != 0ULL;
        bIsDataAvailable              = true;
      }
    }
    break;

    case xiiGALQueryType::Timestamp:
    {
      xiiUInt64 uiCounter = 0ULL;
      if (ReadbackQueryData(0U, &uiCounter, sizeof(uiCounter)))
      {
        auto& queryData       = *reinterpret_cast<xiiGALQueryDataTimestamp*>(pData);
        queryData.m_uiCounter = uiCounter;
        queryData.m_uiFrequency = m_pQueryPoolD3D12->GetCounterFrequency();
        bIsDataAvailable        = true;
      }
    }
    break;

    case xiiGALQueryType::PipelineStatistics:
    {
      D3D12_QUERY_DATA_PIPELINE_STATISTICS pipelineStatistics = {};
      if (ReadbackQueryData(0U, &pipelineStatistics, sizeof(pipelineStatistics)))
      {
        auto& queryData = *reinterpret_cast<xiiGALQueryDataPipelineStatistics*>(pData);

        queryData.m_uiInputVertices       = pipelineStatistics.IAVertices;
        queryData.m_uiInputPrimitives     = pipelineStatistics.IAPrimitives;
        queryData.m_uiGSPrimitives        = pipelineStatistics.GSPrimitives;
        queryData.m_uiClippingInvocations = pipelineStatistics.CInvocations;
        queryData.m_uiClippingPrimitives  = pipelineStatistics.CPrimitives;
        queryData.m_uiVSInvocations       = pipelineStatistics.VSInvocations;
        queryData.m_uiGSInvocations       = pipelineStatistics.GSInvocations;
        queryData.m_uiPSInvocations       = pipelineStatistics.PSInvocations;
        queryData.m_uiHSInvocations       = pipelineStatistics.HSInvocations;
        queryData.m_uiDSInvocations       = pipelineStatistics.DSInvocations;
        queryData.m_uiCSInvocations       = pipelineStatistics.CSInvocations;

        bIsDataAvailable = true;
      }
    }
    break;

    case xiiGALQueryType::Duration:
    {
      xiiUInt64 uiStartCounter = 0ULL;
      xiiUInt64 uiEndCounter   = 0ULL;

      if (ReadbackQueryData(0U, &uiStartCounter, sizeof(uiStartCounter)) && ReadbackQueryData(1U, &uiEndCounter, sizeof(uiEndCounter)))
      {
        auto& queryData = *reinterpret_cast<xiiGALQueryDataDuration*>(pData);

        XII_ASSERT_DEV(uiEndCounter >= uiStartCounter, "D3D12 duration query '{}' end timestamp is lower than start timestamp.", GetDebugName());

        queryData.m_uiDuration  = (uiEndCounter >= uiStartCounter) ? (uiEndCounter - uiStartCounter) : 0ULL;
        queryData.m_uiFrequency = m_pQueryPoolD3D12->GetCounterFrequency();

        bIsDataAvailable = true;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (bIsDataAvailable && bAutoInvalidate)
  {
    Invalidate();
  }

  return bIsDataAvailable;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_QueryD3D12);
