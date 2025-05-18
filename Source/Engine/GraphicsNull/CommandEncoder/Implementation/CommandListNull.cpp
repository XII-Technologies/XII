#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandListNull.h>
#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/FenceNull.h>
#include <GraphicsNull/Resources/QueryNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandListNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandListNull::xiiGALCommandListNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, xiiGALCommandQueueNull* pCommandQueue, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceNull, pCommandQueue, creationDescription)
{
}

xiiGALCommandListNull::~xiiGALCommandListNull() = default;

void xiiGALCommandListNull::BeginPlatform()
{
}

void xiiGALCommandListNull::EndPlatform()
{
}

void xiiGALCommandListNull::ResetPlatform()
{
}

xiiUInt64 xiiGALCommandListNull::SubmitPlatform()
{
  return static_cast<xiiGALCommandQueueNull*>(m_pCommandQueue)->Submit(this);
}

void xiiGALCommandListNull::SetPipelineStatePlatform(xiiSharedPtr<xiiGALPipelineState> pPipelineState)
{
  XII_IGNORE_UNUSED(pPipelineState);
}

void xiiGALCommandListNull::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  XII_IGNORE_UNUSED(uiStencilRef);
}

void xiiGALCommandListNull::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  XII_IGNORE_UNUSED(blendFactor);
}

void xiiGALCommandListNull::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
  XII_IGNORE_UNUSED(pViewports);
}

void xiiGALCommandListNull::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
  XII_IGNORE_UNUSED(pRects);
}

void xiiGALCommandListNull::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pConstantBuffer);
}

void xiiGALCommandListNull::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pBufferView);
}

void xiiGALCommandListNull::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pTextureView);
}

void xiiGALCommandListNull::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pBufferView);
}

void xiiGALCommandListNull::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pTextureView);
}

void xiiGALCommandListNull::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler)
{
  XII_IGNORE_UNUSED(bindingInformation);
  XII_IGNORE_UNUSED(pSampler);
}

xiiResult xiiGALCommandListNull::CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)
{
  XII_IGNORE_UNUSED(mode);
  return XII_SUCCESS;
}

void xiiGALCommandListNull::SetIndexBufferPlatform(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset)
{
  XII_IGNORE_UNUSED(pIndexBuffer);
  XII_IGNORE_UNUSED(uiByteOffset);
}

void xiiGALCommandListNull::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  XII_IGNORE_UNUSED(uiStartSlot);
  XII_IGNORE_UNUSED(pVertexBuffers);
  XII_IGNORE_UNUSED(pByteOffsets);
  XII_IGNORE_UNUSED(flags);
}

void xiiGALCommandListNull::ClearRenderTargetViewPlatform(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor)
{
  XII_IGNORE_UNUSED(pRenderTargetView);
  XII_IGNORE_UNUSED(clearColor);
}

void xiiGALCommandListNull::ClearDepthStencilViewPlatform(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  XII_IGNORE_UNUSED(pDepthStencilView);
  XII_IGNORE_UNUSED(bClearDepth);
  XII_IGNORE_UNUSED(bClearStencil);
  XII_IGNORE_UNUSED(fDepthClear);
  XII_IGNORE_UNUSED(uiStencilClear);
}

void xiiGALCommandListNull::BeginRenderPassPlatform(xiiSharedPtr<xiiGALRenderPass> pRenderPass, xiiSharedPtr<xiiGALFramebuffer> pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
  XII_IGNORE_UNUSED(pRenderPass);
  XII_IGNORE_UNUSED(pFramebuffer);
  XII_IGNORE_UNUSED(pOptimizedClearValues);
}

void xiiGALCommandListNull::NextSubpassPlatform()
{
}

void xiiGALCommandListNull::EndRenderPassPlatform()
{
}

xiiResult xiiGALCommandListNull::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  XII_IGNORE_UNUSED(uiVertexCount);
  XII_IGNORE_UNUSED(uiStartVertex);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  XII_IGNORE_UNUSED(uiIndexCount);
  XII_IGNORE_UNUSED(uiStartIndex);
  XII_IGNORE_UNUSED(uiBaseVertex);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  XII_IGNORE_UNUSED(uiIndexCountPerInstance);
  XII_IGNORE_UNUSED(uiInstanceCount);
  XII_IGNORE_UNUSED(uiStartIndex);
  XII_IGNORE_UNUSED(uiBaseVertex);
  XII_IGNORE_UNUSED(uiFirstInstance);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawIndexedInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_IGNORE_UNUSED(pIndirectArgumentBuffer);
  XII_IGNORE_UNUSED(uiArgumentOffsetInBytes);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  XII_IGNORE_UNUSED(uiVertexCountPerInstance);
  XII_IGNORE_UNUSED(uiInstanceCount);
  XII_IGNORE_UNUSED(uiStartVertex);
  XII_IGNORE_UNUSED(uiFirstInstance);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_IGNORE_UNUSED(pIndirectArgumentBuffer);
  XII_IGNORE_UNUSED(uiArgumentOffsetInBytes);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  XII_IGNORE_UNUSED(uiThreadGroupCountX);
  XII_IGNORE_UNUSED(uiThreadGroupCountY);
  XII_IGNORE_UNUSED(uiThreadGroupCountZ);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  XII_IGNORE_UNUSED(uiThreadGroupCountX);
  XII_IGNORE_UNUSED(uiThreadGroupCountY);
  XII_IGNORE_UNUSED(uiThreadGroupCountZ);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DispatchIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_IGNORE_UNUSED(pIndirectArgumentBuffer);
  XII_IGNORE_UNUSED(uiArgumentOffsetInBytes);
  return XII_SUCCESS;
}

