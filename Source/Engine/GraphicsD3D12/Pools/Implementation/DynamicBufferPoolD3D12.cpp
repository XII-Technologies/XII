/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/DynamicBufferPoolD3D12.h>

xiiGALDynamicBufferPoolD3D12::xiiGALDynamicBufferPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiAlignment, xiiBitflags<xiiGALBindFlags> bindFlags) :
  m_pDeviceD3D12(pDeviceD3D12), m_uiAlignment(uiAlignment), m_BindFlags(bindFlags)
{
}

xiiGALDynamicBufferPoolD3D12::~xiiGALDynamicBufferPoolD3D12()
{
  for (DynamicBufferPage& dynamicBufferPage : m_DynamicBufferPages)
  {
    ReleasePage(dynamicBufferPage);
  }
  m_DynamicBufferPages.Clear();

  for (DynamicBufferPage& largeAllocation : m_LargeAllocations)
  {
    ReleasePage(largeAllocation);
  }
  m_LargeAllocations.Clear();
}

void xiiGALDynamicBufferPoolD3D12::ReleasePage(DynamicBufferPage& dynamicBufferPage)
{
  if (dynamicBufferPage.m_pBuffer == nullptr)
    return;

  if (dynamicBufferPage.m_pMappedAddress != nullptr)
  {
    D3D12_RANGE writtenRange = {};
    dynamicBufferPage.m_pBuffer->Unmap(0U, &writtenRange);
    dynamicBufferPage.m_pMappedAddress = nullptr;
  }

  m_pDeviceD3D12->SafeReleaseBuffer(dynamicBufferPage.m_pBuffer, dynamicBufferPage.m_Allocation);

  dynamicBufferPage = {};
}

void xiiGALDynamicBufferPoolD3D12::CreateDynamicBufferPage()
{
  xiiD3D12MemoryAllocator* pD3D12Allocator = m_pDeviceD3D12->GetD3D12Allocator();
  XII_ASSERT_DEV(pD3D12Allocator != nullptr, "D3D12 memory allocator must be initialized.");

  DynamicBufferPage dynamicBufferPage;

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = s_uiDynamicBufferDefaultPageSize;
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(m_BindFlags | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Upload;

  if (pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, xiiD3D12TypeConversions::GetDynamicBufferState(), &dynamicBufferPage.m_pBuffer, &dynamicBufferPage.m_Allocation).Failed())
  {
    xiiLog::Error("Failed to create D3D12 dynamic-buffer pool page.");
    return;
  }

  dynamicBufferPage.m_uiSize       = s_uiDynamicBufferDefaultPageSize;
  dynamicBufferPage.m_uiGPUAddress = dynamicBufferPage.m_pBuffer->GetGPUVirtualAddress();

  D3D12_RANGE readRange = {0U, 0U};
  if (FAILED(dynamicBufferPage.m_pBuffer->Map(0U, &readRange, &dynamicBufferPage.m_pMappedAddress)))
  {
    xiiLog::Error("Failed to map D3D12 dynamic-buffer pool page.");
    ReleasePage(dynamicBufferPage);
    return;
  }

  m_DynamicBufferPages.PushBack(dynamicBufferPage);
}

void xiiGALDynamicBufferPoolD3D12::CreateLargeBuffer(xiiUInt64 uiSize)
{
  xiiD3D12MemoryAllocator* pD3D12Allocator = m_pDeviceD3D12->GetD3D12Allocator();
  XII_ASSERT_DEV(pD3D12Allocator != nullptr, "D3D12 memory allocator must be initialized.");

  DynamicBufferPage dynamicBufferPage;

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
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(m_BindFlags | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};
  allocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Upload;

  if (pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, xiiD3D12TypeConversions::GetDynamicBufferState(), &dynamicBufferPage.m_pBuffer, &dynamicBufferPage.m_Allocation).Failed())
  {
    xiiLog::Error("Failed to create D3D12 large dynamic-buffer allocation (size={}).", uiSize);
    return;
  }

  dynamicBufferPage.m_uiSize       = uiSize;
  dynamicBufferPage.m_uiGPUAddress = dynamicBufferPage.m_pBuffer->GetGPUVirtualAddress();

  D3D12_RANGE readRange = {0U, 0U};
  if (FAILED(dynamicBufferPage.m_pBuffer->Map(0U, &readRange, &dynamicBufferPage.m_pMappedAddress)))
  {
    xiiLog::Error("Failed to map D3D12 large dynamic-buffer allocation.");
    ReleasePage(dynamicBufferPage);
    return;
  }

  m_LargeAllocations.PushBack(dynamicBufferPage);
}

