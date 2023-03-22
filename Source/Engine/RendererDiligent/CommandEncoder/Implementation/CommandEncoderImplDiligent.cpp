#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/QueryDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererDiligent/State/StateDiligent.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#undef NULL
#define NULL 0

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

xiiGALCommandEncoderImplDiligent::xiiGALCommandEncoderImplDiligent(xiiGALDeviceDiligent& deviceDiligent) :
  m_GALDeviceDiligent(deviceDiligent), m_pContext(m_GALDeviceDiligent.GetImmediateContext())
{
  // Pipeline state object encompasses configuration of all GPU stages

  // Graphics Pipeline
  {
    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    m_PipelineStateDesc.PSODesc.Name = "Graphics Pipeline State";

    // This is a graphics pipeline
    m_PipelineStateDesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;

    // Define variable type that will be used by default
    m_PipelineStateDesc.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
  }

  // Compute Pipeline
  {
    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    m_PipelineStateComputeDesc.PSODesc.Name = "Compute Pipeline State";

    // This is a graphics pipeline
    m_PipelineStateComputeDesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_COMPUTE;

    // Define variable type that will be used by default
    m_PipelineStateComputeDesc.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
  }
}

xiiGALCommandEncoderImplDiligent::~xiiGALCommandEncoderImplDiligent()
{
  m_bPipelineStateModified   = true;
  m_PipelineStateDesc        = Diligent::GraphicsPipelineStateCreateInfo();
  m_PipelineStateComputeDesc = Diligent::ComputePipelineStateCreateInfo();

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0;

  m_pIndexBuffer = nullptr;
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
    m_VertexBufferStrides[i] = 0;
  }

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_CONSTANT_BUFFER_COUNT; ++i)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    m_pBoundShaderResourceViews[i].Clear();
  }
  m_pBoundUnoderedAccessViews.Clear();

  xiiMemoryUtils::ZeroFill(&m_pBoundSamplerStates[0][0], xiiGALShaderStage::ENUM_COUNT * XII_GAL_MAX_SAMPLER_COUNT);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateGraphics);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingGraphics);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateCompute);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingCompute);
}

// State setting functions

void xiiGALCommandEncoderImplDiligent::SetShaderPlatform(const xiiGALShader* pShader)
{
  if (m_pCurrentShader != pShader)
  {
    xiiGALShader* pShaderNonConst = const_cast<xiiGALShader*>(pShader);
    m_pCurrentShader              = pShader != nullptr ? static_cast<xiiGALShaderDiligent*>(pShaderNonConst) : nullptr;
    m_bPipelineStateModified      = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALBuffer* pGALBuffer = const_cast<xiiGALBuffer*>(pBuffer);

  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pGALBuffer) : nullptr;
  m_bDescriptorsModified          = true;
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALSamplerState* pGALSampler = const_cast<xiiGALSamplerState*>(pSamplerState);

  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<xiiGALSamplerStateDiligent*>(pGALSampler) : nullptr;
  m_bDescriptorsModified               = true;
}

void xiiGALCommandEncoderImplDiligent::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  xiiGALResourceView* pResource    = const_cast<xiiGALResourceView*>(pResourceView);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<xiiGALResourceViewDiligent*>(pResource) : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

void xiiGALCommandEncoderImplDiligent::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessView* pUAView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);

  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView) : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

// Query functions

void xiiGALCommandEncoderImplDiligent::BeginQueryPlatform(const xiiGALQuery* pQuery)
{
  xiiGALQuery* pGALQuery = const_cast<xiiGALQuery*>(pQuery);
  m_pContext->BeginQuery(static_cast<xiiGALQueryDiligent*>(pGALQuery)->GetQuery());
}

void xiiGALCommandEncoderImplDiligent::EndQueryPlatform(const xiiGALQuery* pQuery)
{
  xiiGALQuery* pGALQuery = const_cast<xiiGALQuery*>(pQuery);
  m_pContext->EndQuery(static_cast<xiiGALQueryDiligent*>(pGALQuery)->GetQuery());
}

xiiResult xiiGALCommandEncoderImplDiligent::GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& uiQueryResult)
{
  xiiGALQuery* pGALQuery = const_cast<xiiGALQuery*>(pQuery);
  bool         bResult   = static_cast<xiiGALQueryDiligent*>(pGALQuery)->GetQuery()->GetData(static_cast<void*>(&uiQueryResult), sizeof(uiQueryResult), false);
  return bResult ? XII_SUCCESS : XII_FAILURE;
}

// Timestamp functions

void xiiGALCommandEncoderImplDiligent::InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp)
{
}

// Resource update functions

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues)
{
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessViewDiligent = nullptr;
  Diligent::IBufferView*             pUAVDiligent                 = nullptr;
  {
    xiiGALUnorderedAccessView* pGALUnorderedAccessView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);
    pUnorderedAccessViewDiligent                       = static_cast<xiiGALUnorderedAccessViewDiligent*>(pGALUnorderedAccessView);
    pUAVDiligent                                       = pUnorderedAccessViewDiligent->GetBufferView();
  }

  switch (m_GALDeviceDiligent.GetDeviceType())
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      /// \todo Implement unordered access view clearing in D3D11
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
      // \todo Implement unordered access view clearing in D3D12
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
    {
      // \todo Implement unordered access view clearing in Vulkan
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessViewDiligent = nullptr;
  {
    xiiGALUnorderedAccessView* pGALUnorderedAccessView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);
    pUnorderedAccessViewDiligent                       = static_cast<xiiGALUnorderedAccessViewDiligent*>(pGALUnorderedAccessView);
  }

  switch (m_GALDeviceDiligent.GetDeviceType())
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      /// \todo Implement unordered access view clearing in D3D11
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

