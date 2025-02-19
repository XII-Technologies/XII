#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/QueryPoolVulkan.h>

xiiGALQueryPoolVulkan::xiiGALQueryPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, xiiGALQueueInformationVulkan queueInformation) :
  m_pDeviceVulkan(pDeviceVulkan), m_pCommandQueueVulkan(pCommandQueueVulkan), m_CommandQueueInformation(queueInformation)
{
  float fTimeStampPeriod = pDeviceVulkan->GetVulkanPhysicalDeviceProperties().limits.timestampPeriod;
  m_uiCounterFrequency   = static_cast<xiiUInt64>(1000000000.0 / fTimeStampPeriod);

  const auto& deviceDescription       = pDeviceVulkan->GetDescription();
  const auto& enabledFeatures         = pDeviceVulkan->GetVulkanLogicalDeviceFeatures();
  const auto  stageFlags              = pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(m_CommandQueueInformation.m_uiQueueFamilyIndex);
  const auto  queueFlags              = pDeviceVulkan->GetPhysicalDeviceQueueFamilyProperties()[m_CommandQueueInformation.m_uiQueueFamilyIndex].queueFlags;
  const bool  bIsTransferQueue        = (queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics)) == (vk::QueueFlagBits)0;
  const bool  bQueueSupportsTimestamp = pDeviceVulkan->GetPhysicalDeviceQueueFamilyProperties()[m_CommandQueueInformation.m_uiQueueFamilyIndex].timestampValidBits > 0;

  m_QueryPools.PushBack(XII_NEW(pDeviceVulkan->GetAllocator(), QueryPoolInformation, pDeviceVulkan));

  for (xiiUInt32 uiQueryType = xiiGALQueryType::Undefined + 1; uiQueryType < xiiGALQueryType::ENUM_COUNT; ++uiQueryType)
  {
    m_QueryPools.PushBack(XII_NEW(pDeviceVulkan->GetAllocator(), QueryPoolInformation, pDeviceVulkan));

    xiiGALQueryType::Enum queryType = static_cast<xiiGALQueryType::Enum>(uiQueryType);

    if ((queryType == xiiGALQueryType::Occlusion && !enabledFeatures.occlusionQueryPrecise) || (queryType == xiiGALQueryType::PipelineStatistics && !enabledFeatures.pipelineStatisticsQuery))
      continue;

    // Time and duration queries are supported in all queues.
    if (queryType == xiiGALQueryType::Timestamp || queryType == xiiGALQueryType::Duration)
    {
      if (!bQueueSupportsTimestamp)
        continue;

      if (bIsTransferQueue && (deviceDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries == xiiGALDeviceFeatureState::Disabled))
        continue; // Not supported in transfer queue.
    }
    else if ((queueFlags & vk::QueueFlagBits::eGraphics) == (vk::QueueFlagBits)0)
      // Other queries are supported only in graphics queue.
      continue;

    vk::QueryPoolCreateInfo vkQueryPoolCreateInfo = {};
    vkQueryPoolCreateInfo.pNext                   = nullptr;
    vkQueryPoolCreateInfo.flags                   = {};

    switch (queryType)
    {
      case xiiGALQueryType::Occlusion:
      case xiiGALQueryType::BinaryOcclusion:
      {
        vkQueryPoolCreateInfo.queryType = vk::QueryType::eOcclusion;
      };
      break;
      case xiiGALQueryType::Duration:
      case xiiGALQueryType::Timestamp:
      {
        vkQueryPoolCreateInfo.queryType = vk::QueryType::eTimestamp;
      };
      break;
      case xiiGALQueryType::PipelineStatistics:
      {
        vkQueryPoolCreateInfo.queryType = vk::QueryType::ePipelineStatistics;

        vkQueryPoolCreateInfo.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices | vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives | vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations | vk::QueryPipelineStatisticFlagBits::eClippingInvocations | vk::QueryPipelineStatisticFlagBits::eClippingPrimitives | vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations | vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

        if (stageFlags & vk::PipelineStageFlagBits::eGeometryShader)
        {
          vkQueryPoolCreateInfo.pipelineStatistics |= vk::QueryPipelineStatisticFlagBits::eGeometryShaderInvocations | vk::QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives;
        }
        if (stageFlags & vk::PipelineStageFlagBits::eTessellationControlShader)
        {
          vkQueryPoolCreateInfo.pipelineStatistics |= vk::QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches;
        }
        if (stageFlags & vk::PipelineStageFlagBits::eTessellationEvaluationShader)
        {
          vkQueryPoolCreateInfo.pipelineStatistics |= vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations;
        }
      };
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    xiiUInt32 queryPoolSizes[xiiGALQueryType::ENUM_COUNT]{
      0,   // Ignored
      128, // xiiGALQueryType::Occlusion
      128, // xiiGALQueryType::BinaryOcclusion
      512, // xiiGALQueryType::Timestamp
      128, // xiiGALQueryType::PipelineStatistics
      256  // xiiGALQueryType::Duration
    };

    vkQueryPoolCreateInfo.queryCount = queryPoolSizes[uiQueryType];

    if (queryType == xiiGALQueryType::Duration)
    {
      vkQueryPoolCreateInfo.queryCount *= 2U;
    }

    auto& queryPoolInfo = m_QueryPools[queryType];

    queryPoolInfo->Initialize(vkQueryPoolCreateInfo, queryType);

    XII_ASSERT_DEV(!queryPoolInfo->IsInvalidated() && queryPoolInfo->GetQueryCount() == vkQueryPoolCreateInfo.queryCount && queryPoolInfo->GetQueryType() == queryType, "");
  }
}

