#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/PipelineStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

xiiGALCommandListD3D12::xiiGALCommandListD3D12(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListD3D12::~xiiGALCommandListD3D12() = default;

xiiResult xiiGALCommandListD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics))
  {
    m_pCommandList = pDeviceD3D12->GetImmediateContext();

    xiiLog::Error("Failed to create command list, no graphics command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Compute))
  {
    m_pCommandList = pDeviceD3D12->GetComputeContext();

    xiiLog::Error("Failed to create command list, no compute command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer))
  {
    m_pCommandList = pDeviceD3D12->GetTransferContext();

    xiiLog::Error("Failed to create command list, no transfer command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::SparseBinding))
  {
    m_pCommandList = pDeviceD3D12->GetSparseBindingContext();

    xiiLog::Error("Failed to create command list, no sparse binding command queue found.");
  }
  return m_pCommandList != nullptr ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pCommandList);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::ExecutePlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandListD3D12::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  auto pPipelineStateD3D12 = static_cast<xiiGALPipelineStateD3D12*>(pPipelineState);

  if (m_pPipelineState == (pPipelineStateD3D12 != nullptr ? pPipelineStateD3D12->GetPipelineState() : nullptr))
    return;

  m_pPipelineState = (pPipelineStateD3D12 != nullptr ? pPipelineStateD3D12->GetPipelineState() : nullptr);
}

void xiiGALCommandListD3D12::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  m_pCommandList->SetStencilRef(uiStencilRef);
}

void xiiGALCommandListD3D12::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  m_pCommandList->SetBlendFactors(blendFactor.GetData());
}

void xiiGALCommandListD3D12::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  xiiHybridArray<Diligent::Viewport, XII_GAL_MAX_VIEWPORT_COUNT> viewports;

  for (xiiUInt32 i = 0; i < pViewports.GetCount(); ++i)
  {
    const auto& sourceView = pViewports[i];
    auto&       view       = viewports.ExpandAndGetRef();

    view.TopLeftX = sourceView.m_fTopLeftX;
    view.TopLeftY = sourceView.m_fTopLeftY;
    view.Width    = sourceView.m_fWidth;
    view.Height   = sourceView.m_fHeight;
    view.MinDepth = sourceView.m_fMinDepth;
    view.MaxDepth = sourceView.m_fMaxDepth;
  }
  m_pCommandList->SetViewports(viewports.GetCount(), viewports.GetData(), uiRenderTargetWidth, uiRenderTargetHeight);
}

void xiiGALCommandListD3D12::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  xiiHybridArray<Diligent::Rect, XII_GAL_MAX_VIEWPORT_COUNT> rects;

  for (xiiUInt32 i = 0; i < pRects.GetCount(); ++i)
  {
    const auto& sourceRect = pRects[i];
    auto&       rect       = rects.ExpandAndGetRef();

    rect.top    = sourceRect.Top();
    rect.bottom = sourceRect.Bottom();
    rect.left   = sourceRect.Left();
    rect.right  = sourceRect.Right();
  }
  m_pCommandList->SetScissorRects(rects.GetCount(), rects.GetData(), uiRenderTargetWidth, uiRenderTargetHeight);
}

void xiiGALCommandListD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto pIndexBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pIndexBuffer);

  m_pBoundIndexBuffer = (pIndexBufferD3D12 != nullptr) ? pIndexBufferD3D12->GetBuffer() : nullptr;
  m_IndexFormat       = (pIndexBufferD3D12 != nullptr) ? pIndexBufferD3D12->GetIndexFormat() : Diligent::VT_UNDEFINED;

  m_pCommandList->SetIndexBuffer(m_pBoundIndexBuffer, uiByteOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D12::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    xiiMemoryUtils::DefaultConstruct(m_pBoundVertexBuffers);

    m_BoundVertexBuffersRange.Reset();
  }
  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    auto* pVertexBuffersD3D12 = static_cast<xiiGALBufferD3D12*>(pVertexBuffers[i]);

    m_pBoundVertexBuffers[i] = (pVertexBuffersD3D12 != nullptr) ? pVertexBuffersD3D12->GetBuffer() : nullptr;

    m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }

  Diligent::SET_VERTEX_BUFFERS_FLAGS setVertexBufferFlags = Diligent::SET_VERTEX_BUFFERS_FLAG_NONE;
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
    setVertexBufferFlags |= Diligent::SET_VERTEX_BUFFERS_FLAG_RESET;

  m_pCommandList->SetVertexBuffers(uiStartSlot, pVertexBuffers.GetCount(), m_pBoundVertexBuffers + uiStartSlot, pByteOffsets.GetPtr() + uiStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, setVertexBufferFlags);
}

