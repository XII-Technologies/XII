/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  xiiResult UploadInitialDataWithImmediateCommandList(const xiiSharedPtr<xiiGALDeviceD3D12>& pDeviceD3D12, ID3D12Resource* pDestinationBuffer, const D3D12_RESOURCE_DESC& destinationResourceDescription, const void* pData, xiiUInt64 uiDataSize, xiiBitflags<xiiGALResourceStateFlags> finalState)
  {
    XII_ASSERT_DEV(pDeviceD3D12 != nullptr, "Invalid D3D12 device.");
    XII_ASSERT_DEV(pDestinationBuffer != nullptr, "Invalid destination D3D12 buffer.");
    XII_ASSERT_DEV(pData != nullptr, "Invalid source data pointer.");
    XII_ASSERT_DEV(uiDataSize > 0U, "Source data size must not be zero.");

    xiiD3D12MemoryAllocator* pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();
    XII_ASSERT_DEV(pD3D12Allocator != nullptr, "D3D12 memory allocator must be initialized before uploading initial buffer data.");

    D3D12_RESOURCE_DESC uploadResourceDescription = destinationResourceDescription;
    uploadResourceDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource*            pUploadBuffer      = nullptr;
    xiiD3D12Allocation         uploadAllocation   = nullptr;
    ID3D12Fence*               pUploadFence       = nullptr;
    bool                       bPooledUploadFence = false;
    HANDLE                     hUploadFenceSignal = nullptr;
    ID3D12CommandAllocator*    pCommandAllocator  = nullptr;
    ID3D12GraphicsCommandList* pCommandList       = nullptr;

    XII_SCOPE_EXIT(
      {
        if (pUploadFence != nullptr)
        {
          if (bPooledUploadFence)
          {
            if (xiiGALFencePoolD3D12* pFencePoolD3D12 = pDeviceD3D12->GetD3D12FencePool())
            {
              pFencePoolD3D12->ReclaimFence(pUploadFence);
            }
            else
            {
              XII_GAL_D3D12_RELEASE(pUploadFence);
            }
          }
          else
          {
            XII_GAL_D3D12_RELEASE(pUploadFence);
          }
        }
        XII_GAL_D3D12_RELEASE(pCommandList);
        XII_GAL_D3D12_RELEASE(pCommandAllocator);

        if (hUploadFenceSignal != nullptr && hUploadFenceSignal != INVALID_HANDLE_VALUE)
        {
          CloseHandle(hUploadFenceSignal);
          hUploadFenceSignal = nullptr;
        }

        if (pUploadBuffer != nullptr)
        {
          pD3D12Allocator->DestroyBuffer(pUploadBuffer, uploadAllocation);
        }
      });

    xiiD3D12MemoryAllocationCreateInfo uploadAllocationCreateInfo = {};
    uploadAllocationCreateInfo.m_HeapType                         = xiiD3D12MemoryHeapType::Upload;

    XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(uploadResourceDescription, uploadAllocationCreateInfo, xiiGALResourceStateFlags::CopySource, &pUploadBuffer, &uploadAllocation));

    void*       pMappedMemory = nullptr;
    D3D12_RANGE readRange     = {0U, 0U};
    if (FAILED(pUploadBuffer->Map(0U, &readRange, &pMappedMemory)))
    {
      xiiLog::Error("Failed to map temporary D3D12 upload buffer for initial data upload.");
      return XII_FAILURE;
    }

    xiiMemoryUtils::RawByteCopy(pMappedMemory, pData, static_cast<size_t>(uiDataSize));

    D3D12_RANGE writeRange = {0U, static_cast<SIZE_T>(uiDataSize)};
    pUploadBuffer->Unmap(0U, &writeRange);

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));
    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));

    pCommandList->CopyBufferRegion(pDestinationBuffer, 0U, pUploadBuffer, 0U, uiDataSize);

    if (finalState != xiiGALResourceStateFlags::CopyDestination)
    {
      D3D12_RESOURCE_BARRIER resourceBarrier = {};
      resourceBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      resourceBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      resourceBarrier.Transition.pResource   = pDestinationBuffer;
      resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      resourceBarrier.Transition.StateAfter  = xiiD3D12TypeConversions::GetResourceState(finalState);

      pCommandList->ResourceBarrier(1U, &resourceBarrier);
    }

    XII_HRESULT_TO_FAILURE_LOG(pCommandList->Close());

    xiiGALCommandQueueD3D12* pCommandQueueD3D12 = xiiDynamicCast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetCommandQueue(xiiGALCommandQueueFlags::Graphics));
    if (pCommandQueueD3D12 == nullptr || pCommandQueueD3D12->GetD3D12CommandQueue() == nullptr)
    {
      xiiLog::Error("Failed to upload initial data to D3D12 buffer: no valid graphics queue is available.");
      return XII_FAILURE;
    }

    ID3D12CommandList* pCommandLists[] = {reinterpret_cast<ID3D12CommandList*>(pCommandList)};
    pCommandQueueD3D12->GetD3D12CommandQueue()->ExecuteCommandLists(1U, pCommandLists);

    xiiGALFencePoolD3D12* pFencePoolD3D12 = pDeviceD3D12->GetD3D12FencePool();
    if (pFencePoolD3D12 == nullptr)
    {
      xiiLog::Error("Failed to upload initial data to D3D12 buffer: fence pool is unavailable.");
      return XII_FAILURE;
    }

    pUploadFence = pFencePoolD3D12->RequestFence();
    if (pUploadFence == nullptr)
    {
      xiiLog::Error("Failed to upload initial data to D3D12 buffer: fence pool failed to provide a fence.");
      return XII_FAILURE;
    }

    bPooledUploadFence = true;
    pUploadFence->Signal(0U);

    constexpr xiiUInt64 uiFenceValue = 1ULL;
    XII_HRESULT_TO_FAILURE_LOG(pCommandQueueD3D12->GetD3D12CommandQueue()->Signal(pUploadFence, uiFenceValue));

    if (pUploadFence->GetCompletedValue() < uiFenceValue)
    {
      hUploadFenceSignal = CreateEvent(nullptr, FALSE, FALSE, nullptr);

      if (hUploadFenceSignal == nullptr || hUploadFenceSignal == INVALID_HANDLE_VALUE)
      {
        xiiLog::Error("Failed to create D3D12 upload fence event.");
        return XII_FAILURE;
      }

      XII_HRESULT_TO_FAILURE_LOG(pUploadFence->SetEventOnCompletion(uiFenceValue, hUploadFenceSignal));

      WaitForSingleObject(hUploadFenceSignal, INFINITE);
    }

    return XII_SUCCESS;
  }
} // namespace

