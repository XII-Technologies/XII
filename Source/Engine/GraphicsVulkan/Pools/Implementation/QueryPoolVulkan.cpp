/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

    if (queryPoolInfo->Initialize(vkQueryPoolCreateInfo, queryType).Failed())
      xiiLog::Error("Failed to create Vulkan query pool for type {}.", queryType);
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

void xiiGALQueryPoolVulkan::SetQueryFenceValue(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex, xiiUInt64 uiFenceValue)
{
  m_QueryPools[queryType]->SetFenceValue(uiIndex, uiFenceValue);
}

xiiUInt64 xiiGALQueryPoolVulkan::GetQueryFenceValue(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex) const
{
  return m_QueryPools[queryType]->GetFenceValue(uiIndex);
}

void xiiGALQueryPoolVulkan::RetainQuery(xiiGALQueryType::Enum queryType, xiiUInt32 uiIndex)
{
  m_QueryPools[queryType]->Retain(uiIndex);
}

///////////////////////////////////////////////////////////

xiiGALQueryPoolVulkan::QueryPoolInformation::QueryPoolInformation(xiiGALDeviceVulkan* pDeviceVulkan) :
  m_pDeviceVulkan(pDeviceVulkan)
{
}

xiiGALQueryPoolVulkan::QueryPoolInformation::~QueryPoolInformation()
{
  const xiiUInt32 uiPendingQueries = GetQueryCount() - m_AvailableQueries.GetCount();
  if (uiPendingQueries > 0U)
    xiiLog::Error("There are '{}' pending queries of type {}.", uiPendingQueries, xiiArgEnum(m_QueryType));

  for (vk::QueryPool pool : m_Blocks)
    m_pDeviceVulkan->GetVulkanLogicalDevice().destroyQueryPool(pool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

xiiResult xiiGALQueryPoolVulkan::QueryPoolInformation::Initialize(const vk::QueryPoolCreateInfo& createInfo, xiiGALQueryType::Enum queryType)
{
  XII_ASSERT_DEV(queryType != xiiGALQueryType::Undefined, "Invalid query pool type.");
  m_QueryType    = queryType;
  m_vkCreateInfo = createInfo;
  return AddBlock();
}

xiiResult xiiGALQueryPoolVulkan::QueryPoolInformation::AddBlock()
{
  if (GetQueryCount() > xiiInvalidIndex - m_vkCreateInfo.queryCount)
    return XII_FAILURE;

  vk::QueryPool pool = VK_NULL_HANDLE;
  if (m_pDeviceVulkan->GetVulkanLogicalDevice().createQueryPool(&m_vkCreateInfo, nullptr, &pool, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()) != vk::Result::eSuccess)
    return XII_FAILURE;

  const xiiUInt32 uiFirstIndex = GetQueryCount();
  m_Blocks.PushBack(pool);
  m_ReferenceCounts.SetCount(uiFirstIndex + m_vkCreateInfo.queryCount, 0U);
  m_FenceValues.SetCount(uiFirstIndex + m_vkCreateInfo.queryCount, xiiMath::MaxValue<xiiUInt64>());
  for (xiiUInt32 i = 0U; i < m_vkCreateInfo.queryCount; ++i)
    m_AvailableQueries.PushBack(uiFirstIndex + i);
  return XII_SUCCESS;
}

xiiUInt32 xiiGALQueryPoolVulkan::QueryPoolInformation::Allocate()
{
  XII_LOCK(m_QueriesMutex);
  if (IsInvalidated() || (m_AvailableQueries.IsEmpty() && AddBlock().Failed()))
    return xiiInvalidIndex;

  const xiiUInt32 uiIndex = m_AvailableQueries.PeekBack();
  m_AvailableQueries.PopBack();
  m_ReferenceCounts[uiIndex] = 1U;
  m_FenceValues[uiIndex]     = xiiMath::MaxValue<xiiUInt64>();
  m_uiMaxAllocatedQueries    = xiiMath::Max(m_uiMaxAllocatedQueries, GetQueryCount() - m_AvailableQueries.GetCount());
  return uiIndex;
}

vk::QueryPool xiiGALQueryPoolVulkan::QueryPoolInformation::GetQueryPool(xiiUInt32 uiIndex) const
{
  XII_LOCK(m_QueriesMutex);
  XII_ASSERT_DEV(uiIndex < GetQueryCount(), "Query index out of range.");
  return m_Blocks[uiIndex / m_vkCreateInfo.queryCount];
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::SetFenceValue(xiiUInt32 uiIndex, xiiUInt64 uiFenceValue)
{
  XII_LOCK(m_QueriesMutex);
  m_FenceValues[uiIndex] = uiFenceValue;
}

xiiUInt64 xiiGALQueryPoolVulkan::QueryPoolInformation::GetFenceValue(xiiUInt32 uiIndex) const
{
  XII_LOCK(m_QueriesMutex);
  return m_FenceValues[uiIndex];
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::Retain(xiiUInt32 uiIndex)
{
  XII_LOCK(m_QueriesMutex);
  XII_ASSERT_DEV(uiIndex < GetQueryCount() && m_ReferenceCounts[uiIndex] > 0U, "Cannot retain an unallocated query.");
  ++m_ReferenceCounts[uiIndex];
}

void xiiGALQueryPoolVulkan::QueryPoolInformation::Discard(xiiUInt32 uiIndex)
{
  XII_LOCK(m_QueriesMutex);
  XII_ASSERT_DEV(uiIndex < GetQueryCount() && m_ReferenceCounts[uiIndex] > 0U, "Cannot discard an unallocated query.");
  if (--m_ReferenceCounts[uiIndex] == 0U)
    m_AvailableQueries.PushBack(uiIndex);
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_QueryPoolVulkan);