xiiGALDynamicBufferAllocationD3D12 xiiGALDynamicBufferPoolD3D12::Allocate(xiiUInt64 uiSize, bool bForceLargePage /*= false*/)
{
  if (uiSize == 0U)
    return {};

  if (bForceLargePage || uiSize >= (s_uiDynamicBufferDefaultPageSize >> 2U))
  {
    CreateLargeBuffer(uiSize);

    if (m_LargeAllocations.IsEmpty())
      return {};

    const DynamicBufferPage& largeAllocation = m_LargeAllocations.PeekBack();

    xiiGALDynamicBufferAllocationD3D12 dynamicBufferAllocation = {};
    dynamicBufferAllocation.m_pD3D12Buffer      = largeAllocation.m_pBuffer;
    dynamicBufferAllocation.m_Allocation        = largeAllocation.m_Allocation;
    dynamicBufferAllocation.m_uiOffset          = 0U;
    dynamicBufferAllocation.m_uiGPUVirtualAddress = largeAllocation.m_uiGPUAddress;
    dynamicBufferAllocation.m_pMappedAddress    = largeAllocation.m_pMappedAddress;
    return dynamicBufferAllocation;
  }

  xiiUInt64 uiBufferAllocationOffset = xiiMemoryUtils::AlignSize(m_uiOffsetAllocationCounter, xiiUInt64{m_uiAlignment});
  xiiUInt32 uiBufferID               = xiiInvalidIndex;

  for (xiiUInt32 i = m_uiPageAllocationCounter; i < m_DynamicBufferPages.GetCount(); ++i)
  {
    if ((uiBufferAllocationOffset + uiSize) <= m_DynamicBufferPages[i].m_uiSize)
    {
      uiBufferID = i;
      break;
    }
    uiBufferAllocationOffset = 0U;
  }

  if (uiBufferID == xiiInvalidIndex)
  {
    CreateDynamicBufferPage();

    if (m_DynamicBufferPages.IsEmpty())
      return {};

    uiBufferID = m_DynamicBufferPages.GetCount() - 1U;
  }

  const DynamicBufferPage& dynamicBufferPage = m_DynamicBufferPages[uiBufferID];

  xiiGALDynamicBufferAllocationD3D12 dynamicBufferAllocation = {};
  dynamicBufferAllocation.m_pD3D12Buffer      = dynamicBufferPage.m_pBuffer;
  dynamicBufferAllocation.m_Allocation        = dynamicBufferPage.m_Allocation;
  dynamicBufferAllocation.m_uiOffset          = uiBufferAllocationOffset;
  dynamicBufferAllocation.m_uiGPUVirtualAddress = dynamicBufferPage.m_uiGPUAddress + uiBufferAllocationOffset;
  dynamicBufferAllocation.m_pMappedAddress    = xiiMemoryUtils::AddByteOffset(dynamicBufferPage.m_pMappedAddress, uiBufferAllocationOffset);

  m_uiPageAllocationCounter   = uiBufferID;
  m_uiOffsetAllocationCounter = uiBufferAllocationOffset + uiSize;

  return dynamicBufferAllocation;
}

void xiiGALDynamicBufferPoolD3D12::Reset()
{
  m_uiPageAllocationCounter   = 0U;
  m_uiOffsetAllocationCounter = 0U;

  for (DynamicBufferPage& dynamicBufferPage : m_DynamicBufferPages)
  {
    ReleasePage(dynamicBufferPage);
  }
  m_DynamicBufferPages.Clear();

  for (DynamicBufferPage& largeAllocation : m_LargeAllocations)
  {
    ReleasePage(largeAllocation);
  }
  m_LargeAllocations.Clear();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_DynamicBufferPoolD3D12);
