/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

class XII_GRAPHICSD3D12_DLL xiiGALFencePoolD3D12
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALFencePoolD3D12);

public:
  ID3D12Fence* RequestFence();
  void         ReclaimFence(ID3D12Fence*& pFence);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;

  xiiGALFencePoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiInitialSize);
  ~xiiGALFencePoolD3D12();

  ID3D12Fence* CreateD3D12Fence();

private:
  xiiGALDeviceD3D12* m_pDeviceD3D12 = nullptr;

  mutable xiiMutex              m_PoolMutex;
  xiiDynamicArray<ID3D12Fence*> m_Fences;
  xiiDeque<ID3D12Fence*>        m_QueuedFences;
};