xiiGALQueryPoolVulkan::~xiiGALQueryPoolVulkan()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiLog::Info("Vulkan query manager peak usage:");

  for (xiiUInt32 uiQueryType = xiiGALQueryType::Undefined + 1; uiQueryType < xiiGALQueryType::ENUM_COUNT; ++uiQueryType)
  {
    auto& queryPoolInformation = m_QueryPools[uiQueryType];

    if (queryPoolInformation->IsInvalidated())
      continue;

    xiiLog::Info("Query Type {} : {} / {}", uiQueryType, queryPoolInformation->GetMaxAllocatedQueries(), queryPoolInformation->GetQueryCount());

    queryPoolInformation->DeInitialize();
  }
#endif

  m_QueryPools.Clear();
}

xiiUInt32 xiiGALQueryPoolVulkan::AllocateQuery(xiiGALQueryType::Enum queryType)
{
  return m_QueryPools[queryType]->Allocate();
}

void xiiGALQueryPoolVulkan::DiscardQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex)
{
  m_QueryPools[queryType]->Discard(uiIndex);
}

xiiUInt32 xiiGALQueryPoolVulkan::ResetStaleQueries(const vk::CommandBuffer& vkCommandBuffer)
{
  xiiUInt32 uiResetQueryCount = 0U;

  for (auto& queryPoolInfo : m_QueryPools)
  {
    uiResetQueryCount += queryPoolInfo->ResetStaleQueries(vkCommandBuffer);
  }

  return uiResetQueryCount;
}

///////////////////////////////////////////////////////////

xiiGALQueryPoolVulkan::QueryPoolInformation::QueryPoolInformation(xiiGALDeviceVulkan* pDeviceVulkan) :
  m_pDeviceVulkan(pDeviceVulkan)
{
}

