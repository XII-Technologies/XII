#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandList, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandList::xiiGALCommandList() :
  xiiGALDeviceObject()
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

void xiiGALCommandList::SetPipelineState(xiiGALPipelineStateHandle hPipelineState)
{
}

void xiiGALCommandList::SetStencilRef(xiiUInt8 uiStencilRef)
{
}

void xiiGALCommandList::SetBlendFactor(const xiiColor& blendFactor)
{
}

void xiiGALCommandList::SetViewports(xiiArrayPtr<xiiRectFloat> pViewports, float fMinDepth, float fMaxDepth)
{
}

void xiiGALCommandList::SetScissorRects(xiiArrayPtr<xiiRectU32> pRects)
{
}

void xiiGALCommandList::SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt32 uiByteOffset)
{
}

void xiiGALCommandList::SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt32> pByteOffsets)
{
}

void xiiGALCommandList::ClearRenderTargetView(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor)
{
}

void xiiGALCommandList::ClearDepthStencilView(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

xiiResult xiiGALCommandList::Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  return XII_FAILURE;
}

void xiiGALCommandList::BeginQuery(xiiGALQueryHandle hQuery)
{
}

void xiiGALCommandList::EndQuery(xiiGALQueryHandle hQuery)
{
}

void xiiGALCommandList::BeginDebugGroup(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandList::EndDebugGroup()
{
}

void xiiGALCommandList::InsertDebugLabel(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandList::UpdateBuffer(xiiGALBufferHandle hBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
}

void xiiGALCommandList::CopyBuffer(xiiGALBufferHandle hSourceBuffer, xiiGALBufferHandle hDestinationBuffer)
{
}

void xiiGALCommandList::CopyBufferRegion(xiiGALBufferHandle hSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBufferHandle hDestinationBuffer, xiiUInt64 uiDestinationOffset)
{
}

void xiiGALCommandList::MapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
}

void xiiGALCommandList::UnmapBuffer(xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType)
{
}

void xiiGALCommandList::UpdateTexture(xiiGALTextureHandle hTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandList::CopyTexture(xiiGALTextureHandle hSourceTexture, xiiGALTextureHandle hDestinationTexture)
{
}

void xiiGALCommandList::CopyTextureRegion(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
}

void xiiGALCommandList::ResolveTextureSubResource(xiiGALTextureHandle hSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTextureHandle hDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
}

void xiiGALCommandList::GenerateMips(xiiGALTextureViewHandle hTextureView)
{
}

void xiiGALCommandList::InvalidateState()
{
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);
