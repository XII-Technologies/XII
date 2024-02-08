#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/States/PipelineState.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandList, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#define XII_VERIFY_COMMAND_LIST(expression, ...) \
  do                                             \
  {                                              \
    XII_ASSERT_DEV((expression), __VA_ARGS__);   \
    if (!(expression)) { return; }               \
  } while (false)

xiiGALCommandList::xiiGALCommandList(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALDeviceObject(), m_Description(creationDescription)
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

void xiiGALCommandList::SetPipelineState(xiiGALPipelineStateHandle hPipelineState)
{
}

void xiiGALCommandList::SetStencilRef(xiiUInt32 uiStencilRef)
{
  if (m_uiStencilRef != uiStencilRef)
  {
    m_uiStencilRef = uiStencilRef;

    SetStencilRefPlatform(m_uiStencilRef);
  }
}

void xiiGALCommandList::SetBlendFactor(const xiiColor& blendFactor)
{
  if (blendFactor != m_BlendFactors)
  {
    m_BlendFactors = blendFactor;

    SetBlendFactorPlatform(m_BlendFactors);
  }
}

void xiiGALCommandList::SetViewports(xiiArrayPtr<xiiGALViewport> pViewports)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetViewports arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  XII_ASSERT_DEV(pViewports.GetCount() < XII_GAL_MAX_VIEWPORT_COUNT, "The number of viewports ({0}) exceeds the maximum viewport count ({1}).", pViewports.GetCount(), XII_GAL_MAX_VIEWPORT_COUNT);

  xiiUInt32 uiViewportCount = xiiMath::Min<xiiUInt32>(XII_GAL_MAX_VIEWPORT_COUNT, pViewports.GetCount());

  if (uiViewportCount > 1)
  {
    XII_VERIFY_COMMAND_LIST(m_pDevice->GetFeatures().m_MultiViewport == xiiGALDeviceFeatureState::Enabled, "SetViewports arguments are invalid. The device does does not have the Multi Viewport feature enabled.");
  }

  m_Viewports.Clear();
  m_Viewports.PushBackRange(pViewports);

  for (xiiUInt32 i = 0; i < m_Viewports.GetCount(); ++i)
  {
    const auto& viewport = m_Viewports[i];

    XII_VERIFY_COMMAND_LIST(viewport.m_fWidth >= 0.0f, "SetViewports arguments are invalid. Incorrect viewport width ({0}) for index {1}.", viewport.m_fWidth, i);
    XII_VERIFY_COMMAND_LIST(viewport.m_fHeight >= 0.0f, "SetViewports arguments are invalid. Incorrect viewport height ({0}) for index {1}.", viewport.m_fHeight, i);
    XII_VERIFY_COMMAND_LIST(viewport.m_fMaxDepth >= viewport.m_fMinDepth, "SetViewports arguments are invalid. Incorrect viewport depth range [{0}, {1}] for index {2}.", viewport.m_fMinDepth, viewport.m_fMaxDepth, i);
  }

  SetViewportsPlatform(m_Viewports);
}

void xiiGALCommandList::SetScissorRects(xiiArrayPtr<xiiRectU32> pRects)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetScissorRects arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  XII_ASSERT_DEV(pRects.GetCount() < XII_GAL_MAX_VIEWPORT_COUNT, "The number of scissor rects ({0}) exceeds the maximum scissor rect count ({1}).", pRects.GetCount(), XII_GAL_MAX_VIEWPORT_COUNT);

  xiiUInt32 uiRectCount = pRects.GetCount();

  if (uiRectCount > 1)
  {
    XII_VERIFY_COMMAND_LIST(m_pDevice->GetFeatures().m_MultiViewport == xiiGALDeviceFeatureState::Enabled, "SetScissorRects arguments are invalid. The device does does not have the Multi Viewport feature enabled.");
  }

  m_ScissorRects.Clear();
  m_ScissorRects.PushBackRange(pRects);

  SetScissorRectsPlatform(m_ScissorRects);
}

