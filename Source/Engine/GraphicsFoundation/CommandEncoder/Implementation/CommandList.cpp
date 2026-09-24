/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Utilities/Stats.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/BottomLevelAS.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Query.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALCommandListFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALCommandListFlags::Secondary),
  XII_BITFLAGS_CONSTANT(xiiGALCommandListFlags::MultiSubmit),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALSetVertexBufferFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALSetVertexBufferFlags::Reset),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALStateTransitionType, 1)
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::Immediate),
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::Begin),
  XII_ENUM_CONSTANT(xiiGALStateTransitionType::End),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALStateTransitionFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::UpdateState),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::DiscardContent),
  XII_BITFLAGS_CONSTANT(xiiGALStateTransitionFlags::Aliasing),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALStateTransitionMode, 1)
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::None),
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::Transition),
  XII_ENUM_CONSTANT(xiiGALStateTransitionMode::Verify),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALASCopyMode, 1)
  XII_ENUM_CONSTANT(xiiGALASCopyMode::Clone),
  XII_ENUM_CONSTANT(xiiGALASCopyMode::Compact),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandList, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  XII_FORCE_INLINE xiiUInt32 GetPrimitiveCount(xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiElements)
  {
    if (topology >= xiiGALPrimitiveTopology::ControlPointPatchList1 && topology <= xiiGALPrimitiveTopology::ControlPointPatchList32)
    {
      return uiElements / (topology - xiiGALPrimitiveTopology::ControlPointPatchList1 + 1);
    }
    else
    {
      switch (topology)
      {
        case xiiGALPrimitiveTopology::Undefined:
          XII_REPORT_FAILURE("Undefined primitive topology.");
          return 0;

        case xiiGALPrimitiveTopology::TriangleList:
          return uiElements / 3;
        case xiiGALPrimitiveTopology::TriangleStrip:
          return xiiMath::Max(uiElements, 2U) - 2;
        case xiiGALPrimitiveTopology::PointList:
          return uiElements;
        case xiiGALPrimitiveTopology::LineList:
          return uiElements / 2;
        case xiiGALPrimitiveTopology::LineStrip:
          return xiiMath::Max(uiElements, 1U) - 1;
        case xiiGALPrimitiveTopology::TriangleListAdjacent:
          return uiElements / 6;
        case xiiGALPrimitiveTopology::TriangleStripAdjacent:
          return xiiMath::Max(uiElements, 4U) - 4;
        case xiiGALPrimitiveTopology::LineListAdjacent:
          return uiElements / 4;
        case xiiGALPrimitiveTopology::LineStripAdjacent:
          return xiiMath::Max(uiElements, 3U) - 3;

        default: XII_REPORT_FAILURE("Unexpected primitive topology"); return 0;
      }
    }
  }
} // namespace

xiiGALCommandList::xiiGALCommandList(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription), m_bNativeMultiDrawSupported{m_pDevice->GetGraphicsDeviceAdapterProperties().m_Features.m_NativeMultiDraw != xiiGALDeviceFeatureState::Disabled}
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

void xiiGALCommandList::ValidateTextureRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& box) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(uiMipLevel < textureDescription.m_uiMipLevels, "Mip level ({}) is out of permitted range [0, {}].", uiMipLevel, textureDescription.m_uiMipLevels - 1);
  XII_ASSERT_DEV(box.IsValid(), "Invalid box range provided.");

  if (textureDescription.IsArray())
  {
    XII_ASSERT_DEV(uiSlice < textureDescription.m_uiArraySizeOrDepth, "Array slice ({}) is out of permitted range [0, {}].", textureDescription.m_uiArraySizeOrDepth - 1);
  }
  else
  {
    XII_ASSERT_DEV(uiSlice == 0, "Array slice ({}) must be 0 for non-array textures.", uiSlice);
  }

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

  xiiUInt32 uiMipWidth = xiiMath::Max(textureDescription.m_Size.width >> uiMipLevel, 1U);

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    const xiiUInt32 uiBlockAlignedMipWidth = (uiMipWidth + (formatProperties.m_uiBlockWidth - 1)) & ~(formatProperties.m_uiBlockWidth - 1);

    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "The block width must be a power of 2.");
    XII_ASSERT_DEV(box.m_vMax.x <= uiBlockAlignedMipWidth, "Region max X coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.x, uiBlockAlignedMipWidth);
    XII_ASSERT_DEV((box.m_vMin.x % formatProperties.m_uiBlockWidth) == 0, "For compressed formats, the region min X coordinate ({}) must be a multiple of the block width ({}).", box.m_vMin.x, formatProperties.m_uiBlockWidth);
    XII_ASSERT_DEV((box.m_vMax.x % formatProperties.m_uiBlockWidth) == 0 || box.m_vMax.x == uiMipWidth, "For compressed formats, the region max X coordinate ({}) must be a multiple of the block width ({}) or equal to the mip level ({}).", box.m_vMax.x, formatProperties.m_uiBlockWidth, uiMipWidth);
  }
  else
  {
    XII_ASSERT_DEV(box.m_vMax.x <= uiMipWidth, "Region max X coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.x, uiMipWidth);
  }

  if (textureDescription.m_Type != xiiGALResourceDimension::Texture1D && textureDescription.m_Type != xiiGALResourceDimension::Texture1DArray)
  {
    const xiiUInt32 uiMipHeight = xiiMath::Max(textureDescription.m_Size.height >> uiMipLevel, 1U);

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
    {
      XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "The block height must be a power of 2.");

      const xiiUInt32 uiBlockAlignedMipHeight = (uiMipHeight + (formatProperties.m_uiBlockHeight - 1)) & ~(formatProperties.m_uiBlockHeight - 1);

      XII_ASSERT_DEV(box.m_vMax.y <= uiBlockAlignedMipHeight, "Region max Y coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.y, uiBlockAlignedMipHeight);
      XII_ASSERT_DEV((box.m_vMin.y % formatProperties.m_uiBlockHeight) == 0U, "For compressed formats, the region min Y coordinate ({}) must be a multiple of block height ({}).", box.m_vMin.y, formatProperties.m_uiBlockHeight);
      XII_ASSERT_DEV((box.m_vMax.y % formatProperties.m_uiBlockHeight) == 0U || box.m_vMax.y == uiMipHeight, "For compressed formats, the region max Y coordinate ({}) must be a multiple of block height ({}) or equal the mip level height.", box.m_vMax.y, formatProperties.m_uiBlockHeight, uiMipHeight);
    }
    else
    {
      XII_ASSERT_DEV(box.m_vMax.y <= uiMipHeight, "Region max Y coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.y, uiMipHeight);
    }
  }

  if (textureDescription.m_Type == xiiGALResourceDimension::Texture3D)
  {
    const xiiUInt32 uiMipDepth = xiiMath::Max(textureDescription.m_uiArraySizeOrDepth >> uiMipLevel, 1U);

    XII_ASSERT_DEV(box.m_vMax.z <= uiMipDepth, "Region max Z coordinate ({}) is out of permitted range [0, {}].", box.m_vMax.z, uiMipDepth);
  }
  else
  {
    XII_ASSERT_DEV(box.m_vMin.z == 0, "Region min Z ({}) must be 0 for all but 3D textures.", box.m_vMin.z);
    XII_ASSERT_DEV(box.m_vMax.z == 1, "Region max Z ({}) must be 1 for all but 3D textures.", box.m_vMax.z);
  }
#else
  XII_IGNORE_UNUSED(textureDescription);
  XII_IGNORE_UNUSED(uiMipLevel);
  XII_IGNORE_UNUSED(uiSlice);
  XII_IGNORE_UNUSED(box);
#endif
}

void xiiGALCommandList::ValidateTextureUpdateRegion(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel, xiiUInt32 uiSlice, const xiiBoundingBoxU32& destinationBox, const xiiGALTextureSubResourceData& subresourceData) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(!subresourceData.m_pData.IsEmpty(), "CPU data pointer must not be empty.");

  ValidateTextureRegion(textureDescription, uiMipLevel, uiSlice, destinationBox);

  XII_ASSERT_DEV(textureDescription.m_uiSampleCount == 1, "Only non-multisampled textures can be updated UpdateTexture().");
  XII_ASSERT_DEV(xiiMemoryUtils::IsSizeAligned(subresourceData.m_uiStride, 16ULL), "Texture data stride ({}) must be at least 16-bit aligned.", subresourceData.m_uiStride);
  XII_ASSERT_DEV(xiiMemoryUtils::IsSizeAligned(subresourceData.m_uiDepthStride, 16ULL), "Texture data depth stride ({}) must be at least 16-bit aligned.", subresourceData.m_uiDepthStride);

  xiiVec3U32                             vUpdateRegion    = destinationBox.GetExtents();
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);
  xiiUInt32                              uiRowSize        = 0;
  xiiUInt32                              uiRowCount       = 0;

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    // Align update region size by the block size. This is only necessary when updating coarse mip levels. Otherwise, update region Width/Height should be multiples of the block size.
    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "");
    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "");

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

  XII_ASSERT_DEV(subresourceData.m_uiStride >= uiRowSize, "Source data stride ({}) is below the image row size ({}).", subresourceData.m_uiStride, uiRowSize);

  const xiiUInt64 uiPlaneSize = subresourceData.m_uiStride * uiRowCount;

  XII_ASSERT_DEV(vUpdateRegion.z == 1U || subresourceData.m_uiDepthStride >= uiPlaneSize, "Source data depth stride ({}) is below the image plane size ({}).", uiPlaneSize);
#else
  XII_IGNORE_UNUSED(textureDescription);
  XII_IGNORE_UNUSED(uiMipLevel);
  XII_IGNORE_UNUSED(uiSlice);
  XII_IGNORE_UNUSED(destinationBox);
  XII_IGNORE_UNUSED(subresourceData);
#endif
}

void xiiGALCommandList::Begin()
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Reset, "The command list has not been reset.");

  BeginPlatform();
}

void xiiGALCommandList::End()
{
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "The current active render pass has not been ended.");
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "The command list has not begun.");

  EndPlatform();
}

void xiiGALCommandList::Reset()
{
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "The current active render pass has not been ended.");

  if (m_RecordingState == RecordingState::Recording)
  {
    End();
  }

  InvalidateState();

  if (m_RecordingState != RecordingState::Reset)
  {
    ResetPlatform();
  }
}

void xiiGALCommandList::Submit(xiiGALCommandList* pSecondaryCommandList)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const char* szRecordingEnumToString[] = {
    "Reset",
    "Recording",
    "Ended",
  };

  XII_ASSERT_DEV(pSecondaryCommandList != nullptr, "xiiGALCommandList::Submit(): secondary list pointer is null.");

  const xiiGALCommandListCreationDescription& description  = pSecondaryCommandList->GetDescription();
  const bool                                  bIsSecondary = description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary);
  const bool                                  bIsPrimary   = m_Description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary);

  XII_ASSERT_DEV(bIsSecondary, "Submit(): provided list must be flagged Secondary (got Flags={0}).", xiiArgEnum(description.m_Flags));
  XII_ASSERT_DEV(!bIsPrimary, "Submit(): cannot inject a secondary into another secondary; primary required.");
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "Submit(): primary command list must be Recording (current state={0}).", szRecordingEnumToString[static_cast<xiiUInt8>(m_RecordingState)]);
  XII_ASSERT_DEV(pSecondaryCommandList->GetRecordingState() == xiiGALCommandList::RecordingState::Ended, "Submit(): secondary command list must have been Ended before submission (current state={0}).", szRecordingEnumToString[static_cast<xiiUInt8>(pSecondaryCommandList->GetRecordingState())]);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSubmit;

  return SubmitPlatform(pSecondaryCommandList);
}

void xiiGALCommandList::SetPipelineState(xiiGALPipelineState* pPipelineState)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetPipelineState must be called while recording.");

  if (m_pPipelineState == pPipelineState)
    return;

  m_pPipelineState             = pPipelineState;
  m_pPipelineResourceSignature = nullptr;

  if (m_pPipelineState != nullptr)
  {
    m_pPipelineResourceSignature = pPipelineState->GetDescription().m_pPipelineResourceSignature.Borrow();
  }
  else
  {
    m_pPipelineState             = nullptr;
    m_pPipelineResourceSignature = nullptr;
  }

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSetPipelineState;

  SetPipelineStatePlatform(pPipelineState);
}

void xiiGALCommandList::PushConstants(xiiUInt32 uiOffset, xiiArrayPtr<xiiUInt8> pData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "PushConstants must be called while recording.");
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "PushConstants arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "PushConstants: No pipeline resource signature set. A pipeline state with a valid pipeline resource signature must be set before push constants can be used.");
  XII_ASSERT_DEV(!pData.IsEmpty(), "PushConstants: pData must not be null");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    const xiiGALGraphicsDeviceAdapterDescription& graphicsAdapterProperties = m_pDevice->GetGraphicsDeviceAdapterProperties();

    XII_ASSERT_DEV(uiOffset < graphicsAdapterProperties.m_DeviceLimits.m_uiMaxPushConstantsSize, "PushConstants: Offset ({}) exceeds device limit of {} bytes.", uiOffset, graphicsAdapterProperties.m_DeviceLimits.m_uiMaxPushConstantsSize);
  }
#endif

  if (m_PushConstantStaging.GetCount() < uiOffset + pData.GetCount())
  {
    m_PushConstantStaging.SetCount(uiOffset + pData.GetCount());
  }
  xiiMemoryUtils::Copy(m_PushConstantStaging.GetData() + uiOffset, pData.GetPtr(), pData.GetCount());

  PushConstantsPlatform(uiOffset, m_PushConstantStaging.GetArrayPtr().GetSubArray(uiOffset, pData.GetCount()));
}

void xiiGALCommandList::SetStencilRef(xiiUInt32 uiStencilRef)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetStencilRef arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetStencilRef must be called while recording.");

  if (m_uiStencilRef != uiStencilRef)
  {
    m_uiStencilRef = uiStencilRef;

    ++m_CommandListStatistics.m_CommandListCounters.m_uiSetStencilRef;

    SetStencilRefPlatform(m_uiStencilRef);
  }
}

void xiiGALCommandList::SetBlendFactor(const xiiColor& blendFactor)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetBlendFactor arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetBlendFactor must be called while recording.");

  if (blendFactor != m_BlendFactors)
  {
    m_BlendFactors = blendFactor;

    ++m_CommandListStatistics.m_CommandListCounters.m_uiSetBlendFactors;

    SetBlendFactorPlatform(m_BlendFactors);
  }
}

