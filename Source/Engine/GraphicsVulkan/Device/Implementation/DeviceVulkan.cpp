/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#endif

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/SwapChainVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>
#include <GraphicsVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <GraphicsVulkan/Pools/DynamicBufferPoolVulkan.h>
#include <GraphicsVulkan/Pools/FencePoolVulkan.h>
#include <GraphicsVulkan/Pools/QueryPoolVulkan.h>
#include <GraphicsVulkan/Pools/SemaphorePoolVulkan.h>
#include <GraphicsVulkan/Pools/StagingBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/ComputePipelineStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/GraphicsPipelineStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>
#include <GraphicsVulkan/States/RayTracingPipelineStateVulkan.h>
#include <GraphicsVulkan/States/TilePipelineStateVulkan.h>
#include <GraphicsVulkan/Utilities/CpuWaitOnlyFenceVulkan.h>

#include <bitset>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDeviceVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInternal::NewInstance<xiiGALDevice> CreateVulkanDevice(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  return XII_NEW(pAllocator, xiiGALDeviceVulkan, pAllocator, description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsVulkan, DeviceFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const xiiGALDeviceImplementationDescription implementation = {.m_APIType = xiiGALGraphicsDeviceType::Vulkan, .m_sShaderModel = "VK_SM60", .m_sShaderCompiler = "xiiShaderCompilerSPIRV" };

    xiiGALDeviceFactory::RegisterImplementation("Vulkan", &CreateVulkanDevice, implementation);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALDeviceFactory::UnregisterImplementation("Vulkan");
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  VKAPI_ATTR vk::Bool32 VKAPI_CALL xiiVulkanDebugMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
  {
    switch (messageSeverity)
    {
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      {
        xiiLog::Debug("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      {
        xiiLog::Info("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      {
        xiiLog::Warning("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      {
        xiiLog::Error("Vulkan: {}.", pCallbackData->pMessage);
      }
      break;
      default:
        break;
    }
    // The application should always return vk::False. The vk::True value is reserved for use in layer development.
    return vk::False;
  }

  VKAPI_ATTR xiiUInt32 VKAPI_CALL xiiVulkanDebugReportCallback(vk::DebugReportFlagsEXT reportFlags, vk::DebugReportObjectTypeEXT, xiiUInt64, size_t, xiiInt32, const char*, const char* szMessage, void*)
  {
    if (reportFlags & vk::DebugReportFlagBitsEXT::eError)
    {
      xiiLog::Error("Vulkan: {}", szMessage);
    }
    else if (reportFlags & (vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning))
    {
      xiiLog::Warning("Vulkan: {}", szMessage);
    }
    else if (reportFlags & vk::DebugReportFlagBitsEXT::eDebug)
    {
      xiiLog::Debug("Vulkan: {}", szMessage);
    }
    else
    {
      xiiLog::Info("Vulkan: {}", szMessage);
    }
    return 0U;
  }
} // namespace


///////////////////////////////////////////////////////////////////////////

class xiiGALDeviceVulkan::DeferredDeletionQueue
{
public:
  explicit DeferredDeletionQueue(xiiGALDeviceVulkan* pDeviceVulkan) :
    m_pDeviceVulkan(pDeviceVulkan)
  {
    XII_ASSERT_DEV(m_pDeviceVulkan != nullptr, "Vulkan deferred deletion queue requires a valid device.");
  }

  ~DeferredDeletionQueue()
  {
    ReleaseResources(true);
  }

  void EnqueueResource(vk::ObjectType vkObjectType, void* pObject)
  {
    XII_ASSERT_DEV(vkObjectType != vk::ObjectType::eUnknown, "Vulkan object type must be valid.");
    XII_ASSERT_DEV(pObject != nullptr, "Object must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry  = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData = CaptureSubmittedQueueSyncData();
    entry.m_vkObjectType  = vkObjectType;
    entry.m_pObject       = pObject;
  }

  void EnqueueResource(vk::ObjectType vkObjectType, void* pObject, xiiVulkanAllocation allocation)
  {
    XII_ASSERT_DEV(vkObjectType == vk::ObjectType::eBuffer || vkObjectType == vk::ObjectType::eImage, "Vulkan object type does not have a valid Vulkan memory allocation.");
    XII_ASSERT_DEV(pObject != nullptr, "Object must be valid.");
    XII_ASSERT_DEV(allocation != VK_NULL_HANDLE, "Vulkan memory allocation must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry     = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData    = CaptureSubmittedQueueSyncData();
    entry.m_vkObjectType     = vkObjectType;
    entry.m_pObject          = pObject;
    entry.m_VulkanAllocation = allocation;
  }

  void EnqueueResource(vk::ObjectType vkObjectType, void* pObject, vk::DeviceMemory vkExternalMemory)
  {
    XII_ASSERT_DEV(vkObjectType == vk::ObjectType::eBuffer || vkObjectType == vk::ObjectType::eImage, "Vulkan object type does not have a valid Vulkan external memory.");
    XII_ASSERT_DEV(pObject != nullptr, "Object must be valid.");
    XII_ASSERT_DEV(vkExternalMemory != VK_NULL_HANDLE, "Vulkan external memory must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry     = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData    = CaptureSubmittedQueueSyncData();
    entry.m_vkObjectType     = vkObjectType;
    entry.m_pObject          = pObject;
    entry.m_vkExternalMemory = vkExternalMemory;
  }

  void EnqueueResource(xiiGALSemaphorePoolVulkan* pSemaphorePool, vk::Semaphore vkSemaphore)
  {
    XII_ASSERT_DEV(pSemaphorePool != nullptr, "Semaphore pool must be valid.");
    XII_ASSERT_DEV(vkSemaphore != VK_NULL_HANDLE, "Semaphore must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry   = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData  = CaptureSubmittedQueueSyncData();
    entry.m_pSemaphorePool = pSemaphorePool;
    entry.m_vkSemaphore    = vkSemaphore;
  }

  void EnqueueResource(xiiGALDescriptorSetPoolVulkan* pDescriptorSetPool, vk::DescriptorPool vkDescriptorPool)
  {
    XII_ASSERT_DEV(pDescriptorSetPool != nullptr, "Descriptor set pool must be valid.");
    XII_ASSERT_DEV(vkDescriptorPool != VK_NULL_HANDLE, "Descriptor pool must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry       = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData      = CaptureSubmittedQueueSyncData();
    entry.m_pDescriptorSetPool = pDescriptorSetPool;
    entry.m_vkDescriptorPool   = vkDescriptorPool;
  }

  void EnqueueResource(xiiGALFencePoolVulkan* pFencePool, vk::Fence vkFence)
  {
    XII_ASSERT_DEV(pFencePool != nullptr, "Fence pool must be valid.");
    XII_ASSERT_DEV(vkFence != VK_NULL_HANDLE, "Fence must be valid.");

    XII_LOCK(m_DeletionQueueMutex);

    DeletionEntry& entry  = m_DeletionQueue.ExpandAndGetRef();
    entry.m_QueueSyncData = CaptureSubmittedQueueSyncData();
    entry.m_pFencePool    = pFencePool;
    entry.m_vkFence       = vkFence;
  }

  void ReleaseResources(bool bForceReleaseAll = false)
  {
    const vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

    while (true)
    {
      QueueSyncData completedQueueSyncData = {};
      if (!bForceReleaseAll)
      {
        completedQueueSyncData = CaptureCompletedQueueSyncData();
      }

      DeletionEntry entryToDestroy;
      bool          bHasEntryToDestroy = false;

      {
        XII_LOCK(m_DeletionQueueMutex);

        if (m_DeletionQueue.IsEmpty())
          break;

        if (!bForceReleaseAll && !IsReadyForDeletion(m_DeletionQueue.PeekFront(), completedQueueSyncData))
          break;

        entryToDestroy = m_DeletionQueue.PeekFront();
        m_DeletionQueue.PopFront();
        bHasEntryToDestroy = true;
      }

      if (bHasEntryToDestroy)
      {
        DestroyEntry(vkLogicalDevice, entryToDestroy);
      }
    }
  }

  [[nodiscard]] bool IsEmpty() const
  {
    XII_LOCK(m_DeletionQueueMutex);
    return m_DeletionQueue.IsEmpty();
  }

private:
  struct QueueSyncData
  {
    xiiUInt64 m_uiGraphicsFenceValue = 0ULL;
    xiiUInt64 m_uiComputeFenceValue  = 0ULL;
    xiiUInt64 m_uiTransferFenceValue = 0ULL;
  };

  struct DeletionEntry
  {
    QueueSyncData m_QueueSyncData = {};

    vk::ObjectType      m_vkObjectType     = vk::ObjectType::eUnknown;
    void*               m_pObject          = VK_NULL_HANDLE;
    xiiVulkanAllocation m_VulkanAllocation = VK_NULL_HANDLE;
    vk::DeviceMemory    m_vkExternalMemory = VK_NULL_HANDLE;

    xiiGALSemaphorePoolVulkan* m_pSemaphorePool = nullptr;
    vk::Semaphore              m_vkSemaphore    = VK_NULL_HANDLE;

    xiiGALFencePoolVulkan* m_pFencePool = nullptr;
    vk::Fence              m_vkFence    = VK_NULL_HANDLE;

    xiiGALDescriptorSetPoolVulkan* m_pDescriptorSetPool = nullptr;
    vk::DescriptorPool             m_vkDescriptorPool   = VK_NULL_HANDLE;
  };

  [[nodiscard]] static xiiUInt64 GetSubmittedFenceValue(const xiiGALCommandQueueVulkan* pCommandQueueVulkan)
  {
    if (pCommandQueueVulkan == nullptr)
      return 0ULL;

    return pCommandQueueVulkan->GetNextFenceValue();
  }

  [[nodiscard]] static xiiUInt64 GetCompletedFenceValue(xiiGALCommandQueueVulkan* pCommandQueueVulkan)
  {
    return pCommandQueueVulkan != nullptr ? pCommandQueueVulkan->GetCompletedFenceValue() : 0ULL;
  }

  [[nodiscard]] QueueSyncData CaptureSubmittedQueueSyncData() const
  {
    QueueSyncData queueSyncData = {};

    queueSyncData.m_uiGraphicsFenceValue = GetSubmittedFenceValue(m_pDeviceVulkan->m_pGraphicsCommandQueue.Borrow());
    queueSyncData.m_uiComputeFenceValue  = GetSubmittedFenceValue(m_pDeviceVulkan->m_pComputeCommandQueue.Borrow());
    queueSyncData.m_uiTransferFenceValue = GetSubmittedFenceValue(m_pDeviceVulkan->m_pTransferCommandQueue.Borrow());

    return queueSyncData;
  }

  [[nodiscard]] QueueSyncData CaptureCompletedQueueSyncData() const
  {
    QueueSyncData queueSyncData = {};

    queueSyncData.m_uiGraphicsFenceValue = GetCompletedFenceValue(m_pDeviceVulkan->m_pGraphicsCommandQueue.Borrow());
    queueSyncData.m_uiComputeFenceValue  = GetCompletedFenceValue(m_pDeviceVulkan->m_pComputeCommandQueue.Borrow());
    queueSyncData.m_uiTransferFenceValue = GetCompletedFenceValue(m_pDeviceVulkan->m_pTransferCommandQueue.Borrow());

    return queueSyncData;
  }

  [[nodiscard]] static bool IsReadyForDeletion(const DeletionEntry& entry, const QueueSyncData& completedQueueSyncData)
  {
    return completedQueueSyncData.m_uiGraphicsFenceValue >= entry.m_QueueSyncData.m_uiGraphicsFenceValue && completedQueueSyncData.m_uiComputeFenceValue >= entry.m_QueueSyncData.m_uiComputeFenceValue && completedQueueSyncData.m_uiTransferFenceValue >= entry.m_QueueSyncData.m_uiTransferFenceValue;
  }

  void DestroyEntry(vk::Device vkLogicalDevice, DeletionEntry& entry)
  {
    if (entry.m_pSemaphorePool != nullptr)
    {
      DestroySemaphore(entry.m_pSemaphorePool, std::move(entry.m_vkSemaphore));
    }
    else if (entry.m_pDescriptorSetPool != nullptr)
    {
      DestroyDescriptorSetPool(entry.m_pDescriptorSetPool, std::move(entry.m_vkDescriptorPool));
    }
    else if (entry.m_pFencePool != nullptr)
    {
      DestroyFence(entry.m_pFencePool, std::move(entry.m_vkFence));
    }
    else if (entry.m_vkExternalMemory != VK_NULL_HANDLE)
    {
      DestroyObject(entry.m_vkObjectType, entry.m_pObject, entry.m_vkExternalMemory);
    }
    else if (entry.m_VulkanAllocation != VK_NULL_HANDLE)
    {
      DestroyObject(entry.m_vkObjectType, entry.m_pObject, entry.m_VulkanAllocation);
    }
    else
    {
      DestroyObject(vkLogicalDevice, entry.m_vkObjectType, entry.m_pObject);
    }
  }

  void DestroyObject(vk::Device vkLogicalDevice, vk::ObjectType vkObjectType, void* pObject)
  {
    switch (vkObjectType)
    {
      case vk::ObjectType::eSemaphore:
      {
        vkLogicalDevice.destroySemaphore(reinterpret_cast<vk::Semaphore&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eFence:
      {
        vkLogicalDevice.destroyFence(reinterpret_cast<vk::Fence&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eBuffer:
      {
        XII_REPORT_FAILURE("Buffer must be destroyed using the Vulkan memory allocator or external memory semantics.");
      }
      break;
      case vk::ObjectType::eImage:
      {
        XII_REPORT_FAILURE("Image must be destroyed using the Vulkan memory allocator or external memory semantics.");
      }
      break;
      case vk::ObjectType::eEvent:
      {
        vkLogicalDevice.destroyEvent(reinterpret_cast<vk::Event&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eQueryPool:
      {
        vkLogicalDevice.destroyQueryPool(reinterpret_cast<vk::QueryPool&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eBufferView:
      {
        vkLogicalDevice.destroyBufferView(reinterpret_cast<vk::BufferView&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eImageView:
      {
        vkLogicalDevice.destroyImageView(reinterpret_cast<vk::ImageView&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eShaderModule:
      {
        vkLogicalDevice.destroyShaderModule(reinterpret_cast<vk::ShaderModule&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::ePipelineCache:
      {
        vkLogicalDevice.destroyPipelineCache(reinterpret_cast<vk::PipelineCache&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::ePipelineLayout:
      {
        vkLogicalDevice.destroyPipelineLayout(reinterpret_cast<vk::PipelineLayout&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eRenderPass:
      {
        vkLogicalDevice.destroyRenderPass(reinterpret_cast<vk::RenderPass&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::ePipeline:
      {
        vkLogicalDevice.destroyPipeline(reinterpret_cast<vk::Pipeline&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eDescriptorSetLayout:
      {
        vkLogicalDevice.destroyDescriptorSetLayout(reinterpret_cast<vk::DescriptorSetLayout&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eSampler:
      {
        vkLogicalDevice.destroySampler(reinterpret_cast<vk::Sampler&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eDescriptorPool:
      {
        vkLogicalDevice.destroyDescriptorPool(reinterpret_cast<vk::DescriptorPool&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eFramebuffer:
      {
        vkLogicalDevice.destroyFramebuffer(reinterpret_cast<vk::Framebuffer&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eCommandPool:
      {
        vkLogicalDevice.destroyCommandPool(reinterpret_cast<vk::CommandPool&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eAccelerationStructureKHR:
      {
        vkLogicalDevice.destroyAccelerationStructureKHR(reinterpret_cast<vk::AccelerationStructureKHR&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  void DestroyObject(vk::ObjectType vkObjectType, void* pObject, xiiVulkanAllocation allocation)
  {
    switch (vkObjectType)
    {
      case vk::ObjectType::eBuffer:
      {
        m_pDeviceVulkan->m_pVulkanMemoryAllocator->DestroyBuffer(reinterpret_cast<vk::Buffer&>(pObject), allocation);
      }
      break;
      case vk::ObjectType::eImage:
      {
        m_pDeviceVulkan->m_pVulkanMemoryAllocator->DestroyImage(reinterpret_cast<vk::Image&>(pObject), allocation);
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  void DestroyObject(vk::ObjectType vkObjectType, void* pObject, vk::DeviceMemory vkExternalMemory)
  {
    vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();
    switch (vkObjectType)
    {
      case vk::ObjectType::eBuffer:
      {
        vkLogicalDevice.destroyBuffer(reinterpret_cast<vk::Buffer&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        vkLogicalDevice.freeMemory(vkExternalMemory, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;
      case vk::ObjectType::eImage:
      {
        vkLogicalDevice.destroyImage(reinterpret_cast<vk::Image&>(pObject), nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
        vkLogicalDevice.freeMemory(vkExternalMemory, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  static void DestroySemaphore(xiiGALSemaphorePoolVulkan* pSemaphorePool, vk::Semaphore&& vkSemaphore)
  {
    pSemaphorePool->ReclaimSemaphore(std::move(vkSemaphore));
  }

  static void DestroyFence(xiiGALFencePoolVulkan* pFencePool, vk::Fence&& vkReclaimFence)
  {
    pFencePool->ReclaimFence(std::move(vkReclaimFence));
  }

  static void DestroyDescriptorSetPool(xiiGALDescriptorSetPoolVulkan* pDescriptorSetPool, vk::DescriptorPool&& vkDescriptorPool)
  {
    pDescriptorSetPool->ReclaimDescriptorPool(std::move(vkDescriptorPool));
  }

  xiiGALDeviceVulkan*     m_pDeviceVulkan = nullptr;
  xiiDeque<DeletionEntry> m_DeletionQueue;
  mutable xiiMutex        m_DeletionQueueMutex;
};

///////////////////////////////////////////////////////////////////////////

xiiGALDeviceVulkan::xiiGALDeviceVulkan(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description) :
  xiiGALDevice(pAllocator, description), m_PhysicalDeviceQueueFamilyProperties(GetAllocator()), m_PhysicalDeviceSupportedExtensions(GetAllocator()), m_LogicalDeviceEnabledExtensions(GetAllocator()), m_LogicalDeviceSupportedStagesFlags(GetAllocator()), m_LogicalDeviceSupportedAccessFlags(GetAllocator())
{
}

xiiGALDeviceVulkan::~xiiGALDeviceVulkan()
{
  WaitIdlePlatform();

  XII_ASSERT_DEV(m_pDeferredDeletionQueue->IsEmpty(), "There should be no pending deferred deletion resources at this stage.");

  m_pTransferCommandQueue.Clear();
  m_pTransferCommandBufferPool.Clear();
  m_pTransferCommandQueueQueryPool.Clear();

  m_pComputeCommandQueue.Clear();
  m_pComputeCommandBufferPool.Clear();
  m_pComputeCommandQueueQueryPool.Clear();

  m_pGraphicsCommandQueue.Clear();
  m_pGraphicsCommandBufferPool.Clear();
  m_pGraphicsCommandQueueQueryPool.Clear();

  m_pDeferredDeletionQueue.Clear();
  m_pFencePool.Clear();
  m_pSemaphorePool.Clear();

  m_pVulkanMemoryAllocator.Clear();

  if (m_LogicalDevice != VK_NULL_HANDLE)
  {
    m_LogicalDevice.destroy(nullptr, m_InstanceDispatchLoader);
  }

  if (m_Instance != VK_NULL_HANDLE)
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (m_InstanceFlags.m_DebugMode != DebugMode::Disabled)
    {
      if (m_DebugMessenger != VK_NULL_HANDLE)
      {
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_InstanceDispatchLoader);
      }

      if (m_DebugCallback != VK_NULL_HANDLE)
      {
        m_Instance.destroyDebugReportCallbackEXT(m_DebugCallback, nullptr, m_InstanceDispatchLoader);
      }
    }
#endif

    m_Instance.destroy(nullptr, m_InstanceDispatchLoader);
  }
}

xiiResult xiiGALDeviceVulkan::InitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceVulkan::InitializePlatform");

  m_InstanceDispatchLoader.init();

  // Enumerate available instance extensions.
  xiiTemporaryArray<vk::ExtensionProperties> extensionProperties;
  {
    XII_LOG_BLOCK("Supported Vulkan Instance Extensions");

    xiiUInt32 uiExtensionCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, nullptr, m_InstanceDispatchLoader));

    extensionProperties.SetCountUninitialized(uiExtensionCount);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, extensionProperties.GetData(), m_InstanceDispatchLoader));

    XII_ASSERT_DEV(extensionProperties.GetCount() == uiExtensionCount, "Expected extension count ({0}) does not match the retrieved extension count ({1}).", uiExtensionCount, extensionProperties.GetCount());

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    for (const vk::ExtensionProperties& extension : extensionProperties)
    {
      xiiLog::Dev("{} {}.{}.{}", extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }
#endif
  }

#if XII_ENABLED(XII_PLATFORM_OSX)
  // From the 1.3.216 Vulkan SDK and later, the Vulkan Loader is strictly enforcing the new VK_KHR_PORTABILITY_subset extension.
  constexpr bool bUsePortabilityEnumeration = true;
#else
  constexpr bool bUsePortabilityEnumeration = false;
#endif

  xiiTemporaryHybridArray<const char*, 6U> instanceExtensions;
  {
    if (IsExtensionAvailable(extensionProperties, VK_KHR_SURFACE_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);

      // Enable surface extensions depending on build configuration.
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      instanceExtensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
      instanceExtensions.PushBack(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
      instanceExtensions.PushBack(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
      instanceExtensions.PushBack(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif
    }

    if (bUsePortabilityEnumeration)
    {
      instanceExtensions.PushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }

    // This extension added to core in 1.1, but current version is 1.0.
    if (IsExtensionAvailable(extensionProperties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    // External memory extensions (required for interop with other APIs).
    if (IsExtensionAvailable(extensionProperties, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);

      m_InstanceFlags.m_bExternalFenceCapabilities = true;
    }
    if (IsExtensionAvailable(extensionProperties, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);

      m_InstanceFlags.m_bExternalSemaphoreCapabilities = true;
    }
    if (IsExtensionAvailable(extensionProperties, VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME))
    {
      instanceExtensions.PushBack(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);

      m_InstanceFlags.m_bExternalMemoryCapabilities = true;
    }

    for (const auto* szExtensionName : instanceExtensions)
    {
      XII_SUCCEED_OR_RETURN_FAILURE(IsExtensionAvailable(extensionProperties, szExtensionName), "Required extension ({}) is not available.", szExtensionName);
    }
  }


  // Add required instance extensions and layers for debugging if validation is enabled.
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiTemporaryHybridArray<const char*, 6U> instanceLayers;
  {
    xiiTemporaryArray<vk::LayerProperties> layerProperties;

    xiiUInt32 uiLayerCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceLayerProperties(&uiLayerCount, nullptr, m_InstanceDispatchLoader));

    layerProperties.SetCountUninitialized(uiLayerCount);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceLayerProperties(&uiLayerCount, layerProperties.GetData(), m_InstanceDispatchLoader));

    XII_ASSERT_DEV(layerProperties.GetCount() == uiLayerCount, "Expected layer count ({0}) does not match the retrieved layer count ({1}).", uiLayerCount, layerProperties.GetCount());

    {
      XII_LOG_BLOCK("Available Vulkan Instance Layers");

      for (const vk::LayerProperties& layer : layerProperties)
      {
        xiiLog::Dev("{} {}.{}.{}", layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion));
      }
    }

    if (m_Description.m_ValidationLevel > xiiGALDeviceValidationLevel::Disabled)
    {
      if (IsExtensionAvailable(extensionProperties, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
      {
        // Prefer VK_EXT_debug_utils.
        m_InstanceFlags.m_DebugMode = DebugMode::Utils;
      }
      else if (IsExtensionAvailable(extensionProperties, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
      {
        // If debug utils are unavailable, use VK_EXT_debug_report.
        m_InstanceFlags.m_DebugMode = DebugMode::Report;
      }

      const char* validationLayerNames[] = {
        "VK_LAYER_KHRONOS_validation", // Unified validation layer used on Desktop and Mobile platforms.
      };

      for (const char* szValidationLayerName : validationLayerNames)
      {
        xiiUInt32 uiLayerVersion = 0xFFFFFFFFU;
        if (!IsLayerAvailable(layerProperties, szValidationLayerName, &uiLayerVersion))
        {
          xiiLog::Error("Instance layer ({0}) is not available.", szValidationLayerName);
          continue;
        }

        // Beta extensions may vary and result in a crash.
        // New enums are not supported and may cause validation error.
        if (uiLayerVersion < VK_HEADER_VERSION_COMPLETE)
        {
          xiiLog::Warning("Layer '{}' version ({}.{}.{}) is less than the header version ({}.{}.{}).", szValidationLayerName, VK_API_VERSION_MAJOR(uiLayerVersion), VK_API_VERSION_MINOR(uiLayerVersion), VK_API_VERSION_PATCH(uiLayerVersion),
                          VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE), VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));
        }

        instanceLayers.PushBack(szValidationLayerName);

        if (m_InstanceFlags.m_DebugMode != DebugMode::Utils)
        {
          // VK_EXT_debug_utils extension may not be supported by the loader, but supported by the layer.

          xiiTemporaryHybridArray<vk::ExtensionProperties, 4U> layerExtensions;
          if (EnumerateInstanceExtensions(szValidationLayerName, layerExtensions))
          {
            if (IsExtensionAvailable(layerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
              m_InstanceFlags.m_DebugMode = DebugMode::Utils;
            }

            if (m_InstanceFlags.m_DebugMode == DebugMode::Disabled && IsExtensionAvailable(layerExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
            {
              m_InstanceFlags.m_DebugMode = DebugMode::Report;
            }
          }
          else
          {
            xiiLog::Error("Failed to enumerate extensions for {} layer.", szValidationLayerName);
          }
        }
      }

      if (m_InstanceFlags.m_DebugMode == DebugMode::Utils)
      {
        instanceExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
      else if (m_InstanceFlags.m_DebugMode == DebugMode::Report)
      {
        instanceExtensions.PushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      }
      else
      {
        xiiLog::Error("Neither {} nor {} extension is available. Debug tools (validation layer message logging, performance markers, etc.) will be disabled.", VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        m_InstanceFlags.m_DebugMode     = DebugMode::Disabled;
        m_Description.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
      }
    }
  }
#endif

  // Create Vulkan Instance.
  {
    // Find instance maximum available version.
    {
      if (m_InstanceDispatchLoader.vkEnumerateInstanceVersion != nullptr)
      {
        // If the implementation is available, this call must return vk::Result::eSuccess.

        VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::enumerateInstanceVersion(&m_uiVulkanVersion, m_InstanceDispatchLoader));
      }
      else
      {
        // Only Vulkan 1.0 is supported.
        m_uiVulkanVersion = VK_API_VERSION_1_0;
      }
    }

    vk::ApplicationInfo applicationInformation = {};
    applicationInformation.sType               = vk::StructureType::eApplicationInfo;
    applicationInformation.apiVersion          = m_uiVulkanVersion;
    applicationInformation.applicationVersion  = VK_MAKE_VERSION(BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH);
    applicationInformation.engineVersion       = VK_MAKE_VERSION(BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH);
    applicationInformation.pApplicationName    = "XII";
    applicationInformation.pEngineName         = "XII";
    applicationInformation.pNext               = nullptr;

    vk::InstanceCreateInfo instanceCreateInformation  = {};
    instanceCreateInformation.sType                   = vk::StructureType::eInstanceCreateInfo;
    instanceCreateInformation.pNext                   = nullptr;
    instanceCreateInformation.pApplicationInfo        = &applicationInformation;
    instanceCreateInformation.enabledExtensionCount   = instanceExtensions.GetCount();
    instanceCreateInformation.ppEnabledExtensionNames = instanceExtensions.GetData();
    instanceCreateInformation.flags                   = {};

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    instanceCreateInformation.enabledLayerCount   = instanceLayers.GetCount();
    instanceCreateInformation.ppEnabledLayerNames = instanceLayers.GetData();
#endif

    if (bUsePortabilityEnumeration)
    {
      // The instance will enumerate available Vulkan Portability-compliant physical devices and groups in addition to the Vulkan physical devices and groups that
      // are enumerated by default.
      instanceCreateInformation.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vk::createInstance(&instanceCreateInformation, nullptr, &m_Instance, m_InstanceDispatchLoader));

    m_InstanceDispatchLoader.init(m_Instance);

    m_EnabledExtensions = instanceExtensions;
  }

  // If requested, we enable the default validation layers for debugging purposes.
  if (m_InstanceFlags.m_DebugMode == DebugMode::Utils)
  {
    constexpr vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    constexpr vk::DebugUtilsMessageTypeFlagsEXT     messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
    debugMessengerCreateInfo.pNext                                = nullptr;
    debugMessengerCreateInfo.flags                                = {};
    debugMessengerCreateInfo.messageSeverity                      = messageSeverity;
    debugMessengerCreateInfo.messageType                          = messageType;
    debugMessengerCreateInfo.pfnUserCallback                      = reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(xiiVulkanDebugMessengerCallback);
    debugMessengerCreateInfo.pUserData                            = nullptr;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.createDebugUtilsMessengerEXT(&debugMessengerCreateInfo, nullptr, &m_DebugMessenger, m_InstanceDispatchLoader));
  }
  else if (m_InstanceFlags.m_DebugMode == DebugMode::Report)
  {
    constexpr vk::DebugReportFlagsEXT reportFlags = vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eError;

    vk::DebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
    debugReportCallbackCreateInfo.pNext                                = nullptr;
    debugReportCallbackCreateInfo.flags                                = reportFlags;
    debugReportCallbackCreateInfo.pfnCallback                          = reinterpret_cast<vk::PFN_DebugReportCallbackEXT>(xiiVulkanDebugReportCallback);
    debugReportCallbackCreateInfo.pUserData                            = nullptr;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.createDebugReportCallbackEXT(&debugReportCallbackCreateInfo, nullptr, &m_DebugCallback, m_InstanceDispatchLoader));
  }

  // Enumerate physical devices and select a suitable device.
  {
    xiiTemporaryHybridArray<vk::PhysicalDevice, 2U> physicalDevices;

    xiiUInt32 uiPhysicalDeviceCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.enumeratePhysicalDevices(&uiPhysicalDeviceCount, nullptr, m_InstanceDispatchLoader));

    XII_ASSERT_ALWAYS(uiPhysicalDeviceCount != 0U, "No physical devices are found on the system.");

    physicalDevices.SetCountUninitialized(uiPhysicalDeviceCount);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_Instance.enumeratePhysicalDevices(&uiPhysicalDeviceCount, physicalDevices.GetData(), m_InstanceDispatchLoader));

    XII_ASSERT_DEV(physicalDevices.GetCount() == uiPhysicalDeviceCount, "Expected physical device count ({0}) does not match the retrieved physical device count ({1}).", uiPhysicalDeviceCount, physicalDevices.GetCount());

    m_PhysicalDevice = SelectPhysicalDevice(physicalDevices, m_Description.m_uiAdapterID);

    m_PhysicalDevice.getProperties(&m_PhysicalDeviceProperties, m_InstanceDispatchLoader);
    m_PhysicalDevice.getFeatures(&m_PhysicalDeviceFeatures, m_InstanceDispatchLoader);
    m_PhysicalDevice.getMemoryProperties(&m_PhysicalDeviceMemoryProperties, m_InstanceDispatchLoader);

    xiiUInt32 uiQueueFamilyCount = 0U;
    m_PhysicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, nullptr, m_InstanceDispatchLoader);

    XII_ASSERT_DEV(uiQueueFamilyCount > 0U, "");

    m_PhysicalDeviceQueueFamilyProperties.SetCountUninitialized(uiQueueFamilyCount);
    m_PhysicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, m_PhysicalDeviceQueueFamilyProperties.GetData(), m_InstanceDispatchLoader);

    XII_ASSERT_DEV(m_PhysicalDeviceQueueFamilyProperties.GetCount() == uiQueueFamilyCount, "");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    {
      xiiStringBuilder sb;
      sb.SetFormat("Device '{}' Queue Families", m_PhysicalDeviceProperties.deviceName);

      XII_LOG_BLOCK(sb);

      for (const vk::QueueFamilyProperties& queueFamilyProperty : m_PhysicalDeviceQueueFamilyProperties)
      {
        xiiLog::Dev("Queue Count: {},  Flags: {}", queueFamilyProperty.queueCount, vk::to_string(queueFamilyProperty.queueFlags).data());
      }
    }
#endif

    // Get list of supported extensions.
    xiiUInt32 uiExtensionCount = 0U;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &uiExtensionCount, nullptr, m_InstanceDispatchLoader));

    if (uiExtensionCount > 0U)
    {
      m_PhysicalDeviceSupportedExtensions.SetCountUninitialized(uiExtensionCount);

      VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &uiExtensionCount, m_PhysicalDeviceSupportedExtensions.GetData(), m_InstanceDispatchLoader));

      XII_ASSERT_DEV(m_PhysicalDeviceSupportedExtensions.GetCount() == uiExtensionCount, "");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      {
        xiiStringBuilder sb;
        sb.SetFormat("Device '{}' Supported Extensions", m_PhysicalDeviceProperties.deviceName);

        XII_LOG_BLOCK(sb);

        for (const vk::ExtensionProperties& extensionProperty : m_PhysicalDeviceSupportedExtensions)
        {
          xiiLog::Dev("{} {}.{}.{}", extensionProperty.extensionName, VK_API_VERSION_MAJOR(extensionProperty.specVersion), VK_API_VERSION_MINOR(extensionProperty.specVersion), VK_API_VERSION_PATCH(extensionProperty.specVersion));
        }
      }
#endif
    }

    if (m_PhysicalDevice != VK_NULL_HANDLE)
    {
      xiiLog::Dev("Using physical device '{}', API version {}.{}.{}, Driver version {}.{}.{}.", m_PhysicalDeviceProperties.deviceName,
                  VK_API_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion), VK_API_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_API_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion),
                  VK_API_VERSION_MAJOR(m_PhysicalDeviceProperties.driverVersion), VK_API_VERSION_MINOR(m_PhysicalDeviceProperties.driverVersion), VK_API_VERSION_PATCH(m_PhysicalDeviceProperties.driverVersion));
    }
    else
    {
      xiiLog::Error("Failed to find suitable Vulkan physical device.");

      return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(InitializePhysicalDeviceProperties());
  }

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceVulkan::PostInitializePlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceVulkan::PostInitializePlatform");

  xiiTemporaryHybridArray<const char*, 4U> deviceExtensions;

  if (IsExtensionEnabled(m_EnabledExtensions, VK_KHR_SURFACE_EXTENSION_NAME))
  {
    deviceExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MAINTENANCE1_EXTENSION_NAME))
  {
    deviceExtensions.PushBack(VK_KHR_MAINTENANCE1_EXTENSION_NAME); // To allow negative viewport height.
  }
  else
  {
    xiiLog::Warning("{} is not supported.", VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  }

  xiiTemporaryHybridArray<vk::DeviceQueueCreateInfo, 4U> queueDescriptions;
  float                                                  fQueuePriorities = 1.0f;

  // Setup device queues.
  {
    xiiTemporaryHybridArray<xiiUInt32, 3U> excludedQueueIndices;

    // If an implementation exposes any queue family that supports graphics operations, at least one queue family of at least one physical device exposed by the implementation
    // must support both graphics and compute operations.

    xiiUInt32 uiGraphicsQueueIndex = FindQueueFamily(vk::QueueFlagBits::eGraphics, excludedQueueIndices);
    if (uiGraphicsQueueIndex != xiiInvalidIndex)
    {
      XII_ASSERT_DEV(!excludedQueueIndices.Contains(uiGraphicsQueueIndex), "");

      excludedQueueIndices.PushBack(uiGraphicsQueueIndex);

      vk::DeviceQueueCreateInfo& queueDescription = queueDescriptions.ExpandAndGetRef();
      queueDescription.pNext                      = nullptr;
      queueDescription.flags                      = {};
      queueDescription.queueFamilyIndex           = uiGraphicsQueueIndex;
      queueDescription.queueCount                 = 1U;
      queueDescription.pQueuePriorities           = &fQueuePriorities;

      m_GraphicsQueueInformation.m_uiQueueFamilyIndex = uiGraphicsQueueIndex;
      m_ActiveQueueFamilyIndices.PushBack(uiGraphicsQueueIndex);
    }
    else
    {
      xiiLog::Error("Failed to locate a valid Vulkan queue family index for {}.", vk::to_string(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute).data());
      return XII_FAILURE;
    }

    if (m_AdapterDescription.m_Features.m_NativeFence != xiiGALDeviceFeatureState::Disabled && IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
    {
      xiiUInt32 uiComputeQueueIndex = FindQueueFamily(vk::QueueFlagBits::eCompute, excludedQueueIndices);
      if (uiComputeQueueIndex != xiiInvalidIndex)
      {
        XII_ASSERT_DEV(!excludedQueueIndices.Contains(uiComputeQueueIndex), "");

        excludedQueueIndices.PushBack(uiComputeQueueIndex);

        vk::DeviceQueueCreateInfo& queueDescription = queueDescriptions.ExpandAndGetRef();
        queueDescription.pNext                      = nullptr;
        queueDescription.flags                      = {};
        queueDescription.queueFamilyIndex           = uiComputeQueueIndex;
        queueDescription.queueCount                 = 1U;
        queueDescription.pQueuePriorities           = &fQueuePriorities;

        m_ComputeQueueInformation.m_uiQueueFamilyIndex = uiComputeQueueIndex;
        m_ActiveQueueFamilyIndices.PushBack(uiComputeQueueIndex);
      }

      xiiUInt32 uiTransferQueueIndex = FindQueueFamily(vk::QueueFlagBits::eTransfer, excludedQueueIndices);
      if (uiTransferQueueIndex != xiiInvalidIndex)
      {
        XII_ASSERT_DEV(!excludedQueueIndices.Contains(uiTransferQueueIndex), "");

        excludedQueueIndices.PushBack(uiTransferQueueIndex);

        vk::DeviceQueueCreateInfo& queueDescription = queueDescriptions.ExpandAndGetRef();
        queueDescription.pNext                      = nullptr;
        queueDescription.flags                      = {};
        queueDescription.queueFamilyIndex           = uiTransferQueueIndex;
        queueDescription.queueCount                 = 1U;
        queueDescription.pQueuePriorities           = &fQueuePriorities;

        m_TransferQueueInformation.m_uiQueueFamilyIndex = uiTransferQueueIndex;
        m_ActiveQueueFamilyIndices.PushBack(uiTransferQueueIndex);
      }
    }
  }

  // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#extended-functionality-device-layer-deprecation
  vk::DeviceCreateInfo deviceCreationDescription = {};
  deviceCreationDescription.flags                = {};
  deviceCreationDescription.queueCreateInfoCount = queueDescriptions.GetCount();
  deviceCreationDescription.pQueueCreateInfos    = queueDescriptions.GetData();

  vk::PhysicalDeviceFeatures vkEnabledFeatures = {};
  deviceCreationDescription.pEnabledFeatures   = &vkEnabledFeatures;

#define ENABLE_VULKAN_FEATURE(vkFeature, state) vkEnabledFeatures.vkFeature = (state == xiiGALDeviceFeatureState::Enabled ? vk::True : vk::False)

  xiiGALDeviceFeatureState::Enum imageCubeArrayFeature    = xiiGALDeviceFeatureState::Optional;
  xiiGALDeviceFeatureState::Enum samplerAnisotropyFeature = xiiGALDeviceFeatureState::Optional;

  // clang-format off
  ENABLE_VULKAN_FEATURE(geometryShader,                    m_AdapterDescription.m_Features.m_GeometryShaders);
  ENABLE_VULKAN_FEATURE(tessellationShader,                m_AdapterDescription.m_Features.m_Tessellation);
  ENABLE_VULKAN_FEATURE(pipelineStatisticsQuery,           m_AdapterDescription.m_Features.m_PipelineStatisticsQueries);
  ENABLE_VULKAN_FEATURE(occlusionQueryPrecise,             m_AdapterDescription.m_Features.m_OcclusionQueries);
  ENABLE_VULKAN_FEATURE(imageCubeArray,                    imageCubeArrayFeature);
  ENABLE_VULKAN_FEATURE(fillModeNonSolid,                  m_AdapterDescription.m_Features.m_WireframeFill);
  ENABLE_VULKAN_FEATURE(samplerAnisotropy,                 samplerAnisotropyFeature);
  ENABLE_VULKAN_FEATURE(depthBiasClamp,                    m_AdapterDescription.m_Features.m_DepthBiasClamp);
  ENABLE_VULKAN_FEATURE(depthClamp,                        m_AdapterDescription.m_Features.m_DepthClamp);
  ENABLE_VULKAN_FEATURE(independentBlend,                  m_AdapterDescription.m_Features.m_IndependentBlend);
  ENABLE_VULKAN_FEATURE(dualSrcBlend,                      m_AdapterDescription.m_Features.m_DualSourceBlend);
  ENABLE_VULKAN_FEATURE(multiViewport,                     m_AdapterDescription.m_Features.m_MultiViewport);
  ENABLE_VULKAN_FEATURE(textureCompressionBC,              m_AdapterDescription.m_Features.m_TextureCompressionBC);
  ENABLE_VULKAN_FEATURE(vertexPipelineStoresAndAtomics,    m_AdapterDescription.m_Features.m_VertexPipelineUAVWritesAndAtomics);
  ENABLE_VULKAN_FEATURE(fragmentStoresAndAtomics,          m_AdapterDescription.m_Features.m_PixelUAVWritesAndAtomics);
  ENABLE_VULKAN_FEATURE(shaderStorageImageExtendedFormats, m_AdapterDescription.m_Features.m_TextureUAVExtendedFormats);
  // clang-format on

#undef ENABLE_VULKAN_FEATURE

  // Enable features (if they are supported) that are not covered by DeviceFeatures but required for some operations.
  vkEnabledFeatures.imageCubeArray                          = m_PhysicalDeviceFeatures.imageCubeArray;
  vkEnabledFeatures.samplerAnisotropy                       = m_PhysicalDeviceFeatures.samplerAnisotropy;
  vkEnabledFeatures.fullDrawIndexUint32                     = m_PhysicalDeviceFeatures.fullDrawIndexUint32;
  vkEnabledFeatures.drawIndirectFirstInstance               = m_PhysicalDeviceFeatures.drawIndirectFirstInstance;
  vkEnabledFeatures.shaderStorageImageMultisample           = m_PhysicalDeviceFeatures.shaderStorageImageMultisample;
  vkEnabledFeatures.shaderStorageImageReadWithoutFormat     = m_PhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat;
  vkEnabledFeatures.shaderStorageImageWriteWithoutFormat    = m_PhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat;
  vkEnabledFeatures.shaderUniformBufferArrayDynamicIndexing = m_PhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing;
  vkEnabledFeatures.shaderSampledImageArrayDynamicIndexing  = m_PhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing;
  vkEnabledFeatures.shaderStorageBufferArrayDynamicIndexing = m_PhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing;
  vkEnabledFeatures.shaderStorageImageArrayDynamicIndexing  = m_PhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing;
  vkEnabledFeatures.shaderImageGatherExtended               = m_PhysicalDeviceFeatures.shaderImageGatherExtended;

  if (m_AdapterDescription.m_Features.m_SparseResources != xiiGALDeviceFeatureState::Disabled)
  {
    vkEnabledFeatures.sparseBinding            = vk::True;
    vkEnabledFeatures.sparseResidency16Samples = m_PhysicalDeviceFeatures.sparseResidency16Samples;
    vkEnabledFeatures.sparseResidency2Samples  = m_PhysicalDeviceFeatures.sparseResidency2Samples;
    vkEnabledFeatures.sparseResidency4Samples  = m_PhysicalDeviceFeatures.sparseResidency4Samples;
    vkEnabledFeatures.sparseResidency8Samples  = m_PhysicalDeviceFeatures.sparseResidency8Samples;
    vkEnabledFeatures.sparseResidencyAliased   = m_PhysicalDeviceFeatures.sparseResidencyAliased;
    vkEnabledFeatures.sparseResidencyBuffer    = m_PhysicalDeviceFeatures.sparseResidencyBuffer;
    vkEnabledFeatures.sparseResidencyImage2D   = m_PhysicalDeviceFeatures.sparseResidencyImage2D;
    vkEnabledFeatures.sparseResidencyImage3D   = m_PhysicalDeviceFeatures.sparseResidencyImage3D;
    vkEnabledFeatures.shaderResourceResidency  = m_PhysicalDeviceFeatures.shaderResourceResidency;
  }

  ExtensionFeatures enabledExtensionFeatures = {};

  // To enable some device extensions you must enable instance extension VK_KHR_get_physical_device_properties2 and add feature description to DeviceCreateInfo.pNext.
  const bool bSupportsDeviceFeatures2 = IsExtensionEnabled(m_EnabledExtensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  if (bSupportsDeviceFeatures2)
  {
    void** pNextExtension = const_cast<void**>(&deviceCreationDescription.pNext);

    // Mesh shader
    if (m_AdapterDescription.m_Features.m_MeshShaders != xiiGALDeviceFeatureState::Disabled)
    {
      // Enable only the mesh/task features represented by the GAL feature bit. Copying the full
      // queried structure also enables optional dependent features such as multiviewMeshShader and
      // primitiveFragmentShadingRateMeshShader without enabling their parent Vulkan features.
      enabledExtensionFeatures.m_MeshShader            = vk::PhysicalDeviceMeshShaderFeaturesEXT{};
      enabledExtensionFeatures.m_MeshShader.taskShader = m_PhysicalDeviceExtensionFeatures.m_MeshShader.taskShader;
      enabledExtensionFeatures.m_MeshShader.meshShader = m_PhysicalDeviceExtensionFeatures.m_MeshShader.meshShader;

      XII_ASSERT_DEV(enabledExtensionFeatures.m_MeshShader.taskShader != vk::False && enabledExtensionFeatures.m_MeshShader.meshShader != vk::False, "");

      const char* szMeshShaderExtensionName = VK_EXT_MESH_SHADER_EXTENSION_NAME;

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, szMeshShaderExtensionName), "{} extension must be supported as it has already been checked by VulkanPhysicalDevice and both taskShader and meshShader features are TRUE.", szMeshShaderExtensionName);

      deviceExtensions.PushBack(szMeshShaderExtensionName);

      *pNextExtension = &enabledExtensionFeatures.m_MeshShader;
      pNextExtension  = &enabledExtensionFeatures.m_MeshShader.pNext;
    }

    if (m_AdapterDescription.m_Features.m_ShaderFloat16 != xiiGALDeviceFeatureState::Disabled || m_AdapterDescription.m_Features.m_ShaderInt8 != xiiGALDeviceFeatureState::Disabled)
    {
      enabledExtensionFeatures.m_ShaderFloat16Int8 = m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8;

      XII_ASSERT_DEV(enabledExtensionFeatures.m_ShaderFloat16Int8.shaderFloat16 != vk::False || enabledExtensionFeatures.m_ShaderFloat16Int8.shaderInt8 != vk::False, "");

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME), "VK_KHR_shader_float16_int8 extension must be supported as it has already been checked by VulkanPhysicalDevice and at least one of shaderFloat16 or shaderInt8 features is TRUE");

      deviceExtensions.PushBack(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);

      if (m_AdapterDescription.m_Features.m_ShaderFloat16 == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_ShaderFloat16Int8.shaderFloat16 = vk::False;
      if (m_AdapterDescription.m_Features.m_ShaderInt8 == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_ShaderFloat16Int8.shaderInt8 = vk::False;

      *pNextExtension = &enabledExtensionFeatures.m_ShaderFloat16Int8;
      pNextExtension  = &enabledExtensionFeatures.m_ShaderFloat16Int8.pNext;
    }

    bool bStorageBufferStorageClassExtensionRequired = false;

    if (m_AdapterDescription.m_Features.m_ResourceBuffer16BitAccess != xiiGALDeviceFeatureState::Disabled || m_AdapterDescription.m_Features.m_UniformBuffer16BitAccess != xiiGALDeviceFeatureState::Disabled || m_AdapterDescription.m_Features.m_ShaderInputOutput16 != xiiGALDeviceFeatureState::Disabled)
    {
      enabledExtensionFeatures.m_Storage16Bit = m_PhysicalDeviceExtensionFeatures.m_Storage16Bit;

      XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_ResourceBuffer16BitAccess == xiiGALDeviceFeatureState::Disabled || enabledExtensionFeatures.m_Storage16Bit.storageBuffer16BitAccess != vk::False, "");
      XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_UniformBuffer16BitAccess == xiiGALDeviceFeatureState::Disabled || enabledExtensionFeatures.m_Storage16Bit.uniformAndStorageBuffer16BitAccess != vk::False, "");
      XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_ShaderInputOutput16 == xiiGALDeviceFeatureState::Disabled || enabledExtensionFeatures.m_Storage16Bit.storageInputOutput16 != vk::False, "");

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME), "VK_KHR_16bit_storage must be supported as it has already been checked by VulkanPhysicalDevice and at least one of storageBuffer16BitAccess, uniformAndStorageBuffer16BitAccess, or storagePushConstant16 features is TRUE");

      deviceExtensions.PushBack(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);

      // VK_KHR_16bit_storage extension requires VK_KHR_storage_buffer_storage_class extension.
      // All required extensions for each extension in the vk::DeviceCreateInfo::ppEnabledExtensionNames list must also be present in that list.
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME), "VK_KHR_storage_buffer_storage_class must be supported as it has already been checked by VulkanPhysicalDevice and at least one of storageBuffer16BitAccess, uniformAndStorageBuffer16BitAccess, or storagePushConstant16 features is TRUE");

      bStorageBufferStorageClassExtensionRequired = true;

      vkEnabledFeatures.shaderInt16 = vk::True;

      if (m_AdapterDescription.m_Features.m_ResourceBuffer16BitAccess == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_Storage16Bit.storageBuffer16BitAccess = vk::False;

      if (m_AdapterDescription.m_Features.m_UniformBuffer16BitAccess == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_Storage16Bit.uniformAndStorageBuffer16BitAccess = vk::False;

      if (m_AdapterDescription.m_Features.m_ShaderInputOutput16 == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_Storage16Bit.storageInputOutput16 = vk::False;

      *pNextExtension = &enabledExtensionFeatures.m_Storage16Bit;
      pNextExtension  = &enabledExtensionFeatures.m_Storage16Bit.pNext;
    }

    if (m_AdapterDescription.m_Features.m_ResourceBuffer8BitAccess != xiiGALDeviceFeatureState::Disabled || m_AdapterDescription.m_Features.m_UniformBuffer8BitAccess != xiiGALDeviceFeatureState::Disabled)
    {
      enabledExtensionFeatures.m_Storage8Bit = m_PhysicalDeviceExtensionFeatures.m_Storage8Bit;

      XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_ResourceBuffer8BitAccess == xiiGALDeviceFeatureState::Disabled || enabledExtensionFeatures.m_Storage8Bit.storageBuffer8BitAccess != vk::False, "");
      XII_ASSERT_DEV(m_AdapterDescription.m_Features.m_UniformBuffer8BitAccess == xiiGALDeviceFeatureState::Disabled || enabledExtensionFeatures.m_Storage8Bit.uniformAndStorageBuffer8BitAccess != vk::False, "");

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME), "VK_KHR_8bit_storage must be supported as it has already been checked by VulkanPhysicalDevice and at least one of storageBuffer8BitAccess or uniformAndStorageBuffer8BitAccess features is TRUE");

      deviceExtensions.PushBack(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);

      // VK_KHR_8bit_storage extension requires VK_KHR_storage_buffer_storage_class extension.
      // All required extensions for each extension in the vk::DeviceCreateInfo::ppEnabledExtensionNames list must also be present in that list.
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME), "VK_KHR_storage_buffer_storage_class must be supported as it has already been checked by VulkanPhysicalDevice and at least one of storageBuffer8BitAccess or uniformAndStorageBuffer8BitAccess features is TRUE");

      bStorageBufferStorageClassExtensionRequired = true;

      if (m_AdapterDescription.m_Features.m_ResourceBuffer8BitAccess == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_Storage8Bit.storageBuffer8BitAccess = vk::False;

      if (m_AdapterDescription.m_Features.m_UniformBuffer8BitAccess == xiiGALDeviceFeatureState::Disabled)
        enabledExtensionFeatures.m_Storage8Bit.uniformAndStorageBuffer8BitAccess = vk::False;

      *pNextExtension = &enabledExtensionFeatures.m_Storage8Bit;
      pNextExtension  = &enabledExtensionFeatures.m_Storage8Bit.pNext;
    }

    if (bStorageBufferStorageClassExtensionRequired)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME), "VK_KHR_storage_buffer_storage_class extension must be supported");

      deviceExtensions.PushBack(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
    }

    if (m_AdapterDescription.m_Features.m_ShaderResourceRuntimeArray != xiiGALDeviceFeatureState::Disabled || m_AdapterDescription.m_Features.m_RayTracing != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MAINTENANCE3_EXTENSION_NAME), "VK_KHR_maintenance3 extension must be supported");
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME), "VK_EXT_descriptor_indexing extension must be supported");

      deviceExtensions.PushBack(VK_KHR_MAINTENANCE3_EXTENSION_NAME); // Required for VK_EXT_descriptor_indexing
      deviceExtensions.PushBack(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

      enabledExtensionFeatures.m_DescriptorIndexing = m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing;

      XII_ASSERT_DEV(enabledExtensionFeatures.m_DescriptorIndexing.runtimeDescriptorArray != vk::False, "");

      *pNextExtension = &enabledExtensionFeatures.m_DescriptorIndexing;
      pNextExtension  = &enabledExtensionFeatures.m_DescriptorIndexing.pNext;
    }

    // Ray tracing
    if (m_AdapterDescription.m_Features.m_RayTracing != xiiGALDeviceFeatureState::Disabled)
    {
      // This extensions added to Vulkan 1.2 core
      if (!m_PhysicalDeviceExtensionFeatures.m_bSpirv15)
      {
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME), "VK_KHR_shader_float_controls extension must be supported");
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME), "VK_KHR_spirv_1_4 extension must be supported");

        deviceExtensions.PushBack(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME); // Required for VK_KHR_spirv_1_4
        deviceExtensions.PushBack(VK_KHR_SPIRV_1_4_EXTENSION_NAME);             // Required for VK_KHR_ray_tracing_pipeline or VK_KHR_ray_query

        enabledExtensionFeatures.m_bSpirv14 = m_PhysicalDeviceExtensionFeatures.m_bSpirv14;

        XII_ASSERT_DEV(m_PhysicalDeviceExtensionFeatures.m_bSpirv14, "");
      }

      // SPIRV 1.5 is in Vulkan 1.2 core
      enabledExtensionFeatures.m_bSpirv15 = m_PhysicalDeviceExtensionFeatures.m_bSpirv15;

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME), "VK_KHR_buffer_device_address extension must be supported");
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME), "VK_KHR_deferred_host_operations extension must be supported");
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME), "VK_KHR_acceleration_structure extension must be supported");

      deviceExtensions.PushBack(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);    // Required for VK_KHR_acceleration_structure
      deviceExtensions.PushBack(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME); // Required for VK_KHR_acceleration_structure
      deviceExtensions.PushBack(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);   // Required for ray tracing

      enabledExtensionFeatures.m_AccelerationStructure = m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure;
      enabledExtensionFeatures.m_BufferDeviceAddress   = m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress;

      // Disable unused features
      enabledExtensionFeatures.m_AccelerationStructure.accelerationStructureCaptureReplay                    = false;
      enabledExtensionFeatures.m_AccelerationStructure.accelerationStructureHostCommands                     = false;
      enabledExtensionFeatures.m_AccelerationStructure.descriptorBindingAccelerationStructureUpdateAfterBind = false;
      enabledExtensionFeatures.m_AccelerationStructure.accelerationStructureIndirectBuild                    = false;

      *pNextExtension = &enabledExtensionFeatures.m_AccelerationStructure;
      pNextExtension  = &enabledExtensionFeatures.m_AccelerationStructure.pNext;
      *pNextExtension = &enabledExtensionFeatures.m_BufferDeviceAddress;
      pNextExtension  = &enabledExtensionFeatures.m_BufferDeviceAddress.pNext;

      // Ray tracing shader.
      if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) && m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline == vk::True)
      {
        deviceExtensions.PushBack(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

        enabledExtensionFeatures.m_RayTracingPipeline = m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline;

        // Disable unused features
        enabledExtensionFeatures.m_RayTracingPipeline.rayTracingPipelineShaderGroupHandleCaptureReplay      = false;
        enabledExtensionFeatures.m_RayTracingPipeline.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false;

        *pNextExtension = &enabledExtensionFeatures.m_RayTracingPipeline;
        pNextExtension  = &enabledExtensionFeatures.m_RayTracingPipeline.pNext;
      }

      // Inline ray tracing from any shader.
      if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_RAY_QUERY_EXTENSION_NAME) && m_PhysicalDeviceExtensionFeatures.m_RayQuery.rayQuery == vk::True)
      {
        deviceExtensions.PushBack(VK_KHR_RAY_QUERY_EXTENSION_NAME);

        enabledExtensionFeatures.m_RayQuery = m_PhysicalDeviceExtensionFeatures.m_RayQuery;

        *pNextExtension = &enabledExtensionFeatures.m_RayQuery;
        pNextExtension  = &enabledExtensionFeatures.m_RayQuery.pNext;
      }
    }

    if (m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset)
    {
      enabledExtensionFeatures.m_bHasPortabilitySubset = m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset;
      enabledExtensionFeatures.m_PortabilitySubset     = m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset;

      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME), "VK_KHR_portability_subset extension must be supported");

      deviceExtensions.PushBack(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);

      *pNextExtension = &enabledExtensionFeatures.m_PortabilitySubset;
      pNextExtension  = &enabledExtensionFeatures.m_PortabilitySubset.pNext;
    }

    if (m_AdapterDescription.m_Features.m_WaveOperation != xiiGALDeviceFeatureState::Disabled)
    {
      enabledExtensionFeatures.m_bSubgroupOps = true;
    }

    if (m_AdapterDescription.m_Features.m_InstanceDataStepRate != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME);

      enabledExtensionFeatures.m_VertexAttributeDivisor = m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor;

      *pNextExtension = &enabledExtensionFeatures.m_VertexAttributeDivisor;
      pNextExtension  = &enabledExtensionFeatures.m_VertexAttributeDivisor.pNext;
    }

    if (m_AdapterDescription.m_Features.m_NativeFence != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

      enabledExtensionFeatures.m_TimelineSemaphore = m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore;

      *pNextExtension = &enabledExtensionFeatures.m_TimelineSemaphore;
      pNextExtension  = &enabledExtensionFeatures.m_TimelineSemaphore.pNext;
    }

    if (m_AdapterDescription.m_Features.m_TransferQueueTimestampQueries != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);

      enabledExtensionFeatures.m_HostQueryReset = m_PhysicalDeviceExtensionFeatures.m_HostQueryReset;

      *pNextExtension = &enabledExtensionFeatures.m_HostQueryReset;
      pNextExtension  = &enabledExtensionFeatures.m_HostQueryReset.pNext;
    }

    if (m_AdapterDescription.m_Features.m_VariableRateShading != xiiGALDeviceFeatureState::Disabled)
    {
      if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate != vk::False || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.primitiveFragmentShadingRate != vk::False || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
      {
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MAINTENANCE2_EXTENSION_NAME), "");
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MULTIVIEW_EXTENSION_NAME), "");
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME), "");
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME), "");

        deviceExtensions.PushBack(VK_KHR_MAINTENANCE2_EXTENSION_NAME);        // Required for RenderPass2
        deviceExtensions.PushBack(VK_KHR_MULTIVIEW_EXTENSION_NAME);           // Required for RenderPass2
        deviceExtensions.PushBack(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME); // Required for ShadingRate
        deviceExtensions.PushBack(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);

        enabledExtensionFeatures.m_Multiview    = m_PhysicalDeviceExtensionFeatures.m_Multiview;
        enabledExtensionFeatures.m_bRenderPass2 = m_PhysicalDeviceExtensionFeatures.m_bRenderPass2;
        enabledExtensionFeatures.m_ShadingRate  = m_PhysicalDeviceExtensionFeatures.m_ShadingRate;

        *pNextExtension = &enabledExtensionFeatures.m_Multiview;
        pNextExtension  = &enabledExtensionFeatures.m_Multiview.pNext;

        *pNextExtension = &enabledExtensionFeatures.m_ShadingRate;
        pNextExtension  = &enabledExtensionFeatures.m_ShadingRate.pNext;
      }
      else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False)
      {
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME), "");

        deviceExtensions.PushBack(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);

        enabledExtensionFeatures.m_FragmentDensityMap = m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap;

        *pNextExtension = &enabledExtensionFeatures.m_FragmentDensityMap;
        pNextExtension  = &enabledExtensionFeatures.m_FragmentDensityMap.pNext;

        if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2.fragmentDensityMapDeferred != vk::False)
        {
          XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME), "");

          deviceExtensions.PushBack(VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME);

          enabledExtensionFeatures.m_FragmentDensityMap2 = m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2;

          *pNextExtension = &enabledExtensionFeatures.m_FragmentDensityMap2;
          pNextExtension  = &enabledExtensionFeatures.m_FragmentDensityMap2.pNext;
        }
      }
      else
      {
        XII_REPORT_FAILURE("One of Vulkan features: fragment shading rate or fragment density map must be enabled");
      }
    }

    {
      vkEnabledFeatures.multiDrawIndirect = m_PhysicalDeviceFeatures.multiDrawIndirect;

      if (m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount)
      {
        XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME), "");

        deviceExtensions.PushBack(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
      }
    }

    if (m_AdapterDescription.m_Features.m_NativeMultiDraw != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_MULTI_DRAW_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_EXT_MULTI_DRAW_EXTENSION_NAME);

      enabledExtensionFeatures.m_MultiDraw = m_PhysicalDeviceExtensionFeatures.m_MultiDraw;

      *pNextExtension = &enabledExtensionFeatures.m_MultiDraw;
      pNextExtension  = &enabledExtensionFeatures.m_MultiDraw.pNext;

      enabledExtensionFeatures.m_ShaderDrawParameters = m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters;

      *pNextExtension = &enabledExtensionFeatures.m_ShaderDrawParameters;
      pNextExtension  = &enabledExtensionFeatures.m_ShaderDrawParameters.pNext;
    }

    if (m_AdapterDescription.m_Features.m_VertexShaderRenderTargetArrayIndex != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);
    }

    // Custom border color.
    if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME))
    {
      deviceExtensions.PushBack(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

      enabledExtensionFeatures.m_CustomBorderColor = m_PhysicalDeviceExtensionFeatures.m_CustomBorderColor;

      *pNextExtension = &enabledExtensionFeatures.m_CustomBorderColor;
      pNextExtension  = &enabledExtensionFeatures.m_CustomBorderColor.pNext;
    }

    // External memory support.
    if (m_AdapterDescription.m_Features.m_ExternalMemory != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif
    }

    // External semaphore support.
    if (m_AdapterDescription.m_Features.m_ExternalSemaphore != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#endif
    }

    // External fence support.
    if (m_AdapterDescription.m_Features.m_ExternalFence != xiiGALDeviceFeatureState::Disabled)
    {
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
      XII_ASSERT_DEV(IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME), "");

      deviceExtensions.PushBack(VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME);
#endif
    }

    // Ensure that the last next is null.
    *pNextExtension = nullptr;
  }
  else
  {
    xiiLog::Error("Can not enable extended device features when VK_KHR_get_physical_device_properties2 extension is not supported by device");
  }

  static_assert(sizeof(xiiGALDeviceFeatures) == 46, "There may be uninitialized device features.");

  deviceCreationDescription.ppEnabledExtensionNames = deviceExtensions.IsEmpty() ? nullptr : deviceExtensions.GetData();
  deviceCreationDescription.enabledExtensionCount   = deviceExtensions.GetCount();

  // Create Logical Device
  {
    m_LogicalDeviceFeatures          = vkEnabledFeatures;
    m_LogicalDeviceExtensionFeatures = enabledExtensionFeatures;
    m_LogicalDeviceEnabledExtensions = deviceExtensions;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.createDevice(&deviceCreationDescription, nullptr, &m_LogicalDevice, m_InstanceDispatchLoader));

    m_InstanceDispatchLoader.init(m_LogicalDevice);
  }

  {
    vk::PipelineStageFlags graphicsStages = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eAllGraphics;
    vk::PipelineStageFlags computeStages  = vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eComputeShader;

    vk::AccessFlags graphicsAccessFlags = vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eInputAttachmentRead | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    vk::AccessFlags computeAccessFlags  = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eUniformRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    vk::AccessFlags transferAccessFlags = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostRead | vk::AccessFlagBits::eHostWrite;

    if (m_LogicalDeviceFeatures.geometryShader)
    {
      graphicsStages |= vk::PipelineStageFlagBits::eGeometryShader;
    }
    if (m_LogicalDeviceFeatures.tessellationShader)
    {
      graphicsStages |= vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    }
    if (m_LogicalDeviceExtensionFeatures.m_MeshShader.meshShader != vk::False && m_LogicalDeviceExtensionFeatures.m_MeshShader.taskShader != vk::False)
    {
      graphicsStages |= vk::PipelineStageFlagBits::eTaskShaderEXT | vk::PipelineStageFlagBits::eMeshShaderEXT;
    }
    if (m_LogicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline != vk::False)
    {
      computeStages |= vk::PipelineStageFlagBits::eRayTracingShaderKHR | vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR;
      computeAccessFlags |= vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR;
    }
    if (m_LogicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
    {
      graphicsStages |= vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
      graphicsAccessFlags |= vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
    }
    if (m_LogicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False)
    {
      graphicsStages |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT;
      graphicsAccessFlags |= vk::AccessFlagBits::eFragmentDensityMapReadEXT;
    }

    const xiiUInt32 uiQueueCount = m_PhysicalDeviceQueueFamilyProperties.GetCount();

    m_LogicalDeviceSupportedStagesFlags.SetCount(uiQueueCount, vk::PipelineStageFlagBits{0});
    m_LogicalDeviceSupportedAccessFlags.SetCount(uiQueueCount, vk::AccessFlags{0});

    for (xiiUInt32 i = 0; i < uiQueueCount; ++i)
    {
      const vk::QueueFamilyProperties& queueFamily = m_PhysicalDeviceQueueFamilyProperties[i];
      vk::PipelineStageFlags&          stageFlags  = m_LogicalDeviceSupportedStagesFlags[i];
      vk::AccessFlags&                 accessFlags = m_LogicalDeviceSupportedAccessFlags[i];

      if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
      {
        stageFlags |= graphicsStages | computeStages | VulkanUtilities::VK_PIPELINE_STAGE_ALL_TRANSFER;
        accessFlags |= graphicsAccessFlags | computeAccessFlags | transferAccessFlags;
      }
      else if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
      {
        stageFlags |= computeStages | VulkanUtilities::VK_PIPELINE_STAGE_ALL_TRANSFER;
        accessFlags |= computeAccessFlags | transferAccessFlags;
      }
      else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
      {
        stageFlags |= VulkanUtilities::VK_PIPELINE_STAGE_ALL_TRANSFER;
        accessFlags |= transferAccessFlags;
      }
    }
  }

  // Initialize Vulkan Memory Allocator (VMA).
  {
    m_pVulkanMemoryAllocator = XII_NEW(&m_Allocator, xiiVulkanMemoryAllocator);

    VK_SUCCEED_OR_RETURN_XII_FAILURE(m_pVulkanMemoryAllocator->Initialize(this));
  }

  // Create pools.
  {
    m_pFencePool             = XII_NEW(&m_Allocator, xiiGALFencePoolVulkan, this, 16U);
    m_pSemaphorePool         = XII_NEW(&m_Allocator, xiiGALSemaphorePoolVulkan, this, 16U);
    m_pDeferredDeletionQueue = XII_NEW(&m_Allocator, DeferredDeletionQueue, this);
  }

  // Create command queues.
  {
    {
      m_LogicalDevice.getQueue(m_GraphicsQueueInformation.m_uiQueueFamilyIndex, m_GraphicsQueueInformation.m_uiQueueIndex, &m_GraphicsQueueInformation.m_vkQueue, m_InstanceDispatchLoader);

      xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Graphics};
      m_pGraphicsCommandQueue                                = XII_NEW(&m_Allocator, xiiGALCommandQueueVulkan, this, queueDescription, m_GraphicsQueueInformation);
      m_pGraphicsCommandBufferPool                           = XII_NEW(&m_Allocator, xiiGALCommandBufferPoolVulkan, this, m_pGraphicsCommandQueue.Borrow());
      m_pGraphicsCommandQueueQueryPool                       = XII_NEW(&m_Allocator, xiiGALQueryPoolVulkan, this, m_pGraphicsCommandQueue.Borrow(), m_GraphicsQueueInformation);

      m_pGraphicsCommandQueue->SetDebugName("Command Queue (Default Graphics)");

      xiiLog::Dev("Created {}", m_pGraphicsCommandQueue->GetDebugName());
    }

    if (m_ComputeQueueInformation.m_uiQueueFamilyIndex != xiiInvalidIndex)
    {
      m_LogicalDevice.getQueue(m_ComputeQueueInformation.m_uiQueueFamilyIndex, m_ComputeQueueInformation.m_uiQueueIndex, &m_ComputeQueueInformation.m_vkQueue, m_InstanceDispatchLoader);

      xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Compute};
      m_pComputeCommandQueue                                 = XII_NEW(&m_Allocator, xiiGALCommandQueueVulkan, this, queueDescription, m_ComputeQueueInformation);
      m_pComputeCommandBufferPool                            = XII_NEW(&m_Allocator, xiiGALCommandBufferPoolVulkan, this, m_pComputeCommandQueue.Borrow());
      m_pComputeCommandQueueQueryPool                        = XII_NEW(&m_Allocator, xiiGALQueryPoolVulkan, this, m_pComputeCommandQueue.Borrow(), m_ComputeQueueInformation);

      m_pComputeCommandQueue->SetDebugName("Command Queue (Default Compute)");

      xiiLog::Dev("Created {}", m_pComputeCommandQueue->GetDebugName());
    }

    if (m_TransferQueueInformation.m_uiQueueFamilyIndex != xiiInvalidIndex)
    {
      m_LogicalDevice.getQueue(m_TransferQueueInformation.m_uiQueueFamilyIndex, m_TransferQueueInformation.m_uiQueueIndex, &m_TransferQueueInformation.m_vkQueue, m_InstanceDispatchLoader);

      xiiGALCommandQueueCreationDescription queueDescription = {.m_QueueFlags = xiiGALCommandQueueFlags::Transfer};
      m_pTransferCommandQueue                                = XII_NEW(&m_Allocator, xiiGALCommandQueueVulkan, this, queueDescription, m_TransferQueueInformation);
      m_pTransferCommandBufferPool                           = XII_NEW(&m_Allocator, xiiGALCommandBufferPoolVulkan, this, m_pTransferCommandQueue.Borrow());
      m_pTransferCommandQueueQueryPool                       = XII_NEW(&m_Allocator, xiiGALQueryPoolVulkan, this, m_pTransferCommandQueue.Borrow(), m_TransferQueueInformation);

      m_pTransferCommandQueue->SetDebugName("Command Queue (Default Transfer)");

      xiiLog::Dev("Created {}", m_pTransferCommandQueue->GetDebugName());
    }
  }

  xiiClipSpaceDepthRange::Default = xiiClipSpaceDepthRange::ZeroToOne;

  // We use xiiClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  return XII_SUCCESS;
}