#if D3D12_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D12:
    {
      // \todo Implement unordered access view clearing in D3D12
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

#if VULKAN_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_VULKAN:
    {
      // \todo Implement unordered access view clearing in Vulkan
      XII_ASSERT_NOT_IMPLEMENTED;
    }
    break;
#endif

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderImplDiligent::CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource)
{
  xiiGALBuffer*      pSrc               = const_cast<xiiGALBuffer*>(pSource);
  xiiGALBuffer*      pDst               = const_cast<xiiGALBuffer*>(pDestination);
  Diligent::IBuffer* pDestinationBuffer = static_cast<xiiGALBufferDiligent*>(pDst)->GetBuffer();
  Diligent::IBuffer* pSourceBuffer      = static_cast<xiiGALBufferDiligent*>(pSrc)->GetBuffer();

  m_pContext->CopyBuffer(pSourceBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBuffer, 0, pDestination->GetSize(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  xiiGALBuffer*      pSrc               = const_cast<xiiGALBuffer*>(pSource);
  xiiGALBuffer*      pDst               = const_cast<xiiGALBuffer*>(pDestination);
  Diligent::IBuffer* pDestinationBuffer = static_cast<xiiGALBufferDiligent*>(pDst)->GetBuffer();
  Diligent::IBuffer* pSourceBuffer      = static_cast<xiiGALBufferDiligent*>(pSrc)->GetBuffer();

  m_pContext->CopyBuffer(pSourceBuffer, uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBuffer, uiDestOffset, uiByteCount, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiGALUpdateMode::Enum updateMode)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  Diligent::IBuffer* pDestinationBuffer = nullptr;
  {
    xiiGALBuffer* pDestinationNonConst = const_cast<xiiGALBuffer*>(pDestination);
    pDestinationBuffer                 = static_cast<xiiGALBufferDiligent*>(pDestinationNonConst)->GetBuffer();
  }

  const Diligent::BufferDesc& bufferDescription = pDestinationBuffer->GetDesc();
  Diligent::MAP_FLAGS         mapFlags          = Diligent::MAP_FLAG_NONE;

  if (updateMode & xiiGALUpdateMode::DoNotWait)
    mapFlags |= Diligent::MAP_FLAG_DO_NOT_WAIT;

  if (updateMode & xiiGALUpdateMode::Discard)
  {
    mapFlags |= Diligent::MAP_FLAG_DISCARD;

    XII_ASSERT_DEV(pSourceData.GetCount() == pDestination->GetSize(), "Only the entire buffer can currently be mapped with the Discard flag.");
  }

  if (updateMode & xiiGALUpdateMode::NoOverWrite)
    mapFlags |= Diligent::MAP_FLAG_NO_OVERWRITE;

  switch (bufferDescription.Usage)
  {
    case Diligent::USAGE_DEFAULT:
    {
      m_pContext->UpdateBuffer(pDestinationBuffer, uiDestOffset, pSourceData.GetCount(), reinterpret_cast<const void*>(pSourceData.GetPtr()), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    break;

    case Diligent::USAGE_DYNAMIC:
    {
      Diligent::PVoid pMapResult;
      m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, mapFlags, reinterpret_cast<Diligent::PVoid&>(pMapResult));

      if (pMapResult)
      {
        memcpy(xiiMemoryUtils::AddByteOffset((xiiUInt8*)pMapResult, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

        m_pContext->UnmapBuffer(pDestinationBuffer, Diligent::MAP_WRITE);
      }
      else
      {
        xiiLog::Error("Failed to map buffer to update content.");
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
}

void xiiGALCommandEncoderImplDiligent::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  Diligent::CopyTextureAttribs CopyTexAttribs = {};
  CopyTexAttribs.pSrcTexture                  = pSourceTexture;
  CopyTexAttribs.pDstTexture                  = pDestinationTexture;
  CopyTexAttribs.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  CopyTexAttribs.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->CopyTexture(CopyTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  Diligent::Box srcBox = {};
  srcBox.MinX          = Box.m_vMin.x;
  srcBox.MinY          = Box.m_vMin.y;
  srcBox.MinZ          = Box.m_vMin.z;
  srcBox.MaxX          = Box.m_vMax.x;
  srcBox.MaxY          = Box.m_vMax.y;
  srcBox.MaxZ          = Box.m_vMax.z;

  Diligent::CopyTextureAttribs CopyTexAttribs = {};
  CopyTexAttribs.pSrcTexture                  = pSourceTexture;
  CopyTexAttribs.pDstTexture                  = pDestinationTexture;
  CopyTexAttribs.pSrcBox                      = &srcBox;

  CopyTexAttribs.SrcMipLevel              = SourceSubResource.m_uiMipLevel;
  CopyTexAttribs.SrcSlice                 = SourceSubResource.m_uiArraySlice;
  CopyTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  CopyTexAttribs.DstMipLevel              = DestinationSubResource.m_uiMipLevel;
  CopyTexAttribs.DstSlice                 = DestinationSubResource.m_uiArraySlice;
  CopyTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  CopyTexAttribs.DstX                     = DestinationPoint.x;
  CopyTexAttribs.DstY                     = DestinationPoint.y;
  CopyTexAttribs.DstZ                     = DestinationPoint.z;

  m_pContext->CopyTexture(CopyTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData)
{
  Diligent::ITexture* pDestinationTexture = nullptr;
  {
    xiiGALTexture* pDestinationNonConst = const_cast<xiiGALTexture*>(pDestination);
    pDestinationTexture                 = static_cast<xiiGALTextureDiligent*>(pDestinationNonConst)->GetTexture();
  }

  xiiUInt32                  uiWidth  = xiiMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  xiiUInt32                  uiHeight = xiiMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  xiiUInt32                  uiDepth  = xiiMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  xiiGALResourceFormat::Enum format   = pDestination->GetDescription().m_Format;

  const Diligent::TextureDesc& textureDescription = pDestinationTexture->GetDesc();
  switch (textureDescription.Usage)
  {
    case Diligent::USAGE_DEFAULT:
    {
      xiiUInt32 uiRowPitch   = uiWidth * xiiGALResourceFormat::GetBitsPerElement(format) / 8;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;
      XII_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
      XII_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
                     uiSlicePitch, pSourceData.m_uiSlicePitch);

      Diligent::Box SubRegion = {};
      SubRegion.MinX          = DestinationBox.m_vMin.x;
      SubRegion.MinY          = DestinationBox.m_vMin.y;
      SubRegion.MinZ          = DestinationBox.m_vMin.z;
      SubRegion.MaxX          = DestinationBox.m_vMax.x;
      SubRegion.MaxY          = DestinationBox.m_vMax.y;
      SubRegion.MaxZ          = DestinationBox.m_vMax.z;

      Diligent::TextureSubResData SubResData = {};
      SubResData.pData                       = pSourceData.m_pData;
      SubResData.Stride                      = uiRowPitch;
      SubResData.DepthStride                 = uiSlicePitch;

      m_pContext->UpdateTexture(pDestinationTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, SubRegion, SubResData, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    break;

    case Diligent::USAGE_DYNAMIC:
    {
      Diligent::Box SubRegion = {};
      SubRegion.MinX          = DestinationBox.m_vMin.x;
      SubRegion.MinY          = DestinationBox.m_vMin.y;
      SubRegion.MinZ          = DestinationBox.m_vMin.z;
      SubRegion.MaxX          = DestinationBox.m_vMax.x;
      SubRegion.MaxY          = DestinationBox.m_vMax.y;
      SubRegion.MaxZ          = DestinationBox.m_vMax.z;

      Diligent::MappedTextureSubresource MapResult;
      m_pContext->MapTextureSubresource(pDestinationTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, &SubRegion, MapResult);

      xiiUInt32 uiRowPitch   = uiWidth * xiiGALResourceFormat::GetBitsPerElement(format) / 8;
      xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;
      XII_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
      XII_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
                     uiSlicePitch, pSourceData.m_uiSlicePitch);

      if (MapResult.Stride == uiRowPitch && MapResult.DepthStride == uiSlicePitch)
      {
        memcpy(MapResult.pData, pSourceData.m_pData, uiSlicePitch * uiDepth);
      }
      else
      {
        // Copy by row
        for (xiiUInt32 z = 0; z < uiDepth; ++z)
        {
          const void* pSource = xiiMemoryUtils::AddByteOffset(pSourceData.m_pData, z * uiSlicePitch);
          void*       pDest   = xiiMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthStride);

          for (xiiUInt32 y = 0; y < uiHeight; ++y)
          {
            memcpy(pDest, pSource, uiRowPitch);

            pSource = xiiMemoryUtils::AddByteOffset(pSource, uiRowPitch);
            pDest   = xiiMemoryUtils::AddByteOffset(pDest, MapResult.Stride);
          }
        }
      }

      m_pContext->UnmapTextureSubresource(pDestinationTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
}

void xiiGALCommandEncoderImplDiligent::ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  Diligent::TEXTURE_FORMAT Format = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
  ResolveTexAttribs.Format = Format;

  ResolveTexAttribs.SrcMipLevel              = SourceSubResource.m_uiMipLevel;
  ResolveTexAttribs.SrcSlice                 = SourceSubResource.m_uiArraySlice;
  ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  ResolveTexAttribs.DstMipLevel              = DestinationSubResource.m_uiMipLevel;
  ResolveTexAttribs.DstSlice                 = DestinationSubResource.m_uiArraySlice;
  ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->ResolveTextureSubresource(pSourceTexture, pDestinationTexture, ResolveTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::ReadbackTexturePlatform(const xiiGALTexture* pTexture)
{
  xiiGALTexture*         pTex             = const_cast<xiiGALTexture*>(pTexture);
  xiiGALTextureDiligent* pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pTex);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pTextureDiligent->GetDescription().m_SampleCount != xiiGALMSAASampleCount::None;

  XII_ASSERT_DEV(pTextureDiligent->GetStagingTexture() != nullptr, "No staging resource available for read-back");
  XII_ASSERT_DEV(pTextureDiligent->GetTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
    ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pContext->ResolveTextureSubresource(pTextureDiligent->GetTexture(), pTextureDiligent->GetStagingTexture(), ResolveTexAttribs);
  }
  else
  {
    Diligent::CopyTextureAttribs CopyTexAttribs;
    CopyTexAttribs.pSrcTexture              = pTextureDiligent->GetTexture();
    CopyTexAttribs.pDstTexture              = pTextureDiligent->GetStagingTexture();
    CopyTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    CopyTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pContext->CopyTexture(CopyTexAttribs);
  }
}

xiiUInt32 GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel)
{
  for (xiiUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return xiiMath::Max(1u, uiSize);
}

void xiiGALCommandEncoderImplDiligent::CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData)
{
  xiiGALTexture*         pTex             = const_cast<xiiGALTexture*>(pTexture);
  xiiGALTextureDiligent* pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pTex);

  XII_ASSERT_DEV(pTextureDiligent->GetStagingTexture() != nullptr, "No staging resource available for read-back");
  XII_ASSERT_DEV(SourceSubResource.GetCount() == TargetData.GetCount(), "Source and target arrays must be of the same size.");

  const xiiUInt32 uiSubResources = SourceSubResource.GetCount();
  for (xiiUInt32 i = 0; i < uiSubResources; i++)
  {
    const xiiGALTextureSubresource&      subRes  = SourceSubResource[i];
    const xiiGALSystemMemoryDescription& memDesc = TargetData[i];

    Diligent::MappedTextureSubresource MappedSubRes;
    m_pContext->MapTextureSubresource(pTextureDiligent->GetStagingTexture(), subRes.m_uiMipLevel, subRes.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, nullptr, MappedSubRes);
    {
      // TODO: Depth pitch
      if (MappedSubRes.Stride == memDesc.m_uiRowPitch)
      {
        const xiiUInt32 uiMemorySize = xiiGALResourceFormat::GetBitsPerElement(pTextureDiligent->GetDescription().m_Format) * GetMipSize(pTextureDiligent->GetDescription().m_uiWidth, subRes.m_uiMipLevel) * GetMipSize(pTextureDiligent->GetDescription().m_uiHeight, subRes.m_uiMipLevel) / 8;

        memcpy(memDesc.m_pData, MappedSubRes.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const xiiUInt32 uiHeight = GetMipSize(pTextureDiligent->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = xiiMemoryUtils::AddByteOffset(MappedSubRes.pData, y * MappedSubRes.Stride);
          void*       pDest   = xiiMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(pDest, pSource, xiiGALResourceFormat::GetBitsPerElement(pTextureDiligent->GetDescription().m_Format) * GetMipSize(pTextureDiligent->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }

      m_pContext->UnmapTextureSubresource(pTextureDiligent->GetStagingTexture(), subRes.m_uiMipLevel, subRes.m_uiArraySlice);
    }
  }
}

void xiiGALCommandEncoderImplDiligent::GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView)
{
  xiiGALResourceView*         pResource             = const_cast<xiiGALResourceView*>(pResourceView);
  xiiGALResourceViewDiligent* pResourceViewDiligent = static_cast<xiiGALResourceViewDiligent*>(pResource);

  m_pContext->GenerateMips(pResourceViewDiligent->GetTextureView());
}

void xiiGALCommandEncoderImplDiligent::FlushPlatform()
{
  m_pContext->Flush();
}

// Debug helper functions

void xiiGALCommandEncoderImplDiligent::PushMarkerPlatform(const char* szMarker)
{
  /// \todo Fix Debug Groups
  // m_pContext->BeginDebugGroup(szMarker);
}

void xiiGALCommandEncoderImplDiligent::PopMarkerPlatform()
{
  /// \todo Fix Debug Groups
  // m_pContext->EndDebugGroup();
}

void xiiGALCommandEncoderImplDiligent::InsertEventMarkerPlatform(const char* szMarker)
{
  /// \todo Fix Debug Groups
  // m_pContext->InsertDebugLabel(szMarker);
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::BeginRendering(const xiiGALRenderingSetup& renderingSetup)
{
  if (m_RenderingSetup != renderingSetup)
  {
    m_RenderingSetup = renderingSetup;
  }

  // ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void xiiGALCommandEncoderImplDiligent::EndRendering()
{
}

// Draw functions

void xiiGALCommandEncoderImplDiligent::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  const bool      bHasDepth              = !m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt32 uiColorAttachmentCount = m_RenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  if (uiRenderTargetClearMask != 0)
  {
    for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
    {
      if (uiRenderTargetClearMask & (1u << i) && i < uiColorAttachmentCount)
      {
        xiiGALRenderTargetViewHandle hColorRenderTarget = m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i));

        xiiGALRenderTargetView*         pGALRenderTargetView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));
        xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALRenderTargetView);

        m_pContext->ClearRenderTarget(pRenderTargetViewDiligent->GetRenderTargetView(), ClearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      }
    }
  }

  if (bHasDepth && (bClearDepth || bClearStencil))
  {
    xiiGALRenderTargetView*         pGALDepthStencilView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
    xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALDepthStencilView);

    Diligent::CLEAR_DEPTH_STENCIL_FLAGS flags = Diligent::CLEAR_DEPTH_FLAG_NONE;

    if (bClearDepth)
      flags |= Diligent::CLEAR_DEPTH_FLAG;

    if (bClearStencil)
      flags |= Diligent::CLEAR_STENCIL_FLAG;

    m_pContext->ClearDepthStencil(pRenderTargetViewDiligent->GetDepthStencilView(), flags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
}

void xiiGALCommandEncoderImplDiligent::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChangesGraphics();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCount;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.NumInstances          = 1;
  drawAttribs.FirstInstanceLocation = 0;
  drawAttribs.StartVertexLocation   = uiStartVertex;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChangesGraphics();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCount;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.NumInstances          = 1;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0;
  drawAttribs.FirstInstanceLocation = 0;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChangesGraphics();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.IndexType             = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.NumIndices            = uiIndexCountPerInstance;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0;
  drawAttribs.FirstInstanceLocation = 0;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChangesGraphics();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndexedIndirectAttribs drawAttribs;
  drawAttribs.IndexType                        = m_pIndexBuffer != nullptr ? m_pIndexBuffer->GetIndexFormat() : Diligent::VT_UNDEFINED;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 5;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->DrawIndexedIndirect(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChangesGraphics();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCountPerInstance;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.FirstInstanceLocation = 0;
  drawAttribs.StartVertexLocation   = uiStartVertex;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChangesGraphics();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawAttribs;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
  drawAttribs.DrawCount                        = 1;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 4;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->DrawIndirect(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawAutoPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::BeginStreamOutPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::EndStreamOutPlatform()
{
}

void xiiGALCommandEncoderImplDiligent::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
  xiiGALBuffer* pGALIndexBuffer = const_cast<xiiGALBuffer*>(pIndexBuffer);

  if (m_pIndexBuffer != pIndexBuffer)
  {
    m_pIndexBuffer         = pIndexBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pGALIndexBuffer) : nullptr;
    m_bIndexBufferModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)
{
  XII_ASSERT_DEV(uiSlot < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  xiiGALBuffer*      pVBuffer         = const_cast<xiiGALBuffer*>(pVertexBuffer);
  Diligent::IBuffer* pVBufferDiligent = pVertexBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pVBuffer)->GetBuffer() : nullptr;
  xiiUInt32          stride           = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;

  if (m_pBoundVertexBuffers[uiSlot] != pVBufferDiligent)
  {
    m_pBoundVertexBuffers[uiSlot] = pVBufferDiligent;
    m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);

    if (m_VertexBufferStrides[uiSlot] != stride)
    {
      m_VertexBufferStrides[uiSlot] = stride;
      m_bPipelineStateModified      = true;
    }
  }
}

void xiiGALCommandEncoderImplDiligent::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  if (m_pVertexDeclaration != pVertexDeclaration)
  {
    m_pVertexDeclaration     = static_cast<const xiiGALVertexDeclarationDiligent*>(pVertexDeclaration);
    m_bPipelineStateModified = true;
  }
}

static const Diligent::PRIMITIVE_TOPOLOGY GALTopologyToDiligent[xiiGALPrimitiveTopology::ENUM_COUNT] = {
  Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

void xiiGALCommandEncoderImplDiligent::SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology)
{
  if (m_PrimitiveTopology != GALTopologyToDiligent[Topology])
  {
    m_PrimitiveTopology      = GALTopologyToDiligent[Topology];
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  const float BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pContext->SetBlendFactors(BlendFactors);

  if (m_pBlendStateState != pBlendState)
  {
    m_pBlendStateState       = pBlendState != nullptr ? static_cast<const xiiGALBlendStateDiligent*>(pBlendState) : nullptr;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  /// \todo Diligent: Implement uiStenciValue

  if (m_pDepthStencilState != pDepthStencilState)
  {
    m_pDepthStencilState     = pDepthStencilState != nullptr ? static_cast<const xiiGALDepthStencilStateDiligent*>(pDepthStencilState) : nullptr;
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  if (m_pRasterizerState != pRasterizerState)
  {
    m_pRasterizerState       = pRasterizerState != nullptr ? static_cast<const xiiGALRasterizerStateDiligent*>(pRasterizerState) : nullptr;
    m_bPipelineStateModified = true;

    if (m_pRasterizerState != nullptr)
    {
      if (m_pRasterizerState->GetRasterizerStateDesc()->ScissorEnable != m_bScissorEnabled)
      {
        m_bScissorEnabled   = m_pRasterizerState->GetRasterizerStateDesc()->ScissorEnable;
        m_bViewportModified = true;
      }
    }
  }
}

void xiiGALCommandEncoderImplDiligent::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  if (m_Viewport.TopLeftX != rect.x || m_Viewport.TopLeftY != rect.y || m_Viewport.Width != rect.width || m_Viewport.Height != rect.height || m_Viewport.MinDepth != fMinDepth || m_Viewport.MaxDepth != fMaxDepth)
  {
    m_Viewport.TopLeftX = rect.x;
    m_Viewport.TopLeftY = rect.y;
    m_Viewport.Width    = rect.width;
    m_Viewport.Height   = rect.height;
    m_Viewport.MinDepth = fMinDepth;
    m_Viewport.MaxDepth = fMaxDepth;

    m_bViewportModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetScissorRectPlatform(const xiiRectU32& rect)
{
  if (m_ScissorRect.left != rect.x || m_ScissorRect.top != rect.y || m_ScissorRect.right != (rect.x + rect.width) || m_ScissorRect.bottom != (rect.y + rect.height))
  {
    m_ScissorRect.left   = rect.x;
    m_ScissorRect.top    = rect.y;
    m_ScissorRect.right  = rect.x + rect.width;
    m_ScissorRect.bottom = rect.y + rect.height;

    m_bViewportModified = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::BeginCompute()
{
  m_bComputePipelineRequested = true;
}

void xiiGALCommandEncoderImplDiligent::EndCompute()
{
  m_bComputePipelineRequested = false;
}

void xiiGALCommandEncoderImplDiligent::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChangesCompute();

  Diligent::DispatchComputeAttribs DispatchAttribs;
  DispatchAttribs.ThreadGroupCountX = uiThreadGroupCountX;
  DispatchAttribs.ThreadGroupCountY = uiThreadGroupCountY;
  DispatchAttribs.ThreadGroupCountZ = uiThreadGroupCountZ;

  // These are only needed in a Metal backend.
  DispatchAttribs.MtlThreadGroupSizeX = 0;
  DispatchAttribs.MtlThreadGroupSizeY = 0;
  DispatchAttribs.MtlThreadGroupSizeZ = 0;

  m_pContext->DispatchCompute(DispatchAttribs);
}

void xiiGALCommandEncoderImplDiligent::DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChangesCompute();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs;
  DispatchAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  DispatchAttribs.DispatchArgsByteOffset           = uiArgumentOffsetInBytes;

  // These are only needed in a Metal backend.
  DispatchAttribs.MtlThreadGroupSizeX = 0;
  DispatchAttribs.MtlThreadGroupSizeY = 0;
  DispatchAttribs.MtlThreadGroupSizeZ = 0;

  m_pContext->DispatchComputeIndirect(DispatchAttribs);
}

void xiiGALCommandEncoderImplDiligent::MarkDirty()
{
  m_bPipelineStateModified = true;
  m_bViewportModified      = true;
  m_bIndexBufferModified   = true;
  m_bDescriptorsModified   = true;

  m_BoundVertexBuffersRange.Reset();
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }
}

void xiiGALCommandEncoderImplDiligent::Reset()
{
  m_bPipelineStateModified = true;
  m_bViewportModified      = true;
  m_bIndexBufferModified   = true;
  m_bDescriptorsModified   = true;

  m_BoundVertexBuffersRange.Reset();

  m_PipelineStateDesc        = Diligent::GraphicsPipelineStateCreateInfo();
  m_PipelineStateComputeDesc = Diligent::ComputePipelineStateCreateInfo();

  m_Viewport    = Diligent::Viewport();
  m_ScissorRect = Diligent::Rect();

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0; // Unused / Unset

  m_pIndexBuffer = nullptr;
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferStrides[i] = 0;
  }

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_CONSTANT_BUFFER_COUNT; ++i)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    m_pBoundShaderResourceViews[i].Clear();
  }
  m_pBoundUnoderedAccessViews.Clear();

  xiiMemoryUtils::ZeroFill(&m_pBoundSamplerStates[0][0], xiiGALShaderStage::ENUM_COUNT * XII_GAL_MAX_SAMPLER_COUNT);
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChangesCompute()
{
  XII_ASSERT_DEV(m_bComputePipelineRequested, "Compute pipeline is not requested.");

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingCompute);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateCompute);

  // Copy attributes set in the graphics pipeline state
  m_PipelineStateComputeDesc.PSODesc.SRBAllocationGranularity = m_PipelineStateDesc.PSODesc.SRBAllocationGranularity;
  m_PipelineStateComputeDesc.PSODesc.ImmediateContextMask     = m_PipelineStateDesc.PSODesc.ImmediateContextMask;
  m_PipelineStateComputeDesc.PSODesc.ResourceLayout           = m_PipelineStateDesc.PSODesc.ResourceLayout;

  if (!m_pCurrentShader)
  {
    xiiLog::Error("No shader set in pipeline");
    return;
  }

  m_PipelineStateComputeDesc.pCS = m_pCurrentShader->GetComputeShader();

  m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateComputeDesc, &m_pPipelineStateCompute);

  XII_ASSERT_DEV(m_pPipelineStateCompute != nullptr, "Failed to create compute pipeline state.");

  // Always fill descriptor bindings for now
  FillDescriptorBindings(m_pPipelineStateCompute);

  // Create a shader resource binding object and bind all static resources in it
  m_pPipelineStateCompute->CreateShaderResourceBinding(&m_pShaderResourceBindingCompute, true);

  m_pContext->SetPipelineState(m_pPipelineStateCompute);

  m_pContext->CommitShaderResources(m_pShaderResourceBindingCompute, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  m_bComputePipelineRequested = false;
}

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChangesGraphics()
{
  XII_ASSERT_DEV(!m_bComputePipelineRequested, "Cannot flush deferred state changes while the compute pipeline is active");

  // Retrieve the current render pass
  {
    xiiGALPassDiligent*    pDefaultPass              = m_GALDeviceDiligent.m_pDefaultPass.Borrow();
    Diligent::IRenderPass* pRenderPass               = pDefaultPass->GetRenderPass(m_RenderingSetup);
    m_PipelineStateDesc.GraphicsPipeline.pRenderPass = pRenderPass;
  }

  if (m_bPipelineStateModified)
  {
    if (!m_pCurrentShader)
    {
      xiiLog::Error("No shader set in pipeline");
      return;
    }

    m_PipelineStateDesc.pVS = m_pCurrentShader->GetVertexShader();
    m_PipelineStateDesc.pPS = m_pCurrentShader->GetPixelShader();
    m_PipelineStateDesc.pDS = m_pCurrentShader->GetDomainShader();
    m_PipelineStateDesc.pHS = m_pCurrentShader->GetHullShader();
    m_PipelineStateDesc.pGS = m_pCurrentShader->GetGeometryShader();
    m_PipelineStateDesc.pAS = nullptr; // Not yet supported
    m_PipelineStateDesc.pMS = nullptr; // Not yet supported

    m_PipelineStateDesc.GraphicsPipeline.PrimitiveTopology = m_PrimitiveTopology;

    if (m_pVertexDeclaration)
      m_PipelineStateDesc.GraphicsPipeline.InputLayout = *m_pVertexDeclaration->GetInputLayoutDesc();

    if (m_pBlendStateState)
      m_PipelineStateDesc.GraphicsPipeline.BlendDesc = *m_pBlendStateState->GetBlendStateDesc();

    if (m_pDepthStencilState)
      m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc = *m_pDepthStencilState->GetDepthStencilStateDesc();

    if (m_pRasterizerState)
      m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc = *m_pRasterizerState->GetRasterizerStateDesc();

    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingGraphics);

    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateGraphics);

    m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateDesc, &m_pPipelineStateGraphics);

    XII_ASSERT_DEV(m_pPipelineStateGraphics != nullptr, "Failed to create graphics pipeline state.");

    // Do not set m_bPipelineStateModified to false here, some updates are deferred to the end of the function.

    // Changes to the descriptor layout always require the descriptor set to be re-created.
    m_bDescriptorsModified = true;
  }

  if (m_bViewportModified)
  {
    m_pContext->SetViewports(1, &m_Viewport, static_cast<xiiUInt32>(m_Viewport.Width), static_cast<xiiUInt32>(m_Viewport.Height));

    if (m_bScissorEnabled)
    {
      m_pContext->SetScissorRects(1, &m_ScissorRect, m_ScissorRect.right - m_ScissorRect.left, m_ScissorRect.bottom - m_ScissorRect.top);
    }
    else
    {
      Diligent::Rect ViewRectNoScissor;
      ViewRectNoScissor.left   = (xiiUInt32)m_Viewport.TopLeftX;
      ViewRectNoScissor.top    = (xiiUInt32)m_Viewport.TopLeftY;
      ViewRectNoScissor.right  = (xiiUInt32)m_Viewport.Width;
      ViewRectNoScissor.bottom = (xiiUInt32)m_Viewport.Height;

      m_pContext->SetScissorRects(1, &ViewRectNoScissor, ViewRectNoScissor.right - ViewRectNoScissor.left, ViewRectNoScissor.bottom - ViewRectNoScissor.top);
    }
    m_bViewportModified = false;
  }

  if (m_BoundVertexBuffersRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const xiiUInt32 uiNumSlots  = m_BoundVertexBuffersRange.GetCount();

    xiiUInt32 uiCurrentStartSlot = uiStartSlot;

    // Finding valid ranges.
    for (xiiUInt32 i = uiStartSlot; i < (uiStartSlot + uiNumSlots); i++)
    {
      if (!m_pBoundVertexBuffers[i])
      {
        if (i - uiCurrentStartSlot > 0)
        {
          // There are some null elements in the array. We can't submit these to Diligent and need to skip them so flush everything before it.
          m_pContext->SetVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        }
        uiCurrentStartSlot = i + 1;
      }
    }

    // The last element in the buffer range must always be valid so we can simply flush the rest.
    if (m_pBoundVertexBuffers[uiCurrentStartSlot])
    {
      m_pContext->SetVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
    }

    m_BoundVertexBuffersRange.Reset();
  }

  if (m_bIndexBufferModified)
  {
    if (m_pIndexBuffer)
    {
      m_pContext->SetIndexBuffer(m_pIndexBuffer->GetBuffer(), 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    }
    m_bIndexBufferModified = false;
  }

  if (m_bDescriptorsModified)
  {
    FillDescriptorBindings(m_pPipelineStateGraphics);

    m_bDescriptorsModified = false;
  }

  // Create a shader resource binding object and bind all static resources in it
  m_pPipelineStateGraphics->CreateShaderResourceBinding(&m_pShaderResourceBindingGraphics, true);

  if (m_bPipelineStateModified)
  {
    m_bPipelineStateModified = false;
  }

  m_pContext->SetPipelineState(m_pPipelineStateGraphics);

  m_pContext->CommitShaderResources(m_pShaderResourceBindingGraphics, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void xiiGALCommandEncoderImplDiligent::TransitionResourceStates()
{
  xiiHybridArray<Diligent::StateTransitionDesc, 2u> stateTransitions;

  bool bVertexBufferSet = false;

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    if (m_pBoundVertexBuffers[i] == nullptr)
      continue;

    Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
    transitionDesc.pResource                      = m_pBoundVertexBuffers[i];
    transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
    transitionDesc.NewState                       = Diligent::RESOURCE_STATE_VERTEX_BUFFER;
    transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
    transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

    bVertexBufferSet = true;
  }

  if (m_pIndexBuffer != nullptr)
  {
    Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
    transitionDesc.pResource                      = m_pIndexBuffer->GetBuffer();
    transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
    transitionDesc.NewState                       = Diligent::RESOURCE_STATE_INDEX_BUFFER;
    transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
    transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
  }

  if (m_pCurrentShader != nullptr)
  {
    for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto& bindings = m_pCurrentShader->GetDescriptorSets((xiiGALShaderStage::Enum)stage);
      for (xiiUInt32 i = 0; i < bindings.GetCount(); ++i)
      {
        auto& binding = bindings[i].Bindings;

        for (xiiUInt32 j = 0; j < binding.GetCount(); ++j)
        {
          auto& currentBinding = binding[j];

          xiiStringBuilder sData;
          currentBinding.m_sName.GetData(sData);

          switch (currentBinding.m_Type)
          {
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::ConstantBuffer:
            {
              Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
              transitionDesc.pResource                      = m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer();
              transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
              transitionDesc.NewState                       = Diligent::RESOURCE_STATE_CONSTANT_BUFFER;
              transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
              transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceView:
            {
              if (Diligent::ITextureView* pTextureView = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetTextureView())
              {
                auto& description = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetDescription();

                Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
                transitionDesc.pResource                      = pTextureView->GetTexture();
                transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
                transitionDesc.NewState                       = Diligent::RESOURCE_STATE_SHADER_RESOURCE;
                transitionDesc.FirstMipLevel                  = description.m_uiMostDetailedMipLevel;
                transitionDesc.MipLevelsCount                 = description.m_uiMipLevelsToUse;
                transitionDesc.FirstArraySlice                = description.m_uiFirstArraySlice;
                transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
                transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
              }

              if (Diligent::IBufferView* pBufferView = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetBufferView())
              {
                auto& description = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetDescription();

                Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
                transitionDesc.pResource                      = pBufferView->GetBuffer();
                transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
                transitionDesc.NewState                       = Diligent::RESOURCE_STATE_SHADER_RESOURCE;
                transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
                transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
              }
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessView:
            {
              if (Diligent::ITextureView* pTextureView = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetTextureView())
              {
                auto& description = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetDescription();

                Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
                transitionDesc.pResource                      = pTextureView->GetTexture();
                transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
                transitionDesc.NewState                       = Diligent::RESOURCE_STATE_SHADER_RESOURCE;
                transitionDesc.FirstMipLevel                  = description.m_uiMipLevelToUse;
                transitionDesc.FirstArraySlice                = description.m_uiFirstArraySlice;
                transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
                transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
              }

              if (Diligent::IBufferView* pBufferView = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetBufferView())
              {
                auto& description = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetDescription();

                Diligent::StateTransitionDesc& transitionDesc = stateTransitions.ExpandAndGetRef();
                transitionDesc.pResource                      = pBufferView->GetBuffer();
                transitionDesc.OldState                       = Diligent::RESOURCE_STATE_UNKNOWN;
                transitionDesc.NewState                       = Diligent::RESOURCE_STATE_SHADER_RESOURCE;
                transitionDesc.TransitionType                 = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
                transitionDesc.Flags                          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;
              }
            }
            break;
          }
        }
      }
    }
  }

  m_pContext->TransitionResourceStates(stateTransitions.GetCount(), stateTransitions.GetData());

  // The pipeline state requires at least one vertex buffer to be set.
  if (bVertexBufferSet)
    FlushDeferredStateChangesGraphics();
}

void xiiGALCommandEncoderImplDiligent::FillDescriptorBindings(Diligent::IPipelineState* pPipelineState)
{
  // Note that this function does not check if the bindings have been modified

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    auto& bindings = m_pCurrentShader->GetDescriptorSets((xiiGALShaderStage::Enum)stage);
    for (xiiUInt32 i = 0; i < bindings.GetCount(); ++i)
    {
      auto& binding = bindings[i].Bindings;

      for (xiiUInt32 j = 0; j < binding.GetCount(); ++j)
      {
        auto& currentBinding = binding[j];

        xiiStringBuilder sData;
        currentBinding.m_sName.GetData(sData);

        switch (currentBinding.m_Type)
        {
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ConstantBuffer:
          {
            auto* pConstantBuffer = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pConstantBuffer == nullptr)
            {
              xiiLog::Error("Constant buffer pointer for {} returned null.", sData);
              continue;
            }
            pConstantBuffer->Set(static_cast<Diligent::IBuffer*>(m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer()), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceView:
          {
            auto* pResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pResourceView == nullptr)
            {
              xiiLog::Error("Resource view pointer for {} returned null.", sData);
              continue;
            }
            pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetResourceView(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessView:
          {
            auto* pUnorderedResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pUnorderedResourceView == nullptr)
            {
              xiiLog::Error("Unordered access view pointer for {} returned null.", sData);
              continue;
            }
            pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetResourceView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::Sampler:
          {
            auto* pSampler = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pSampler == nullptr)
            {
              xiiLog::Error("Sampler pointer for {} returned null.", sData);
              continue;
            }
            pSampler->Set(m_pBoundSamplerStates[stage][currentBinding.m_uiVirtualBinding]->GetSamplerState(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
        }
      }
    }
  }
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_CommandEncoder_Implementation_CommandEncoderImplDiligent);
