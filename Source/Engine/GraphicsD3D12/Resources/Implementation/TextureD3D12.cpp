/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/System/Process.h>
#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  void FillSparseTextureProperties(const xiiSharedPtr<xiiGALDeviceD3D12>& pDeviceD3D12, ID3D12Resource* pTexture, const xiiGALTextureCreationDescription& description, xiiGALSparseTextureProperties& out_sparseTextureProperties)
  {
    XII_ASSERT_DEV(pDeviceD3D12 != nullptr, "D3D12 device must be valid.");
    XII_ASSERT_DEV(pTexture != nullptr, "D3D12 texture must be valid.");

    out_sparseTextureProperties = {};

    UINT                  uiSubresourceTilingCount = 0U;
    UINT                  uiTileCount              = 0U;
    D3D12_PACKED_MIP_INFO packedMipInfo            = {};
    D3D12_TILE_SHAPE      tileShape                = {};

    pDeviceD3D12->GetD3D12Device()->GetResourceTiling(pTexture, &uiTileCount, &packedMipInfo, &tileShape, &uiSubresourceTilingCount, 0U, nullptr);

    out_sparseTextureProperties.m_uiBlockSize        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    out_sparseTextureProperties.m_uiAddressSpaceSize = static_cast<xiiUInt64>(uiTileCount) * out_sparseTextureProperties.m_uiBlockSize;
    out_sparseTextureProperties.m_vTileSize          = xiiVec3U32(tileShape.WidthInTexels, tileShape.HeightInTexels, tileShape.DepthInTexels);
    out_sparseTextureProperties.m_uiFirstMipInTail   = packedMipInfo.NumStandardMips;
    out_sparseTextureProperties.m_uiMipTailOffset    = static_cast<xiiUInt64>(packedMipInfo.StartTileIndexInOverallResource) * out_sparseTextureProperties.m_uiBlockSize;
    out_sparseTextureProperties.m_uiMipTailSize      = static_cast<xiiUInt64>(packedMipInfo.NumTilesForPackedMips) * out_sparseTextureProperties.m_uiBlockSize;

    if (description.m_uiArraySizeOrDepth > 1U && packedMipInfo.NumTilesForPackedMips > 0U)
    {
      out_sparseTextureProperties.m_Flags |= xiiGALSparseTextureFlags::SingleMipTail;
      out_sparseTextureProperties.m_uiMipTailStride = out_sparseTextureProperties.m_uiMipTailSize;
    }
  }

  xiiResult InitializeStagingTextureData(ID3D12Resource* pStagingResource, const xiiGALTextureCreationDescription& description, const xiiGALTextureData& initialData)
  {
    if (pStagingResource == nullptr)
      return XII_FAILURE;

    const xiiUInt32 uiExpectedSubresourceCount = description.m_uiMipLevels * description.m_uiArraySizeOrDepth;
    if (initialData.m_pSubResources.GetCount() != uiExpectedSubresourceCount)
    {
      xiiLog::Error("Invalid initial data for D3D12 staging texture: expected {} subresources, got {}.", uiExpectedSubresourceCount, initialData.m_pSubResources.GetCount());
      return XII_FAILURE;
    }

    const xiiGALResourceFormatDescription& formatProperties  = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);
    const xiiUInt64                        uiStagingDataSize = xiiGALTextureUtilities::GetStagingTextureDataSize(description);

    void*       pMappedMemory = nullptr;
    D3D12_RANGE writeRange    = {0U, uiStagingDataSize};
    if (FAILED(pStagingResource->Map(0U, &writeRange, &pMappedMemory)))
    {
      xiiLog::Error("Failed to map D3D12 staging texture upload buffer for initialization.");
      return XII_FAILURE;
    }

    xiiUInt32 uiSubresourceIndex = 0U;

    for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < description.m_uiArraySizeOrDepth; ++uiArraySlice)
    {
      for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < description.m_uiMipLevels; ++uiMipLevel)
      {
        const xiiGALTextureSubResourceData& subresourceData     = initialData.m_pSubResources[uiSubresourceIndex++];
        const xiiGALMipLevelProperties      mipLevelData        = xiiGALTextureUtilities::GetMipLevelProperties(description, uiMipLevel);
        const xiiUInt64                     uiDestinationOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(description, uiArraySlice, uiMipLevel, 4U);

        const xiiUInt32 uiRowCount          = mipLevelData.m_StorageSize.height / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U);
        const xiiUInt64 uiSourceRowStride   = subresourceData.m_uiStride != 0U ? subresourceData.m_uiStride : mipLevelData.m_uiRowSize;
        const xiiUInt64 uiSourceDepthStride = subresourceData.m_uiDepthStride != 0U ? subresourceData.m_uiDepthStride : uiSourceRowStride * uiRowCount;

        xiiUInt8* pDestinationSubresourceData = xiiMemoryUtils::AddByteOffset(static_cast<xiiUInt8*>(pMappedMemory), uiDestinationOffset);
        for (xiiUInt32 uiDepthSlice = 0U; uiDepthSlice < mipLevelData.m_uiDepth; ++uiDepthSlice)
        {
          const xiiUInt8* pSourceDepthSlice      = xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), uiDepthSlice * uiSourceDepthStride);
          xiiUInt8*       pDestinationDepthSlice = xiiMemoryUtils::AddByteOffset(pDestinationSubresourceData, uiDepthSlice * mipLevelData.m_uiDepthSliceSize);

          for (xiiUInt32 uiRow = 0U; uiRow < uiRowCount; ++uiRow)
          {
            memcpy(xiiMemoryUtils::AddByteOffset(pDestinationDepthSlice, uiRow * mipLevelData.m_uiRowSize), xiiMemoryUtils::AddByteOffset(pSourceDepthSlice, uiRow * uiSourceRowStride), static_cast<size_t>(mipLevelData.m_uiRowSize));
          }
        }
      }
    }

    pStagingResource->Unmap(0U, &writeRange);

    return XII_SUCCESS;
  }

  xiiResult InitializeExternalMemoryDescription(const xiiSharedPtr<xiiGALDeviceD3D12>& pDeviceD3D12, ID3D12Resource* pResource, xiiGALExternalMemoryDescription& out_externalMemoryDescription)
  {
    HANDLE  hSharedHandle = nullptr;
    HRESULT hResult       = pDeviceD3D12->GetD3D12Device()->CreateSharedHandle(pResource, nullptr, GENERIC_ALL, nullptr, &hSharedHandle);
    if (FAILED(hResult))
    {
      xiiLog::Error("Failed to create a shared handle for D3D12 texture external memory export: {}.", xiiHRESULTtoString(hResult));
      return XII_FAILURE;
    }

    const D3D12_RESOURCE_DESC            resourceDescription = pResource->GetDesc();
    const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo      = pDeviceD3D12->GetD3D12Device()->GetResourceAllocationInfo(0U, 1U, &resourceDescription);

    out_externalMemoryDescription.m_Type              = xiiGALExternalMemoryKind::Exportable;
    out_externalMemoryDescription.m_Flags             = xiiGALExternalMemoryFlags::SharedAccess;
    out_externalMemoryDescription.m_uiNativeHandle    = reinterpret_cast<uintptr_t>(hSharedHandle);
    out_externalMemoryDescription.m_uiProcessId       = xiiProcess::GetCurrentProcessID();
    out_externalMemoryDescription.m_uiSize            = allocationInfo.SizeInBytes;
    out_externalMemoryDescription.m_uiMemoryTypeIndex = 0U;

    return XII_SUCCESS;
  }
} // namespace