void xiiGALDeviceVulkan::SafeReleaseDeviceObjectInternal(vk::ObjectType vkObjectType, void* pObject, xiiVulkanAllocation allocation, vk::DeviceMemory vkExternalMemory)
{
  if (vkExternalMemory != VK_NULL_HANDLE)
  {
    m_pDeferredDeletionQueue->EnqueueResource(vkObjectType, pObject, vkExternalMemory);
    return;
  }
  if (allocation != VK_NULL_HANDLE)
  {
    m_pDeferredDeletionQueue->EnqueueResource(vkObjectType, pObject, allocation);
  }
  else
  {
    m_pDeferredDeletionQueue->EnqueueResource(vkObjectType, pObject);
  }
}

void xiiGALDeviceVulkan::ReclaimLaterInternal(vk::ObjectType vkObjectType, void* pObject)
{
  switch (vkObjectType)
  {
    case vk::ObjectType::eSemaphore:
    {
      m_pDeferredDeletionQueue->EnqueueResource(m_pSemaphorePool.Borrow(), reinterpret_cast<vk::Semaphore&>(pObject));
    }
    break;
    case vk::ObjectType::eFence:
    {
      m_pDeferredDeletionQueue->EnqueueResource(m_pFencePool.Borrow(), reinterpret_cast<vk::Fence&>(pObject));
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALDeviceVulkan::SetVulkanAllocationDebugName(xiiVulkanAllocation allocation, const char* szDebugName) const
{
  if (allocation != VK_NULL_HANDLE)
  {
    m_pVulkanMemoryAllocator->SetAllocationUserData(allocation, szDebugName);
  }
}

void xiiGALDeviceVulkan::BeginFramePlatform()
{
}

void xiiGALDeviceVulkan::EndFramePlatform()
{
  if (m_pTransferCommandBufferPool)
  {
    m_pTransferCommandBufferPool->ReclaimCompleted();
  }
  if (m_pComputeCommandBufferPool)
  {
    m_pComputeCommandBufferPool->ReclaimCompleted();
  }
  m_pGraphicsCommandBufferPool->ReclaimCompleted();

  m_pDeferredDeletionQueue->ReleaseResources();
}

xiiGALCommandQueue* xiiGALDeviceVulkan::GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.AreAllSet(xiiGALCommandQueueFlags::Graphics))
    return m_pGraphicsCommandQueue.Borrow();

  if (queueFlags.AreAllSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueue.Borrow();

  if (queueFlags.AreAllSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueue.Borrow();

  return m_pGraphicsCommandQueue.Borrow();
}

void xiiGALDeviceVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder tmp;

  SetVulkanObjectDebugName(m_LogicalDevice, sName.GetData(tmp));
}

xiiInternal::NewInstance<xiiGALSwapChain> xiiGALDeviceVulkan::CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSwapChainVulkan> pSwapChainVulkan = XII_NEW(&m_Allocator, xiiGALSwapChainVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pSwapChainVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pSwapChainVulkan.m_pAllocator, pSwapChainVulkan.m_pInstance);
    return nullptr;
  }

  return pSwapChainVulkan;
}