void xiiGALCommandList::SetViewports(xiiArrayPtr<const xiiGALViewport> pViewports)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetViewports must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGALDeviceLimits&   deviceLimits   = m_pDevice->GetLimits();
  const xiiGALDeviceFeatures& deviceFeatures = m_pDevice->GetFeatures();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetViewports arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pViewports.GetCount() < deviceLimits.m_uiMaxViewports, "The number of viewports ({0}) exceeds the maximum viewport count ({1}).", pViewports.GetCount(), deviceLimits.m_uiMaxViewports);
  XII_ASSERT_DEV(pViewports.GetCount() <= 1U || deviceFeatures.m_MultiViewport == xiiGALDeviceFeatureState::Enabled, "SetViewports arguments are invalid. The device does does not have the Multi-Viewport feature enabled.");

  for (xiiUInt32 i = 0; i < pViewports.GetCount(); ++i)
  {
    const xiiGALViewport& viewport = pViewports[i];

    XII_ASSERT_DEV(viewport.m_fWidth >= 0.0f, "SetViewports arguments are invalid. Incorrect viewport width ({0}) for index {1}.", viewport.m_fWidth, i);
    XII_ASSERT_DEV(viewport.m_fHeight >= 0.0f, "SetViewports arguments are invalid. Incorrect viewport height ({0}) for index {1}.", viewport.m_fHeight, i);
    XII_ASSERT_DEV(viewport.m_fMaxDepth >= viewport.m_fMinDepth, "SetViewports arguments are invalid. Incorrect viewport depth range [{0}, {1}] for index {2}.", viewport.m_fMinDepth, viewport.m_fMaxDepth, i);
  }
#endif

  m_Viewports.Clear();
  m_Viewports.PushBackRange(pViewports);

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSetViewports;

  SetViewportsPlatform(m_Viewports);
}

void xiiGALCommandList::SetScissorRects(xiiArrayPtr<const xiiRectU32> pRects)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetScissorRects must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGALDeviceLimits&   deviceLimits   = m_pDevice->GetLimits();
  const xiiGALDeviceFeatures& deviceFeatures = m_pDevice->GetFeatures();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetScissorRects arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pRects.GetCount() < deviceLimits.m_uiMaxViewports, "The number of scissor rects ({0}) exceeds the maximum scissor rect count ({1}).", pRects.GetCount(), deviceLimits.m_uiMaxViewports);
  XII_ASSERT_DEV(pRects.GetCount() <= 1U || deviceFeatures.m_MultiViewport == xiiGALDeviceFeatureState::Enabled, "SetScissorRects arguments are invalid. The device does does not have the Multi Viewport feature enabled.");
#endif

  m_ScissorRects.Clear();
  m_ScissorRects.PushBackRange(pRects);

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSetScissorRects;

  SetScissorRectsPlatform(m_ScissorRects);
}

void xiiGALCommandList::SetIndexBuffer(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset /*= 0U*/, xiiEnum<xiiGALStateTransitionMode> transitionMode /*= xiiGALStateTransitionMode::Transition*/)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetIndexBuffer must be called while recording.");

  if (m_pIndexBuffer == pIndexBuffer && m_uiIndexDataOffset == uiByteOffset)
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetIndexBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr || transitionMode != xiiGALStateTransitionMode::Transition, "Resource state transitions are not permitted inside a render pass and may result in an undefined behavior. Do not use xiiGALStateTransitionMode::Transition or end the render pass first.");

  if (pIndexBuffer)
  {
    const xiiGALBufferCreationDescription& bufferDescription = pIndexBuffer->GetDescription();

    XII_ASSERT_DEV(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndexBuffer), "SetIndexBuffer arguments are invalid. The Index buffer '{0}' was not created with the xiiGALBindFlags::IndexBuffer bind flag.", pIndexBuffer->GetDebugName());
  }
#endif

  m_pIndexBuffer      = pIndexBuffer;
  m_uiIndexDataOffset = uiByteOffset;

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSetIndexBuffer;

  SetIndexBufferPlatform(pIndexBuffer, m_uiIndexDataOffset, transitionMode);
}

void xiiGALCommandList::SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets /*= {}*/, xiiBitflags<xiiGALSetVertexBufferFlags> flags /*= xiiGALSetVertexBufferFlags::None*/, xiiEnum<xiiGALStateTransitionMode> transitionMode /*= xiiGALStateTransitionMode::Transition*/)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetVertexBuffers must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGALDeviceLimits& deviceLimits = m_pDevice->GetLimits();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "SetVertexBuffers arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(uiStartSlot < deviceLimits.m_uiMaxVertexBuffers, "SetVertexBuffers arguments are invalid. The start slot ({0}) is out of range [0, {1}].", uiStartSlot, deviceLimits.m_uiMaxVertexBuffers - 1);
  XII_ASSERT_DEV((uiStartSlot + pVertexBuffers.GetCount()) < deviceLimits.m_uiMaxVertexBuffers, "SetVertexBuffers arguments are invalid. The range of vertex buffer slots being set [{0}, {1}] is out of allowed range [0, {2}].", uiStartSlot, uiStartSlot + pVertexBuffers.GetCount() - 1U, deviceLimits.m_uiMaxVertexBuffers - 1U);
  XII_ASSERT_DEV(m_pRenderPass == nullptr || transitionMode != xiiGALStateTransitionMode::Transition, "Resource state transitions are not permitted inside a render pass and may result in an undefined behavior. Do not use xiiGALStateTransitionMode::Transition or end the render pass first.");
#endif

  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    // Reset only the buffer slots that are not being set.
    for (xiiUInt32 i = 0; i < uiStartSlot; ++i)
    {
      m_VertexStreams[i] = {};
    }
    for (xiiUInt32 i = uiStartSlot + pVertexBuffers.GetCount(); i < m_VertexStreams.GetCount(); ++i)
    {
      m_VertexStreams[i] = {};
    }
  }

  m_VertexStreams.SetCount(uiStartSlot + pVertexBuffers.GetCount());

  for (xiiUInt32 i = uiStartSlot; i < pVertexBuffers.GetCount(); ++i)
  {
    if (pVertexBuffers[i] != nullptr)
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      const xiiGALBufferCreationDescription& bufferDescription = pVertexBuffers[i]->GetDescription();

      XII_ASSERT_DEV(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::VertexBuffer), "SetVertexBuffer arguments are invalid. The Vertex buffer '{0}' was not created with the xiiGALBindFlags::VertexBuffer bind flag.", pVertexBuffers[i]->GetDebugName());
#endif

      m_VertexStreams[i].m_pBuffer  = pVertexBuffers[i];
      m_VertexStreams[i].m_uiOffset = i < pByteOffsets.GetCount() ? pByteOffsets[i] : 0U;
    }
  }

  ++m_CommandListStatistics.m_CommandListCounters.m_uiSetVertexBuffers;

  SetVertexBuffersPlatform(uiStartSlot, m_VertexStreams, flags, transitionMode);
}

void xiiGALCommandList::SetConstantBuffer(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetConstantBuffer must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetConstantBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetConstantBuffer requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The constant buffer resource '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_ASSERT_DEV(pConstantBuffer == nullptr || pConstantBuffer->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer), "Incorrect buffer bind flags. The buffer must be created with xiiGALBindFlags::UniformBuffer if not invalidated.");
#endif

  SetConstantBufferPlatform(bindingInformation, pConstantBuffer);
}

void xiiGALCommandList::SetShaderResourceBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetShaderResourceBufferView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetShaderResourceBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetShaderResourceBufferView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The buffer resource view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_ASSERT_DEV(pBufferView == nullptr || pBufferView->GetDescription().m_ViewType == xiiGALBufferViewType::ShaderResource, "Incorrect buffer view type. The view must be created with xiiGALBufferViewType::ShaderResource if not invalidated.");
#endif

  SetShaderResourceBufferViewPlatform(bindingInformation, pBufferView);
}

void xiiGALCommandList::SetShaderResourceBufferViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording && m_pPipelineState != nullptr, "Buffer descriptor arrays require an active recording and pipeline state.");
  XII_ASSERT_DEV(uiFirstElement + pBufferViews.GetCount() <= bindingInformation.m_uiArraySize, "Buffer descriptor write exceeds array '{}' capacity ({} + {} > {}).", bindingInformation.m_sName, uiFirstElement, pBufferViews.GetCount(), bindingInformation.m_uiArraySize);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiGALBufferView* pView : pBufferViews)
    XII_ASSERT_DEV(pView == nullptr || pView->GetDescription().m_ViewType == xiiGALBufferViewType::ShaderResource, "All buffer views must be shader-resource views.");
#endif
  SetShaderResourceBufferViewsPlatform(bindingInformation, uiFirstElement, pBufferViews);
}

void xiiGALCommandList::SetShaderResourceTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetShaderResourceTextureView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetShaderResourceTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetShaderResourceTextureView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The texture resource view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_ASSERT_DEV(pTextureView == nullptr || pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::ShaderResource, "Incorrect buffer view type. The view must be created with xiiGALTextureViewType::ShaderResource if not invalidated.");
#endif

  SetShaderResourceTextureViewPlatform(bindingInformation, pTextureView);
}

void xiiGALCommandList::SetShaderResourceTextureViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording && m_pPipelineState != nullptr, "Texture descriptor arrays require an active recording and pipeline state.");
  XII_ASSERT_DEV(uiFirstElement + pTextureViews.GetCount() <= bindingInformation.m_uiArraySize, "Texture descriptor write exceeds array '{}' capacity.", bindingInformation.m_sName);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiGALTextureView* pView : pTextureViews)
    XII_ASSERT_DEV(pView == nullptr || pView->GetDescription().m_ViewType == xiiGALTextureViewType::ShaderResource, "All texture views must be shader-resource views.");
#endif
  SetShaderResourceTextureViewsPlatform(bindingInformation, uiFirstElement, pTextureViews);
}

void xiiGALCommandList::SetUnorderedAccessBufferView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetUnorderedAccessBufferView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetUnorderedAccessBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetUnorderedAccessBufferView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The unordered access buffer view '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_ASSERT_DEV(pBufferView == nullptr || pBufferView->GetDescription().m_ViewType == xiiGALBufferViewType::UnorderedAccess, "Incorrect buffer view type. The view must be created with xiiGALBufferViewType::UnorderedAccess if not invalidated.");
#endif

  SetUnorderedAccessBufferViewPlatform(bindingInformation, pBufferView);
}

void xiiGALCommandList::SetUnorderedAccessBufferViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording && m_pPipelineState != nullptr, "Buffer UAV descriptor arrays require an active recording and pipeline state.");
  XII_ASSERT_DEV(uiFirstElement + pBufferViews.GetCount() <= bindingInformation.m_uiArraySize, "Buffer UAV descriptor write exceeds array '{}' capacity.", bindingInformation.m_sName);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiGALBufferView* pView : pBufferViews)
    XII_ASSERT_DEV(pView == nullptr || pView->GetDescription().m_ViewType == xiiGALBufferViewType::UnorderedAccess, "All buffer views must be unordered-access views.");
#endif
  SetUnorderedAccessBufferViewsPlatform(bindingInformation, uiFirstElement, pBufferViews);
}

void xiiGALCommandList::SetUnorderedAccessTextureView(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetUnorderedAccessTextureView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetUnorderedAccessTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetUnorderedAccessTextureView requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The unordered access texture view '{0}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
  XII_ASSERT_DEV(pTextureView == nullptr || pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::UnorderedAccess, "Incorrect buffer view type. The view must be created with xiiGALTextureViewType::UnorderedAccess if not invalidated.");
#endif

  SetUnorderedAccessTextureViewPlatform(bindingInformation, pTextureView);
}

void xiiGALCommandList::SetUnorderedAccessTextureViews(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording && m_pPipelineState != nullptr, "Texture UAV descriptor arrays require an active recording and pipeline state.");
  XII_ASSERT_DEV(uiFirstElement + pTextureViews.GetCount() <= bindingInformation.m_uiArraySize, "Texture UAV descriptor write exceeds array '{}' capacity.", bindingInformation.m_sName);
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiGALTextureView* pView : pTextureViews)
    XII_ASSERT_DEV(pView == nullptr || pView->GetDescription().m_ViewType == xiiGALTextureViewType::UnorderedAccess, "All texture views must be unordered-access views.");
#endif
  SetUnorderedAccessTextureViewsPlatform(bindingInformation, uiFirstElement, pTextureViews);
}

void xiiGALCommandList::SetSampler(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetSampler must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetSampler arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetSampler requires a pipeline state to be set.");

  // Check that the pipeline resource signature contains the binding information at the required shader stage.
  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The sampler resource '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
#endif

  SetSamplerPlatform(bindingInformation, pSampler);
}

void xiiGALCommandList::SetSamplers(const xiiGALPipelineResourceDescription& bindingInformation, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording && m_pPipelineState != nullptr, "Sampler descriptor arrays require an active recording and pipeline state.");
  XII_ASSERT_DEV(uiFirstElement + pSamplers.GetCount() <= bindingInformation.m_uiArraySize, "Sampler descriptor write exceeds array '{}' capacity.", bindingInformation.m_sName);
  SetSamplersPlatform(bindingInformation, uiFirstElement, pSamplers);
}

void xiiGALCommandList::SetAccelerationStructure(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetAccelerationStructure must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "SetAccelerationStructure arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "SetAccelerationStructure requires a pipeline state to be set.");

  bool bResourceFound = false;
  {
    const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource == bindingInformation)
      {
        bResourceFound = true;
        break;
      }
    }
  }

  XII_ASSERT_DEV(bResourceFound, "The acceleration structure resource '{}' with the required shader stages does not exist in the pipeline resource signature.", bindingInformation.m_sName);
#endif

  SetAccelerationStructurePlatform(bindingInformation, pTopLevelAS);
}

void xiiGALCommandList::ResolveAndSetConstantBuffer(const xiiTempHashedString& sResourceName, xiiGALBuffer* pConstantBuffer, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetConstantBuffer arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetConstantBuffer requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::ConstantBuffer && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetConstantBuffer(resource, pConstantBuffer);
    }
  }
}

void xiiGALCommandList::ResolveAndSetShaderResourceBufferView(const xiiTempHashedString& sResourceName, xiiGALBufferView* pBufferView, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetShaderResourceBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetShaderResourceBufferView requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::BufferSRV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetShaderResourceBufferView(resource, pBufferView);
    }
  }
}

void xiiGALCommandList::ResolveAndSetShaderResourceBufferViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews, xiiBitflags<xiiGALShaderType> shaderStages)
{
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "Resolving buffer descriptor arrays requires a pipeline resource signature.");
  for (const xiiGALPipelineResourceDescription& resource : m_pPipelineResourceSignature->GetDescription().m_Resources)
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::BufferSRV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
      SetShaderResourceBufferViews(resource, uiFirstElement, pBufferViews);
}

void xiiGALCommandList::ResolveAndSetShaderResourceTextureView(const xiiTempHashedString& sResourceName, xiiGALTextureView* pTextureView, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetShaderResourceTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetShaderResourceTextureView requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && (resource.m_ResourceType == xiiGALShaderResourceType::TextureSRV || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetShaderResourceTextureView(resource, pTextureView);
    }
  }
}