void xiiGALCommandListD3D12::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  auto pRenderTargetViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pRenderTargetView);

  m_pCommandList->ClearRenderTarget(pRenderTargetViewD3D12->GetTextureView(), clearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D12::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  auto pDepthStencilViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pDepthStencilView);

  Diligent::CLEAR_DEPTH_STENCIL_FLAGS clearFlags = Diligent::CLEAR_DEPTH_FLAG_NONE;
  if (bClearDepth)
    clearFlags |= Diligent::CLEAR_DEPTH_FLAG;
  if (bClearStencil)
    clearFlags |= Diligent::CLEAR_STENCIL_FLAG;

  m_pCommandList->ClearDepthStencil(pDepthStencilViewD3D12->GetTextureView(), clearFlags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

xiiResult xiiGALCommandListD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  Diligent::DrawAttribs drawDescription = {};
  drawDescription.NumVertices           = uiVertexCount;
  drawDescription.StartVertexLocation   = uiStartVertex;
  drawDescription.NumInstances          = 1U;
  drawDescription.FirstInstanceLocation = 0U;
  drawDescription.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pCommandList->Draw(drawDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  Diligent::DrawIndexedAttribs drawIndexedDescription = {};
  ;
  drawIndexedDescription.NumIndices            = uiIndexCount;
  drawIndexedDescription.FirstIndexLocation    = uiStartIndex;
  drawIndexedDescription.IndexType             = m_IndexFormat;
  drawIndexedDescription.BaseVertex            = 0U;
  drawIndexedDescription.NumInstances          = 1U;
  drawIndexedDescription.FirstInstanceLocation = 0U;
  drawIndexedDescription.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pCommandList->DrawIndexed(drawIndexedDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  Diligent::DrawIndexedAttribs drawIndexedDescription;
  drawIndexedDescription.NumIndices            = uiIndexCountPerInstance;
  drawIndexedDescription.NumInstances          = uiInstanceCount;
  drawIndexedDescription.IndexType             = m_IndexFormat;
  drawIndexedDescription.FirstIndexLocation    = uiStartIndex;
  drawIndexedDescription.BaseVertex            = 0U;
  drawIndexedDescription.FirstInstanceLocation = 0U;
  drawIndexedDescription.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pCommandList->DrawIndexed(drawIndexedDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer);

  Diligent::DrawIndexedIndirectAttribs drawIndexedInstancedDescription = {};
  drawIndexedInstancedDescription.IndexType                            = m_IndexFormat;
  drawIndexedInstancedDescription.pAttribsBuffer                       = pIndirectArgumentBufferD3D12->GetBuffer();
  drawIndexedInstancedDescription.DrawArgsOffset                       = uiArgumentOffsetInBytes;
  drawIndexedInstancedDescription.Flags                                = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawIndexedInstancedDescription.DrawCount                            = 1U;
  drawIndexedInstancedDescription.DrawArgsStride                       = sizeof(xiiUInt32) * 5U;
  drawIndexedInstancedDescription.AttribsBufferStateTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawIndexedInstancedDescription.pCounterBuffer                       = nullptr;
  drawIndexedInstancedDescription.CounterOffset                        = 0U;
  drawIndexedInstancedDescription.CounterBufferStateTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pCommandList->DrawIndexedIndirect(drawIndexedInstancedDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  Diligent::DrawAttribs drawInstancedDescription = {};
  drawInstancedDescription.NumVertices           = uiVertexCountPerInstance;
  drawInstancedDescription.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawInstancedDescription.NumInstances          = uiInstanceCount;
  drawInstancedDescription.FirstInstanceLocation = 0U;
  drawInstancedDescription.StartVertexLocation   = uiStartVertex;

  m_pCommandList->Draw(drawInstancedDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawInstancedIndirectDescription    = {};
  drawInstancedIndirectDescription.pAttribsBuffer                   = pIndirectArgumentBufferD3D12->GetBuffer();
  drawInstancedIndirectDescription.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawInstancedIndirectDescription.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawInstancedIndirectDescription.DrawCount                        = 1U;
  drawInstancedIndirectDescription.DrawArgsStride                   = sizeof(xiiUInt32) * 4U;
  drawInstancedIndirectDescription.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawInstancedIndirectDescription.pCounterBuffer                   = nullptr;
  drawInstancedIndirectDescription.CounterOffset                    = 0U;
  drawInstancedIndirectDescription.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pCommandList->DrawIndirect(drawInstancedIndirectDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DrawMeshAttribs drawMeshDescription = {};
  drawMeshDescription.ThreadGroupCountX         = uiThreadGroupCountX;
  drawMeshDescription.ThreadGroupCountY         = uiThreadGroupCountY;
  drawMeshDescription.ThreadGroupCountZ         = uiThreadGroupCountZ;

  m_pCommandList->DrawMesh(drawMeshDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DispatchComputeAttribs dispatchDescription = {};
  dispatchDescription.ThreadGroupCountX                = uiThreadGroupCountX;
  dispatchDescription.ThreadGroupCountY                = uiThreadGroupCountY;
  dispatchDescription.ThreadGroupCountZ                = uiThreadGroupCountZ;

  m_pCommandList->DispatchCompute(dispatchDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer);

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs = {};
  DispatchAttribs.pAttribsBuffer                           = pIndirectArgumentBufferD3D12->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode         = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  DispatchAttribs.DispatchArgsByteOffset                   = uiArgumentOffsetInBytes;

  m_pCommandList->DispatchComputeIndirect(DispatchAttribs);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  m_pCommandList->BeginQuery(pQueryD3D12->GetQuery());
}

void xiiGALCommandListD3D12::EndQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  m_pCommandList->EndQuery(pQueryD3D12->GetQuery());
}

void xiiGALCommandListD3D12::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  m_pCommandList->UpdateBuffer(pDestinationBufferD3D12->GetBuffer(), uiDestinationOffset, pSourceData.GetCount(), pSourceData.GetPtr(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D12::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSourceBuffer);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestinationBuffer);

  m_pCommandList->CopyBuffer(pSourceBufferD3D12->GetBuffer(), 0U, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferD3D12->GetBuffer(), 0U, pDestinationBuffer->GetDescription().m_uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSourceBuffer);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestinationBuffer);

  m_pCommandList->CopyBuffer(pSourceBufferD3D12->GetBuffer(), uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferD3D12->GetBuffer(), uiDestinationOffset, uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

xiiResult xiiGALCommandListD3D12::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  auto pBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  Diligent::MAP_TYPE  bufferMapType  = xiiDiligentTypeConversions::GetMapType(mapType);
  Diligent::MAP_FLAGS bufferMapFlags = xiiDiligentTypeConversions::GetMapFlags(mapFlags);

  m_pCommandList->MapBuffer(pBufferD3D12->GetBuffer(), bufferMapType, bufferMapFlags, pMappedData);

  return (pMappedData != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListD3D12::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  auto pBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  Diligent::MAP_TYPE bufferMapType = xiiDiligentTypeConversions::GetMapType(mapType);

  m_pCommandList->UnmapBuffer(pBufferD3D12->GetBuffer(), bufferMapType);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  auto        pTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pTexture);
  const auto& textureDescription = pTextureD3D12->GetDescription();

  xiiUInt32 uiWidth  = xiiMath::Max(textureBox.m_vMax.x - textureBox.m_vMin.x, 1U);
  xiiUInt32 uiHeight = xiiMath::Max(textureBox.m_vMax.y - textureBox.m_vMin.y, 1U);
  xiiUInt32 uiDepth  = xiiMath::Max(textureBox.m_vMax.z - textureBox.m_vMin.z, 1U);

  Diligent::Box destinationBox = {};
  destinationBox.MinX          = textureBox.m_vMin.x;
  destinationBox.MinY          = textureBox.m_vMin.y;
  destinationBox.MinZ          = textureBox.m_vMin.z;
  destinationBox.MaxX          = textureBox.m_vMax.x;
  destinationBox.MaxY          = textureBox.m_vMax.y;
  destinationBox.MaxZ          = textureBox.m_vMax.z;

  Diligent::TextureSubResData textureSubresourceData = {};
  textureSubresourceData.pData                       = subresourceData.m_pData;
  textureSubresourceData.Stride                      = subresourceData.m_uiStride;
  textureSubresourceData.DepthStride                 = subresourceData.m_uiDepthStride;

  m_pCommandList->UpdateTexture(pTextureD3D12->GetTexture(), textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, destinationBox, textureSubresourceData, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D12::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  auto pSourceTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pSourceTexture);
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestinationTexture);

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTextureD3D12->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTextureD3D12->GetTexture();
  copyTextureDescription.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copyTextureDescription.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pCommandList->CopyTexture(copyTextureDescription);
}

void xiiGALCommandListD3D12::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  auto pSourceTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pSourceTexture);
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestinationTexture);

  Diligent::Box sourceBox = {};
  sourceBox.MinX          = box.m_vMin.x;
  sourceBox.MinY          = box.m_vMin.y;
  sourceBox.MinZ          = box.m_vMin.z;
  sourceBox.MaxX          = box.m_vMax.x;
  sourceBox.MaxY          = box.m_vMax.y;
  sourceBox.MaxZ          = box.m_vMax.z;

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTextureD3D12->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTextureD3D12->GetTexture();
  copyTextureDescription.pSrcBox                      = &sourceBox;

  copyTextureDescription.SrcMipLevel              = sourceMipLevelData.m_uiMipLevel;
  copyTextureDescription.SrcSlice                 = sourceMipLevelData.m_uiArraySlice;
  copyTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  copyTextureDescription.DstMipLevel              = destinationMipLevelData.m_uiMipLevel;
  copyTextureDescription.DstSlice                 = destinationMipLevelData.m_uiArraySlice;
  copyTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copyTextureDescription.DstX                     = vDestinationPoint.x;
  copyTextureDescription.DstY                     = vDestinationPoint.y;
  copyTextureDescription.DstZ                     = vDestinationPoint.z;

  m_pCommandList->CopyTexture(copyTextureDescription);
}

