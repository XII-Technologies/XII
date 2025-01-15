#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

xiiGALCommandListD3D12::xiiGALCommandListD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALCommandQueueD3D12* pCommandQueueD3D12, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceD3D12, pCommandQueueD3D12, creationDescription), m_pD3D12CommandQueue(nullptr), m_pD3D12CommandList(nullptr), m_uiCurrentAllocatorIndex(xiiInvalidIndex)
{
  XII_ASSERT_DEV(m_D3D12CommandAllocators.IsEmpty(), "");
  XII_ASSERT_DEV(m_D3D12CommandAllocatorFenceData.IsEmpty(), "");
  XII_ASSERT_DEV(m_uiCurrentAllocatorIndex == xiiInvalidIndex, "");

  // We will use specialized command list types for compute/copy queues.
  D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
  if (m_Description.m_QueueType == xiiGALCommandQueueType::Compute)
  {
    commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
  }
  else if (m_Description.m_QueueType == xiiGALCommandQueueType::Transfer)
  {
    commandListType = D3D12_COMMAND_LIST_TYPE_COPY;
  }

  ID3D12CommandAllocator* pD3D12CommandAllocator = nullptr;
  XII_VERIFY(SUCCEEDED(pDeviceD3D12->GetD3D12Device()->CreateCommandAllocator(commandListType, _uuidof(ID3D12CommandAllocator), (void**)&pD3D12CommandAllocator)), "Failed to create the ID3D12CommandAllocator for command list!");

  m_D3D12CommandAllocators.PushBack(pD3D12CommandAllocator);
  m_uiCurrentAllocatorIndex = 0U;

  XII_VERIFY(SUCCEEDED(pDeviceD3D12->GetD3D12Device()->CreateCommandList(0U, commandListType, pD3D12CommandAllocator, nullptr, _uuidof(ID3D12GraphicsCommandList), (void**)&m_pD3D12CommandList)), "Failed to create command list from the ID3D12GraphicsCommandList interface.");
}

xiiGALCommandListD3D12::~xiiGALCommandListD3D12()
{
}

void xiiGALCommandListD3D12::BeginPlatform()
{
}

void xiiGALCommandListD3D12::EndPlatform()
{
}

void xiiGALCommandListD3D12::ResetPlatform()
{
}

xiiUInt64 xiiGALCommandListD3D12::SubmitPlatform()
{
  return xiiUInt64();
}

void xiiGALCommandListD3D12::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
}

void xiiGALCommandListD3D12::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
}

void xiiGALCommandListD3D12::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
}

void xiiGALCommandListD3D12::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
}

void xiiGALCommandListD3D12::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
}

void xiiGALCommandListD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  m_pD3D12CommandList->IASetIndexBuffer(nullptr);
}

void xiiGALCommandListD3D12::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  m_pD3D12CommandList->IASetVertexBuffers(0U, 0U, nullptr);
}

void xiiGALCommandListD3D12::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
}

void xiiGALCommandListD3D12::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandListD3D12::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandListD3D12::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandListD3D12::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandListD3D12::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
}

void xiiGALCommandListD3D12::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  m_pD3D12CommandList->ClearRenderTargetView({}, clearColor.GetData(), 0, nullptr);
}

void xiiGALCommandListD3D12::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  m_pD3D12CommandList->ClearDepthStencilView({}, {}, fDepthClear, uiStencilClear, 0U, nullptr);
}

void xiiGALCommandListD3D12::BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
}

void xiiGALCommandListD3D12::NextSubpassPlatform()
{
}

void xiiGALCommandListD3D12::EndRenderPassPlatform()
{
}