xiiGALBufferD3D12::xiiGALBufferD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALBufferD3D12::~xiiGALBufferD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (m_pD3D12Buffer != nullptr)
  {
    if (pDeviceD3D12 == nullptr)
    {
      XII_GAL_D3D12_RELEASE(m_pD3D12Buffer);
      XII_GAL_D3D12_RELEASE(m_BufferAllocation);
    }
    else if (m_Description.m_Usage == xiiGALResourceUsage::Sparse || m_BufferAllocation == nullptr)
    {
      IUnknown* pObject = m_pD3D12Buffer;
      pDeviceD3D12->SafeReleaseDeviceObject(pObject);
      m_pD3D12Buffer = nullptr;
    }
    else
    {
      pDeviceD3D12->SafeReleaseBuffer(m_pD3D12Buffer, m_BufferAllocation);
    }
  }

  m_pD3D12Buffer     = nullptr;
  m_BufferAllocation = nullptr;
}

xiiResult xiiGALBufferD3D12::InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiSharedPtr<xiiGALDeviceD3D12>       pDeviceD3D12    = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiD3D12MemoryAllocator*              pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();
  const bool                            bHasInitialData = (pInitialData != nullptr && pInitialData->m_pData != nullptr && pInitialData->m_uiDataSize > 0U);
  xiiBitflags<xiiGALResourceStateFlags> desiredState    = xiiD3D12TypeConversions::GetResourceStateFromBindFlags(m_Description.m_BindFlags);

  if (desiredState == xiiGALResourceStateFlags::Undefined)
  {
    desiredState = xiiGALResourceStateFlags::Common;
  }

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = m_Description.m_uiSize;
  resourceDescription.Height              = 1U;
  resourceDescription.DepthOrArraySize    = 1U;
  resourceDescription.MipLevels           = 1U;
  resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
  resourceDescription.SampleDesc.Count    = 1U;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetBufferResourceFlagsFromBindFlags(m_Description.m_BindFlags);

  m_ExternalMemoryKind = xiiGALExternalMemoryKind::None;

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      xiiLog::Error("Sparse D3D12 buffers currently do not support external memory import/export.");
      return XII_FAILURE;
    }

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateReservedResource(&resourceDescription, xiiD3D12TypeConversions::GetResourceState(desiredState), nullptr, IID_PPV_ARGS(&m_pD3D12Buffer)));

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    return XII_SUCCESS;
  }

  xiiD3D12MemoryAllocationCreateInfo    allocationCreateInfo = {};
  xiiBitflags<xiiGALResourceStateFlags> creationState        = desiredState;
  xiiBitflags<xiiGALResourceStateFlags> finalState           = desiredState;

  switch (m_Description.m_Usage)
  {
    case xiiGALResourceUsage::Immutable:
    case xiiGALResourceUsage::Mutable:
      allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Default;
      if (bHasInitialData)
      {
        creationState = xiiGALResourceStateFlags::CopyDestination;
      }
      break;

    case xiiGALResourceUsage::Dynamic:
      allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
      creationState                   = xiiD3D12TypeConversions::GetDynamicBufferState();
      finalState                      = creationState;
      m_bHostVisibleBuffer            = true;
      m_MemoryPropertyFlags           = xiiGALMemoryPropertyFlags::HostCoherent;
      break;

    case xiiGALResourceUsage::Staging:
      if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
        creationState                   = xiiGALResourceStateFlags::CopyDestination;
        finalState                      = creationState;
        m_bReadbackBuffer               = true;
      }
      else
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
        creationState                   = xiiGALResourceStateFlags::CopySource;
        finalState                      = creationState;
      }
      m_bHostVisibleBuffer  = true;
      m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
      break;

    case xiiGALResourceUsage::Unified:
      if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read) && !m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
        creationState                   = xiiGALResourceStateFlags::CopyDestination;
        finalState                      = creationState;
        m_bReadbackBuffer               = true;
      }
      else
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
        creationState |= xiiGALResourceStateFlags::CopySource;
        finalState = creationState;
      }
      m_bHostVisibleBuffer  = true;
      m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
      break;

    default:
      XII_REPORT_FAILURE("Unhandled D3D12 buffer usage.");
      return XII_FAILURE;
  }

  if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
  {
    m_ExternalMemoryKind             = externalMemoryKind;
    allocationCreateInfo.m_HeapFlags = xiiD3D12MemoryHeapFlags::Shared;
    allocationCreateInfo.m_Flags     = xiiD3D12AllocationFlags::Committed;

    if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
    {
      xiiLog::Warning("D3D12 buffer creation requested imported external memory, but no buffer external-memory descriptor is currently available in GraphicsFoundation. Falling back to engine-owned shared allocation.");
    }
  }

  XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, creationState, &m_pD3D12Buffer, &m_BufferAllocation));

  if (bHasInitialData)
  {
    if (m_bHostVisibleBuffer && !m_bReadbackBuffer)
    {
      void*       pMappedMemory = nullptr;
      D3D12_RANGE readRange     = {0U, 0U};
      if (FAILED(m_pD3D12Buffer->Map(0U, &readRange, &pMappedMemory)))
      {
        xiiLog::Error("Failed to map host-visible D3D12 buffer '{}' for initial data upload.", GetDebugName());
        return XII_FAILURE;
      }

      xiiMemoryUtils::RawByteCopy(pMappedMemory, pInitialData->m_pData, static_cast<size_t>(pInitialData->m_uiDataSize));

      D3D12_RANGE writeRange = {0U, static_cast<SIZE_T>(pInitialData->m_uiDataSize)};
      m_pD3D12Buffer->Unmap(0U, &writeRange);
    }
    else
    {
      XII_SUCCEED_OR_RETURN(UploadInitialDataWithImmediateCommandList(pDeviceD3D12, m_pD3D12Buffer, resourceDescription, pInitialData->m_pData, pInitialData->m_uiDataSize, finalState));
      creationState = finalState;
    }
  }

  SetResourceState(creationState);

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALBufferView> xiiGALBufferD3D12::CreateViewPlatform(const xiiGALBufferViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D12>                 pDeviceD3D12     = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiInternal::NewInstance<xiiGALBufferViewD3D12> pBufferViewD3D12 = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALBufferViewD3D12, pDeviceD3D12, xiiSharedPtr<xiiGALBuffer>(this, pDeviceD3D12->GetAllocator()), description);

  if (pBufferViewD3D12->InitPlatform().Succeeded())
    return pBufferViewD3D12;

  XII_DELETE(pBufferViewD3D12.m_pAllocator, pBufferViewD3D12.m_pInstance);

  return pBufferViewD3D12;
}

