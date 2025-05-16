#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSetVertexBufferFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSetVertexBufferFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALSetVertexBufferFlags::Reset),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALStateTransitionType, 1)
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::Immediate),
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::Begin),
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::End),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALStateTransitionFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::UpdateState),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::DiscardContent),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::Aliasing),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALStateTransitionMode, 1)
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::None),
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::Transition),
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::Verify),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandList, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#define XII_VERIFY_COMMAND_LIST(expression, ...) \
  do                                             \
  {                                              \
    XII_ASSERT_DEV((expression), __VA_ARGS__);   \
    if (!(expression)) { return; }               \
  } while (false)

#define XII_VERIFY_COMMAND_LIST_RESULT(expression, ...) \
  do                                                    \
  {                                                     \
    XII_ASSERT_DEV((expression), __VA_ARGS__);          \
    if (!(expression)) { return XII_FAILURE; }          \
  } while (false)

#define XII_VERIFY_COMMAND_LIST_BOOL(expression, ...) \
  do                                                  \
  {                                                   \
    XII_ASSERT_DEV((expression), __VA_ARGS__);        \
    if (!(expression)) { return false; }              \
  } while (false)

xiiGALCommandList::xiiGALCommandList(xiiSharedPtr<xiiGALDevice> pDevice, xiiGALCommandQueue* pCommandQueue, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALDeviceObject(pDevice), m_Description(creationDescription), m_pCommandQueue(pCommandQueue)
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

void xiiGALCommandList::ValidateTextureRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& box)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_VERIFY_COMMAND_LIST(uiMipLevel < textureDescription.m_uiMipLevels, "Mip level ({}) is out of permitted range [0, {}].", uiMipLevel, textureDescription.m_uiMipLevels - 1);
  XII_VERIFY_COMMAND_LIST(box.IsValid(), "Invalid box range provided.");

  if (textureDescription.IsArray())
  {
    XII_VERIFY_COMMAND_LIST(uiSlice < textureDescription.GetArraySize(), "Array slice ({}) is out of permitted range [0, {}].", textureDescription.GetArraySize() - 1);
  }
  else
  {
    XII_VERIFY_COMMAND_LIST(uiSlice == 0, "Array slice ({}) must be 0 for non-array textures.", uiSlice);
  }

  const auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

  xiiUInt32 uiMipWidth = xiiMath::Max(textureDescription.GetWidth() >> uiMipLevel, 1U);

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    const xiiUInt32 uiBlockAlignedMipWidth = (uiMipWidth + (formatProperties.m_uiBlockWidth - 1)) & ~(formatProperties.m_uiBlockWidth - 1);

    XII_VERIFY_COMMAND_LIST(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "");
    XII_VERIFY_COMMAND_LIST(box.m_vMax.x <= uiBlockAlignedMipWidth, "Region max X coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.x, uiBlockAlignedMipWidth);
    XII_VERIFY_COMMAND_LIST((box.m_vMin.x % formatProperties.m_uiBlockWidth) == 0, "For compressed formats, the region min X coordinate ({}) must be a multiple of the block width ({}).", box.m_vMin.x, formatProperties.m_uiBlockWidth);
    XII_VERIFY_COMMAND_LIST((box.m_vMax.x % formatProperties.m_uiBlockWidth) == 0 || box.m_vMax.x == uiMipWidth, "For compressed formats, the region max X coordinate ({}) must be a multiple of the block width ({}) or equal to the mip level ({}).", box.m_vMax.x, formatProperties.m_uiBlockWidth, uiMipWidth);
  }
  else
  {
    XII_VERIFY_COMMAND_LIST(box.m_vMax.x <= uiMipWidth, "Region max X coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.x, uiMipWidth);
  }

  if (textureDescription.m_Type != xiiGALResourceDimension::Texture1D && textureDescription.m_Type != xiiGALResourceDimension::Texture1DArray)
  {
    const xiiUInt32 uiMipHeight = xiiMath::Max(textureDescription.GetHeight() >> uiMipLevel, 1U);

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
    {
      XII_VERIFY_COMMAND_LIST(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "");

      const xiiUInt32 uiBlockAlignedMipHeight = (uiMipHeight + (formatProperties.m_uiBlockHeight - 1)) & ~(formatProperties.m_uiBlockHeight - 1);

      XII_VERIFY_COMMAND_LIST(box.m_vMax.y <= uiBlockAlignedMipHeight, "Region max Y coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.y, uiBlockAlignedMipHeight);
      XII_VERIFY_COMMAND_LIST((box.m_vMin.y % formatProperties.m_uiBlockHeight) == 0U, "For compressed formats, the region min Y coordinate ({}) must be a multiple of block height ({}).", box.m_vMin.y, formatProperties.m_uiBlockHeight);
      XII_VERIFY_COMMAND_LIST((box.m_vMax.y % formatProperties.m_uiBlockHeight) == 0U || box.m_vMax.y == uiMipHeight, "For compressed formats, the region max Y coordinate ({}) must be a multiple of block height ({}) or equal the mip level height.", box.m_vMax.y, formatProperties.m_uiBlockHeight, uiMipHeight);
    }
    else
    {
      XII_VERIFY_COMMAND_LIST(box.m_vMax.y <= uiMipHeight, "Region max Y coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.y, uiMipHeight);
    }
  }

  if (textureDescription.m_Type == xiiGALResourceDimension::Texture3D)
  {
    const xiiUInt32 uiMipDepth = xiiMath::Max(textureDescription.GetDepth() >> uiMipLevel, 1U);

    XII_VERIFY_COMMAND_LIST(box.m_vMax.z <= uiMipDepth, "Region max Z coordinate ({}) is out of permitted range [0, {}].", uiMipDepth);
  }
  else
  {
    XII_VERIFY_COMMAND_LIST(box.m_vMin.z == 0, "Region min Z ({}) must be 0 for all but 3D textures.", box.m_vMin.z);
    XII_VERIFY_COMMAND_LIST(box.m_vMax.z == 1, "Region max Z ({}) must be 1 for all but 3D textures.", box.m_vMax.z);
  }
#endif
}

void xiiGALCommandList::ValidateTextureUpdateRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox, const xiiGALTextureSubResourceData& subresourceData)
{
  XII_VERIFY_COMMAND_LIST(!subresourceData.m_pData.IsEmpty(), "CPU data pointer must not be empty.");

  ValidateTextureRegion(textureDescription, uiMipLevel, uiSlice, destinationBox);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_VERIFY_COMMAND_LIST(textureDescription.m_uiSampleCount == 1, "Only non-multisampled textures can be updated UpdateTexture().");
  XII_VERIFY_COMMAND_LIST(xiiMemoryUtils::IsSizeAligned(subresourceData.m_uiStride, 16ULL), "Texture data stride ({}) must be at least 16-bit aligned.", subresourceData.m_uiStride);
  XII_VERIFY_COMMAND_LIST(xiiMemoryUtils::IsSizeAligned(subresourceData.m_uiDepthStride, 16ULL), "Texture data depth stride ({}) must be at least 16-bit aligned.", subresourceData.m_uiDepthStride);

  xiiVec3U32  vUpdateRegion    = destinationBox.GetExtents();
  const auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);
  xiiUInt32   uiRowSize        = 0;
  xiiUInt32   uiRowCount       = 0;

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    // Align update region size by the block size. This is only necessary when updating coarse mip levels. Otherwise, update region Width/Height should be multiples of the block size.
    XII_VERIFY_COMMAND_LIST(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "");
    XII_VERIFY_COMMAND_LIST(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "");

    vUpdateRegion.x = (vUpdateRegion.x + (formatProperties.m_uiBlockWidth - 1)) & ~(formatProperties.m_uiBlockWidth - 1);
    vUpdateRegion.y = (vUpdateRegion.y + (formatProperties.m_uiBlockHeight - 1)) & ~(formatProperties.m_uiBlockHeight - 1);

    uiRowSize  = vUpdateRegion.x / xiiUInt32{formatProperties.m_uiBlockWidth} * xiiUInt32{formatProperties.m_uiComponentSize};
    uiRowCount = vUpdateRegion.y / formatProperties.m_uiBlockHeight;
  }
  else
  {
    uiRowSize  = vUpdateRegion.x * xiiUInt32{formatProperties.m_uiComponentSize} * xiiUInt32{formatProperties.m_uiComponentCount};
    uiRowCount = vUpdateRegion.y;
  }

  XII_VERIFY_COMMAND_LIST(subresourceData.m_uiStride >= uiRowSize, "Source data stride ({}) is below the image row size ({}).", subresourceData.m_uiStride, uiRowSize);

  const xiiUInt64 uiPlaneSize = subresourceData.m_uiStride * uiRowCount;

  XII_VERIFY_COMMAND_LIST(vUpdateRegion.z == 1U || subresourceData.m_uiDepthStride >= uiPlaneSize, "Source data depth stride ({}) is below the image plane size ({}).", uiPlaneSize);
