/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/System/Process.h>
#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <GraphicsVulkan/Utilities/ExternalMemoryVulkan.h>

vk::ExternalMemoryHandleTypeFlagBits xiiVulkanExternalMemory::GetHandleType()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  return vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  return vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#else
  return {};
#endif
}

xiiResult xiiVulkanExternalMemory::Initialize(xiiGALDeviceVulkan& device, xiiBitflags<xiiGALExternalMemoryKind> kind,
                                              const xiiVulkanAllocationInfo& exportAllocation, const vk::MemoryDedicatedAllocateInfo& dedicated,
                                              xiiGALExternalMemoryDescription& ref_description, vk::Semaphore& out_semaphore,
                                              vk::DeviceMemory& out_importedMemory, bool& out_bOwnsHandles)
{
  const auto                     logical  = device.GetVulkanLogicalDevice();
  const auto&                    dispatch = device.GetVulkanDynamicDispatchLoader();
  vk::PhysicalDeviceIDProperties id;
  vk::PhysicalDeviceProperties2  properties;
  properties.pNext = &id;
  device.GetVulkanPhysicalDevice().getProperties2(&properties, dispatch);
  xiiUuid uuid;
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&uuid), id.deviceUUID.data(), sizeof(uuid));
  static_assert(sizeof(xiiUuid) == VK_UUID_SIZE);
  xiiUuid driverUuid;
  xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&driverUuid), id.driverUUID.data(), sizeof(driverUuid));
  const bool bImported = kind.IsSet(xiiGALExternalMemoryKind::Imported);
  if (bImported)
  {
    if (ref_description.m_DeviceUUID != uuid || ref_description.m_DriverUUID != driverUuid)
    {
      xiiLog::Error("External Vulkan memory requires the exporting physical device and driver.");
      return XII_FAILURE;
    }
    xiiGALExternalMemoryDescription local;
    XII_SUCCEED_OR_RETURN(ref_description.DuplicateHandles(local));
    ref_description        = local;
    ref_description.m_Type = xiiGALExternalMemoryKind::Imported;
  }
  else
  {
    const auto flags                    = ref_description.m_Flags;
    ref_description                     = {};
    ref_description.m_Type              = xiiGALExternalMemoryKind::Exportable;
    ref_description.m_Flags             = flags | xiiGALExternalMemoryFlags::SharedAccess;
    ref_description.m_uiProcessId       = xiiProcess::GetCurrentProcessID();
    ref_description.m_uiSize            = exportAllocation.m_uiSize;
    ref_description.m_uiMemoryTypeIndex = exportAllocation.m_uiMemoryType;
    ref_description.m_DriverUUID        = driverUuid;
    ref_description.m_DeviceUUID        = uuid;
  }
  out_bOwnsHandles = true;

  vk::ExportSemaphoreCreateInfo exportSemaphore;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  exportSemaphore.handleTypes = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  exportSemaphore.handleTypes = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
#else
  xiiLog::Error("Vulkan external memory handles are unavailable on this platform.");
  return XII_FAILURE;
#endif
  vk::SemaphoreTypeCreateInfo type;
  type.semaphoreType = ref_description.m_Flags.IsSet(xiiGALExternalMemoryFlags::BinarySemaphore) ? vk::SemaphoreType::eBinary : vk::SemaphoreType::eTimeline;
  vk::PhysicalDeviceExternalSemaphoreInfo semaphoreQuery;
  semaphoreQuery.pNext      = &type;
  semaphoreQuery.handleType = static_cast<vk::ExternalSemaphoreHandleTypeFlagBits>(static_cast<VkExternalSemaphoreHandleTypeFlags>(exportSemaphore.handleTypes));
  vk::ExternalSemaphoreProperties semaphoreProperties;
  device.GetVulkanPhysicalDevice().getExternalSemaphoreProperties(&semaphoreQuery, &semaphoreProperties, dispatch);
  const auto required = bImported ? vk::ExternalSemaphoreFeatureFlagBits::eImportable : vk::ExternalSemaphoreFeatureFlagBits::eExportable;
  if (!(semaphoreProperties.externalSemaphoreFeatures & required))
  {
    return XII_FAILURE;
  }
  type.pNext = bImported ? nullptr : &exportSemaphore;
  vk::SemaphoreCreateInfo semaphoreInfo;
  semaphoreInfo.pNext = &type;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.createSemaphore(&semaphoreInfo, nullptr, &out_semaphore, dispatch));

  if (!bImported)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    vk::MemoryGetWin32HandleInfoKHR memoryInfo;
    memoryInfo.memory     = exportAllocation.m_vkDeviceMemory;
    memoryInfo.handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
    HANDLE memory         = nullptr;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.getMemoryWin32HandleKHR(&memoryInfo, &memory, dispatch));
    ref_description.m_uiNativeHandle = reinterpret_cast<xiiUInt64>(memory);
    vk::SemaphoreGetWin32HandleInfoKHR semaphore;
    semaphore.semaphore  = out_semaphore;
    semaphore.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
    HANDLE handle        = nullptr;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.getSemaphoreWin32HandleKHR(&semaphore, &handle, dispatch));
    ref_description.m_uiNativeSemaphoreHandle = reinterpret_cast<xiiUInt64>(handle);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
    vk::MemoryGetFdInfoKHR memoryInfo;
    memoryInfo.memory     = exportAllocation.m_vkDeviceMemory;
    memoryInfo.handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    int memory            = -1;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.getMemoryFdKHR(&memoryInfo, &memory, dispatch));
    ref_description.m_uiNativeHandle = memory;
    vk::SemaphoreGetFdInfoKHR semaphore;
    semaphore.semaphore  = out_semaphore;
    semaphore.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
    int handle           = -1;
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.getSemaphoreFdKHR(&semaphore, &handle, dispatch));
    ref_description.m_uiNativeSemaphoreHandle = handle;
