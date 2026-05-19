/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

class XII_GRAPHICSD3D12_DLL xiiGALStagingBufferPoolD3D12
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALStagingBufferPoolD3D12);

public:
  struct StagingBufferPage
  {
    XII_DECLARE_POD_TYPE();

    ID3D12Resource*    m_pBuffer        = nullptr;
    xiiD3D12Allocation m_Allocation     = nullptr;
    xiiUInt64          m_uiSize         = 0U;
    void*              m_pMappedAddress = nullptr;
  };

  void CreateStagingBufferPage();
  void CreateLargeBuffer(xiiUInt64 uiSize);

  xiiGALStagingBufferAllocationD3D12 Allocate(xiiUInt64 uiSize, bool bForceLargePage = false);
  void                               Reset();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;
  friend class xiiGALCommandListD3D12;

  xiiGALStagingBufferPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiAlignment, xiiBitflags<xiiGALBindFlags> bindFlags);
  ~xiiGALStagingBufferPoolD3D12();

  void ReleasePage(StagingBufferPage& stagingBufferPage);

private:
  static constexpr xiiUInt64 s_uiStagingBufferDefaultPageSize = 1ULL * 1024ULL * 1024ULL;

  xiiGALDeviceD3D12* m_pDeviceD3D12 = nullptr;

  xiiUInt32                    m_uiAlignment = 0U;
  xiiBitflags<xiiGALBindFlags> m_BindFlags;

  xiiDynamicArray<StagingBufferPage> m_StagingBufferPages;
  xiiDynamicArray<StagingBufferPage> m_LargeAllocations;

  xiiUInt32 m_uiPageAllocationCounter   = 0U;
  xiiUInt64 m_uiOffsetAllocationCounter = 0U;
};

