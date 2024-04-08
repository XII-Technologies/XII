#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

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

xiiGALCommandListD3D11::xiiGALCommandListD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceD3D11, creationDescription), m_pCommandList(pDeviceD3D11->GetImmediateContext())
{
}

xiiGALCommandListD3D11::~xiiGALCommandListD3D11() = default;

void xiiGALCommandListD3D11::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  auto pPipelineStateD3D11 = static_cast<xiiGALPipelineStateD3D11*>(pPipelineState);

#if 0
  if (m_pPipelineState == (pPipelineStateD3D11 != nullptr ? pPipelineStateD3D11->GetPipelineState() : nullptr))
    return;

  m_pPipelineState = (pPipelineStateD3D11 != nullptr ? pPipelineStateD3D11->GetPipelineState() : nullptr);
#endif
}

void xiiGALCommandListD3D11::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  ID3D11DepthStencilState* pD3D11DepthStencilState = m_pPipelineState ? m_pPipelineState->GetD3D11DepthStencilState() : nullptr;

  m_pCommandList->OMSetDepthStencilState(pD3D11DepthStencilState, uiStencilRef);
}

void xiiGALCommandListD3D11::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  xiiUInt32         uiSampleMask     = 0xFFFFFFFFU;
  ID3D11BlendState* pD3D11BlendState = nullptr;

  if (m_pPipelineState != nullptr)
  {
    const auto& description = m_pPipelineState->GetDescription();

    if (description.IsAnyGraphicsPipeline())
    {
      uiSampleMask     = description.m_GraphicsPipeline.m_uiSampleMask;
      pD3D11BlendState = m_pPipelineState->GetD3D11BlendState();
    }
  }
  m_pCommandList->OMSetBlendState(pD3D11BlendState, blendFactor.GetData(), uiSampleMask);
}

void xiiGALCommandListD3D11::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_CHECK_AT_COMPILETIME_MSG(XII_GAL_MAX_VIEWPORT_COUNT >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "The XII_GAL_MAX_VIEWPORT_COUNT must be greater than (or equal to) D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.");

  XII_ASSERT_DEV(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");

  D3D11_VIEWPORT d3d11Viewports[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiViewPortIndex = 0; uiViewPortIndex < pViewports.GetCount(); ++uiViewPortIndex)
  {
    d3d11Viewports[uiViewPortIndex].TopLeftX = pViewports[uiViewPortIndex].m_fTopLeftX;
    d3d11Viewports[uiViewPortIndex].TopLeftY = pViewports[uiViewPortIndex].m_fTopLeftY;
    d3d11Viewports[uiViewPortIndex].Width    = pViewports[uiViewPortIndex].m_fWidth;
    d3d11Viewports[uiViewPortIndex].Height   = pViewports[uiViewPortIndex].m_fHeight;
    d3d11Viewports[uiViewPortIndex].MinDepth = pViewports[uiViewPortIndex].m_fMinDepth;
    d3d11Viewports[uiViewPortIndex].MaxDepth = pViewports[uiViewPortIndex].m_fMaxDepth;
  }

  // All viewports must be set atomically as one operation.
  // Any viewports not defined by the call are disabled.
  m_pCommandList->RSSetViewports(pViewports.GetCount(), d3d11Viewports);
}

void xiiGALCommandListD3D11::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_CHECK_AT_COMPILETIME_MSG(XII_GAL_MAX_VIEWPORT_COUNT >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "The XII_GAL_MAX_VIEWPORT_COUNT must be greater than (or equal to) D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.");

  XII_ASSERT_DEV(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");

  D3D11_RECT d3d11ScissorRects[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiScissorRectIndex = 0; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    d3d11ScissorRects[uiScissorRectIndex].left   = pRects[uiScissorRectIndex].Left();
    d3d11ScissorRects[uiScissorRectIndex].top    = pRects[uiScissorRectIndex].Top();
    d3d11ScissorRects[uiScissorRectIndex].right  = pRects[uiScissorRectIndex].Right();
    d3d11ScissorRects[uiScissorRectIndex].bottom = pRects[uiScissorRectIndex].Bottom();
  }

  // All scissor rects must be set atomically as one operation.
  // Any scissor rects not defined by the call are disabled.
  m_pCommandList->RSSetScissorRects(pRects.GetCount(), d3d11ScissorRects);
}