xiiInternal::NewInstance<xiiGALCommandList> xiiGALDeviceVulkan::CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALCommandListVulkan> pCommandListVulkan = XII_NEW(&m_Allocator, xiiGALCommandListVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pCommandListVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pCommandListVulkan.m_pAllocator, pCommandListVulkan.m_pInstance);
    return nullptr;
  }

  return pCommandListVulkan;
}

xiiInternal::NewInstance<xiiGALBlendState> xiiGALDeviceVulkan::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBlendStateVulkan> pBlendStateVulkan = XII_NEW(&m_Allocator, xiiGALBlendStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pBlendStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pBlendStateVulkan.m_pAllocator, pBlendStateVulkan.m_pInstance);
    return nullptr;
  }

  return pBlendStateVulkan;
}

xiiInternal::NewInstance<xiiGALDepthStencilState> xiiGALDeviceVulkan::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALDepthStencilStateVulkan> pDepthStencilStateVulkan = XII_NEW(&m_Allocator, xiiGALDepthStencilStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pDepthStencilStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pDepthStencilStateVulkan.m_pAllocator, pDepthStencilStateVulkan.m_pInstance);
    return nullptr;
  }

  return pDepthStencilStateVulkan;
}

xiiInternal::NewInstance<xiiGALRasterizerState> xiiGALDeviceVulkan::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRasterizerStateVulkan> pRasterizerStateVulkan = XII_NEW(&m_Allocator, xiiGALRasterizerStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pRasterizerStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pRasterizerStateVulkan.m_pAllocator, pRasterizerStateVulkan.m_pInstance);
    return nullptr;
  }

  return pRasterizerStateVulkan;
}