void xiiGALCommandList::ResolveAndSetShaderResourceTextureViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews, xiiBitflags<xiiGALShaderType> shaderStages)
{
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "Resolving texture descriptor arrays requires a pipeline resource signature.");
  for (const xiiGALPipelineResourceDescription& resource : m_pPipelineResourceSignature->GetDescription().m_Resources)
    if (resource.m_sName == sResourceName && (resource.m_ResourceType == xiiGALShaderResourceType::TextureSRV || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
      SetShaderResourceTextureViews(resource, uiFirstElement, pTextureViews);
}

void xiiGALCommandList::ResolveAndSetUnorderedAccessBufferView(const xiiTempHashedString& sResourceName, xiiGALBufferView* pBufferView, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetUnorderedAccessBufferView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetUnorderedAccessBufferView requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::BufferUAV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetUnorderedAccessBufferView(resource, pBufferView);
    }
  }
}

void xiiGALCommandList::ResolveAndSetUnorderedAccessBufferViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALBufferView*> pBufferViews, xiiBitflags<xiiGALShaderType> shaderStages)
{
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "Resolving buffer UAV descriptor arrays requires a pipeline resource signature.");
  for (const xiiGALPipelineResourceDescription& resource : m_pPipelineResourceSignature->GetDescription().m_Resources)
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::BufferUAV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
      SetUnorderedAccessBufferViews(resource, uiFirstElement, pBufferViews);
}

void xiiGALCommandList::ResolveAndSetUnorderedAccessTextureView(const xiiTempHashedString& sResourceName, xiiGALTextureView* pTextureView, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetUnorderedAccessTextureView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetUnorderedAccessTextureView requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::TextureUAV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetUnorderedAccessTextureView(resource, pTextureView);
    }
  }
}

void xiiGALCommandList::ResolveAndSetUnorderedAccessTextureViews(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALTextureView*> pTextureViews, xiiBitflags<xiiGALShaderType> shaderStages)
{
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "Resolving texture UAV descriptor arrays requires a pipeline resource signature.");
  for (const xiiGALPipelineResourceDescription& resource : m_pPipelineResourceSignature->GetDescription().m_Resources)
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::TextureUAV && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
      SetUnorderedAccessTextureViews(resource, uiFirstElement, pTextureViews);
}

void xiiGALCommandList::ResolveAndSetSampler(const xiiTempHashedString& sResourceName, xiiGALSampler* pSampler, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetSampler arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetSampler requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && (resource.m_ResourceType == xiiGALShaderResourceType::Sampler || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetSampler(resource, pSampler);
    }
  }
}

void xiiGALCommandList::ResolveAndSetSamplers(const xiiTempHashedString& sResourceName, xiiUInt32 uiFirstElement, xiiArrayPtr<xiiGALSampler*> pSamplers, xiiBitflags<xiiGALShaderType> shaderStages)
{
  XII_ASSERT_DEV(m_pPipelineResourceSignature != nullptr, "Resolving sampler descriptor arrays requires a pipeline resource signature.");
  for (const xiiGALPipelineResourceDescription& resource : m_pPipelineResourceSignature->GetDescription().m_Resources)
    if (resource.m_sName == sResourceName && (resource.m_ResourceType == xiiGALShaderResourceType::Sampler || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
      SetSamplers(resource, uiFirstElement, pSamplers);
}

void xiiGALCommandList::ResolveAndSetAccelerationStructure(const xiiTempHashedString& sResourceName, xiiGALTopLevelAS* pTopLevelAS, xiiBitflags<xiiGALShaderType> shaderStages /*= xiiGALShaderType::Unknown*/)
{
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "ResolveAndSetAccelerationStructure arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "ResolveAndSetAccelerationStructure requires a pipeline state to be set.");

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    if (resource.m_sName == sResourceName && resource.m_ResourceType == xiiGALShaderResourceType::AccelerationStructure && (!shaderStages.IsAnyFlagSet() || resource.m_ShaderStages.AreAllSet(shaderStages)))
    {
      SetAccelerationStructure(resource, pTopLevelAS);
    }
  }
}

xiiResult xiiGALCommandList::CommitShaderResources(xiiEnum<xiiGALStateTransitionMode> mode)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CommitShaderResources must be called while recording.");

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCommitShaderResources;

  return CommitShaderResourcesPlatform(mode);
}

void xiiGALCommandList::ClearRenderTargetView(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "ClearRenderTargetView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "ClearRenderTargetView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pRenderTargetView != nullptr, "ClearRenderTargetView arguments are invalid. The texture view handle has been invalidated.");

  const xiiGALTextureViewCreationDescription& viewDescription = pRenderTargetView->GetDescription();

  XII_ASSERT_DEV(viewDescription.m_ViewType == xiiGALTextureViewType::RenderTarget, "The texture view '{0}' was not created with the xiiGALTextureViewType::RenderTarget.", pRenderTargetView->GetDebugName());
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiClearRenderTarget;

  ClearRenderTargetViewPlatform(pRenderTargetView, clearColor);
}

void xiiGALCommandList::ClearDepthStencilView(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "ClearDepthStencilView must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "ClearDepthStencilView arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pDepthStencilView != nullptr, "ClearDepthStencilView arguments are invalid. The texture view handle has been invalidated.");

  const xiiGALTextureViewCreationDescription& viewDescription = pDepthStencilView->GetDescription();

  XII_ASSERT_DEV(viewDescription.m_ViewType == xiiGALTextureViewType::DepthStencil, "The texture view '{0}' was not created with the xiiGALTextureViewType::DepthStencil.", pDepthStencilView->GetDebugName());
  XII_ASSERT_DEV(bClearDepth || bClearStencil, "At least one of bClearDepth or bClearStencil must be set.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiClearDepthStencil;

  ClearDepthStencilViewPlatform(pDepthStencilView, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void xiiGALCommandList::BeginRenderPass(const xiiGALBeginRenderPassDescription& beginRenderPass)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "BeginRenderPass must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(beginRenderPass.m_pRenderPass != nullptr, "BeginRenderPass: Render pass handle is invalid.");
  XII_ASSERT_DEV(beginRenderPass.m_pFramebuffer != nullptr, "BeginRenderPass: Framebuffer handle is invalid.");

  const xiiGALRenderPassCreationDescription& renderPassDescription = beginRenderPass.m_pRenderPass->GetDescription();

  xiiUInt32 uiRequiredClearValueCount = 0;
  for (xiiUInt32 i = 0; i < renderPassDescription.m_Attachments.GetCount(); ++i)
  {
    const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[i];
    const xiiGALResourceFormatDescription&       formatProperties      = xiiGALTextureUtilities::GetResourceFormatProperties(attachmentDescription.m_Format);

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

  XII_ASSERT_DEV(beginRenderPass.m_ClearValues.GetCount() >= uiRequiredClearValueCount, "BeginRenderPass: At least {0} clear values are required, but only {1} are provided.", uiRequiredClearValueCount, beginRenderPass.m_ClearValues.GetCount());
#endif

  m_pRenderPass  = beginRenderPass.m_pRenderPass;
  m_pFramebuffer = beginRenderPass.m_pFramebuffer;

  ++m_CommandListStatistics.m_CommandListCounters.m_uiBeginRenderPass;

  BeginRenderPassPlatform(m_pRenderPass, m_pFramebuffer, beginRenderPass.m_ClearValues.GetArrayPtr());
}

void xiiGALCommandList::NextSubpass()
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "NextSubpass must be called while recording.");
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pRenderPass != nullptr, "NextSubpass: Render pass handle is invalid.");
  XII_ASSERT_DEV(m_pFramebuffer != nullptr, "NextSubpass: Framebuffer handle is invalid.");

  ++m_CommandListStatistics.m_CommandListCounters.m_uiNextSubPass;

  NextSubpassPlatform();
}

void xiiGALCommandList::EndRenderPass()
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "EndRenderPass must be called while recording.");
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "BeginRenderPass arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pRenderPass != nullptr, "NextSubpass: Render pass handle is invalid.");
  XII_ASSERT_DEV(m_pFramebuffer != nullptr, "NextSubpass: Framebuffer handle is invalid.");

  EndRenderPassPlatform();

  m_pRenderPass  = nullptr;
  m_pFramebuffer = nullptr;
}

