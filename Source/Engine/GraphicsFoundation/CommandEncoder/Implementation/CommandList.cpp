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

void xiiGALCommandList::SetStencilRef(xiiUInt8 uiStencilRef)
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

void xiiGALCommandList::SetViewports(xiiArrayPtr<xiiRectFloat> pViewports, float fMinDepth, float fMaxDepth)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetViewports arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  if (pViewports.GetCount() > 1)
  {
    XII_VERIFY_COMMAND_LIST(m_pDevice->GetFeatures().m_MultiViewport == xiiGALDeviceFeatureState::Enabled, "SetViewports arguments are invalid. The device does does not have the Multi Viewport feature enabled.");
  }

  XII_ASSERT_DEV(pViewports.GetCount() < XII_GAL_MAX_VIEWPORT_COUNT, "The number of viewports ({0}) exceeds the maximum viewport count ({1}).", pViewports.GetCount(), XII_GAL_MAX_VIEWPORT_COUNT);

  xiiUInt32 uiViewportCount = xiiMath::Min<xiiUInt32>(XII_GAL_MAX_VIEWPORT_COUNT, pViewports.GetCount());

  // If no viewports are set

  m_Viewports.SetCount(uiViewportCount);
  m_Viewports.PushBackRange(pViewports);
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

#define XII_VERIFY_DRAW(expression, ...)       \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return XII_FAILURE; } \
  } while (false)

xiiResult xiiGALCommandList::Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawCommand arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawCommand arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawCommand arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(uiVertexCount != 0, "DrawCommand vertex count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  if (DrawPlatform(uiVertexCount, uiStartVertex).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexed command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexed command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexed command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!m_hIndexBuffer.IsInvalidated(), "DrawIndexed command argumenst are invalid. No index buffer is bound.");
  XII_VERIFY_DRAW(uiIndexCount != 0, "DrawIndexed index count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  if (DrawIndexedPlatform(uiIndexCount, uiStartIndex).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!m_hIndexBuffer.IsInvalidated(), "DrawIndexedInstanced command argumenst are invalid. No index buffer is bound.");
  XII_VERIFY_DRAW(uiIndexCountPerInstance != 0, "DrawIndexedInstanced index count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_DRAW(uiInstanceCount != 0, "DrawIndexedInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  if (DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!hIndirectArgumentBuffer.IsInvalidated(), "DrawIndexedInstancedIndirect command argumenst are invalid. The indirect argument buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DRAW(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offse/stride, etc.).

  if (DrawIndexedInstancedIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(uiVertexCountPerInstance != 0, "DrawInstanced vertex count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_DRAW(uiInstanceCount != 0, "DrawInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  if (DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_VERIFY_DRAW(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_DRAW(!m_hPipelineState.IsInvalidated(), "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DRAW(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DRAW(!hIndirectArgumentBuffer.IsInvalidated(), "DrawIndexedInstancedIndirect command argumenst are invalid. The indirect argument buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DRAW(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offse/stride, etc.).

  if (DrawInstancedIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes).Succeeded())
  {
    CountDrawCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
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
  XII_VERIFY_DISPATCH(!m_hPipelineState.IsInvalidated(), "Dispatch command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DISPATCH(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "Dispatch command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DISPATCH(m_hRenderPass.IsInvalidated(), "Dispatch command arguments are invalid. Dispatch command must be performed outside of render pass.");
  XII_VERIFY_DISPATCH(uiThreadGroupCountX != 0U && uiThreadGroupCountY != 0U && uiThreadGroupCountZ != 0U, "Dispatch command arguments are invalid. At least one of the thread group counts are zero, this is OK as the dispatch command will be ignored, but may be unintentional.");

  if (DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ).Succeeded())
  {
    CountDispatchCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiResult xiiGALCommandList::DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  XII_VERIFY_DISPATCH(!m_hPipelineState.IsInvalidated(), "DispatchIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_DISPATCH(m_pDevice->GetPipelineState(m_hPipelineState)->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "DispatchIndirect command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pDevice->GetPipelineState(m_hPipelineState)->GetDebugName());
  XII_VERIFY_DISPATCH(m_hRenderPass.IsInvalidated(), "DispatchIndirect command arguments are invalid. DispatchIndirect command must be performed outside of render pass.");

  XII_VERIFY_DISPATCH(!hIndirectArgumentBuffer.IsInvalidated(), "The indirect arguments buffer is invalidated.");

  xiiGALBuffer* pIndirectArgumentsBuffer = m_pDevice->GetBuffer(hIndirectArgumentBuffer);
  const auto&   bufferDescription        = pIndirectArgumentsBuffer->GetDescription();

  XII_VERIFY_DISPATCH(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentsBuffer->GetDebugName());

  const xiiUInt32 uiOffset = ((sizeof(xiiUInt32) * 3) + uiArgumentOffsetInBytes);
  XII_VERIFY_DISPATCH(uiOffset <= bufferDescription.m_uiSize, "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' offset in bytes must be at least {1} bytes.", pIndirectArgumentsBuffer->GetDebugName());

  if (DispatchIndirectPlatform(pIndirectArgumentsBuffer, uiArgumentOffsetInBytes).Succeeded())
  {
    CountDispatchCall();

    return XII_SUCCESS;
  }
  return XII_FAILURE;
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
