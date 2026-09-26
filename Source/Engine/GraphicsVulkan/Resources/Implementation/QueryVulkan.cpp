/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/QueryPoolVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALQueryVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALQueryVulkan::xiiGALQueryVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(std::move(pDeviceVulkan), creationDescription)
{
  m_QueryPoolIndex.SetCount(2U, xiiInvalidIndex);
}

xiiGALQueryVulkan::~xiiGALQueryVulkan()
{
  DiscardQueries();
}

xiiResult xiiGALQueryVulkan::InitPlatform()
{
  return XII_SUCCESS;
}

void xiiGALQueryVulkan::Invalidate()
{
  DiscardQueries();

  xiiGALQuery::Invalidate();
}

bool xiiGALQueryVulkan::OnBeginQuery(xiiGALCommandListVulkan* pCommandListVulkan)
{
  xiiGALQuery::OnBeginQuery(pCommandListVulkan);

  return AllocateQueries();
}

bool xiiGALQueryVulkan::OnEndQuery(xiiGALCommandListVulkan* pCommandListVulkan)
{
  xiiGALQuery::OnEndQuery(pCommandListVulkan);

  if (m_Description.m_Type == xiiGALQueryType::Timestamp)
  {
    if (!AllocateQueries())
      return false;
  }

  if ((m_QueryPoolIndex[0] == xiiInvalidIndex) || (m_Description.m_Type == xiiGALQueryType::Duration && m_QueryPoolIndex[1] == xiiInvalidIndex))
  {
    xiiLog::Error("Query '{}' is invalid. Vulkan query allocation failed!", xiiArgEnum(m_Description.m_Type));
    return false;
  }

  XII_ASSERT_DEV(m_pQueryPoolVulkan != nullptr, "");

  xiiSharedPtr<xiiGALDeviceVulkan>            pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  const xiiGALCommandListCreationDescription& description   = pCommandListVulkan->GetDescription();

  m_uiQueryEndFenceValue = pDeviceVulkan->GetCommandQueue(description.m_QueueFlags)->GetNextFenceValue();

  return false;
}

bool xiiGALQueryVulkan::AllocateQueries()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  DiscardQueries();

  XII_ASSERT_DEV(m_pQueryPoolVulkan == nullptr, "");
  XII_ASSERT_DEV(m_pCommandList != nullptr, "");

  const xiiGALCommandListCreationDescription& description = m_pCommandList->GetDescription();
  m_pQueryPoolVulkan                                      = pDeviceVulkan->GetCommandQueueQueryPool(description.m_QueueFlags);

  XII_ASSERT_DEV(m_pQueryPoolVulkan != nullptr, "");

  for (xiiUInt32 i = 0; i < (m_Description.m_Type == xiiGALQueryType::Duration ? 2U : 1U); ++i)
  {
    xiiUInt32& uiQueryPoolIndex = m_QueryPoolIndex[i];

    XII_ASSERT_DEV(uiQueryPoolIndex == xiiInvalidIndex, "");

    uiQueryPoolIndex = m_pQueryPoolVulkan->AllocateQuery(m_Description.m_Type);

    if (uiQueryPoolIndex == xiiInvalidIndex)
    {
      xiiLog::Error("Failed to allocate Vulkan query for type {}. Increase the query pool size.", xiiArgEnum(m_Description.m_Type));

      DiscardQueries();

      return false;
    }
  }

  return true;
}

void xiiGALQueryVulkan::DiscardQueries()
{
  for (auto& uiQueryPoolIndex : m_QueryPoolIndex)
  {
    if (uiQueryPoolIndex != xiiInvalidIndex)
    {
      XII_ASSERT_DEV(m_pQueryPoolVulkan != nullptr, "");

      m_pQueryPoolVulkan->DiscardQuery(m_Description.m_Type, uiQueryPoolIndex);

      uiQueryPoolIndex = xiiInvalidIndex;
    }
  }

  m_pQueryPoolVulkan     = nullptr;
  m_uiQueryEndFenceValue = xiiInvalidIndex;
}

