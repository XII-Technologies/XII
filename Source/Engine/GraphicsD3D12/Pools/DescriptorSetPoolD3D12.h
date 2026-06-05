/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

class XII_GRAPHICSD3D12_DLL xiiGALDescriptorSetPoolD3D12
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALDescriptorSetPoolD3D12);

public:
  struct DescriptorAllocation
  {
    XII_DECLARE_POD_TYPE();

    ID3D12DescriptorHeap*       m_pDescriptorHeap   = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle         = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle         = {};
    xiiUInt32                   m_uiDescriptorCount = 0U;
    xiiUInt32                   m_uiDescriptorSize  = 0U;
    D3D12_DESCRIPTOR_HEAP_TYPE  m_HeapType          = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  };

  DescriptorAllocation RequestDescriptorAllocation(D3D12_DESCRIPTOR_HEAP_TYPE heapType, xiiUInt32 uiDescriptorCount = 1U);
  void                 Reset();

  [[nodiscard]] ID3D12DescriptorHeap* GetCurrentDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;

private:
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListD3D12;
  friend class xiiGALDeviceD3D12;

  xiiGALDescriptorSetPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiBaseHeapSize = 1024U, bool bShaderVisibleDescriptorHeaps = true);
  ~xiiGALDescriptorSetPoolD3D12();

  struct HeapBlock
  {
    XII_DECLARE_POD_TYPE();

    ID3D12DescriptorHeap*      m_pHeap            = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType         = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    xiiUInt32                  m_uiCapacity       = 0U;
    xiiUInt32                  m_uiUsed           = 0U;
    xiiUInt32                  m_uiDescriptorSize = 0U;
  };

  static xiiUInt32 GetHeapTypeIndex(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
  xiiUInt32        FindOrCreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, xiiUInt32 uiRequiredDescriptorCount);
  xiiUInt32        GetDefaultHeapSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;

private:
  xiiGALDeviceD3D12* m_pDeviceD3D12                  = nullptr;
  xiiUInt32          m_uiBaseHeapSize                = 0U;
  bool               m_bShaderVisibleDescriptorHeaps = true;

  mutable xiiMutex m_PoolMutex;

  xiiStaticArray<xiiDynamicArray<HeapBlock>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeaps;
  xiiStaticArray<xiiUInt32, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>                  m_uiCurrentHeapIndex;
};
