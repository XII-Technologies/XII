/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/QueryPoolD3D12.h>

namespace
{
  [[nodiscard]] XII_ALWAYS_INLINE xiiArgEnum GetQueryTypeLogValue(xiiGALQueryType::Enum queryType)
  {
    return xiiArgEnum(xiiEnum<xiiGALQueryType>(queryType));
  }

  constexpr xiiUInt32 s_uiQueryPoolSizes[xiiGALQueryType::ENUM_COUNT] = {
    0U,   // Undefined
    128U, // Occlusion
    128U, // BinaryOcclusion
    512U, // Timestamp
    128U, // PipelineStatistics
    256U  // Duration
  };

  static D3D12_QUERY_TYPE GetD3D12QueryType(xiiGALQueryType::Enum queryType)
  {
    switch (queryType)
    {
      case xiiGALQueryType::Occlusion:
        return D3D12_QUERY_TYPE_OCCLUSION;

      case xiiGALQueryType::BinaryOcclusion:
        return D3D12_QUERY_TYPE_BINARY_OCCLUSION;

      case xiiGALQueryType::Timestamp:
      case xiiGALQueryType::Duration:
        return D3D12_QUERY_TYPE_TIMESTAMP;

      case xiiGALQueryType::PipelineStatistics:
        return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 query type '{}'.", GetQueryTypeLogValue(queryType));
        return D3D12_QUERY_TYPE_TIMESTAMP;
    }
  }

  static xiiUInt32 GetQueryResultStride(xiiGALQueryType::Enum queryType)
  {
    switch (queryType)
    {
      case xiiGALQueryType::Occlusion:
      case xiiGALQueryType::BinaryOcclusion:
      case xiiGALQueryType::Timestamp:
      case xiiGALQueryType::Duration:
        return sizeof(xiiUInt64);

      case xiiGALQueryType::PipelineStatistics:
        return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 query type '{}'.", GetQueryTypeLogValue(queryType));
        return sizeof(xiiUInt64);
    }
  }

  static bool IsGraphicsQueue(const xiiGALCommandQueueCreationDescription& queueDescription)
  {
    return queueDescription.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics);
  }

  static bool IsTransferOnlyQueue(const xiiGALCommandQueueCreationDescription& queueDescription)
  {
    const bool bTransfer = queueDescription.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer);
    const bool bGraphics = queueDescription.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics);
    const bool bCompute  = queueDescription.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Compute);
    return bTransfer && !bGraphics && !bCompute;
  }
} // namespace

xiiGALQueryPoolD3D12::xiiGALQueryPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, const xiiGALQueueInformationD3D12& queueInformation) :
  m_pDeviceD3D12(pDeviceD3D12), m_pCommandQueueD3D12(pCommandQueueD3D12), m_CommandQueueInformation(queueInformation)
{
  LARGE_INTEGER frequency = {};
  if (QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0LL)
  {
    m_uiCounterFrequency = static_cast<xiiUInt64>(frequency.QuadPart);
  }
  else
  {
    m_uiCounterFrequency = 0ULL;
  }

  m_QueryPools.SetCount(xiiGALQueryType::ENUM_COUNT);

  const xiiGALCommandQueueCreationDescription& queueDescription = m_pCommandQueueD3D12->GetDescription();
  const bool bGraphicsQueue = IsGraphicsQueue(queueDescription);
  const bool bTransferOnlyQueue = IsTransferOnlyQueue(queueDescription);
  const bool bTransferTimestampSupported = m_pDeviceD3D12->GetDescription().m_DeviceFeatures.m_TransferQueueTimestampQueries != xiiGALDeviceFeatureState::Disabled;

  for (xiiUInt32 uiQueryType = xiiGALQueryType::Undefined + 1U; uiQueryType < xiiGALQueryType::ENUM_COUNT; ++uiQueryType)
  {
    const xiiGALQueryType::Enum queryType = static_cast<xiiGALQueryType::Enum>(uiQueryType);

    bool bSupported = false;
    if (queryType == xiiGALQueryType::Timestamp || queryType == xiiGALQueryType::Duration)
    {
      bSupported = !bTransferOnlyQueue || bTransferTimestampSupported;
    }
    else
    {
      bSupported = bGraphicsQueue;
    }

    if (!bSupported)
      continue;

    m_QueryPools[queryType] = XII_NEW(pDeviceD3D12->GetAllocator(), QueryPoolInformation, pDeviceD3D12);

    const D3D12_QUERY_HEAP_TYPE queryHeapType = xiiD3D12TypeConversions::GetQueryType(queryType);
    const D3D12_QUERY_TYPE d3d12QueryType = GetD3D12QueryType(queryType);
    xiiUInt32 uiQueryCount = s_uiQueryPoolSizes[uiQueryType];
    if (queryType == xiiGALQueryType::Duration)
    {
      uiQueryCount *= 2U;
    }

    m_QueryPools[queryType]->Initialize(queryType, queryHeapType, d3d12QueryType, uiQueryCount, GetQueryResultStride(queryType));
  }
}