void xiiGALCommandListD3D12::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  auto pSourceTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pSourceTexture);
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestinationTexture);

  const auto& sourceTextureDescription = pSourceTextureD3D12->GetDescription();

  Diligent::ResolveTextureSubresourceAttribs resolveTextureDescription;
  resolveTextureDescription.Format = xiiDiligentTypeConversions::GetTextureFormat(sourceTextureDescription.m_Format);

  resolveTextureDescription.SrcMipLevel              = sourceMipLevelData.m_uiMipLevel;
  resolveTextureDescription.SrcSlice                 = sourceMipLevelData.m_uiArraySlice;
  resolveTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  resolveTextureDescription.DstMipLevel              = destinationMipLevelData.m_uiMipLevel;
  resolveTextureDescription.DstSlice                 = destinationMipLevelData.m_uiArraySlice;
  resolveTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
}

void xiiGALCommandListD3D12::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  auto* pTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pTextureView);

  m_pCommandList->GenerateMips(pTextureViewD3D12->GetTextureView());
}

xiiResult xiiGALCommandListD3D12::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData)
{
  auto pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pTexture);

  Diligent::MAP_TYPE  textureMapType  = xiiDiligentTypeConversions::GetMapType(mapType);
  Diligent::MAP_FLAGS textureMapFlags = xiiDiligentTypeConversions::GetMapFlags(mapFlags);

  Diligent::Box sourceBox = {};
  sourceBox.MinX          = textureBox.m_vMin.x;
  sourceBox.MinY          = textureBox.m_vMin.y;
  sourceBox.MinZ          = textureBox.m_vMin.z;
  sourceBox.MaxX          = textureBox.m_vMax.x;
  sourceBox.MaxY          = textureBox.m_vMax.y;
  sourceBox.MaxZ          = textureBox.m_vMax.z;

  Diligent::MappedTextureSubresource mappedSubResource = {};
  m_pCommandList->MapTextureSubresource(pTextureD3D12->GetTexture(), textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureMapType, textureMapFlags, &sourceBox, mappedSubResource);

  mappedData.m_pData         = mappedSubResource.pData;
  mappedData.m_uiStride      = mappedSubResource.Stride;
  mappedData.m_uiDepthStride = mappedSubResource.DepthStride;

  return (mappedData.m_pData != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListD3D12::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  auto pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pTexture);

  m_pCommandList->UnmapTextureSubresource(pTextureD3D12->GetTexture(), textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiStringBuilder sb;
  m_pCommandList->BeginDebugGroup(sName.GetData(sb), color.GetData());
}

void xiiGALCommandListD3D12::EndDebugGroupPlatform()
{
  m_pCommandList->EndDebugGroup();
}

void xiiGALCommandListD3D12::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiStringBuilder sb;
  m_pCommandList->InsertDebugLabel(sName.GetData(sb), color.GetData());
}

void xiiGALCommandListD3D12::FlushPlatform()
{
  m_pCommandList->Flush();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandListD3D12);
