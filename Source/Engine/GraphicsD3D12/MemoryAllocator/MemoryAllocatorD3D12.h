/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

/// Flags used to configure Direct3D 12 memory allocation behavior.
///
/// These flags guide the D3D12 Memory Allocator (D3D12MA) or custom allocation logic in selecting memory types, strategies, and mapping behavior for D3D12 resources.
struct XII_GRAPHICSD3D12_DLL xiiD3D12AllocationFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Committed         = 0x00000001, ///< Allocate dedicated memory for the resource (committed resource with implicit heap).
    NeverAllocate     = 0x00000002, ///< Fail allocation if no suitable memory is available; do not create new blocks.
    WithinBudget      = 0x00000004, ///< Create allocation only if additional memory required for it, if any, won't exceed memory budget.
    UpperAddress      = 0x00000008, ///< Allocation will be created from upper stack in a double stack pool.
    CanAlias          = 0x00000010, ///< Allocation can alias other allocations. Use with care and proper synchronization.
    StrategyMinMemory = 0x00010000, ///< Allocation strategy that chooses smallest possible free range for the allocation to minimize memory usage and fragmentation, possibly at the expense of allocation time.
    StrategyMinTime   = 0x00020000, ///< Allocation strategy that chooses first suitable free range for the allocation - not necessarily in terms of the smallest offset but the one that is easiest and fastest to find to minimize allocation time, possibly at the expense of allocation quality.
    StrategyMinOffset = 0x00040000, ///< Allocation strategy that chooses always the lowest offset in available space.

    Default = 0U, ///< No special flags; use default allocation behavior.
  };

  struct Bits
  {
    StorageType Committed : 1;
    StorageType NeverAllocate : 1;
    StorageType WithinBudget : 1;
    StorageType UpperAddress : 1;
    StorageType CanAlias : 1;
    StorageType StrategyMinMemory : 1;
    StorageType StrategyMinTime : 1;
    StorageType StrategyMinOffset : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiD3D12AllocationFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D12_DLL, xiiD3D12AllocationFlags);

/// Preferred memory heap types for Direct3D 12 allocations.
struct XII_GRAPHICSD3D12_DLL xiiD3D12MemoryHeapType
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Default = 0U, ///< Default heap type, typically GPU local memory.
    Upload,       ///< Upload heap type, typically CPU accessible memory for uploading data to the GPU.
    Readback,     ///< Readback heap type, typically CPU accessible memory for reading data back from the GPU.

    ENUM_COUNT,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D12_DLL, xiiD3D12MemoryHeapType);

/// Flags used to configure Direct3D 12 memory heap properties.
struct XII_GRAPHICSD3D12_DLL xiiD3D12MemoryHeapFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Shared                                     = 0x00000001, ///< Heap can be shared between multiple devices (e.g., for cross-adapter sharing).
    DenyBuffers                                = 0x00000004, ///< Heap cannot be used for buffers (e.g., vertex/index buffers).
    AllowDisplay                               = 0x00000008, ///< Heap can be used for displayable resources (e.g., swap chain buffers).
    SharedCrossAdapter                         = 0x00000020, ///< Heap can be shared across multiple adapters (e.g., for multi-GPU setups).
    DenyRenderTargetAndDepthStencilTextures    = 0x00000040, ///< Heap cannot be used for render target or depth stencil textures.
    DenyNonRenderTargetAndDepthStencilTextures = 0x00000080, ///< Heap cannot be used for non-render target and non-depth stencil textures (e.g., regular textures).
    HardwareProtected                          = 0x00000100, ///< Heap is hardware protected (e.g., for secure video decoding).
    AllowWriteWatch                            = 0x00000200, ///< Heap supports write-watch functionality for tracking modified memory pages.
    AllowShaderAtomics                         = 0x00000400, ///< Heap supports shader atomic operations (e.g., for UAVs with atomic counters).
    CreateNotResident                          = 0x00000800, ///< Heap is created in a non-resident state and must be made resident before use.
    CreateNotZeroed                            = 0x00001000, ///< Heap memory is not zero-initialized upon allocation (use with care to avoid uninitialized memory).
    ToolsUseManualWriteTracking                = 0x00002000, ///< Heap is used by tools with manual write tracking; requires explicit synchronization for CPU writes.
    AllowOnlyBuffers                           = 0x000000C0, ///< Heap can only be used for buffers (combination of DenyBuffers and DenyNonRenderTargetAndDepthStencilTextures).
    AllowOnlyNonDisplayableTextures            = 0x000000D0, ///< Heap can only be used for non-displayable textures (combination of DenyBuffers and AllowDisplay).
    AllowOnlyDisplayableTextures               = 0x000000B0, ///< Heap can only be used for displayable textures (combination of DenyBuffers and DenyNonRenderTargetAndDepthStencilTextures).

    Default = 0U, ///< No special flags; heap can be used for any resource type.
  };

  struct Bits
  {
    StorageType Shared : 1;
    StorageType DenyBuffers : 1;
    StorageType AllowDisplay : 1;
    StorageType SharedCrossAdapter : 1;
    StorageType DenyRenderTargetAndDepthStencilTextures : 1;
    StorageType DenyNonRenderTargetAndDepthStencilTextures : 1;
    StorageType HardwareProtected : 1;
    StorageType AllowWriteWatch : 1;
    StorageType AllowShaderAtomics : 1;
    StorageType CreateNotResident : 1;
    StorageType CreateNotZeroed : 1;
    StorageType ToolsUseManualWriteTracking : 1;
    StorageType AllowOnlyBuffers : 1;
    StorageType AllowOnlyNonDisplayableTextures : 1;
    StorageType AllowOnlyDisplayableTextures : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiD3D12MemoryHeapFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D12_DLL, xiiD3D12MemoryHeapFlags);