xiiInternal::NewInstance<xiiGALShader> xiiGALDeviceVulkan::CreateShaderPlatform(const xiiGALShaderCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALShaderVulkan> pShaderVulkan = XII_NEW(&m_Allocator, xiiGALShaderVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pShaderVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pShaderVulkan.m_pAllocator, pShaderVulkan.m_pInstance);
    return nullptr;
  }

  return pShaderVulkan;
}

xiiInternal::NewInstance<xiiGALBuffer> xiiGALDeviceVulkan::CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiInternal::NewInstance<xiiGALBufferVulkan> pBufferVulkan = XII_NEW(&m_Allocator, xiiGALBufferVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pBufferVulkan->InitPlatform(pInitialData, externalMemoryKind).Failed())
  {
    XII_DELETE(pBufferVulkan.m_pAllocator, pBufferVulkan.m_pInstance);
    return nullptr;
  }

  return pBufferVulkan;
}

xiiInternal::NewInstance<xiiGALTexture> xiiGALDeviceVulkan::CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiInternal::NewInstance<xiiGALTextureVulkan> pTextureVulkan = XII_NEW(&m_Allocator, xiiGALTextureVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pTextureVulkan->InitPlatform(pInitialData, externalMemoryKind).Failed())
  {
    XII_DELETE(pTextureVulkan.m_pAllocator, pTextureVulkan.m_pInstance);
    return nullptr;
  }

  return pTextureVulkan;
}