#endif
}

void xiiGALCommandList::Begin()
{
  XII_VERIFY_COMMAND_LIST(m_RecordingState == RecordingState::Ended || m_RecordingState == RecordingState::Reset, "The command list has not been ended.");
  XII_VERIFY_COMMAND_LIST(m_RecordingState != RecordingState::Submitted, "The command list has been submitted and is no longer available for recording commands.");

  BeginPlatform();
}

void xiiGALCommandList::End()
{
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "The current active render pass has not been ended.");
  XII_VERIFY_COMMAND_LIST(m_RecordingState == RecordingState::Recording, "The command list has not begun.");

  EndPlatform();
}

void xiiGALCommandList::Reset()
{
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "The current active render pass has not been ended.");
  XII_VERIFY_COMMAND_LIST(m_RecordingState != RecordingState::Submitted, "The command list has been submitted and cannot be resetted until after queue execution.");

  if (m_RecordingState == RecordingState::Recording)
  {
    End();
  }
  if (m_RecordingState != RecordingState::Reset)
  {
    ResetPlatform();
  }
}

xiiUInt64 xiiGALCommandList::Submit()
{
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "The current active render pass has not been ended.");
  XII_ASSERT_DEV(m_RecordingState != xiiGALCommandList::RecordingState::Reset, "Commandlist is already reset.");
  XII_ASSERT_DEV(m_RecordingState != xiiGALCommandList::RecordingState::Submitted, "Commandlist is already submitted!");

  if (m_RecordingState == xiiGALCommandList::RecordingState::Recording)
  {
    End();
  }
  if (m_RecordingState == xiiGALCommandList::RecordingState::Ended)
  {
    SubmitPlatform();
  }
  return xiiMath::MaxValue<xiiUInt64>();
}

void xiiGALCommandList::SetPipelineState(xiiSharedPtr<xiiGALPipelineState> pPipelineState)
{
  if (m_pPipelineState == pPipelineState)
    return;

  m_pPipelineState             = pPipelineState;
  m_pPipelineResourceSignature = nullptr;

  if (m_pPipelineState != nullptr)
  {
    m_pPipelineResourceSignature = pPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else
  {
    m_pPipelineState             = nullptr;
    m_pPipelineResourceSignature = nullptr;
  }

  SetPipelineStatePlatform(pPipelineState);
}

void xiiGALCommandList::SetStencilRef(xiiUInt32 uiStencilRef)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetStencilRef arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  if (m_uiStencilRef != uiStencilRef)
  {
    m_uiStencilRef = uiStencilRef;

    SetStencilRefPlatform(m_uiStencilRef);
  }
}

void xiiGALCommandList::SetBlendFactor(const xiiColor& blendFactor)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetBlendFactor arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

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

void xiiGALCommandList::SetIndexBuffer(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetIndexBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");

  if (m_pIndexBuffer == pIndexBuffer && m_uiIndexDataOffset == uiByteOffset)
    return;

  if (pIndexBuffer)
  {
    const auto& bufferDescription = pIndexBuffer->GetDescription();

    XII_VERIFY_COMMAND_LIST(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndexBuffer), "SetIndexBuffer arguments are invalid. The Index buffer '{0}' was not created with the xiiGALBindFlags::IndexBuffer bind flag.", pIndexBuffer->GetDebugName());
  }

  m_pIndexBuffer      = pIndexBuffer;
  m_uiIndexDataOffset = uiByteOffset;

  SetIndexBufferPlatform(pIndexBuffer, m_uiIndexDataOffset);
}

