/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

/// \brief Direct3D 12 memory allocator wrapper.
///
/// Provides high-level allocation and resource management for D3D12 buffers and images.
/// Internally wraps D3D12 Memory Allocator (D3D12MA) and custom logic to simplify memory handling.
class XII_GRAPHICSD3D12_DLL xiiD3D12MemoryAllocator
{
public:
  xiiD3D12MemoryAllocator();

  ~xiiD3D12MemoryAllocator();

  /// \brief Constructs the memory allocator with D3D12 device.
  ///
  /// \param pDeviceD3D12         - The D3D12 device implementation.
  /// \param uiPreferredBlockSize - Optional preferred block size for allocations (Set to 0 to use default, which is currently 64 MiB.).
  xiiResult Initialize(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiPreferredBlockSize = 0U);

  /// \brief Cleans up internal resources.
  void DeInitialize();

private:
  struct Implementation; ///< Internal implementation details.

  friend void* xiiD3D12AllocatePtr(size_t uiSize, size_t uiAlignment, void* pPrivateData);
  friend void  xiiD3D12FreePtr(void* pMemory, void* pPrivateData);

private:
  xiiUniquePtr<Implementation> m_pImplementation; ///< Pointer to the internal allocator implementation.
};