void xiiGALCommandList::SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt32 uiByteOffset)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetIndexBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  if (m_hIndexBuffer == hIndexBuffer && m_uiIndexDataOffset == uiByteOffset)
    return;

  xiiGALBuffer* pIndexBuffer = m_pDevice->GetBuffer(hIndexBuffer);

  if (pIndexBuffer)
  {
    const auto& bufferDescription = pIndexBuffer->GetDescription();

    XII_VERIFY_COMMAND_LIST(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndexBuffer), "SetIndexBuffer arguments are invalid. The Index buffer '{0}' was not created with the xiiGALBindFlags::IndexBuffer bind flag.", pIndexBuffer->GetDebugName());
  }

  m_hIndexBuffer      = hIndexBuffer;
  m_uiIndexDataOffset = uiByteOffset;

  SetIndexBufferPlatform(pIndexBuffer, m_uiIndexDataOffset);
}

void xiiGALCommandList::SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt32> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetVertexBuffers arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(uiStartSlot < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "SetVertexBuffers arguments are invalid. The start slot ({0}) is out of range [0, {1}].", uiStartSlot, XII_GAL_MAX_VERTEX_BUFFER_COUNT - 1);
  XII_VERIFY_COMMAND_LIST((uiStartSlot + pVertexBuffers.GetCount()) < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "SetVertexBuffers arguments are invalid. The range of vertex buffer slots being set [{0}, {1}] is out of allowed range [0, {2}].", uiStartSlot, uiStartSlot + pVertexBuffers.GetCount() - 1, XII_GAL_MAX_VERTEX_BUFFER_COUNT - 1);

  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    // Reset only the buffer slots that are not being set.
    for (xiiUInt32 i = 0; i < uiStartSlot; ++i)
    {
      m_BoundVertexBuffers[i] = xiiGALBufferHandle();
    }
    for (xiiUInt32 i = uiStartSlot + pVertexBuffers.GetCount(); i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
    {
      m_BoundVertexBuffers[i] = xiiGALBufferHandle();
    }
  }

  xiiHybridArray<xiiGALBuffer*, 2U> boundVertexBuffers;

  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    xiiGALBuffer* pVertexBuffer = m_pDevice->GetBuffer(pVertexBuffers[i]);

    if (pVertexBuffer != nullptr)
    {
      const auto& bufferDescription = pVertexBuffer->GetDescription();

      XII_VERIFY_COMMAND_LIST(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::VertexBuffer), "SetVertexBuffer arguments are invalid. The Vertex buffer '{0}' was not created with the xiiGALBindFlags::VertexBuffer bind flag.", pVertexBuffer->GetDebugName());

      boundVertexBuffers.PushBack(pVertexBuffer);
    }
  }

  SetVertexBuffersPlatform(uiStartSlot, boundVertexBuffers, pByteOffsets, flags);
}

void xiiGALCommandList::ClearRenderTargetView(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor)
{
}

void xiiGALCommandList::ClearDepthStencilView(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

#define XII_VERIFY_DRAW(expression, ...)       \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return XII_FAILURE; } \
  } while (false)

xiiResult xiiGALCommandList::Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawCommand arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawCommand arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawCommand arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(uiVertexCount != 0, "DrawCommand vertex count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawPlatform(uiVertexCount, uiStartVertex);
}

xiiResult xiiGALCommandList::DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexed command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexed command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexed command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!m_hIndexBuffer.IsInvalidated(), "DrawIndexed command argumenst are invalid. No index buffer is bound.");
  XII_VERIFY_DRAW(uiIndexCount != 0, "DrawIndexed index count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

xiiResult xiiGALCommandList::DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!m_hIndexBuffer.IsInvalidated(), "DrawIndexedInstanced command argumenst are invalid. No index buffer is bound.");
  XII_VERIFY_DRAW(uiIndexCountPerInstance != 0, "DrawIndexedInstanced index count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_DRAW(uiInstanceCount != 0, "DrawIndexedInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

xiiResult xiiGALCommandList::DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!hIndirectArgumentBuffer.IsInvalidated(), "DrawIndexedInstancedIndirect command argumenst are invalid. The indirect argument buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DRAW(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offse/stride, etc.).

  return DrawIndexedInstancedIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes);
}

xiiResult xiiGALCommandList::DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(uiVertexCountPerInstance != 0, "DrawInstanced vertex count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_DRAW(uiInstanceCount != 0, "DrawInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

xiiResult xiiGALCommandList::DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!hIndirectArgumentBuffer.IsInvalidated(), "DrawIndexedInstancedIndirect command argumenst are invalid. The indirect argument buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DRAW(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offse/stride, etc.).

  return DrawInstancedIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes);
}