void xiiGALCommandListD3D11::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto pIndexBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndexBuffer);

  if (pIndexBufferD3D11 != nullptr)
  {
    const auto& indexFormat = pIndexBufferD3D11->GetIndexFormat();

    DXGI_FORMAT d3d11IndexFormat = DXGI_FORMAT_UNKNOWN;
    if (indexFormat == xiiGALValueType::UInt32)
    {
      d3d11IndexFormat = DXGI_FORMAT_R32_UINT;
    }
    else if (indexFormat == xiiGALValueType::UInt16)
    {
      d3d11IndexFormat = DXGI_FORMAT_R16_UINT;
    }
    else
    {
      xiiLog::Error("Unsupported index format, only xiiGALValueType::UInt16 or xiiGALValueType::UInt32 are supported.");
      return;
    }

    m_pCommittedIndexBuffer           = pIndexBufferD3D11->GetBuffer();
    m_CommittedIndexBufferFormat      = indexFormat;
    m_uiCommittedIndexDataStartOffset = static_cast<xiiUInt32>(uiByteOffset);
  }
  else
  {
    m_pCommittedIndexBuffer           = nullptr;
    m_CommittedIndexBufferFormat      = xiiGALValueType::Undefined;
    m_uiCommittedIndexDataStartOffset = static_cast<xiiUInt32>(uiByteOffset);
  }
  m_bCommittedIndexBufferUpToDate = false;
}

void xiiGALCommandListD3D11::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  XII_ASSERT_DEV((uiStartSlot + pVertexBuffers.GetCount()) <= XII_GAL_MAX_VERTEX_BUFFER_COUNT, "The number of vertex buffers to set, exceeds the maximum amount.");

  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    m_CommittedVertexBuffersRange.Reset();

    // Reset only the buffer slots that are not being set.
    for (xiiUInt32 i = 0; i < uiStartSlot; ++i)
    {
      m_pCommittedVertexBuffers[i]      = nullptr;
      m_CommittedVertexBufferOffsets[i] = 0U;
      m_CommittedVertexBufferStrides[i] = 0U;
    }
    for (xiiUInt32 i = uiStartSlot + pVertexBuffers.GetCount(); i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
    {
      m_pCommittedVertexBuffers[i]      = nullptr;
      m_CommittedVertexBufferOffsets[i] = 0U;
      m_CommittedVertexBufferStrides[i] = 0U;
    }

    m_bCommittedVertexBufferUpToDate = false;
  }

  for (xiiUInt32 i = uiStartSlot; i, pVertexBuffers.GetCount(); ++i)
  {
    auto pVertexBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pVertexBuffers[i]);

    ID3D11Buffer* pD3D11VertexBuffer   = pVertexBufferD3D11 ? pVertexBufferD3D11->GetBuffer() : nullptr;
    xiiUInt32     uiVertexBufferOffset = static_cast<xiiUInt32>(pByteOffsets[i]);
    xiiUInt32     uiVertexBufferStride = pVertexBufferD3D11 ? pVertexBufferD3D11->GetDescription().m_uiElementByteStride : 0U;

    if (m_pCommittedVertexBuffers[i] != pD3D11VertexBuffer || m_CommittedVertexBufferOffsets[i] != uiVertexBufferOffset || m_CommittedVertexBufferStrides[i] != uiVertexBufferStride)
    {
      m_pCommittedVertexBuffers[i]      = pD3D11VertexBuffer;
      m_CommittedVertexBufferOffsets[i] = uiVertexBufferOffset;
      m_CommittedVertexBufferStrides[i] = uiVertexBufferStride;

      m_CommittedVertexBuffersRange.SetToIncludeValue(i);

      m_bCommittedVertexBufferUpToDate = false;
    }
  }
  m_bCommittedVertexBufferUpToDate = true;
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
  m_pCommandList->ClearDepthStencilView(static_cast<ID3D11DepthStencilView*>(pDepthStencilViewD3D11->GetTextureView()), uiClearFlags, fDepthClear, uiStencilClear);
}

