#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandEncoderD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

#include <Diligent/Graphics/GraphicsEngineD3D12/interface/BufferViewD3D12.h>
#include <Diligent/Graphics/GraphicsEngineD3D12/interface/DeviceContextD3D12.h>
#include <Diligent/Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h>
#include <Diligent/Graphics/GraphicsEngineD3D12/interface/TextureViewD3D12.h>

#include <GraphicsD3D12/Utilities/DiligentTypeConversions.h>

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALTextureViewHandle));

namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiGALTextureViewHandle& Value)
  {
    Stream << reinterpret_cast<const xiiUInt32&>(Value);
    return Stream;
  }

  XII_ALWAYS_INLINE bool operator==(const Diligent::Rect& lhs, const Diligent::Rect& rhs)
  {
    return lhs.top == rhs.top && lhs.left == rhs.left && lhs.right == rhs.right && lhs.bottom == rhs.bottom;
  };

  XII_ALWAYS_INLINE bool operator!=(const Diligent::Rect& lhs, const Diligent::Rect& rhs)
  {
    return !(lhs == rhs);
  };

  XII_ALWAYS_INLINE bool operator==(const Diligent::Viewport& lhs, const Diligent::Viewport& rhs)
  {
    return lhs.TopLeftX == rhs.TopLeftX && lhs.TopLeftY == rhs.TopLeftY && lhs.Width == rhs.Width && lhs.Height == rhs.Height && lhs.MinDepth == rhs.MinDepth && lhs.MaxDepth == rhs.MaxDepth;
  };

  XII_ALWAYS_INLINE bool operator!=(const Diligent::Viewport& lhs, const Diligent::Viewport& rhs)
  {
    return !(lhs == rhs);
  };
} // namespace

xiiGALCommandEncoderD3D12::xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12) :
  m_GALDeviceD3D12(deviceD3D12), m_pContext(deviceD3D12.GetImmediateContext())
{
  // Create synchronization fence.
  Diligent::FenceDesc synchronizationFenceDescription;
  synchronizationFenceDescription.Name = "Command Encoder D3D12 Readback Fence.";
  synchronizationFenceDescription.Type = Diligent::FENCE_TYPE_GENERAL;
  m_GALDeviceD3D12.GetDevice()->CreateFence(synchronizationFenceDescription, &m_pSynchronizationFence);
}

xiiGALCommandEncoderD3D12::~xiiGALCommandEncoderD3D12()
{
  m_uiSynchronizationFenceCompletedValue = 0U;
  XII_GAL_DILIGENT_PTR_RELEASE(m_pSynchronizationFence);

  FlushPipelineStateCache();
}

void xiiGALCommandEncoderD3D12::SetShaderPlatform(xiiGALShader* pShader)
{
  auto pShaderD3D12 = static_cast<xiiGALShaderD3D12*>(pShader);

  if (m_pCurrentShader != pShaderD3D12)
  {
    m_pCurrentShader = pShaderD3D12;

    m_bPipelineStateModified = true;
  }

  if (pShaderD3D12 != nullptr && !m_CachedShaderEventIDs.Contains(pShaderD3D12))
  {
    m_CachedShaderEventIDs.Insert(pShaderD3D12, pShaderD3D12->m_Events.AddEventHandler([=](const xiiGALShaderD3D12::ShaderEvent& e) {
      switch (e.m_Type)
      {
        case xiiGALShaderD3D12::ShaderEvent::BeforeDeletion:
        {
          for (auto iterator = m_CachedComputePipelineStates.GetIterator(); iterator.IsValid(); ++iterator)
          {
            if (iterator.Value().m_pShader == e.m_pShader)
            {
              PipelineStateInfo pipelineInfo;
              XII_VERIFY(m_CachedComputePipelineStates.Remove(iterator.Key(), &pipelineInfo), "Failed to remove cached compute pipeline state object.");

              XII_GAL_DILIGENT_PTR_RELEASE(pipelineInfo.m_pShaderResourceBinding);
              XII_GAL_DILIGENT_PTR_RELEASE(pipelineInfo.m_pPipelineState);

              iterator = m_CachedComputePipelineStates.GetIterator();
            }
          }
          for (auto iterator = m_CachedGraphicsPipelineStates.GetIterator(); iterator.IsValid(); ++iterator)
          {
            if (iterator.Value().m_pShader == e.m_pShader)
            {
              PipelineStateInfo pipelineInfo;
              XII_VERIFY(m_CachedGraphicsPipelineStates.Remove(iterator.Key(), &pipelineInfo), "Failed to remove cached graphics pipeline state object.");

              XII_GAL_DILIGENT_PTR_RELEASE(pipelineInfo.m_pShaderResourceBinding);
              XII_GAL_DILIGENT_PTR_RELEASE(pipelineInfo.m_pPipelineState);

              iterator = m_CachedGraphicsPipelineStates.GetIterator();
            }
          }

          xiiEventSubscriptionID id;
          m_CachedShaderEventIDs.Remove(e.m_pShader, &id);
          e.m_pShader->m_Events.RemoveEventHandler(id);
        }
        break;
        default:
          break;
      }
    }));
  }
}

void xiiGALCommandEncoderD3D12::SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer)
{
  auto pBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pBuffer);

  m_pBoundConstantBuffers[uiSlot] = pBufferD3D12->GetBuffer();
  m_bDescriptorsModified          = true;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
  }
}

