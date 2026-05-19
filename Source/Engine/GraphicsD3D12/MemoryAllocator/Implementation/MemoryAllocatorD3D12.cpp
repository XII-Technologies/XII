/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <D3D12MemoryAllocator/include/D3D12MemAlloc.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiD3D12AllocationFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::Committed),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::NeverAllocate),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::WithinBudget),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::UpperAddress),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::CanAlias),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::StrategyMinMemory),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::StrategyMinTime),
  XII_BITFLAGS_CONSTANT(xiiD3D12AllocationFlags::StrategyMinOffset),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiD3D12MemoryHeapType, 1)
  XII_ENUM_CONSTANT(xiiD3D12MemoryHeapType::Upload),
  XII_ENUM_CONSTANT(xiiD3D12MemoryHeapType::Readback),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiD3D12MemoryHeapFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::Shared),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::DenyBuffers),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowDisplay),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::SharedCrossAdapter),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::DenyRenderTargetAndDepthStencilTextures),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::DenyNonRenderTargetAndDepthStencilTextures),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::HardwareProtected),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowWriteWatch),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowShaderAtomics),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::CreateNotResident),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::CreateNotZeroed),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::ToolsUseManualWriteTracking),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowOnlyBuffers),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowOnlyNonDisplayableTextures),
  XII_BITFLAGS_CONSTANT(xiiD3D12MemoryHeapFlags::AllowOnlyDisplayableTextures),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

//////////////////////////////////////////////////////////////////////////
// Helpers: map our flags/usage -> D3D12MA

namespace
{
  XII_ALWAYS_INLINE D3D12MA::ALLOCATION_FLAGS ConvertAllocationFlags(const xiiBitflags<xiiD3D12AllocationFlags>& flags)
  {
    D3D12MA::ALLOCATION_FLAGS allocationFlags = D3D12MA::ALLOCATION_FLAG_NONE;

    if (flags.IsSet(xiiD3D12AllocationFlags::Committed))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
    if (flags.IsSet(xiiD3D12AllocationFlags::NeverAllocate))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;
    if (flags.IsSet(xiiD3D12AllocationFlags::WithinBudget))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
    if (flags.IsSet(xiiD3D12AllocationFlags::UpperAddress))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;
    if (flags.IsSet(xiiD3D12AllocationFlags::CanAlias))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    if (flags.IsSet(xiiD3D12AllocationFlags::StrategyMinMemory))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
    if (flags.IsSet(xiiD3D12AllocationFlags::StrategyMinTime))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    if (flags.IsSet(xiiD3D12AllocationFlags::StrategyMinOffset))
      allocationFlags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_OFFSET;

    return allocationFlags;
  }

  XII_ALWAYS_INLINE D3D12_HEAP_TYPE ConvertHeapType(xiiEnum<xiiD3D12MemoryHeapType> heapType)
  {
    switch (heapType)
    {
      case xiiD3D12MemoryHeapType::Default:
        return D3D12_HEAP_TYPE_DEFAULT;
      case xiiD3D12MemoryHeapType::Upload:
        return D3D12_HEAP_TYPE_UPLOAD;
      case xiiD3D12MemoryHeapType::Readback:
        return D3D12_HEAP_TYPE_READBACK;
      default:
        XII_REPORT_FAILURE("Unknown xiiD3D12MemoryHeapType value: {}", xiiArgEnum(heapType));
        return D3D12_HEAP_TYPE_DEFAULT;
    }
  }

  XII_ALWAYS_INLINE D3D12_HEAP_FLAGS ConvertHeapFlags(xiiBitflags<xiiD3D12MemoryHeapFlags> flags)
  {
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

    if (flags.IsSet(xiiD3D12MemoryHeapFlags::Shared))
      heapFlags |= D3D12_HEAP_FLAG_SHARED;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::DenyBuffers))
      heapFlags |= D3D12_HEAP_FLAG_DENY_BUFFERS;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::AllowDisplay))
      heapFlags |= D3D12_HEAP_FLAG_ALLOW_DISPLAY;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::SharedCrossAdapter))
      heapFlags |= D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::DenyRenderTargetAndDepthStencilTextures))
      heapFlags |= D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::DenyNonRenderTargetAndDepthStencilTextures))
      heapFlags |= D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::HardwareProtected))
      heapFlags |= D3D12_HEAP_FLAG_HARDWARE_PROTECTED;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::AllowWriteWatch))
      heapFlags |= D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::AllowShaderAtomics))
      heapFlags |= D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS;
    if (flags.IsSet(xiiD3D12MemoryHeapFlags::CreateNotResident))
      heapFlags |= D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT;

    return heapFlags;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////