void xiiGALCommandList::SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetVertexBuffers arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(uiStartSlot < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "SetVertexBuffers arguments are invalid. The start slot ({0}) is out of range [0, {1}].", uiStartSlot, XII_GAL_MAX_VERTEX_BUFFER_COUNT - 1);
  XII_VERIFY_COMMAND_LIST((uiStartSlot + pVertexBuffers.GetCount()) < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "SetVertexBuffers arguments are invalid. The range of vertex buffer slots being set [{0}, {1}] is out of allowed range [0, {2}].", uiStartSlot, uiStartSlot + pVertexBuffers.GetCount() - 1, XII_GAL_MAX_VERTEX_BUFFER_COUNT - 1);

  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    // Reset only the buffer slots that are not being set.
    for (xiiUInt32 i = 0; i < uiStartSlot; ++i)
    {
      m_VertexBuffers.EnsureCount(i + 1);
      m_VertexBuffersOffsets.EnsureCount(i + 1);

      m_VertexBuffers[i]        = nullptr;
      m_VertexBuffersOffsets[i] = 0;
    }
    for (xiiUInt32 i = uiStartSlot + pVertexBuffers.GetCount(); i < m_VertexBuffers.GetCount(); ++i)
    {
      m_VertexBuffers[i]        = nullptr;
      m_VertexBuffersOffsets[i] = 0;
    }
  }

  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    if (pVertexBuffers[i] != nullptr)
    {
      const auto& bufferDescription = pVertexBuffers[i]->GetDescription();

      XII_VERIFY_COMMAND_LIST(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::VertexBuffer), "SetVertexBuffer arguments are invalid. The Vertex buffer '{0}' was not created with the xiiGALBindFlags::VertexBuffer bind flag.", pVertexBuffers[i]->GetDebugName());

      m_VertexBuffers[i]        = pVertexBuffers[i];
      m_VertexBuffersOffsets[i] = pByteOffsets[i];
    }
  }

  SetVertexBuffersPlatform(uiStartSlot, m_VertexBuffers, m_VertexBuffersOffsets, flags);
}

void xiiGALCommandList::SetConstantBuffer(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetConstantBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetConstantBuffer requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && resource.m_ResourceType == xiiGALShaderResourceType::ConstantBuffer && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The constant buffer resource '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_VERIFY_COMMAND_LIST(pConstantBuffer == nullptr || pConstantBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer), "Incorrect buffer bind flags. The buffer must be created with xiiGALBindFlags::UniformBuffer if not invalidated.");

  SetConstantBufferPlatform(bindingInformation, pConstantBuffer);
}

void xiiGALCommandList::SetShaderResourceBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetShaderResourceBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetShaderResourceBufferView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && resource.m_ResourceType == xiiGALShaderResourceType::BufferSRV && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The buffer resource view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_VERIFY_COMMAND_LIST(pBufferView == nullptr || pBufferView->GetDescription().m_ViewType == xiiGALBufferViewType::ShaderResource, "Incorrect buffer view type. The view must be created with xiiGALBufferViewType::ShaderResource if not invalidated.");

  SetShaderResourceBufferViewPlatform(bindingInformation, pBufferView);
}

void xiiGALCommandList::SetShaderResourceTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetShaderResourceTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetShaderResourceTextureView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && (resource.m_ResourceType == xiiGALShaderResourceType::TextureSRV || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The texture resource view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_VERIFY_COMMAND_LIST(pTextureView == nullptr || pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::ShaderResource, "Incorrect buffer view type. The view must be created with xiiGALTextureViewType::ShaderResource if not invalidated.");

  SetShaderResourceTextureViewPlatform(bindingInformation, pTextureView);
}

void xiiGALCommandList::SetUnorderedAccessBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetUnorderedAccessBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetUnorderedAccessBufferView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && resource.m_ResourceType == xiiGALShaderResourceType::BufferUAV && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The unordered access buffer view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_VERIFY_COMMAND_LIST(pBufferView == nullptr || pBufferView->GetDescription().m_ViewType == xiiGALBufferViewType::UnorderedAccess, "Incorrect buffer view type. The view must be created with xiiGALBufferViewType::UnorderedAccess if not invalidated.");

  SetUnorderedAccessBufferViewPlatform(bindingInformation, pBufferView);
}

void xiiGALCommandList::SetUnorderedAccessTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetUnorderedAccessTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetUnorderedAccessTextureView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && resource.m_ResourceType == xiiGALShaderResourceType::TextureUAV && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The unordered access texture view '{0}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_VERIFY_COMMAND_LIST(pTextureView == nullptr || pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::UnorderedAccess, "Incorrect buffer view type. The view must be created with xiiGALTextureViewType::UnorderedAccess if not invalidated.");

  SetUnorderedAccessTextureViewPlatform(bindingInformation, pTextureView);
}

void xiiGALCommandList::SetSampler(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "SetSampler arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pPipelineState != nullptr, "SetSampler requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const auto& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const auto& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == bindingInformation.m_sName && (resource.m_ResourceType == xiiGALShaderResourceType::Sampler || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && resource.m_ShaderStages.AreAllSet(bindingInformation.m_ShaderStages))
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(bResourceFound, "The sampler resource '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);

  SetSamplerPlatform(bindingInformation, pSampler);
}

xiiResult xiiGALCommandList::CommitShaderResources(xiiEnum<xiiGALStateTransitionMode> mode)
{
  return CommitShaderResourcesPlatform(mode);
}

void xiiGALCommandList::ClearRenderTargetView(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "ClearRenderTargetView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(pRenderTargetView != nullptr, "ClearRenderTargetView arguments are invalid. The texture view handle has been invalidated.");

  const auto& viewDescription = pRenderTargetView->GetDescription();

  XII_VERIFY_COMMAND_LIST(viewDescription.m_ViewType == xiiGALTextureViewType::RenderTarget, "The texture view '{0}' was not created with the xiiGALTextureViewType::RenderTarget.", pRenderTargetView->GetDebugName());

  ClearRenderTargetViewPlatform(pRenderTargetView, clearColor);
}

void xiiGALCommandList::ClearDepthStencilView(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "ClearDepthStencilView arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(pDepthStencilView != nullptr, "ClearDepthStencilView arguments are invalid. The texture view handle has been invalidated.");

  const auto& viewDescription = pDepthStencilView->GetDescription();

  XII_VERIFY_COMMAND_LIST(viewDescription.m_ViewType == xiiGALTextureViewType::DepthStencil, "The texture view '{0}' was not created with the xiiGALTextureViewType::DepthStencil.", pDepthStencilView->GetDebugName());
  XII_VERIFY_COMMAND_LIST(bClearDepth || bClearStencil, "At least one of bClearDepth or bClearStencil must be set.");

  ClearDepthStencilViewPlatform(pDepthStencilView, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void xiiGALCommandList::BeginRenderPass(const xiiGALBeginRenderPassDescription& beginRenderPass)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(beginRenderPass.m_pRenderPass != nullptr, "BeginRenderPass: Render pass handle is invalid.");
  XII_VERIFY_COMMAND_LIST(beginRenderPass.m_pFramebuffer != nullptr, "BeginRenderPass: Framebuffer handle is invalid.");

  const auto& renderPassDescription = beginRenderPass.m_pRenderPass->GetDescription();

  xiiUInt32 uiRequiredClearValueCount = 0;
  for (xiiUInt32 i = 0; i < renderPassDescription.m_Attachments.GetCount(); ++i)
  {
    const auto& attachmentDescription = renderPassDescription.m_Attachments[i];
    const auto& formatProperties      = xiiGALTextureUtilities::GetResourceFormatProperties(attachmentDescription.m_Format);

    if (attachmentDescription.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
    {
      uiRequiredClearValueCount = i + 1;
    }

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
    {
      if (attachmentDescription.m_StencilLoadOperation == xiiGALAttachmentLoadOperation::Clear)
      {
        uiRequiredClearValueCount = i + 1;
      }
    }
  }

  XII_VERIFY_COMMAND_LIST(beginRenderPass.m_ClearValues.GetCount() >= uiRequiredClearValueCount, "BeginRenderPass: At least {0} clear values are required, but only {1} are provided.", uiRequiredClearValueCount, beginRenderPass.m_ClearValues.GetCount());

  /// \todo GraphicsFoundation: Potentially reset the current render targets here.
  /// \todo GraphicsFoundation: Implement render pass attachment handling in the GAL, as well as state transitions in the begin render pass description.

  m_pRenderPass  = beginRenderPass.m_pRenderPass;
  m_pFramebuffer = beginRenderPass.m_pFramebuffer;

  BeginRenderPassPlatform(m_pRenderPass, m_pFramebuffer, beginRenderPass.m_ClearValues.GetArrayPtr());
}

void xiiGALCommandList::NextSubpass()
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass != nullptr, "NextSubpass: Render pass handle is invalid.");
  XII_VERIFY_COMMAND_LIST(m_pFramebuffer != nullptr, "NextSubpass: Framebuffer handle is invalid.");

  NextSubpassPlatform();
}

void xiiGALCommandList::EndRenderPass()
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass != nullptr, "NextSubpass: Render pass handle is invalid.");
  XII_VERIFY_COMMAND_LIST(m_pFramebuffer != nullptr, "NextSubpass: Framebuffer handle is invalid.");

  EndRenderPassPlatform();

  m_pRenderPass  = nullptr;
  m_pFramebuffer = nullptr;
}