void xiiGALBufferD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_pD3D12Buffer != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D12 buffer debug name.");
    }
  }
}

void xiiGALBufferD3D12::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_pD3D12Buffer == nullptr)
    return;

  VerifyFlushMappedRangeArguments(uiStartOffset, uiSize);
}

void xiiGALBufferD3D12::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  if (m_pD3D12Buffer == nullptr)
    return;

  VerifyInvalidateMappedRangeArguments(uiStartOffset, uiSize);
}

xiiGALSparseBufferProperties xiiGALBufferD3D12::GetSparseProperties() const
{
  XII_ASSERT_DEV(m_Description.m_Usage == xiiGALResourceUsage::Sparse, "xiiGALBuffer::GetSparseProperties() must be used for sparse buffers.");

  xiiGALSparseBufferProperties sparseBufferProperties = {};

  if (m_pD3D12Buffer == nullptr)
    return sparseBufferProperties;

  xiiSharedPtr<xiiGALDeviceD3D12>      pDeviceD3D12        = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  const D3D12_RESOURCE_DESC            resourceDescription = m_pD3D12Buffer->GetDesc();
  const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo      = pDeviceD3D12->GetD3D12Device()->GetResourceAllocationInfo(0U, 1U, &resourceDescription);

  sparseBufferProperties.m_uiAddressSpaceSize = resourceDescription.Width;
  sparseBufferProperties.m_uiBlockSize        = static_cast<xiiUInt32>(allocationInfo.Alignment);

  return sparseBufferProperties;
}

xiiUInt64 xiiGALBufferD3D12::GetD3D12BufferGPUVirtualAddress() const
{
  if (m_pD3D12Buffer == nullptr)
    return 0U;

  return m_pD3D12Buffer->GetGPUVirtualAddress();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferD3D12);