xiiInternal::NewInstance<xiiGALSampler> xiiGALDeviceVulkan::CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALSamplerVulkan> pSamplerVulkan = XII_NEW(&m_Allocator, xiiGALSamplerVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pSamplerVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pSamplerVulkan.m_pAllocator, pSamplerVulkan.m_pInstance);
    return nullptr;
  }

  return pSamplerVulkan;
}

xiiInternal::NewInstance<xiiGALQuery> xiiGALDeviceVulkan::CreateQueryPlatform(const xiiGALQueryCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALQueryVulkan> pQueryVulkan = XII_NEW(&m_Allocator, xiiGALQueryVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pQueryVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pQueryVulkan.m_pAllocator, pQueryVulkan.m_pInstance);
    return nullptr;
  }

  return pQueryVulkan;
}

xiiInternal::NewInstance<xiiGALFence> xiiGALDeviceVulkan::CreateFencePlatform(const xiiGALFenceCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFenceVulkan> pFenceVulkan = XII_NEW(&m_Allocator, xiiGALFenceVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pFenceVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pFenceVulkan.m_pAllocator, pFenceVulkan.m_pInstance);
    return nullptr;
  }

  return pFenceVulkan;
}

xiiInternal::NewInstance<xiiGALRenderPass> xiiGALDeviceVulkan::CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRenderPassVulkan> pRenderPassVulkan = XII_NEW(&m_Allocator, xiiGALRenderPassVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pRenderPassVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pRenderPassVulkan.m_pAllocator, pRenderPassVulkan.m_pInstance);
    return nullptr;
  }

  return pRenderPassVulkan;
}

xiiInternal::NewInstance<xiiGALFramebuffer> xiiGALDeviceVulkan::CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALFramebufferVulkan> pFramebufferVulkan = XII_NEW(&m_Allocator, xiiGALFramebufferVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pFramebufferVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pFramebufferVulkan.m_pAllocator, pFramebufferVulkan.m_pInstance);
    return nullptr;
  }

  return pFramebufferVulkan;
}

xiiInternal::NewInstance<xiiGALBottomLevelAS> xiiGALDeviceVulkan::CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALBottomLevelASVulkan> pBottomLevelASVulkan = XII_NEW(&m_Allocator, xiiGALBottomLevelASVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pBottomLevelASVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pBottomLevelASVulkan.m_pAllocator, pBottomLevelASVulkan.m_pInstance);
    return nullptr;
  }

  return pBottomLevelASVulkan;
}

xiiInternal::NewInstance<xiiGALTopLevelAS> xiiGALDeviceVulkan::CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALTopLevelASVulkan> pTopLevelASVulkan = XII_NEW(&m_Allocator, xiiGALTopLevelASVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pTopLevelASVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pTopLevelASVulkan.m_pAllocator, pTopLevelASVulkan.m_pInstance);
    return nullptr;
  }

  return pTopLevelASVulkan;
}

xiiInternal::NewInstance<xiiGALPipelineResourceSignature> xiiGALDeviceVulkan::CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALPipelineResourceSignatureVulkan> pPipelineResourceSignatureVulkan = XII_NEW(&m_Allocator, xiiGALPipelineResourceSignatureVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pPipelineResourceSignatureVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pPipelineResourceSignatureVulkan.m_pAllocator, pPipelineResourceSignatureVulkan.m_pInstance);
    return nullptr;
  }

  return pPipelineResourceSignatureVulkan;
}

xiiInternal::NewInstance<xiiGALGraphicsPipelineState> xiiGALDeviceVulkan::CreateGraphicsPipelineStatePlatform(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALGraphicsPipelineStateVulkan> pGraphicsPipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALGraphicsPipelineStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pGraphicsPipelineStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pGraphicsPipelineStateVulkan.m_pAllocator, pGraphicsPipelineStateVulkan.m_pInstance);
    return nullptr;
  }

  return pGraphicsPipelineStateVulkan;
}

xiiInternal::NewInstance<xiiGALComputePipelineState> xiiGALDeviceVulkan::CreateComputePipelineStatePlatform(const xiiGALComputePipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALComputePipelineStateVulkan> pComputePipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALComputePipelineStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pComputePipelineStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pComputePipelineStateVulkan.m_pAllocator, pComputePipelineStateVulkan.m_pInstance);
    return nullptr;
  }

  return pComputePipelineStateVulkan;
}

xiiInternal::NewInstance<xiiGALRayTracingPipelineState> xiiGALDeviceVulkan::CreateRayTracingPipelineStatePlatform(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALRayTracingPipelineStateVulkan> pRayTracingPipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALRayTracingPipelineStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pRayTracingPipelineStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pRayTracingPipelineStateVulkan.m_pAllocator, pRayTracingPipelineStateVulkan.m_pInstance);
    return nullptr;
  }

  return pRayTracingPipelineStateVulkan;
}

xiiInternal::NewInstance<xiiGALTilePipelineState> xiiGALDeviceVulkan::CreateTilePipelineStatePlatform(const xiiGALTilePipelineStateCreationDescription& description)
{
  xiiInternal::NewInstance<xiiGALTilePipelineStateVulkan> pTilePipelineStateVulkan = XII_NEW(&m_Allocator, xiiGALTilePipelineStateVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(this, m_Allocator.GetParent()), description);

  if (pTilePipelineStateVulkan->InitPlatform().Failed())
  {
    XII_DELETE(pTilePipelineStateVulkan.m_pAllocator, pTilePipelineStateVulkan.m_pInstance);
    return nullptr;
  }

  return pTilePipelineStateVulkan;
}

void xiiGALDeviceVulkan::WaitIdlePlatform()
{
  if (m_pTransferCommandQueue)
  {
    m_pTransferCommandQueue->WaitForIdle();
  }
  if (m_pComputeCommandQueue)
  {
    m_pComputeCommandQueue->WaitForIdle();
  }
  m_pGraphicsCommandQueue->WaitForIdle();

  VK_ASSERT_DEV(m_LogicalDevice.waitIdle(m_InstanceDispatchLoader));

  m_pDeferredDeletionQueue->ReleaseResources(true);
}