void xiiGALCommandList::Draw(const xiiGALDrawDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "Draw must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "Draw arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "Draw arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());

  if (description.m_uiVertexCount == 0)
  {
    xiiLog::Info("Draw vertex count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
  if (description.m_uiInstanceCount == 0)
  {
    xiiLog::Info("Draw instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDraw;

  if (m_pPipelineState)
  {
    const xiiGALGraphicsPipelineStateCreationDescription& pipelineDescription = xiiDynamicCast<xiiGALGraphicsPipelineState*>(m_pPipelineState)->GetDescription();

    m_CommandListStatistics.m_PrimitiveCounters[pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology] += GetPrimitiveCount(pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology, description.m_uiVertexCount) * description.m_uiInstanceCount;
  }

  DrawPlatform(description);
}

void xiiGALCommandList::DrawIndexed(const xiiGALDrawIndexedDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DrawIndexed must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "DrawIndexed command arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DrawIndexed command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexed command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pIndexBuffer != nullptr, "DrawIndexed command arguments are invalid. No index buffer is set.");
  XII_ASSERT_DEV(description.m_IndexType == xiiGALValueType::UInt16 || description.m_IndexType == xiiGALValueType::UInt32, "DrawIndexed command arguments are invalid. Index type must be xiiGALValueType::UInt16 or xiiGALValueType::UInt32.");

  if (description.m_uiIndexCount == 0)
  {
    xiiLog::Info("DrawIndexed index count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
  if (description.m_uiInstanceCount == 0)
  {
    xiiLog::Info("DrawIndexed instance count is zero. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDrawIndexed;

  if (m_pPipelineState)
  {
    const xiiGALGraphicsPipelineStateCreationDescription& pipelineDescription = xiiDynamicCast<xiiGALGraphicsPipelineState*>(m_pPipelineState)->GetDescription();

    m_CommandListStatistics.m_PrimitiveCounters[pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology] += GetPrimitiveCount(pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology, description.m_uiIndexCount) * description.m_uiInstanceCount;
  }

  DrawIndexedPlatform(description);
}

void xiiGALCommandList::DrawIndirect(const xiiGALDrawIndirectDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DrawIndirect must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(description.m_pCounterBuffer == nullptr || m_pDevice->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer), "DrawIndirect command arguments are invalid. Counter buffer requires the xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer capability.");

  // There is no need to check xiiGALDrawCommandCapabilityFlags::DrawIndirect because an indirect buffer can only be created if this capability is supported.

  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DrawIndirect command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pRenderPass == nullptr || description.m_BufferStateTransition != xiiGALStateTransitionMode::Transition, "Resource state transitions are not permitted inside a render pass and may result in an undefined behavior. Do not use xiiGALStateTransitionMode::Transition or end the render pass first.");

  XII_ASSERT_DEV(description.m_pBuffer != nullptr, "DrawIndirect command arguments are invalid. Indirect draw arguments buffer must not be null.");

  const xiiGALBufferCreationDescription& argumentsBufferDescription = description.m_pBuffer->GetDescription();
  XII_ASSERT_DEV(argumentsBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawIndirect command arguments are invalid. Indirect draw arguments buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pBuffer->GetDebugName());

  if (description.m_uiDrawCount > 1U)
  {
    constexpr xiiUInt32 uiMinimumArgumentStride = sizeof(xiiUInt32) * 4U;

    XII_ASSERT_DEV(description.m_uiDrawArgumentStride >= uiMinimumArgumentStride, "DrawIndirect command arguments are invalid. Stride must be greater than {} bytes.", uiMinimumArgumentStride);
    XII_ASSERT_DEV((description.m_uiDrawArgumentStride % 4U) == 0U, "DrawIndirect command arguments are invalid. Stride must be greater a multiple of 4.");
  }

  const xiiUInt64 uiRequiredArgumentBufferSize = description.m_uiDrawArgumentOffset + (description.m_uiDrawCount > 1U ? description.m_uiDrawCount * description.m_uiDrawArgumentStride : xiiUInt32{sizeof(xiiUInt32)} * 5U);
  XII_ASSERT_DEV(uiRequiredArgumentBufferSize <= argumentsBufferDescription.m_uiSize, "DrawIndirect command arguments are invalid. Indirect draw arguments buffer ({}) size must be at least {} bytes.", description.m_pBuffer->GetDebugName(), uiRequiredArgumentBufferSize);

  if (description.m_pCounterBuffer != nullptr)
  {
    const xiiGALBufferCreationDescription& counterBufferDescription = description.m_pCounterBuffer->GetDescription();
    XII_ASSERT_DEV(counterBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawIndirect command arguments are invalid. Indirect counter buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pCounterBuffer->GetDebugName());

    const xiiUInt64 uiRequiredCounterBufferSize = description.m_uiCounterOffset + sizeof(xiiUInt32);
    XII_ASSERT_DEV(uiRequiredCounterBufferSize <= counterBufferDescription.m_uiSize, "DrawIndirect command arguments are invalid. Invalid counter offset ({}) or counter buffer '{}' size must be at least {} bytes.", uiRequiredCounterBufferSize, description.m_pBuffer->GetDebugName(), uiRequiredCounterBufferSize);
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDrawIndirect;

  DrawIndirectPlatform(description);
}

void xiiGALCommandList::DrawIndexedIndirect(const xiiGALDrawIndexedIndirectDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DrawIndexedIndirect must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(description.m_pCounterBuffer == nullptr || m_pDevice->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer), "DrawIndexedIndirect command arguments are invalid. Counter buffer requires the xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer capability.");

  // There is no need to check xiiGALDrawCommandCapabilityFlags::DrawIndirect because an indirect buffer can only be created if this capability is supported.

  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DrawIndexedIndirect command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "DrawIndexedIndirect command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pIndexBuffer != nullptr, "DrawIndexedIndirect command arguments are invalid. No index buffer is set.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr || description.m_BufferStateTransition != xiiGALStateTransitionMode::Transition, "Resource state transitions are not permitted inside a render pass and may result in an undefined behavior. Do not use xiiGALStateTransitionMode::Transition or end the render pass first.");

  XII_ASSERT_DEV(description.m_pBuffer != nullptr, "DrawIndexedIndirect command arguments are invalid. Indirect draw arguments buffer must not be null.");

  const xiiGALBufferCreationDescription& argumentsBufferDescription = description.m_pBuffer->GetDescription();
  XII_ASSERT_DEV(argumentsBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawIndexedIndirect command arguments are invalid. Indirect draw arguments buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pBuffer->GetDebugName());
  XII_ASSERT_DEV(description.m_IndexType == xiiGALValueType::UInt16 || description.m_IndexType == xiiGALValueType::UInt32, "DrawIndexedIndirect command arguments are invalid. Index type must be xiiGALValueType::UInt16 or xiiGALValueType::UInt32.");

  if (description.m_uiDrawCount > 1U)
  {
    constexpr xiiUInt32 uiMinimumArgumentStride = sizeof(xiiUInt32) * 5U;

    XII_ASSERT_DEV(description.m_uiDrawArgumentStride >= uiMinimumArgumentStride, "DrawIndexedIndirect command arguments are invalid. Stride must be greater than {} bytes.", uiMinimumArgumentStride);
    XII_ASSERT_DEV((description.m_uiDrawArgumentStride % 4U) == 0U, "DrawIndexedIndirect command arguments are invalid. Stride must be greater a multiple of 4.");
  }

  const xiiUInt64 uiRequiredArgumentBufferSize = description.m_uiDrawArgumentOffset + (description.m_uiDrawCount > 1U ? description.m_uiDrawCount * description.m_uiDrawArgumentStride : xiiUInt32{sizeof(xiiUInt32)} * 5U);
  XII_ASSERT_DEV(uiRequiredArgumentBufferSize <= argumentsBufferDescription.m_uiSize, "DrawIndexedIndirect command arguments are invalid. Indirect draw arguments buffer ({}) size must be at least {} bytes.", description.m_pBuffer->GetDebugName(), uiRequiredArgumentBufferSize);

  if (description.m_pCounterBuffer != nullptr)
  {
    const xiiGALBufferCreationDescription& counterBufferDescription = description.m_pCounterBuffer->GetDescription();
    XII_ASSERT_DEV(counterBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawIndexedIndirect command arguments are invalid. Indirect counter buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pCounterBuffer->GetDebugName());

    const xiiUInt64 uiRequiredCounterBufferSize = description.m_uiCounterOffset + sizeof(xiiUInt32);
    XII_ASSERT_DEV(uiRequiredCounterBufferSize <= counterBufferDescription.m_uiSize, "DrawIndexedIndirect command arguments are invalid. Invalid counter offset ({}) or counter buffer '{}' size must be at least {} bytes.", uiRequiredCounterBufferSize, description.m_pBuffer->GetDebugName(), uiRequiredCounterBufferSize);
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDrawIndexedIndirect;

  DrawIndexedIndirectPlatform(description);
}

void xiiGALCommandList::DrawMesh(const xiiGALDrawMeshDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DrawMesh must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "DrawMesh command arguments are invalid. Mesh shaders are not supported by this device.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DrawMesh command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Mesh, "DrawMesh command arguments are invalid. Pipeline state {0} is not a mesh pipeline.", m_pPipelineState->GetDebugName());

  const auto& meshProperties = m_pDevice->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;

  if (description.m_uiThreadGroupCountX == 0)
  {
    xiiLog::Info("DrawMeshDescription.ThreadGroupCountX is 0. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
  if (description.m_uiThreadGroupCountY == 0)
  {
    xiiLog::Info("DrawMeshDescription.ThreadGroupCountY is 0. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
  if (description.m_uiThreadGroupCountZ == 0)
  {
    xiiLog::Info("DrawMeshDescription.ThreadGroupCountZ is 0. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }

  XII_ASSERT_DEV(description.m_uiThreadGroupCountX <= meshProperties.m_uiMaxThreadGroupCountX, "DrawMesh command arguments are invalid. The thread group count X ({0}) exceeds the maximum supported by the device ({1}).", description.m_uiThreadGroupCountX, meshProperties.m_uiMaxThreadGroupCountX);
  XII_ASSERT_DEV(description.m_uiThreadGroupCountY <= meshProperties.m_uiMaxThreadGroupCountY, "DrawMesh command arguments are invalid. The thread group count Y ({0}) exceeds the maximum supported by the device ({1}).", description.m_uiThreadGroupCountY, meshProperties.m_uiMaxThreadGroupCountY);
  XII_ASSERT_DEV(description.m_uiThreadGroupCountZ <= meshProperties.m_uiMaxThreadGroupCountZ, "DrawMesh command arguments are invalid. The thread group count Z ({0}) exceeds the maximum supported by the device ({1}).", description.m_uiThreadGroupCountZ, meshProperties.m_uiMaxThreadGroupCountZ);

  const auto uiTotalThreadGroupCount = description.m_uiThreadGroupCountX + description.m_uiThreadGroupCountY + description.m_uiThreadGroupCountZ;

  XII_ASSERT_DEV(uiTotalThreadGroupCount <= meshProperties.m_uiMaxThreadGroupTotalCount, "DrawMesh command arguments are invalid. The total thread group count ({0}) exceeds the maximum supported by the device ({1}).", uiTotalThreadGroupCount, meshProperties.m_uiMaxThreadGroupTotalCount);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDrawMesh;

  DrawMeshPlatform(description);
}

void xiiGALCommandList::DrawMeshIndirect(const xiiGALDrawMeshIndirectDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DrawMeshIndirect must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "DrawMeshIndirect command arguments are invalid. Mesh shaders are not supported by this device.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DrawMeshIndirect command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Mesh, "DrawMeshIndirect command arguments are invalid. Pipeline state {0} is not a mesh pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(description.m_pCounterBuffer == nullptr || (m_pDevice->GetGraphicsDeviceAdapterProperties().m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer)), "DrawMeshIndirect command arguments are invalid. Counter buffer requires xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer draw capability.");

  // There is no need to check xiiGALDrawCommandCapabilityFlags::DrawIndirect because an indirect buffer can only be created if this capability is supported.

  XII_ASSERT_DEV(description.m_pBuffer != nullptr, "DrawMeshIndirect command arguments are invalid. Indirect draw arguments buffer must not be null.");

  const xiiGALBufferCreationDescription& argumentsBufferDescription = description.m_pBuffer->GetDescription();
  XII_ASSERT_DEV(argumentsBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawMeshIndirect command arguments are invalid. Indirect draw arguments buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pBuffer->GetDebugName());

  const xiiUInt64 uiRequiredArgumentBufferSize = description.m_uiDrawArgumentOffset + xiiUInt64{s_uiDrawMeshIndirectCommandStride} * xiiUInt64{description.m_uiCommandCount};
  XII_ASSERT_DEV(uiRequiredArgumentBufferSize <= argumentsBufferDescription.m_uiSize, "DrawMeshIndirect command arguments are invalid. Indirect draw arguments buffer ({}) size must be at least {} bytes.", description.m_pBuffer->GetDebugName(), uiRequiredArgumentBufferSize);

  if (description.m_pCounterBuffer != nullptr)
  {
    const xiiGALBufferCreationDescription& counterBufferDescription = description.m_pCounterBuffer->GetDescription();
    XII_ASSERT_DEV(counterBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DrawMeshIndirect command arguments are invalid. Indirect counter buffer ({}) was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pCounterBuffer->GetDebugName());

    const xiiUInt64 uiRequiredCounterBufferSize = description.m_uiCounterOffset + sizeof(xiiUInt32);
    XII_ASSERT_DEV(uiRequiredCounterBufferSize <= counterBufferDescription.m_uiSize, "DrawMeshIndirect command arguments are invalid. Invalid counter offset ({}) or counter buffer '{}' size must be at least {} bytes.", uiRequiredCounterBufferSize, description.m_pBuffer->GetDebugName(), uiRequiredCounterBufferSize);
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDrawMeshIndirect;

  DrawMeshIndirectPlatform(description);
}

void xiiGALCommandList::MultiDraw(const xiiGALMultiDrawDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "MultiDraw must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "MultiDraw command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "MultiDraw command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pIndexBuffer != nullptr, "MultiDraw command arguments are invalid. No index buffer is set.");

  if (description.m_uiInstanceCount == 0)
  {
    xiiLog::Info("MultiDrawDescription.InstanceCount is 0. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
#endif

  if (m_pPipelineState)
  {
    const xiiGALGraphicsPipelineStateCreationDescription& pipelineDescription = xiiDynamicCast<xiiGALGraphicsPipelineState*>(m_pPipelineState)->GetDescription();

    for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
    {
      m_CommandListStatistics.m_PrimitiveCounters[pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology] += GetPrimitiveCount(pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology, description.m_pDrawItems[i].m_uiVertexCount) * description.m_uiInstanceCount;
    }
  }

  if (m_bNativeMultiDrawSupported)
  {
    ++m_CommandListStatistics.m_CommandListCounters.m_uiMultiDraw;
  }
  else
  {
    m_CommandListStatistics.m_CommandListCounters.m_uiDraw += description.m_pDrawItems.GetCount();
  }

  MultiDrawPlatform(description);
}

void xiiGALCommandList::MultiDrawIndexed(const xiiGALMultiDrawIndexedDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "MultiDrawIndexed must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "Draw arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "MultiDrawIndexed command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Graphics, "MultiDrawIndexed command arguments are invalid. Pipeline state {0} is not a graphics pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pIndexBuffer != nullptr, "MultiDrawIndexed command arguments are invalid. No index buffer is set.");
  XII_ASSERT_DEV(description.m_IndexType == xiiGALValueType::UInt16 || description.m_IndexType == xiiGALValueType::UInt32, "MultiDrawIndexed command arguments are invalid. Index type must be xiiGALValueType::UInt16 or xiiGALValueType::UInt32.");

  if (description.m_uiInstanceCount == 0)
  {
    xiiLog::Info("MultiDrawIndexedDescription.InstanceCount is 0. This is acceptable but the draw command will be ignored, but may be unintentional.");
  }
#endif

  if (m_pPipelineState)
  {
    const xiiGALGraphicsPipelineStateCreationDescription& pipelineDescription = xiiDynamicCast<xiiGALGraphicsPipelineState*>(m_pPipelineState)->GetDescription();

    for (xiiUInt32 i = 0; i < description.m_pDrawItems.GetCount(); ++i)
    {
      m_CommandListStatistics.m_PrimitiveCounters[pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology] += GetPrimitiveCount(pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology, description.m_pDrawItems[i].m_uiIndexCount) * description.m_uiInstanceCount;
    }
  }

  if (m_bNativeMultiDrawSupported)
  {
    ++m_CommandListStatistics.m_CommandListCounters.m_uiMultiDrawIndexed;
  }
  else
  {
    m_CommandListStatistics.m_CommandListCounters.m_uiDrawIndexed += description.m_pDrawItems.GetCount();
  }

  MultiDrawIndexedPlatform(description);
}

void xiiGALCommandList::DispatchCompute(const xiiGALDispatchComputeDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DispatchCompute must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Compute), "Dispatch arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "Dispatch command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "Dispatch command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "Dispatch command arguments are invalid. Dispatch command must be performed outside of render pass.");

  if (description.m_uiThreadGroupCountX == 0)
  {
    xiiLog::Info("DispatchComputeDescription.ThreadGroupCountX is 0. This is acceptable but the dispatch command will be ignored, but may be unintentional.");
  }
  if (description.m_uiThreadGroupCountY == 0)
  {
    xiiLog::Info("DispatchComputeDescription.ThreadGroupCountY is 0. This is acceptable but the dispatch command will be ignored, but may be unintentional.");
  }
  if (description.m_uiThreadGroupCountZ == 0)
  {
    xiiLog::Info("DispatchComputeDescription.ThreadGroupCountZ is 0. This is acceptable but the dispatch command will be ignored, but may be unintentional.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDispatchCompute;

  DispatchComputePlatform(description);
}

void xiiGALCommandList::DispatchComputeIndirect(const xiiGALDispatchComputeIndirectDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DispatchComputeIndirect must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Compute), "DispatchIndirect arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "DispatchIndirect command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::Compute, "DispatchIndirect command arguments are invalid. Pipeline state {0} is not a compute pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "DispatchIndirect command arguments are invalid. DispatchIndirect command must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pBuffer != nullptr, "The indirect arguments buffer is invalidated.");

  const auto& bufferDescription = description.m_pBuffer->GetDescription();
  XII_ASSERT_DEV(bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments), "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' was not created with the xiiGALBindFlags::IndirectDrawArguments bind flag.", description.m_pBuffer->GetDebugName());

  const xiiUInt64 uiOffset = ((sizeof(xiiUInt32) * 3) + description.m_uiDispatchArgumentOffset);
  XII_ASSERT_DEV(uiOffset <= bufferDescription.m_uiSize, "DispatchIndirect command arguments are invalid. The dispatch indirect arguments buffer '{0}' offset in bytes must be at least {1} bytes.", description.m_pBuffer->GetDebugName());
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiDispatchComputeIndirect;

  DispatchComputeIndirectPlatform(description);
}

void xiiGALCommandList::TraceRays(const xiiGALTraceRaysDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "TraceRays must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "TraceRays arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "TraceRays command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::RayTracing, "TraceRays command arguments are invalid. Pipeline state {0} is not a ray tracing pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "TraceRays command arguments are invalid. TraceRays command must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pShaderBindingTable != nullptr, "TraceRays command arguments are invalid. Shader binding table buffer must not be null.");

  const xiiGALBufferCreationDescription& sbtBufferDescription = description.m_pShaderBindingTable->GetDescription();
  XII_ASSERT_DEV(sbtBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing), "TraceRays command arguments are invalid. Shader binding table buffer ({}) must be created with xiiGALBindFlags::RayTracing.", description.m_pShaderBindingTable->GetDebugName());

  auto VerifyRegion = [&](const xiiGALRayTracingSBTRegionDescription& region, const char* szRegionName) {
    if (region.m_uiSize == 0U)
      return;

    XII_ASSERT_DEV(region.m_uiStride > 0U, "TraceRays command arguments are invalid. {} region stride must be non-zero when size is non-zero.", szRegionName);
    XII_ASSERT_DEV((region.m_uiOffset + region.m_uiSize) <= sbtBufferDescription.m_uiSize, "TraceRays command arguments are invalid. {} region exceeds shader binding table buffer size.", szRegionName);
  };

  VerifyRegion(description.m_RayGenerationTable, "RayGeneration");
  VerifyRegion(description.m_MissTable, "Miss");
  VerifyRegion(description.m_HitTable, "Hit");
  VerifyRegion(description.m_CallableTable, "Callable");

  if (description.m_uiWidth == 0U || description.m_uiHeight == 0U || description.m_uiDepth == 0U)
  {
    xiiLog::Info("TraceRays dimensions contain 0. This is acceptable but the trace command will be ignored, and may be unintentional.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiTraceRays;

  TraceRaysPlatform(description);
}

void xiiGALCommandList::TraceRaysIndirect(const xiiGALTraceRaysIndirectDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "TraceRaysIndirect must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "TraceRaysIndirect arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pPipelineState != nullptr, "TraceRaysIndirect command arguments are invalid. No pipeline state is set.");
  XII_ASSERT_DEV(m_pPipelineState->GetDescription().m_PipelineType == xiiGALPipelineType::RayTracing, "TraceRaysIndirect command arguments are invalid. Pipeline state {0} is not a ray tracing pipeline.", m_pPipelineState->GetDebugName());
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "TraceRaysIndirect command arguments are invalid. TraceRaysIndirect command must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pShaderBindingTable != nullptr, "TraceRaysIndirect command arguments are invalid. Shader binding table buffer must not be null.");
  XII_ASSERT_DEV(description.m_pArgumentBuffer != nullptr, "TraceRaysIndirect command arguments are invalid. Indirect argument buffer must not be null.");

  const xiiGALBufferCreationDescription& sbtBufferDescription = description.m_pShaderBindingTable->GetDescription();
  XII_ASSERT_DEV(sbtBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing), "TraceRaysIndirect command arguments are invalid. Shader binding table buffer ({}) must be created with xiiGALBindFlags::RayTracing.", description.m_pShaderBindingTable->GetDebugName());

  const xiiGALBufferCreationDescription& argumentBufferDescription = description.m_pArgumentBuffer->GetDescription();
  XII_ASSERT_DEV(argumentBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing), "TraceRaysIndirect command arguments are invalid. Argument buffer ({}) must be created with xiiGALBindFlags::RayTracing.", description.m_pArgumentBuffer->GetDebugName());

  auto VerifyRegion = [&](const xiiGALRayTracingSBTRegionDescription& region, const char* szRegionName) {
    if (region.m_uiSize == 0U)
      return;

    XII_ASSERT_DEV(region.m_uiStride > 0U, "TraceRaysIndirect command arguments are invalid. {} region stride must be non-zero when size is non-zero.", szRegionName);
    XII_ASSERT_DEV((region.m_uiOffset + region.m_uiSize) <= sbtBufferDescription.m_uiSize, "TraceRaysIndirect command arguments are invalid. {} region exceeds shader binding table buffer size.", szRegionName);
  };

  VerifyRegion(description.m_RayGenerationTable, "RayGeneration");
  VerifyRegion(description.m_MissTable, "Miss");
  VerifyRegion(description.m_HitTable, "Hit");
  VerifyRegion(description.m_CallableTable, "Callable");

  constexpr xiiUInt64 uiIndirectArgumentSize = sizeof(xiiUInt32) * 3U;
  XII_ASSERT_DEV((description.m_uiArgumentOffset + uiIndirectArgumentSize) <= argumentBufferDescription.m_uiSize, "TraceRaysIndirect command arguments are invalid. Argument buffer offset and size exceed the indirect argument buffer bounds.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiTraceRaysIndirect;

  TraceRaysIndirectPlatform(description);
}

void xiiGALCommandList::UpdateSBT(const xiiGALUpdateSBTDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "UpdateSBT must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "UpdateSBT arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "UpdateSBT command arguments are invalid. UpdateSBT must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pShaderBindingTable != nullptr, "UpdateSBT command arguments are invalid. Shader binding table buffer must not be null.");

  xiiGALPipelineState* pPipelineState = description.m_pPipelineState;
  if (pPipelineState == nullptr)
  {
    pPipelineState = m_pPipelineState;
  }

  XII_ASSERT_DEV(pPipelineState != nullptr, "UpdateSBT command arguments are invalid. No ray tracing pipeline was provided or currently bound.");
  XII_ASSERT_DEV(pPipelineState->GetDescription().IsRayTracingPipeline(), "UpdateSBT command arguments are invalid. Provided pipeline must be a ray tracing pipeline.");

  const xiiGALBufferCreationDescription& sbtBufferDescription = description.m_pShaderBindingTable->GetDescription();
  XII_ASSERT_DEV(sbtBufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::RayTracing), "UpdateSBT command arguments are invalid. Shader binding table buffer ({}) must be created with xiiGALBindFlags::RayTracing.", description.m_pShaderBindingTable->GetDebugName());

  auto VerifyRegion = [&](const xiiGALRayTracingSBTRegionDescription& region, const char* szRegionName) {
    if (region.m_uiSize == 0U)
      return;

    XII_ASSERT_DEV(region.m_uiStride > 0U, "UpdateSBT command arguments are invalid. {} region stride must be non-zero when size is non-zero.", szRegionName);
    XII_ASSERT_DEV((region.m_uiOffset + region.m_uiSize) <= sbtBufferDescription.m_uiSize, "UpdateSBT command arguments are invalid. {} region exceeds shader binding table buffer size.", szRegionName);
  };

  VerifyRegion(description.m_RayGenerationTable, "RayGeneration");
  VerifyRegion(description.m_MissTable, "Miss");
  VerifyRegion(description.m_HitTable, "Hit");
  VerifyRegion(description.m_CallableTable, "Callable");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiUpdateSBT;

  UpdateSBTPlatform(description);
}

void xiiGALCommandList::BuildBLAS(const xiiGALBuildBLASDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "BuildBLAS must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "BuildBLAS arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "BuildBLAS command arguments are invalid. BuildBLAS must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pBottomLevelAS != nullptr, "BuildBLAS command arguments are invalid. BLAS handle is invalid.");
  XII_ASSERT_DEV(description.m_pScratchBuffer != nullptr, "BuildBLAS command arguments are invalid. Scratch buffer handle is invalid.");

  const xiiGALBottomLevelASCreationDescription& blasDescription = description.m_pBottomLevelAS->GetDescription();
  XII_ASSERT_DEV(description.m_Triangles.GetCount() == blasDescription.m_Triangles.GetCount(), "BuildBLAS command arguments are invalid. Triangle build input count ({}) must match BLAS triangle geometry count ({}).", description.m_Triangles.GetCount(), blasDescription.m_Triangles.GetCount());
  XII_ASSERT_DEV(description.m_BoundingBoxes.GetCount() == blasDescription.m_BoundingBoxes.GetCount(), "BuildBLAS command arguments are invalid. Bounding box build input count ({}) must match BLAS AABB geometry count ({}).", description.m_BoundingBoxes.GetCount(), blasDescription.m_BoundingBoxes.GetCount());
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiBuildBLAS;

  BuildBLASPlatform(description);
}

void xiiGALCommandList::BuildTLAS(const xiiGALBuildTLASDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "BuildTLAS must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "BuildTLAS arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "BuildTLAS command arguments are invalid. BuildTLAS must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pTopLevelAS != nullptr, "BuildTLAS command arguments are invalid. TLAS handle is invalid.");
  XII_ASSERT_DEV(description.m_pInstanceBuffer != nullptr, "BuildTLAS command arguments are invalid. Instance buffer handle is invalid.");
  XII_ASSERT_DEV(description.m_pScratchBuffer != nullptr, "BuildTLAS command arguments are invalid. Scratch buffer handle is invalid.");
  XII_ASSERT_DEV(description.m_uiInstanceCount > 0U, "BuildTLAS command arguments are invalid. InstanceCount must be non-zero.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiBuildTLAS;

  BuildTLASPlatform(description);
}

void xiiGALCommandList::CopyBLAS(const xiiGALCopyBLASDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyBLAS must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "CopyBLAS arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyBLAS command arguments are invalid. CopyBLAS must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pSourceBottomLevelAS != nullptr, "CopyBLAS command arguments are invalid. Source BLAS is invalid.");
  XII_ASSERT_DEV(description.m_pDestinationBottomLevelAS != nullptr, "CopyBLAS command arguments are invalid. Destination BLAS is invalid.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyBLAS;

  CopyBLASPlatform(description);
}

void xiiGALCommandList::CopyTLAS(const xiiGALCopyTLASDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyTLAS must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "CopyTLAS arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyTLAS command arguments are invalid. CopyTLAS must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pSourceTopLevelAS != nullptr, "CopyTLAS command arguments are invalid. Source TLAS is invalid.");
  XII_ASSERT_DEV(description.m_pDestinationTopLevelAS != nullptr, "CopyTLAS command arguments are invalid. Destination TLAS is invalid.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyTLAS;

  CopyTLASPlatform(description);
}

void xiiGALCommandList::WriteBLASCompactedSize(const xiiGALWriteBLASCompactedSizeDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "WriteBLASCompactedSize must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "WriteBLASCompactedSize arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "WriteBLASCompactedSize command arguments are invalid. The operation must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pBottomLevelAS != nullptr, "WriteBLASCompactedSize command arguments are invalid. Source BLAS is invalid.");
  XII_ASSERT_DEV(description.m_pDestinationBuffer != nullptr, "WriteBLASCompactedSize command arguments are invalid. Destination buffer is invalid.");

  const xiiGALBufferCreationDescription& destinationBufferDescription = description.m_pDestinationBuffer->GetDescription();
  XII_ASSERT_DEV((description.m_uiDestinationBufferOffset + sizeof(xiiUInt64)) <= destinationBufferDescription.m_uiSize, "WriteBLASCompactedSize command arguments are invalid. Destination offset ({}) exceeds destination buffer size ({}).", description.m_uiDestinationBufferOffset, destinationBufferDescription.m_uiSize);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiWriteBLASCompactedSize;

  WriteBLASCompactedSizePlatform(description);
}

void xiiGALCommandList::WriteTLASCompactedSize(const xiiGALWriteTLASCompactedSizeDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "WriteTLASCompactedSize must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsAnySet(xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Compute), "WriteTLASCompactedSize arguments are invalid. The command list does not have the xiiGALCommandQueueFlags::Graphics or xiiGALCommandQueueFlags::Compute flag.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "WriteTLASCompactedSize command arguments are invalid. The operation must be performed outside of render pass.");
  XII_ASSERT_DEV(description.m_pTopLevelAS != nullptr, "WriteTLASCompactedSize command arguments are invalid. Source TLAS is invalid.");
  XII_ASSERT_DEV(description.m_pDestinationBuffer != nullptr, "WriteTLASCompactedSize command arguments are invalid. Destination buffer is invalid.");

  const xiiGALBufferCreationDescription& destinationBufferDescription = description.m_pDestinationBuffer->GetDescription();
  XII_ASSERT_DEV((description.m_uiDestinationBufferOffset + sizeof(xiiUInt64)) <= destinationBufferDescription.m_uiSize, "WriteTLASCompactedSize command arguments are invalid. Destination offset ({}) exceeds destination buffer size ({}).", description.m_uiDestinationBufferOffset, destinationBufferDescription.m_uiSize);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiWriteTLASCompactedSize;

  WriteTLASCompactedSizePlatform(description);
}

void xiiGALCommandList::BeginQuery(xiiGALQuery* pQuery)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "BeginQuery must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pQuery != nullptr, "BeginQuery must not be called on an invalidated query.");

  const xiiGALQueryCreationDescription& queryDescription = pQuery->GetDescription();

  XII_ASSERT_DEV(queryDescription.m_Type != xiiGALQueryType::Timestamp, "BeginQuery cannot be called on timestamp queries. Use EndQuery instead to set the timestamp.");
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(queryDescription.m_Type == xiiGALQueryType::Duration ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics), "BeginQuery command arguments are invalid. Invalid command queue of query type.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiBeginQuery;

  BeginQueryPlatform(pQuery);
}

void xiiGALCommandList::EndQuery(xiiGALQuery* pQuery)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "EndQuery must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pQuery != nullptr, "EndQuery must not be called on an invalidated query.");

  const xiiGALQueryCreationDescription& queryDescription = pQuery->GetDescription();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(queryDescription.m_Type == xiiGALQueryType::Duration ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics), "EndQuery command arguments are invalid. Invalid command queue of query type.");
#endif

  EndQueryPlatform(pQuery);
}

void xiiGALCommandList::TransitionResourceStates(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "TransitionResourceStates must be called while recording.");

  if (pResourceBarriers.IsEmpty())
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiUInt32 uiBarrierIndex = 0; uiBarrierIndex < pResourceBarriers.GetCount(); ++uiBarrierIndex)
  {
    const xiiGALStateTransitionDescription& barrier = pResourceBarriers[uiBarrierIndex];

    if (barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::Aliasing))
    {
      XII_ASSERT_DEV(barrier.m_TransitionFlags.IsStrictlyAnySet(xiiGALStateTransitionFlags::Aliasing), "pResourceBarriers[{}].TransitionFlags has flag xiiGALStateTransitionFlags::Aliasing that incompatible with other flags.", uiBarrierIndex);

      auto VerifySparseAliasedResource = [](xiiGALResource* pResource) -> xiiGALResourceDimension::Enum {
        if (pResource == nullptr)
          return xiiGALResourceDimension::Undefined;

        if (xiiGALTexture* pTexture = xiiDynamicCast<xiiGALTexture*>(pResource))
        {
          const xiiGALTextureCreationDescription& textureDescription = pTexture->GetDescription();

          XII_ASSERT_DEV(textureDescription.m_Usage == xiiGALResourceUsage::Sparse, "Texture '{}' used in aliasing barrier is not a sparse resource.", pTexture->GetDebugName());
          XII_ASSERT_DEV(textureDescription.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::SparseAlias), "Texture '{}' used in aliasing barrier was not created with xiiGALMiscTextureFlags::SparseAlias flag.", pTexture->GetDebugName());

          return textureDescription.m_Type;
        }
        else if (xiiGALBuffer* pBuffer = xiiDynamicCast<xiiGALBuffer*>(pResource))
        {
          const xiiGALBufferCreationDescription& bufferDescription = pBuffer->GetDescription();

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

      xiiGALResourceDimension::Enum previousDimension = VerifySparseAliasedResource(barrier.m_pPreviousResource);
      xiiGALResourceDimension::Enum currentDimension  = VerifySparseAliasedResource(barrier.m_pResource);
      if (previousDimension != xiiGALResourceDimension::Undefined && currentDimension != xiiGALResourceDimension::Undefined)
      {
        XII_ASSERT_DEV((previousDimension == xiiGALResourceDimension::Buffer) == (currentDimension == xiiGALResourceDimension::Buffer), "In pResourceBarriers[{}], both previous- and current-resources must either be buffers or textures. Sparse aliasing between textures and buffers are not permitted.", uiBarrierIndex);
      }

      XII_ASSERT_DEV(barrier.m_OldState == xiiGALResourceStateFlags::Unknown && barrier.m_NewState == xiiGALResourceStateFlags::Unknown, "In pResourceBarriers[{}], Aliasing buffer is applied to all subresource. OldState and NewState must be xiiGALResourceStateFlags::Unknown.", uiBarrierIndex);
      XII_ASSERT_DEV(barrier.m_uiFirstArraySlice == 0 && barrier.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS && barrier.m_uiFirstArraySlice == 0 && barrier.m_uiArraySliceCount == XII_GAL_REMAINING_ARRAY_SLICES, "In pResourceBarriers[{}], Aliasing barrier is applied to all subresources. FirstMipLevel, MipLevelCount, FirstArraySlice, ArraySliceCount must be set as default.", uiBarrierIndex);
    }
    else
    {
      XII_ASSERT_DEV(barrier.m_pPreviousResource == nullptr, "In pResourceBarriers[{}].pPreviousResource is only used for aliasing barrier and must be null otherwise.", uiBarrierIndex);
      XII_ASSERT_DEV(barrier.m_NewState != xiiGALResourceStateFlags::Unknown && barrier.m_NewState != xiiGALResourceStateFlags::Undefined, "In pResourceBarriers[{}].NewState must not be xiiGALResourceStateFlags::Unknown or xiiGALResourceStateFlags::Undefined.", uiBarrierIndex);
      XII_ASSERT_DEV(barrier.m_pResource != nullptr, "In pResourceBarriers[{}].pResource must not be null.", uiBarrierIndex);

      xiiBitflags<xiiGALResourceStateFlags> previousState = xiiGALResourceStateFlags::Unknown;

      if (xiiGALTexture* pTexture = xiiDynamicCast<xiiGALTexture*>(barrier.m_pResource))
      {
        previousState = barrier.m_OldState != xiiGALResourceStateFlags::Unknown ? barrier.m_OldState : pTexture->GetResourceState();

        const xiiGALTextureCreationDescription& textureDescription = pTexture->GetDescription();

        XII_ASSERT_DEV(previousState != xiiGALResourceStateFlags::Unknown, "pResourceBarriers[{}].OldState for texture '{}' is unknown to the engine and is not explicitly specified in the barrier.", uiBarrierIndex, pTexture->GetDebugName());
        XII_ASSERT_DEV(VerifyResourceStates(previousState, true), "pResourceBarriers[{}].OldState ({}) is invalid for texture '{}'.", uiBarrierIndex, xiiArgEnum(previousState), pTexture->GetDebugName());
        XII_ASSERT_DEV(VerifyResourceStates(barrier.m_NewState, true), "pResourceBarriers[{}].NewState ({}) is invalid for texture '{}'.", uiBarrierIndex, xiiArgEnum(barrier.m_NewState), pTexture->GetDebugName());

        XII_ASSERT_DEV(barrier.m_uiFirstMipLevel < textureDescription.m_uiMipLevels, "pResourceBarriers[{}].FirstMipLevel ({}) is out of range. Texture '{}' has only {} mip level (s).", uiBarrierIndex, barrier.m_uiFirstMipLevel, pTexture->GetDebugName(), textureDescription.m_uiMipLevels);
        XII_ASSERT_DEV(barrier.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS || (barrier.m_uiFirstMipLevel + barrier.m_uiMipLevelCount) <= textureDescription.m_uiMipLevels, "pResourceBarriers[{}] mip level range [{}, {}] is out of range. Texture '{}' has only {} mip level (s).", uiBarrierIndex, barrier.m_uiFirstMipLevel, barrier.m_uiMipLevelCount - 1, pTexture->GetDebugName(), textureDescription.m_uiMipLevels);

        XII_ASSERT_DEV(barrier.m_uiFirstArraySlice < textureDescription.m_uiArraySizeOrDepth, "pResourceBarriers[{}].FirstArraySlice ({}) is out of range. Array size of texture '{}' is {}.", uiBarrierIndex, barrier.m_uiFirstArraySlice, pTexture->GetDebugName(), textureDescription.m_uiArraySizeOrDepth);
        XII_ASSERT_DEV(barrier.m_uiArraySliceCount == XII_GAL_REMAINING_ARRAY_SLICES || (barrier.m_uiFirstArraySlice + barrier.m_uiArraySliceCount) <= textureDescription.m_uiArraySizeOrDepth, "pResourceBarriers[{}] array slice range [{}, {}] is out of range. Array size of texture '{}' is {}.", uiBarrierIndex, barrier.m_uiFirstArraySlice, barrier.m_uiArraySliceCount - 1, pTexture->GetDebugName(), textureDescription.m_uiArraySizeOrDepth);

        xiiEnum<xiiGALGraphicsDeviceType> adapterType = m_pDevice->GetDescription().m_GraphicsDeviceType;
        if (adapterType != xiiGALGraphicsDeviceType::Vulkan && adapterType != xiiGALGraphicsDeviceType::Direct3D12)
        {
          XII_ASSERT_DEV(barrier.m_uiFirstMipLevel == 0 && (barrier.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS || barrier.m_uiMipLevelCount == textureDescription.m_uiMipLevels), "Failed to transition texture '{}' in pResourceBarriers[{}], only whole resources can be transitioned on this device.", pTexture->GetDebugName(), uiBarrierIndex);
          XII_ASSERT_DEV(barrier.m_uiFirstArraySlice == 0 && (barrier.m_uiArraySliceCount == XII_GAL_REMAINING_MIP_LEVELS || barrier.m_uiArraySliceCount == textureDescription.m_uiArraySizeOrDepth), "Failed to transition texture '{}' in pResourceBarriers[{}], only whole resources can be transitioned on this device.", pTexture->GetDebugName(), uiBarrierIndex);
        }
      }
      else if (xiiGALBuffer* pBuffer = xiiDynamicCast<xiiGALBuffer*>(barrier.m_pResource))
      {
        previousState = barrier.m_OldState != xiiGALResourceStateFlags::Unknown ? barrier.m_OldState : pBuffer->GetResourceState();

        XII_ASSERT_DEV(previousState != xiiGALResourceStateFlags::Unknown, "pResourceBarriers[{}].OldState for buffer '{}' is unknown to the engine and is not explicitly specified in the barrier.", uiBarrierIndex, pBuffer->GetDebugName());
        XII_ASSERT_DEV(VerifyResourceStates(previousState, false), "pResourceBarriers[{}].OldState is invalid for buffer '{}'.", uiBarrierIndex, pBuffer->GetDebugName());
        XII_ASSERT_DEV(VerifyResourceStates(barrier.m_NewState, false), "pResourceBarriers[{}].NewState is invalid for buffer '{}'.", uiBarrierIndex, pBuffer->GetDebugName());
      }
      else if (xiiGALBottomLevelAS* pBottomLevelAS = xiiDynamicCast<xiiGALBottomLevelAS*>(barrier.m_pResource))
      {
        previousState = barrier.m_OldState != xiiGALResourceStateFlags::Unknown ? barrier.m_OldState : pBottomLevelAS->GetResourceState();

        XII_ASSERT_DEV(previousState != xiiGALResourceStateFlags::Unknown, "pResourceBarriers[{}].OldState for BLAS '{}' is unknown to the engine and is not explicitly specified in the barrier.", uiBarrierIndex, pBottomLevelAS->GetDebugName());
        XII_ASSERT_DEV(barrier.m_NewState == xiiGALResourceStateFlags::BuildASRead || barrier.m_NewState == xiiGALResourceStateFlags::BuildASWrite || barrier.m_NewState == xiiGALResourceStateFlags::RayTracing, "pResourceBarriers[{}].NewState for BLAS '{}' is invalid.", uiBarrierIndex, pBottomLevelAS->GetDebugName());
        XII_ASSERT_DEV(barrier.m_TransitionType == xiiGALStateTransitionType::Immediate, "pResourceBarriers[{}].TransitionType for BLAS '{}' is invalid. xiiGALStateTransitionType::Immediate must be used as split barriers are not supported for BLAS.", uiBarrierIndex, pBottomLevelAS->GetDebugName());
      }
      else if (xiiGALTopLevelAS* pTopLevelAS = xiiDynamicCast<xiiGALTopLevelAS*>(barrier.m_pResource))
      {
        previousState = barrier.m_OldState != xiiGALResourceStateFlags::Unknown ? barrier.m_OldState : pTopLevelAS->GetResourceState();

        XII_ASSERT_DEV(previousState != xiiGALResourceStateFlags::Unknown, "pResourceBarriers[{}].OldState for TLAS '{}' is unknown to the engine and is not explicitly specified in the barrier.", uiBarrierIndex, pTopLevelAS->GetDebugName());
        XII_ASSERT_DEV(barrier.m_NewState == xiiGALResourceStateFlags::BuildASRead || barrier.m_NewState == xiiGALResourceStateFlags::BuildASWrite || barrier.m_NewState == xiiGALResourceStateFlags::RayTracing, "pResourceBarriers[{}].NewState for TLAS '{}' is invalid.", uiBarrierIndex, pTopLevelAS->GetDebugName());
        XII_ASSERT_DEV(barrier.m_TransitionType == xiiGALStateTransitionType::Immediate, "pResourceBarriers[{}].TransitionType for TLAS '{}' is invalid. xiiGALStateTransitionType::Immediate must be used as split barriers are not supported for TLAS.", uiBarrierIndex, pTopLevelAS->GetDebugName());
      }
      else
      {
        XII_REPORT_FAILURE("Unexpected resource type.");
      }

      if (barrier.m_OldState == xiiGALResourceStateFlags::UnorderedAccess && barrier.m_NewState == xiiGALResourceStateFlags::UnorderedAccess)
      {
        XII_ASSERT_DEV(barrier.m_TransitionType == xiiGALStateTransitionType::Immediate, "pResourceBarriers[{}].TransitionType must be xiiGALStateTransitionType::Immediate for UAV barriers.", uiBarrierIndex);
      }

      switch (barrier.m_TransitionType)
      {
        case xiiGALStateTransitionType::Immediate:
          break;
        case xiiGALStateTransitionType::Begin:
          XII_ASSERT_DEV(!barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState), "pResourceBarriers[{}].TransitionFlags can not be updated in a begin-split barrier with xiiGALStateTransitionFlags::UpdateState.", uiBarrierIndex);
          break;
        case xiiGALStateTransitionType::End:
          break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      XII_ASSERT_DEV(VerifyResourceState(barrier.m_OldState, m_Description.m_QueueFlags, "OldState"), "");
      XII_ASSERT_DEV(VerifyResourceState(barrier.m_NewState, m_Description.m_QueueFlags, "NewState"), "");
    }
  }
#endif

  TransitionResourceStatesPlatform(pResourceBarriers);
}

void xiiGALCommandList::EnqueueSignal(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "EnqueueSignal must be called while recording.");
  XII_ASSERT_DEV(pFence != nullptr, "The given fence to signal must not be null.");

  EnqueueSignalPlatform(pFence, uiValue);
}

void xiiGALCommandList::DeviceWaitForFence(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "DeviceWaitForFence must be called while recording.");
  XII_ASSERT_DEV(pFence != nullptr, "The given fence to wait for must not be null.");
  XII_ASSERT_DEV(pFence->GetDescription().m_Type == xiiGALFenceType::General, "The given fence to wait for must be created with xiiGALFenceType::General.");

  DeviceWaitForFencePlatform(pFence, uiValue);
}

void xiiGALCommandList::BeginDebugGroup(xiiStringView sName, const xiiColor& color)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "BeginDebugGroup must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  ++m_uiDebugGroupCount;
#endif

  BeginDebugGroupPlatform(sName, color);
}

void xiiGALCommandList::EndDebugGroup()
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "EndDebugGroup must be called while recording.");

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
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "InsertDebugLabel must be called while recording.");

  InsertDebugLabelPlatform(sName, color);
}