xiiGALTextureD3D12::xiiGALTextureD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTextureD3D12::~xiiGALTextureD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (m_ExternalMemoryDescription.m_uiNativeHandle != 0U)
  {
    CloseHandle(reinterpret_cast<HANDLE>(m_ExternalMemoryDescription.m_uiNativeHandle));

    m_ExternalMemoryDescription.m_uiNativeHandle = 0U;
  }

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    IUnknown* pObject = m_pD3D12Texture;

    pDeviceD3D12->SafeReleaseDeviceObject(pObject);

    m_pD3D12Texture = nullptr;
  }
  else
  {
    pDeviceD3D12->SafeReleaseTexture(m_pD3D12Texture, m_TextureAllocation, m_Description.m_Usage == xiiGALResourceUsage::Staging);
  }

  m_pD3D12Texture     = nullptr;
  m_TextureAllocation = nullptr;
}

xiiResult xiiGALTextureD3D12::InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12    = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiD3D12MemoryAllocator*        pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();
  const bool                      bHasInitialData = (pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty());

  if (m_Description.m_Usage == xiiGALResourceUsage::Immutable && !bHasInitialData)
  {
    xiiLog::Error("Immutable D3D12 textures must be initialized with subresource data.");

    return XII_FAILURE;
  }

  if (m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::Memoryless) && bHasInitialData)
  {
    xiiLog::Error("Memoryless D3D12 textures cannot be initialized with data.");

    return XII_FAILURE;
  }

  if (m_Description.m_pExistingNativeObject != nullptr)
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    {
      ID3D12Resource* pTextureObject        = static_cast<ID3D12Resource*>(m_Description.m_pExistingNativeObject);
      ID3D12Resource* pD3D12TextureResource = nullptr;

      if (FAILED(pTextureObject->QueryInterface(__uuidof(ID3D12Resource), reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&pD3D12TextureResource)))))
      {
        xiiLog::Error("The interface interface of the corresponding object is not a texture object.");
        return XII_FAILURE;
      }

      XII_GAL_D3D12_RELEASE(pD3D12TextureResource);
    }