// Implementation struct

struct xiiD3D12MemoryAllocator::Implementation
{
  xiiAllocator*       m_pProxyAllocator   = nullptr; ///< The proxy allocator used for D3D12MA allocations.
  D3D12MA::Allocator* m_pD3D12MAAllocator = nullptr; ///< The D3D12 Memory Allocator instance.
};

void* xiiD3D12AllocatePtr(size_t uiSize, size_t uiAlignment, void* pPrivateData)
{
  XII_ASSERT_DEV(pPrivateData, "Allocation private data is invalid!");

  return static_cast<xiiD3D12MemoryAllocator::Implementation*>(pPrivateData)->m_pProxyAllocator->Allocate(uiSize, uiAlignment);
}

void xiiD3D12FreePtr(void* pMemory, void* pPrivateData)
{
  XII_ASSERT_DEV(pPrivateData, "Allocation private data is invalid!");

  // `pMemory = nullptr` should be accepted and ignored.
  if (pMemory)
  {
    static_cast<xiiD3D12MemoryAllocator::Implementation*>(pPrivateData)->m_pProxyAllocator->Deallocate(pMemory);
  }
}

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor

xiiD3D12MemoryAllocator::xiiD3D12MemoryAllocator()
{
  m_pImplementation = XII_DEFAULT_NEW(xiiD3D12MemoryAllocator::Implementation);
}

xiiD3D12MemoryAllocator::~xiiD3D12MemoryAllocator() = default;

//////////////////////////////////////////////////////////////////////////
// Initialize / DeInitialize