void xiiGALCommandList::UpdateBuffer(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "UpdateBuffer must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiGALGraphicsDeviceAdapterDescription& graphicsAdapterProperties = m_pDevice->GetGraphicsDeviceAdapterProperties();

  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pBuffer != nullptr, "UpdateBuffer arguments are invalid. The buffer handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "UpdateBuffer command must be used outside of render pass.");

  const xiiGALBufferCreationDescription& bufferDescription = pBuffer->GetDescription();

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_CHECK_ALIGNMENT(pSourceData.GetPtr(), graphicsAdapterProperties.m_BufferProperties.m_uiConstantBufferAlignment);
  }
  if (bufferDescription.m_Mode == xiiGALBufferMode::Structured)
  {
    XII_ASSERT_DEV((uiDestinationOffset % graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment) == 0, "Offset must be aligned to {} bytes.", graphicsAdapterProperties.m_BufferProperties.m_uiStructuredBufferOffsetAlignment);
  }

  XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Mutable || bufferDescription.m_Usage == xiiGALResourceUsage::Sparse, "UpdateBuffer command arguments are invalid. Only xiiGALResourceUsage::Mutable or xiiGALResourceUsage::Sparse may be updated with this method.");
  XII_ASSERT_DEV(uiDestinationOffset < bufferDescription.m_uiSize, "UpdateBuffer command arguments are invalid. Unable to update buffer '{0}', the destination offset ({1}) exceeds the buffer size ({2}).", pBuffer->GetDebugName(), uiDestinationOffset, bufferDescription.m_uiSize);
  XII_ASSERT_DEV((uiDestinationOffset + pSourceData.GetCount()) <= bufferDescription.m_uiSize, "UpdateBuffer command arguments are invalid. Unable to update buffer '{0}', the update region [{1}, {2}) is out of buffer bounds [0, {3}).", pBuffer->GetDebugName(), uiDestinationOffset, uiDestinationOffset + pSourceData.GetCount(), bufferDescription.m_uiSize);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiUpdateBuffer;

  UpdateBufferPlatform(pBuffer, uiDestinationOffset, pSourceData);
}

