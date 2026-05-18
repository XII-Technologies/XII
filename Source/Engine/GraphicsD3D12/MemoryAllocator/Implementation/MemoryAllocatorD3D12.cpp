/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>

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
  XII_ENUM_CONSTANT(xiiD3D12MemoryHeapType::Default),
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

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_MemoryAllocator_Implementation_MemoryAllocatorD3D12);