xiiResult xiiGALDeviceVulkan::FillCapabilitiesPlatform()
{
  XII_LOG_BLOCK("xiiGALDeviceVulkan::FillCapabilitiesPlatform");

  // Graphics Adapter Properties
  {
    m_Description.m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Vulkan;

    m_AdapterDescription.m_sAdapterName = m_PhysicalDeviceProperties.deviceName;

    switch (m_PhysicalDeviceProperties.deviceType)
    {
      case vk::PhysicalDeviceType::eIntegratedGpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Integrated;
        break;

      case vk::PhysicalDeviceType::eDiscreteGpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Discrete;
        break;

      case vk::PhysicalDeviceType::eCpu:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Software;
        break;

      default:
        m_AdapterDescription.m_Type = xiiGALDeviceAdapterType::Unknown;
        break;
    }

    m_AdapterDescription.m_Vendor             = xiiGALDeviceUtilities::GetVendorFromID(m_PhysicalDeviceProperties.vendorID);
    m_AdapterDescription.m_uiVendorID         = m_PhysicalDeviceProperties.vendorID;
    m_AdapterDescription.m_uiDeviceID         = m_PhysicalDeviceProperties.deviceID;
    m_AdapterDescription.m_uiVideoOutputCount = 0U;
  }

  // Label all enabled features as optional.
  xiiGALDeviceFeatures availableDeviceFeatures = ConvertVulkanFeaturesToDeviceFeatures(m_uiVulkanVersion, m_PhysicalDeviceFeatures, m_PhysicalDeviceProperties, m_PhysicalDeviceExtensionFeatures, m_PhysicalDeviceExtensionProperties, xiiGALDeviceFeatureState::Optional);
  m_AdapterDescription.m_Features              = GetEnabledDeviceFeatures(availableDeviceFeatures, m_Description.m_DeviceFeatures);

  // Buffer Properties
  {
    m_AdapterDescription.m_BufferProperties.m_uiConstantBufferAlignment         = static_cast<xiiUInt32>(m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    m_AdapterDescription.m_BufferProperties.m_uiStructuredBufferOffsetAlignment = static_cast<xiiUInt32>(m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);

    static_assert(sizeof(m_AdapterDescription.m_BufferProperties) == 8, "There may be uninitialized buffer properties.");
  }

  // Texture Properties
  {
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension1D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture1DArraySlices   = m_PhysicalDeviceProperties.limits.maxImageArrayLayers;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension2D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture2DArraySlices   = m_PhysicalDeviceProperties.limits.maxImageArrayLayers;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTexture3DDimension     = m_PhysicalDeviceProperties.limits.maxImageDimension3D;
    m_AdapterDescription.m_TextureProperties.m_uiMaxTextureCubeDimension   = m_PhysicalDeviceProperties.limits.maxImageDimensionCube;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bTexture2DMSArraySupported  = true;
    m_AdapterDescription.m_TextureProperties.m_bTextureViewSupported       = true;
    m_AdapterDescription.m_TextureProperties.m_bCubeMapArraysSupported     = m_PhysicalDeviceFeatures.imageCubeArray == vk::True;
    m_AdapterDescription.m_TextureProperties.m_bTextureView2DOn3DSupported = m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset ? m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset.imageView2DOn3DImage == vk::True : true;

    static_assert(sizeof(m_AdapterDescription.m_TextureProperties) == 32, "There may be uninitialized texture properties.");
  }

  // Sampler Properties
  {
    m_AdapterDescription.m_SamplerProperties.m_bBorderSamplingModeSupported = true;
    m_AdapterDescription.m_SamplerProperties.m_uiMaxAnisotropy              = static_cast<xiiUInt32>(m_PhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    m_AdapterDescription.m_SamplerProperties.m_bLODBiasSupported            = true;

    static_assert(sizeof(m_AdapterDescription.m_SamplerProperties) == 12, "There may be uninitialized sampler properties.");
  }

  // Ray Tracing Properties
  if (m_AdapterDescription.m_Features.m_RayTracing != xiiGALDeviceFeatureState::Disabled)
  {
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxRecursionDepth        = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxRayRecursionDepth;
    m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupHandleSize    = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.shaderGroupHandleSize;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxShaderRecordStride    = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxShaderGroupStride;
    m_AdapterDescription.m_RayTracingProperties.m_uiShaderGroupBaseAlignment = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.shaderGroupBaseAlignment;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxRayGenThreads         = m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.maxRayDispatchInvocationCount;
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxInstancesPerTLAS      = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxInstanceCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxPrimitivesPerBLAS     = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxPrimitiveCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiMaxGeometriesPerBLAS     = static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.maxGeometryCount);
    m_AdapterDescription.m_RayTracingProperties.m_uiVertexBufferAlignment    = 1U;
    m_AdapterDescription.m_RayTracingProperties.m_uiIndexBufferAlignment     = 1U;
    m_AdapterDescription.m_RayTracingProperties.m_uiTransformBufferAlignment = 16; // From the specification.
    m_AdapterDescription.m_RayTracingProperties.m_uiBoxBufferAlignment       = 8;  // From the specification.
    m_AdapterDescription.m_RayTracingProperties.m_uiScratchBufferAlignment   = m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.minAccelerationStructureScratchOffsetAlignment;
    m_AdapterDescription.m_RayTracingProperties.m_uiInstanceBufferAlignment  = 16; // From the specification.

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::StandaloneShaders;

    if (m_PhysicalDeviceExtensionFeatures.m_RayQuery.rayQuery)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::InlineRayTracing;

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipelineTraceRaysIndirect)
      m_AdapterDescription.m_RayTracingProperties.m_CapabilityFlags |= xiiGALRayTracingCapabilityFlags::IndirectRayTracing;

    static_assert(sizeof(m_AdapterDescription.m_RayTracingProperties) == 60, "There may be uninitialized ray tracing properties.");
  }

  // Wave Operation Properties
  {
    vk::ShaderStageFlags supportedStages = m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute);

    if (m_PhysicalDeviceFeatures.geometryShader != vk::False)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & vk::ShaderStageFlagBits::eGeometry;

    if (m_PhysicalDeviceFeatures.tessellationShader != vk::False)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation);

    if (m_PhysicalDeviceExtensionFeatures.m_MeshShader.meshShader != vk::False && m_PhysicalDeviceExtensionFeatures.m_MeshShader.taskShader != vk::False)
      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & (vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT);

    if (m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.rayTracingPipeline != vk::False)
    {
      constexpr auto allRayTracingFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eIntersectionKHR | vk::ShaderStageFlagBits::eCallableKHR;

      supportedStages |= m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedStages & allRayTracingFlags;
    }

    m_AdapterDescription.m_WaveOperationProperties.m_uiMinSize             = m_PhysicalDeviceExtensionProperties.m_Subgroup.subgroupSize;
    m_AdapterDescription.m_WaveOperationProperties.m_uiMaxSize             = m_PhysicalDeviceExtensionProperties.m_Subgroup.subgroupSize;
    m_AdapterDescription.m_WaveOperationProperties.m_SupportedShaderStages = xiiVulkanTypeConversions::GetGALShaderStageFlags(supportedStages);
    m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures          = xiiGALWaveFeature::Unknown;

    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eBasic)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Basic;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eVote)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Vote;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eArithmetic)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Arithmetic;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eBallot)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Ballot;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eShuffle)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Shuffle;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eShuffleRelative)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::ShuffleRelative;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eClustered)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Clustered;
    if (m_PhysicalDeviceExtensionProperties.m_Subgroup.supportedOperations & vk::SubgroupFeatureFlagBits::eQuad)
      m_AdapterDescription.m_WaveOperationProperties.m_WaveFeatures |= xiiGALWaveFeature::Quad;

    static_assert(sizeof(m_AdapterDescription.m_WaveOperationProperties) == 16, "There may be uninitialized wave operation properties.");
  }

  // Mesh Shader Properties
  {
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountX     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[0];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountY     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[1];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupCountZ     = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupCount[2];
    m_AdapterDescription.m_MeshShaderProperties.m_uiMaxThreadGroupTotalCount = m_PhysicalDeviceExtensionProperties.m_MeshShader.maxMeshWorkGroupTotalCount;

    static_assert(sizeof(m_AdapterDescription.m_MeshShaderProperties) == 16, "There may be uninitialized mesh shader properties.");
  }

  // Compute Shader Properties
  {
    m_AdapterDescription.m_ComputeShaderProperties.m_uiSharedMemorySize          = m_PhysicalDeviceProperties.limits.maxComputeSharedMemorySize;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupInvocations = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations;
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeX       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeY       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupSizeZ       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountX      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountY      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[1];
    m_AdapterDescription.m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ      = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[2];

    static_assert(sizeof(m_AdapterDescription.m_ComputeShaderProperties) == 32, "There may be uninitialized compute shader properties.");
  }

  // Shading Rate Properties
  if (m_AdapterDescription.m_Features.m_VariableRateShading != xiiGALDeviceFeatureState::Disabled)
  {
    // VK_KHR_fragment_shading_rate
    if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate != vk::False || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.primitiveFragmentShadingRate != vk::False || m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
    {
      xiiBitflags<xiiGALShadingRateCapabilityFlags>& shadingRateCapabilityFlags = m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags;
      auto                                           SetShadingRateCapability   = [&shadingRateCapabilityFlags](VkBool32 vkFlag, xiiGALShadingRateCapabilityFlags::Enum capabilityFlag) {
        if (vkFlag != vk::False)
        {
          shadingRateCapabilityFlags |= capabilityFlag;
        }
      };

      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate, xiiGALShadingRateCapabilityFlags::PerDraw);
      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.primitiveFragmentShadingRate, xiiGALShadingRateCapabilityFlags::PerPrimitive);
      SetShadingRateCapability(m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate, xiiGALShadingRateCapabilityFlags::TextureBased);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithSampleMask, xiiGALShadingRateCapabilityFlags::SampleMask);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithShaderSampleMask, xiiGALShadingRateCapabilityFlags::ShaderSampleMask);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateWithShaderDepthStencilWrites, xiiGALShadingRateCapabilityFlags::ShaderDepthStencilWrite);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.primitiveFragmentShadingRateWithMultipleViewports, xiiGALShadingRateCapabilityFlags::PerPrimitiveWithMultipleViewports);
      SetShadingRateCapability(m_PhysicalDeviceExtensionProperties.m_ShadingRate.layeredShadingRateAttachments, xiiGALShadingRateCapabilityFlags::TextureArray);

      if (shadingRateCapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased))
        shadingRateCapabilityFlags |= xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget;

      // Always enabled in Vulkan.
      shadingRateCapabilityFlags |= xiiGALShadingRateCapabilityFlags::ShadingRateShaderInput;

      m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags = xiiGALShadingRateCombinerFlags::PassThrough | xiiGALShadingRateCombinerFlags::CombinerOverride;

      if (m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateNonTrivialCombinerOps != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags |= xiiGALShadingRateCombinerFlags::CombinerMin | xiiGALShadingRateCombinerFlags::CombinerMax;
        m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags |= (m_PhysicalDeviceExtensionProperties.m_ShadingRate.fragmentShadingRateStrictMultiplyCombiner != vk::False) ? xiiGALShadingRateCombinerFlags::CombinerMul : xiiGALShadingRateCombinerFlags::CombinerSum;
      }

      if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_Format             = xiiGALShadingRateFormat::Palette;
        m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.width  = m_PhysicalDeviceExtensionProperties.m_ShadingRate.minFragmentShadingRateAttachmentTexelSize.width;
        m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.height = m_PhysicalDeviceExtensionProperties.m_ShadingRate.minFragmentShadingRateAttachmentTexelSize.height;
        m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.width  = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateAttachmentTexelSize.width;
        m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.height = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateAttachmentTexelSize.height;
      }

      xiiTemporaryHybridArray<vk::PhysicalDeviceFragmentShadingRateKHR, 4U> shadingRates;
      {
        xiiUInt32 uiShadingRateCount = 0U;
        VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.getFragmentShadingRatesKHR(&uiShadingRateCount, nullptr, m_InstanceDispatchLoader));

        shadingRates.SetCount(uiShadingRateCount);

        VK_SUCCEED_OR_RETURN_XII_FAILURE(m_PhysicalDevice.getFragmentShadingRatesKHR(&uiShadingRateCount, shadingRates.GetData(), m_InstanceDispatchLoader));
      }

      for (xiiUInt32 i = 0U; i < shadingRates.GetCount(); ++i)
      {
        const vk::PhysicalDeviceFragmentShadingRateKHR& srcShadingRate = shadingRates[i];
        xiiGALShadingRateMode&                          dstShadingRate = m_AdapterDescription.m_ShadingRateProperties.m_Modes.ExpandAndGetRef();

        // maxFragmentShadingRateRasterizationSamples - contains only maximum bit
        // sampleCounts                               - contains all supported bits
        XII_ASSERT_DEV((srcShadingRate.fragmentSize.width == 1 && srcShadingRate.fragmentSize.height == 1) || (xiiUInt32{srcShadingRate.sampleCounts} <= ((static_cast<xiiUInt32>(m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentShadingRateRasterizationSamples) << 1) - 1)), "");

        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e1)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::OneSample;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e2)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::TwoSamples;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e4)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::FourSamples;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e8)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::EightSamples;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e16)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::SixteenSamples;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e32)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::ThirtyTwoSamples;
        if (srcShadingRate.sampleCounts & vk::SampleCountFlagBits::e64)
          dstShadingRate.m_SampleBits |= xiiGALSampleCount::SixtyFourSamples;

        dstShadingRate.m_ShadingRate = xiiVulkanTypeConversions::FragmentSizeToShadingRate(srcShadingRate.fragmentSize);
      }
    }
    // VK_EXT_fragment_density_map
    else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False)
    {
      m_AdapterDescription.m_ShadingRateProperties.m_Format          = xiiGALShadingRateFormat::RG8UNormalized;
      m_AdapterDescription.m_ShadingRateProperties.m_CombinerFlags   = xiiGALShadingRateCombinerFlags::PassThrough | xiiGALShadingRateCombinerFlags::CombinerOverride;
      m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags = xiiGALShadingRateCapabilityFlags::TextureBased | xiiGALShadingRateCapabilityFlags::SameTextureForWholeRenderPass | xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget;

      if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMapDynamic != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnGPU;
      }
      else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2.fragmentDensityMapDeferred != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnSubmit;
      }
      else
      {
        m_AdapterDescription.m_ShadingRateProperties.m_TextureAccess = xiiGALShadingRateTextureAccess::OnSetRenderTarget;
      }

      if (m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.fragmentDensityInvocations != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::AdditionalInvocations;
      }
      if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMapNonSubsampledImages != vk::False)
      {
        m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags |= xiiGALShadingRateCapabilityFlags::NonSubSampledRenderTarget;
      }

      // This is zero if VK_EXT_fragment_density_map2 is not supported.
      m_AdapterDescription.m_ShadingRateProperties.m_uiMaxSubSampledArraySlices = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap2.maxSubsampledArrayLayers;

      m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.width  = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.minFragmentDensityTexelSize.width;
      m_AdapterDescription.m_ShadingRateProperties.m_MinTileSize.height = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.minFragmentDensityTexelSize.height;
      m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.width  = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.maxFragmentDensityTexelSize.width;
      m_AdapterDescription.m_ShadingRateProperties.m_MaxTileSize.height = m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.maxFragmentDensityTexelSize.height;

      xiiGALShadingRateMode shadingMode{.m_ShadingRate = xiiGALShadingRateFlags::_1X1, .m_SampleBits = xiiGALSampleCount::AllSamples};

      m_AdapterDescription.m_ShadingRateProperties.m_Modes.PushBack(shadingMode);
    }

    // Retrieve supported bind flags.
    if (m_AdapterDescription.m_ShadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased))
    {
      vk::Format          vkShadingRateResourceFormat = vk::Format::eUndefined;
      vk::ImageUsageFlags vkShadingRateTextureUsage   = (vk::ImageUsageFlagBits)0U;

      if (m_AdapterDescription.m_ShadingRateProperties.m_Format == xiiGALShadingRateFormat::RG8UNormalized)
      {
        vkShadingRateResourceFormat = vk::Format::eR8G8Unorm;
        vkShadingRateTextureUsage   = vk::ImageUsageFlagBits::eFragmentDensityMapEXT;
      }
      else
      {
        vkShadingRateResourceFormat = vk::Format::eR8Uint;
        vkShadingRateTextureUsage   = vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;
      }

      auto TestImageUsage = [&](vk::ImageUsageFlags usageFlags) -> bool {
        vk::ImageFormatProperties imageFormatProperties = {};

        vk::Result result = m_PhysicalDevice.getImageFormatProperties(vkShadingRateResourceFormat, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vkShadingRateTextureUsage | usageFlags, {}, &imageFormatProperties, m_InstanceDispatchLoader);

        return result == vk::Result::eSuccess;
      };

      vk::FormatProperties formatProperties = {};
      m_PhysicalDevice.getFormatProperties(vkShadingRateResourceFormat, &formatProperties, m_InstanceDispatchLoader);
      XII_ASSERT_DEV(formatProperties.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eFragmentShadingRateAttachmentKHR | vk::FormatFeatureFlagBits::eFragmentDensityMapEXT), "");

      m_AdapterDescription.m_ShadingRateProperties.m_BindFlags = xiiGALBindFlags::ShadingRate;
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) && TestImageUsage(vk::ImageUsageFlagBits::eSampled))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::ShaderResource;
      }
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage) && TestImageUsage(vk::ImageUsageFlagBits::eStorage))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::UnorderedAccess;
      }
      if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment) && TestImageUsage(vk::ImageUsageFlagBits::eColorAttachment))
      {
        m_AdapterDescription.m_ShadingRateProperties.m_BindFlags |= xiiGALBindFlags::RenderTarget;
      }
    }

    static_assert(sizeof(m_AdapterDescription.m_ShadingRateProperties) == 64, "There may be uninitialized shading rate properties.");
  }

  // Draw Command Properties
  {
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxIndexValue        = m_PhysicalDeviceProperties.limits.maxDrawIndexedIndexValue;
    m_AdapterDescription.m_DrawCommandProperties.m_uiMaxDrawIndirectCount = m_PhysicalDeviceProperties.limits.maxDrawIndirectCount;
    m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::DrawIndirect | xiiGALDrawCommandCapabilityFlags::BaseVertex;

    if (m_PhysicalDeviceFeatures.multiDrawIndirect != vk::False || m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::NativeMultiDrawIndirect;
    }
    if (m_PhysicalDeviceFeatures.drawIndirectFirstInstance != vk::False)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectFirstInstance;
    }
    if (m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount)
    {
      m_AdapterDescription.m_DrawCommandProperties.m_CapabilityFlags |= xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer;
    }

    static_assert(sizeof(m_AdapterDescription.m_DrawCommandProperties) == 12, "There may be uninitialized draw command properties.");
  }

  // Sparse Memory Properties
  {
    XII_ASSERT_DEV(m_PhysicalDeviceFeatures.sparseBinding && (m_PhysicalDeviceFeatures.sparseResidencyBuffer || m_PhysicalDeviceFeatures.sparseResidencyImage2D), "");

    m_AdapterDescription.m_SparseResourceProperties.m_uiAddressSpaceSize  = m_PhysicalDeviceProperties.limits.sparseAddressSpaceSize;
    m_AdapterDescription.m_SparseResourceProperties.m_uiResourceSpaceSize = m_PhysicalDeviceProperties.limits.sparseAddressSpaceSize; // Currently no way to query.
    m_AdapterDescription.m_SparseResourceProperties.m_uiStandardBlockSize = 64U << 10U;                                               // Documentation: "All currently defined standard sparse image block shapes are 64 KB in size."

    m_AdapterDescription.m_SparseResourceProperties.m_BindFlags       = xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing;
    m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags = xiiGALSparseResourceCapabilityFlags::NonResidentSafe | xiiGALSparseResourceCapabilityFlags::MixedResourceTypeSupport;

    auto& sparseResourceCapabilityFlags = m_AdapterDescription.m_SparseResourceProperties.m_CapabilityFlags;
    auto  SetResourceCapabilityFlag     = [&sparseResourceCapabilityFlags](vk::Bool32 feature, xiiBitflags<xiiGALSparseResourceCapabilityFlags> flag) -> void {
      if (feature != vk::False)
      {
        sparseResourceCapabilityFlags |= flag;
      }
    };

    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape, xiiGALSparseResourceCapabilityFlags::Standard2DTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard2DMultisampleBlockShape, xiiGALSparseResourceCapabilityFlags::Standard2DMSTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape, xiiGALSparseResourceCapabilityFlags::Standard3DTileShape);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize, xiiGALSparseResourceCapabilityFlags::AlignedMipSize);
    SetResourceCapabilityFlag(m_PhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict, xiiGALSparseResourceCapabilityFlags::NonResidentStrict);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.shaderResourceResidency, xiiGALSparseResourceCapabilityFlags::ShaderResourceResidency);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyBuffer, xiiGALSparseResourceCapabilityFlags::Buffer);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyImage2D, xiiGALSparseResourceCapabilityFlags::Texture2D | xiiGALSparseResourceCapabilityFlags::Texture2DArrayMipTail);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyImage3D, xiiGALSparseResourceCapabilityFlags::Texture3D);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency2Samples, xiiGALSparseResourceCapabilityFlags::Texture2Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency4Samples, xiiGALSparseResourceCapabilityFlags::Texture4Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency8Samples, xiiGALSparseResourceCapabilityFlags::Texture8Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidency16Samples, xiiGALSparseResourceCapabilityFlags::Texture16Samples);
    SetResourceCapabilityFlag(m_PhysicalDeviceFeatures.sparseResidencyAliased, xiiGALSparseResourceCapabilityFlags::Aliased);

    static_assert(sizeof(m_AdapterDescription.m_SparseResourceProperties) == 32, "There may be uninitialized sparse resource properties.");
  }

  // Memory Properties
  {
    m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory         = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory   = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory       = 0U;
    m_AdapterDescription.m_MemoryProperties.m_uiMaxMemoryAllocation = m_PhysicalDeviceExtensionProperties.m_Maintenance3.maxMemoryAllocationSize;

    std::bitset<VK_MAX_MEMORY_HEAPS> deviceLocalHeap;
    std::bitset<VK_MAX_MEMORY_HEAPS> hostVisibleHeap;
    std::bitset<VK_MAX_MEMORY_HEAPS> unifiedHeap;

    for (xiiUInt32 uiType = 0U; uiType < m_PhysicalDeviceMemoryProperties.memoryTypeCount; ++uiType)
    {
      constexpr vk::MemoryPropertyFlags unifiedMemoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;

      const vk::MemoryType& memoryTypeInformation = m_PhysicalDeviceMemoryProperties.memoryTypes[uiType];

      if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
      {
        m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil | xiiGALBindFlags::InputAttachment;
      }
      else if ((memoryTypeInformation.propertyFlags & unifiedMemoryFlags) == unifiedMemoryFlags)
      {
        unifiedHeap[memoryTypeInformation.heapIndex] = true;

        if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
        {
          m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Write;
        }
        if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached)
        {
          m_AdapterDescription.m_MemoryProperties.m_UnifiedMemoryCPUAccessFlags |= xiiGALCPUAccessFlag::Read;
        }
      }
      else if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
      {
        deviceLocalHeap[memoryTypeInformation.heapIndex] = true;
      }
      else if (memoryTypeInformation.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
      {
        hostVisibleHeap[memoryTypeInformation.heapIndex] = true;
      }

      // In Metal, input attachment with memoryless texture must be used as an imageblock, which is not supported in SPIRV to MSL translator.
#if XII_ENABLED(XII_PLATFORM_OSX)
      if (m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags.IsAnyFlagSet())
      {
        m_AdapterDescription.m_MemoryProperties.m_MemorylessTextureBindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil;
      }
#endif
    }

    for (xiiUInt32 uiHeapIndex = 0U; uiHeapIndex < m_PhysicalDeviceMemoryProperties.memoryHeapCount; ++uiHeapIndex)
    {
      const vk::MemoryHeap& heapInformation = m_PhysicalDeviceMemoryProperties.memoryHeaps[uiHeapIndex];

      if (unifiedHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiUnifiedMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
      else if (deviceLocalHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiLocalMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
      else if (hostVisibleHeap[uiHeapIndex])
      {
        m_AdapterDescription.m_MemoryProperties.m_uiHostVisibleMemory += static_cast<xiiUInt64>(heapInformation.size);
      }
    }

    static_assert(sizeof(m_AdapterDescription.m_MemoryProperties) == 40, "There may be uninitialized memory properties.");
  }

  // Device Limits
  {
    m_AdapterDescription.m_DeviceLimits.m_uiMaxConstantBuffers = m_PhysicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxVertexBuffers   = m_PhysicalDeviceProperties.limits.maxVertexInputBindings;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxSamplers        = m_PhysicalDeviceProperties.limits.maxPerStageDescriptorSamplers;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxRenderTargets   = m_PhysicalDeviceProperties.limits.maxColorAttachments;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxViewports       = m_PhysicalDeviceProperties.limits.maxViewports;

    // KHR_fragment_shading_rate support.
    if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pipelineFragmentShadingRate == vk::True)
    {
      // maxFragmentSize.width/height is the largest texel coverage (e.g. 4x4).
      xiiUInt32 uiMaxWidth  = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentSize.width;
      xiiUInt32 uiMaxHeight = m_PhysicalDeviceExtensionProperties.m_ShadingRate.maxFragmentSize.height;

      // Compute how many discrete rates per axis: {1,2,4,...,max}
      xiiUInt32 uiXRates = xiiMath::FirstBitHigh(uiMaxWidth) + 1U;
      xiiUInt32 uiYRates = xiiMath::FirstBitHigh(uiMaxHeight) + 1U;

      // Total combinations = all pairs of (xRate, yRate)
      m_AdapterDescription.m_DeviceLimits.m_uiMaxShadingRateCombos = uiXRates * uiYRates;

      // X-rate bits sit in the high half of a 4-bit enum: shift = log2(uiMaxWidth)
      m_AdapterDescription.m_DeviceLimits.m_uiShadingRateXShift = xiiMath::FirstBitHigh(uiMaxWidth);
    }
    // EXT_fragment_density_map (desktop) fallback.
    else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap == vk::True)
    {
      m_AdapterDescription.m_DeviceLimits.m_uiMaxShadingRateCombos = 1U;
      m_AdapterDescription.m_DeviceLimits.m_uiShadingRateXShift    = 0U;
    }
    // EXT_fragment_density_map2 (mobile deferred) fallback.
    else if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2.fragmentDensityMapDeferred == vk::True)
    {
      m_AdapterDescription.m_DeviceLimits.m_uiMaxShadingRateCombos = 1U;
      m_AdapterDescription.m_DeviceLimits.m_uiShadingRateXShift    = 0U;
    }
    // No variable‐rate shading support.
    else
    {
      m_AdapterDescription.m_DeviceLimits.m_uiMaxShadingRateCombos = 0U;
      m_AdapterDescription.m_DeviceLimits.m_uiShadingRateXShift    = 0U;
    }

    m_AdapterDescription.m_DeviceLimits.m_uiMaxResourceSignatures = m_PhysicalDeviceProperties.limits.maxBoundDescriptorSets;

    // Buffer sizes.
    m_AdapterDescription.m_DeviceLimits.m_uiMaxPushConstantsSize  = m_PhysicalDeviceProperties.limits.maxPushConstantsSize;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxUniformBufferRange = m_PhysicalDeviceProperties.limits.maxUniformBufferRange;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxStorageBufferRange = m_PhysicalDeviceProperties.limits.maxStorageBufferRange;

    // Descriptor counts per set.
    m_AdapterDescription.m_DeviceLimits.m_uiMaxSampledImages         = m_PhysicalDeviceProperties.limits.maxDescriptorSetSampledImages;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxStorageImages         = m_PhysicalDeviceProperties.limits.maxDescriptorSetStorageImages;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxStorageBuffers        = m_PhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffers;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxCombinedImageSamplers = m_PhysicalDeviceProperties.limits.maxDescriptorSetSampledImages;

    // Compute shader limits.
    m_AdapterDescription.m_DeviceLimits.m_uiMaxComputeWorkGroupInvocations = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations;
    m_AdapterDescription.m_DeviceLimits.m_uiMaxComputeWorkGroupSizeX       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
    m_AdapterDescription.m_DeviceLimits.m_uiMaxComputeWorkGroupSizeY       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
    m_AdapterDescription.m_DeviceLimits.m_uiMaxComputeWorkGroupSizeZ       = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2];
    m_AdapterDescription.m_DeviceLimits.m_uiMaxComputeSharedMemorySize     = m_PhysicalDeviceProperties.limits.maxComputeSharedMemorySize;
  }

  // Queue Information
  {
    for (xiiUInt32 uiQueueIndex = 0U; uiQueueIndex < m_PhysicalDeviceQueueFamilyProperties.GetCount(); ++uiQueueIndex)
    {
      const vk::QueueFamilyProperties& sourceQueue      = m_PhysicalDeviceQueueFamilyProperties[uiQueueIndex];
      xiiGALCommandQueueProperties&    destinationQueue = m_AdapterDescription.m_CommandQueueProperties.ExpandAndGetRef();

      destinationQueue.m_Flags                     = xiiVulkanTypeConversions::GetGALCommandQueueFlags(sourceQueue.queueFlags);
      destinationQueue.m_uiMaxDeviceContexts       = sourceQueue.queueCount;
      destinationQueue.m_TextureCopyGranularity[0] = sourceQueue.minImageTransferGranularity.width;
      destinationQueue.m_TextureCopyGranularity[1] = sourceQueue.minImageTransferGranularity.height;
      destinationQueue.m_TextureCopyGranularity[2] = sourceQueue.minImageTransferGranularity.depth;
    }
  }

  return XII_SUCCESS;
}