void xiiGALCommandList::CopyBuffer(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyBuffer must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pSourceBuffer != nullptr, "CopyBuffer arguments are invalid. The source buffer handle has been invalidated.");
  XII_ASSERT_DEV(pDestinationBuffer != nullptr, "CopyBuffer arguments are invalid. The destination buffer handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyBuffer command must be used outside of render pass.");

  const xiiGALBufferCreationDescription& sourceBufferDescription      = pSourceBuffer->GetDescription();
  const xiiGALBufferCreationDescription& destinationBufferDescription = pDestinationBuffer->GetDescription();

  XII_ASSERT_DEV(sourceBufferDescription.m_uiSize <= destinationBufferDescription.m_uiSize, "CopyBuffer command arguments are invalid. The source buffer bounds exceeds the bounds of the destination buffer.");
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyBuffer;

  CopyBufferPlatform(pSourceBuffer, pDestinationBuffer);
}

void xiiGALCommandList::CopyBufferRegion(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyBufferRegion must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pSourceBuffer != nullptr, "CopyBufferRegion arguments are invalid. The source buffer handle has been invalidated.");
  XII_ASSERT_DEV(pDestinationBuffer != nullptr, "CopyBufferRegion arguments are invalid. The destination buffer handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyBufferRegion command must be used outside of render pass.");

  const xiiGALBufferCreationDescription& sourceBufferDescription      = pSourceBuffer->GetDescription();
  const xiiGALBufferCreationDescription& destinationBufferDescription = pDestinationBuffer->GetDescription();

  XII_ASSERT_DEV((uiSourceOffset + uiSize) <= sourceBufferDescription.m_uiSize, "CopyBufferRegion command arguments are invalid. Failed to copy buffer '{0}' to '{1}', the destination range [{2}, {3}) is out of buffer bounds [0, {4}).", pSourceBuffer->GetDebugName(), pDestinationBuffer->GetDebugName(), uiSourceOffset, uiSourceOffset + uiSize, sourceBufferDescription.m_uiSize);
  XII_ASSERT_DEV((uiDestinationOffset + uiSize) <= destinationBufferDescription.m_uiSize, "CopyBufferRegion command arguments are invalid. Failed to copy buffer '{0}' to '{1}', the destination range [{2}, {3}) is out of buffer bounds [0, {4}).", pSourceBuffer->GetDebugName(), pDestinationBuffer->GetDebugName(), uiDestinationOffset, uiDestinationOffset + uiSize, destinationBufferDescription.m_uiSize);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyBuffer;

  CopyBufferRegionPlatform(pSourceBuffer, uiSourceOffset, pDestinationBuffer, uiDestinationOffset, uiSize);
}