xiiResult xiiGALCommandList::DrawMesh(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  CountDrawCall();

  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "DrawMesh command arguments are invalid. Mesh shaders are not supported by this device.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawMesh command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Mesh, "DrawMesh command arguments are invalid. Pipeline state {0} is not a mesh pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());

  const auto& meshProperties = m_pDevice->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;

  XII_VERIFY_DRAW(uiThreadGroupCountX <= meshProperties.m_uiMaxThreadGroupCountX, "DrawMesh command arguments are invalid. The thread group count X ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountX, meshProperties.m_uiMaxThreadGroupCountX);
  XII_VERIFY_DRAW(uiThreadGroupCountY <= meshProperties.m_uiMaxThreadGroupCountY, "DrawMesh command arguments are invalid. The thread group count Y ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountY, meshProperties.m_uiMaxThreadGroupCountY);
  XII_VERIFY_DRAW(uiThreadGroupCountZ <= meshProperties.m_uiMaxThreadGroupCountZ, "DrawMesh command arguments are invalid. The thread group count Z ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountZ, meshProperties.m_uiMaxThreadGroupCountZ);

  const auto uiTotalThreadGroupCount = uiThreadGroupCountX + uiThreadGroupCountY + uiThreadGroupCountZ;
  XII_VERIFY_DRAW(uiTotalThreadGroupCount <= meshProperties.m_uiMaxThreadGroupTotalCount, "DrawMesh command arguments are invalid. The total thread group count ({0}) exceeds the maximum supported by the device ({1}).", uiTotalThreadGroupCount, meshProperties.m_uiMaxThreadGroupTotalCount);

  return DrawMeshPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

#undef XII_VERIFY_DRAW

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
  XII_VERIFY_DISPATCH(uiThreadGroupCountX != 0U && uiThreadGroupCountY != 0U && uiThreadGroupCountZ != 0U, "Dispatch command arguments are invalid. At least one of the thread group counts are zero, this is OK as the dispatch command will be ignored, but may be unintentional.");

  return DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

xiiResult xiiGALCommandList::DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDispatchCall();

  XII_VERIFY_DISPATCH(!m_hPipelineState.IsInvalidated(), "DispatchIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DISPATCH(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "DispatchIndirect command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DISPATCH(m_hRenderPass.IsInvalidated(), "DispatchIndirect command arguments are invalid. DispatchIndirect command must be performed outside of render pass.");

  XII_VERIFY_DISPATCH(!hIndirectArgumentBuffer.IsInvalidated(), "The indirect arguments buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DISPATCH(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  const xiiUInt32 uiOffset = ((sizeof(xiiUInt32) * 3) + uiArgumentOffsetInBytes);
  XII_VERIFY_DISPATCH(uiOffset <= bufferDescription.m_uiSize, "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' offset in bytes must be at least {1} bytes.", pIndirectArgumentsBuffer->GetDebugName());

  return DispatchIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes);
}

#undef XII_VERIFY_DISPATCH

#define XII_VERIFY_QUERY(expression, ...)      \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return; }             \
  } while (false)

void xiiGALCommandList::BeginQuery(xiiGALQueryHandle hQuery)
{
  XII_VERIFY_QUERY(!hQuery.IsInvalidated(), "BeginQuery must not be called on an invalidated query.");

  xiiGALQuery* pQuery           = m_pDevice->GetQuery(hQuery);
  const auto&  queryDescription = pQuery->GetDescription();

  XII_VERIFY_QUERY(queryDescription.m_Type != xiiGALQueryType::Timestamp, "BeginQuery cannot be called on timestamp queries. Use EndQuery instead to set the timestamp.");

  /// \todo GraphicsFoundation: Assert command queue compatibiliity.

  BeginQueryPlatform(pQuery);
}

void xiiGALCommandList::EndQuery(xiiGALQueryHandle hQuery)
{
  XII_VERIFY_QUERY(!hQuery.IsInvalidated(), "EndQuery must not be called on an invalidated query.");

  /// \todo GraphicsFoundation: Assert command queue compatibiliity.

  xiiGALQuery* pQuery = m_pDevice->GetQuery(hQuery);

  EndQueryPlatform(pQuery);
}

#undef XII_VERIFY_QUERY

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

#undef XII_VERIFY_COMMAND_LIST

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);