xiiResult xiiGALCommandList::Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawCommand arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawCommand arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawCommand arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(uiVertexCount != 0, "DrawCommand vertex count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawPlatform(uiVertexCount, uiStartVertex);
}

xiiResult xiiGALCommandList::DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexed command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawIndexed command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexed command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(m_pIndexBuffer != nullptr, "DrawIndexed command arguments are invalid. No index buffer is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(uiIndexCount != 0, "DrawIndexed index count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawIndexedPlatform(uiIndexCount, uiStartIndex, uiBaseVertex);
}

xiiResult xiiGALCommandList::DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawIndexedInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(m_pIndexBuffer != nullptr, "DrawIndexedInstanced command arguments are invalid. No index buffer is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(uiIndexCountPerInstance != 0, "DrawIndexedInstanced index count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_COMMAND_LIST_RESULT(uiInstanceCount != 0, "DrawIndexedInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance);
}

xiiResult xiiGALCommandList::DrawIndexedInstancedIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(pIndirectArgumentBuffer != nullptr, "DrawIndexedInstancedIndirect command arguments are invalid. The indirect argument buffer is invalidated.");

  const auto& bufferDescription = pIndirectArgumentBuffer->GetDescription();

  XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offset/stride, etc.).

  return DrawIndexedInstancedIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

xiiResult xiiGALCommandList::DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawInstanced command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawInstanced command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawInstanced command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(uiVertexCountPerInstance != 0, "DrawInstanced vertex count per instance is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  XII_VERIFY_COMMAND_LIST_RESULT(uiInstanceCount != 0, "DrawInstanced instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");

  return DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance);
}

xiiResult xiiGALCommandList::DrawInstancedIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "DrawIndexedInstancedIndirect command arguments are invalid. The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawIndexedInstancedIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedInstancedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(pIndirectArgumentBuffer != nullptr, "DrawIndexedInstancedIndirect command arguments are invalid. The indirect argument buffer is invalidated.");

  const auto& bufferDescription = pIndirectArgumentBuffer->GetDescription();

  XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentBuffer->GetDebugName());

  /// \todo GraphicsFoundation: Add more validation and parameters (draw count, draw offset/stride, etc.).

  return DrawInstancedIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