namespace
{
  template <xiiUInt32 ElementCount>
  inline bool GetQueryResults(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, xiiUInt32 uiQueryIndex, xiiStaticArray<xiiUInt64, ElementCount>& results)
  {
    static_assert(ElementCount >= 2, "The number of elements must be at least 2 as the last one is used to get the query status.");

    vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    // If VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is set, the final integer value written for each query
    // is non-zero if the query's status was available or zero if the status was unavailable.

    // Applications must take care to ensure that use of the VK_QUERY_RESULT_WITH_AVAILABILITY_BIT bit has the desired effect.
    // For example, if a query has been used previously and a command buffer records the commands
    // vkCmdResetQueryPool, vkCmdBeginQuery, and vkCmdEndQuery for that query, then the query will
    // remain in the available state until vkResetQueryPoolEXT is called or the vkCmdResetQueryPool
    // command executes on a queue. Applications can use fences or events to ensure that a query has
    // already been reset before checking for its results or availability status. Otherwise, a stale
    // value could be returned from a previous use of the query.

    vk::Result vkResult = vkLogicalDevice.getQueryPoolResults(vkQueryPool, uiQueryIndex, 1U, static_cast<size_t>(sizeof(results[0]) * results.GetCount()), results.GetData(), 0U, vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWithAvailability, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    bool bIsDataAvailable = vkResult == vk::Result::eSuccess;

    constexpr bool bIsSingleElementQuery = ElementCount == 2U;

    if (bIsSingleElementQuery)
    {
      // For single-element queries (timestamp, occlusion, duration, etc.), the second element always contains the availability flag.
      // The number of elements returned for the occlusion query depends on the stage flags, so the availability flag index varies.

      bIsDataAvailable = bIsDataAvailable && (results[1] != 0);
    }

    return bIsDataAvailable;
  }

  inline bool GetOcclusionQueryData(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, xiiUInt32 uiQueryIndex, void* pData, xiiUInt32 uiDataSize)
  {
    XII_IGNORE_UNUSED(uiDataSize);

    xiiStaticArray<xiiUInt64, 2U> results;
    results.SetCount(2U, 0ULL);

    const bool bIsDataAvailable = GetQueryResults(pDeviceVulkan, vkQueryPool, uiQueryIndex, results);

    if (bIsDataAvailable && pData != nullptr)
    {
      auto& queryData = *reinterpret_cast<xiiGALQueryDataOcclusion*>(pData);

      XII_ASSERT_DEV(uiDataSize == sizeof(queryData), "");

      queryData.m_uiSampleCount = results[0];
    }

    return bIsDataAvailable;
  }

  inline bool GetBinaryOcclusionQueryData(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, xiiUInt32 uiQueryIndex, void* pData, xiiUInt32 uiDataSize)
  {
    XII_IGNORE_UNUSED(uiDataSize);

    xiiStaticArray<xiiUInt64, 2U> results;
    results.SetCount(2U, 0ULL);

    const bool bIsDataAvailable = GetQueryResults(pDeviceVulkan, vkQueryPool, uiQueryIndex, results);

    if (bIsDataAvailable && pData != nullptr)
    {
      auto& queryData = *reinterpret_cast<xiiGALQueryDataBinaryOcclusion*>(pData);

      XII_ASSERT_DEV(uiDataSize == sizeof(queryData), "");

      queryData.m_bAnySamplesPassed = results[0] != 0U;
    }

    return bIsDataAvailable;
  }

  inline bool GetTimestampQueryData(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, xiiUInt32 uiQueryIndex, xiiUInt64 uiCounterFrequency, void* pData, xiiUInt32 uiDataSize)
  {
    XII_IGNORE_UNUSED(uiDataSize);

    xiiStaticArray<xiiUInt64, 2U> results;
    results.SetCount(2U, 0ULL);

    const bool bIsDataAvailable = GetQueryResults(pDeviceVulkan, vkQueryPool, uiQueryIndex, results);

    if (bIsDataAvailable && pData != nullptr)
    {
      auto& queryData = *reinterpret_cast<xiiGALQueryDataTimestamp*>(pData);

      XII_ASSERT_DEV(uiDataSize == sizeof(queryData), "");

      queryData.m_uiCounter   = results[0];
      queryData.m_uiFrequency = uiCounterFrequency;
    }

    return bIsDataAvailable;
  }

  inline bool GetDurationQueryData(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, const xiiStaticArray<xiiUInt32, 2>& queryIndex, xiiUInt64 uiCounterFrequency, void* pData, xiiUInt32 uiDataSize)
  {
    XII_IGNORE_UNUSED(uiDataSize);

    xiiUInt64 uiStartCounter = 0ULL;
    xiiUInt64 uiEndCounter   = 0ULL;

    bool bIsDataAvailable = true;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiStaticArray<xiiUInt64, 2U> results;
      results.SetCount(2U, 0ULL);

      if (!GetQueryResults(pDeviceVulkan, vkQueryPool, queryIndex[i], results))
        bIsDataAvailable = false;

      (i == 0 ? uiStartCounter : uiEndCounter) = results[0];
    }

    if (bIsDataAvailable && pData != nullptr)
    {
      auto& queryData = *reinterpret_cast<xiiGALQueryDataDuration*>(pData);

      XII_ASSERT_DEV(uiDataSize == sizeof(queryData), "");
      XII_ASSERT_DEV(uiEndCounter >= uiStartCounter, "");

      queryData.m_uiDuration  = uiEndCounter - uiStartCounter;
      queryData.m_uiFrequency = uiCounterFrequency;
    }

    return bIsDataAvailable;
  }