void xiiGALCommandEncoderD3D12::SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler)
{
  auto pSamplerD3D12 = static_cast<xiiGALSamplerD3D12*>(pSampler);

  m_pBoundSamplers[xiiGALShaderStage::GetStageIndex(stage)][uiSlot] = pSamplerD3D12->GetSampler();
  m_bDescriptorsModified                                            = true;

  m_BoundSamplersRange[xiiGALShaderStage::GetStageIndex(stage)].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderD3D12::SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView)
{
  auto            pBufferViewD3D12         = static_cast<xiiGALBufferViewD3D12*>(pBufferView);
  const xiiUInt32 uiStage                  = xiiGALShaderStage::GetStageIndex(stage);
  auto&           boundShaderResourceViews = m_pBoundShaderResourceViews[uiStage];

  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  boundShaderResourceViews[uiSlot] = ShaderResourceViewDesc{ShaderResourceViewDesc::BufferView, pBufferViewD3D12 != nullptr ? pBufferViewD3D12->GetBufferView() : nullptr, nullptr};
  m_bDescriptorsModified           = true;

  m_BoundShaderResourceViewsRange[uiStage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderD3D12::SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pTextureView)
{
  auto            pTextureViewD3D12        = static_cast<xiiGALTextureViewD3D12*>(pTextureView);
  const xiiUInt32 uiStage                  = xiiGALShaderStage::GetStageIndex(stage);
  auto&           boundShaderResourceViews = m_pBoundShaderResourceViews[uiStage];

  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  boundShaderResourceViews[uiSlot] = ShaderResourceViewDesc{ShaderResourceViewDesc::TextureView, nullptr, pTextureViewD3D12 != nullptr ? pTextureViewD3D12->GetTextureView() : nullptr};
  m_bDescriptorsModified           = true;

  m_BoundShaderResourceViewsRange[uiStage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView)
{
  auto pUnorderedAccessBufferViewD3D12 = static_cast<xiiGALBufferViewD3D12*>(pUnorderedAccessBufferView);

  m_pBoundUnorderedAccessViews.EnsureCount(uiSlot + 1);

  m_pBoundUnorderedAccessViews[uiSlot] = ShaderResourceViewDesc{ShaderResourceViewDesc::BufferView, pUnorderedAccessBufferViewD3D12 != nullptr ? pUnorderedAccessBufferViewD3D12->GetBufferView() : nullptr, nullptr};
  m_bDescriptorsModified               = true;

  m_BoundUnorderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView)
{
  auto pUnorderedAccessTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pUnorderedAccessTextureView);

  m_pBoundUnorderedAccessViews.EnsureCount(uiSlot + 1);

  m_pBoundUnorderedAccessViews[uiSlot] = ShaderResourceViewDesc{ShaderResourceViewDesc::TextureView, nullptr, pUnorderedAccessTextureViewD3D12 != nullptr ? pUnorderedAccessTextureViewD3D12->GetTextureView() : nullptr};
  m_bDescriptorsModified               = true;

  m_BoundUnorderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderD3D12::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  m_pContext->BeginQuery(pQueryD3D12->GetQuery());
}

void xiiGALCommandEncoderD3D12::EndQueryPlatform(xiiGALQuery* pQuery)
{
  auto pQueryD3D12 = static_cast<xiiGALQueryD3D12*>(pQuery);

  m_pContext->EndQuery(pQueryD3D12->GetQuery());
}

/// \todo GraphicsD3D12: Implement Unordered Access View Clear.

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderD3D12::CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSource);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);

  m_pContext->CopyBuffer(pSourceBufferD3D12->GetBuffer(), 0U, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferD3D12->GetBuffer(), 0U, pDestination->GetDescription().m_uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSource);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);

  m_pContext->CopyBuffer(pSourceBufferD3D12->GetBuffer(), uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBufferD3D12->GetBuffer(), uiDestOffset, uiByteCount, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderD3D12::UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(sourceData.GetPtr());

  auto        pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);
  const auto& bufferDescription       = pDestinationBufferD3D12->GetDescription();

  EndRenderPass();

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == bufferDescription.m_uiSize, "Uniform (constant) buffers cannot be mapped partially, there are no checks for partial constant buffer updates.");
  }

  switch (bufferDescription.m_ResourceUsage)
  {
    case xiiGALResourceUsage::Default:
    {
      m_pContext->UpdateBuffer(pDestinationBufferD3D12->GetBuffer(), uiDestOffset, sourceData.GetCount(), reinterpret_cast<const void*>(sourceData.GetPtr()), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    break;
    case xiiGALResourceUsage::Dynamic:
    {
      Diligent::PVoid pMapResult = {};
      m_pContext->MapBuffer(pDestinationBufferD3D12->GetBuffer(), Diligent::MAP_WRITE, xiiDiligentTypeConversions::GetMapFlags(mapFlags), reinterpret_cast<Diligent::PVoid&>(pMapResult));

      if (pMapResult)
      {
        memcpy(xiiMemoryUtils::AddByteOffset((xiiUInt8*)pMapResult, uiDestOffset), sourceData.GetPtr(), sourceData.GetCount());

        m_pContext->UnmapBuffer(pDestinationBufferD3D12->GetBuffer(), Diligent::MAP_WRITE);
      }
      else
      {
        xiiLog::Error("Failed to map buffer to update content.");
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderD3D12::CopyTexturePlatform(xiiGALTexture* pDestination, xiiGALTexture* pSource)
{
  auto pSourceTexture      = static_cast<xiiGALTextureD3D12*>(pSource);
  auto pDestinationTexture = static_cast<xiiGALTextureD3D12*>(pDestination);

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTexture->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTexture->GetTexture();
  copyTextureDescription.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copyTextureDescription.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->CopyTexture(copyTextureDescription);
}

void xiiGALCommandEncoderD3D12::CopyTextureRegionPlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiVec3U32& vDestinationPoint, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource, const xiiBoundingBoxu32& box)
{
  auto pSourceTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pSource);
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestination);

  Diligent::Box srcBox = {};
  srcBox.MinX          = box.m_vMin.x;
  srcBox.MinY          = box.m_vMin.y;
  srcBox.MinZ          = box.m_vMin.z;
  srcBox.MaxX          = box.m_vMax.x;
  srcBox.MaxY          = box.m_vMax.y;
  srcBox.MaxZ          = box.m_vMax.z;

  Diligent::CopyTextureAttribs copyTextureDescription = {};
  copyTextureDescription.pSrcTexture                  = pSourceTextureD3D12->GetTexture();
  copyTextureDescription.pDstTexture                  = pDestinationTextureD3D12->GetTexture();
  copyTextureDescription.pSrcBox                      = &srcBox;

  copyTextureDescription.SrcMipLevel              = sourceSubResource.m_uiMipLevel;
  copyTextureDescription.SrcSlice                 = sourceSubResource.m_uiArraySlice;
  copyTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  copyTextureDescription.DstMipLevel              = destinationSubResource.m_uiMipLevel;
  copyTextureDescription.DstSlice                 = destinationSubResource.m_uiArraySlice;
  copyTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  copyTextureDescription.DstX                     = vDestinationPoint.x;
  copyTextureDescription.DstY                     = vDestinationPoint.y;
  copyTextureDescription.DstZ                     = vDestinationPoint.z;

  m_pContext->CopyTexture(copyTextureDescription);
}

void xiiGALCommandEncoderD3D12::UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALTextureSubResourceData& sourceData)
{
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestination);

  xiiUInt32 uiWidth  = xiiMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1U);
  xiiUInt32 uiHeight = xiiMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1U);
  xiiUInt32 uiDepth  = xiiMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1U);

  const auto& textureDescription = pDestinationTextureD3D12->GetDescription();
  const auto& formatProperties   = m_GALDeviceD3D12.GetTextureFormatProperties(textureDescription.m_Format);

  switch (textureDescription.m_Usage)
  {
    case xiiGALResourceUsage::Default:
    {
      xiiUInt32 uiRowPitch   = uiWidth * formatProperties.m_uiComponentSize;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;

      XII_ASSERT_DEV(sourceData.m_uiStride == uiRowPitch, "Invalid row pitch. Expected {0} got {1}.", uiRowPitch, sourceData.m_uiStride);
      XII_ASSERT_DEV(sourceData.m_uiDepthStride == 0 || sourceData.m_uiDepthStride == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, sourceData.m_uiDepthStride);

      Diligent::Box subRegion = {};
      subRegion.MinX          = destinationBox.m_vMin.x;
      subRegion.MinY          = destinationBox.m_vMin.y;
      subRegion.MinZ          = destinationBox.m_vMin.z;
      subRegion.MaxX          = destinationBox.m_vMax.x;
      subRegion.MaxY          = destinationBox.m_vMax.y;
      subRegion.MaxZ          = destinationBox.m_vMax.z;

      Diligent::TextureSubResData subResData = {};
      subResData.pData                       = sourceData.m_pData;
      subResData.Stride                      = uiRowPitch;
      subResData.DepthStride                 = uiSlicePitch;

      m_pContext->UpdateTexture(pDestinationTextureD3D12->GetTexture(), destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, subRegion, subResData, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    break;
    case xiiGALResourceUsage::Dynamic:
    {
      xiiUInt32 uiRowPitch   = uiWidth * formatProperties.m_uiComponentSize;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;

      XII_ASSERT_DEV(sourceData.m_uiStride == uiRowPitch, "Invalid row pitch. Expected {0} got {1}.", uiRowPitch, sourceData.m_uiStride);
      XII_ASSERT_DEV(sourceData.m_uiDepthStride == 0 || sourceData.m_uiDepthStride == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, sourceData.m_uiDepthStride);

      Diligent::Box subRegion = {};
      subRegion.MinX          = destinationBox.m_vMin.x;
      subRegion.MinY          = destinationBox.m_vMin.y;
      subRegion.MinZ          = destinationBox.m_vMin.z;
      subRegion.MaxX          = destinationBox.m_vMax.x;
      subRegion.MaxY          = destinationBox.m_vMax.y;
      subRegion.MaxZ          = destinationBox.m_vMax.z;

      Diligent::MappedTextureSubresource mappedSubResource = {};
      m_pContext->MapTextureSubresource(pDestinationTextureD3D12->GetTexture(), destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_DO_NOT_WAIT, nullptr, mappedSubResource);

      if (mappedSubResource.pData)
      {
        if (mappedSubResource.Stride == uiRowPitch && mappedSubResource.DepthStride == uiSlicePitch)
        {
          memcpy(mappedSubResource.pData, sourceData.m_pData, uiSlicePitch * uiDepth);
        }
        else
        {
          // Copy by row
          for (xiiUInt32 z = 0; z < uiDepth; ++z)
          {
            const void* pSourceData      = xiiMemoryUtils::AddByteOffset(sourceData.m_pData, z * uiSlicePitch);
            void*       pDestinationData = xiiMemoryUtils::AddByteOffset(mappedSubResource.pData, z * mappedSubResource.DepthStride);

            for (xiiUInt32 y = 0; y < uiHeight; ++y)
            {
              memcpy(pDestinationData, pSourceData, uiRowPitch);

              pSourceData      = xiiMemoryUtils::AddByteOffset(pSourceData, uiRowPitch);
              pDestinationData = xiiMemoryUtils::AddByteOffset(pDestinationData, mappedSubResource.Stride);
            }
          }
        }

        m_pContext->UnmapTextureSubresource(pDestinationTextureD3D12->GetTexture(), destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice);
      }
      else
      {
        xiiLog::Error("Failed to retrieve mapped texture data for reading.");
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderD3D12::ResolveTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, xiiGALTexture* pSource, const xiiGALTextureMipLevelData& sourceSubResource)
{
  auto pSourceTextureD3D12      = static_cast<xiiGALTextureD3D12*>(pSource);
  auto pDestinationTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pDestination);

  const auto& sourceTextureDescription = pSourceTextureD3D12->GetDescription();

  Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
  ResolveTexAttribs.Format = xiiDiligentTypeConversions::GetTextureFormat(sourceTextureDescription.m_Format);

  ResolveTexAttribs.SrcMipLevel              = sourceSubResource.m_uiMipLevel;
  ResolveTexAttribs.SrcSlice                 = sourceSubResource.m_uiArraySlice;
  ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  ResolveTexAttribs.DstMipLevel              = destinationSubResource.m_uiMipLevel;
  ResolveTexAttribs.DstSlice                 = destinationSubResource.m_uiArraySlice;
  ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->ResolveTextureSubresource(pSourceTextureD3D12->GetTexture(), pDestinationTextureD3D12->GetTexture(), ResolveTexAttribs);
}

void xiiGALCommandEncoderD3D12::ReadbackTexturePlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture)
{
  auto pTextureD3D12        = static_cast<xiiGALTextureD3D12*>(pTexture);
  auto pStagingTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pTexture);

  const auto& textureDescription = pTextureD3D12->GetDescription();

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = textureDescription.m_uiSampleCount != 0U;

  if (bMSAASourceTexture)
  {
    /// \todo Support other mip levels?

    Diligent::ResolveTextureSubresourceAttribs resolveTextureSubresourceDescription;
    resolveTextureSubresourceDescription.Format                   = xiiDiligentTypeConversions::GetTextureFormat(textureDescription.m_Format);
    resolveTextureSubresourceDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    resolveTextureSubresourceDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pContext->ResolveTextureSubresource(pTextureD3D12->GetTexture(), pStagingTextureD3D12->GetTexture(), resolveTextureSubresourceDescription);
  }
  else
  {
    Diligent::CopyTextureAttribs copyTextureDescription;
    copyTextureDescription.pSrcTexture              = pTextureD3D12->GetTexture();
    copyTextureDescription.pDstTexture              = pStagingTextureD3D12->GetTexture();
    copyTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    copyTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pContext->CopyTexture(copyTextureDescription);
  }

  xiiUInt64 uiWaitValue = ++m_uiSynchronizationFenceCompletedValue;

  m_pContext->EnqueueSignal(m_pSynchronizationFence, uiWaitValue);
  m_pContext->Flush();
  m_pSynchronizationFence->Wait(uiWaitValue);
}

xiiUInt32 GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel)
{
  for (xiiUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return xiiMath::Max(1u, uiSize);
}

void xiiGALCommandEncoderD3D12::CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALTextureSubResourceData> targetData)
{
  auto pTextureD3D12        = static_cast<xiiGALTextureD3D12*>(pTexture);
  auto pStagingTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pStagingTexture);

  XII_ASSERT_DEV(mipLevelData.GetCount() == targetData.GetCount(), "Source and target arrays must be of the same size.");

  const auto& textureDescription = pTextureD3D12->GetDescription();

  const xiiUInt32 uiSubResources = mipLevelData.GetCount();
  for (xiiUInt32 i = 0; i < uiSubResources; ++i)
  {
    const xiiGALTextureMipLevelData&    subResourceData = mipLevelData[i];
    const xiiGALTextureSubResourceData& textureData     = targetData[i];

    Diligent::MappedTextureSubresource mappedSubResource = {};
    m_pContext->MapTextureSubresource(pTextureD3D12->GetTexture(), subResourceData.m_uiMipLevel, subResourceData.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_DO_NOT_WAIT, nullptr, mappedSubResource);

    xiiUInt64 uiWaitValue = ++m_uiSynchronizationFenceCompletedValue;

    m_pContext->EnqueueSignal(m_pSynchronizationFence, uiWaitValue);
    m_pContext->Flush();
    m_pSynchronizationFence->Wait(uiWaitValue);

    if (mappedSubResource.pData)
    {
      const xiiGALTextureFormatDescription& formatProperties = m_GALDeviceD3D12.GetTextureFormatProperties(textureDescription.m_Format);

      /// \todo Support depth pitch.
      if (mappedSubResource.Stride == textureData.m_uiStride)
      {
        const xiiUInt32 uiMemorySize = formatProperties.m_uiComponentSize * GetMipSize(textureDescription.m_Size.width, subResourceData.m_uiMipLevel) * GetMipSize(textureDescription.m_Size.width, subResourceData.m_uiMipLevel);

        memcpy(textureData.m_pData, mappedSubResource.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row.

        const xiiUInt32 uiHeight = GetMipSize(textureDescription.m_Size.height, subResourceData.m_uiMipLevel);

        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource      = xiiMemoryUtils::AddByteOffset(mappedSubResource.pData, y * mappedSubResource.Stride);
          void*       pDestination = xiiMemoryUtils::AddByteOffset(textureData.m_pData, y * textureData.m_uiStride);

          memcpy(pDestination, pSource, formatProperties.m_uiComponentSize * GetMipSize(textureDescription.m_Size.width, subResourceData.m_uiMipLevel));
        }
      }

      m_pContext->UnmapTextureSubresource(pTextureD3D12->GetTexture(), subResourceData.m_uiMipLevel, subResourceData.m_uiArraySlice);
    }
    else
    {
      xiiLog::Error("Failed to retrieve mapped texture data for reading.");
    }
  }
}

