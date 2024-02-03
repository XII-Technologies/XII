#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/States/PipelineState.h>

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

#define XII_VERIFY_DISPATCH(expression, ...)   \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return XII_FAILURE; } \
  } while (false)

xiiResult xiiGALCommandList::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  CountDispatchCall();

  XII_VERIFY_DISPATCH(!m_hPipelineState.IsInvalidated(), "Dispatch command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DISPATCH(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "Dispatch command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DISPATCH(m_hRenderPass.IsInvalidated(), "Dispatch command arguments are invalid. Dispatch command must be performed outside of render pass.");
  XII_VERIFY_DISPATCH(uiThreadGroupCountX > 0U && uiThreadGroupCountY > 0U && uiThreadGroupCountZ > 0U, "Dispatch command arguments are invalid. At least one of the thread group counts are zero, this is OK as the dispatch command will be ignored, but may be unintentional.");

  return DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

xiiResult xiiGALCommandList::DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDispatchCall();

  XII_VERIFY_DISPATCH(!m_hPipelineState.IsInvalidated(), "Dispatch command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DISPATCH(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "Dispatch command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DISPATCH(m_hRenderPass.IsInvalidated(), "Dispatch command arguments are invalid. Dispatch command must be performed outside of render pass.");

#if 0
#  define CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(Expr, ...) CHECK_PARAMETER(Expr, "Dispatch compute indirect attribs are invalid: ", __VA_ARGS__)

  CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(pAttribsBuffer != nullptr, "indirect dispatch arguments buffer must not be null.");
  const auto& AttrBuffDesc = pAttribsBuffer->GetDesc();
  CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS((AttrBuffDesc.BindFlags & BIND_INDIRECT_DRAW_ARGS) != 0, "indirect dispatch arguments buffer '",
                                          AttrBuffDesc.Name, "' was not created with BIND_INDIRECT_DRAW_ARGS flag.");
  const auto ReqAttrBufSize = sizeof(Uint32) * 3 + Attribs.DispatchArgsByteOffset;
  CHECK_DISPATCH_COMPUTE_INDIRECT_ATTRIBS(ReqAttrBufSize <= AttrBuffDesc.Size, "invalid DispatchArgsByteOffset (", Attribs.DispatchArgsByteOffset,
                                          ") or indirect dispatch arguments buffer '", AttrBuffDesc.Name, "' size must be at least ", ReqAttrBufSize, " bytes");

#endif

  xiiGALBuffer* pIndirectArgumentBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);

  return DispatchIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

#undef XII_VERIFY_DISPATCH

void xiiGALCommandList::BeginQuery(xiiGALQueryHandle hQuery)
{
}

void xiiGALCommandList::EndQuery(xiiGALQueryHandle hQuery)
{
}

void xiiGALCommandList::BeginDebugGroup(xiiStringView sName, const xiiColor& color)
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "The debug group name must not be empty.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  ++m_uiDebugGroupCount;
#endif

  BeginDebugGroupPlatform(sName, color);
}

void xiiGALCommandList::EndDebugGroup()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_uiDebugGroupCount > 0, "There are no active debug groups to end.");

  if (m_uiDebugGroupCount > 0)
  {
    --m_uiDebugGroupCount;
  }
#endif

  EndDebugGroupPlatform();
}

void xiiGALCommandList::InsertDebugLabel(xiiStringView sName, const xiiColor& color)
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "The debug label name must not be empty.");

  InsertDebugLabelPlatform(sName, color);
}

void xiiGALCommandList::Flush()
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