void xiiGALCommandListNull::BeginQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery)
{
  XII_IGNORE_UNUSED(pQuery);
}

void xiiGALCommandListNull::EndQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery)
{
  XII_IGNORE_UNUSED(pQuery);
}

void xiiGALCommandListNull::UpdateBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
  XII_IGNORE_UNUSED(pBuffer);
  XII_IGNORE_UNUSED(uiDestinationOffset);
  XII_IGNORE_UNUSED(pSourceData);
}

void xiiGALCommandListNull::CopyBufferPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer)
{
  XII_IGNORE_UNUSED(pSourceBuffer);
  XII_IGNORE_UNUSED(pDestinationBuffer);
}

void xiiGALCommandListNull::CopyBufferRegionPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  XII_IGNORE_UNUSED(pSourceBuffer);
  XII_IGNORE_UNUSED(uiSourceOffset);
  XII_IGNORE_UNUSED(pDestinationBuffer);
  XII_IGNORE_UNUSED(uiDestinationOffset);
  XII_IGNORE_UNUSED(uiSize);
}

xiiResult xiiGALCommandListNull::MapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  XII_IGNORE_UNUSED(pBuffer);
  XII_IGNORE_UNUSED(mapType);
  XII_IGNORE_UNUSED(mapFlags);
  XII_IGNORE_UNUSED(pMappedData);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::UnmapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  XII_IGNORE_UNUSED(pBuffer);
  XII_IGNORE_UNUSED(mapType);
  return XII_SUCCESS;
}

void xiiGALCommandListNull::UpdateTexturePlatform(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  XII_IGNORE_UNUSED(pTexture);
  XII_IGNORE_UNUSED(textureMiplevelData);
  XII_IGNORE_UNUSED(textureBox);
  XII_IGNORE_UNUSED(subresourceData);
}

void xiiGALCommandListNull::CopyTexturePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture)
{
  XII_IGNORE_UNUSED(pSourceTexture);
  XII_IGNORE_UNUSED(pDestinationTexture);
}

void xiiGALCommandListNull::CopyTextureRegionPlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  XII_IGNORE_UNUSED(pSourceTexture);
  XII_IGNORE_UNUSED(sourceMipLevelData);
  XII_IGNORE_UNUSED(box);
  XII_IGNORE_UNUSED(pDestinationTexture);
  XII_IGNORE_UNUSED(destinationMipLevelData);
  XII_IGNORE_UNUSED(vDestinationPoint);
}

void xiiGALCommandListNull::ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  XII_IGNORE_UNUSED(pSourceTexture);
  XII_IGNORE_UNUSED(sourceMipLevelData);
  XII_IGNORE_UNUSED(pDestinationTexture);
  XII_IGNORE_UNUSED(destinationMipLevelData);
}

void xiiGALCommandListNull::GenerateMipsPlatform(xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_IGNORE_UNUSED(pTextureView);
}

xiiResult xiiGALCommandListNull::MapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  XII_IGNORE_UNUSED(pTexture);
  XII_IGNORE_UNUSED(textureMipLevelData);
  XII_IGNORE_UNUSED(mapType);
  XII_IGNORE_UNUSED(mapFlags);
  XII_IGNORE_UNUSED(pTextureBox);
  XII_IGNORE_UNUSED(mappedData);
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::UnmapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  XII_IGNORE_UNUSED(pTexture);
  XII_IGNORE_UNUSED(textureMipLevelData);
  return XII_SUCCESS;
}

void xiiGALCommandListNull::TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  XII_IGNORE_UNUSED(pResourceBarriers);
}

void xiiGALCommandListNull::EnqueueSignalPlatform(xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue)
{
  XII_IGNORE_UNUSED(pFence);
  XII_IGNORE_UNUSED(uiValue);
}

void xiiGALCommandListNull::DeviceWaitForFencePlatform(xiiSharedPtr<xiiGALFence> pFence, xiiUInt64 uiValue)
{
  XII_IGNORE_UNUSED(pFence);
  XII_IGNORE_UNUSED(uiValue);
}

void xiiGALCommandListNull::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  XII_IGNORE_UNUSED(sName);
  XII_IGNORE_UNUSED(color);
}

void xiiGALCommandListNull::EndDebugGroupPlatform()
{
}

void xiiGALCommandListNull::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  XII_IGNORE_UNUSED(sName);
  XII_IGNORE_UNUSED(color);
}

void xiiGALCommandListNull::InvalidateStatePlatform()
{
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandListNull);
