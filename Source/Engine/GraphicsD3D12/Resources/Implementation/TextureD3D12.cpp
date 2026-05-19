/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/System/Process.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <vector>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] D3D12_RESOURCE_DESC BuildTextureResourceDescription(const xiiGALTextureCreationDescription& description)
  {
    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Dimension           = xiiD3D12TypeConversions::GetResourceDimension(description.m_Type);
    resourceDescription.Alignment           = 0U;
    resourceDescription.Width               = description.m_Size.width;
    resourceDescription.Height              = description.Is1D() ? 1U : description.m_Size.height;
    resourceDescription.DepthOrArraySize    = static_cast<xiiUInt16>(description.Is3D() ? description.GetDepth() : description.GetArraySize());
    resourceDescription.MipLevels           = static_cast<xiiUInt16>(description.m_uiMipLevels);
    resourceDescription.Format              = xiiD3D12TypeConversions::GetFormat(description.m_Format);
    resourceDescription.SampleDesc.Count    = description.m_uiSampleCount;
    resourceDescription.SampleDesc.Quality  = 0U;
    resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags               = xiiD3D12TypeConversions::GetTextureResourceFlagsFromBindFlags(description.m_BindFlags);

    return resourceDescription;
  }

  [[nodiscard]] D3D12_RESOURCE_DESC BuildStagingBufferDescription(const xiiGALTextureCreationDescription& description)
  {
    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment           = 0U;
    resourceDescription.Width               = xiiGALTextureUtilities::GetStagingTextureDataSize(description, 4U);
    resourceDescription.Height              = 1U;
    resourceDescription.DepthOrArraySize    = 1U;
    resourceDescription.MipLevels           = 1U;
    resourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count    = 1U;
    resourceDescription.SampleDesc.Quality  = 0U;
    resourceDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;

    return resourceDescription;
  }

  [[nodiscard]] D3D12_RESOURCE_STATES GetTextureUploadFinalState(xiiBitflags<xiiGALResourceStateFlags> resourceStateFlags)
  {
    xiiBitflags<xiiGALResourceStateFlags> stateFlags = resourceStateFlags;
    if (stateFlags == xiiGALResourceStateFlags::Undefined)
    {
      stateFlags = xiiGALResourceStateFlags::Common;
    }
    return xiiD3D12TypeConversions::GetResourceState(stateFlags);
  }

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

    if (description.GetArraySize() > 1U && packedMipInfo.NumTilesForPackedMips > 0U)
    {
      out_sparseTextureProperties.m_Flags |= xiiGALSparseTextureFlags::SingleMipTail;
      out_sparseTextureProperties.m_uiMipTailStride = out_sparseTextureProperties.m_uiMipTailSize;
    }
  }

  xiiResult InitializeStagingTextureData(ID3D12Resource* pStagingResource, const xiiGALTextureCreationDescription& description, const xiiGALTextureData& initialData)
  {
    if (pStagingResource == nullptr)
      return XII_FAILURE;

    const xiiUInt32 uiExpectedSubresourceCount = description.m_uiMipLevels * description.GetArraySize();
    if (initialData.m_pSubResources.GetCount() != uiExpectedSubresourceCount)
    {
      xiiLog::Error("Invalid initial data for D3D12 staging texture: expected {} subresources, got {}.", uiExpectedSubresourceCount, initialData.m_pSubResources.GetCount());
      return XII_FAILURE;
    }

    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);

    void*       pMappedMemory = nullptr;
    D3D12_RANGE readRange     = {0U, 0U};
    if (FAILED(pStagingResource->Map(0U, &readRange, &pMappedMemory)))
    {
      xiiLog::Error("Failed to map D3D12 staging texture upload buffer for initialization.");
      return XII_FAILURE;
    }

    xiiUInt32 uiSubresourceIndex = 0U;

    for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < description.GetArraySize(); ++uiArraySlice)
    {
      for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < description.m_uiMipLevels; ++uiMipLevel)
      {
        const xiiGALTextureSubResourceData& subresourceData = initialData.m_pSubResources[uiSubresourceIndex++];
        const xiiGALMipLevelProperties      mipLevelData    = xiiGALTextureUtilities::GetMipLevelProperties(description, uiMipLevel);
        const xiiUInt64 uiDestinationOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(description, uiArraySlice, uiMipLevel, 4U);

        const xiiUInt32 uiRowCount          = mipLevelData.m_StorageSize.height / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U);
        const xiiUInt64 uiSourceRowStride   = subresourceData.m_uiStride != 0U ? subresourceData.m_uiStride : mipLevelData.m_uiRowSize;
        const xiiUInt64 uiSourceDepthStride = subresourceData.m_uiDepthStride != 0U ? subresourceData.m_uiDepthStride : uiSourceRowStride * uiRowCount;

        BYTE* pDestinationSubresourceData = xiiMemoryUtils::AddByteOffset(static_cast<BYTE*>(pMappedMemory), uiDestinationOffset);
        for (xiiUInt32 uiDepthSlice = 0U; uiDepthSlice < mipLevelData.m_uiDepth; ++uiDepthSlice)
        {
          const BYTE* pSourceDepthSlice = xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), uiDepthSlice * uiSourceDepthStride);
          BYTE*       pDestinationDepthSlice = xiiMemoryUtils::AddByteOffset(pDestinationSubresourceData, uiDepthSlice * mipLevelData.m_uiDepthSliceSize);

          for (xiiUInt32 uiRow = 0U; uiRow < uiRowCount; ++uiRow)
          {
            memcpy(xiiMemoryUtils::AddByteOffset(pDestinationDepthSlice, uiRow * mipLevelData.m_uiRowSize), xiiMemoryUtils::AddByteOffset(pSourceDepthSlice, uiRow * uiSourceRowStride), static_cast<size_t>(mipLevelData.m_uiRowSize));
          }
        }
      }
    }

    D3D12_RANGE writeRange = {};
    writeRange.Begin       = 0U;
    writeRange.End         = 0U;
    pStagingResource->Unmap(0U, &writeRange);

    return XII_SUCCESS;
  }

  xiiResult UploadInitialTextureDataWithImmediateCommandList(const xiiSharedPtr<xiiGALDeviceD3D12>& pDeviceD3D12, ID3D12Resource* pDestinationTexture, const xiiGALTextureCreationDescription& textureDescription, const xiiGALTextureData& initialData, xiiBitflags<xiiGALResourceStateFlags> finalState)
  {
    XII_ASSERT_DEV(pDeviceD3D12 != nullptr, "D3D12 device must be valid.");
    XII_ASSERT_DEV(pDestinationTexture != nullptr, "Destination D3D12 texture must be valid.");

    const xiiUInt32 uiSubresourceCount = textureDescription.m_uiMipLevels * textureDescription.GetArraySize();
    if (initialData.m_pSubResources.GetCount() != uiSubresourceCount)
    {
      xiiLog::Error("Invalid D3D12 texture initial data: expected {} subresources, got {}.", uiSubresourceCount, initialData.m_pSubResources.GetCount());
      return XII_FAILURE;
    }

    const D3D12_RESOURCE_DESC destinationDescription = pDestinationTexture->GetDesc();

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedSubresourceFootprints(uiSubresourceCount);

    UINT64 uiUploadBufferSize = 0U;
    pDeviceD3D12->GetD3D12Device()->GetCopyableFootprints(&destinationDescription, 0U, uiSubresourceCount, 0U, placedSubresourceFootprints.data(), nullptr, nullptr, &uiUploadBufferSize);

    D3D12_RESOURCE_DESC uploadBufferDescription = {};
    uploadBufferDescription.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadBufferDescription.Alignment           = 0U;
    uploadBufferDescription.Width               = uiUploadBufferSize;
    uploadBufferDescription.Height              = 1U;
    uploadBufferDescription.DepthOrArraySize    = 1U;
    uploadBufferDescription.MipLevels           = 1U;
    uploadBufferDescription.Format              = DXGI_FORMAT_UNKNOWN;
    uploadBufferDescription.SampleDesc.Count    = 1U;
    uploadBufferDescription.SampleDesc.Quality  = 0U;
    uploadBufferDescription.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadBufferDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;

    xiiD3D12MemoryAllocator* pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();

    ID3D12Resource*     pUploadBuffer      = nullptr;
    xiiD3D12Allocation  uploadAllocation   = nullptr;
    ID3D12Fence*        pUploadFence       = nullptr;
    HANDLE              hUploadFenceSignal = nullptr;
    ID3D12CommandAllocator*    pCommandAllocator = nullptr;
    ID3D12GraphicsCommandList* pCommandList      = nullptr;

    XII_SCOPE_EXIT(
      {
        XII_GAL_D3D12_RELEASE(pUploadFence);
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

    XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(uploadBufferDescription, uploadAllocationCreateInfo, xiiGALResourceStateFlags::CopySource, &pUploadBuffer, &uploadAllocation));

    const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

    void*       pMappedUploadData = nullptr;
    D3D12_RANGE uploadReadRange   = {0U, 0U};
    if (FAILED(pUploadBuffer->Map(0U, &uploadReadRange, &pMappedUploadData)))
    {
      xiiLog::Error("Failed to map D3D12 upload buffer while initializing texture data.");
      return XII_FAILURE;
    }

    for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < textureDescription.GetArraySize(); ++uiArraySlice)
    {
      for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < textureDescription.m_uiMipLevels; ++uiMipLevel)
      {
        const xiiUInt32 uiSubresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(uiMipLevel, uiArraySlice, textureDescription.m_uiMipLevels);
        const xiiGALTextureSubResourceData& subresourceData = initialData.m_pSubResources[uiSubresource];
        const xiiGALMipLevelProperties      mipLevelData    = xiiGALTextureUtilities::GetMipLevelProperties(textureDescription, uiMipLevel);

        const xiiUInt32 uiRowCount        = mipLevelData.m_StorageSize.height / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U);
        const xiiUInt64 uiSourceRowStride = subresourceData.m_uiStride != 0U ? subresourceData.m_uiStride : mipLevelData.m_uiRowSize;
        const xiiUInt64 uiSourceDepthStride = subresourceData.m_uiDepthStride != 0U ? subresourceData.m_uiDepthStride : uiSourceRowStride * uiRowCount;

        BYTE* pDestinationSubresourceData = xiiMemoryUtils::AddByteOffset(static_cast<BYTE*>(pMappedUploadData), placedSubresourceFootprints[uiSubresource].Offset);
        for (xiiUInt32 uiDepthSlice = 0U; uiDepthSlice < mipLevelData.m_uiDepth; ++uiDepthSlice)
        {
          const BYTE* pSourceDepthSlice = xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), uiDepthSlice * uiSourceDepthStride);
          BYTE*       pDestinationDepthSlice = xiiMemoryUtils::AddByteOffset(pDestinationSubresourceData, static_cast<xiiUInt64>(placedSubresourceFootprints[uiSubresource].Footprint.RowPitch) * uiRowCount * uiDepthSlice);

          for (xiiUInt32 uiRow = 0U; uiRow < uiRowCount; ++uiRow)
          {
            memcpy(xiiMemoryUtils::AddByteOffset(pDestinationDepthSlice, static_cast<xiiUInt64>(placedSubresourceFootprints[uiSubresource].Footprint.RowPitch) * uiRow), xiiMemoryUtils::AddByteOffset(pSourceDepthSlice, uiSourceRowStride * uiRow), static_cast<size_t>(mipLevelData.m_uiRowSize));
          }
        }
      }
    }

    D3D12_RANGE uploadWriteRange = {0U, static_cast<SIZE_T>(uiUploadBufferSize)};
    pUploadBuffer->Unmap(0U, &uploadWriteRange);

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));
    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));

    for (xiiUInt32 uiArraySlice = 0U; uiArraySlice < textureDescription.GetArraySize(); ++uiArraySlice)
    {
      for (xiiUInt32 uiMipLevel = 0U; uiMipLevel < textureDescription.m_uiMipLevels; ++uiMipLevel)
      {
        const xiiUInt32 uiSubresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(uiMipLevel, uiArraySlice, textureDescription.m_uiMipLevels);

        D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
        sourceLocation.pResource                   = pUploadBuffer;
        sourceLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        sourceLocation.PlacedFootprint             = placedSubresourceFootprints[uiSubresource];

        D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
        destinationLocation.pResource                   = pDestinationTexture;
        destinationLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destinationLocation.SubresourceIndex            = uiSubresource;

        pCommandList->CopyTextureRegion(&destinationLocation, 0U, 0U, 0U, &sourceLocation, nullptr);
      }
    }

    if (finalState != xiiGALResourceStateFlags::CopyDestination)
    {
      D3D12_RESOURCE_BARRIER resourceBarrier = {};
      resourceBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      resourceBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      resourceBarrier.Transition.pResource   = pDestinationTexture;
      resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
      resourceBarrier.Transition.StateAfter  = GetTextureUploadFinalState(finalState);

      pCommandList->ResourceBarrier(1U, &resourceBarrier);
    }

    XII_HRESULT_TO_FAILURE_LOG(pCommandList->Close());

    xiiGALCommandQueueD3D12* pGraphicsQueue = xiiDynamicCast<xiiGALCommandQueueD3D12*>(pDeviceD3D12->GetCommandQueue(xiiGALCommandQueueFlags::Graphics));
    if (pGraphicsQueue == nullptr || pGraphicsQueue->GetD3D12CommandQueue() == nullptr)
    {
      xiiLog::Error("Failed to upload initial data to D3D12 texture: no valid graphics command queue is available.");
      return XII_FAILURE;
    }

    ID3D12CommandList* pCommandLists[] = {reinterpret_cast<ID3D12CommandList*>(pCommandList)};
    pGraphicsQueue->GetD3D12CommandQueue()->ExecuteCommandLists(1U, pCommandLists);

    XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pUploadFence)));

    constexpr xiiUInt64 uiFenceValue = 1ULL;
    XII_HRESULT_TO_FAILURE_LOG(pGraphicsQueue->GetD3D12CommandQueue()->Signal(pUploadFence, uiFenceValue));

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

  xiiResult InitializeExternalMemoryDescription(const xiiSharedPtr<xiiGALDeviceD3D12>& pDeviceD3D12, ID3D12Resource* pResource, xiiGALExternalMemoryDescription& out_externalMemoryDescription)
  {
    HANDLE hSharedHandle = nullptr;
    HRESULT hResult      = pDeviceD3D12->GetD3D12Device()->CreateSharedHandle(pResource, nullptr, GENERIC_ALL, nullptr, &hSharedHandle);
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

  if (m_pD3D12Texture != nullptr)
  {
    if (IsNativeObjectWrapper() || m_TextureAllocation == nullptr || m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    {
      XII_GAL_D3D12_RELEASE(m_pD3D12Texture);
    }
    else
    {
      if (m_Description.m_Usage == xiiGALResourceUsage::Staging)
      {
        pDeviceD3D12->GetD3D12Allocator()->DestroyBuffer(m_pD3D12Texture, m_TextureAllocation);
      }
      else
      {
        pDeviceD3D12->GetD3D12Allocator()->DestroyImage(m_pD3D12Texture, m_TextureAllocation);
      }
    }
  }

  m_pD3D12Texture     = nullptr;
  m_TextureAllocation = nullptr;
}

xiiResult xiiGALTextureD3D12::InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12    = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiD3D12MemoryAllocator*        pD3D12Allocator = pDeviceD3D12->GetD3D12Allocator();

  m_ExternalMemoryDescription = {};

  const bool bHasInitialData = (pInitialData != nullptr && !pInitialData->m_pSubResources.IsEmpty());

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
    m_pD3D12Texture = static_cast<ID3D12Resource*>(m_Description.m_pExistingNativeObject);
    SetResourceState(xiiGALResourceStateFlags::Undefined);
    return XII_SUCCESS;
  }

  xiiD3D12MemoryAllocationCreateInfo    allocationCreateInfo = {};
  xiiBitflags<xiiGALResourceStateFlags> desiredState         = xiiD3D12TypeConversions::GetResourceStateFromBindFlags(m_Description.m_BindFlags);
  xiiBitflags<xiiGALResourceStateFlags> creationState        = desiredState;
  xiiBitflags<xiiGALResourceStateFlags> finalState           = desiredState;

  if (desiredState == xiiGALResourceStateFlags::Undefined)
  {
    desiredState  = xiiGALResourceStateFlags::Common;
    creationState = desiredState;
    finalState    = desiredState;
  }

  if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
  {
    allocationCreateInfo.m_HeapFlags = xiiD3D12MemoryHeapFlags::Shared;
    allocationCreateInfo.m_Flags     = xiiD3D12AllocationFlags::Committed;

    if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Imported))
    {
      xiiLog::Warning("D3D12 texture creation requested imported external memory, but no texture external-memory descriptor is currently available in GraphicsFoundation. Falling back to engine-owned shared allocation.");
    }
  }

  switch (m_Description.m_Usage)
  {
    case xiiGALResourceUsage::Immutable:
    case xiiGALResourceUsage::Mutable:
    {
      allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Default;
      if (bHasInitialData)
      {
        creationState = xiiGALResourceStateFlags::CopyDestination;
      }

      const D3D12_RESOURCE_DESC resourceDescription = BuildTextureResourceDescription(m_Description);
      XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateImage(resourceDescription, allocationCreateInfo, creationState, &m_Description.m_ClearValue, &m_pD3D12Texture, &m_TextureAllocation));

      if (bHasInitialData)
      {
        XII_SUCCEED_OR_RETURN(UploadInitialTextureDataWithImmediateCommandList(pDeviceD3D12, m_pD3D12Texture, m_Description, *pInitialData, finalState));
        creationState = finalState;
      }
    }
    break;

    case xiiGALResourceUsage::Sparse:
    {
      if (externalMemoryKind.IsAnySet(xiiGALExternalMemoryKind::Imported | xiiGALExternalMemoryKind::Exportable))
      {
        xiiLog::Error("Sparse D3D12 textures currently do not support external memory import/export.");
        return XII_FAILURE;
      }

      const D3D12_RESOURCE_DESC resourceDescription = BuildTextureResourceDescription(m_Description);
      XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateReservedResource(&resourceDescription, GetTextureUploadFinalState(desiredState), nullptr, IID_PPV_ARGS(&m_pD3D12Texture)));

      SetResourceState(xiiGALResourceStateFlags::Undefined);
      FillSparseTextureProperties(pDeviceD3D12, m_pD3D12Texture, m_Description, m_SparseTextureProperties);
      return XII_SUCCESS;
    }

    case xiiGALResourceUsage::Staging:
    {
      const D3D12_RESOURCE_DESC resourceDescription = BuildStagingBufferDescription(m_Description);

      if (m_Description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Readback;
        creationState                   = xiiGALResourceStateFlags::CopyDestination;
        finalState                      = creationState;
        m_bReadbackTexture              = true;
      }
      else
      {
        allocationCreateInfo.m_HeapType = xiiD3D12MemoryHeapType::Upload;
        creationState                   = xiiGALResourceStateFlags::CopySource;
        finalState                      = creationState;
        m_bHostVisibleUpload            = true;
      }

      XII_SUCCEED_OR_RETURN(pD3D12Allocator->CreateBuffer(resourceDescription, allocationCreateInfo, creationState, &m_pD3D12Texture, &m_TextureAllocation));

      if (bHasInitialData)
      {
        if (m_bReadbackTexture)
        {
          xiiLog::Error("Read-back staging D3D12 textures cannot be initialized with CPU-side data.");
          return XII_FAILURE;
        }

        XII_SUCCEED_OR_RETURN(InitializeStagingTextureData(m_pD3D12Texture, m_Description, *pInitialData));
      }
    }
    break;

    case xiiGALResourceUsage::Dynamic:
    case xiiGALResourceUsage::Unified:
      xiiLog::Error("D3D12 texture usage '{}' is currently unsupported.", xiiArgEnum(m_Description.m_Usage));
      return XII_FAILURE;

    default:
      XII_REPORT_FAILURE("Unhandled D3D12 texture usage.");
      return XII_FAILURE;
  }

  if (externalMemoryKind.IsSet(xiiGALExternalMemoryKind::Exportable))
  {
    XII_SUCCEED_OR_RETURN(InitializeExternalMemoryDescription(pDeviceD3D12, m_pD3D12Texture, m_ExternalMemoryDescription));
  }

  SetResourceState(creationState);
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