xiiResult xiiGALCommandListD3D11::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->Draw(uiVertexCount, uiStartVertex);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DrawIndexed(uiIndexCount, uiStartIndex, uiBaseVertex);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DrawIndexedInstancedIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DrawInstancedIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  XII_REPORT_FAILURE("DrawMesh is not supported in Direct3D 11.");

  return XII_FAILURE;
}

xiiResult xiiGALCommandListD3D11::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pIndirectArgumentBuffer);

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandList->DispatchIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D11 = static_cast<xiiGALQueryD3D11*>(pQuery);

  XII_ASSERT_DEV(pQueryD3D11 != nullptr, "Invalid resource.");

  if (pQueryD3D11->GetDescription().m_Type == xiiGALQueryType::Duration)
  {
    pQueryD3D11->SetDisjointQuery(BeginDisjointQuery());

    m_pCommandList->Begin(pQueryD3D11->GetQuery(0));
  }
  else
  {
    m_pCommandList->Begin(pQueryD3D11->GetQuery(0));
  }
}

void xiiGALCommandListD3D11::EndQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D11 = static_cast<xiiGALQueryD3D11*>(pQuery);

  XII_ASSERT_DEV(pQueryD3D11 != nullptr, "Invalid resource.");

  xiiEnum<xiiGALQueryType> queryType = pQuery->GetDescription().m_Type;

  XII_ASSERT_DEV(queryType != xiiGALQueryType::Duration || m_pActiveDisjointQuery, "There is no active disjoint query. Did you forget to call BeginQuery for this duration query.");

  if (queryType == xiiGALQueryType::Timestamp)
  {
    pQueryD3D11->SetDisjointQuery(BeginDisjointQuery());
  }
  m_pCommandList->End(pQueryD3D11->GetQuery(queryType == xiiGALQueryType::Duration ? 1 : 0));
}