vk::PhysicalDevice xiiGALDeviceVulkan::SelectPhysicalDevice(xiiArrayPtr<vk::PhysicalDevice> pPhysicalDevices, xiiUInt32 uiAdapterID) const
{
  auto FindGraphicsComputeQueueFamily = [&instanceDispatchLoader = this->m_InstanceDispatchLoader](const vk::PhysicalDevice& physicalDevice) -> xiiUInt32 {
    xiiUInt32 uiQueueFamilyCount = 0U;
    physicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, nullptr, instanceDispatchLoader);
    XII_ASSERT_DEV(uiQueueFamilyCount > 0U, "No queue families available");

    xiiTemporaryHybridArray<vk::QueueFamilyProperties, 8U> queueFamilies;
    queueFamilies.SetCountUninitialized(uiQueueFamilyCount);
    physicalDevice.getQueueFamilyProperties(&uiQueueFamilyCount, queueFamilies.GetData(), instanceDispatchLoader);

    for (xiiUInt32 i = 0U; i < uiQueueFamilyCount; ++i)
    {
      const vk::QueueFamilyProperties& vkQueueFamilyProperties = queueFamilies[i];

      if ((vkQueueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) && (vkQueueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute))
      {
        return i;
      }
    }
    return xiiInvalidIndex;
  };

  // Direct adapter selection if valid.
  if (uiAdapterID < pPhysicalDevices.GetCount())
  {
    const vk::PhysicalDevice& vkPhysicalDeviceCandidate = pPhysicalDevices[uiAdapterID];

    if (FindGraphicsComputeQueueFamily(vkPhysicalDeviceCandidate) != xiiInvalidIndex)
      return vkPhysicalDeviceCandidate;
  }

  // Rank devices: discrete > integrated > others.
  vk::PhysicalDevice     vkBestDevice = VK_NULL_HANDLE;
  vk::PhysicalDeviceType vkBestType   = vk::PhysicalDeviceType::eOther;

  for (const vk::PhysicalDevice& vkPhysicalDevice : pPhysicalDevices)
  {
    vk::PhysicalDeviceProperties vkPhysicalDeviceProperties;
    vkPhysicalDevice.getProperties(&vkPhysicalDeviceProperties, m_InstanceDispatchLoader);

    if (FindGraphicsComputeQueueFamily(vkPhysicalDevice) == xiiInvalidIndex)
      continue; // unsuitable

    // Prefer discrete > integrated > virtual > CPU > other.
    if ((vkBestDevice == VK_NULL_HANDLE) || (xiiVulkanTypeConversions::RankDeviceType(vkPhysicalDeviceProperties.deviceType) > xiiVulkanTypeConversions::RankDeviceType(vkBestType)))
    {
      vkBestDevice = vkPhysicalDevice;
      vkBestType   = vkPhysicalDeviceProperties.deviceType;
    }
  }

  return vkBestDevice;
}

xiiResult xiiGALDeviceVulkan::InitializePhysicalDeviceProperties()
{
  XII_ASSERT_DEV(m_PhysicalDevice != VK_NULL_HANDLE, "");

  if (m_PhysicalDevice == VK_NULL_HANDLE)
    return XII_FAILURE;

  if (!IsExtensionEnabled(m_EnabledExtensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    return XII_SUCCESS;

  vk::PhysicalDeviceFeatures2 features2    = {};
  void**                      pNextFeature = &features2.pNext;

  vk::PhysicalDeviceProperties2 properties2   = {};
  void**                        pNextProperty = &properties2.pNext;

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8.pNext;
  }

  // VK_KHR_16bit_storage and VK_KHR_8bit_storage extensions require VK_KHR_storage_buffer_storage_class extension.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME))
  {
    if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME))
    {
      *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Storage16Bit;
      pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Storage16Bit.pNext;
    }

    if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME))
    {
      *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Storage8Bit;
      pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Storage8Bit.pNext;
    }
  }

  // Get mesh shader features and properties.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_MeshShader;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_MeshShader.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_MeshShader;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_MeshShader.pNext;
  }

  // Get acceleration structure features and properties.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_AccelerationStructure;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.pNext;
  }

  // Get ray tracing pipeline features and properties.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.pNext;
  }

  // Get inline ray tracing features.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_RAY_QUERY_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_RayQuery;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_RayQuery.pNext;
  }

  // Additional extension that is required for ray tracing.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress.pNext;
  }

  // Additional extension that is required for ray tracing.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_DescriptorIndexing;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_DescriptorIndexing.pNext;
  }

  // Additional extension that is required for ray tracing shader.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bSpirv14 = true;
  }

  // Some features require SPIRV 1.4 or 1.5 which was added to the Vulkan 1.2 core.
  if (m_uiVulkanVersion >= VK_API_VERSION_1_2)
  {
    m_PhysicalDeviceExtensionFeatures.m_bSpirv14 = true;
    m_PhysicalDeviceExtensionFeatures.m_bSpirv15 = true;
  }

  // Extension required for MoltenVk
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset.pNext;

    m_PhysicalDeviceExtensionFeatures.m_bHasPortabilitySubset = true;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_PortabilitySubset;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_PortabilitySubset.pNext;
  }

  // Subgroup feature requires Vulkan 1.1 core.
  if (m_uiVulkanVersion >= VK_API_VERSION_1_1)
  {
    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Subgroup;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Subgroup.pNext;

    m_PhysicalDeviceExtensionFeatures.m_bSubgroupOps = true;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_VertexAttributeDivisor;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_VertexAttributeDivisor.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_TimelineSemaphore;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_TimelineSemaphore.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MULTIVIEW_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_Multiview;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_Multiview.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Multiview;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Multiview.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bRenderPass2 = true;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShadingRate;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_ShadingRate;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_ShadingRate.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_HostQueryReset;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_HostQueryReset.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bDrawIndirectCount = true;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bShaderViewportIndexLayer = true;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_MAINTENANCE3_EXTENSION_NAME))
  {
    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_Maintenance3;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_Maintenance3.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_MULTI_DRAW_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_MultiDraw;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_MultiDraw.pNext;

    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_MultiDraw;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_MultiDraw.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME))
  {
    *pNextFeature = &m_PhysicalDeviceExtensionFeatures.m_CustomBorderColor;
    pNextFeature  = &m_PhysicalDeviceExtensionFeatures.m_CustomBorderColor.pNext;

    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_CustomBorderColor;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_CustomBorderColor.pNext;
  }

  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME))
  {
    *pNextProperty = &m_PhysicalDeviceExtensionProperties.m_DepthStencilResolve;
    pNextProperty  = &m_PhysicalDeviceExtensionProperties.m_DepthStencilResolve.pNext;
  }

  // External memory support.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalMemory = true;
  }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalMemoryWin32 = true;
  }
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalMemoryFd = true;
  }
#endif

  // External semaphore support.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphore = true;
  }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphoreWin32 = true;
  }
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphoreFd = true;
  }
#endif

  // External fence support.
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalFence = true;
  }
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalFenceWin32 = true;
  }
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  if (IsExtensionAvailable(m_PhysicalDeviceSupportedExtensions, VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME))
  {
    m_PhysicalDeviceExtensionFeatures.m_bExternalFenceFd = true;
  }
#endif

  // Ensure that last pNext is null.
  *pNextFeature  = nullptr;
  *pNextProperty = nullptr;

  // Initialize device extension features by current physical device features.
  // Some flags may not be supported by the hardware.
  m_PhysicalDevice.getFeatures2KHR(&features2, m_InstanceDispatchLoader);
  m_PhysicalDevice.getProperties2KHR(&properties2, m_InstanceDispatchLoader);

  // Clear next pointer.
  {
    m_PhysicalDeviceExtensionFeatures.m_MeshShader.pNext             = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_Storage16Bit.pNext           = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_Storage8Bit.pNext            = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_ShaderFloat16Int8.pNext      = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_AccelerationStructure.pNext  = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_RayTracingPipeline.pNext     = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_RayQuery.pNext               = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_BufferDeviceAddress.pNext    = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_DescriptorIndexing.pNext     = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_PortabilitySubset.pNext      = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_VertexAttributeDivisor.pNext = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_TimelineSemaphore.pNext      = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_HostQueryReset.pNext         = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_ShadingRate.pNext            = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.pNext     = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2.pNext    = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_Multiview.pNext              = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_MultiDraw.pNext              = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_ShaderDrawParameters.pNext   = nullptr;
    m_PhysicalDeviceExtensionFeatures.m_CustomBorderColor.pNext      = nullptr;

    m_PhysicalDeviceExtensionProperties.m_MeshShader.pNext             = nullptr;
    m_PhysicalDeviceExtensionProperties.m_AccelerationStructure.pNext  = nullptr;
    m_PhysicalDeviceExtensionProperties.m_RayTracingPipeline.pNext     = nullptr;
    m_PhysicalDeviceExtensionProperties.m_DescriptorIndexing.pNext     = nullptr;
    m_PhysicalDeviceExtensionProperties.m_PortabilitySubset.pNext      = nullptr;
    m_PhysicalDeviceExtensionProperties.m_Subgroup.pNext               = nullptr;
    m_PhysicalDeviceExtensionProperties.m_VertexAttributeDivisor.pNext = nullptr;
    m_PhysicalDeviceExtensionProperties.m_TimelineSemaphore.pNext      = nullptr;
    m_PhysicalDeviceExtensionProperties.m_ShadingRate.pNext            = nullptr;
    m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap.pNext     = nullptr;
    m_PhysicalDeviceExtensionProperties.m_Multiview.pNext              = nullptr;
    m_PhysicalDeviceExtensionProperties.m_Maintenance3.pNext           = nullptr;
    m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap2.pNext    = nullptr;
    m_PhysicalDeviceExtensionProperties.m_MultiDraw.pNext              = nullptr;
    m_PhysicalDeviceExtensionProperties.m_CustomBorderColor.pNext      = nullptr;
    m_PhysicalDeviceExtensionProperties.m_DepthStencilResolve.pNext    = nullptr;
  }

  // Check shading rate texture formats.
  {
    if (m_PhysicalDeviceExtensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False)
    {
      // For D3D12 compatibility, shading rate texture must support xiiGALResourceFormat::R8UInt format.
      vk::FormatProperties formatProperties = {};
      m_PhysicalDevice.getFormatProperties(vk::Format::eR8Uint, &formatProperties, m_InstanceDispatchLoader);

      // Disable feature if image format is not supported.
      if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eFragmentShadingRateAttachmentKHR))
      {
        m_PhysicalDeviceExtensionFeatures.m_ShadingRate   = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR{};
        m_PhysicalDeviceExtensionProperties.m_ShadingRate = vk::PhysicalDeviceFragmentShadingRatePropertiesKHR{};
      }
    }

    if (m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False)
    {
      vk::FormatProperties formatProperties = {};
      m_PhysicalDevice.getFormatProperties(vk::Format::eR8G8Unorm, &formatProperties, m_InstanceDispatchLoader);

      // Disable feature if image format is not supported.
      if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eFragmentDensityMapEXT))
      {
        m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap   = vk::PhysicalDeviceFragmentDensityMapFeaturesEXT{};
        m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap = vk::PhysicalDeviceFragmentDensityMapPropertiesEXT{};

        m_PhysicalDeviceExtensionFeatures.m_FragmentDensityMap2   = vk::PhysicalDeviceFragmentDensityMap2FeaturesEXT{};
        m_PhysicalDeviceExtensionProperties.m_FragmentDensityMap2 = vk::PhysicalDeviceFragmentDensityMap2PropertiesEXT{};
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiGALDeviceVulkan::EnumerateInstanceExtensions(const char* szLayerName, xiiDynamicArray<vk::ExtensionProperties>& extensions)
{
  xiiUInt32 uiExtensionCount = 0U;
  if (vk::enumerateInstanceExtensionProperties(szLayerName, &uiExtensionCount, nullptr, m_InstanceDispatchLoader) != vk::Result::eSuccess)
    return false;

  extensions.SetCountUninitialized(uiExtensionCount);

  if (vk::enumerateInstanceExtensionProperties(szLayerName, &uiExtensionCount, extensions.GetData(), m_InstanceDispatchLoader) != vk::Result::eSuccess)
  {
    extensions.Clear();

    return false;
  }

  XII_ASSERT_DEV(extensions.GetCount() == uiExtensionCount, "The number of extensions written by vk::enumerateInstanceExtensionProperties is not consistent with the count returned in the first call. This is likely a Vulkan loader bug.");

  return true;
}

bool xiiGALDeviceVulkan::IsLayerAvailable(xiiArrayPtr<const vk::LayerProperties> pLayers, const char* szLayerName, xiiUInt32* pVersion /*= nullptr*/) const
{
  for (const auto& layer : pLayers)
  {
    if (strcmp(szLayerName, layer.layerName) == 0U)
    {
      if (pVersion != nullptr)
      {
        *pVersion = layer.specVersion;
      }
      return true;
    }
  }
  return false;
}

bool xiiGALDeviceVulkan::IsExtensionAvailable(xiiArrayPtr<const vk::ExtensionProperties> pExtensions, const char* szExtensionName) const
{
  for (const auto& extension : pExtensions)
  {
    if (strcmp(szExtensionName, extension.extensionName) == 0U)
    {
      return true;
    }
  }
  return false;
}

bool xiiGALDeviceVulkan::IsExtensionEnabled(xiiArrayPtr<const char*> pEnabledExtensions, const char* szExtensionName) const
{
  for (const auto* szEnabledExtension : pEnabledExtensions)
  {
    if (strcmp(szExtensionName, szEnabledExtension) == 0)
    {
      return true;
    }
  }
  return false;
}

bool xiiGALDeviceVulkan::IsLogicalDeviceExtensionEnabled(const char* szExtensionName) const
{
  for (const auto* szEnabledExtension : m_LogicalDeviceEnabledExtensions)
  {
    if (strcmp(szExtensionName, szEnabledExtension) == 0)
    {
      return true;
    }
  }
  return false;
}

xiiGALDeviceFeatures xiiGALDeviceVulkan::ConvertVulkanFeaturesToDeviceFeatures(xiiUInt32 uiVulkanVersion, const vk::PhysicalDeviceFeatures& vkFeatures, const vk::PhysicalDeviceProperties& vkDeviceProperties, const ExtensionFeatures& extensionFeatures, const ExtensionProperties& extensionProperties, xiiGALDeviceFeatureState::Enum optionalState)
{
  XII_ASSERT_DEV(optionalState != xiiGALDeviceFeatureState::Disabled, "");

  xiiGALDeviceFeatures deviceFeatures;

#define INITIALIZE_DEVICE_FEATURE(featureName, bIsFeatureSupported) deviceFeatures.m_##featureName = (bIsFeatureSupported) ? optionalState : xiiGALDeviceFeatureState::Disabled

  // The following features are always enabled.
  deviceFeatures.m_MultithreadedResourceCreation = xiiGALDeviceFeatureState::Enabled;
  deviceFeatures.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
  deviceFeatures.m_BindlessResources             = xiiGALDeviceFeatureState::Enabled;
  deviceFeatures.m_BinaryOcclusionQueries        = xiiGALDeviceFeatureState::Enabled;
  deviceFeatures.m_SubpassFramebufferFetch       = xiiGALDeviceFeatureState::Enabled;
  deviceFeatures.m_TextureComponentSwizzle       = xiiGALDeviceFeatureState::Enabled;

  // Timestamps are not a feature and cannot be disabled. They are either supported by the device, or not supported by the device.
  deviceFeatures.m_TimestampQueries = vkDeviceProperties.limits.timestampComputeAndGraphics ? xiiGALDeviceFeatureState::Enabled : xiiGALDeviceFeatureState::Disabled;
  deviceFeatures.m_DurationQueries  = vkDeviceProperties.limits.timestampComputeAndGraphics ? xiiGALDeviceFeatureState::Enabled : xiiGALDeviceFeatureState::Disabled;

  INITIALIZE_DEVICE_FEATURE(GeometryShaders, vkFeatures.geometryShader);
  INITIALIZE_DEVICE_FEATURE(Tessellation, vkFeatures.tessellationShader);
  INITIALIZE_DEVICE_FEATURE(PipelineStatisticsQueries, vkFeatures.pipelineStatisticsQuery);
  INITIALIZE_DEVICE_FEATURE(OcclusionQueries, vkFeatures.occlusionQueryPrecise);
  INITIALIZE_DEVICE_FEATURE(WireframeFill, vkFeatures.fillModeNonSolid);
  INITIALIZE_DEVICE_FEATURE(DepthBiasClamp, vkFeatures.depthBiasClamp);
  INITIALIZE_DEVICE_FEATURE(DepthClamp, vkFeatures.depthClamp);
  INITIALIZE_DEVICE_FEATURE(IndependentBlend, vkFeatures.independentBlend);
  INITIALIZE_DEVICE_FEATURE(DualSourceBlend, vkFeatures.dualSrcBlend);
  INITIALIZE_DEVICE_FEATURE(MultiViewport, vkFeatures.multiViewport);
  INITIALIZE_DEVICE_FEATURE(TextureCompressionBC, vkFeatures.textureCompressionBC);
  INITIALIZE_DEVICE_FEATURE(VertexPipelineUAVWritesAndAtomics, vkFeatures.vertexPipelineStoresAndAtomics);
  INITIALIZE_DEVICE_FEATURE(PixelUAVWritesAndAtomics, vkFeatures.fragmentStoresAndAtomics);
  INITIALIZE_DEVICE_FEATURE(TextureUAVExtendedFormats, vkFeatures.shaderStorageImageExtendedFormats);
  INITIALIZE_DEVICE_FEATURE(SparseResources, vkFeatures.sparseBinding && (vkFeatures.sparseResidencyBuffer || vkFeatures.sparseResidencyImage2D)); // Requires support for resident resources.

  INITIALIZE_DEVICE_FEATURE(MeshShaders, extensionFeatures.m_MeshShader.taskShader != vk::False && extensionFeatures.m_MeshShader.meshShader != vk::False);

  INITIALIZE_DEVICE_FEATURE(ShaderFloat16, extensionFeatures.m_ShaderFloat16Int8.shaderFloat16 != vk::False);
  INITIALIZE_DEVICE_FEATURE(ShaderInt8, extensionFeatures.m_ShaderFloat16Int8.shaderInt8 != vk::False);

  INITIALIZE_DEVICE_FEATURE(ResourceBuffer16BitAccess, extensionFeatures.m_Storage16Bit.storageBuffer16BitAccess != vk::False && vkFeatures.shaderInt16 != vk::False);
  INITIALIZE_DEVICE_FEATURE(UniformBuffer16BitAccess, extensionFeatures.m_Storage16Bit.uniformAndStorageBuffer16BitAccess != vk::False && vkFeatures.shaderInt16 != vk::False);
  INITIALIZE_DEVICE_FEATURE(ShaderInputOutput16, extensionFeatures.m_Storage16Bit.storageInputOutput16 != vk::False && vkFeatures.shaderInt16 != vk::False);

  INITIALIZE_DEVICE_FEATURE(ResourceBuffer8BitAccess, extensionFeatures.m_Storage8Bit.storageBuffer8BitAccess != vk::False);
  INITIALIZE_DEVICE_FEATURE(UniformBuffer8BitAccess, extensionFeatures.m_Storage8Bit.uniformAndStorageBuffer8BitAccess != vk::False);

  INITIALIZE_DEVICE_FEATURE(ShaderResourceRuntimeArray, extensionFeatures.m_DescriptorIndexing.runtimeDescriptorArray != vk::False);
  INITIALIZE_DEVICE_FEATURE(RayTracing, uiVulkanVersion >= VK_API_VERSION_1_1 && extensionFeatures.m_AccelerationStructure.accelerationStructure != vk::False && (extensionFeatures.m_RayTracingPipeline.rayTracingPipeline != vk::False || extensionFeatures.m_RayQuery.rayQuery != vk::False));

  const vk::ShaderStageFlags     requiredSubgroupStages   = vk::ShaderStageFlagBits::eCompute;
  const vk::SubgroupFeatureFlags requiredSubgroupFeatures = vk::SubgroupFeatureFlagBits::eBasic;
  deviceFeatures.m_WaveOperation                          = (uiVulkanVersion >= VK_API_VERSION_1_1 && (extensionProperties.m_Subgroup.supportedOperations & requiredSubgroupFeatures) == requiredSubgroupFeatures && (extensionProperties.m_Subgroup.supportedStages & requiredSubgroupStages) == requiredSubgroupStages) ? xiiGALDeviceFeatureState::Enabled : xiiGALDeviceFeatureState::Disabled;

  INITIALIZE_DEVICE_FEATURE(InstanceDataStepRate, (extensionFeatures.m_VertexAttributeDivisor.vertexAttributeInstanceRateDivisor != vk::False && extensionFeatures.m_VertexAttributeDivisor.vertexAttributeInstanceRateZeroDivisor != vk::False));

  INITIALIZE_DEVICE_FEATURE(NativeFence, extensionFeatures.m_TimelineSemaphore.timelineSemaphore != vk::False);

  // Dedicated transfer queues cannot record vkCmdResetQueryPool. Timestamp profiling on
  // those queues therefore requires host query reset so stale slots can be recycled before
  // command recording begins.
  bool bHasTimestampCapableTransferQueue = false;
  for (const vk::QueueFamilyProperties& queueFamily : m_PhysicalDeviceQueueFamilyProperties)
  {
    const bool bTransferOnly = (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) != vk::QueueFlags{} &&
      (queueFamily.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)) == vk::QueueFlags{};
    bHasTimestampCapableTransferQueue |= bTransferOnly && queueFamily.timestampValidBits > 0U;
  }
  INITIALIZE_DEVICE_FEATURE(TransferQueueTimestampQueries, extensionFeatures.m_HostQueryReset.hostQueryReset != vk::False && bHasTimestampCapableTransferQueue);

  INITIALIZE_DEVICE_FEATURE(TileShaders, false); // Not currently supported.

  INITIALIZE_DEVICE_FEATURE(VariableRateShading, (extensionFeatures.m_ShadingRate.pipelineFragmentShadingRate != vk::False || extensionFeatures.m_ShadingRate.primitiveFragmentShadingRate != vk::False || extensionFeatures.m_ShadingRate.attachmentFragmentShadingRate != vk::False || extensionFeatures.m_FragmentDensityMap.fragmentDensityMap != vk::False));

  INITIALIZE_DEVICE_FEATURE(NativeMultiDraw, (extensionFeatures.m_MultiDraw.multiDraw != vk::False && extensionFeatures.m_ShaderDrawParameters.shaderDrawParameters != vk::False));

  INITIALIZE_DEVICE_FEATURE(VertexShaderRenderTargetArrayIndex, extensionFeatures.m_bShaderViewportIndexLayer);

  INITIALIZE_DEVICE_FEATURE(DepthStencilResolve, extensionProperties.m_DepthStencilResolve.supportedDepthResolveModes != vk::ResolveModeFlagBits::eNone || extensionProperties.m_DepthStencilResolve.supportedStencilResolveModes != vk::ResolveModeFlagBits::eNone);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const bool bPlatformSupportsExternalMemory = m_PhysicalDeviceExtensionFeatures.m_bExternalMemoryWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  const bool bPlatformSupportsExternalMemory = m_PhysicalDeviceExtensionFeatures.m_bExternalMemoryFd;
#else
  const bool bPlatformSupportsExternalMemory = false;
#endif
  INITIALIZE_DEVICE_FEATURE(ExternalMemory, m_PhysicalDeviceExtensionFeatures.m_bExternalMemory && bPlatformSupportsExternalMemory);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const bool bPlatformSupportsExternalSemaphore = m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphoreWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  const bool bPlatformSupportsExternalSemaphore = m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphoreFd;
#else
  const bool bPlatformSupportsExternalSemaphore = false;
#endif
  INITIALIZE_DEVICE_FEATURE(ExternalSemaphore, m_PhysicalDeviceExtensionFeatures.m_bExternalSemaphore && bPlatformSupportsExternalSemaphore);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const bool bPlatformSupportsExternalFence = m_PhysicalDeviceExtensionFeatures.m_bExternalFenceWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  const bool bPlatformSupportsExternalFence = m_PhysicalDeviceExtensionFeatures.m_bExternalFenceFd;
#else
  const bool bPlatformSupportsExternalFence = false;
#endif
  INITIALIZE_DEVICE_FEATURE(ExternalFence, m_PhysicalDeviceExtensionFeatures.m_bExternalFence && bPlatformSupportsExternalFence);

#undef INITIALIZE_DEVICE_FEATURE

  // Not supported in MoltenVk.
#if XII_ENABLED(XII_PLATFORM_OSX)
  deviceFeatures.m_BinaryOcclusionQueries = xiiGALDeviceFeatureState::Disabled;
  deviceFeatures.m_TimestampQueries       = xiiGALDeviceFeatureState::Disabled;
  deviceFeatures.m_DurationQueries        = xiiGALDeviceFeatureState::Disabled;
#endif

  deviceFeatures.m_AsynchronousShaderCompilation = xiiGALDeviceFeatureState::Enabled;

  static_assert(sizeof(xiiGALDeviceFeatures) == 46, "There may be uninitialized device features.");

  return deviceFeatures;
}

