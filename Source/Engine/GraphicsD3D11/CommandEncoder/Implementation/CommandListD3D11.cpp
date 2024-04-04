#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

#include <GraphicsD3D11/Resources/BottomLevelASD3D11.h>
#include <GraphicsD3D11/Resources/BufferD3D11.h>
#include <GraphicsD3D11/Resources/BufferViewD3D11.h>
#include <GraphicsD3D11/Resources/FenceD3D11.h>
#include <GraphicsD3D11/Resources/FramebufferD3D11.h>
#include <GraphicsD3D11/Resources/QueryD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Resources/SamplerD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>
#include <GraphicsD3D11/Resources/TopLevelASD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>
#include <GraphicsD3D11/States/BlendStateD3D11.h>
#include <GraphicsD3D11/States/DepthStencilStateD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>
#include <GraphicsD3D11/States/PipelineStateD3D11.h>
#include <GraphicsD3D11/States/RasterizerStateD3D11.h>

#include <d3d11_1.h>

xiiGALCommandListD3D11::xiiGALCommandListD3D11(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListD3D11::~xiiGALCommandListD3D11() = default;

xiiResult xiiGALCommandListD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  // m_pCommandList = pDeviceD3D11->GetImmediateContext();

  return m_pCommandList != nullptr ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::ExecutePlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandListD3D11::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  auto pPipelineStateD3D11 = static_cast<xiiGALPipelineStateD3D11*>(pPipelineState);

  if (m_pPipelineState == (pPipelineStateD3D11 != nullptr ? pPipelineStateD3D11->GetPipelineState() : nullptr))
    return;

  m_pPipelineState = (pPipelineStateD3D11 != nullptr ? pPipelineStateD3D11->GetPipelineState() : nullptr);
}

void xiiGALCommandListD3D11::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  m_pCommandList->SetStencilRef(uiStencilRef);
}

void xiiGALCommandListD3D11::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  m_pCommandList->SetBlendFactors(blendFactor.GetData());
}

void xiiGALCommandListD3D11::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
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

void xiiGALCommandListD3D11::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
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

void xiiGALCommandListD3D11::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto pIndexBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndexBuffer);

  m_pBoundIndexBuffer = (pIndexBufferD3D11 != nullptr) ? pIndexBufferD3D11->GetBuffer() : nullptr;
  m_IndexFormat       = (pIndexBufferD3D11 != nullptr) ? pIndexBufferD3D11->GetIndexFormat() : Diligent::VT_UNDEFINED;

  m_pCommandList->SetIndexBuffer(m_pBoundIndexBuffer, uiByteOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D11::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    xiiMemoryUtils::DefaultConstruct(m_pBoundVertexBuffers);

    m_BoundVertexBuffersRange.Reset();
  }
  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    auto* pVertexBuffersD3D11 = static_cast<xiiGALBufferD3D11*>(pVertexBuffers[i]);

    m_pBoundVertexBuffers[i] = (pVertexBuffersD3D11 != nullptr) ? pVertexBuffersD3D11->GetBuffer() : nullptr;

    m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }

  Diligent::SET_VERTEX_BUFFERS_FLAGS setVertexBufferFlags = Diligent::SET_VERTEX_BUFFERS_FLAG_NONE;
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
    setVertexBufferFlags |= Diligent::SET_VERTEX_BUFFERS_FLAG_RESET;

  m_pCommandList->SetVertexBuffers(uiStartSlot, pVertexBuffers.GetCount(), m_pBoundVertexBuffers + uiStartSlot, pByteOffsets.GetPtr() + uiStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, setVertexBufferFlags);
}

void xiiGALCommandListD3D11::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  auto pRenderTargetViewD3D11 = static_cast<xiiGALTextureViewD3D11*>(pRenderTargetView);

  XII_ASSERT_DEV(pRenderTargetViewD3D11 != nullptr, "Invalid resource.");

  // The full extent of the resource view is always cleared. Viewport and scissor settings are not applied.
  m_pCommandList->ClearRenderTargetView(static_cast<ID3D11RenderTargetView*>(pRenderTargetViewD3D11->GetTextureView()), clearColor.GetData());
}

void xiiGALCommandListD3D11::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  auto pDepthStencilViewD3D11 = static_cast<xiiGALTextureViewD3D11*>(pDepthStencilView);

  XII_ASSERT_DEV(pDepthStencilViewD3D11 != nullptr, "Invalid resource.");

  xiiUInt32 uiClearFlags = 0;
  if (bClearDepth)
    uiClearFlags |= D3D11_CLEAR_DEPTH;
  if (bClearStencil)
    uiClearFlags |= D3D11_CLEAR_STENCIL;

  // The full extent of the resource view is always cleared. Viewport and scissor settings are not applied.
  m_pCommandList->ClearDepthStencilView(static_cast<ID3D11DepthStencilView*>(pDepthStencilViewD3D11->GetTextureView()), uiClearFlags, fDepthClear, uiStencilClear)
}