/// Describes the parameters used to create a Direct3D 12 memory allocation.
struct XII_GRAPHICSD3D12_DLL xiiD3D12MemoryAllocationCreateInfo
{
  xiiBitflags<xiiD3D12AllocationFlags> m_Flags;               ///< Flags that control allocation strategy and behavior.
  xiiEnum<xiiD3D12MemoryHeapType>      m_HeapType;            ///< Preferred memory heap type for the allocation (e.g., Default, Upload, Readback).
  xiiBitflags<xiiD3D12MemoryHeapFlags> m_HeapFlags;           ///< Memory heap flags that must be present in the selected heap. Used to enforce strict compatibility (e.g., Shared, AllowDisplay).
  void*                                m_pUserData = nullptr; ///< Optional user data pointer that can be associated with the allocation. This can be used to store custom metadata or context information relevant to the allocation, which may be useful for debugging, profiling, or custom allocation logic.
};

/// Direct3D 12 memory allocator wrapper.
///
/// Provides high-level allocation and resource management for D3D12 buffers and images.
/// Internally wraps D3D12 Memory Allocator (D3D12MA) and custom logic to simplify memory handling.
class XII_GRAPHICSD3D12_DLL xiiD3D12MemoryAllocator
{
public:
  xiiD3D12MemoryAllocator();

  ~xiiD3D12MemoryAllocator();

  /// Constructs the memory allocator with D3D12 device.
  ///
  /// \param pDeviceD3D12         - The D3D12 device implementation.
  /// \param uiPreferredBlockSize - Optional preferred block size for allocations (Set to 0 to use default, which is currently 64 MiB.).
  xiiResult Initialize(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiPreferredBlockSize = 0U);

  /// Cleans up internal resources.
  void DeInitialize();

  /// Creates a D3D12 buffer resource with the specified description and memory allocation parameters.
  ///
  ///  \param resourceDescription  - D3D12 resource description defining the buffer properties (size, usage, etc.).
  ///  \param allocationCreateInfo - Parameters that control how memory should be allocated for the buffer, including heap type, flags, and optional user data.
  ///  \param out_ppResource       - Output pointer to the created buffer resource.
  ///  \param out_pAllocation      - Output pointer to the memory allocation info.
  xiiResult CreateBuffer(const D3D12_RESOURCE_DESC& resourceDescription, const xiiD3D12MemoryAllocationCreateInfo& allocationCreateInfo, ID3D12Resource** out_ppResource, xiiD3D12Allocation* out_pAllocation);

  /// Destroys a D3D12 buffer resource and frees its associated memory allocation.
  ///
  ///  \param pResource  - Pointer to the buffer resource to destroy.
  ///  \param pAllocation - The memory allocation info for the buffer.
  void DestroyBuffer(ID3D12Resource*& pResource, xiiD3D12Allocation& pAllocation);

  /// Creates a D3D12 image resource with the specified description and memory allocation parameters.
  ///
  /// \param resourceDescription  - D3D12 resource description defining the image properties (dimensions, format, usage, etc.).
  /// \param allocationCreateInfo - Parameters that control how memory should be allocated for the image including heap type, flags, and optional user data.
  /// \param pOptimizedClearValue - Optional pointer to an optimized clear value for render target or depth stencil images, which can improve clear performance.
  /// \param out_ppResource       - Output pointer to the created image resource.
  /// \param out_pAllocation      - Output pointer to the memory allocation info.
  xiiResult CreateImage(const D3D12_RESOURCE_DESC& resourceDescription, const xiiD3D12MemoryAllocationCreateInfo& allocationCreateInfo, const xiiGALOptimizedClearValue* pOptimizedClearValue, ID3D12Resource** out_ppResource, xiiD3D12Allocation* out_pAllocation);

  /// Destroys a D3D12 image resource and frees its associated memory allocation.
  ///
  /// \param pResource  - Pointer to the image resource to destroy.
  /// \param pAllocation - The memory allocation info for the image.
  void DestroyImage(ID3D12Resource*& pResource, xiiD3D12Allocation& pAllocation);

private:
  struct Implementation; ///< Internal implementation details.

  friend void* xiiD3D12AllocatePtr(size_t uiSize, size_t uiAlignment, void* pPrivateData);
  friend void  xiiD3D12FreePtr(void* pMemory, void* pPrivateData);

private:
  xiiUniquePtr<Implementation> m_pImplementation; ///< Pointer to the internal allocator implementation.
};