xiiResult xiiGALCommandList::DrawMesh(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  CountDrawCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "DrawMesh command arguments are invalid. Mesh shaders are not supported by this device.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DrawMesh command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Mesh, "DrawMesh command arguments are invalid. Pipeline state {0} is not a mesh pipeline.", m_pPipelineState->GetDebugName());

  const auto& meshProperties = m_pDevice->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;

  XII_VERIFY_COMMAND_LIST_RESULT(uiThreadGroupCountX <= meshProperties.m_uiMaxThreadGroupCountX, "DrawMesh command arguments are invalid. The thread group count X ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountX, meshProperties.m_uiMaxThreadGroupCountX);
  XII_VERIFY_COMMAND_LIST_RESULT(uiThreadGroupCountY <= meshProperties.m_uiMaxThreadGroupCountY, "DrawMesh command arguments are invalid. The thread group count Y ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountY, meshProperties.m_uiMaxThreadGroupCountY);
  XII_VERIFY_COMMAND_LIST_RESULT(uiThreadGroupCountZ <= meshProperties.m_uiMaxThreadGroupCountZ, "DrawMesh command arguments are invalid. The thread group count Z ({0}) exceeds the maximum supported by the device ({1}).", uiThreadGroupCountZ, meshProperties.m_uiMaxThreadGroupCountZ);

  const auto uiTotalThreadGroupCount = uiThreadGroupCountX + uiThreadGroupCountY + uiThreadGroupCountZ;
  XII_VERIFY_COMMAND_LIST_RESULT(uiTotalThreadGroupCount <= meshProperties.m_uiMaxThreadGroupTotalCount, "DrawMesh command arguments are invalid. The total thread group count ({0}) exceeds the maximum supported by the device ({1}).", uiTotalThreadGroupCount, meshProperties.m_uiMaxThreadGroupTotalCount);

  return DrawMeshPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

xiiResult xiiGALCommandList::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  CountDispatchCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "Dispatch command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "Dispatch command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(m_pRenderPass == nullptr, "Dispatch command arguments are invalid. Dispatch command must be performed outside of render pass.");
  XII_VERIFY_COMMAND_LIST_RESULT(uiThreadGroupCountX != 0U && uiThreadGroupCountY != 0U && uiThreadGroupCountZ != 0U, "Dispatch command arguments are invalid. At least one of the thread group counts are zero, this is OK as the dispatch command will be ignored, but may be unintentional.");

  return DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

xiiResult xiiGALCommandList::DispatchIndirect(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  CountDispatchCall();

  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState != nullptr, "DispatchIndirect command arguments are invalid. No pipeline state is bound.");
  XII_VERIFY_COMMAND_LIST_RESULT(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "DispatchIndirect command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pPipelineState->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(m_pRenderPass == nullptr, "DispatchIndirect command arguments are invalid. DispatchIndirect command must be performed outside of render pass.");

  XII_VERIFY_COMMAND_LIST_RESULT(pIndirectArgumentBuffer != nullptr, "The indirect arguments buffer is invalidated.");

  const auto& bufferDescription = pIndirectArgumentBuffer->GetDescription();

  XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", pIndirectArgumentBuffer->GetDebugName());

  const xiiUInt32 uiOffset = ((sizeof(xiiUInt32) * 3) + uiArgumentOffsetInBytes);
  XII_VERIFY_COMMAND_LIST_RESULT(uiOffset <= bufferDescription.m_uiSize, "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' offset in bytes must be at least {1} bytes.", pIndirectArgumentBuffer->GetDebugName());

  return DispatchIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

void xiiGALCommandList::BeginQuery(xiiSharedPtr<xiiGALQuery> pQuery)
{
  XII_VERIFY_COMMAND_LIST(pQuery != nullptr, "BeginQuery must not be called on an invalidated query.");

  const auto& queryDescription = pQuery->GetDescription();

  XII_VERIFY_COMMAND_LIST(queryDescription.m_Type != xiiGALQueryType::Timestamp, "BeginQuery cannot be called on timestamp queries. Use EndQuery instead to set the timestamp.");

  /// \todo GraphicsFoundation: Assert command queue compatibiliity.

  BeginQueryPlatform(pQuery);
}

void xiiGALCommandList::EndQuery(xiiSharedPtr<xiiGALQuery> pQuery)
{
  XII_VERIFY_COMMAND_LIST(pQuery != nullptr, "EndQuery must not be called on an invalidated query.");

  /// \todo GraphicsFoundation: Assert command queue compatibiliity.

  EndQueryPlatform(pQuery);
}

void xiiGALCommandList::TransitionResourceStates(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  if (pResourceBarriers.IsEmpty())
    return;

  TransitionResourceStatesPlatform(pResourceBarriers);
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

void xiiGALCommandList::UpdateBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
  const auto& graphicsAdapterProperties = m_pDevice->GetGraphicsDeviceAdapterProperties();

  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pBuffer != nullptr, "UpdateBuffer arguments are invalid. The buffer handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "UpdateBuffer command must be used outside of render pass.");

  const auto& bufferDescription = pBuffer->GetDescription();

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_CHECK_ALIGNMENT(pSourceData.GetPtr(), graphicsAdapterProperties.m_BufferProperties.m_uiConstantBufferAlignment);
  }
  if (bufferDescription.m_Mode == xiiGALBufferMode::Structured)
  {
    XII_VERIFY_COMMAND_LIST((uiDestinationOffset % graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment) == 0, "Offset must be aligned to {} bytes.", graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment);
  }

  XII_VERIFY_COMMAND_LIST(bufferDescription.m_Usage == xiiGALResourceUsage::Default || bufferDescription.m_Usage == xiiGALResourceUsage::Sparse, "UpdateBuffer command arguments are invalid. Only xiiGALResourceUsage::Default or xiiGALResourceUsage::Sparse may be updated with this method.");
  XII_VERIFY_COMMAND_LIST(uiDestinationOffset < bufferDescription.m_uiSize, "UpdateBuffer command arguments are invalid. Unable to update buffer '{0}', the destination offset ({1}) exceeds the buffer size ({2}).", pBuffer->GetDebugName(), uiDestinationOffset, bufferDescription.m_uiSize);
  XII_VERIFY_COMMAND_LIST((uiDestinationOffset + pSourceData.GetCount()) <= bufferDescription.m_uiSize, "UpdateBuffer command arguments are invalid. Unable to update buffer '{0}', the update region [{1}, {2}) is out of buffer bounds [0, {3}).", pBuffer->GetDebugName(), uiDestinationOffset, uiDestinationOffset + pSourceData.GetCount(), bufferDescription.m_uiSize);

  UpdateBufferPlatform(pBuffer, uiDestinationOffset, pSourceData);
}

void xiiGALCommandList::CopyBuffer(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pSourceBuffer != nullptr, "CopyBuffer arguments are invalid. The source buffer handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(pDestinationBuffer != nullptr, "CopyBuffer arguments are invalid. The destination buffer handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "CopyBuffer command must be used outside of render pass.");

  const auto& sourceBufferDescription      = pSourceBuffer->GetDescription();
  const auto& destinationBufferDescription = pDestinationBuffer->GetDescription();

  XII_VERIFY_COMMAND_LIST(sourceBufferDescription.m_uiSize <= destinationBufferDescription.m_uiSize, "CopyBuffer command arguments are invalid. The source buffer bounds exceeds the bounds of the destination buffer.");

  CopyBufferPlatform(pSourceBuffer, pDestinationBuffer);
}

void xiiGALCommandList::CopyBufferRegion(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pSourceBuffer != nullptr, "CopyBufferRegion arguments are invalid. The source buffer handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(pDestinationBuffer != nullptr, "CopyBufferRegion arguments are invalid. The destination buffer handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "CopyBufferRegion command must be used outside of render pass.");

  const auto& sourceBufferDescription      = pSourceBuffer->GetDescription();
  const auto& destinationBufferDescription = pDestinationBuffer->GetDescription();

  XII_VERIFY_COMMAND_LIST((uiSourceOffset + uiSize) <= sourceBufferDescription.m_uiSize, "CopyBufferRegion command arguments are invalid. Failed to copy buffer '{0}' to '{1}', the destination range [{2}, {3}) is out of buffer bounds [0, {4}).", pSourceBuffer->GetDebugName(), pDestinationBuffer->GetDebugName(), uiSourceOffset, uiSourceOffset + uiSize, sourceBufferDescription.m_uiSize);
  XII_VERIFY_COMMAND_LIST((uiDestinationOffset + uiSize) <= destinationBufferDescription.m_uiSize, "CopyBufferRegion command arguments are invalid. Failed to copy buffer '{0}' to '{1}', the destination range [{2}, {3}) is out of buffer bounds [0, {4}).", pSourceBuffer->GetDebugName(), pDestinationBuffer->GetDebugName(), uiDestinationOffset, uiDestinationOffset + uiSize, destinationBufferDescription.m_uiSize);

  CopyBufferPlatform(pSourceBuffer, pDestinationBuffer);
}

xiiResult xiiGALCommandList::MapBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  XII_VERIFY_COMMAND_LIST_RESULT(pBuffer != nullptr, "MapBuffer arguments are invalid. The buffer handle has been invalidated.");

  const auto& bufferDescription = pBuffer->GetDescription();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_VERIFY_COMMAND_LIST_RESULT(!m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has already been mapped.");
  m_MappedBuffers.Insert(pBuffer, mapType);
#endif

  pMappedData = nullptr;
  switch (mapType)
  {
    case xiiGALMapType::Read:
    {
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for reading.");
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Buffer being mapped for reading was not created with the xiiGALCPUAccessFlag::Read flag.");
      XII_VERIFY_COMMAND_LIST_RESULT(!mapFlags.IsSet(xiiGALMapFlags::Discard), "xiiGALMapFlags::Discard is not a valid map flag when mapping a buffer for reading.");
    }
    break;
    case xiiGALMapType::Write:
    {
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic || bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Dynamic or xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for writing.");
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Buffer being mapped for reading was not created with the xiiGALCPUAccessFlag::Write flag.");
    }
    break;
    case xiiGALMapType::ReadWrite:
    {
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for reading and writing.");
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Buffer being mapped for reading and writing was not created with the xiiGALCPUAccessFlag::Read flag.");
      XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Buffer being mapped for reading and writing was not created with the xiiGALCPUAccessFlag::Write flag.");
      XII_VERIFY_COMMAND_LIST_RESULT(!mapFlags.IsSet(xiiGALMapFlags::Discard), "xiiGALMapFlags::Discard is not a valid map flag when mapping a buffer for reading and writing.");
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    XII_VERIFY_COMMAND_LIST_RESULT(mapFlags.IsAnySet(xiiGALMapFlags::Discard | xiiGALMapFlags::NoOverWrite) && mapType == xiiGALMapType::Write, "Dynamic buffers can only be mapped for writing with the xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite flag.");
    XII_VERIFY_COMMAND_LIST_RESULT((mapFlags.IsStrictlyAnySet(xiiGALMapFlags::Discard) || mapFlags.IsStrictlyAnySet(xiiGALMapFlags::NoOverWrite)), "Dynamic buffers can only be mapped for writing with the xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite flag.");
  }

  if (mapFlags.IsSet(xiiGALMapFlags::Discard))
  {
    XII_VERIFY_COMMAND_LIST_RESULT(bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic || bufferDescription.m_Usage == xiiGALResourceUsage::Staging, "Only buffers with xiiGALResourceUsage::Dynamic or xiiGALResourceUsage::Staging can be mapped with the xiiGALMapFlags::Discard flag.");
    XII_VERIFY_COMMAND_LIST_RESULT(mapType == xiiGALMapType::Write, "xiiGALMapType::Write is only valid when mapping buffer for writing.");
  }

  if (MapBufferPlatform(pBuffer, mapType, mapFlags, pMappedData).Failed())
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    XII_VERIFY_COMMAND_LIST_RESULT(m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has not been mapped.", pBuffer->GetDebugName());
    XII_VERIFY_COMMAND_LIST_RESULT(*m_MappedBuffers.GetValue(pBuffer) == mapType, "The map type ({0}) does not match the map type ({1}) that was used to map the buffer.", mapType, *m_MappedBuffers.GetValue(pBuffer));

    m_MappedBuffers.Remove(pBuffer);
#endif
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALCommandList::UnmapBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  XII_VERIFY_COMMAND_LIST_RESULT(pBuffer != nullptr, "MapBuffer arguments are invalid. The buffer handle has been invalidated.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_VERIFY_COMMAND_LIST_RESULT(m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has not been mapped.", pBuffer->GetDebugName());
  XII_VERIFY_COMMAND_LIST_RESULT(*m_MappedBuffers.GetValue(pBuffer) == mapType, "The map type ({0}) does not match the map type ({1}) that was used to map the buffer.", mapType, *m_MappedBuffers.GetValue(pBuffer));

  m_MappedBuffers.Remove(pBuffer);
#endif

  return UnmapBufferPlatform(pBuffer, mapType);
}

void xiiGALCommandList::UpdateTexture(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pTexture != nullptr, "UpdateTexture arguments are invalid. The texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "UpdateTexture command must be used outside of render pass.");

  ValidateTextureUpdateRegion(pTexture->GetDescription(), textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureBox, subresourceData);

  UpdateTexturePlatform(pTexture, textureMiplevelData, textureBox, subresourceData);
}

void xiiGALCommandList::CopyTexture(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pSourceTexture != nullptr, "CopyTexture arguments are invalid. The source texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(pDestinationTexture != nullptr, "CopyTexture arguments are invalid. The destination texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "CopyTexture command must be used outside of render pass.");

  xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(pSourceTexture->GetDescription(), 0);
  xiiBoundingBoxU32        sourceBox          = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), xiiVec3U32(mipLevelProperties.m_LogicalSize.width, mipLevelProperties.m_LogicalSize.height, mipLevelProperties.m_uiDepth));

  ValidateTextureRegion(pSourceTexture->GetDescription(), 0, 0, sourceBox);
  ValidateTextureRegion(pDestinationTexture->GetDescription(), 0, 0, sourceBox);

  CopyTexturePlatform(pSourceTexture, pDestinationTexture);
}