xiiResult xiiGALCommandListD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  m_pD3D12CommandList->DrawInstanced(uiVertexCount, 1U, uiStartVertex, 0U);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  m_pD3D12CommandList->DrawIndexedInstanced(uiIndexCount, 1U, uiStartIndex, uiBaseVertex, 0U);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  m_pD3D12CommandList->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  // m_pD3D12CommandList->ExecuteIndirect(nullptr, 0U, nullptr, 0ULL, nullptr, 0ULL);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  m_pD3D12CommandList->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  // m_pD3D12CommandList->ExecuteIndirect(nullptr, 0U, nullptr, 0ULL, nullptr, 0ULL);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  m_pD3D12CommandList->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  m_pD3D12CommandList->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  // m_pD3D12CommandList->ExecuteIndirect(nullptr, 0U, nullptr, 0ULL, nullptr, 0ULL);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListD3D12::EndQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListD3D12::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
}

void xiiGALCommandListD3D12::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  const xiiUInt64 uiSize = pSourceBuffer->GetDescription().m_uiSize;

  m_pD3D12CommandList->CopyBufferRegion(nullptr, 0U, nullptr, 0U, uiSize);
}

void xiiGALCommandListD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  m_pD3D12CommandList->CopyBufferRegion(nullptr, uiDestinationOffset, nullptr, uiSourceOffset, uiSize);
}

xiiResult xiiGALCommandListD3D12::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListD3D12::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  D3D12_BOX sourceBox = {
    .left   = 0U,
    .top    = 0U,
    .front  = 0U,
    .right  = 0U,
    .bottom = 0U,
    .back   = 0U,
  };

  D3D12_TEXTURE_COPY_LOCATION sourceTextureCopyLocation = {
    .pResource        = nullptr,
    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(0U, 0U, pSourceTexture->GetDescription().m_uiMipLevels),
  };

  D3D12_TEXTURE_COPY_LOCATION destinationTextureCopyLocation = {
    .pResource        = nullptr,
    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(0U, 0U, pDestinationTexture->GetDescription().m_uiMipLevels),
  };

  m_pD3D12CommandList->CopyTextureRegion(&destinationTextureCopyLocation, 0U, 0U, 0U, &sourceTextureCopyLocation, &sourceBox);
}

void xiiGALCommandListD3D12::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  D3D12_BOX sourceBox = {
    .left   = box.m_vMin.x,
    .top    = box.m_vMin.y,
    .front  = box.m_vMin.z,
    .right  = box.m_vMax.x,
    .bottom = box.m_vMax.y,
    .back   = box.m_vMax.z,
  };

  D3D12_TEXTURE_COPY_LOCATION sourceTextureCopyLocation = {
    .pResource        = nullptr,
    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pSourceTexture->GetDescription().m_uiMipLevels),
  };

  D3D12_TEXTURE_COPY_LOCATION destinationTextureCopyLocation = {
    .pResource        = nullptr,
    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, pDestinationTexture->GetDescription().m_uiMipLevels),
  };

  m_pD3D12CommandList->CopyTextureRegion(&destinationTextureCopyLocation, 0U, 0U, 0U, &sourceTextureCopyLocation, &sourceBox);
}

void xiiGALCommandListD3D12::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  const xiiUInt32   uiSourceSubResourceIndex      = xiiD3D12TypeConversions::CalculateSubResourceIndex(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pSourceTexture->GetDescription().m_uiMipLevels);
  const xiiUInt32   uiDestinationSubResourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, pDestinationTexture->GetDescription().m_uiMipLevels);
  const DXGI_FORMAT dxgiFormat                    = xiiD3D12TypeConversions::GetFormat(pDestinationTexture->GetDescription().m_Format);

  m_pD3D12CommandList->ResolveSubresource(nullptr, uiDestinationSubResourceIndex, nullptr, uiSourceSubResourceIndex, dxgiFormat);
}

void xiiGALCommandListD3D12::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
}

xiiResult xiiGALCommandListD3D12::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandListD3D12::EndDebugGroupPlatform()
{
}

void xiiGALCommandListD3D12::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandListD3D12::InvalidateStatePlatform()
{
}

void xiiGALCommandListD3D12::SetDebugNamePlatform(xiiStringView sName)
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandListD3D12);