xiiGALQueryPoolD3D12::~xiiGALQueryPoolD3D12()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiLog::Info("D3D12 query manager peak usage:");
#endif

  for (xiiUInt32 uiQueryType = xiiGALQueryType::Undefined + 1U; uiQueryType < xiiGALQueryType::ENUM_COUNT; ++uiQueryType)
  {
    if (m_QueryPools[uiQueryType] == nullptr)
      continue;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    xiiLog::Info("Query Type {} : {} / {}", uiQueryType, m_QueryPools[uiQueryType]->GetMaxAllocatedQueries(), m_QueryPools[uiQueryType]->GetQueryCount());
#endif
    m_QueryPools[uiQueryType]->DeInitialize();
  }

  m_QueryPools.Clear();
}

xiiUInt32 xiiGALQueryPoolD3D12::AllocateQuery(xiiGALQueryType::Enum queryType)
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->Allocate() : xiiInvalidIndex;
}

void xiiGALQueryPoolD3D12::DiscardQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex)
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  if (pQueryPoolInformation != nullptr)
  {
    pQueryPoolInformation->Discard(uiIndex);
  }
}

xiiUInt32 xiiGALQueryPoolD3D12::ResetStaleQueries()
{
  xiiUInt32 uiResetQueryCount = 0U;

  for (auto& pQueryPoolInformation : m_QueryPools)
  {
    if (pQueryPoolInformation != nullptr)
    {
      uiResetQueryCount += pQueryPoolInformation->ResetStaleQueries();
    }
  }

  return uiResetQueryCount;
}

ID3D12QueryHeap* xiiGALQueryPoolD3D12::GetQueryHeap(xiiGALQueryType::Enum queryType) const
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->GetQueryHeap() : nullptr;
}

ID3D12Resource* xiiGALQueryPoolD3D12::GetReadbackBuffer(xiiGALQueryType::Enum queryType) const
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->GetReadbackBuffer() : nullptr;
}

D3D12_QUERY_TYPE xiiGALQueryPoolD3D12::GetD3D12QueryType(xiiGALQueryType::Enum queryType) const
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->GetD3D12QueryType() : D3D12_QUERY_TYPE_TIMESTAMP;
}

xiiUInt64 xiiGALQueryPoolD3D12::GetQueryReadbackOffset(xiiGALQueryType::Enum queryType, xiiUInt32 uiQueryIndex) const
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->GetQueryReadbackOffset(uiQueryIndex) : 0ULL;
}

xiiUInt32 xiiGALQueryPoolD3D12::GetQueryResultStride(xiiGALQueryType::Enum queryType) const
{
  QueryPoolInformation* pQueryPoolInformation = m_QueryPools[queryType].Borrow();
  return pQueryPoolInformation != nullptr ? pQueryPoolInformation->GetQueryResultStride() : 0U;
}

///////////////////////////////////////////////////////////

xiiGALQueryPoolD3D12::QueryPoolInformation::QueryPoolInformation(xiiGALDeviceD3D12* pDeviceD3D12) :
  m_pDeviceD3D12(pDeviceD3D12)
{
}

xiiGALQueryPoolD3D12::QueryPoolInformation::~QueryPoolInformation()
{
  DeInitialize();
}

