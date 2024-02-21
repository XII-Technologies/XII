#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

xiiGALCommandListD3D12::xiiGALCommandListD3D12(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListD3D12::~xiiGALCommandListD3D12() = default;

xiiResult xiiGALCommandListD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  XII_ASSERT_NOT_IMPLEMENTED;

  return m_pCommandList == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pCommandList);

  return XII_SUCCESS;
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

void xiiGALCommandListD3D12::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
}

void xiiGALCommandListD3D12::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
}

void xiiGALCommandListD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
}

void xiiGALCommandListD3D12::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
}

void xiiGALCommandListD3D12::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
}

void xiiGALCommandListD3D12::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

xiiResult xiiGALCommandListD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListD3D12::EndQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListD3D12::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
}

void xiiGALCommandListD3D12::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
}

void xiiGALCommandListD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
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
}

void xiiGALCommandListD3D12::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
}

void xiiGALCommandListD3D12::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
}

void xiiGALCommandListD3D12::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
}

xiiResult xiiGALCommandListD3D12::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData)
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

void xiiGALCommandListD3D12::FlushPlatform()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandListD3D12);