#endif

    m_pD3D12Texture = static_cast<ID3D12Resource*>(m_Description.m_pExistingNativeObject);

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    return XII_SUCCESS;
  }

  D3D12_RESOURCE_DESC resourceDescription = {};
  resourceDescription.Dimension           = xiiD3D12TypeConversions::GetResourceDimension(m_Description.m_Type);
  resourceDescription.Alignment           = 0U;
  resourceDescription.Width               = m_Description.m_Size.width;
  resourceDescription.Height              = m_Description.Is1D() ? 1U : m_Description.m_Size.height;
  resourceDescription.DepthOrArraySize    = static_cast<xiiUInt16>(m_Description.m_uiArraySizeOrDepth);
  resourceDescription.MipLevels           = static_cast<xiiUInt16>(m_Description.m_uiMipLevels);
  resourceDescription.Format              = xiiD3D12TypeConversions::GetFormat(m_Description.m_Format);
  resourceDescription.SampleDesc.Count    = m_Description.m_uiSampleCount;
  resourceDescription.SampleDesc.Quality  = 0U;
  resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  resourceDescription.Flags               = xiiD3D12TypeConversions::GetTextureResourceFlagsFromBindFlags(m_Description.m_BindFlags);

  xiiD3D12MemoryAllocationCreateInfo allocationCreateInfo = {};

  m_ExternalMemoryDescription.m_Type = xiiGALExternalMemoryKind::None;

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      xiiLog::Error("Sparse D3D12 textures currently do not support external memory import/export.");

      return XII_FAILURE;
    }

    D3D12_CLEAR_VALUE clearValue = xiiD3D12TypeConversions::GetClearValue(m_Description.m_ClearValue);

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateReservedResource(&resourceDescription, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_pD3D12Texture)));

    SetResourceState(xiiGALResourceStateFlags::Undefined);

    FillSparseTextureProperties(pDeviceD3D12, m_pD3D12Texture, m_Description, m_SparseTextureProperties);

    return XII_SUCCESS;
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Immutable || m_Description.m_Usage == xiiGALResourceUsage::Mutable || m_Description.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      allocationCreateInfo.m_HeapFlags = xiiD3D12MemoryHeapFlags::Shared;
      allocationCreateInfo.m_Flags     = xiiD3D12AllocationFlags::Committed;

      if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
      {
        xiiLog::Error("Importing external memory into D3D12 texture is currently not supported.");

        return XII_FAILURE;
      }
    }

    allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Default;

    if (bHasInitialData)
    {
      // If the texture is initialized with data, we can avoid zero-initialization of the entire resource, as the initial data will be fully overwritten during initialization.

      allocationCreateInfo.m_HeapFlags |= xiiD3D12MemoryHeapFlags::CreateNotZeroed;
    }

    XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateImage(resourceDescription, allocationCreateInfo, &m_Description.m_ClearValue, &m_pD3D12Texture, &m_TextureAllocation));

    if (bHasInitialData)
    {
      auto UploadStagingData = [&](xiiGALCommandListD3D12* pCommandListD3D12) -> xiiResult {
        const xiiUInt32 uiSubresourceCount = m_Description.m_uiMipLevels * m_Description.m_uiArraySizeOrDepth;
        if (pInitialData->m_pSubResources.GetCount() != uiSubresourceCount)
        {
          xiiLog::Error("Invalid D3D12 texture initial data: expected {} subresources, got {}.", uiSubresourceCount, pInitialData->m_pSubResources.GetCount());

          return XII_FAILURE;
        }

        xiiTemporaryArray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedSubresourceFootprints;
        placedSubresourceFootprints.SetCount(uiSubresourceCount);

        UINT64                    uiUploadBufferSize     = 0U;
        const D3D12_RESOURCE_DESC destinationDescription = m_pD3D12Texture->GetDesc();
        pDeviceD3D12->GetD3D12Device()->GetCopyableFootprints(&destinationDescription, 0U, uiSubresourceCount, 0U, placedSubresourceFootprints.GetData(), nullptr, nullptr, &uiUploadBufferSize);

        // The allocation will stay in the upload heap until the end of the frame at which point all upload pages will be discarded.
        xiiGALStagingBufferAllocationD3D12 stagingBufferAllocation = pCommandListD3D12->GetD3D12UploadStagingBufferPool()->Allocate(uiUploadBufferSize);
        void*                              pMappedMemory           = nullptr;
        D3D12_RANGE                        writeRange              = {stagingBufferAllocation.m_uiOffset, stagingBufferAllocation.m_uiOffset + uiUploadBufferSize};

        if (FAILED(stagingBufferAllocation.m_pD3D12Buffer->Map(0, &writeRange, &pMappedMemory)))
        {
          xiiLog::Error("Failed to map staging buffer for initial data upload.");

          return XII_FAILURE;
        }

        pMappedMemory                                           = xiiMemoryUtils::AddByteOffset(pMappedMemory, stagingBufferAllocation.m_uiOffset);
        const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);

        for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < m_Description.m_uiArraySizeOrDepth; ++uiArraySlice)
        {
          for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < m_Description.m_uiMipLevels; ++uiMipLevel)
          {
            const xiiUInt32                     uiSubresource   = xiiD3D12TypeConversions::CalculateSubResourceIndex(uiMipLevel, uiArraySlice, m_Description.m_uiMipLevels);
            const xiiGALTextureSubResourceData& subresourceData = pInitialData->m_pSubResources[uiSubresource];
            const xiiGALMipLevelProperties      mipLevelData    = xiiGALTextureUtilities::GetMipLevelProperties(m_Description, uiMipLevel);

            const xiiUInt32 uiRowCount          = mipLevelData.m_StorageSize.height / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U);
            const xiiUInt64 uiSourceRowStride   = subresourceData.m_uiStride != 0U ? subresourceData.m_uiStride : mipLevelData.m_uiRowSize;
            const xiiUInt64 uiSourceDepthStride = subresourceData.m_uiDepthStride != 0U ? subresourceData.m_uiDepthStride : uiSourceRowStride * uiRowCount;

            xiiUInt8* pDestinationSubresourceData = xiiMemoryUtils::AddByteOffset(static_cast<xiiUInt8*>(pMappedMemory), placedSubresourceFootprints[uiSubresource].Offset);
            for (xiiUInt32 uiDepthSlice = 0U; uiDepthSlice < mipLevelData.m_uiDepth; ++uiDepthSlice)
            {
              const xiiUInt8* pSourceDepthSlice      = xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), uiDepthSlice * uiSourceDepthStride);
              xiiUInt8*       pDestinationDepthSlice = xiiMemoryUtils::AddByteOffset(pDestinationSubresourceData, static_cast<xiiUInt64>(placedSubresourceFootprints[uiSubresource].Footprint.RowPitch) * uiRowCount * uiDepthSlice);

              for (xiiUInt32 uiRow = 0U; uiRow < uiRowCount; ++uiRow)
              {
                memcpy(xiiMemoryUtils::AddByteOffset(pDestinationDepthSlice, static_cast<xiiUInt64>(placedSubresourceFootprints[uiSubresource].Footprint.RowPitch) * uiRow), xiiMemoryUtils::AddByteOffset(pSourceDepthSlice, uiSourceRowStride * uiRow), static_cast<size_t>(mipLevelData.m_uiRowSize));
              }
            }
          }
        }

        stagingBufferAllocation.m_pD3D12Buffer->Unmap(0, &writeRange);

        for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < m_Description.m_uiArraySizeOrDepth; ++uiArraySlice)
        {
          for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < m_Description.m_uiMipLevels; ++uiMipLevel)
          {
            const xiiUInt32 uiSubresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(uiMipLevel, uiArraySlice, m_Description.m_uiMipLevels);

            D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
            sourceLocation.pResource                   = stagingBufferAllocation.m_pD3D12Buffer;
            sourceLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            sourceLocation.PlacedFootprint             = placedSubresourceFootprints[uiSubresource];

            D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
            destinationLocation.pResource                   = m_pD3D12Texture;
            destinationLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destinationLocation.SubresourceIndex            = uiSubresource;

            pCommandListD3D12->GetD3D12CommandList()->CopyTextureRegion(&destinationLocation, 0U, 0U, 0U, &sourceLocation, nullptr);
          }
        }

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

    SetResourceState(bHasInitialData ? xiiGALResourceStateFlags::CopyDestination : xiiGALResourceStateFlags::Undefined);
  }
  else if (m_Description.m_Usage == xiiGALResourceUsage::Staging)
  {
    if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
    {
      allocationCreateInfo.m_HeapFlags = xiiD3D12MemoryHeapFlags::Shared;
      allocationCreateInfo.m_Flags     = xiiD3D12AllocationFlags::Committed;

      if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
      {
        xiiLog::Error("Importing external memory into D3D12 texture is currently not supported.");

        return XII_FAILURE;
      }
    }

    resourceDescription.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment          = 0U;
    resourceDescription.Width              = xiiGALTextureUtilities::GetStagingTextureDataSize(m_Description);
    resourceDescription.Height             = 1U;
    resourceDescription.DepthOrArraySize   = 1U;
    resourceDescription.MipLevels          = 1U;
    resourceDescription.Format             = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count   = 1U;
    resourceDescription.SampleDesc.Quality = 0U;
    resourceDescription.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
    {
      allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
      m_bReadbackTexture              = true;
    }
    else
    {
      allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
      m_bHostVisibleUpload            = true;
    }

    XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, &m_pD3D12Texture, &m_TextureAllocation));

    if (bHasInitialData)
    {
      if (m_bReadbackTexture)
      {
        xiiLog::Error("Read-back staging D3D12 textures cannot be initialized with CPU-side data.");

        return XII_FAILURE;
      }

      XII_SUCCEED_OR_RETURN(InitializeStagingTextureData(m_pD3D12Texture, m_Description, *pInitialData));
    }

    SetResourceState(bHasInitialData ? xiiGALResourceStateFlags::CopyDestination : xiiGALResourceStateFlags::Undefined);
  }
  else
  {
    xiiLog::Error("D3D12 texture usage '{}' is currently unsupported.", xiiArgEnum(m_Description.m_Usage));

    return XII_FAILURE;
  }

  if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Exportable))
  {
    XII_SUCCEED_OR_RETURN(InitializeExternalMemoryDescription(pDeviceD3D12, m_pD3D12Texture, m_ExternalMemoryDescription));
  }

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALTextureView> xiiGALTextureD3D12::CreateViewPlatform(const xiiGALTextureViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D12>                  pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiInternal::NewInstance<xiiGALTextureViewD3D12> pTextureViewD3D12 = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALTextureViewD3D12, pDeviceD3D12, xiiSharedPtr<xiiGALTexture>(this, pDeviceD3D12->GetAllocator()), description);

  if (pTextureViewD3D12->InitPlatform().Succeeded())
    return pTextureViewD3D12;

  XII_DELETE(pTextureViewD3D12.m_pAllocator, pTextureViewD3D12.m_pInstance);

  return pTextureViewD3D12;
}

void xiiGALTextureD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_pD3D12Texture != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pD3D12Texture->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D12 texture debug name.");
    }
  }
}

const xiiGALSparseTextureProperties& xiiGALTextureD3D12::GetSparseProperties() const
{
  return m_SparseTextureProperties;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureD3D12);