  inline bool GetStatisticsQueryData(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, vk::QueryPool vkQueryPool, xiiUInt32 uiQueryIndex, xiiGALQueueInformationVulkan queueInformation, void* pData, xiiUInt32 uiDataSize)
  {
    XII_IGNORE_UNUSED(uiDataSize);

    // Pipeline statistics queries write one integer value for each bit that is enabled in the pipelineStatistics when the pool is created, and the statistics values are written in bit
    // order starting from the least significant bit. (17.2)

    xiiStaticArray<xiiUInt64, 12U> results;
    results.SetCount(12U, 0ULL);

    bool bIsDataAvailable = GetQueryResults(pDeviceVulkan, vkQueryPool, uiQueryIndex, results);
    if (bIsDataAvailable && pData != nullptr)
    {
      const vk::PipelineStageFlags vkSupportedStageFlags = pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(queueInformation.m_uiQueueFamilyIndex);

      auto& queryData = *reinterpret_cast<xiiGALQueryDataPipelineStatistics*>(pData);

      XII_ASSERT_DEV(uiDataSize == sizeof(queryData), "");

      xiiUInt32 uiWriteIndex = 0U;

      queryData.m_uiInputVertices   = results[uiWriteIndex++]; // INPUT_ASSEMBLY_VERTICES_BIT   = 0x00000001
      queryData.m_uiInputPrimitives = results[uiWriteIndex++]; // INPUT_ASSEMBLY_PRIMITIVES_BIT = 0x00000002
      queryData.m_uiVSInvocations   = results[uiWriteIndex++]; // VERTEX_SHADER_INVOCATIONS_BIT = 0x00000004

      if (vkSupportedStageFlags & vk::PipelineStageFlagBits::eGeometryShader)
      {
        queryData.m_uiGSInvocations = results[uiWriteIndex++]; // GEOMETRY_SHADER_INVOCATIONS_BIT = 0x00000008
        queryData.m_uiGSPrimitives  = results[uiWriteIndex++]; // GEOMETRY_SHADER_PRIMITIVES_BIT  = 0x00000010
      }

      queryData.m_uiClippingInvocations = results[uiWriteIndex++]; // CLIPPING_INVOCATIONS_BIT         = 0x00000020
      queryData.m_uiClippingPrimitives  = results[uiWriteIndex++]; // CLIPPING_PRIMITIVES_BIT          = 0x00000040
      queryData.m_uiPSInvocations       = results[uiWriteIndex++]; // FRAGMENT_SHADER_INVOCATIONS_BIT  = 0x00000080

      if (vkSupportedStageFlags & vk::PipelineStageFlagBits::eTessellationControlShader)
      {
        queryData.m_uiHSInvocations = results[uiWriteIndex++]; // TESSELLATION_CONTROL_SHADER_PATCHES_BIT = 0x00000100
      }
      if (vkSupportedStageFlags & vk::PipelineStageFlagBits::eTessellationEvaluationShader)
      {
        queryData.m_uiDSInvocations = results[uiWriteIndex++]; // TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT = 0x00000200
      }

      queryData.m_uiCSInvocations = results[uiWriteIndex++]; // COMPUTE_SHADER_INVOCATIONS_BIT = 0x00000400

      bIsDataAvailable = bIsDataAvailable && (results[uiWriteIndex] != 0U);
    }

    return bIsDataAvailable;
  }
} // namespace

bool xiiGALQueryVulkan::GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate)
{
  CheckQueryDataPtr(pData, uiDataSize);

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  const xiiGALQueueInformationVulkan& queueInformation      = m_pQueryPoolVulkan->GetQueueInformation();
  const xiiUInt64                     uiCompletedFenceValue = m_pQueryPoolVulkan->GetCommandQueue()->GetCompletedFenceValue();
  bool                                bIsDataAvailable      = false;

  if (uiCompletedFenceValue >= m_uiQueryEndFenceValue)
  {
    vk::QueryPool vkQueryPool = m_pQueryPoolVulkan->GetQueryPool(m_Description.m_Type);

    switch (m_Description.m_Type)
    {
      case xiiGALQueryType::Occlusion:
      {
        bIsDataAvailable = GetOcclusionQueryData(pDeviceVulkan, vkQueryPool, m_QueryPoolIndex[0], pData, uiDataSize);
      }
      break;
      case xiiGALQueryType::BinaryOcclusion:
      {
        bIsDataAvailable = GetBinaryOcclusionQueryData(pDeviceVulkan, vkQueryPool, m_QueryPoolIndex[0], pData, uiDataSize);
      }
      break;
      case xiiGALQueryType::Timestamp:
      {
        bIsDataAvailable = GetTimestampQueryData(pDeviceVulkan, vkQueryPool, m_QueryPoolIndex[0], m_pQueryPoolVulkan->GetCounterFrequency(), pData, uiDataSize);
      }
      break;
      case xiiGALQueryType::PipelineStatistics:
      {
        bIsDataAvailable = GetStatisticsQueryData(pDeviceVulkan, vkQueryPool, m_QueryPoolIndex[0], queueInformation, pData, uiDataSize);
      }
      break;
      case xiiGALQueryType::Duration:
      {
        bIsDataAvailable = GetDurationQueryData(pDeviceVulkan, vkQueryPool, m_QueryPoolIndex, m_pQueryPoolVulkan->GetCounterFrequency(), pData, uiDataSize);
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  if (bIsDataAvailable && pData != nullptr && bAutoInvalidate)
  {
    Invalidate();
  }

  return bIsDataAvailable;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_QueryVulkan);