void xiiGALQueryPoolD3D12::QueryPoolInformation::Initialize(xiiGALQueryType::Enum queryType, D3D12_QUERY_HEAP_TYPE queryHeapType, D3D12_QUERY_TYPE d3d12QueryType, xiiUInt32 uiQueryCount, xiiUInt32 uiQueryResultStride)
{
  XII_ASSERT_DEV(queryType != xiiGALQueryType::Undefined, "Invalid D3D12 query type.");
  XII_ASSERT_DEV(uiQueryCount > 0U, "D3D12 query pool size must be greater than zero.");

  m_QueryType           = queryType;
  m_D3D12QueryType      = d3d12QueryType;
  m_uiQueryCount        = uiQueryCount;
  m_uiQueryResultStride = uiQueryResultStride;

  D3D12_QUERY_HEAP_DESC queryHeapDescription = {};
  queryHeapDescription.Count                 = uiQueryCount;
  queryHeapDescription.Type                  = queryHeapType;
  queryHeapDescription.NodeMask              = 0U;

  HRESULT hResult = m_pDeviceD3D12->GetD3D12Device()->CreateQueryHeap(&queryHeapDescription, IID_PPV_ARGS(&m_pD3D12QueryHeap));
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 query heap for query type '{}': {}.", GetQueryTypeLogValue(queryType), xiiHRESULTtoString(hResult));
    DeInitialize();
    return;
  }

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = static_cast<xiiUInt64>(uiQueryCount) * static_cast<xiiUInt64>(uiQueryResultStride);
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;

  xiiD3D12MemoryAllocator* pD3D12Allocator = m_pDeviceD3D12->GetD3D12Allocator();
  if (pD3D12Allocator == nullptr)
  {
    xiiLog::Error("Cannot create D3D12 query readback buffer because the memory allocator is not initialized.");
    DeInitialize();
    return;
  }

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Readback;

  if (pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, xiiGALResourceStateFlags::CopyDestination, &m_pReadbackBuffer, &m_ReadbackAllocation).Failed())
  {
    xiiLog::Error("Failed to create D3D12 query readback buffer for query type '{}'.", GetQueryTypeLogValue(queryType));
    DeInitialize();
    return;
  }

  m_AvailableQueries.SetCountUninitialized(uiQueryCount);
  for (xiiUInt32 i = 0U; i < uiQueryCount; ++i)
  {
    m_AvailableQueries[i] = i;
  }
  m_StaleQueries.Clear();
}

void xiiGALQueryPoolD3D12::QueryPoolInformation::DeInitialize()
{
  if (m_pReadbackBuffer != nullptr)
  {
    m_pDeviceD3D12->SafeReleaseBuffer(m_pReadbackBuffer, m_ReadbackAllocation);
  }

  if (m_pD3D12QueryHeap != nullptr)
  {
    IUnknown* pObject = m_pD3D12QueryHeap;
    m_pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12QueryHeap = nullptr;
  }

  m_ReadbackAllocation = nullptr;
  m_AvailableQueries.Clear();
  m_StaleQueries.Clear();
  m_uiQueryCount          = 0U;
  m_uiQueryResultStride   = 0U;
  m_uiMaxAllocatedQueries = 0U;
}

xiiUInt32 xiiGALQueryPoolD3D12::QueryPoolInformation::Allocate()
{
  XII_LOCK(m_QueriesMutex);

  if (m_AvailableQueries.IsEmpty())
    return xiiInvalidIndex;

  const xiiUInt32 uiIndex = m_AvailableQueries.PeekBack();
  m_AvailableQueries.PopBack();

  m_uiMaxAllocatedQueries = xiiMath::Max(m_uiMaxAllocatedQueries, m_uiQueryCount - m_AvailableQueries.GetCount());

  return uiIndex;
}

void xiiGALQueryPoolD3D12::QueryPoolInformation::Discard(xiiUInt32 uiIndex)
{
  XII_LOCK(m_QueriesMutex);

  if (uiIndex >= m_uiQueryCount)
  {
    xiiLog::Error("D3D12 query index ({}) is out of range for query type '{}'.", uiIndex, xiiArgEnum(m_QueryType));
    return;
  }

  if (m_AvailableQueries.Contains(uiIndex) || m_StaleQueries.Contains(uiIndex))
  {
    xiiLog::Warning("D3D12 query index ({}) for query type '{}' has already been queued for reuse.", uiIndex, xiiArgEnum(m_QueryType));
    return;
  }

  m_StaleQueries.PushBack(uiIndex);
}

xiiUInt32 xiiGALQueryPoolD3D12::QueryPoolInformation::ResetStaleQueries()
{
  XII_LOCK(m_QueriesMutex);

  if (m_StaleQueries.IsEmpty())
    return 0U;

  const xiiUInt32 uiResetCount = m_StaleQueries.GetCount();
  m_AvailableQueries.PushBackRange(m_StaleQueries);
  m_StaleQueries.Clear();

  return uiResetCount;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_QueryPoolD3D12);