xiiGALDeviceFeatures xiiGALDeviceVulkan::GetEnabledDeviceFeatures(const xiiGALDeviceFeatures& supportedDeviceFeatures, const xiiGALDeviceFeatures& requestedDeviceFeatures)
{
  auto GetFeatureState = [](xiiGALDeviceFeatureState::Enum requestedState, xiiGALDeviceFeatureState::Enum supportedState, const char* szFeatureName) -> xiiGALDeviceFeatureState::Enum {
    XII_IGNORE_UNUSED(szFeatureName);

    switch (requestedState)
    {
      case xiiGALDeviceFeatureState::Disabled:
      {
        // If enabled, then the feature is supported by default and can not be disabled.
        return supportedState == xiiGALDeviceFeatureState::Enabled ? xiiGALDeviceFeatureState::Enabled : xiiGALDeviceFeatureState::Disabled;
      }
      case xiiGALDeviceFeatureState::Enabled:
      {
        if (supportedState != xiiGALDeviceFeatureState::Disabled)
        {
          return xiiGALDeviceFeatureState::Enabled;
        }
        else
        {
          XII_REPORT_FAILURE("{} not supported by this device.", szFeatureName);
        }
        break;
      }
      case xiiGALDeviceFeatureState::Optional:
      {
        return supportedState != xiiGALDeviceFeatureState::Disabled ? xiiGALDeviceFeatureState::Enabled : xiiGALDeviceFeatureState::Disabled;
      }

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
    return xiiGALDeviceFeatureState::Disabled;
  };

  xiiGALDeviceFeatures deviceFeatures;

#define ENABLE_DEVICE_FEATURE(feature, featureName) deviceFeatures.m_##feature = GetFeatureState(requestedDeviceFeatures.m_##feature, supportedDeviceFeatures.m_##feature, featureName)

  // clang-format off
  ENABLE_DEVICE_FEATURE(WireframeFill,                      "Wireframe fill is");
  ENABLE_DEVICE_FEATURE(MultithreadedResourceCreation,      "Multithreaded resource creation is");
  ENABLE_DEVICE_FEATURE(ComputeShaders,                     "Compute shaders are");
  ENABLE_DEVICE_FEATURE(GeometryShaders,                    "Geometry shaders are");
  ENABLE_DEVICE_FEATURE(Tessellation,                       "Tessellation is");
  ENABLE_DEVICE_FEATURE(MeshShaders,                        "Mesh shaders are");
  ENABLE_DEVICE_FEATURE(RayTracing,                         "Ray tracing is");
  ENABLE_DEVICE_FEATURE(BindlessResources,                  "Bindless resources are");
  ENABLE_DEVICE_FEATURE(OcclusionQueries,                   "Occlusion queries are");
  ENABLE_DEVICE_FEATURE(BinaryOcclusionQueries,             "Binary occlusion queries are");
  ENABLE_DEVICE_FEATURE(TimestampQueries,                   "Timestamp queries are");
  ENABLE_DEVICE_FEATURE(PipelineStatisticsQueries,          "Pipeline statistics queries are");
  ENABLE_DEVICE_FEATURE(DurationQueries,                    "Duration queries are");
  ENABLE_DEVICE_FEATURE(DepthBiasClamp,                     "Depth bias clamp is");
  ENABLE_DEVICE_FEATURE(DepthClamp,                         "Depth clamp is");
  ENABLE_DEVICE_FEATURE(IndependentBlend,                   "Independent blend is");
  ENABLE_DEVICE_FEATURE(DualSourceBlend,                    "Dual-source blend is");
  ENABLE_DEVICE_FEATURE(MultiViewport,                      "Multi-viewport is");
  ENABLE_DEVICE_FEATURE(TextureCompressionBC,               "BC texture compression is");
  ENABLE_DEVICE_FEATURE(VertexPipelineUAVWritesAndAtomics,  "Vertex pipeline UAV writes and atomics are");
  ENABLE_DEVICE_FEATURE(PixelUAVWritesAndAtomics,           "Pixel UAV writes and atomics are");
  ENABLE_DEVICE_FEATURE(TextureUAVExtendedFormats,          "Texture UAV extended formats are");
  ENABLE_DEVICE_FEATURE(ShaderFloat16,                      "16-bit float shader operations are");
  ENABLE_DEVICE_FEATURE(ResourceBuffer16BitAccess,          "16-bit resource buffer access is");
  ENABLE_DEVICE_FEATURE(UniformBuffer16BitAccess,           "16-bit uniform buffer access is");
  ENABLE_DEVICE_FEATURE(ShaderInputOutput16,                "16-bit shader inputs/outputs are");
  ENABLE_DEVICE_FEATURE(ShaderInt8,                         "8-bit int shader operations are");
  ENABLE_DEVICE_FEATURE(ResourceBuffer8BitAccess,           "8-bit resource buffer access is");
  ENABLE_DEVICE_FEATURE(UniformBuffer8BitAccess,            "8-bit uniform buffer access is");
  ENABLE_DEVICE_FEATURE(ShaderResourceRuntimeArray,         "Shader resource runtime array is");
  ENABLE_DEVICE_FEATURE(WaveOperation,                      "Wave operations are");
  ENABLE_DEVICE_FEATURE(InstanceDataStepRate,               "Instance data step rate is");
  ENABLE_DEVICE_FEATURE(NativeFence,                        "Native fence is");
  ENABLE_DEVICE_FEATURE(TileShaders,                        "Tile shaders are");
  ENABLE_DEVICE_FEATURE(TransferQueueTimestampQueries,      "Timestamp queries in transfer queues are");
  ENABLE_DEVICE_FEATURE(VariableRateShading,                "Variable shading rate is");
  ENABLE_DEVICE_FEATURE(SparseResources,                    "Sparse resources are");
  ENABLE_DEVICE_FEATURE(SubpassFramebufferFetch,            "Subpass framebuffer fetch is");
  ENABLE_DEVICE_FEATURE(TextureComponentSwizzle,            "Texture component swizzle is");
  ENABLE_DEVICE_FEATURE(NativeMultiDraw,                    "Native multi-draw commands are");
  ENABLE_DEVICE_FEATURE(AsynchronousShaderCompilation,      "Asynchronous shader compilation is");
  ENABLE_DEVICE_FEATURE(VertexShaderRenderTargetArrayIndex, "Vertex shader render target array index is");
  ENABLE_DEVICE_FEATURE(DepthStencilResolve,                "Depth/stencil resolve is");
  ENABLE_DEVICE_FEATURE(ExternalMemory,                     "External memory support is");
  ENABLE_DEVICE_FEATURE(ExternalSemaphore,                  "External semaphore support is");
  ENABLE_DEVICE_FEATURE(ExternalFence,                      "External fence support is");
  // clang-format on

#undef ENABLE_DEVICE_FEATURE

  static_assert(sizeof(xiiGALDeviceFeatures) == 46, "There may be uninitialized device features.");

  return deviceFeatures;
}

xiiUInt32 xiiGALDeviceVulkan::FindQueueFamily(vk::QueueFlags queueFlags, xiiArrayPtr<xiiUInt32> excludedQueueIndices /* = xiiArrayPtr<xiiUInt32>()*/) const
{
  // All commands that are allowed on a queue that supports transfer operations are also allowed on a queue that supports either graphics or compute operations.
  // Thus, if the capabilities of a queue family include vk::QueueFlagBits::eGraphics or vk::QueueFlagBits::eCompute, then reporting the vk::QueueFlagBits::Transfer
  // capability separately for that queue family is optional (4.1).

  vk::QueueFlags queueFlagsOption = queueFlags;
  if (queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute))
  {
    queueFlags &= ~vk::QueueFlagBits::eTransfer;
    queueFlagsOption = queueFlags | vk::QueueFlagBits::eTransfer;
  }

  xiiUInt32 uiQueueFamilyIndex = xiiInvalidIndex;

  for (xiiUInt32 i = 0; i < m_PhysicalDeviceQueueFamilyProperties.GetCount(); ++i)
  {
    // First try to find a queue, for which the flags match exactly. (i.e. dedicated compute or transfer queue)
    const vk::QueueFamilyProperties& properties = m_PhysicalDeviceQueueFamilyProperties[i];

    if ((properties.queueFlags == queueFlags || properties.queueFlags == queueFlagsOption) && !excludedQueueIndices.Contains(i))
    {
      uiQueueFamilyIndex = i;
      break;
    }
  }

  if (uiQueueFamilyIndex == xiiInvalidIndex)
  {
    for (xiiUInt32 i = 0; i < m_PhysicalDeviceQueueFamilyProperties.GetCount(); ++i)
    {
      // Try to find a queue for which all requested flags are set.
      const vk::QueueFamilyProperties& properties = m_PhysicalDeviceQueueFamilyProperties[i];

      // Check only queueFlags as vk::QueueFlagBits::eTransfer is optional for graphics and/or compute queues.
      if (((properties.queueFlags & queueFlags) == queueFlags) && !excludedQueueIndices.Contains(i))
      {
        uiQueueFamilyIndex = i;
        break;
      }
    }
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (uiQueueFamilyIndex != xiiInvalidIndex)
  {
    if (queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute))
    {
      const vk::QueueFamilyProperties& properties = m_PhysicalDeviceQueueFamilyProperties[uiQueueFamilyIndex];

      // Queues supporting graphics and/or compute operations must report (1,1,1).
      // In minImageTransferGranularity, meaning that there are no additional restrictions on the granularity of image transfer operations for these queues (4.1).
      XII_ASSERT_DEV(properties.minImageTransferGranularity.width == 1 && properties.minImageTransferGranularity.height == 1 && properties.minImageTransferGranularity.depth == 1, "Queue family does not meet the required minImageTransferGranularity of (1,1,1).");
    }
  }
#endif

  return uiQueueFamilyIndex;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_DeviceVulkan);
