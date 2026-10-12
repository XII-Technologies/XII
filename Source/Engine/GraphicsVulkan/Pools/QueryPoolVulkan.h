/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class QueryPool;
}

class XII_GRAPHICSVULKAN_DLL xiiGALQueryPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALQueryPoolVulkan);

public:
  [[nodiscard]] xiiUInt32 AllocateQuery(xiiGALQueryType::Enum queryType);
  void                    SetQueryFenceValue(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex, xiiUInt64 uiFenceValue);
  xiiUInt64               GetQueryFenceValue(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex) const;
  void                    RetainQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex);
  void                    DiscardQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex);

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandQueueVulkan* GetCommandQueue() const { return m_pCommandQueueVulkan; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::QueryPool                GetQueryPool(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex) const { return m_QueryPools[queryType]->GetQueryPool(uiIndex); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32                    GetLocalQueryIndex(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex) const { return uiIndex % m_QueryPools[queryType]->GetBlockSize(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64                    GetCounterFrequency() const { return m_uiCounterFrequency; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueueInformationVulkan GetQueueInformation() const { return m_CommandQueueInformation; }


private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  class QueryPoolInformation
  {
    XII_DISALLOW_COPY_AND_ASSIGN(QueryPoolInformation);

  public:
    QueryPoolInformation(xiiGALDeviceVulkan* pDeviceVulkan);
    ~QueryPoolInformation();

    xiiResult Initialize(const vk::QueryPoolCreateInfo& vkQueryPoolCreateInfo, xiiGALQueryType::Enum queryType);

    [[nodiscard]] xiiUInt32 Allocate();
    void                    SetFenceValue(xiiUInt32 uiIndex, xiiUInt64 uiFenceValue);
    xiiUInt64               GetFenceValue(xiiUInt32 uiIndex) const;
    void                    Retain(xiiUInt32 uiIndex);
    void                    Discard(xiiUInt32 uiIndex);

    [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryType::Enum GetQueryType() const { return m_QueryType; }
    [[nodiscard]] vk::QueryPool                           GetQueryPool(xiiUInt32 uiIndex) const;
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetQueryCount() const { return m_ReferenceCounts.GetCount(); }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetBlockSize() const { return m_vkCreateInfo.queryCount; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32             GetMaxAllocatedQueries() const { return m_uiMaxAllocatedQueries; }
    [[nodiscard]] XII_ALWAYS_INLINE bool                  IsInvalidated() const { return m_QueryType == xiiGALQueryType::Undefined; }

  private:
    xiiGALDeviceVulkan* m_pDeviceVulkan;

    xiiResult AddBlock();

    vk::QueryPoolCreateInfo        m_vkCreateInfo;
    xiiDynamicArray<vk::QueryPool> m_Blocks;

    xiiEnum<xiiGALQueryType> m_QueryType             = xiiGALQueryType::Undefined;
    xiiUInt32                m_uiMaxAllocatedQueries = 0U;

    mutable xiiMutex           m_QueriesMutex;
    xiiDynamicArray<xiiUInt32> m_AvailableQueries;
    xiiDynamicArray<xiiUInt32> m_ReferenceCounts;
    xiiDynamicArray<xiiUInt64> m_FenceValues;
  };

  xiiGALQueryPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, xiiGALQueueInformationVulkan queueInformation);
  ~xiiGALQueryPoolVulkan();

  xiiGALDeviceVulkan*                                                             m_pDeviceVulkan;
  xiiGALCommandQueueVulkan*                                                       m_pCommandQueueVulkan;
  xiiStaticArray<xiiUniquePtr<QueryPoolInformation>, xiiGALQueryType::ENUM_COUNT> m_QueryPools;
  xiiGALQueueInformationVulkan                                                    m_CommandQueueInformation;
  xiiUInt64                                                                       m_uiCounterFrequency = 0ULL;
};

/// Keeps an allocated slot alive while its commands are recorded or in flight, independently of the public query object's lifetime.
struct xiiGALQueryReferenceVulkan
{
  xiiGALQueryReferenceVulkan(xiiGALQueryPoolVulkan* pPool, xiiGALQueryType::Enum type, xiiUInt32 uiIndex) :
    m_pPool(pPool), m_Type(type), m_uiIndex(uiIndex)
  {
    m_pPool->RetainQuery(m_Type, m_uiIndex);
  }

  xiiGALQueryReferenceVulkan(xiiGALQueryReferenceVulkan&& other) noexcept :
    m_pPool(std::exchange(other.m_pPool, nullptr)), m_Type(other.m_Type), m_uiIndex(other.m_uiIndex)
  {
  }

  xiiGALQueryReferenceVulkan& operator=(xiiGALQueryReferenceVulkan&& other) noexcept
  {
    if (m_pPool != nullptr)
      m_pPool->DiscardQuery(m_Type, m_uiIndex);
    m_pPool   = std::exchange(other.m_pPool, nullptr);
    m_Type    = other.m_Type;
    m_uiIndex = other.m_uiIndex;
    return *this;
  }

  ~xiiGALQueryReferenceVulkan()
  {
    if (m_pPool != nullptr)
      m_pPool->DiscardQuery(m_Type, m_uiIndex);
  }

  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALQueryReferenceVulkan);

  xiiGALQueryPoolVulkan* m_pPool;
  xiiGALQueryType::Enum  m_Type;
  xiiUInt32              m_uiIndex;
};
