/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>

class xiiGALCommandList;
class xiiGALCommandListVulkan;
class xiiGALDeviceVulkan;
struct xiiVulkanAllocationInfo;

namespace vk
{
  enum class ExternalMemoryHandleTypeFlagBits : xiiUInt32;
  struct MemoryDedicatedAllocateInfo;
  class Semaphore;
  class DeviceMemory;
} // namespace vk

namespace xiiVulkanExternalMemory
{
  vk::ExternalMemoryHandleTypeFlagBits GetHandleType();

  /// Import dedicated memory for the native resource identified by dedicated, or export
  /// the existing allocation. Publishes ownership of created handles for failure cleanup.
  xiiResult Initialize(xiiGALDeviceVulkan& device, xiiBitflags<xiiGALExternalMemoryKind> kind,
                       const xiiVulkanAllocationInfo& exportAllocation, const vk::MemoryDedicatedAllocateInfo& dedicated,
                       xiiGALExternalMemoryDescription& ref_description, vk::Semaphore& out_semaphore,
                       vk::DeviceMemory& out_importedMemory, bool& out_bOwnsHandles);

  xiiGALCommandListVulkan* GetTransferCommandList(xiiGALCommandList* pCommandList, const xiiGALDeviceVulkan& device,
                                                  const xiiGALExternalMemoryDescription& description, vk::Semaphore vkSemaphore);
} // namespace xiiVulkanExternalMemory
