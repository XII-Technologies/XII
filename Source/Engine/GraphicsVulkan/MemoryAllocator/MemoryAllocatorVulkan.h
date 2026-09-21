/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

/// Preferred memory usage hints for Vulkan allocations using VMA.
///
/// These usage modes guide VMA in selecting the most suitable memory type based on the intended access patterns and performance goals.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanMemoryUsage
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Auto = 0U,          ///< Automatically select the best memory type. VMA chooses between device-local and host-visible memory based on the resource type. Suitable for general-purpose allocations when no strong preference exists.
    AutoPreferDevice,   ///< Prefer device-local memory when possible. VMA attempts to allocate memory that resides on the GPU. Ideal for resources that are accessed frequently by the GPU and rarely updated by the CPU.
    AutoPreferHost,     ///< Prefer host-visible memory when possible. VMA favors memory that can be accessed directly by the CPU. Useful for staging buffers, dynamic resources, or CPU-driven updates.
    GpuLazilyAllocated, ///< Allocates memory that may not be physically backed until used. Typically used for transient attachments like depth or color buffers in render passes. Only available on GPUs that support `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT`.

    ENUM_COUNT,

    Default = Auto
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSVULKAN_DLL, xiiVulkanMemoryUsage);

/// Flags used to configure Vulkan memory allocation behavior.
///
/// These flags guide the Vulkan Memory Allocator (VMA) or custom allocation logic in selecting memory types, strategies, and mapping behavior.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanAllocationCreateFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Dedicated              = 0x00000001, ///< Allocate memory as a dedicated block (not suballocated).
    NeverAllocate          = 0x00000002, ///< Fail allocation if no suitable memory is available; do not create new blocks.
    Mapped                 = 0x00000004, ///< Keep memory persistently mapped after allocation.
    UserDataCopy           = 0x00000008, ///< Copy user data into internal allocation metadata.
    UpperAddress           = 0x00000010, ///< Prefer higher virtual addresses (useful for debugging or layout).
    StrategyMinMemory      = 0x00000020, ///< Minimize total memory usage across allocations.
    StrategyMinTime        = 0x00000040, ///< Minimize allocation time (fastest strategy).
    StrategyFirstFit       = 0x00000200, ///< Use first-fit strategy for suballocation.
    StrategyCanAlias       = 0x00000400, ///< Allow aliasing between allocations (requires manual synchronization).
    StrategyWithinBudget   = 0x00000800, ///< Respect memory budget constraints during allocation.
    StrategyHostSequential = 0x00001000, ///< Optimize for sequential host access (e.g., streaming uploads).
    StrategyHostRandom     = 0x00002000, ///< Optimize for random host access (e.g., sparse updates).

    Default = 0U ///< No flags set; use default allocator behavior.
  };

  struct Bits
  {
    StorageType Dedicated : 1;
    StorageType NeverAllocate : 1;
    StorageType Mapped : 1;
    StorageType UserDataCopy : 1;
    StorageType UpperAddress : 1;
    StorageType StrategyMinMemory : 1;
    StorageType StrategyMinTime : 1;
    StorageType StrategyWorstFit : 1;
    StorageType StrategyCanAlias : 1;
    StorageType StrategyWithinBudget : 1;
    StorageType StrategyHostSequential : 1;
    StorageType StrategyHostRandom : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiVulkanAllocationCreateFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSVULKAN_DLL, xiiVulkanAllocationCreateFlags);

/// Abstract representation of Vulkan memory property flags.
///
/// Helps describe physical device memory types in a readable and type-safe way.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanMemoryPropertyFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    DeviceLocal     = 0x00000001, ///< Memory is physically located on the device (fast access for GPU).
    HostVisible     = 0x00000002, ///< Memory is visible to the host (CPU) and can be mapped.
    HostCoherent    = 0x00000004, ///< Host writes are automatically visible to the device without flushing.
    HostCached      = 0x00000008, ///< Host memory access is cached (may require flushing/invalidation).
    LazilyAllocated = 0x00000010, ///< Memory is lazily allocated by the driver (used with transient resources).
    Protected       = 0x00000020, ///< Memory with protection features - typically used for secure buffers.

    Default = 0U ///< No flags set.
  };

  struct Bits
  {
    StorageType DeviceLocal : 1;
    StorageType HostVisible : 1;
    StorageType HostCoherent : 1;
    StorageType HostCached : 1;
    StorageType LazilyAllocated : 1;
    StorageType Protected : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiVulkanMemoryPropertyFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSVULKAN_DLL, xiiVulkanMemoryPropertyFlags);

