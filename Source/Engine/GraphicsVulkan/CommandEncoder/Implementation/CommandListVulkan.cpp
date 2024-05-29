#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>
#include <GraphicsVulkan/States/PipelineStateVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

xiiGALCommandListVulkan::xiiGALCommandListVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListVulkan::~xiiGALCommandListVulkan() = default;

xiiResult xiiGALCommandListVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics))
  {
    m_pCommandList = pDeviceVulkan->GetImmediateContext();

    xiiLog::Error("Failed to create command list, no graphics command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Compute))
  {
    m_pCommandList = pDeviceVulkan->GetComputeContext();

    xiiLog::Error("Failed to create command list, no compute command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer))
  {
    m_pCommandList = pDeviceVulkan->GetTransferContext();

    xiiLog::Error("Failed to create command list, no transfer command queue found.");
  }
  else if (m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::SparseBinding))
  {
    m_pCommandList = pDeviceVulkan->GetSparseBindingContext();

    xiiLog::Error("Failed to create command list, no sparse binding command queue found.");
  }
  return m_pCommandList != nullptr ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListVulkan::DeInitPlatform()
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::ExecutePlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandListVulkan::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  auto pPipelineStateVulkan = static_cast<xiiGALPipelineStateVulkan*>(pPipelineState);

  if (m_pPipelineState == (pPipelineStateVulkan != nullptr ? pPipelineStateVulkan->GetPipelineState() : nullptr))
    return;

  m_pPipelineState = (pPipelineStateVulkan != nullptr ? pPipelineStateVulkan->GetPipelineState() : nullptr);
}

void xiiGALCommandListVulkan::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  m_pCommandList->SetStencilRef(uiStencilRef);
}

void xiiGALCommandListVulkan::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  m_pCommandList->SetBlendFactors(blendFactor.GetData());
}

void xiiGALCommandListVulkan::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
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

void xiiGALCommandListVulkan::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
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

void xiiGALCommandListVulkan::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto pIndexBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndexBuffer);

  m_pBoundIndexBuffer = (pIndexBufferVulkan != nullptr) ? pIndexBufferVulkan->GetBuffer() : nullptr;
  m_IndexFormat       = (pIndexBufferVulkan != nullptr) ? pIndexBufferVulkan->GetIndexFormat() : Diligent::VT_UNDEFINED;

  m_pCommandList->SetIndexBuffer(m_pBoundIndexBuffer, uiByteOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListVulkan::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    xiiMemoryUtils::DefaultConstruct(m_pBoundVertexBuffers);

    m_BoundVertexBuffersRange.Reset();
  }
  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    auto* pVertexBuffersVulkan = static_cast<xiiGALBufferVulkan*>(pVertexBuffers[i]);

    m_pBoundVertexBuffers[i] = (pVertexBuffersVulkan != nullptr) ? pVertexBuffersVulkan->GetBuffer() : nullptr;

    m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }

  Diligent::SET_VERTEX_BUFFERS_FLAGS setVertexBufferFlags = Diligent::SET_VERTEX_BUFFERS_FLAG_NONE;
  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
    setVertexBufferFlags |= Diligent::SET_VERTEX_BUFFERS_FLAG_RESET;

  m_pCommandList->SetVertexBuffers(uiStartSlot, pVertexBuffers.GetCount(), m_pBoundVertexBuffers + uiStartSlot, pByteOffsets.GetPtr() + uiStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, setVertexBufferFlags);
}