xiiGALQueryPoolVulkan::QueryPoolInformation::~QueryPoolInformation()
{
  xiiUInt32 uiPendingQueries = GetQueryCount() - (m_AvailableQueries.GetCount() + m_StaleQueries.GetCount());

  if (uiPendingQueries > 0)
  {
    xiiLog::Error("There are '{}' pending queries of type {}.", uiPendingQueries, m_QueryType.GetValue());
  }

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  vkLogicalDevice.destroyQueryPool(m_vkQueryPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::Initialize(const vk::QueryPoolCreateInfo& vkQueryPoolCreateInfo, xiiGALQueryType::Enum queryType)
{
  m_QueryType    = queryType;
  m_uiQueryCount = vkQueryPoolCreateInfo.queryCount;

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  VK_ASSERT_DEV(vkLogicalDevice.createQueryPool(&vkQueryPoolCreateInfo, nullptr, &m_vkQueryPool, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_StaleQueries.SetCountUninitialized(m_uiQueryCount);

  for (xiiUInt32 i = 0; i < m_uiQueryCount; ++i)
  {
    m_StaleQueries[i] = i;
  }
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::DeInitialize()
{
  if (!IsInvalidated())
  {
    // Nothing todo.
  }
}

xiiUInt32 xiiGALQueryPoolVulkan::QueryPoolInformation::Allocate()
{
  xiiUInt32 uiIndex = xiiInvalidIndex;

  XII_LOCK(m_QueriesMutex);

  if (!m_AvailableQueries.IsEmpty())
  {
    uiIndex = m_AvailableQueries.PeekBack();

    m_AvailableQueries.PopBack();

    m_uiMaxAllocatedQueries = xiiMath::Max(m_uiMaxAllocatedQueries, m_uiQueryCount - m_AvailableQueries.GetCount());
  }

  return uiIndex;
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::Discard(xiiUInt32 uiIndex)
{
  XII_LOCK(m_QueriesMutex);

  XII_ASSERT_DEV(uiIndex < m_uiQueryCount, "Queue index ({}) is out of range.", uiIndex);
  XII_ASSERT_DEV(m_vkQueryPool != VK_NULL_HANDLE, "Query pool is not yet initialized");
  XII_ASSERT_DEV(!m_AvailableQueries.Contains(uiIndex), "Index ({}) is already present in available queries list.", uiIndex);
  XII_ASSERT_DEV(!m_StaleQueries.Contains(uiIndex), "Index ({}) is already present in stale queries list.", uiIndex);

  m_StaleQueries.PushBack(uiIndex);
}

xiiUInt32 xiiGALQueryPoolVulkan::QueryPoolInformation::ResetStaleQueries(const vk::CommandBuffer& vkCommandBuffer)
{
  if (m_StaleQueries.IsEmpty())
    return 0U;

  vk::Device  vkLogicalDevice            = m_pDeviceVulkan->GetVulkanLogicalDevice();
  const auto& vkEnabledExtensionFeatures = m_pDeviceVulkan->GetVulkanLogicalDeviceExtensionFeatures();

  auto ResetQueries = [&](xiiUInt32 uiFirstQuery, xiiUInt32 uiQueryCount) -> void {
    if (vkEnabledExtensionFeatures.m_HostQueryReset.hostQueryReset)
    {
      vkLogicalDevice.resetQueryPool(m_vkQueryPool, uiFirstQuery, uiQueryCount, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
    else
    {
      // Note that vkCmdResetQueryPool must be called outside of a render pass, so it is suboptimal to postpone query reset until it is used.
      vkCommandBuffer.resetQueryPool(m_vkQueryPool, uiFirstQuery, uiQueryCount, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
  };

  XII_LOCK(m_QueriesMutex);

  XII_ASSERT_DEV(!IsInvalidated(), "Query pool is not yet initialized.");

  // After query pool creation, each query must be reset before it is used.
  // Queries must also be reset between uses (17.2).
  xiiUInt32 uiCommandCount = 0U;

  if (m_StaleQueries.GetCount() == m_uiQueryCount)
  {
    ResetQueries(0, m_uiQueryCount);

    m_AvailableQueries.SetCountUninitialized(m_uiQueryCount);

    for (xiiUInt32 i = 0; i < m_uiQueryCount; ++i)
    {
      m_AvailableQueries[i] = i;
    }

    uiCommandCount = 1;
  }
  else
  {
    for (auto& uiStaleQuery : m_StaleQueries)
    {
      ResetQueries(uiStaleQuery, 1);

      m_AvailableQueries.PushBack(uiStaleQuery);
    }

    uiCommandCount = m_StaleQueries.GetCount();
  }

  m_StaleQueries.Clear();

  return uiCommandCount;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_QueryPoolVulkan);