xiiResult xiiGALCommandListD3D11::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
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

xiiResult xiiGALCommandListD3D11::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
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

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
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

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  Diligent::DrawIndexedIndirectAttribs drawIndexedInstancedDescription = {};
  drawIndexedInstancedDescription.IndexType                            = m_IndexFormat;
  drawIndexedInstancedDescription.pAttribsBuffer                       = pIndirectArgumentBufferD3D11->GetBuffer();
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

xiiResult xiiGALCommandListD3D11::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
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

xiiResult xiiGALCommandListD3D11::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawInstancedIndirectDescription    = {};
  drawInstancedIndirectDescription.pAttribsBuffer                   = pIndirectArgumentBufferD3D11->GetBuffer();
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

xiiResult xiiGALCommandListD3D11::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DrawMeshAttribs drawMeshDescription = {};
  drawMeshDescription.ThreadGroupCountX         = uiThreadGroupCountX;
  drawMeshDescription.ThreadGroupCountY         = uiThreadGroupCountY;
  drawMeshDescription.ThreadGroupCountZ         = uiThreadGroupCountZ;

  m_pCommandList->DrawMesh(drawMeshDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DispatchComputeAttribs dispatchDescription = {};
  dispatchDescription.ThreadGroupCountX                = uiThreadGroupCountX;
  dispatchDescription.ThreadGroupCountY                = uiThreadGroupCountY;
  dispatchDescription.ThreadGroupCountZ                = uiThreadGroupCountZ;

  m_pCommandList->DispatchCompute(dispatchDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs = {};
  DispatchAttribs.pAttribsBuffer                           = pIndirectArgumentBufferD3D11->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode         = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  DispatchAttribs.DispatchArgsByteOffset                   = uiArgumentOffsetInBytes;

  m_pCommandList->DispatchComputeIndirect(DispatchAttribs);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D11 = static_cast<xiiGALQueryD3D11*>(pQuery);

  m_pCommandList->BeginQuery(pQueryD3D11->GetQuery());
}

void xiiGALCommandListD3D11::EndQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D11 = static_cast<xiiGALQueryD3D11*>(pQuery);

  m_pCommandList->EndQuery(pQueryD3D11->GetQuery());
}

void xiiGALCommandListD3D11::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pDestinationBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pBuffer);

  m_pCommandList->UpdateBuffer(pDestinationBufferD3D11->GetBuffer(), uiDestinationOffset, pSourceData.GetCount(), pSourceData.GetPtr(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D11::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  auto pSourceBufferD3D11      = static_cast<xiiGALBufferD3D11*>(pSourceBuffer);
  auto pDestinationBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pDestinationBuffer);

  XII_ASSERT_DEV(pSourceBufferD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationBufferD3D11 != nullptr, "Invalid resource.");

  m_pCommandList->CopyResource(pDestinationBufferD3D11->GetBuffer(), pSourceBufferD3D11->GetBuffer());
}

void xiiGALCommandListD3D11::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  auto pSourceBufferD3D11      = static_cast<xiiGALBufferD3D11*>(pSourceBuffer);
  auto pDestinationBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pDestinationBuffer);

  XII_ASSERT_DEV(pSourceBufferD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationBufferD3D11 != nullptr, "Invalid resource.");

  D3D11_BOX sourceBox = {};
  sourceBox.left      = uiSourceOffset;
  sourceBox.right     = uiSourceOffset + uiSize;
  sourceBox.top       = 0;
  sourceBox.bottom    = 1;
  sourceBox.front     = 0;
  sourceBox.back      = 1;

  m_pCommandList->CopySubresourceRegion(pDestinationBufferD3D11->GetBuffer(), 0, uiDestinationOffset, 0, 0, pSourceBufferD3D11->GetBuffer(), 0, &sourceBox);
}

xiiResult xiiGALCommandListD3D11::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  auto pBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pBuffer);

  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid resource.");

  D3D11_MAP bufferMapType = static_cast<D3D11_MAP>(0U);
  xiiUInt32 uiMapFlags    = 0U;
  xiiD3D11TypeConversions::GetMapTypeAndFlags(mapType, mapFlags, bufferMapType, uiMapFlags);

  D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  if (FAILED(m_pCommandList->Map(pBufferD3D11->GetBuffer(), 0U, bufferMapType, uiMapFlags, &mappedSubresource)))
  {
    xiiLog::Error("Failed to map buffer '{0}'.", pBufferD3D11->GetDebugName());
    return XII_FAILURE;
  }

  pMappedData = mappedSubresource.pData;

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  auto pBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pBuffer);

  XII_ASSERT_DEV(pBuffer != nullptr, "");

  m_pCommandList->Unmap(pBufferD3D11->GetBuffer(), 0U);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  auto        pTextureD3D11      = static_cast<xiiGALTextureD3D11*>(pTexture);
  const auto& textureDescription = pTextureD3D11->GetDescription();

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

  m_pCommandList->UpdateTexture(pTextureD3D11->GetTexture(), textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, destinationBox, textureSubresourceData, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListD3D11::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  auto pSourceTextureD3D11      = static_cast<xiiGALTextureD3D11*>(pSourceTexture);
  auto pDestinationTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pDestinationTexture);

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  m_pCommandList->CopyResource(pDestinationTextureD3D11->GetTexture(), pSourceTextureD3D11->GetTexture());
}