void xiiGALCommandListVulkan::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  auto pRenderTargetViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pRenderTargetView);

  m_pCommandList->ClearRenderTarget(pRenderTargetViewVulkan->GetTextureView(), clearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListVulkan::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  auto pDepthStencilViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pDepthStencilView);

  Diligent::CLEAR_DEPTH_STENCIL_FLAGS clearFlags = Diligent::CLEAR_DEPTH_FLAG_NONE;
  if (bClearDepth)
    clearFlags |= Diligent::CLEAR_DEPTH_FLAG;
  if (bClearStencil)
    clearFlags |= Diligent::CLEAR_STENCIL_FLAG;

  m_pCommandList->ClearDepthStencil(pDepthStencilViewVulkan->GetTextureView(), clearFlags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

xiiResult xiiGALCommandListVulkan::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
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

xiiResult xiiGALCommandListVulkan::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
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

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
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

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  Diligent::DrawIndexedIndirectAttribs drawIndexedInstancedDescription = {};
  drawIndexedInstancedDescription.IndexType                            = m_IndexFormat;
  drawIndexedInstancedDescription.pAttribsBuffer                       = pIndirectArgumentBufferVulkan->GetBuffer();
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

xiiResult xiiGALCommandListVulkan::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
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

xiiResult xiiGALCommandListVulkan::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawInstancedIndirectDescription    = {};
  drawInstancedIndirectDescription.pAttribsBuffer                   = pIndirectArgumentBufferVulkan->GetBuffer();
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

xiiResult xiiGALCommandListVulkan::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DrawMeshAttribs drawMeshDescription = {};
  drawMeshDescription.ThreadGroupCountX         = uiThreadGroupCountX;
  drawMeshDescription.ThreadGroupCountY         = uiThreadGroupCountY;
  drawMeshDescription.ThreadGroupCountZ         = uiThreadGroupCountZ;

  m_pCommandList->DrawMesh(drawMeshDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  Diligent::DispatchComputeAttribs dispatchDescription = {};
  dispatchDescription.ThreadGroupCountX                = uiThreadGroupCountX;
  dispatchDescription.ThreadGroupCountY                = uiThreadGroupCountY;
  dispatchDescription.ThreadGroupCountZ                = uiThreadGroupCountZ;

  m_pCommandList->DispatchCompute(dispatchDescription);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs = {};
  DispatchAttribs.pAttribsBuffer                           = pIndirectArgumentBufferVulkan->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode         = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  DispatchAttribs.DispatchArgsByteOffset                   = uiArgumentOffsetInBytes;

  m_pCommandList->DispatchComputeIndirect(DispatchAttribs);

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);

  m_pCommandList->BeginQuery(pQueryVulkan->GetQuery());
}

void xiiGALCommandListVulkan::EndQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryVulkan = static_cast<xiiGALQueryVulkan*>(pQuery);

  m_pCommandList->EndQuery(pQueryVulkan->GetQuery());
}