void xiiGALCommandListD3D11::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pDestinationBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pBuffer);

  XII_ASSERT_DEV(pDestinationBufferD3D11 != nullptr, "Invalid resource.");

  D3D11_BOX destinationBox = {};
  destinationBox.left      = uiDestinationOffset;
  destinationBox.right     = uiDestinationOffset + pSourceData.GetCount();
  destinationBox.top       = 0U;
  destinationBox.bottom    = 1U;
  destinationBox.front     = 0U;
  destinationBox.back      = 1U;

  D3D11_BOX* pDestinationBox = (uiDestinationOffset == 0 && pSourceData.GetCount() == pDestinationBufferD3D11->GetDescription().m_uiSize) ? nullptr : &destinationBox;

  m_pCommandList->UpdateSubresource(pDestinationBufferD3D11->GetBuffer(), 0U, pDestinationBox, pSourceData.GetPtr(), 0U, 0U);
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
  sourceBox.left      = static_cast<xiiUInt32>(uiSourceOffset);
  sourceBox.right     = static_cast<xiiUInt32>(uiSourceOffset + uiSize);
  sourceBox.top       = 0U;
  sourceBox.bottom    = 1U;
  sourceBox.front     = 0U;
  sourceBox.back      = 1U;

  m_pCommandList->CopySubresourceRegion(pDestinationBufferD3D11->GetBuffer(), 0, static_cast<xiiUInt32>(uiDestinationOffset), 0, 0, pSourceBufferD3D11->GetBuffer(), 0, &sourceBox);
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

  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid resource.");

  m_pCommandList->Unmap(pBufferD3D11->GetBuffer(), 0U);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  auto pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(pTexture);

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();

  XII_ASSERT_DEV(textureDescription.m_Usage == xiiGALResourceUsage::Default || textureDescription.m_Usage == xiiGALResourceUsage::Sparse, "Only xiiGALResourceUsage::Default or xiiGALResourceUsage::Default textures should be updated with this method.");

  if (!subresourceData.m_hSourceBuffer.IsInvalidated())
  {
    xiiLog::Error("Direct3D11 does not support texture updates using texture subresource from GPU buffer");
    return;
  }

  xiiUInt32 uiWidth  = xiiMath::Max(textureBox.m_vMax.x - textureBox.m_vMin.x, 1U);
  xiiUInt32 uiHeight = xiiMath::Max(textureBox.m_vMax.y - textureBox.m_vMin.y, 1U);
  xiiUInt32 uiDepth  = xiiMath::Max(textureBox.m_vMax.z - textureBox.m_vMin.z, 1U);

  D3D11_BOX destinationBox = {};
  destinationBox.left      = textureBox.m_vMin.x;
  destinationBox.top       = textureBox.m_vMin.y;
  destinationBox.front     = textureBox.m_vMin.z;
  destinationBox.right     = textureBox.m_vMax.x;
  destinationBox.bottom    = textureBox.m_vMax.y;
  destinationBox.back      = textureBox.m_vMax.z;

  const auto& formatProperties = xiiGALGraphicsUtilities::GetTextureFormatProperties(textureDescription.m_Format);

  if (formatProperties.m_ComponentType == xiiGALTextureFormatComponentType::Compressed)
  {
    // Align update region by the compressed block size.
    XII_ASSERT_DEV((destinationBox.left % formatProperties.m_uiBlockWidth) == 0, "The update region min X coordinate ({0}) must be a multiple of a compressed block width ({1}).", destinationBox.left, formatProperties.m_uiBlockWidth);
    XII_ASSERT_DEV((formatProperties.m_uiBlockWidth % (formatProperties.m_uiBlockWidth - 1)) == 0, "The compressed block width ({0}) is expected to be a power of 2.", formatProperties.m_uiBlockWidth);
    destinationBox.right = (destinationBox.right + formatProperties.m_uiBlockWidth - 1) & ~(formatProperties.m_uiBlockHeight - 1);

    XII_ASSERT_DEV((destinationBox.top % formatProperties.m_uiBlockHeight) == 0, "The update region min X coordinate ({0}) must be a multiple of a compressed block height ({1}).", destinationBox.top, formatProperties.m_uiBlockHeight);
    XII_ASSERT_DEV((formatProperties.m_uiBlockHeight % (formatProperties.m_uiBlockHeight - 1)) == 0, "The compressed block height ({0}) is expected to be a power of 2.", formatProperties.m_uiBlockHeight);
    destinationBox.bottom = (destinationBox.bottom + formatProperties.m_uiBlockHeight - 1) & ~(formatProperties.m_uiBlockHeight - 1);
  }

  xiiUInt32 uiDestinationSubresourceIndex = D3D11CalcSubresource(textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  m_pCommandList->UpdateSubresource(pTextureD3D11->GetTexture(), uiDestinationSubresourceIndex, &destinationBox, subresourceData.m_pData, static_cast<xiiUInt32>(subresourceData.m_uiStride), static_cast<xiiUInt32>(subresourceData.m_uiDepthStride));
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

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  const auto& sourceTextureDescription      = pSourceTextureD3D11->GetDescription();
  const auto& destinationTextureDescription = pSourceTextureD3D11->GetDescription();

  DXGI_FORMAT textureFormat                 = xiiD3D11TypeConversions ::GetFormat(sourceTextureDescription.m_Format);
  xiiUInt32   uiSourceSubresourceIndex      = D3D11CalcSubresource(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiMipLevel, sourceTextureDescription.m_uiMipLevels);
  xiiUInt32   uiDestinationSubresourceIndex = D3D11CalcSubresource(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiMipLevel, destinationTextureDescription.m_uiMipLevels);

  m_pCommandList->ResolveSubresource(pDestinationTextureD3D11->GetTexture(), uiDestinationSubresourceIndex, pSourceTextureD3D11->GetTexture(), uiSourceSubresourceIndex, textureFormat);
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
  XII_ASSERT_DEV(m_hRenderPass.IsInvalidated(), "Flushing commandlist inside an active render pass is not allowed.");

  m_pCommandList->Flush();
}

xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> xiiGALCommandListD3D11::BeginDisjointQuery()
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  if (!m_pActiveDisjointQuery)
  {
    m_pActiveDisjointQuery = m_DisjointQueryPool.GetDisjointQuery(pDeviceD3D11->GetD3D11Device());

    // Disjoint timestamp queries should be only invoked once per frame or less.
    m_pCommandList->Begin(m_pActiveDisjointQuery->m_pQueryD3D11);

    m_pActiveDisjointQuery->m_bIsEnded = false;
  }
  return m_pActiveDisjointQuery;
}

xiiResult xiiGALCommandListD3D11::FlushDeferredStateChanges()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandListD3D11);