xiiResult xiiD3D12MemoryAllocator::Initialize(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiPreferredBlockSize /*= 0U*/)
{
  m_pImplementation->m_pProxyAllocator = pDeviceD3D12->GetAllocator();

  D3D12MA::ALLOCATION_CALLBACKS allocationCallbacks = {
    .pAllocate    = &xiiD3D12AllocatePtr,
    .pFree        = &xiiD3D12FreePtr,
    .pPrivateData = m_pImplementation.Borrow(),
  };

  D3D12MA::ALLOCATOR_DESC allocatorDescription = {
    .Flags                = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED,
    .pDevice              = pDeviceD3D12->GetD3D12Device(),
    .PreferredBlockSize   = uiPreferredBlockSize,
    .pAllocationCallbacks = &allocationCallbacks,
    .pAdapter             = pDeviceD3D12->GetDXGIAdapter(),
  };

  if (FAILED(D3D12MA::CreateAllocator(&allocatorDescription, &m_pImplementation->m_pD3D12MAAllocator)))
  {
    xiiLog::Error("Failed to initialize D3D12 Memory Allocator.");

    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiD3D12MemoryAllocator::DeInitialize()
{
  XII_ASSERT_DEV(m_pImplementation->m_pD3D12MAAllocator, "D3D12 Memory Allocator is not initialized or already de-initialized.");

  XII_GAL_D3D12_RELEASE(m_pImplementation->m_pD3D12MAAllocator);
}

xiiResult xiiD3D12MemoryAllocator::CreateBuffer(const D3D12_RESOURCE_DESC& resourceDescription, const xiiD3D12MemoryAllocationCreateInfo& allocationCreateInfo, xiiBitflags<xiiGALResourceStateFlags> initialStates, ID3D12Resource** out_ppResource, xiiD3D12Allocation* out_pAllocation)
{
  XII_ASSERT_DEV(m_pImplementation->m_pD3D12MAAllocator != nullptr, "D3D12 Memory Allocator is not initialized.");
  XII_ASSERT_DEV(out_ppResource != nullptr, "Output resource pointer is null.");

  D3D12MA::ALLOCATION_DESC allocationDescription = {};
  allocationDescription.Flags                    = ConvertAllocationFlags(allocationCreateInfo.m_Flags);
  allocationDescription.HeapType                 = ConvertHeapType(allocationCreateInfo.m_HeapType);
  allocationDescription.ExtraHeapFlags           = ConvertHeapFlags(allocationCreateInfo.m_HeapFlags);
  allocationDescription.CustomPool               = nullptr; // Not supported for now.
  allocationDescription.pPrivateData             = allocationCreateInfo.m_pUserData;

  D3D12MA::Allocation* pD3D12MAAllocation = nullptr;
  if (FAILED(m_pImplementation->m_pD3D12MAAllocator->CreateResource(&allocationDescription, &resourceDescription, xiiD3D12TypeConversions::GetResourceState(initialStates), nullptr, &pD3D12MAAllocation, __uuidof(*out_ppResource), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(out_ppResource)))))
  {
    xiiLog::Error("Failed to create buffer resource with D3D12 Memory Allocator.");

    return XII_FAILURE;
  }

  *out_pAllocation = static_cast<xiiD3D12Allocation>(pD3D12MAAllocation);

  return XII_SUCCESS;
}

void xiiD3D12MemoryAllocator::DestroyBuffer(ID3D12Resource*& pResource, xiiD3D12Allocation& pAllocation)
{
  XII_ASSERT_DEV(m_pImplementation->m_pD3D12MAAllocator != nullptr, "D3D12 Memory Allocator is not initialized.");

  XII_GAL_D3D12_RELEASE(pResource);
  XII_GAL_D3D12_RELEASE(pAllocation);
}

xiiResult xiiD3D12MemoryAllocator::CreateImage(const D3D12_RESOURCE_DESC& resourceDescription, const xiiD3D12MemoryAllocationCreateInfo& allocationCreateInfo, xiiBitflags<xiiGALResourceStateFlags> initialStates, const xiiGALOptimizedClearValue* pOptimizedClearValue, ID3D12Resource** out_ppResource, xiiD3D12Allocation* out_pAllocation)
{
  XII_ASSERT_DEV(m_pImplementation->m_pD3D12MAAllocator != nullptr, "D3D12 Memory Allocator is not initialized.");
  XII_ASSERT_DEV(out_ppResource != nullptr, "Output resource pointer is null.");

  D3D12MA::ALLOCATION_DESC allocationDescription = {};
  allocationDescription.Flags                    = ConvertAllocationFlags(allocationCreateInfo.m_Flags);
  allocationDescription.HeapType                 = ConvertHeapType(allocationCreateInfo.m_HeapType);
  allocationDescription.ExtraHeapFlags           = ConvertHeapFlags(allocationCreateInfo.m_HeapFlags);
  allocationDescription.CustomPool               = nullptr; // Not supported for now.
  allocationDescription.pPrivateData             = allocationCreateInfo.m_pUserData;

  D3D12_CLEAR_VALUE          optimizedClearValue      = {};
  const D3D12_CLEAR_VALUE*   pD3D12OptimizedClearValue = nullptr;

  if (pOptimizedClearValue != nullptr && pOptimizedClearValue->m_ResourceFormat != xiiGALResourceFormat::Unknown)
  {
    optimizedClearValue.Format = xiiD3D12TypeConversions::GetFormat(pOptimizedClearValue->m_ResourceFormat);

    const xiiGALResourceFormatDescription& formatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(pOptimizedClearValue->m_ResourceFormat);
    if (formatDescription.m_ComponentType == xiiGALResourceFormatComponentType::Depth || formatDescription.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      optimizedClearValue.DepthStencil.Depth   = pOptimizedClearValue->m_DepthStencil.m_fDepth;
      optimizedClearValue.DepthStencil.Stencil = pOptimizedClearValue->m_DepthStencil.m_uiStencil;
    }
    else
    {
      optimizedClearValue.Color[0] = pOptimizedClearValue->m_ClearColour.r;
      optimizedClearValue.Color[1] = pOptimizedClearValue->m_ClearColour.g;
      optimizedClearValue.Color[2] = pOptimizedClearValue->m_ClearColour.b;
      optimizedClearValue.Color[3] = pOptimizedClearValue->m_ClearColour.a;
    }

    pD3D12OptimizedClearValue = &optimizedClearValue;
  }

  D3D12MA::Allocation* pD3D12MAAllocation = nullptr;
  if (FAILED(m_pImplementation->m_pD3D12MAAllocator->CreateResource(&allocationDescription, &resourceDescription, xiiD3D12TypeConversions::GetResourceState(initialStates), pD3D12OptimizedClearValue, &pD3D12MAAllocation, __uuidof(*out_ppResource), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(out_ppResource)))))
  {
    xiiLog::Error("Failed to create image resource with D3D12 Memory Allocator.");

    return XII_FAILURE;
  }

  *out_pAllocation = static_cast<xiiD3D12Allocation>(pD3D12MAAllocation);

  return XII_SUCCESS;
}

void xiiD3D12MemoryAllocator::DestroyImage(ID3D12Resource*& pResource, xiiD3D12Allocation& pAllocation)
{
  XII_ASSERT_DEV(m_pImplementation->m_pD3D12MAAllocator != nullptr, "D3D12 Memory Allocator is not initialized.");

  XII_GAL_D3D12_RELEASE(pResource);
  XII_GAL_D3D12_RELEASE(pAllocation);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_MemoryAllocator_Implementation_MemoryAllocatorD3D12);