void xiiGALCommandEncoderD3D12::GenerateMipMapsPlatform(xiiGALTextureView* pTextureView)
{
  /// \todo End render pass.

  m_pContext->GenerateMips(static_cast<xiiGALTextureViewD3D12*>(pTextureView)->GetTextureView());
}

void xiiGALCommandEncoderD3D12::FlushPlatform()
{
  EndRenderPass();

  FlushDeferredStateChanges();

  m_pContext->Flush();
}

void xiiGALCommandEncoderD3D12::PushMarkerPlatform(xiiStringView sMarker)
{
  m_pContext->BeginDebugGroup(sMarker.GetStartPointer());
}

void xiiGALCommandEncoderD3D12::PopMarkerPlatform()
{
  m_pContext->EndDebugGroup();
}

void xiiGALCommandEncoderD3D12::InsertEventMarkerPlatform(xiiStringView sMarker, const xiiColor& color)
{
  m_pContext->InsertDebugLabel(sMarker.GetStartPointer(), color.GetData());
}

void xiiGALCommandEncoderD3D12::ClearRenderTargetPlatform(xiiGALTextureView* pTextureView, const xiiColor& clearColor)
{
  auto pTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pTextureView);

  m_pContext->ClearRenderTarget(pTextureViewD3D12->GetTextureView(), clearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderD3D12::ClearDepthStencilPlatform(xiiGALTextureView* pTextureView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  auto pTextureViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(pTextureView);

  Diligent::CLEAR_DEPTH_STENCIL_FLAGS clearFlags = {};

  if (bClearDepth)
    clearFlags |= Diligent::CLEAR_DEPTH_FLAG;
  if (bClearStencil)
    clearFlags |= Diligent::CLEAR_STENCIL_FLAG;

  m_pContext->ClearDepthStencil(pTextureViewD3D12->GetTextureView(), clearFlags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCount;
  drawAttribs.StartVertexLocation   = uiStartVertex;
  drawAttribs.NumInstances          = 1U;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCount;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_IndexFormat : Diligent::VT_UNDEFINED;
  drawAttribs.BaseVertex            = 0U;
  drawAttribs.NumInstances          = 1U;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCountPerInstance;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_IndexFormat : Diligent::VT_UNDEFINED;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0U;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  Diligent::DrawIndexedIndirectAttribs drawAttribs;
  drawAttribs.IndexType                        = m_pIndexBuffer != nullptr ? m_IndexFormat : Diligent::VT_UNDEFINED;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1U;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 5U;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0U;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->DrawIndexedIndirect(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCountPerInstance;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.StartVertexLocation   = uiStartVertex;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  BeginRenderPass();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawAttribs;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferD3D12*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1U;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 4U;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0U;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->DrawIndirect(drawAttribs);
}

void xiiGALCommandEncoderD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto pIndexBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pIndexBuffer);

  if (m_pIndexBuffer != pIndexBufferD3D12->GetBuffer())
  {
    m_pIndexBuffer            = pIndexBufferD3D12->GetBuffer();
    m_IndexFormat             = pIndexBufferD3D12->GetIndexFormat();
    m_uiIndexBufferByteOffset = uiByteOffset;
    m_bIndexBufferModified    = true;
  }
}

void xiiGALCommandEncoderD3D12::SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer)
{
  auto            pVertexBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pVertexBuffer);
  const xiiUInt64 uiStride           = pVertexBufferD3D12 != nullptr ? pVertexBufferD3D12->GetDescription().m_uiElementByteStride : 0U;

  if (m_pBoundVertexBuffers[uiSlot] != pVertexBufferD3D12->GetBuffer())
  {
    m_pBoundVertexBuffers[uiSlot] = pVertexBufferD3D12->GetBuffer();

    m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);

    if (m_VertexBufferStrides[uiSlot] != uiStride)
    {
      m_VertexBufferStrides[uiSlot] = static_cast<xiiUInt32>(uiStride);
      m_bPipelineStateModified      = true;
    }
  }
}