/// Describes the parameters used to create a Vulkan memory allocation.
///
/// This structure defines how memory should be allocated, including usage hints, allocation strategy flags, and optional user metadata.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanAllocationCreateInfo
{
  xiiBitflags<xiiVulkanAllocationCreateFlags> m_Flags;                             ///< Flags that control allocation strategy and behavior.
  xiiEnum<xiiVulkanMemoryUsage>               m_Usage;                             ///< Intended usage pattern for the allocation (e.g., GPU-only, CPU-to-GPU).
  xiiBitflags<xiiVulkanMemoryPropertyFlags>   m_RequiredFlags;                     ///< Memory property flags that must be present in the selected memory type. Used to enforce strict compatibility (e.g., HostVisible, DeviceLocal).
  xiiBitflags<xiiVulkanMemoryPropertyFlags>   m_PreferredFlags;                    ///< Memory property flags that are desirable but not mandatory. The allocator will prioritize memory types that include these flags when multiple options are available.
  const char*                                 m_pUserData               = nullptr; ///< Optional pointer to user-defined data associated with the allocation.
  bool                                        m_bExportSharedAllocation = false;   ///< If true, the allocation will be created with exportable handle capabilities for sharing between Vulkan devices or APIs.
};

/// Describes a Vulkan memory allocation and its associated metadata.
///
/// This structure holds information about a Vulkan memory block, including the device memory handle, offset, size, memory type, mapped pointer, user data, and optional debug name.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanAllocationInfo
{
  xiiUInt32        m_uiMemoryType;   ///< Index of the Vulkan memory type used for this allocation.
  vk::DeviceMemory m_vkDeviceMemory; ///< Vulkan device memory handle associated with the allocation.
  vk::DeviceSize   m_uiOffset;       ///< Byte offset into the device memory where the allocation begins.
  vk::DeviceSize   m_uiSize;         ///< Size of the allocation in bytes.
  void*            m_pMappedData;    ///< Pointer to mapped memory, if the allocation is host-visible and mapped.
  void*            m_pUserData;      ///< Optional user-defined data associated with the allocation.
  const char*      m_szName;         ///< Optional debug name for the allocation, used for profiling or diagnostics.
};

/// Reports memory usage statistics for Vulkan allocations.
///
/// This structure provides an overview of memory consumption, including the number of blocks and allocations, as well as their total sizes in bytes.
struct XII_GRAPHICSVULKAN_DLL xiiVulkanMemoryStatistics
{
  xiiUInt32 m_uiBlockCount      = 0U; ///< Number of memory blocks currently allocated.
  xiiUInt32 m_uiAllocationCount = 0U; ///< Number of individual allocations across all blocks.
  xiiUInt64 m_uiBlockBytes      = 0U; ///< Total size in bytes of all allocated memory blocks.
  xiiUInt64 m_uiAllocationBytes = 0U; ///< Total size in bytes of all active allocations.
};

/// Vulkan memory allocator wrapper.
///
/// Provides high-level allocation and resource management for Vulkan buffers and images.
/// Internally wraps Vulkan Memory Allocator (VMA) and custom logic to simplify memory handling.
class XII_GRAPHICSVULKAN_DLL xiiVulkanMemoryAllocator
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVulkanMemoryAllocator);

