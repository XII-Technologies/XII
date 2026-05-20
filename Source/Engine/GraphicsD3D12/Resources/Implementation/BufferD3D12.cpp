/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferD3D12::xiiGALBufferD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALBufferD3D12::~xiiGALBufferD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  pDeviceD3D12->SafeReleaseBuffer(m_pD3D12Buffer, m_BufferAllocation);

  m_pD3D12Buffer     = nullptr;
  m_BufferAllocation = nullptr;
}

xiiResult xiiGALBufferD3D12::InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12    = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiD3D12MemoryAllocator*        pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();
  const bool                      bHasInitialData = (pInitialData != nullptr && pInitialData->m_pData != nullptr && pInitialData->m_uiDataSize > 0U);

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

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateReservedResource(&resourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_pD3D12Buffer)));

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    return XII_SUCCESS;
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    // We do not create a D3D12 backing resource for dynamic buffers, as they are meant to be persistently mapped and updated by the CPU.
    // Instead, we will create a committed resource with the upload heap type when the buffer is first mapped for writing.
    // This allows us to avoid unnecessary memory allocation and resource creation for dynamic buffers that are never actually used.
  }
  else
  {
    xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};

    switch (m_Description.m_Usage)
    {
      case xiiGALResourceUsage::Immutable:
      case xiiGALResourceUsage::Mutable:
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Default;
      }
      break;
      case xiiGALResourceUsage::Staging:
      {
        if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
        {
          allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
          m_bReadbackBuffer               = true;
        }
        else
        {
          allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
        }

        m_bHostVisibleBuffer  = true;
        m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
      }
      break;

      case xiiGALResourceUsage::Unified:
      {
        if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read) && !m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
        {
          allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
          m_bReadbackBuffer               = true;
        }
        else
        {
          allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
        }

        m_bHostVisibleBuffer  = true;
        m_MemoryPropertyFlags = xiiGALMemoryPropertyFlags::HostCoherent;
      }
      break;

      default:
        xiiLog::Error("Unsupported buffer usage type: {}", xiiArgEnum(m_Description.m_Usage));
        return XII_FAILURE;
    }

    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      m_ExternalMemoryKind             = externalMemoryKind;
      allocationCreateInfo.m_HeapFlags = xiiD3D12MemoryHeapFlags::Shared;
      allocationCreateInfo.m_Flags     = xiiD3D12AllocationFlags::Committed;

      if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
      {
        xiiLog::Error("Importing external memory into D3D12 buffers is currently not supported.");
        return XII_FAILURE;
      }
    }

    XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, &m_pD3D12Buffer, &m_BufferAllocation));

    if (bHasInitialData)
    {
      if (m_bHostVisibleBuffer && !m_bReadbackBuffer)
      {
        void*       pMappedMemory = nullptr;
        D3D12_RANGE writeRange    = {0U, static_cast<SIZE_T>(pInitialData->m_uiDataSize)};

        if (FAILED(m_pD3D12Buffer->Map(0U, &writeRange, &pMappedMemory)))
        {
          xiiLog::Error("Failed to map host-visible D3D12 buffer '{}' for initial data upload.", GetDebugName());

          return XII_FAILURE;
        }

        xiiMemoryUtils::RawByteCopy(pMappedMemory, pInitialData->m_pData, static_cast<size_t>(pInitialData->m_uiDataSize));

        m_pD3D12Buffer->Unmap(0U, &writeRange);

        SetResourceState(xiiD3D12TypeConversions::GetResourceStateFromBindFlags(m_Description.m_BindFlags));
      }
      else
      {
        SetResourceState(xiiD3D12TypeConversions::GetResourceStateFromBindFlags(m_Description.m_BindFlags));

        auto UploadStagingData = [&](xiiGALCommandListD3D12* pCommandListD3D12) -> xiiResult {
          // The allocation will stay in the upload heap until the end of the frame at which point all upload pages will be discarded.
          xiiGALStagingBufferAllocationD3D12 stagingBufferAllocation = pCommandListD3D12->GetD3D12UploadStagingBufferPool()->Allocate(pInitialData->m_uiDataSize);
          void*                              pMappedMemory           = nullptr;
          D3D12_RANGE                        writeRange              = {stagingBufferAllocation.m_uiOffset, stagingBufferAllocation.m_uiOffset + pInitialData->m_uiDataSize};

          if (FAILED(stagingBufferAllocation.m_pD3D12Buffer->Map(0, &writeRange, &pMappedMemory)))
          {
            xiiLog::Error("Failed to map staging buffer for initial data upload.");

            return XII_FAILURE;
          }

          xiiMemoryUtils::RawByteCopy(pMappedMemory, pInitialData->m_pData, pInitialData->m_uiDataSize);

          stagingBufferAllocation.m_pD3D12Buffer->Unmap(0, &writeRange);

          return XII_SUCCESS;
        };

        if (auto pCommandListD3D12 = xiiDynamicCast<xiiGALCommandListD3D12*>(pInitialData->m_pCommandList))
        {
          XII_SUCCEED_OR_RETURN(UploadStagingData(pCommandListD3D12));
        }
        else if (auto pCommandQueue = pDeviceD3D12->GetCommandQueue(xiiGALCommandQueueFlags::Graphics))
        {
          if (auto pImmediateCommandListD3D12 = pDeviceD3D12->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics}).Downcast<xiiGALCommandListD3D12>())
          {
            pImmediateCommandListD3D12->Begin();
            {
              XII_SUCCEED_OR_RETURN(UploadStagingData(pImmediateCommandListD3D12));
            }
            pImmediateCommandListD3D12->End();

            pCommandQueue->Submit(pImmediateCommandListD3D12);
          }
        }
      }
    }
  }

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