xiiResult xiiGALCommandList::MapBuffer(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "MapBuffer must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pBuffer != nullptr, "MapBuffer arguments are invalid. The buffer handle has been invalidated.");

  const xiiGALBufferCreationDescription& bufferDescription = pBuffer->GetDescription();

  XII_ASSERT_DEV(!m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has already been mapped.");
  m_MappedBuffers.Insert(pBuffer, mapType);

  switch (mapType)
  {
    case xiiGALMapType::Read:
    {
      XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for reading.");
      XII_ASSERT_DEV(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Buffer being mapped for reading was not created with the xiiGALCPUAccessFlag::Read flag.");
      XII_ASSERT_DEV(!mapFlags.IsSet(xiiGALMapFlags::Discard), "xiiGALMapFlags::Discard is not a valid map flag when mapping a buffer for reading.");
    }
    break;
    case xiiGALMapType::Write:
    {
      XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic || bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Dynamic or xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for writing.");
      XII_ASSERT_DEV(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Buffer being mapped for reading was not created with the xiiGALCPUAccessFlag::Write flag.");
    }
    break;
    case xiiGALMapType::ReadWrite:
    {
      XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified, "Only buffers with xiiGALResourceUsage::Staging or xiiGALResourceUsage::Unified can be mapped for reading and writing.");
      XII_ASSERT_DEV(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read), "Buffer being mapped for reading and writing was not created with the xiiGALCPUAccessFlag::Read flag.");
      XII_ASSERT_DEV(bufferDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write), "Buffer being mapped for reading and writing was not created with the xiiGALCPUAccessFlag::Write flag.");
      XII_ASSERT_DEV(!mapFlags.IsSet(xiiGALMapFlags::Discard), "xiiGALMapFlags::Discard is not a valid map flag when mapping a buffer for reading and writing.");
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic)
  {
    XII_ASSERT_DEV(mapFlags.IsAnySet(xiiGALMapFlags::Discard | xiiGALMapFlags::NoOverWrite) && mapType == xiiGALMapType::Write, "Dynamic buffers can only be mapped for writing with the xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite flag.");
    XII_ASSERT_DEV((mapFlags.IsStrictlyAnySet(xiiGALMapFlags::Discard) || mapFlags.IsStrictlyAnySet(xiiGALMapFlags::NoOverWrite)), "Dynamic buffers can only be mapped for writing with the xiiGALMapFlags::Discard or xiiGALMapFlags::NoOverWrite flag.");
  }

  if (mapFlags.IsSet(xiiGALMapFlags::Discard))
  {
    XII_ASSERT_DEV(bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic || bufferDescription.m_Usage == xiiGALResourceUsage::Staging, "Only buffers with xiiGALResourceUsage::Dynamic or xiiGALResourceUsage::Staging can be mapped with the xiiGALMapFlags::Discard flag.");
    XII_ASSERT_DEV(mapType == xiiGALMapType::Write, "xiiGALMapType::Write is only valid when mapping buffer for writing.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiMapBuffer;

  if (MapBufferPlatform(pBuffer, mapType, mapFlags, pMappedData).Failed())
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    XII_ASSERT_DEV(m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has not been mapped.", pBuffer->GetDebugName());
    XII_ASSERT_DEV(*m_MappedBuffers.GetValue(pBuffer) == mapType, "The map type ({0}) does not match the map type ({1}) that was used to map the buffer.", xiiArgEnum(mapType), xiiArgEnum(*m_MappedBuffers.GetValue(pBuffer)));

    m_MappedBuffers.Remove(pBuffer);
#endif
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALCommandList::UnmapBuffer(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "UnmapBuffer must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pBuffer != nullptr, "MapBuffer arguments are invalid. The buffer handle has been invalidated.");
  XII_ASSERT_DEV(m_MappedBuffers.Contains(pBuffer), "The buffer '{0}' has not been mapped.", pBuffer->GetDebugName());
  XII_ASSERT_DEV(*m_MappedBuffers.GetValue(pBuffer) == mapType, "The map type ({0}) does not match the map type ({1}) that was used to map the buffer.", xiiArgEnum(mapType), xiiArgEnum(*m_MappedBuffers.GetValue(pBuffer)));

  m_MappedBuffers.Remove(pBuffer);
#endif

  return UnmapBufferPlatform(pBuffer, mapType);
}

void xiiGALCommandList::UpdateTexture(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "UpdateTexture must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pTexture != nullptr, "UpdateTexture arguments are invalid. The texture handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "UpdateTexture command must be used outside of render pass.");

  ValidateTextureUpdateRegion(pTexture->GetDescription(), textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureBox, subresourceData);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiUpdateTexture;

  UpdateTexturePlatform(pTexture, textureMiplevelData, textureBox, subresourceData);
}

void xiiGALCommandList::CopyTexture(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyTexture must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pSourceTexture != nullptr, "CopyTexture arguments are invalid. The source texture handle has been invalidated.");
  XII_ASSERT_DEV(pDestinationTexture != nullptr, "CopyTexture arguments are invalid. The destination texture handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyTexture command must be used outside of render pass.");

  xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(pSourceTexture->GetDescription(), 0);
  xiiBoundingBoxU32        sourceBox          = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), xiiVec3U32(mipLevelProperties.m_LogicalSize.width, mipLevelProperties.m_LogicalSize.height, mipLevelProperties.m_uiDepth));

  ValidateTextureRegion(pSourceTexture->GetDescription(), 0, 0, sourceBox);
  ValidateTextureRegion(pDestinationTexture->GetDescription(), 0, 0, sourceBox);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyTexture;

  CopyTexturePlatform(pSourceTexture, pDestinationTexture);
}

void xiiGALCommandList::CopyTextureRegion(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "CopyTextureRegion must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Transfer), "The command list does not have the xiiGALCommandQueueFlags::Transfer flag.");
  XII_ASSERT_DEV(pSourceTexture != nullptr, "CopyTextureRegion arguments are invalid. The source texture handle has been invalidated.");
  XII_ASSERT_DEV(pDestinationTexture != nullptr, "CopyTextureRegion arguments are invalid. The destination texture handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "CopyTextureRegion command must be used outside of render pass.");

  ValidateTextureRegion(pSourceTexture->GetDescription(), destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, box);

  xiiBoundingBoxU32 destinationBox = xiiBoundingBoxU32::MakeFromMinMax(vDestinationPoint, vDestinationPoint + box.GetExtents());

  ValidateTextureRegion(pDestinationTexture->GetDescription(), destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, destinationBox);
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiCopyTexture;

  CopyTextureRegionPlatform(pSourceTexture, sourceMipLevelData, box, pDestinationTexture, destinationMipLevelData, vDestinationPoint);
}

void xiiGALCommandList::ResolveTextureSubResource(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "ResolveTextureSubresource must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pSourceTexture != nullptr, "ResolveTextureSubResource arguments are invalid. The source texture handle has been invalidated.");
  XII_ASSERT_DEV(pDestinationTexture != nullptr, "ResolveTextureSubResource arguments are invalid. The destination texture handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "ResolveTextureSubResource command must be used outside of render pass.");

  const xiiGALTextureCreationDescription& sourceTextureDescription      = pSourceTexture->GetDescription();
  const xiiGALTextureCreationDescription& destinationTextureDescription = pDestinationTexture->GetDescription();

  XII_ASSERT_DEV(sourceTextureDescription.m_uiSampleCount > 1U, "ResolveTextureSubResource arguments are invalid: source texture '{}' of a resolve operation is not multi-sampled.", pSourceTexture->GetDebugName());
  XII_ASSERT_DEV(destinationTextureDescription.m_uiSampleCount == 1U, "ResolveTextureSubResource arguments are invalid: destination texture '{}' of a resolve operation is multi-sampled.", pDestinationTexture->GetDebugName());

  xiiGALMipLevelProperties sourceMipLevelProperties      = xiiGALTextureUtilities::GetMipLevelProperties(sourceTextureDescription, description.m_uiSourceMipLevel);
  xiiGALMipLevelProperties destinationMipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(destinationTextureDescription, description.m_uiDestinationMipLevel);

  XII_ASSERT_DEV(sourceMipLevelProperties.m_LogicalSize == destinationMipLevelProperties.m_LogicalSize, "ResolveTextureSubResource arguments are invalid: the size ({}x{}) of the source subresource of a resolve operation (texture '{}', mip {}, slice {}) does not match the size ({}x{}) of the destination subresource (texture '{}', mip {}, slice {}).",
                 sourceMipLevelProperties.m_LogicalSize.width, sourceMipLevelProperties.m_LogicalSize.height, pSourceTexture->GetDebugName(), description.m_uiSourceMipLevel, description.m_uiSourceSlice, destinationMipLevelProperties.m_LogicalSize.width, destinationMipLevelProperties.m_LogicalSize.height, pDestinationTexture->GetDebugName(), description.m_uiDestinationMipLevel, description.m_uiDestinationSlice);

  const xiiGALResourceFormatDescription& sourceFormatProperties      = xiiGALTextureUtilities::GetResourceFormatProperties(sourceTextureDescription.m_Format);
  const xiiGALResourceFormatDescription& destinationFormatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(destinationTextureDescription.m_Format);
  const xiiGALResourceFormatDescription& resolveFormatProperties     = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);

  if (!sourceFormatProperties.m_bIsTypeless && !destinationFormatProperties.m_bIsTypeless)
  {
    XII_ASSERT_DEV(sourceTextureDescription.m_Format == destinationTextureDescription.m_Format, "ResolveTextureSubResource arguments are invalid: source ({}) and destination ({}) texture formats of a resolve operation must match exactly or be compatible typeless formats.");
    XII_ASSERT_DEV(description.m_Format == xiiGALResourceFormat::Unknown || sourceTextureDescription.m_Format == description.m_Format, "ResolveTextureSubResource arguments are invalid: invalid format of a resolve operation.");
  }
  if (sourceFormatProperties.m_bIsTypeless && destinationFormatProperties.m_bIsTypeless)
  {
    XII_ASSERT_DEV(description.m_Format != xiiGALResourceFormat::Unknown, "ResolveTextureSubResource arguments are invalid: format of a resolve operation must not be unknown when both source and destination texture formats are typeless.");
  }
  if (sourceFormatProperties.m_bIsTypeless || destinationFormatProperties.m_bIsTypeless)
  {
    XII_ASSERT_DEV(!resolveFormatProperties.m_bIsTypeless, "ResolveTextureSubResource arguments are invalid: format of a resolve operation must not be typeless when one of the texture formats is typeless.");
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiResolveTextureSubresource;

  ResolveTextureSubResourcePlatform(pSourceTexture, pDestinationTexture, description);
}

void xiiGALCommandList::GenerateMips(xiiGALTextureView* pTextureView)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "GenerateMips must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(pTextureView != nullptr, "GenerateMips arguments are invalid. The texture view handle has been invalidated.");
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "GenerateMips command must be used outside of render pass.");

  const xiiGALTextureViewCreationDescription& textureViewDescription = pTextureView->GetDescription();

  XII_ASSERT_DEV(textureViewDescription.m_ViewType == xiiGALTextureViewType::ShaderResource, "GenerateMips arguments are invalid. Texture view '{0}' is not of type xiiGALTextureViewType::ShaderResource.", pTextureView->GetDebugName());
  XII_ASSERT_DEV(textureViewDescription.m_Flags.IsSet(xiiGALTextureViewFlags::AllowMipGeneration), "GenerateMips arguments are invalid. Texture view '{0}' does not have xiiGALTextureViewFlags::AllowMipGeneration flag.", pTextureView->GetDebugName());
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiGenerateMips;

  GenerateMipsPlatform(pTextureView);
}

xiiResult xiiGALCommandList::MapTextureSubresource(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "MapTextureSubresource must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pTexture != nullptr, "MapTextureSubresource arguments are invalid. The texture handle has been invalidated.");

  const xiiGALTextureCreationDescription& textureDescription = pTexture->GetDescription();

  XII_ASSERT_DEV(textureMipLevelData.m_uiMipLevel < textureDescription.m_uiMipLevels, "Mip level ({}) is out of permitted range [0, {}].", textureMipLevelData.m_uiMipLevel, textureDescription.m_uiMipLevels - 1);

  if (textureDescription.IsArray())
  {
    XII_ASSERT_DEV(textureMipLevelData.m_uiArraySlice < textureDescription.m_uiArraySizeOrDepth, "Array slice ({}) is out of permitted range [0, {}].", textureMipLevelData.m_uiArraySlice, textureDescription.m_uiArraySizeOrDepth - 1);
  }
  else
  {
    XII_ASSERT_DEV(textureMipLevelData.m_uiArraySlice == 0, "Array slice ({}) must be 0 for non-array textures.", textureMipLevelData.m_uiArraySlice);
  }

  if (pTextureBox != nullptr)
  {
    ValidateTextureRegion(textureDescription, textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, *pTextureBox);
  }
#endif

  ++m_CommandListStatistics.m_CommandListCounters.m_uiMapTextureSubresource;

  return MapTextureSubresourcePlatform(pTexture, textureMipLevelData, mapType, mapFlags, pTextureBox, mappedData);
}

xiiResult xiiGALCommandList::UnmapTextureSubresource(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "UnmapTextureSubresource must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(pTexture != nullptr, "MapTextureSubresource arguments are invalid. The texture handle has been invalidated.");
  XII_ASSERT_DEV(textureMipLevelData.m_uiMipLevel < pTexture->GetDescription().m_uiMipLevels, "MapTextureSubresource arguments are invalid. The mip level is out of range.");
  XII_ASSERT_DEV(textureMipLevelData.m_uiArraySlice < pTexture->GetDescription().m_uiArraySizeOrDepth, "MapTextureSubresource arguments are invalid. The array slice is out of range.");
#endif

  return UnmapTextureSubresourcePlatform(pTexture, textureMipLevelData);
}

void xiiGALCommandList::SetShadingRate(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags)
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Recording, "SetShadingRate must be called while recording.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_Description.m_QueueFlags.IsSet(xiiGALCommandQueueFlags::Graphics), "The command list does not have the xiiGALCommandQueueFlags::Graphics flag.");
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(primitiveCombinerFlags.GetValue()), "Primitive combiner flags ({}) must represent a single combiner mode.", xiiArgEnum(primitiveCombinerFlags));
  XII_ASSERT_DEV(xiiMath::IsPowerOf2(textureCombinerFlags.GetValue()), "Texture combiner flags ({}) must represent a single combiner mode.", xiiArgEnum(textureCombinerFlags));
  XII_ASSERT_DEV(m_pDevice->GetGraphicsDeviceAdapterProperties().m_Features.m_VariableRateShading == xiiGALDeviceFeatureState::Enabled, "xiiGALCommandList::SetShadingRate requires VariableRateShading feature support on the device.");

  const xiiGALShadingRateProperties&                  shadingRateProperties = m_pDevice->GetGraphicsDeviceAdapterProperties().m_ShadingRateProperties;
  const xiiBitflags<xiiGALShadingRateCapabilityFlags> requiredCapabilities  = xiiGALShadingRateCapabilityFlags::PerDraw | xiiGALShadingRateCapabilityFlags::PerPrimitive | xiiGALShadingRateCapabilityFlags::TextureBased;
  XII_ASSERT_DEV(shadingRateProperties.m_CapabilityFlags.IsAnySet(requiredCapabilities), "xiiGALCommandList::SetShadingRate requires one of the following capabilities: xiiGALShadingRateCapabilityFlags::PerDraw, or xiiGALShadingRateCapabilityFlags::PerPrimitive, or xiiGALShadingRateCapabilityFlags::TextureBased");

  if (shadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::PerPrimitive))
  {
    XII_ASSERT_DEV(primitiveCombinerFlags.IsAnySet(primitiveCombinerFlags), "Primitive combiner flags ({}) must contain at least one primitive combiner flag when the device supports per-primitive shading rates.", xiiArgEnum(primitiveCombinerFlags));
  }
  else
  {
    XII_ASSERT_DEV(primitiveCombinerFlags == xiiGALShadingRateCombinerFlags::PassThrough, "xiiGALCommandList::SetShadingRate requires primitive combiner to be xiiGALShadingRateCombinerFlags::PassThrough when per-primitive shading is not supported.");
  }

  if (shadingRateProperties.m_CapabilityFlags.IsSet(xiiGALShadingRateCapabilityFlags::TextureBased))
  {
    XII_ASSERT_DEV(textureCombinerFlags.IsAnySet(textureCombinerFlags), "Texture combiner flags ({}) must contain at least one texture combiner flag when the device supports texture-based shading rates.", xiiArgEnum(textureCombinerFlags));
  }
  else
  {
    XII_ASSERT_DEV(textureCombinerFlags == xiiGALShadingRateCombinerFlags::PassThrough, "xiiGALCommandList::SetShadingRate requires texture combiner to be xiiGALShadingRateCombinerFlags::PassThrough when texture-based shading is not supported.");
  }

  bool bIsSupportedRate = false;
  for (const xiiGALShadingRateMode& mode : shadingRateProperties.m_Modes)
  {
    if (mode.m_ShadingRate == baseRateFlags)
    {
      bIsSupportedRate = true;
      break;
    }
  }
  XII_ASSERT_DEV(bIsSupportedRate, "xiiGALCommandList::SetShadingRate: Base shading rate flags ({}) are not supported by the device.", xiiArgEnum(baseRateFlags));
