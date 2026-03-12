#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <D3D12MemoryAllocator/D3D12MemAlloc.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>

void* xiiD3D12AllocatePtr(size_t uiSize, size_t uiAlignment, void* pPrivateData)
{
  XII_ASSERT_DEV(pPrivateData, "Allocation private data is invalid!");

  return static_cast<xiiMemoryAllocatorD3D12*>(pPrivateData)->GetProxyAllocator()->Allocate(uiSize, uiAlignment);
}

void xiiD3D12FreePtr(void* pMemory, void* pPrivateData)
{
  XII_ASSERT_DEV(pPrivateData, "Allocation private data is invalid!");

  // `pMemory = nullptr` should be accepted and ignored.
  if (pMemory)
  {
    static_cast<xiiMemoryAllocatorD3D12*>(pPrivateData)->GetProxyAllocator()->Deallocate(pMemory);
  }
}

xiiMemoryAllocatorD3D12::xiiMemoryAllocatorD3D12(IDXGIAdapter1* pDXGIAdapter, ID3D12Device* pDeviceD3D12)
{
  m_pAllocator = XII_DEFAULT_NEW(xiiProxyAllocator, "D3D12-MemoryAllocator", xiiFoundation::GetAlignedAllocator());

  D3D12MA::ALLOCATION_CALLBACKS allocationCallbacks = {
    .pAllocate    = &xiiD3D12AllocatePtr,
    .pFree        = &xiiD3D12FreePtr,
    .pPrivateData = this,
  };

  D3D12MA::ALLOCATOR_DESC allocatorDescription = {
    .Flags                = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED,
    .pDevice              = pDeviceD3D12,
    .PreferredBlockSize   = 0U, // Set to 0 to use default, which is currently 64 MiB.
    .pAllocationCallbacks = &allocationCallbacks,
    .pAdapter             = pDXGIAdapter,
  };

  XII_VERIFY(SUCCEEDED(D3D12MA::CreateAllocator(&allocatorDescription, &m_pD3D12MAAllocator)), "Failed to initialize D3D12 Memory Allocator.");
}

xiiMemoryAllocatorD3D12::~xiiMemoryAllocatorD3D12()
{
  XII_GAL_D3D12_RELEASE(m_pD3D12MAAllocator);

  m_pAllocator.Clear();
}