void xiiGALCommandList::CopyTextureRegion(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Transfer), "The command list does not have the xiiGALCommandQueueType::Transfer flag.");
  XII_VERIFY_COMMAND_LIST(pSourceTexture != nullptr, "CopyTextureRegion arguments are invalid. The source texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(pDestinationTexture != nullptr, "CopyTextureRegion arguments are invalid. The destination texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "CopyTextureRegion command must be used outside of render pass.");

  ValidateTextureRegion(pSourceTexture->GetDescription(), destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, box);

  xiiBoundingBoxU32 destinationBox = xiiBoundingBoxU32::MakeFromMinMax(vDestinationPoint, vDestinationPoint + box.GetExtents());

  ValidateTextureRegion(pDestinationTexture->GetDescription(), destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, destinationBox);

  CopyTextureRegionPlatform(pSourceTexture, sourceMipLevelData, box, pDestinationTexture, destinationMipLevelData, vDestinationPoint);
}

void xiiGALCommandList::ResolveTextureSubResource(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(pSourceTexture != nullptr, "ResolveTextureSubResource arguments are invalid. The source texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(pDestinationTexture != nullptr, "ResolveTextureSubResource arguments are invalid. The destination texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "ResolveTextureSubResource command must be used outside of render pass.");

  /// \todo GraphicsFoundation: Validate resolve texture parameters.

  ResolveTextureSubResourcePlatform(pSourceTexture, sourceMipLevelData, pDestinationTexture, destinationMipLevelData);
}

