/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Pools/DescriptorSetPoolD3D12.h>

xiiGALDescriptorSetPoolD3D12::xiiGALDescriptorSetPoolD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiUInt32 uiBaseHeapSize /*= 1024U*/) :
  m_pDeviceD3D12(pDeviceD3D12), m_uiBaseHeapSize(uiBaseHeapSize)
{
  XII_ASSERT_DEV(m_pDeviceD3D12 != nullptr, "D3D12 device must be valid.");

  m_uiCurrentHeapIndex.SetCount(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, xiiInvalidIndex);
  m_DescriptorHeaps.SetCount(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
}

xiiGALDescriptorSetPoolD3D12::~xiiGALDescriptorSetPoolD3D12()
{
  for (xiiUInt32 uiHeapType = 0U; uiHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++uiHeapType)
  {
    for (HeapBlock& heapBlock : m_DescriptorHeaps[uiHeapType])
    {
      if (heapBlock.m_pHeap != nullptr)
      {
        IUnknown* pObject = heapBlock.m_pHeap;
        m_pDeviceD3D12->SafeReleaseDeviceObject(pObject);
        heapBlock.m_pHeap = nullptr;
      }
    }

    m_DescriptorHeaps[uiHeapType].Clear();
  }
}

xiiGALDescriptorSetPoolD3D12::DescriptorAllocation xiiGALDescriptorSetPoolD3D12::RequestDescriptorAllocation(D3D12_DESCRIPTOR_HEAP_TYPE heapType, xiiUInt32 uiDescriptorCount /*= 1U*/)
{
  if (uiDescriptorCount == 0U)
  {
    xiiLog::Warning("D3D12 descriptor allocation request ignored because descriptor count is zero.");
    return {};
  }

  const xiiUInt32 uiHeapTypeIndex = GetHeapTypeIndex(heapType);
  if (uiHeapTypeIndex == xiiInvalidIndex)
  {
    xiiLog::Error("Unsupported D3D12 descriptor heap type '{}' requested.", static_cast<xiiUInt32>(heapType));
    return {};
  }

  const xiiUInt32 uiHeapBlockIndex = FindOrCreateHeap(heapType, uiDescriptorCount);
  if (uiHeapBlockIndex == xiiInvalidIndex)
    return {};

  HeapBlock& heapBlock = m_DescriptorHeaps[uiHeapTypeIndex][uiHeapBlockIndex];

  DescriptorAllocation allocation = {};
  allocation.m_pDescriptorHeap    = heapBlock.m_pHeap;
  allocation.m_uiDescriptorCount  = uiDescriptorCount;
  allocation.m_uiDescriptorSize   = heapBlock.m_uiDescriptorSize;
  allocation.m_HeapType           = heapType;

  const SIZE_T uiOffsetInBytes = static_cast<SIZE_T>(heapBlock.m_uiUsed) * static_cast<SIZE_T>(heapBlock.m_uiDescriptorSize);

  allocation.m_CPUHandle.ptr = heapBlock.m_pHeap->GetCPUDescriptorHandleForHeapStart().ptr + uiOffsetInBytes;
  allocation.m_GPUHandle.ptr = heapBlock.m_pHeap->GetGPUDescriptorHandleForHeapStart().ptr + uiOffsetInBytes;

  heapBlock.m_uiUsed += uiDescriptorCount;
  m_uiCurrentHeapIndex[uiHeapTypeIndex] = uiHeapBlockIndex;

  return allocation;
}

void xiiGALDescriptorSetPoolD3D12::Reset()
{
  for (xiiUInt32 uiHeapType = 0U; uiHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++uiHeapType)
  {
    for (HeapBlock& heapBlock : m_DescriptorHeaps[uiHeapType])
    {
      heapBlock.m_uiUsed = 0U;
    }

    m_uiCurrentHeapIndex[uiHeapType] = m_DescriptorHeaps[uiHeapType].IsEmpty() ? xiiInvalidIndex : 0U;
  }
}

ID3D12DescriptorHeap* xiiGALDescriptorSetPoolD3D12::GetCurrentDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
{
  const xiiUInt32 uiHeapTypeIndex = GetHeapTypeIndex(heapType);
  if (uiHeapTypeIndex == xiiInvalidIndex)
    return nullptr;

  const xiiUInt32 uiCurrentHeapIndex = m_uiCurrentHeapIndex[uiHeapTypeIndex];
  if (uiCurrentHeapIndex == xiiInvalidIndex || uiCurrentHeapIndex >= m_DescriptorHeaps[uiHeapTypeIndex].GetCount())
    return nullptr;

  return m_DescriptorHeaps[uiHeapTypeIndex][uiCurrentHeapIndex].m_pHeap;
}

xiiUInt32 xiiGALDescriptorSetPoolD3D12::GetHeapTypeIndex(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
  switch (heapType)
  {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
      return static_cast<xiiUInt32>(heapType);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiInvalidIndex;
}

xiiUInt32 xiiGALDescriptorSetPoolD3D12::FindOrCreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, xiiUInt32 uiRequiredDescriptorCount)
{
  const xiiUInt32 uiHeapTypeIndex = GetHeapTypeIndex(heapType);
  XII_ASSERT_DEV(uiHeapTypeIndex != xiiInvalidIndex, "Invalid descriptor heap type.");

  // Prefer the currently active heap for locality.
  if (m_uiCurrentHeapIndex[uiHeapTypeIndex] != xiiInvalidIndex)
  {
    HeapBlock& currentHeap = m_DescriptorHeaps[uiHeapTypeIndex][m_uiCurrentHeapIndex[uiHeapTypeIndex]];
    if ((currentHeap.m_uiUsed + uiRequiredDescriptorCount) <= currentHeap.m_uiCapacity)
      return m_uiCurrentHeapIndex[uiHeapTypeIndex];
  }

  // Try existing heaps first.
  for (xiiUInt32 i = 0U; i < m_DescriptorHeaps[uiHeapTypeIndex].GetCount(); ++i)
  {
    HeapBlock& heapBlock = m_DescriptorHeaps[uiHeapTypeIndex][i];
    if ((heapBlock.m_uiUsed + uiRequiredDescriptorCount) <= heapBlock.m_uiCapacity)
      return i;
  }

  // No existing heap can satisfy this request; create a new one.
  const xiiUInt32 uiCapacity = xiiMath::Max(GetDefaultHeapSize(heapType), uiRequiredDescriptorCount);

  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
  descriptorHeapDescription.Type                       = heapType;
  descriptorHeapDescription.NumDescriptors             = uiCapacity;
  descriptorHeapDescription.Flags                      = (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  descriptorHeapDescription.NodeMask                   = 0U;

  ID3D12DescriptorHeap* pDescriptorHeap = nullptr;
  const HRESULT         hResult         = m_pDeviceD3D12->GetD3D12Device()->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&pDescriptorHeap));
  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create D3D12 descriptor heap (type={}, count={}): {}.", static_cast<xiiUInt32>(heapType), uiCapacity, xiiHRESULTtoString(hResult));
    return xiiInvalidIndex;
  }

  HeapBlock& heapBlock         = m_DescriptorHeaps[uiHeapTypeIndex].ExpandAndGetRef();
  heapBlock.m_pHeap            = pDescriptorHeap;
  heapBlock.m_HeapType         = heapType;
  heapBlock.m_uiCapacity       = uiCapacity;
  heapBlock.m_uiUsed           = 0U;
  heapBlock.m_uiDescriptorSize = m_pDeviceD3D12->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType);

  return m_DescriptorHeaps[uiHeapTypeIndex].GetCount() - 1U;
}

xiiUInt32 xiiGALDescriptorSetPoolD3D12::GetDefaultHeapSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
{
  switch (heapType)
  {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
      return m_uiBaseHeapSize;

    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
      return xiiMath::Max(256U, m_uiBaseHeapSize / 4U);

    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
      return xiiMath::Max(128U, m_uiBaseHeapSize / 8U);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return m_uiBaseHeapSize;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Pools_Implementation_DescriptorSetPoolD3D12);
