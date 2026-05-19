/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSD3D12_DLL xiiGALQueryPoolD3D12
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALQueryPoolD3D12);

public:
  [[nodiscard]] xiiUInt32 AllocateQuery(xiiGALQueryType::Enum queryType);
  void                    DiscardQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex, xiiUInt64 uiFenceValue);
  xiiUInt32               ResetStaleQueries();

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandQueueD3D12*           GetCommandQueue() const { return m_pCommandQueueD3D12; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64                          GetCounterFrequency() const { return m_uiCounterFrequency; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALQueueInformationD3D12& GetQueueInformation() const { return m_CommandQueueInformation; }

  [[nodiscard]] ID3D12QueryHeap* GetQueryHeap(xiiGALQueryType::Enum queryType) const;
  [[nodiscard]] ID3D12Resource*  GetReadbackBuffer(xiiGALQueryType::Enum queryType) const;
  [[nodiscard]] D3D12_QUERY_TYPE GetD3D12QueryType(xiiGALQueryType::Enum queryType) const;
  [[nodiscard]] xiiUInt64        GetQueryReadbackOffset(xiiGALQueryType::Enum queryType, xiiUInt32 uiQueryIndex) const;
  [[nodiscard]] xiiUInt32        GetQueryResultStride(xiiGALQueryType::Enum queryType) const;

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;
  friend class xiiGALQueryD3D12;

  class QueryPoolInformation
  {
    XII_DISALLOW_COPY_AND_ASSIGN(QueryPoolInformation);

    struct StaleQuery
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt32 m_uiIndex      = xiiInvalidIndex;
      xiiUInt64 m_uiFenceValue = xiiInvalidIndex;
    };

  public:
    QueryPoolInformation(xiiGALDeviceD3D12* pDeviceD3D12);
    ~QueryPoolInformation();

    void Initialize(xiiGALQueryType::Enum queryType, D3D12_QUERY_HEAP_TYPE queryHeapType, D3D12_QUERY_TYPE d3d12QueryType, xiiUInt32 uiQueryCount, xiiUInt32 uiQueryResultStride);
    void DeInitialize();

    [[nodiscard]] xiiUInt32 Allocate();
    void                    Discard(xiiUInt32 uiIndex, xiiUInt64 uiFenceValue);
    [[nodiscard]] xiiUInt32 ResetStaleQueries(xiiUInt64 uiCompletedFenceValue);

    [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryType::Enum GetQueryType() const { return m_QueryType; }
    [[nodiscard]] XII_ALWAYS_INLINE ID3D12QueryHeap*      GetQueryHeap() const { return m_pD3D12QueryHeap; }
    [[nodiscard]] XII_ALWAYS_INLINE ID3D12Resource*       GetReadbackBuffer() const { return m_pReadbackBuffer; }
    [[nodiscard]] XII_ALWAYS_INLINE D3D12_QUERY_TYPE      GetD3D12QueryType() const { return m_D3D12QueryType; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetQueryCount() const { return m_uiQueryCount; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetMaxAllocatedQueries() const { return m_uiMaxAllocatedQueries; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetQueryResultStride() const { return m_uiQueryResultStride; }
    [[nodiscard]] XII_ALWAYS_INLINE bool                  IsInvalidated() const { return m_pD3D12QueryHeap == nullptr; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64             GetQueryReadbackOffset(xiiUInt32 uiIndex) const { return static_cast<xiiUInt64>(uiIndex) * static_cast<xiiUInt64>(m_uiQueryResultStride); }

  private:
    xiiGALDeviceD3D12* m_pDeviceD3D12 = nullptr;

    ID3D12QueryHeap*   m_pD3D12QueryHeap    = nullptr;
    ID3D12Resource*    m_pReadbackBuffer    = nullptr;
    xiiD3D12Allocation m_ReadbackAllocation = nullptr;

    xiiEnum<xiiGALQueryType> m_QueryType             = xiiGALQueryType::Undefined;
    D3D12_QUERY_TYPE         m_D3D12QueryType        = D3D12_QUERY_TYPE_TIMESTAMP;
    xiiUInt32                m_uiQueryCount          = 0U;
    xiiUInt32                m_uiQueryResultStride   = 0U;
    xiiUInt32                m_uiMaxAllocatedQueries = 0U;

    xiiMutex                    m_QueriesMutex;
    xiiDynamicArray<xiiUInt32>  m_AvailableQueries;
    xiiDynamicArray<StaleQuery> m_StaleQueries;
  };

  xiiGALQueryPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, const xiiGALQueueInformationD3D12& queueInformation);
  ~xiiGALQueryPoolD3D12();

private:
  xiiGALDeviceD3D12*       m_pDeviceD3D12       = nullptr;
  xiiGALCommandQueueD3D12* m_pCommandQueueD3D12 = nullptr;

  xiiStaticArray<xiiUniquePtr<QueryPoolInformation>, xiiGALQueryType::ENUM_COUNT> m_QueryPools;
  xiiGALQueueInformationD3D12                                                     m_CommandQueueInformation;
  xiiUInt64                                                                       m_uiCounterFrequency = 0ULL;
};
