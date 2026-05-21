/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/StagingBufferPoolD3D12.h>

xiiGALStagingBufferPoolD3D12::xiiGALStagingBufferPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiAlignment, xiiBitflags<xiiGALBindFlags> bindFlags) :
  m_pDeviceD3D12(pDeviceD3D12), m_uiAlignment(uiAlignment), m_BindFlags(bindFlags)
{
}

xiiGALStagingBufferPoolD3D12::~xiiGALStagingBufferPoolD3D12()
{
  for (StagingBufferPage& stagingBufferPage : m_StagingBufferPages)
  {
    ReleasePage(stagingBufferPage);
  }
  m_StagingBufferPages.Clear();

  for (StagingBufferPage& largeAllocation : m_LargeAllocations)
  {
    ReleasePage(largeAllocation);
  }
  m_LargeAllocations.Clear();
}

void xiiGALStagingBufferPoolD3D12::ReleasePage(StagingBufferPage& stagingBufferPage)
{
  if (stagingBufferPage.m_pBuffer == nullptr)
    return;

  if (stagingBufferPage.m_pMappedAddress != nullptr)
  {
    D3D12_RANGE writtenRange = {0U, stagingBufferPage.m_uiSize};
    stagingBufferPage.m_pBuffer->Unmap(0U, &writtenRange);
    stagingBufferPage.m_pMappedAddress = nullptr;
  }

  m_pDeviceD3D12->SafeReleaseBuffer(stagingBufferPage.m_pBuffer, stagingBufferPage.m_Allocation);
  stagingBufferPage = {};
}

void xiiGALStagingBufferPoolD3D12::CreateStagingBufferPage()
{
  xiiD3D12MemoryAllocator* pD3D12Allocator = m_pDeviceD3D12->GetD3D12Allocator();
  XII_ASSERT_DEV(pD3D12Allocator != nullptr, "D3D12 memory allocator must be initialized.");

  StagingBufferPage stagingBufferPage = {};

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = s_uiStagingBufferDefaultPageSize;
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(m_BindFlags);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Upload;

  if (pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, &stagingBufferPage.m_pBuffer, &stagingBufferPage.m_Allocation).Failed())
  {
    xiiLog::Error("Failed to create D3D12 staging-buffer pool page.");
    return;
  }

  stagingBufferPage.m_uiSize = s_uiStagingBufferDefaultPageSize;

  D3D12_RANGE readRange = {0U, stagingBufferPage.m_uiSize};
  if (FAILED(stagingBufferPage.m_pBuffer->Map(0U, &readRange, &stagingBufferPage.m_pMappedAddress)))
  {
    xiiLog::Error("Failed to map D3D12 staging-buffer pool page.");
    ReleasePage(stagingBufferPage);
    return;
  }

  m_StagingBufferPages.PushBack(stagingBufferPage);
}

void xiiGALStagingBufferPoolD3D12::CreateLargeBuffer(xiiUInt64 uiSize)
{
  xiiD3D12MemoryAllocator* pD3D12Allocator = m_pDeviceD3D12->GetD3D12Allocator();
  XII_ASSERT_DEV(pD3D12Allocator != nullptr, "D3D12 memory allocator must be initialized.");

  StagingBufferPage stagingBufferPage = {};

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = uiSize;
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(m_BindFlags);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Upload;

  if (pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, &stagingBufferPage.m_pBuffer, &stagingBufferPage.m_Allocation).Failed())
  {
    xiiLog::Error("Failed to create D3D12 large staging-buffer allocation (size={}).", uiSize);
    return;
  }

  stagingBufferPage.m_uiSize = uiSize;

  D3D12_RANGE readRange = {0U, stagingBufferPage.m_uiSize};
  if (FAILED(stagingBufferPage.m_pBuffer->Map(0U, &readRange, &stagingBufferPage.m_pMappedAddress)))
  {
    xiiLog::Error("Failed to map D3D12 large staging-buffer allocation.");
    ReleasePage(stagingBufferPage);
    return;
  }

  m_LargeAllocations.PushBack(stagingBufferPage);
}

xiiGALStagingBufferAllocationD3D12 xiiGALStagingBufferPoolD3D12::Allocate(xiiUInt64 uiSize, bool bForceLargePage /*= false*/)
{
  if (uiSize == 0U)
    return {};

  if (bForceLargePage || uiSize >= (s_uiStagingBufferDefaultPageSize >> 2U))
  {
    CreateLargeBuffer(uiSize);

    if (m_LargeAllocations.IsEmpty())
      return {};

    const StagingBufferPage& largeAllocation = m_LargeAllocations.PeekBack();

    xiiGALStagingBufferAllocationD3D12 stagingBufferAllocation = {};
    stagingBufferAllocation.m_pD3D12Buffer                     = largeAllocation.m_pBuffer;
    stagingBufferAllocation.m_Allocation                       = largeAllocation.m_Allocation;
    stagingBufferAllocation.m_uiOffset                         = 0U;
    stagingBufferAllocation.m_pMappedAddress                   = largeAllocation.m_pMappedAddress;
    return stagingBufferAllocation;
  }

  xiiUInt64 uiBufferAllocationOffset = xiiMemoryUtils::AlignSize(m_uiOffsetAllocationCounter, xiiUInt64{m_uiAlignment});
  xiiUInt32 uiBufferID               = xiiInvalidIndex;

  for (xiiUInt32 i = m_uiPageAllocationCounter; i < m_StagingBufferPages.GetCount(); ++i)
  {
    if ((uiBufferAllocationOffset + uiSize) <= m_StagingBufferPages[i].m_uiSize)
    {
      uiBufferID = i;
      break;
    }
    uiBufferAllocationOffset = 0U;
  }

  if (uiBufferID == xiiInvalidIndex)
  {
    CreateStagingBufferPage();

    if (m_StagingBufferPages.IsEmpty())
      return {};

    uiBufferID = m_StagingBufferPages.GetCount() - 1U;
  }

  const StagingBufferPage& stagingBufferPage = m_StagingBufferPages[uiBufferID];

  xiiGALStagingBufferAllocationD3D12 stagingBufferAllocation = {};
  stagingBufferAllocation.m_pD3D12Buffer                     = stagingBufferPage.m_pBuffer;
  stagingBufferAllocation.m_Allocation                       = stagingBufferPage.m_Allocation;
  stagingBufferAllocation.m_uiOffset                         = uiBufferAllocationOffset;
  stagingBufferAllocation.m_pMappedAddress                   = xiiMemoryUtils::AddByteOffset(stagingBufferPage.m_pMappedAddress, uiBufferAllocationOffset);

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return stagingBufferAllocation;
}

void xiiGALStagingBufferPoolD3D12::Reset()
{
  m_uiPageAllocationCounter   = 0U;
  m_uiOffsetAllocationCounter = 0U;

  for (StagingBufferPage& stagingBufferPage : m_StagingBufferPages)
  {
    ReleasePage(stagingBufferPage);
  }
  m_StagingBufferPages.Clear();

  for (StagingBufferPage& largeAllocation : m_LargeAllocations)
  {
    ReleasePage(largeAllocation);
  }
  m_LargeAllocations.Clear();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_StagingBufferPoolD3D12);