void xiiGALCommandList::GenerateMips(xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_VERIFY_COMMAND_LIST(m_Description.m_QueueType.IsSet(xiiGALCommandQueueType::Graphics), "The command list does not have the xiiGALCommandQueueType::Graphics flag.");
  XII_VERIFY_COMMAND_LIST(pTextureView != nullptr, "GenerateMips arguments are invalid. The texture view handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "GenerateMips command must be used outside of render pass.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const auto& textureViewDescription = pTextureView->GetDescription();

  XII_VERIFY_COMMAND_LIST(textureViewDescription.m_ViewType == xiiGALTextureViewType::ShaderResource, "GenerateMips arguments are invalid. Texture view '{0}' is not of type xiiGALTextureViewType::ShaderResource.", pTextureView->GetDebugName());
  XII_VERIFY_COMMAND_LIST(textureViewDescription.m_Flags.IsSet(xiiGALTextureViewFlags::AllowMipGeneration), "GenerateMips arguments are invalid. Texture view '{0}' does not have xiiGALTextureViewFlags::AllowMipGeneration flag.", pTextureView->GetDebugName());
#endif

  GenerateMipsPlatform(pTextureView);
}

xiiResult xiiGALCommandList::MapTextureSubresource(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  XII_VERIFY_COMMAND_LIST_RESULT(pTexture != nullptr, "MapTextureSubresource arguments are invalid. The texture handle has been invalidated.");

  const auto& textureDescription = pTexture->GetDescription();

  XII_VERIFY_COMMAND_LIST_RESULT(textureMipLevelData.m_uiMipLevel < textureDescription.m_uiMipLevels, "Mip level ({}) is out of permitted range [0, {}].", textureMipLevelData.m_uiMipLevel, textureDescription.m_uiMipLevels - 1);

  if (textureDescription.IsArray())
  {
    XII_VERIFY_COMMAND_LIST_RESULT(textureMipLevelData.m_uiArraySlice < textureDescription.GetArraySize(), "Array slice ({}) is out of permitted range [0, {}].", textureMipLevelData.m_uiArraySlice, textureDescription.GetArraySize() - 1);
  }
  else
  {
    XII_VERIFY_COMMAND_LIST_RESULT(textureMipLevelData.m_uiArraySlice == 0, "Array slice ({}) must be 0 for non-array textures.", textureMipLevelData.m_uiArraySlice);
  }

  if (pTextureBox != nullptr)
  {
    ValidateTextureRegion(textureDescription, textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, *pTextureBox);
  }

  return MapTextureSubresourcePlatform(pTexture, textureMipLevelData, mapType, mapFlags, pTextureBox, mappedData);
}

xiiResult xiiGALCommandList::UnmapTextureSubresource(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  XII_VERIFY_COMMAND_LIST_RESULT(pTexture != nullptr, "MapTextureSubresource arguments are invalid. The texture handle has been invalidated.");
  XII_VERIFY_COMMAND_LIST_RESULT(textureMipLevelData.m_uiMipLevel < pTexture->GetDescription().m_uiMipLevels, "MapTextureSubresource arguments are invalid. The mip level is out of range.");
  XII_VERIFY_COMMAND_LIST_RESULT(textureMipLevelData.m_uiArraySlice < pTexture->GetDescription().GetArraySize(), "MapTextureSubresource arguments are invalid. The array slice is out of range.");

  return UnmapTextureSubresourcePlatform(pTexture, textureMipLevelData);
}

void xiiGALCommandList::InvalidateState()
{
  XII_VERIFY_COMMAND_LIST(m_pRenderPass == nullptr, "Invalidating the command list is disallowed while a render pass is active. Call EndRenderPass to finish the pass.");

  m_pPipelineState             = nullptr;
  m_pPipelineResourceSignature = nullptr;

  m_VertexBuffers.Clear();
  m_VertexBuffersOffsets.Clear();

  m_pIndexBuffer      = nullptr;
  m_uiIndexDataOffset = 0;

  m_pRenderPass  = nullptr;
  m_pFramebuffer = nullptr;

  m_BlendFactors = xiiColor::Black;
  m_uiStencilRef = 0;

  m_Viewports.Clear();
  m_ScissorRects.Clear();

  ClearStatisticCounters();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_MappedBuffers.IsEmpty(), "Mapped buffers have not yet been released.");
#endif

  InvalidateStatePlatform();
}

bool xiiGALCommandList::VerifyResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags, xiiBitflags<xiiGALCommandQueueType> queueType, const char* szParameterName) const
{
  bool bResult = true;
  for (auto state : stateFlags)
  {
    switch (state)
    {
      case xiiGALResourceStateFlags::Undefined:
      case xiiGALResourceStateFlags::CopySource:
      case xiiGALResourceStateFlags::CopyDestination:
      case xiiGALResourceStateFlags::Common:
      {
        if (!queueType.IsSet(xiiGALCommandQueueType::Transfer))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, queueType.GetValue());
        }
      }
      break;
      case xiiGALResourceStateFlags::ConstantBuffer:
      case xiiGALResourceStateFlags::UnorderedAccess:
      case xiiGALResourceStateFlags::ShaderResource:
      case xiiGALResourceStateFlags::IndirectArgument:
      case xiiGALResourceStateFlags::BuildASRead:
      case xiiGALResourceStateFlags::BuildASWrite:
      case xiiGALResourceStateFlags::RayTracing:
      {
        if (!queueType.IsSet(xiiGALCommandQueueType::Compute))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, queueType.GetValue());
        }
      }
      break;
      case xiiGALResourceStateFlags::VertexBuffer:
      case xiiGALResourceStateFlags::IndexBuffer:
      case xiiGALResourceStateFlags::RenderTarget:
      case xiiGALResourceStateFlags::DepthWrite:
      case xiiGALResourceStateFlags::DepthRead:
      case xiiGALResourceStateFlags::StreamOut:
      case xiiGALResourceStateFlags::ResolveSource:
      case xiiGALResourceStateFlags::ResolveDestination:
      case xiiGALResourceStateFlags::InputAttachment:
      case xiiGALResourceStateFlags::Present:
      case xiiGALResourceStateFlags::ShadingRate:
      {
        if (!queueType.IsSet(xiiGALCommandQueueType::Graphics))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, queueType.GetValue());
        }
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
  return bResult;
}

