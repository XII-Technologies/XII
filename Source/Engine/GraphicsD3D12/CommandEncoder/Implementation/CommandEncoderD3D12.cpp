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
  XII_GAL_DILIGENT_REF_RELEASE(m_pSynchronizationFence);
}

void xiiGALCommandEncoderD3D12::SetShaderPlatform(xiiGALShader* pShader)
{
}

void xiiGALCommandEncoderD3D12::SetConstantBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetSamplerPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALSampler* pSampler)
{
}

void xiiGALCommandEncoderD3D12::SetBufferViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandEncoderD3D12::SetTextureViewPlatform(xiiBitflags<xiiGALShaderStage> stage, xiiUInt32 uiSlot, xiiGALTextureView* pRTextureView)
{
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessBufferViewPlatform(xiiUInt32 uiSlot, xiiGALBufferView* pUnorderedAccessBufferView)
{
}

void xiiGALCommandEncoderD3D12::SetUnorderedAccessTextureViewPlatform(xiiUInt32 uiSlot, xiiGALTextureView* pUnorderedAccessTextureView)
{
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

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALBufferView* pBufferView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::ClearUnorderedAccessViewPlatform(xiiGALTextureView* pTextureView, xiiVec4U32 vClearValues)
{
}

void xiiGALCommandEncoderD3D12::CopyBufferPlatform(xiiGALBuffer* pDestination, xiiGALBuffer* pSource)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSource);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);

  m_pContext->CopyBuffer(pSourceBufferD3D12->GetBuffer(), 0U, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, pDestinationBufferD3D12->GetBuffer(), 0U, pDestination->GetDescription().m_uiSize, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void xiiGALCommandEncoderD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  auto pSourceBufferD3D12      = static_cast<xiiGALBufferD3D12*>(pSource);
  auto pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);

  m_pContext->CopyBuffer(pSourceBufferD3D12->GetBuffer(), uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, pDestinationBufferD3D12->GetBuffer(), uiDestOffset, uiByteCount, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void xiiGALCommandEncoderD3D12::UpdateBufferPlatform(xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> sourceData, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_CHECK_ALIGNMENT_16(sourceData.GetPtr());

  auto        pDestinationBufferD3D12 = static_cast<xiiGALBufferD3D12*>(pDestination);
  const auto& bufferDescription       = pDestinationBufferD3D12->GetDescription();

  if (bufferDescription.m_BindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == bufferDescription.m_uiSize, "Uniform (constant) buffers cannot be mapped partially, there are no checks for partial constant buffer updates.");
  }

  switch (bufferDescription.m_ResourceUsage)
  {
    case xiiGALResourceUsage::Default:
    {
      m_pContext->UpdateBuffer(pDestinationBufferD3D12->GetBuffer(), uiDestOffset, sourceData.GetCount(), reinterpret_cast<const void*>(sourceData.GetPtr()), Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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
  copyTextureDescription.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  copyTextureDescription.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

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
  copyTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  copyTextureDescription.DstMipLevel              = destinationSubResource.m_uiMipLevel;
  copyTextureDescription.DstSlice                 = destinationSubResource.m_uiArraySlice;
  copyTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  copyTextureDescription.DstX                     = vDestinationPoint.x;
  copyTextureDescription.DstY                     = vDestinationPoint.y;
  copyTextureDescription.DstZ                     = vDestinationPoint.z;

  m_pContext->CopyTexture(copyTextureDescription);
}

void xiiGALCommandEncoderD3D12::UpdateTexturePlatform(xiiGALTexture* pDestination, const xiiGALTextureMipLevelData& destinationSubResource, const xiiBoundingBoxu32& destinationBox, const xiiGALMappedTextureSubresource& sourceData)
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

      m_pContext->UpdateTexture(pDestinationTextureD3D12->GetTexture(), destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, subRegion, subResData, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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
  ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  ResolveTexAttribs.DstMipLevel              = destinationSubResource.m_uiMipLevel;
  ResolveTexAttribs.DstSlice                 = destinationSubResource.m_uiArraySlice;
  ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

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
    resolveTextureSubresourceDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
    resolveTextureSubresourceDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

    m_pContext->ResolveTextureSubresource(pTextureD3D12->GetTexture(), pStagingTextureD3D12->GetTexture(), resolveTextureSubresourceDescription);
  }
  else
  {
    Diligent::CopyTextureAttribs copyTextureDescription;
    copyTextureDescription.pSrcTexture              = pTextureD3D12->GetTexture();
    copyTextureDescription.pDstTexture              = pStagingTextureD3D12->GetTexture();
    copyTextureDescription.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
    copyTextureDescription.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

    m_pContext->CopyTexture(copyTextureDescription);
  }

  /// \todo Add GPU Synchronization.
}

xiiUInt32 GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel)
{
  for (xiiUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return xiiMath::Max(1u, uiSize);
}

void xiiGALCommandEncoderD3D12::CopyTextureReadbackResultPlatform(xiiGALTexture* pTexture, xiiGALTexture* pStagingTexture, xiiArrayPtr<xiiGALTextureMipLevelData> mipLevelData, xiiArrayPtr<xiiGALMappedTextureSubresource> targetData)
{
  auto pTextureD3D12        = static_cast<xiiGALTextureD3D12*>(pTexture);
  auto pStagingTextureD3D12 = static_cast<xiiGALTextureD3D12*>(pStagingTexture);

  XII_ASSERT_DEV(mipLevelData.GetCount() == targetData.GetCount(), "Source and target arrays must be of the same size.");

  const auto& textureDescription = pTextureD3D12->GetDescription();

  const xiiUInt32 uiSubResources = mipLevelData.GetCount();
  for (xiiUInt32 i = 0; i < uiSubResources; ++i)
  {
    const xiiGALTextureMipLevelData&      subResourceData = mipLevelData[i];
    const xiiGALMappedTextureSubresource& textureData     = targetData[i];

    Diligent::MappedTextureSubresource mappedSubResource = {};
    m_pContext->MapTextureSubresource(pTextureD3D12->GetTexture(), subResourceData.m_uiMipLevel, subResourceData.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_DO_NOT_WAIT, nullptr, mappedSubResource);

    /// \todo Wait for GPU to synchronize.

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

void xiiGALCommandEncoderD3D12::InsertEventMarkerPlatform(xiiStringView sMarker)
{
  // TODO: Add support for debug label colours.

  m_pContext->InsertDebugLabel(sMarker.GetStartPointer());
}

void xiiGALCommandEncoderD3D12::ClearPlatform(const xiiColor& clearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

void xiiGALCommandEncoderD3D12::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

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

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCount;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.BaseVertex            = 0U;
  drawAttribs.NumInstances          = 1U;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCountPerInstance;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0U;
  drawAttribs.FirstInstanceLocation = 0U;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawIndexedInstancedIndirectPlatform(xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  Diligent::DrawIndexedIndirectAttribs drawAttribs;
  drawAttribs.IndexType                        = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferD3D12*>(pIndirectArgumentBuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1U;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 5U;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0U;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->DrawIndexedIndirect(drawAttribs);
}

void xiiGALCommandEncoderD3D12::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

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

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawAttribs;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferD3D12*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1U;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 4U;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0U;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->DrawIndirect(drawAttribs);
}

void xiiGALCommandEncoderD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetVertexBufferPlatform(xiiUInt32 uiSlot, xiiGALBuffer* pVertexBuffer)
{
}

void xiiGALCommandEncoderD3D12::SetInputLayoutPlatform(xiiGALInputLayout* pInputLayout)
{
}

void xiiGALCommandEncoderD3D12::SetPrimitiveTopologyPlatform(xiiEnum<xiiGALPrimitiveTopology> topology)
{
}

void xiiGALCommandEncoderD3D12::SetBlendStatePlatform(xiiGALBlendState* pBlendState, const xiiColor& blendFactor, xiiUInt32 uiSampleMask)
{
}

void xiiGALCommandEncoderD3D12::SetDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
}

void xiiGALCommandEncoderD3D12::SetRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
}

void xiiGALCommandEncoderD3D12::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
}

void xiiGALCommandEncoderD3D12::SetScissorRectPlatform(const xiiRectU32& rect)
{
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
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  DispatchAttribs.DispatchArgsByteOffset           = uiArgumentOffsetInBytes;

  m_pContext->DispatchComputeIndirect(DispatchAttribs);
}

void xiiGALCommandEncoderD3D12::BeginRendering(xiiGALRenderPassD3D12* pRenderPassD3D12, xiiGALFramebufferD3D12* pFramebufferD3D12)
{
  m_pRenderPass  = pRenderPassD3D12;
  m_pFramebuffer = pFramebufferD3D12;
}

void xiiGALCommandEncoderD3D12::EndRendering()
{
}

void xiiGALCommandEncoderD3D12::BeginCompute()
{
}

void xiiGALCommandEncoderD3D12::EndCompute()
{
}

void xiiGALCommandEncoderD3D12::FlushDeferredStateChanges()
{
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
    writer << (desc.GraphicsPipeline.InputLayout.LayoutElements + i);
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