public:
  xiiVulkanMemoryAllocator();

  ~xiiVulkanMemoryAllocator();

  /// Constructs the memory allocator with Vulkan instance, physical device, and logical device.
  ///
  /// \param pDeviceVulkan        - The Vulkan device implementation.
  /// \param uiPreferredBlockSize - Optional preferred block size for allocations.
  vk::Result Initialize(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiPreferredBlockSize = 0U);

  /// Cleans up internal resources.
  void DeInitialize();

  /// Creates a Vulkan buffer and allocates memory for it.
  ///
  /// \param vkBufferCreateInfo   - Buffer creation parameters.
  /// \param allocationCreateInfo - Allocation configuration.
  /// \param out_buffer           - Output buffer handle.
  /// \param out_allocation       - Output allocation handle.
  /// \param pAllocationInfo      - Optional pointer to receive detailed allocation info.
  vk::Result CreateBuffer(const vk::BufferCreateInfo& vkBufferCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo = nullptr) const;

  /// Destroys a Vulkan buffer and frees its associated memory.
  ///
  /// \param vkBuffer   - Buffer to destroy.
  /// \param allocation - Allocation handle to free.
  void DestroyBuffer(vk::Buffer& vkBuffer, xiiVulkanAllocation& allocation) const;

  /// Creates a Vulkan image and allocates memory for it.
  ///
  /// \param vkImageCreateInfo    - Image creation parameters.
  /// \param allocationCreateInfo - Allocation configuration.
  /// \param out_image            - Output image handle.
  /// \param out_allocation       - Output allocation handle.
  /// \param pAllocationInfo      - Optional pointer to receive detailed allocation info.
  vk::Result CreateImage(const vk::ImageCreateInfo& vkImageCreateInfo, const xiiVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, xiiVulkanAllocation& out_allocation, xiiVulkanAllocationInfo* pAllocationInfo = nullptr) const;

  /// Destroys a Vulkan image and frees its associated memory.
  ///
  /// \param vkImage    - Image to destroy.
  /// \param allocation - Allocation handle to free.
  void DestroyImage(vk::Image& vkImage, xiiVulkanAllocation& allocation) const;

  /// Retrieves detailed information about a memory allocation.
  ///
  /// \param allocation - Allocation handle.
  ///
  /// \return Allocation metadata including offset, size, and mapped pointer.
  xiiVulkanAllocationInfo GetAllocationInfo(xiiVulkanAllocation allocation) const;

  /// Returns the memory property flags for a given allocation.
  ///
  /// \param allocation - Allocation handle.
  ///
  /// \return Vulkan memory property flags (e.g., host-visible, coherent).
  vk::MemoryPropertyFlags GetMemoryPropertyFlags(xiiVulkanAllocation allocation) const;

  /// Sets user-defined metadata for a memory allocation.
  ///
  /// \param allocation - Allocation handle.
  ///
  /// \param pUserData Pointer to user data string.
  void SetAllocationUserData(xiiVulkanAllocation allocation, const char* pUserData) const;

  /// Maps a memory allocation to a CPU-accessible pointer.
  ///
  /// \param allocation - Allocation handle.
  /// \param pData      - Output pointer to mapped memory.
  ///
  /// \return Vulkan result code.
  vk::Result MapMemory(xiiVulkanAllocation allocation, void** pData) const;

  /// Unmaps a previously mapped memory allocation.
  ///
  /// \param allocation - Allocation handle.
  void UnmapMemory(xiiVulkanAllocation allocation) const;

  /// Flushes a memory allocation to ensure GPU visibility.
  ///
  /// \param allocation - Allocation handle.
  /// \param offset     - Byte offset to flush from.
  /// \param size       - Number of bytes to flush.
  ///
  /// \return Vulkan result code.
  vk::Result FlushAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset = 0U, vk::DeviceSize size = vk::WholeSize) const;

  /// Invalidates a memory allocation to ensure CPU visibility.
  ///
  /// \param allocation - Allocation handle.
  /// \param offset     - Byte offset to invalidate from.
  /// \param size       - Number of bytes to invalidate.
  ///
  /// \return Vulkan result code.
  vk::Result InvalidateAllocation(xiiVulkanAllocation allocation, vk::DeviceSize offset = 0U, vk::DeviceSize size = vk::WholeSize) const;

  /// Returns current memory usage statistics.
  ///
  /// \return Struct containing block and allocation counts and sizes.
  xiiVulkanMemoryStatistics GetStatistics() const;

private:
  struct Implementation; ///< Internal implementation details.

  xiiUniquePtr<Implementation> m_pImplementation; ///< Pointer to internal allocator implementation.
};
