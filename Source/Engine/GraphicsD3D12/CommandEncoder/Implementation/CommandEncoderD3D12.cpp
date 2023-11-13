#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandEncoderD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

xiiGALCommandEncoderD3D12::xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12) :
  m_GALDeviceD3D12(deviceD3D12)
{
}

xiiGALCommandEncoderD3D12::~xiiGALCommandEncoderD3D12()
{
}

void xiiGALCommandEncoderD3D12::SetShaderPlatform(const xiiGALShader* pShader)
{
}

void xiiGALCommandEncoderD3D12::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALSampler* pSampler)
{
}

void xiiGALCommandEncoderD3D12::SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandEncoderD3D12::SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, const xiiGALTextureView* pRTextureView)
{
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, const xiiGALBufferView* pUnorderedAccessBufferView)
{
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, const xiiGALTextureView* pUnorderedAccessTextureView)
{
}

void xiiGALCommandEncoderD3D12::BeginQueryPlatform(const xiiGALQuery* pQuery)
{
}

void xiiGALCommandEncoderD3D12::EndQueryPlatform(const xiiGALQuery* pQuery)
{
}

xiiResult xiiGALCommandEncoderD3D12::GetQueryResultPlatform(const xiiGALQuery* pQuery, void* pData)
{
  return XII_FAILURE;
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(const xiiGALBufferView* pBufferView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(const xiiGALTextureView* pTextureView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(const xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(const xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource)
{
}

void xiiGALCommandEncoderD3D12::CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
}

void xiiGALCommandEncoderD3D12::UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
}

void xiiGALCommandEncoderD3D12::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
}

void xiiGALCommandEncoderD3D12::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiVec3U32& vDestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubResourceData& sourceSubResource, const xiiBoundingBoxu32& box)
{
}

void xiiGALCommandEncoderD3D12::UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureData& sourceData)
{
}

void xiiGALCommandEncoderD3D12::ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubResourceData& destinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubResourceData& sourceSubResource)
{
}

void xiiGALCommandEncoderD3D12::ReadbackTexturePlatform(const xiiGALTexture* pTexture, const xiiGALTexture* pStagingTexture)
{
}

void xiiGALCommandEncoderD3D12::CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, const xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureSubResourceData> sourceSubResource, xiiArrayPtr<xiiGALTextureData> targetData)
{
}

void xiiGALCommandEncoderD3D12::GenerateMipMapsPlatform(const xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandEncoderD3D12::FlushPlatform()
{
}

void xiiGALCommandEncoderD3D12::PushMarkerPlatform(xiiStringView sMarker)
{
}

void xiiGALCommandEncoderD3D12::PopMarkerPlatform()
{
}

void xiiGALCommandEncoderD3D12::InsertEventMarkerPlatform(xiiStringView sMarker)
{
}

void xiiGALCommandEncoderD3D12::ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

void xiiGALCommandEncoderD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
}

void xiiGALCommandEncoderD3D12::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

void xiiGALCommandEncoderD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
}

void xiiGALCommandEncoderD3D12::DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

void xiiGALCommandEncoderD3D12::BeginStreamOutPlatform()
{
}

void xiiGALCommandEncoderD3D12::EndStreamOutPlatform()
{
}

void xiiGALCommandEncoderD3D12::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetInputLayoutPlatform(const xiiGALInputLayout* pInputLayout)
{
}

void xiiGALCommandEncoderD3D12::SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology)
{
}

void xiiGALCommandEncoderD3D12::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask)
{
}

void xiiGALCommandEncoderD3D12::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
}

void xiiGALCommandEncoderD3D12::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
}

void xiiGALCommandEncoderD3D12::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
}

void xiiGALCommandEncoderD3D12::SetScissorRectPlatform(const xiiRectU32& rect)
{
}

void xiiGALCommandEncoderD3D12::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
}

void xiiGALCommandEncoderD3D12::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
}

void xiiGALCommandEncoderD3D12::DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandEncoderD3D12);