#endif
    return XII_SUCCESS;
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  vk::ImportSemaphoreWin32HandleInfoKHR semaphoreImport;
  semaphoreImport.semaphore  = out_semaphore;
  semaphoreImport.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
  semaphoreImport.handle     = reinterpret_cast<HANDLE>(ref_description.m_uiNativeSemaphoreHandle);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.importSemaphoreWin32HandleKHR(&semaphoreImport, dispatch));
  vk::ImportMemoryWin32HandleInfoKHR memoryImport;
  memoryImport.handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
  memoryImport.handle     = reinterpret_cast<HANDLE>(ref_description.m_uiNativeHandle);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  vk::ImportSemaphoreFdInfoKHR semaphoreImport;
  semaphoreImport.semaphore  = out_semaphore;
  semaphoreImport.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
  semaphoreImport.fd         = static_cast<int>(ref_description.m_uiNativeSemaphoreHandle);
  VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.importSemaphoreFdKHR(&semaphoreImport, dispatch));
  // Successful FD imports transfer ownership to Vulkan.
  ref_description.m_uiNativeSemaphoreHandle = 0;
  vk::ImportMemoryFdInfoKHR memoryImport;
  memoryImport.handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
  memoryImport.fd         = static_cast<int>(ref_description.m_uiNativeHandle);
#endif
#if XII_ENABLED(XII_PLATFORM_WINDOWS) || XII_ENABLED(XII_PLATFORM_LINUX)
  vk::MemoryRequirements requirements;
  if (dedicated.image)
  {
    logical.getImageMemoryRequirements(dedicated.image, &requirements, dispatch);
  }
  else
  {
    logical.getBufferMemoryRequirements(dedicated.buffer, &requirements, dispatch);
  }
  if (ref_description.m_uiSize < requirements.size || ref_description.m_uiMemoryTypeIndex >= 32 || !(requirements.memoryTypeBits & (1U << ref_description.m_uiMemoryTypeIndex)))
    return XII_FAILURE;
  memoryImport.pNext = &dedicated;
  vk::MemoryAllocateInfo allocation;
  allocation.pNext           = &memoryImport;
  allocation.allocationSize  = ref_description.m_uiSize;
  allocation.memoryTypeIndex = ref_description.m_uiMemoryTypeIndex;
  VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.allocateMemory(&allocation, nullptr, &out_importedMemory, dispatch));
#  if XII_ENABLED(XII_PLATFORM_LINUX)
  ref_description.m_uiNativeHandle = 0;
#  endif
  if (dedicated.image)
  {
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.bindImageMemory(dedicated.image, out_importedMemory, 0, dispatch));
  }
  else
  {
    VK_SUCCEED_OR_RETURN_XII_FAILURE(logical.bindBufferMemory(dedicated.buffer, out_importedMemory, 0, dispatch));
  }
#endif
  return XII_SUCCESS;
}

xiiGALCommandListVulkan* xiiVulkanExternalMemory::GetTransferCommandList(xiiGALCommandList* pCommandList, const xiiGALDeviceVulkan& device,
                                                                         const xiiGALExternalMemoryDescription& description, vk::Semaphore vkSemaphore)
{
  auto* pCommands = xiiDynamicCast<xiiGALCommandListVulkan*>(pCommandList);
  if (!pCommands || pCommands->GetDevice().Borrow() != &device || pCommands->GetRecordingState() != xiiGALCommandList::RecordingState::Recording ||
      pCommands->GetDescription().m_Flags.IsSet(xiiGALCommandListFlags::Secondary) ||
      !vkSemaphore || !description.m_Flags.IsSet(xiiGALExternalMemoryFlags::BinarySemaphore))
  {
    return nullptr;
  }
  return pCommands;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Utilities_Implementation_ExternalMemoryVulkan);