bool xiiGALCommandList::VerifyResourceStates(xiiBitflags<xiiGALResourceStateFlags> stateFlags, bool bIsTexture) const
{
#define XII_VERIFY_EXCLUSIVE_STATE(exclusiveState)                                                                                                       \
  if (!stateFlags.IsStrictlyAnySet((xiiGALResourceStateFlags::exclusiveState)))                                                                          \
  {                                                                                                                                                      \
    xiiLog::Error("State {} is invalid: {} can not be combined with any other state.", stateFlags.GetValue(), xiiGALResourceStateFlags::exclusiveState); \
  }

  XII_VERIFY_EXCLUSIVE_STATE(Common);
  XII_VERIFY_EXCLUSIVE_STATE(Undefined);
  XII_VERIFY_EXCLUSIVE_STATE(UnorderedAccess);
  XII_VERIFY_EXCLUSIVE_STATE(RenderTarget);
  XII_VERIFY_EXCLUSIVE_STATE(DepthWrite);
  XII_VERIFY_EXCLUSIVE_STATE(CopyDestination);
  XII_VERIFY_EXCLUSIVE_STATE(ResolveDestination);
  XII_VERIFY_EXCLUSIVE_STATE(Present);
  XII_VERIFY_EXCLUSIVE_STATE(BuildASWrite);
  XII_VERIFY_EXCLUSIVE_STATE(RayTracing);
  XII_VERIFY_EXCLUSIVE_STATE(ShadingRate);

#undef XII_VERIFY_EXCLUSIVE_STATE

  if (bIsTexture)
  {
    if (stateFlags.IsAnySet(xiiGALResourceStateFlags::VertexBuffer | xiiGALResourceStateFlags::ConstantBuffer | xiiGALResourceStateFlags::IndexBuffer | xiiGALResourceStateFlags::StreamOut | xiiGALResourceStateFlags::IndirectArgument))
    {
      xiiLog::Error("State {} is invalid: states xiiGALResourceStateFlags::VertexBuffer, xiiGALResourceStateFlags::ConstantBuffer, xiiGALResourceStateFlags::IndexBuffer, xiiGALResourceStateFlags::StreamOut, xiiGALResourceStateFlags::IndirectArgument are not applicable to textures.", stateFlags.GetValue());
      return false;
    }
  }
  else
  {
    if (stateFlags.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::Present | xiiGALResourceStateFlags::ShadingRate | xiiGALResourceStateFlags::InputAttachment))
    {
      xiiLog::Error("State {} is invalid: states xiiGALResourceStateFlags::RenderTarget, xiiGALResourceStateFlags::DepthWrite, xiiGALResourceStateFlags::DepthRead, xiiGALResourceStateFlags::ResolveSource, xiiGALResourceStateFlags::ResolveDestination, xiiGALResourceStateFlags::Present, xiiGALResourceStateFlags::ShadingRate, xiiGALResourceStateFlags::InputAttachment are not applicable to buffers.", stateFlags.GetValue());
      return false;
    }
  }

  return true;
}

bool xiiGALCommandList::VerifyAliasingBarrierDescription(const xiiGALStateTransitionDescription& description) const
{
  XII_VERIFY_COMMAND_LIST_BOOL(description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::Aliasing), "The transition description does not have the aliasing flag.");

  auto VerifySparseAliasedResource = [](xiiGALResource* pResource) -> xiiGALResourceDimension::Enum {
    if (pResource == nullptr)
      return xiiGALResourceDimension::Undefined;

    if (xiiGALTexture* pTexture = xiiDynamicCast<xiiGALTexture*>(pResource))
    {
      const auto& textureDescription = pTexture->GetDescription();

      XII_ASSERT_DEV(textureDescription.m_Usage == xiiGALResourceUsage::Sparse, "Texture '{}' used in aliasing barrier is not a sparse resource.", pTexture->GetDebugName());
      XII_ASSERT_DEV(textureDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::SparseAlias), "Texture '{}' used in aliasing barrier was not created with xiiGALMiscTextureFlags::SparseAlias flag.", pTexture->GetDebugName());

      return textureDescription.m_Type;
    }
    else if (xiiGALBuffer* pBuffer = xiiDynamicCast<xiiGALBuffer*>(pResource))
    {
      const auto& bufferDescription = pBuffer->GetDescription();

      XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Sparse, "Buffer '{}' used in aliasing barrier is not a sparse resource.", pBuffer->GetDebugName());
      XII_ASSERT_DEV(bufferDescription.m_MiscFlags.IsSet(xiiGALMiscBufferFlags::SparseAlias), "Buffer '{}' used in aliasing barrier was not created with xiiGALMiscBufferFlags::SparseAlias flag.", pBuffer->GetDebugName());

      return xiiGALResourceDimension::Buffer;
    }
    else
    {
      XII_ASSERT_DEV(false, "Only textures and buffers are permitted in aliasing barriers.");
      return xiiGALResourceDimension::Undefined;
    }
  };

  xiiGALResourceDimension::Enum previousDimension = VerifySparseAliasedResource(description.m_pPreviousResource.Borrow());
  xiiGALResourceDimension::Enum currentDimension  = VerifySparseAliasedResource(description.m_pResource.Borrow());
  if (previousDimension != xiiGALResourceDimension::Undefined && currentDimension != xiiGALResourceDimension::Undefined)
  {
    XII_ASSERT_DEV((previousDimension == xiiGALResourceDimension::Buffer) == (currentDimension == xiiGALResourceDimension::Buffer), "Both previous- and current-resources must either be buffers or textures. Sparse aliasing between textures and buffers are not permitted.");
  }

  XII_ASSERT_DEV(description.m_OldState == xiiGALResourceStateFlags::Unknown && description.m_NewState == xiiGALResourceStateFlags::Unknown, "Aliasing buffer is applied to all subresource. OldState and NewState must be xiiGALResourceStateFlags::Unknown.");
  XII_ASSERT_DEV(description.m_uiFirstArraySlice == 0 && description.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS && description.m_uiFirstArraySlice == 0 && description.m_uiArraySliceCount == XII_GAL_REMAINING_ARRAY_SLICES, "Aliasing barrier is applied to all subresources. FirstMipLevel, MipLevelCount, FirstArraySlice, ArraySliceCount must be set as default.");

  return true;
}

#undef XII_VERIFY_COMMAND_LIST_BOOL
#undef XII_VERIFY_COMMAND_LIST_RESULT
#undef XII_VERIFY_COMMAND_LIST

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);