void xiiGALCommandListD3D11::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  auto pSourceTextureD3D11      = static_cast<xiiGALTextureD3D11*>(pSourceTexture);
  auto pDestinationTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pDestinationTexture);

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  D3D11_BOX sourceBox = {};
  sourceBox.left      = box.m_vMin.x;
  sourceBox.top       = box.m_vMin.y;
  sourceBox.front     = box.m_vMin.z;
  sourceBox.right     = box.m_vMax.x;
  sourceBox.bottom    = box.m_vMax.y;
  sourceBox.back      = box.m_vMax.z;

  xiiUInt32 uiSourceSubresource      = D3D11CalcSubresource(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pSourceTextureD3D11->GetDescription().m_uiMipLevels);
  xiiUInt32 uiDestinationSubresource = D3D11CalcSubresource(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, pDestinationTextureD3D11->GetDescription().m_uiMipLevels);

  m_pCommandList->CopySubresourceRegion(pDestinationTextureD3D11->GetTexture(), uiDestinationSubresource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pSourceTextureD3D11->GetTexture(), uiSourceSubresource, &sourceBox);
}

void xiiGALCommandListD3D11::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  auto pSourceTextureD3D11      = static_cast<xiiGALTextureD3D11*>(pSourceTexture);
  auto pDestinationTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pDestinationTexture);

  const auto& sourceTextureDescription = pSourceTextureD3D11->GetDescription();

  Diligent::ResolveTextureSubresourceAttribs resolveTextureDescription;
  resolveTextureDescription.Format = xiiDiligentTypeConversions::GetTextureFormat(sourceTextureDescription.m_Format);

  resolveTextureDescription.SrcMipLevel              = sourceMipLevelData.m_uiMipLevel;
  resolveTextureDescription.SrcSlice                 = sourceMipLevelData.m_uiArraySlice;
  resolveTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  resolveTextureDescription.DstMipLevel              = destinationMipLevelData.m_uiMipLevel;
  resolveTextureDescription.DstSlice                 = destinationMipLevelData.m_uiArraySlice;
  resolveTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
}

void xiiGALCommandListD3D11::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  auto* pTextureViewD3D11 = static_cast<xiiGALTextureViewD3D11*>(pTextureView);

  XII_ASSERT_DEV(pTextureViewD3D11 != nullptr, "Invalid resource.");

  m_pCommandList->GenerateMips(static_cast<ID3D11ShaderResourceView*>(pTextureViewD3D11->GetTextureView()));
}

xiiResult xiiGALCommandListD3D11::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData)
{
  auto pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pTexture);

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();
  D3D11_MAP   textureMapType     = static_cast<D3D11_MAP>(0U);
  xiiUInt32   uiMapFlags         = 0U;
  xiiD3D11TypeConversions::GetMapTypeAndFlags(mapType, mapFlags, textureMapType, uiMapFlags);

  xiiUInt32 uiSubresource = D3D11CalcSubresource(textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  if (FAILED(m_pCommandList->Map(pTextureD3D11->GetTexture(), uiSubresource, textureMapType, uiMapFlags, &mappedSubresource)))
  {
    // XII_ASSERT_DEV(hResult == DXGI_ERROR_WAS_STILL_DRAWING, "");

    xiiLog::Error("Failed to map texture subresource.");

    mappedData = xiiGALMappedTextureSubresource();
    return XII_FAILURE;
  }

  mappedData.m_pData         = mappedSubresource.pData;
  mappedData.m_uiStride      = mappedSubresource.RowPitch;
  mappedData.m_uiDepthStride = mappedSubresource.DepthPitch;

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  auto pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pTexture);

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();
  xiiUInt32   uiSubresource      = D3D11CalcSubresource(textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  m_pCommandList->Unmap(pTextureD3D11->GetTexture(), uiSubresource);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  if (SUCCEEDED(m_pCommandList->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    xiiStringBuilder sb;
    xiiStringWChar   wsMarker(sName.GetData(sb));
    pAnnotationD3D11->BeginEvent(wsMarker.GetData());
  }
}

void xiiGALCommandListD3D11::EndDebugGroupPlatform()
{
  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  if (SUCCEEDED(m_pCommandList->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    pAnnotationD3D11->EndEvent();
  }
}

void xiiGALCommandListD3D11::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  if (SUCCEEDED(m_pCommandList->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    xiiStringBuilder sb;
    xiiStringWChar   wsMarker(sName.GetData(sb));
    pAnnotationD3D11->SetMarker(wsMarker.GetData());
  }
}

void xiiGALCommandListD3D11::FlushPlatform()
{
  m_pCommandList->Flush();
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandListD3D11);
