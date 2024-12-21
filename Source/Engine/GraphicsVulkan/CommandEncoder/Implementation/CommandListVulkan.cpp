#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

xiiGALCommandListVulkan::xiiGALCommandListVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceVulkan, pCommandQueueVulkan, creationDescription)
{
  vk::CommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
  vkCommandBufferAllocateInfo.pNext                         = nullptr;
  vkCommandBufferAllocateInfo.commandPool                   = pCommandQueueVulkan->GetVulkanCommandPool();
  vkCommandBufferAllocateInfo.level                         = vk::CommandBufferLevel::ePrimary;
  vkCommandBufferAllocateInfo.commandBufferCount            = 1U;

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanLogicalDevice().allocateCommandBuffers(&vkCommandBufferAllocateInfo, &m_vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
}

xiiGALCommandListVulkan::~xiiGALCommandListVulkan()
{
  xiiGALDeviceVulkan*       pDeviceVulkan       = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(m_pCommandQueue);

  pDeviceVulkan->GetVulkanLogicalDevice().freeCommandBuffers(pCommandQueueVulkan->GetVulkanCommandPool(), 1U, &m_vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::BeginPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_ASSERT_DEV(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  vk::CommandBufferBeginInfo vkCommandBufferBeginInfo = {};
  vkCommandBufferBeginInfo.pNext                      = nullptr;
  vkCommandBufferBeginInfo.flags                      = vk::CommandBufferUsageFlagBits::eOneTimeSubmit; // Each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission.
  vkCommandBufferBeginInfo.pInheritanceInfo           = nullptr;                                        // Ignored for a primary command buffer.

  VK_ASSERT_DEV(m_vkCommandBuffer.begin(&vkCommandBufferBeginInfo, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_RecordingState = RecordingState::Recording;

  if (xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(GetCommandQueue()))
  {
    pCommandQueueVulkan->BeginCommandList(this);
  }
}

void xiiGALCommandListVulkan::EndPlatform()
{
  XII_ASSERT_DEV(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.end(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_RecordingState = RecordingState::Ended;
}

void xiiGALCommandListVulkan::ResetPlatform()
{
  XII_ASSERT_DEV(m_vkCommandBuffer != VK_NULL_HANDLE, "");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  InvalidateState();

  m_RecordingState = RecordingState::Reset;

  if (xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(GetCommandQueue()))
  {
    pCommandQueueVulkan->ResetCommandList(this);
  }
}

xiiUInt64 xiiGALCommandListVulkan::SubmitPlatform(bool bReset)
{
  if (xiiGALCommandQueueVulkan* pCommandQueueVulkan = static_cast<xiiGALCommandQueueVulkan*>(GetCommandQueue()))
  {
    return pCommandQueueVulkan->SubmitCommandList(this, bReset);
  }
  return xiiMath::MaxValue<xiiUInt64>();
}

void xiiGALCommandListVulkan::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
}

void xiiGALCommandListVulkan::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, uiStencilRef, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.setBlendConstants(blendFactor.GetData(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_ASSERT_DEV(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::Viewport vkViewPorts[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiViewPortIndex = 0; uiViewPortIndex < pViewports.GetCount(); ++uiViewPortIndex)
  {
    vkViewPorts[uiViewPortIndex].x        = pViewports[uiViewPortIndex].m_fTopLeftX;
    vkViewPorts[uiViewPortIndex].y        = pViewports[uiViewPortIndex].m_fTopLeftY;
    vkViewPorts[uiViewPortIndex].width    = pViewports[uiViewPortIndex].m_fWidth;
    vkViewPorts[uiViewPortIndex].height   = pViewports[uiViewPortIndex].m_fHeight;
    vkViewPorts[uiViewPortIndex].minDepth = pViewports[uiViewPortIndex].m_fMinDepth;
    vkViewPorts[uiViewPortIndex].maxDepth = pViewports[uiViewPortIndex].m_fMaxDepth;

    // Turn the viewport upside down to be consistent with Direct3D. Note that in both APIs, the viewport covers the same texture rows. The difference is that Direct3D inverts
    // normalized device Y coordinate when transforming NDC to window coordinates. In Vulkan, we achieve the same effect by using negative viewport height. Therefore we need to
    // invert normalized device Y coordinate when transforming to texture V.
    //
    //
    //       Image                Direct3D                                       Image               Vulkan
    //        row                                                                 row
    //         0 _   (0,0)_______________________(1,0)                  Tex Height _   (0,1)_______________________(1,1)
    //         1 _       |                       |      |             VP Top + Hght _ _ _ _|   __________          |      A
    //         2 _       |                       |      |                          .       |  |   .--> +x|         |      |
    //           .       |                       |      |                          .       |  |   |      |         |      |
    //           .       |                       |      | V Coord                          |  |   V +y   |         |      | V Coord
    //     VP Top _ _ _ _|   __________          |      |                    VP Top _ _ _ _|  |__________|         |      |
    //           .       |  |    A +y  |         |      |                          .       |                       |      |
    //           .       |  |    |     |         |      |                          .       |                       |      |
    //           .       |  |    '-->+x|         |      |                        2 _       |                       |      |
    //           .       |  |__________|         |      |                        1 _       |                       |      |
    //Tex Height _       |_______________________|      V                        0 _       |_______________________|      |
    //               (0,1)                       (1,1)                                 (0,0)                       (1,0)
    //
    //

    vkViewPorts[uiViewPortIndex].y      = vkViewPorts[uiViewPortIndex].y + vkViewPorts[uiViewPortIndex].height;
    vkViewPorts[uiViewPortIndex].height = -vkViewPorts[uiViewPortIndex].height;
  }

  m_vkCommandBuffer.setViewport(0, m_Viewports.GetCount(), vkViewPorts, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects, xiiUInt32 uiRenderTargetWidth, xiiUInt32 uiRenderTargetHeight)
{
  XII_ASSERT_DEV(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  vk::Rect2D vkScissorRects[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiScissorRectIndex = 0; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    vkScissorRects[uiScissorRectIndex].offset = vk::Offset2D{static_cast<xiiInt32>(pRects[uiScissorRectIndex].x), static_cast<xiiInt32>(pRects[uiScissorRectIndex].y)};
    vkScissorRects[uiScissorRectIndex].extent = vk::Extent2D{pRects[uiScissorRectIndex].width, pRects[uiScissorRectIndex].height};
  }

  m_vkCommandBuffer.setScissor(0, m_ScissorRects.GetCount(), vkScissorRects, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
}

void xiiGALCommandListVulkan::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
}

void xiiGALCommandListVulkan::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
}

void xiiGALCommandListVulkan::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

void xiiGALCommandListVulkan::BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
}

void xiiGALCommandListVulkan::NextSubpassPlatform()
{
}

void xiiGALCommandListVulkan::EndRenderPassPlatform()
{
}

xiiResult xiiGALCommandListVulkan::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.draw(uiVertexCount, 1U, uiStartVertex, 0, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.drawIndexed(uiIndexCount, 1U, uiStartIndex, uiBaseVertex, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  m_vkCommandBuffer.drawIndexedIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, 0U, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  m_vkCommandBuffer.drawIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, 0U, 0U, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.drawMeshTasksEXT(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan = static_cast<xiiGALBufferVulkan*>(pIndirectArgumentBuffer);

  m_vkCommandBuffer.dispatchIndirect(pBufferVulkan->GetVulkanBuffer(), uiArgumentOffsetInBytes, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListVulkan::EndQueryPlatform(xiiGALQuery* pQuery)
{
}

void xiiGALCommandListVulkan::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
}

void xiiGALCommandListVulkan::UpdateBufferExtendedPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags, bool bCopyToTemporaryStorage)
{
}

void xiiGALCommandListVulkan::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
}

void xiiGALCommandListVulkan::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
}

xiiResult xiiGALCommandListVulkan::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListVulkan::UpdateTextureExtendedPlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListVulkan::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
}

void xiiGALCommandListVulkan::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
}

void xiiGALCommandListVulkan::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
}

void xiiGALCommandListVulkan::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
}

xiiResult xiiGALCommandListVulkan::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  return XII_SUCCESS;
}

void xiiGALCommandListVulkan::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  xiiStringBuilder tmp;

  vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
  vkDebugUtilsLabel.pNext                  = nullptr;
  vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
  vkDebugUtilsLabel.color[0]               = color.r;
  vkDebugUtilsLabel.color[1]               = color.g;
  vkDebugUtilsLabel.color[2]               = color.b;
  vkDebugUtilsLabel.color[3]               = color.a;

  m_vkCommandBuffer.beginDebugUtilsLabelEXT(vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::EndDebugGroupPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkCommandBuffer.endDebugUtilsLabelEXT(pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  xiiStringBuilder tmp;

  vk::DebugUtilsLabelEXT vkDebugUtilsLabel = {};
  vkDebugUtilsLabel.pNext                  = nullptr;
  vkDebugUtilsLabel.pLabelName             = sName.GetData(tmp);
  vkDebugUtilsLabel.color[0]               = color.r;
  vkDebugUtilsLabel.color[1]               = color.g;
  vkDebugUtilsLabel.color[2]               = color.b;
  vkDebugUtilsLabel.color[3]               = color.a;

  m_vkCommandBuffer.insertDebugUtilsLabelEXT(vkDebugUtilsLabel, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandListVulkan::FlushPlatform()
{
}

void xiiGALCommandListVulkan::InvalidateStatePlatform()
{
}

void xiiGALCommandListVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandBuffer, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandListVulkan);
