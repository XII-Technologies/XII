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
  void                    DiscardQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex);

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandQueueVulkan* GetCommandQueue() const { return m_pCommandQueueVulkan; }
  [[nodiscard]] XII_ALWAYS_INLINE vk::QueryPool                GetQueryPool(xiiGALQueryType::Enum queryType) const { return m_QueryPools[queryType]->GetQueryPool(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64                    GetCounterFrequency() const { return m_uiCounterFrequency; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueueInformationVulkan GetQueueInformation() const { return m_CommandQueueInformation; }

  xiiUInt32 ResetStaleQueries(const vk::CommandBuffer& vkCommandBuffer);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  class QueryPoolInformation
  {
    XII_DISALLOW_COPY_AND_ASSIGN(QueryPoolInformation);

  public:
    QueryPoolInformation(xiiGALDeviceVulkan* pDeviceVulkan);
    ~QueryPoolInformation();

    void Initialize(const vk::QueryPoolCreateInfo& vkQueryPoolCreateInfo, xiiGALQueryType::Enum queryType);
    void DeInitialize();

    [[nodiscard]] xiiUInt32 Allocate();
    void                    Discard(xiiUInt32 uiIndex);
    [[nodiscard]] xiiUInt32 ResetStaleQueries(const vk::CommandBuffer& vkCommandBuffer);

    [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryType::Enum GetQueryType() const { return m_QueryType; }
    [[nodiscard]] XII_ALWAYS_INLINE vk::QueryPool GetQueryPool() const { return m_vkQueryPool; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32     GetQueryCount() const { return m_uiQueryCount; }
    [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32     GetMaxAllocatedQueries() const { return m_uiMaxAllocatedQueries; }
    [[nodiscard]] XII_ALWAYS_INLINE bool          IsInvalidated() const { return m_vkQueryPool == VK_NULL_HANDLE; }

  private:
    xiiGALDeviceVulkan* m_pDeviceVulkan;

    vk::QueryPool m_vkQueryPool = VK_NULL_HANDLE;

    xiiEnum<xiiGALQueryType> m_QueryType             = xiiGALQueryType::Undefined;
    xiiUInt32                m_uiQueryCount          = 0U;
    xiiUInt32                m_uiMaxAllocatedQueries = 0U;

    xiiMutex                   m_QueriesMutex;
    xiiDynamicArray<xiiUInt32> m_AvailableQueries;
    xiiDynamicArray<xiiUInt32> m_StaleQueries;
  };

  xiiGALQueryPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, xiiGALQueueInformationVulkan queueInformation);
  ~xiiGALQueryPoolVulkan();

  xiiGALDeviceVulkan*                                                             m_pDeviceVulkan;
  xiiGALCommandQueueVulkan*                                                       m_pCommandQueueVulkan;
  xiiStaticArray<xiiUniquePtr<QueryPoolInformation>, xiiGALQueryType::ENUM_COUNT> m_QueryPools;
  xiiGALQueueInformationVulkan                                                    m_CommandQueueInformation;
  xiiUInt64                                                                       m_uiCounterFrequency = 0ULL;
};