void xiiGALCommandListVulkan::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pDestinationBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  m_pCommandList->UpdateBuffer(pDestinationBufferVulkan->GetBuffer(), uiDestinationOffset, pSourceData.GetCount(), pSourceData.GetPtr(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListVulkan::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  auto pSourceBufferVulkan      = static_cast<xiiGALBufferVulkan*>(pSourceBuffer);
  auto pDestinationBufferVulkan = static_cast<xiiGALBufferVulkan*>(pDestinationBuffer);

  m_pCommandList->CopyBuffer(pSourceBufferVulkan->GetBuffer(), 0U, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferVulkan->GetBuffer(), 0U, pDestinationBuffer->GetDescription().m_uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListVulkan::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  auto pSourceBufferVulkan      = static_cast<xiiGALBufferVulkan*>(pSourceBuffer);
  auto pDestinationBufferVulkan = static_cast<xiiGALBufferVulkan*>(pDestinationBuffer);

  m_pCommandList->CopyBuffer(pSourceBufferVulkan->GetBuffer(), uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferVulkan->GetBuffer(), uiDestinationOffset, uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

xiiResult xiiGALCommandListVulkan::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  auto pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  Diligent::MAP_TYPE  bufferMapType  = xiiDiligentTypeConversions::GetMapType(mapType);
  Diligent::MAP_FLAGS bufferMapFlags = xiiDiligentTypeConversions::GetMapFlags(mapFlags);

  m_pCommandList->MapBuffer(pBufferVulkan->GetBuffer(), bufferMapType, bufferMapFlags, pMappedData);

  return (pMappedData != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListVulkan::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  auto pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pBuffer);

  Diligent::MAP_TYPE bufferMapType = xiiDiligentTypeConversions::GetMapType(mapType);

  m_pCommandList->UnmapBuffer(pBufferVulkan->GetBuffer(), bufferMapType);

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  auto        pTextureVulkan     = static_cast<xiiGALTextureVulkan*>(pTexture);
  const auto& textureDescription = pTextureVulkan->GetDescription();

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

  m_pCommandList->UpdateTexture(pTextureVulkan->GetTexture(), textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, destinationBox, textureSubresourceData, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandListVulkan::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  auto pSourceTextureVulkan      = static_cast<xiiGALTextureVulkan*>(pSourceTexture);
  auto pDestinationTextureVulkan = static_cast<xiiGALTextureVulkan*>(pDestinationTexture);

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTextureVulkan->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTextureVulkan->GetTexture();
  copyTextureDescription.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copyTextureDescription.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pCommandList->CopyTexture(copyTextureDescription);
}

void xiiGALCommandListVulkan::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  auto pSourceTextureVulkan      = static_cast<xiiGALTextureVulkan*>(pSourceTexture);
  auto pDestinationTextureVulkan = static_cast<xiiGALTextureVulkan*>(pDestinationTexture);

  Diligent::Box sourceBox = {};
  sourceBox.MinX          = box.m_vMin.x;
  sourceBox.MinY          = box.m_vMin.y;
  sourceBox.MinZ          = box.m_vMin.z;
  sourceBox.MaxX          = box.m_vMax.x;
  sourceBox.MaxY          = box.m_vMax.y;
  sourceBox.MaxZ          = box.m_vMax.z;

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTextureVulkan->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTextureVulkan->GetTexture();
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

void xiiGALCommandListVulkan::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  auto pSourceTextureVulkan      = static_cast<xiiGALTextureVulkan*>(pSourceTexture);
  auto pDestinationTextureVulkan = static_cast<xiiGALTextureVulkan*>(pDestinationTexture);

  const auto& sourceTextureDescription = pSourceTextureVulkan->GetDescription();

  Diligent::ResolveTextureSubresourceAttribs resolveTextureDescription;
  resolveTextureDescription.Format = xiiDiligentTypeConversions::GetTextureFormat(sourceTextureDescription.m_Format);

  resolveTextureDescription.SrcMipLevel              = sourceMipLevelData.m_uiMipLevel;
  resolveTextureDescription.SrcSlice                 = sourceMipLevelData.m_uiArraySlice;
  resolveTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  resolveTextureDescription.DstMipLevel              = destinationMipLevelData.m_uiMipLevel;
  resolveTextureDescription.DstSlice                 = destinationMipLevelData.m_uiArraySlice;
  resolveTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
}

void xiiGALCommandListVulkan::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  auto* pTextureViewVulkan = static_cast<xiiGALTextureViewVulkan*>(pTextureView);

  m_pCommandList->GenerateMips(pTextureViewVulkan->GetTextureView());
}

xiiResult xiiGALCommandListVulkan::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32 textureBox, xiiGALMappedTextureSubresource& mappedData)
{
  auto pTextureVulkan = static_cast<xiiGALTextureVulkan*>(pTexture);

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
  m_pCommandList->MapTextureSubresource(pTextureVulkan->GetTexture(), textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureMapType, textureMapFlags, &sourceBox, mappedSubResource);

  mappedData.m_pData         = mappedSubResource.pData;
  mappedData.m_uiStride      = mappedSubResource.Stride;
  mappedData.m_uiDepthStride = mappedSubResource.DepthStride;

  return (mappedData.m_pData != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALCommandListVulkan::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  auto pTextureVulkan = static_cast<xiiGALTextureVulkan*>(pTexture);

  m_pCommandList->UnmapTextureSubresource(pTextureVulkan->GetTexture(), textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice);

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiStringBuilder sb;
  m_pCommandList->BeginDebugGroup(sName.GetData(sb), color.GetData());
}

void xiiGALCommandListVulkan::EndDebugGroupPlatform()
{
  m_pCommandList->EndDebugGroup();
}

void xiiGALCommandListVulkan::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiStringBuilder sb;
  m_pCommandList->InsertDebugLabel(sName.GetData(sb), color.GetData());
}

void xiiGALCommandListVulkan::FlushPlatform()
{
  m_pCommandList->Flush();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandListVulkan);