#endif

  SetShadingRatePlatform(baseRateFlags, primitiveCombinerFlags, textureCombinerFlags);
}

void xiiGALCommandList::InvalidateState()
{
  XII_ASSERT_DEV(m_pRenderPass == nullptr, "Invalidating the command list is disallowed while a render pass is active. Call EndRenderPass to finish the pass.");

  m_pPipelineState             = nullptr;
  m_pPipelineResourceSignature = nullptr;

  m_VertexStreams.Clear();

  m_pIndexBuffer      = nullptr;
  m_uiIndexDataOffset = 0;

  m_pRenderPass  = nullptr;
  m_pFramebuffer = nullptr;

  m_BlendFactors = xiiColor::Black;
  m_uiStencilRef = 0;

  m_Viewports.Clear();
  m_ScissorRects.Clear();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_MappedBuffers.IsEmpty(), "Mapped buffers have not yet been released.");
#endif

  InvalidateStatePlatform();
}

bool xiiGALCommandList::VerifyResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, const char* szParameterName) const
{
  XII_IGNORE_UNUSED(szParameterName);

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
        if (!queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, xiiArgEnum(queueFlags));
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
        if (!queueFlags.IsSet(xiiGALCommandQueueFlags::Compute))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, xiiArgEnum(queueFlags));
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
        if (!queueFlags.IsSet(xiiGALCommandQueueFlags::Graphics))
        {
          bResult = false;
          XII_ASSERT_DEV("{} contains state '{}' that is not supported in {} queue.", szParameterName, state, xiiArgEnum(queueFlags));
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
#define XII_VERIFY_EXCLUSIVE_STATE(exclusiveState)                                                                                                                                                                 \
  if (stateFlags.IsSet(xiiGALResourceStateFlags::exclusiveState) && !stateFlags.IsStrictlyAnySet((xiiGALResourceStateFlags::exclusiveState)))                                                                      \
  {                                                                                                                                                                                                                \
    xiiLog::Error("State {} is invalid: {} can not be combined with any other state.", xiiArgEnum(stateFlags), xiiArgEnum(xiiBitflags<xiiGALResourceStateFlags>(xiiGALResourceStateFlags::exclusiveState), true)); \
    return false;                                                                                                                                                                                                  \
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
      xiiLog::Error("State {} is invalid: states xiiGALResourceStateFlags::VertexBuffer, xiiGALResourceStateFlags::ConstantBuffer, xiiGALResourceStateFlags::IndexBuffer, xiiGALResourceStateFlags::StreamOut, xiiGALResourceStateFlags::IndirectArgument are not applicable to textures.", xiiArgEnum(stateFlags));
      return false;
    }
  }
  else
  {
    if (stateFlags.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead | xiiGALResourceStateFlags::ResolveSource | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::Present | xiiGALResourceStateFlags::ShadingRate | xiiGALResourceStateFlags::InputAttachment))
    {
      xiiLog::Error("State {} is invalid: states xiiGALResourceStateFlags::RenderTarget, xiiGALResourceStateFlags::DepthWrite, xiiGALResourceStateFlags::DepthRead, xiiGALResourceStateFlags::ResolveSource, xiiGALResourceStateFlags::ResolveDestination, xiiGALResourceStateFlags::Present, xiiGALResourceStateFlags::ShadingRate, xiiGALResourceStateFlags::InputAttachment are not applicable to buffers.", xiiArgEnum(stateFlags));
      return false;
    }
  }

  return true;
}

void xiiGALCommandList::VerifyBufferState(const xiiGALBuffer* pBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const
{
  if (pBuffer == nullptr)
    return;

  if (pBuffer->IsInKnownState() && !pBuffer->CheckState(requiredState))
  {
    xiiLog::Error("{} requires buffer '{}' to be transitioned to {} state. Actual buffer state: {}. Use appropriate state transition flags or explicitly transition the buffer using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pBuffer->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pBuffer->GetResourceState()));
  }
}

void xiiGALCommandList::VerifyTextureState(const xiiGALTexture* pTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const
{
  if (pTexture == nullptr)
    return;

  if (pTexture->IsInKnownState() && !pTexture->CheckState(requiredState))
  {
    xiiLog::Error("{} requires texture '{}' to be transitioned to {} state. Actual texture state: {}. Use appropriate state transition flags or explicitly transition the texture using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pTexture->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pTexture->GetResourceState()));
  }
}

void xiiGALCommandList::VerifyBottomLevelASState(const xiiGALBottomLevelAS* pBottomLevelAS, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const
{
  if (pBottomLevelAS == nullptr)
    return;

  if (pBottomLevelAS->IsInKnownState() && !pBottomLevelAS->CheckState(requiredState))
  {
    xiiLog::Error("{} requires bottom-level acceleration structure '{}' to be transitioned to {} state. Actual bottom-level acceleration structure state: {}. Use appropriate state transition flags or explicitly transition the bottom-level acceleration structure using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pBottomLevelAS->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pBottomLevelAS->GetResourceState()));
  }
}

void xiiGALCommandList::VerifyTopLevelASState(const xiiGALTopLevelAS* pTopLevelAS, xiiBitflags<xiiGALResourceStateFlags> requiredState, const char* szOperationName) const
{
  if (pTopLevelAS == nullptr)
    return;

  if (pTopLevelAS->IsInKnownState() && !pTopLevelAS->CheckState(requiredState))
  {
    xiiLog::Error("{} requires top-level acceleration structure '{}' to be transitioned to {} state. Actual top-level acceleration structure state: {}. Use appropriate state transition flags or explicitly transition the top-level acceleration structure using xiiGALCommandList::TransitionResourceStates() method.", szOperationName, pTopLevelAS->GetDebugName(), xiiArgEnum(requiredState), xiiArgEnum(pTopLevelAS->GetResourceState()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////

void xiiGALCommandListStatistics::SetStatistics()
{
  // clang-format off
  xiiStats::SetStat("CommandList/Submit", m_CommandListCounters.m_uiSubmit);

  // Primitive statistics, counts for geometry-related commands.
  xiiStats::SetStat("CommandList/Primitives/TotalTriangleCount", GetTotalTriangleCount());
  xiiStats::SetStat("CommandList/Primitives/TotalLineCount",     GetTotalLineCount());
  xiiStats::SetStat("CommandList/Primitives/TotalPointCount",    GetTotalPointCount());

  // Pipeline management, configuration of rendering pipeline.
  xiiStats::SetStat("CommandList/Pipeline/StateManagement/SetPipelineState",      m_CommandListCounters.m_uiSetPipelineState);
  xiiStats::SetStat("CommandList/Pipeline/StateManagement/CommitShaderResources", m_CommandListCounters.m_uiCommitShaderResources);
  xiiStats::SetStat("CommandList/Pipeline/StateManagement/SetVertexBuffers",      m_CommandListCounters.m_uiSetVertexBuffers);
  xiiStats::SetStat("CommandList/Pipeline/StateManagement/SetIndexBuffer",        m_CommandListCounters.m_uiSetIndexBuffer);
  xiiStats::SetStat("CommandList/Pipeline/Blending/SetBlendFactors",              m_CommandListCounters.m_uiSetBlendFactors);
  xiiStats::SetStat("CommandList/Pipeline/Stencil/SetStencilRef",                 m_CommandListCounters.m_uiSetStencilRef);
  xiiStats::SetStat("CommandList/Pipeline/Viewports/SetViewports",                m_CommandListCounters.m_uiSetViewports);
  xiiStats::SetStat("CommandList/Pipeline/Viewports/SetScissorRects",             m_CommandListCounters.m_uiSetScissorRects);
  xiiStats::SetStat("CommandList/Pipeline/Passes/BeginRenderPass",                m_CommandListCounters.m_uiBeginRenderPass);
  xiiStats::SetStat("CommandList/Pipeline/Passes/NextSubPass",                    m_CommandListCounters.m_uiNextSubPass);
  xiiStats::SetStat("CommandList/Pipeline/Clears/ClearRenderTarget",              m_CommandListCounters.m_uiClearRenderTarget);
  xiiStats::SetStat("CommandList/Pipeline/Clears/ClearDepthStencil",              m_CommandListCounters.m_uiClearDepthStencil);

  // Drawing commands, rendering-related commands.
  xiiStats::SetStat("CommandList/Draw/Draw",                           m_CommandListCounters.m_uiDraw);
  xiiStats::SetStat("CommandList/Draw/DrawIndexed",                    m_CommandListCounters.m_uiDrawIndexed);
  xiiStats::SetStat("CommandList/Draw/DrawIndirect",                   m_CommandListCounters.m_uiDrawIndirect);
  xiiStats::SetStat("CommandList/Draw/DrawIndexedIndirect",            m_CommandListCounters.m_uiDrawIndexedIndirect);
  xiiStats::SetStat("CommandList/Draw/MultiDraw",                      m_CommandListCounters.m_uiMultiDraw);
  xiiStats::SetStat("CommandList/Draw/MultiDrawIndexed",               m_CommandListCounters.m_uiMultiDrawIndexed);
  xiiStats::SetStat("CommandList/Draw/MeshRendering/DrawMesh",         m_CommandListCounters.m_uiDrawMesh);
  xiiStats::SetStat("CommandList/Draw/MeshRendering/DrawMeshIndirect", m_CommandListCounters.m_uiDrawMeshIndirect);

  // Compute dispatch, execution of compute workloads.
  xiiStats::SetStat("CommandList/Compute/DispatchCompute",         m_CommandListCounters.m_uiDispatchCompute);
  xiiStats::SetStat("CommandList/Compute/DispatchComputeIndirect", m_CommandListCounters.m_uiDispatchComputeIndirect);
  xiiStats::SetStat("CommandList/Compute/TileDispatch",            m_CommandListCounters.m_uiDispatchTile);

  // Operations and memory management, other GPU tasks.
  xiiStats::SetStat("CommandList/Memory/BLAS/Build",                                     m_CommandListCounters.m_uiBuildBLAS);
  xiiStats::SetStat("CommandList/Memory/BLAS/Copy",                                      m_CommandListCounters.m_uiCopyBLAS);
  xiiStats::SetStat("CommandList/Memory/BLAS/WriteCompactedSize",                        m_CommandListCounters.m_uiWriteBLASCompactedSize);
  xiiStats::SetStat("CommandList/Memory/TLAS/Build",                                     m_CommandListCounters.m_uiBuildTLAS);
  xiiStats::SetStat("CommandList/Memory/TLAS/Copy",                                      m_CommandListCounters.m_uiCopyTLAS);
  xiiStats::SetStat("CommandList/Memory/TLAS/WriteCompactedSize",                        m_CommandListCounters.m_uiWriteTLASCompactedSize);
  xiiStats::SetStat("CommandList/RayTracing/TraceRays",                                  m_CommandListCounters.m_uiTraceRays);
  xiiStats::SetStat("CommandList/RayTracing/TraceRaysIndirect",                          m_CommandListCounters.m_uiTraceRaysIndirect);
  xiiStats::SetStat("CommandList/RayTracing/UpdateSBT",                                  m_CommandListCounters.m_uiUpdateSBT);
  xiiStats::SetStat("CommandList/Resources/BufferManagement/UpdateBuffer",               m_CommandListCounters.m_uiUpdateBuffer);
  xiiStats::SetStat("CommandList/Resources/BufferManagement/CopyBuffer",                 m_CommandListCounters.m_uiCopyBuffer);
  xiiStats::SetStat("CommandList/Resources/BufferManagement/MapBuffer",                  m_CommandListCounters.m_uiMapBuffer);
  xiiStats::SetStat("CommandList/Resources/TextureManagement/UpdateTexture",             m_CommandListCounters.m_uiUpdateTexture);
  xiiStats::SetStat("CommandList/Resources/TextureManagement/CopyTexture",               m_CommandListCounters.m_uiCopyTexture);
  xiiStats::SetStat("CommandList/Resources/TextureManagement/MapTextureSubresource",     m_CommandListCounters.m_uiMapTextureSubresource);
  xiiStats::SetStat("CommandList/Resources/TextureManagement/GenerateMips",              m_CommandListCounters.m_uiGenerateMips);
  xiiStats::SetStat("CommandList/Resources/TextureManagement/ResolveTextureSubresource", m_CommandListCounters.m_uiResolveTextureSubresource);
  xiiStats::SetStat("CommandList/Queries/BeginQuery",                                    m_CommandListCounters.m_uiBeginQuery);
  xiiStats::SetStat("CommandList/SparseMemory/BindSparseResourceMemory",                 m_CommandListCounters.m_uiBindSparseResourceMemory);
  // clang-format on
}

////////////////////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);