void xiiGALCommandEncoderD3D12::SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
  auto pInputLayoutD3D12 = static_cast<xiiGALInputLayoutD3D12*>(pInputLayout);

  if (m_pInputLayout != pInputLayoutD3D12)
  {
    m_pInputLayout           = pInputLayoutD3D12;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderD3D12::SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology)
{
  if (m_PrimitiveTopology != topology)
  {
    m_PrimitiveTopology      = topology;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderD3D12::SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask)
{
  auto pBlendStateD3D12 = static_cast<xiiGALBlendStateD3D12*>(pBlendState);

  m_pContext->SetBlendFactors(blendFactor.GetData());

  if (m_pBlendState != pBlendStateD3D12)
  {
    m_pBlendState            = pBlendStateD3D12;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderD3D12::SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  auto pDepthStencilStateD3D12 = static_cast<xiiGALDepthStencilStateD3D12*>(pDepthStencilState);

  m_pContext->SetStencilRef(uiStencilRefValue);

  if (m_pDepthStencilState != pDepthStencilStateD3D12)
  {
    m_pDepthStencilState     = pDepthStencilStateD3D12;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderD3D12::SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  auto pRasterizerStateD3D12 = static_cast<xiiGALRasterizerStateD3D12*>(pRasterizerState);

  if (m_pRasterizerState != pRasterizerStateD3D12)
  {
    m_pRasterizerState       = pRasterizerStateD3D12;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderD3D12::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  Diligent::Viewport viewport;
  viewport.TopLeftX = rect.x;
  viewport.TopLeftY = rect.y;
  viewport.Width    = rect.width;
  viewport.Height   = rect.height;
  viewport.MinDepth = fMinDepth;
  viewport.MaxDepth = fMaxDepth;

  m_pContext->SetViewports(1U, &viewport, static_cast<xiiUInt32>(rect.width), static_cast<xiiUInt32>(rect.height));
}

void xiiGALCommandEncoderD3D12::SetScissorRectPlatform(const xiiRectU32& rect)
{
  Diligent::Rect scissorRect;
  scissorRect.left   = rect.x;
  scissorRect.top    = rect.y;
  scissorRect.right  = rect.x + rect.width;
  scissorRect.bottom = rect.y + rect.height;

  m_pContext->SetScissorRects(1U, &scissorRect, rect.width, rect.height);
}

void xiiGALCommandEncoderD3D12::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  Diligent::DispatchComputeAttribs DispatchAttribs;
  DispatchAttribs.ThreadGroupCountX = uiThreadGroupCountX;
  DispatchAttribs.ThreadGroupCountY = uiThreadGroupCountY;
  DispatchAttribs.ThreadGroupCountZ = uiThreadGroupCountZ;

  m_pContext->DispatchCompute(DispatchAttribs);
}

void xiiGALCommandEncoderD3D12::DispatchIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs;
  DispatchAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer)->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  DispatchAttribs.DispatchArgsByteOffset           = uiArgumentOffsetInBytes;

  m_pContext->DispatchComputeIndirect(DispatchAttribs);
}

void xiiGALCommandEncoderD3D12::BeginRendering(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPassD3D12* pRenderPassD3D12, xiiGALFramebufferD3D12* pFramebufferD3D12)
{
  m_pRenderPass            = pRenderPassD3D12;
  m_pFramebuffer           = pFramebufferD3D12;
  m_RenderingSetup         = renderingSetup;
  m_bPipelineStateModified = true;
}

void xiiGALCommandEncoderD3D12::EndRendering()
{
  EndRenderPass();

  m_pRenderPass    = nullptr;
  m_pFramebuffer   = nullptr;
  m_RenderingSetup = {};
}

void xiiGALCommandEncoderD3D12::BeginCompute()
{
  m_bIsComputeRequested    = true;
  m_bPipelineStateModified = true;
}

void xiiGALCommandEncoderD3D12::EndCompute()
{
  m_bIsComputeRequested = false;
}

void xiiGALCommandEncoderD3D12::FlushDeferredStateChanges()
{
  if (m_bPipelineStateModified)
  {
    {
      if (m_bIsComputeRequested)
      {
        Diligent::ComputePipelineStateCreateInfo computePipelineStateDescription;
        computePipelineStateDescription.PSODesc.PipelineType    = Diligent::PIPELINE_TYPE_COMPUTE;
        computePipelineStateDescription.Flags                   = Diligent::PSO_CREATE_FLAG_NONE;
        computePipelineStateDescription.pCS                     = m_pCurrentShader->GetComputeShader();
        computePipelineStateDescription.ppResourceSignatures    = m_pCurrentShader->GetResourceSignatures().GetPtr();
        computePipelineStateDescription.ResourceSignaturesCount = m_pCurrentShader->GetResourceSignatures().GetCount();

        PipelineStateInfo pipelineInfo;
        if (!m_CachedComputePipelineStates.TryGetValue(computePipelineStateDescription, pipelineInfo))
        {
          m_GALDeviceD3D12.GetDevice()->CreatePipelineState(computePipelineStateDescription, &pipelineInfo.m_pPipelineState);

          XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new Compute pipeline state object.");

          m_pCurrentShader->GetResourceSignatures()[0]->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

          pipelineInfo.m_pShader = m_pCurrentShader;

          XII_VERIFY(!m_CachedComputePipelineStates.Insert(computePipelineStateDescription, pipelineInfo), "Erased existing compute pipeline state object, this is unexpected.");
        }

        m_pCurrentPipelineState         = pipelineInfo.m_pPipelineState;
        m_pCurrentShaderResourceBinding = pipelineInfo.m_pShaderResourceBinding;
      }
      else
      {
        Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDescription;
        graphicsPipelineStateDescription.PSODesc.PipelineType    = Diligent::PIPELINE_TYPE_GRAPHICS;
        graphicsPipelineStateDescription.Flags                   = Diligent::PSO_CREATE_FLAG_NONE;
        graphicsPipelineStateDescription.pVS                     = m_pCurrentShader->GetVertexShader();
        graphicsPipelineStateDescription.pPS                     = m_pCurrentShader->GetPixelShader();
        graphicsPipelineStateDescription.pDS                     = m_pCurrentShader->GetDomainShader();
        graphicsPipelineStateDescription.pHS                     = m_pCurrentShader->GetHullShader();
        graphicsPipelineStateDescription.pGS                     = m_pCurrentShader->GetGeometryShader();
        graphicsPipelineStateDescription.pAS                     = m_pCurrentShader->GetAmplificationShader();
        graphicsPipelineStateDescription.pMS                     = m_pCurrentShader->GetMeshShader();
        graphicsPipelineStateDescription.ppResourceSignatures    = m_pCurrentShader->GetResourceSignatures().GetPtr();
        graphicsPipelineStateDescription.ResourceSignaturesCount = m_pCurrentShader->GetResourceSignatures().GetCount();

        Diligent::GraphicsPipelineDesc& graphicsPipelineDescription = graphicsPipelineStateDescription.GraphicsPipeline;
        graphicsPipelineDescription.pRenderPass                     = m_pRenderPass->GetRenderPass();
        graphicsPipelineDescription.PrimitiveTopology               = xiiDiligentTypeConversions::GetPrimitiveTopology(m_PrimitiveTopology);
        graphicsPipelineDescription.NumViewports                    = 1U;

        if (m_pBlendState)
          graphicsPipelineDescription.BlendDesc = *m_pBlendState->GetBlendState();
        if (m_pDepthStencilState)
          graphicsPipelineDescription.DepthStencilDesc = *m_pDepthStencilState->GetDepthStencilState();
        if (m_pRasterizerState)
          graphicsPipelineDescription.RasterizerDesc = *m_pRasterizerState->GetRasterizerState();

        PipelineStateInfo pipelineInfo = {};
        if (!m_CachedGraphicsPipelineStates.TryGetValue(graphicsPipelineStateDescription, pipelineInfo))
        {
          // Hashing the input layout will cause problems with hash deduction, due to the nature of its lifetime.
          if (m_pInputLayout)
          {
            auto layoutElements = m_pInputLayout->GetElements();

            // Assign an appropriate vertex buffer stride if XII_GAL_LAYOUT_ELEMENT_AUTO_STRIDE is used in the input layout.
            for (xiiUInt32 i = 0; i < layoutElements.GetCount(); ++i)
            {
              auto& element = layoutElements[i];

              if (m_BoundVertexBuffersRange.HasIncludeValue(element.BufferSlot) && element.Stride == XII_GAL_LAYOUT_ELEMENT_AUTO_STRIDE)
              {
                element.Stride = m_VertexBufferStrides[element.BufferSlot];
              }
            }

            graphicsPipelineDescription.InputLayout = *m_pInputLayout->GetLayout();
          }

          m_GALDeviceD3D12.GetDevice()->CreatePipelineState(graphicsPipelineStateDescription, &pipelineInfo.m_pPipelineState);

          XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new Graphics pipeline state object.");

          m_pCurrentShader->GetResourceSignatures()[0]->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

          pipelineInfo.m_pShader = m_pCurrentShader;

          graphicsPipelineDescription.InputLayout = {};

          XII_VERIFY(!m_CachedGraphicsPipelineStates.Insert(graphicsPipelineStateDescription, pipelineInfo), "Erased existing graphics pipeline state object, this is unexpected.");
        }

        m_pCurrentPipelineState         = pipelineInfo.m_pPipelineState;
        m_pCurrentShaderResourceBinding = pipelineInfo.m_pShaderResourceBinding;
      }
    }

    m_bPipelineStateModified = false;
    m_bDescriptorsModified   = true; // Changes to pipeline state always require that the descriptor set be rebound.
  }

  if (!m_bIsComputeRequested && m_BoundVertexBuffersRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const xiiUInt32 uiNumSlots  = m_BoundVertexBuffersRange.GetCount();

    // Diligent will handle unsetting null buffers with the SET_VERTEX_BUFFERS_FLAG_RESET flag.
    m_pContext->SetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferOffsets + uiStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

    m_BoundVertexBuffersRange.Reset();
  }

  if (!m_bIsComputeRequested && m_bIndexBufferModified)
  {
    m_pContext->SetIndexBuffer(m_pIndexBuffer, m_uiIndexBufferByteOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_bIndexBufferModified = false;
  }

  if (m_bDescriptorsModified)
  {
    // Note that this function does not check if the bindings have been modified.
    for (xiiUInt32 uiShaderStage = 0; uiShaderStage < xiiGALShaderStage::ENUM_COUNT; ++uiShaderStage)
    {
      xiiBitflags<xiiGALShaderStage> shaderStage                  = xiiGALShaderStage::GetStageFlag(uiShaderStage);
      const auto&                    shaderResourceBinding        = m_pCurrentShader->GetShaderResourceBinding(shaderStage);
      const xiiUInt32                uiShaderResourceBindingCount = shaderResourceBinding.GetCount();

      for (xiiUInt32 uiBindingIndex = 0; uiBindingIndex < uiShaderResourceBindingCount; ++uiBindingIndex)
      {
        const auto& binding = shaderResourceBinding[uiBindingIndex];

        switch (binding.m_Type)
        {
          case xiiGALShaderResourceType::ConstantBuffer:
          {
            auto pConstantBuffer = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pConstantBuffer && m_BoundConstantBuffersRange[uiShaderStage].HasIncludeValue(binding.m_uiSlot))
            {
              pConstantBuffer->Set(m_pBoundConstantBuffers[binding.m_uiSlot], Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pConstantBuffer)
            {
              xiiLog::Error("Constant buffer pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::TextureSRV:
          {
            auto pTextureSRV = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pTextureSRV && !m_pBoundShaderResourceViews[uiShaderStage].IsEmpty() && m_pBoundShaderResourceViews[uiShaderStage].GetCount() > binding.m_uiSlot && m_BoundShaderResourceViewsRange[uiShaderStage].HasIncludeValue(binding.m_uiSlot))
            {
              auto resourceView = m_pBoundShaderResourceViews[uiShaderStage][binding.m_uiSlot];

              XII_ASSERT_DEV(resourceView.m_Type == ShaderResourceViewDesc::TextureView, "Expected a bound texture SRV.");

              pTextureSRV->Set(m_pBoundShaderResourceViews[uiShaderStage][binding.m_uiSlot].m_pTextureView, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pTextureSRV)
            {
              xiiLog::Error("SRV Texture view pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::BufferSRV:
          {
            auto pBufferSRV = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pBufferSRV && !m_pBoundShaderResourceViews[uiShaderStage].IsEmpty() && m_pBoundShaderResourceViews[uiShaderStage].GetCount() > binding.m_uiSlot && m_BoundShaderResourceViewsRange[uiShaderStage].HasIncludeValue(binding.m_uiSlot))
            {
              auto resourceView = m_pBoundShaderResourceViews[uiShaderStage][binding.m_uiSlot];

              XII_ASSERT_DEV(resourceView.m_Type == ShaderResourceViewDesc::BufferView, "Expected a bound buffer SRV.");

              pBufferSRV->Set(m_pBoundShaderResourceViews[uiShaderStage][binding.m_uiSlot].m_pBufferView, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pBufferSRV)
            {
              xiiLog::Error("SRV Buffer view pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::TextureUAV:
          {
            auto pTextureUAV = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pTextureUAV && !m_pBoundUnorderedAccessViews.IsEmpty() && m_pBoundUnorderedAccessViews.GetCount() > binding.m_uiSlot && m_BoundUnorderedAccessViewsRange.HasIncludeValue(binding.m_uiSlot))
            {
              auto resourceView = m_pBoundUnorderedAccessViews[binding.m_uiSlot];

              XII_ASSERT_DEV(resourceView.m_Type == ShaderResourceViewDesc::TextureView, "Expected a bound texture UAV.");

              pTextureUAV->Set(m_pBoundUnorderedAccessViews[binding.m_uiSlot].m_pTextureView, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pTextureUAV)
            {
              xiiLog::Error("UAV Texture view pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::BufferUAV:
          {
            auto pBufferUAV = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pBufferUAV && !m_pBoundUnorderedAccessViews.IsEmpty() && m_pBoundUnorderedAccessViews.GetCount() > binding.m_uiSlot && m_BoundUnorderedAccessViewsRange.HasIncludeValue(binding.m_uiSlot))
            {
              auto resourceView = m_pBoundUnorderedAccessViews[binding.m_uiSlot];

              XII_ASSERT_DEV(resourceView.m_Type == ShaderResourceViewDesc::BufferView, "Expected a bound buffer UAV.");

              pBufferUAV->Set(m_pBoundUnorderedAccessViews[binding.m_uiSlot].m_pBufferView, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pBufferUAV)
            {
              xiiLog::Error("UAV Buffer view pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::Sampler:
          {
            auto pSampler = m_pCurrentShaderResourceBinding->GetVariableByName(xiiDiligentTypeConversions::GetShaderTypeFlags(shaderStage), binding.m_sName.GetData());
            if (pSampler && m_BoundSamplersRange[uiShaderStage].HasIncludeValue(binding.m_uiSlot))
            {
              auto resourceView = m_pBoundSamplers[binding.m_uiSlot];

              pSampler->Set(m_pBoundSamplers[uiShaderStage][binding.m_uiSlot], Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
            else if (!pSampler)
            {
              xiiLog::Error("Sampler pointer for '{}' returned null.", binding.m_sName);
            }
          }
          break;
          case xiiGALShaderResourceType::InputAttachment:
          {
            XII_ASSERT_NOT_IMPLEMENTED;
          }
          break;
          case xiiGALShaderResourceType::AccelerationStructure:
          {
            XII_ASSERT_NOT_IMPLEMENTED;
          }
          break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }

      m_BoundConstantBuffersRange[uiShaderStage].Reset();
      m_BoundShaderResourceViewsRange[uiShaderStage].Reset();
      m_BoundSamplersRange[uiShaderStage].Reset();
    }
    m_BoundVertexBuffersRange.Reset();
    m_BoundUnorderedAccessViewsRange.Reset();

    m_bDescriptorsModified = false;
  }

  m_pContext->SetPipelineState(m_pCurrentPipelineState);
  m_pContext->CommitShaderResources(m_pCurrentShaderResourceBinding, m_bRenderPassActive ? Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY : Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderD3D12::FlushPipelineStateCache()
{
  for (auto& iter : m_CachedGraphicsPipelineStates)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(iter.Value().m_pShaderResourceBinding);
    XII_GAL_DILIGENT_PTR_RELEASE(iter.Value().m_pPipelineState);
  }
  m_CachedGraphicsPipelineStates.Clear();
  m_CachedGraphicsPipelineStates.Compact();

  for (auto& iter : m_CachedComputePipelineStates)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(iter.Value().m_pShaderResourceBinding);
    XII_GAL_DILIGENT_PTR_RELEASE(iter.Value().m_pPipelineState);
  }
  m_CachedComputePipelineStates.Clear();
  m_CachedComputePipelineStates.Compact();
}

void xiiGALCommandEncoderD3D12::BeginRenderPass()
{
  XII_ASSERT_DEV(!m_bIsComputeRequested, "Cannot begin render pass while compute pipeline is active!");

  if (!m_bRenderPassActive)
  {
    xiiHybridArray<Diligent::OptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> clearValues;
    {
      const bool     bHasDepthAttachment    = !m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
      const xiiUInt8 uiColorAttachmentCount = m_RenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

      if (bHasDepthAttachment)
      {
        const xiiGALTextureViewD3D12* pRenderTargetViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(m_GALDeviceD3D12.GetTextureView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

        xiiGALTextureHandle       hTexture      = pRenderTargetViewD3D12->GetDescription().m_hTexture;
        const xiiGALTextureD3D12* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(m_GALDeviceD3D12.GetTexture(hTexture));

        const xiiGALTextureCreationDescription& textureDescription = pTextureD3D12->GetDescription();
        const auto&                             formatInfo         = m_GALDeviceD3D12.GetFormatLookupTable().GetFormatInfo(textureDescription.m_Format);

        Diligent::OptimizedClearValue& depthClear = clearValues.ExpandAndGetRef();
        depthClear.SetDepthStencil(formatInfo.m_eDepthStencilType, 1.0f, 0);
      }

      for (xiiUInt8 i = 0; i < uiColorAttachmentCount; ++i)
      {
        const xiiGALTextureViewD3D12* pRenderTargetViewD3D12 = static_cast<xiiGALTextureViewD3D12*>(m_GALDeviceD3D12.GetTextureView(m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(i)));

        xiiGALTextureHandle       hTexture      = pRenderTargetViewD3D12->GetDescription().m_hTexture;
        const xiiGALTextureD3D12* pTextureD3D12 = static_cast<xiiGALTextureD3D12*>(m_GALDeviceD3D12.GetTexture(hTexture));

        const xiiGALTextureCreationDescription& textureDescription = pTextureD3D12->GetDescription();
        const auto&                             formatInfo         = m_GALDeviceD3D12.GetFormatLookupTable().GetFormatInfo(textureDescription.m_Format);

        Diligent::OptimizedClearValue& colorClear = clearValues.ExpandAndGetRef();
        colorClear.SetColor(formatInfo.m_eRenderTarget, m_RenderingSetup.m_ClearColor.GetData());
      }
    }

    Diligent::BeginRenderPassAttribs renderPassBeginDescription;
    renderPassBeginDescription.pRenderPass         = m_pRenderPass->GetRenderPass();
    renderPassBeginDescription.pFramebuffer        = m_pFramebuffer->GetFramebuffer();
    renderPassBeginDescription.StateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    renderPassBeginDescription.pClearValues        = clearValues.GetData();
    renderPassBeginDescription.ClearValueCount     = clearValues.GetCount();

    m_pContext->BeginRenderPass(renderPassBeginDescription);

    m_bRenderPassActive = true;
  }
}

void xiiGALCommandEncoderD3D12::EndRenderPass()
{
  if (m_bRenderPassActive)
  {
    m_pContext->EndRenderPass();

    m_bRenderPassActive = false;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Resource Cache Hash

xiiUInt32 xiiGALCommandEncoderD3D12::ResourceCacheHash::Hash(const Diligent::GraphicsPipelineStateCreateInfo& desc)
{
  xiiHashStreamWriter32 writer;

  writer << desc.PSODesc.PipelineType;
  writer << static_cast<xiiUInt32>(desc.PSODesc.SRBAllocationGranularity);
  writer << static_cast<xiiUInt64>(desc.PSODesc.ImmediateContextMask);
  writer << desc.PSODesc.ResourceLayout.DefaultVariableType;
  writer << desc.PSODesc.ResourceLayout.DefaultVariableMergeStages;

  for (xiiUInt32 i = 0; i < desc.PSODesc.ResourceLayout.NumVariables; ++i)
  {
    writer << (desc.PSODesc.ResourceLayout.Variables + i);
  }

  for (xiiUInt32 i = 0; i < desc.PSODesc.ResourceLayout.NumImmutableSamplers; ++i)
  {
    writer << (desc.PSODesc.ResourceLayout.ImmutableSamplers + i);
  }

  writer << desc.Flags;

  for (xiiUInt32 i = 0; i < desc.ResourceSignaturesCount; ++i)
  {
    writer << *(desc.ppResourceSignatures + i);
  }

  writer << desc.pPSOCache;

  writer << desc.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable;
  writer << desc.GraphicsPipeline.BlendDesc.IndependentBlendEnable;

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].BlendEnable;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].LogicOperationEnable;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].SrcBlend;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].DestBlend;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].BlendOp;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].SrcBlendAlpha;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].DestBlendAlpha;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].BlendOpAlpha;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].LogicOp;
    writer << desc.GraphicsPipeline.BlendDesc.RenderTargets[i].RenderTargetWriteMask;
  }

  writer << desc.GraphicsPipeline.SampleMask;
  writer << desc.GraphicsPipeline.RasterizerDesc.FillMode;
  writer << desc.GraphicsPipeline.RasterizerDesc.CullMode;
  writer << desc.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise;
  writer << desc.GraphicsPipeline.RasterizerDesc.DepthClipEnable;
  writer << desc.GraphicsPipeline.RasterizerDesc.ScissorEnable;
  writer << desc.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable;
  writer << desc.GraphicsPipeline.RasterizerDesc.DepthBias;
  writer << desc.GraphicsPipeline.RasterizerDesc.DepthBiasClamp;
  writer << desc.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias;
  writer << desc.GraphicsPipeline.DepthStencilDesc.DepthEnable;
  writer << desc.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable;
  writer << desc.GraphicsPipeline.DepthStencilDesc.DepthFunc;
  writer << desc.GraphicsPipeline.DepthStencilDesc.StencilEnable;
  writer << desc.GraphicsPipeline.DepthStencilDesc.StencilReadMask;
  writer << desc.GraphicsPipeline.DepthStencilDesc.StencilWriteMask;
  writer << desc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc;
  writer << desc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp;
  writer << desc.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc;

  for (xiiUInt32 i = 0; i < desc.GraphicsPipeline.InputLayout.NumElements; ++i)
  {
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->InputIndex;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->BufferSlot;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->NumComponents;
    writer << (xiiUInt8)(desc.GraphicsPipeline.InputLayout.LayoutElements + i)->ValueType;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->IsNormalized;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->IsNormalized;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->RelativeOffset;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->Stride;
    writer << (xiiUInt8)(desc.GraphicsPipeline.InputLayout.LayoutElements + i)->Frequency;
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i)->InstanceDataStepRate;
  }

  writer << desc.GraphicsPipeline.PrimitiveTopology;
  writer << desc.GraphicsPipeline.NumViewports;
  writer << desc.GraphicsPipeline.NumRenderTargets;
  writer << desc.GraphicsPipeline.SubpassIndex;

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    writer << desc.GraphicsPipeline.RTVFormats[i];
  }

  writer << desc.GraphicsPipeline.DSVFormat;
  writer << desc.GraphicsPipeline.SmplDesc.Count;
  writer << desc.GraphicsPipeline.SmplDesc.Quality;
  writer << desc.GraphicsPipeline.pRenderPass;
  writer << desc.GraphicsPipeline.NodeMask;
  writer << desc.pVS;
  writer << desc.pPS;
  writer << desc.pDS;
  writer << desc.pHS;
  writer << desc.pGS;
  writer << desc.pAS;
  writer << desc.pMS;

  return writer.GetHashValue();
}

bool xiiGALCommandEncoderD3D12::ResourceCacheHash::Equal(const Diligent::GraphicsPipelineStateCreateInfo& a, const Diligent::GraphicsPipelineStateCreateInfo& b)
{
  return a == b;
}

xiiUInt32 xiiGALCommandEncoderD3D12::ResourceCacheHash::Hash(const Diligent::ComputePipelineStateCreateInfo& desc)
{
  xiiHashStreamWriter32 writer;

  writer << desc.PSODesc.PipelineType;
  writer << static_cast<xiiUInt32>(desc.PSODesc.SRBAllocationGranularity);
  writer << static_cast<xiiUInt64>(desc.PSODesc.ImmediateContextMask);
  writer << desc.PSODesc.ResourceLayout.DefaultVariableType;
  writer << desc.PSODesc.ResourceLayout.DefaultVariableMergeStages;

  for (xiiUInt32 i = 0; i < desc.PSODesc.ResourceLayout.NumVariables; ++i)
  {
    writer << (desc.PSODesc.ResourceLayout.Variables + i);
  }

  for (xiiUInt32 i = 0; i < desc.PSODesc.ResourceLayout.NumImmutableSamplers; ++i)
  {
    writer << (desc.PSODesc.ResourceLayout.ImmutableSamplers + i);
  }

  writer << desc.Flags;

  for (xiiUInt32 i = 0; i < desc.ResourceSignaturesCount; ++i)
  {
    writer << *desc.ppResourceSignatures;
  }

  writer << desc.pPSOCache;
  writer << desc.pCS;

  return writer.GetHashValue();
}

bool xiiGALCommandEncoderD3D12::ResourceCacheHash::Equal(const Diligent::ComputePipelineStateCreateInfo& a, const Diligent::ComputePipelineStateCreateInfo& b)
{
  return a == b;
}

///////////////////////////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandEncoderD3D12);
