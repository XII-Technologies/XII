#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
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

xiiGALCommandEncoderImplDiligent::xiiGALCommandEncoderImplDiligent(xiiGALDeviceDiligent& deviceDiligent) :
  m_GALDeviceDiligent(deviceDiligent), m_pContext(m_GALDeviceDiligent.GetImmediateContext())
{
}

xiiGALCommandEncoderImplDiligent::~xiiGALCommandEncoderImplDiligent()
{
}

void xiiGALCommandEncoderImplDiligent::Reset()
{
  m_bPipelineStateDirty = true;
}

void xiiGALCommandEncoderImplDiligent::MarkDirty()
{
  m_bPipelineStateDirty = true;
}

// State setting functions

void xiiGALCommandEncoderImplDiligent::SetShaderPlatform(const xiiGALShader* pShader)
{
  Diligent::IShader* pVS = nullptr;
  Diligent::IShader* pHS = nullptr;
  Diligent::IShader* pDS = nullptr;
  Diligent::IShader* pGS = nullptr;
  Diligent::IShader* pPS = nullptr;
  Diligent::IShader* pCS = nullptr;

  if (pShader != nullptr)
  {
    xiiGALShaderDiligent* pShaderDiligent = nullptr;
    {
      xiiGALShader* pShaderNonConst = const_cast<xiiGALShader*>(pShader);
      pShaderDiligent               = static_cast<xiiGALShaderDiligent*>(pShaderNonConst);
    }

    pVS = pShaderDiligent->GetVertexShader();
    pHS = pShaderDiligent->GetHullShader();
    pDS = pShaderDiligent->GetDomainShader();
    pGS = pShaderDiligent->GetGeometryShader();
    pPS = pShaderDiligent->GetPixelShader();
    pCS = pShaderDiligent->GetComputeShader();
  }

  if (pVS != m_pBoundShaders[xiiGALShaderStage::VertexShader])
  {
    m_PipelineStateDesc.pVS                          = pVS;
    m_pBoundShaders[xiiGALShaderStage::VertexShader] = pVS;
    m_bPipelineStateDirty                            = true;
  }

  if (pHS != m_pBoundShaders[xiiGALShaderStage::HullShader])
  {
    m_PipelineStateDesc.pHS                        = pHS;
    m_pBoundShaders[xiiGALShaderStage::HullShader] = pHS;
    m_bPipelineStateDirty                          = true;
  }

  if (pDS != m_pBoundShaders[xiiGALShaderStage::DomainShader])
  {
    m_PipelineStateDesc.pDS                          = pDS;
    m_pBoundShaders[xiiGALShaderStage::DomainShader] = pDS;
    m_bPipelineStateDirty                            = true;
  }

  if (pGS != m_pBoundShaders[xiiGALShaderStage::GeometryShader])
  {
    m_PipelineStateDesc.pGS                            = pGS;
    m_pBoundShaders[xiiGALShaderStage::GeometryShader] = pGS;
    m_bPipelineStateDirty                              = true;
  }

  if (pPS != m_pBoundShaders[xiiGALShaderStage::PixelShader])
  {
    m_PipelineStateDesc.pPS                         = pPS;
    m_pBoundShaders[xiiGALShaderStage::PixelShader] = pPS;
    m_bPipelineStateDirty                           = true;
  }

  if (pCS != m_pBoundShaders[xiiGALShaderStage::ComputeShader])
  {
    // TODO Create Compute Graphics Pipeline
    m_pBoundShaders[xiiGALShaderStage::ComputeShader] = pCS;
    m_bPipelineStateDirty                             = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  xiiGALBuffer* pBufferNonConst   = const_cast<xiiGALBuffer*>(pBuffer);
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pBufferNonConst)->GetBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALSamplerState* pSampler = const_cast<xiiGALSamplerState*>(pSamplerState);
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<xiiGALSamplerStateDiligent*>(pSampler)->GetSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDiligent::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  xiiGALResourceView* pResource = const_cast<xiiGALResourceView*>(pResourceView);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<xiiGALResourceViewDiligent*>(pResource)->GetResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDiligent::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessView* pUAView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);

  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView)->GetResourceView() : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
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
  // TODO
}

// Resource update functions

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues)
{
  xiiGALUnorderedAccessView*         pUAView                      = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessViewDiligent = static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView);
  m_pContext->ClearRenderTarget(static_cast<Diligent::ITextureView*>(pUnorderedAccessViewDiligent->GetResourceView().RawPtr()), &clearValues.x, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  xiiGALUnorderedAccessView*         pUAView                      = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);
  xiiGALUnorderedAccessViewDiligent* pUnorderedAccessViewDiligent = static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView);
  XII_ASSERT_NOT_IMPLEMENTED
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

  xiiGALBuffer*      pDst               = const_cast<xiiGALBuffer*>(pDestination);
  Diligent::IBuffer* pDestinationBuffer = static_cast<xiiGALBufferDiligent*>(pDst)->GetBuffer();

  if (pDestination->GetDescription().m_BufferType == xiiGALBufferType::ConstantBuffer)
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && pSourceData.GetCount() == pDestination->GetSize(),
                   "Constant buffers can't be updated partially (and we don't check for Diligent.1)!");

    Diligent::PVoid MapResult;
    m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_NO_OVERWRITE, MapResult);
    memcpy(MapResult, pSourceData.GetPtr(), pSourceData.GetCount());

    m_pContext->UnmapBuffer(pDestinationBuffer, Diligent::MAP_WRITE);
  }
  else
  {
    if (updateMode == xiiGALUpdateMode::CopyToTempStorage)
    {
      if (Diligent::IBuffer* pTempBuffer = m_GALDeviceDiligent.FindTempBuffer(pSourceData.GetCount()))
      {
        Diligent::PVoid MapResult;
        m_pContext->MapBuffer(pTempBuffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_NONE, MapResult);

        memcpy(MapResult, pSourceData.GetPtr(), pSourceData.GetCount());

        m_pContext->UnmapBuffer(pTempBuffer, Diligent::MAP_WRITE);

        m_pContext->CopyBuffer(pTempBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBuffer, 0, pSourceData.GetCount(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      }
      else
      {
        XII_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      Diligent::MAP_FLAGS mapType = (updateMode == xiiGALUpdateMode::Discard) ? Diligent::MAP_FLAG_DISCARD : Diligent::MAP_FLAG_NO_OVERWRITE;

      Diligent::PVoid MapResult;
      m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, mapType, MapResult); // TODO Verify
      memcpy(xiiMemoryUtils::AddByteOffset(MapResult, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

      m_pContext->UnmapBuffer(pDestinationBuffer, Diligent::MAP_WRITE);
    }
  }
}

void xiiGALCommandEncoderImplDiligent::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  Diligent::CopyTextureAttribs CopyTexAttribs;
  CopyTexAttribs.pSrcTexture              = pSourceTexture;
  CopyTexAttribs.pDstTexture              = pDestinationTexture;
  CopyTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  CopyTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

  m_pContext->CopyTexture(CopyTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  Diligent::Box srcBox;
  srcBox.MinX = Box.m_vMin.x;
  srcBox.MinY = Box.m_vMin.y;
  srcBox.MinZ = Box.m_vMin.z;
  srcBox.MaxX = Box.m_vMax.x;
  srcBox.MaxY = Box.m_vMax.y;
  srcBox.MaxZ = Box.m_vMax.z;

  Diligent::CopyTextureAttribs CopyTexAttribs;
  CopyTexAttribs.pSrcTexture = pSourceTexture;
  CopyTexAttribs.pDstTexture = pDestinationTexture;
  CopyTexAttribs.pSrcBox     = &srcBox;

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
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  xiiUInt32                  uiWidth  = xiiMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  xiiUInt32                  uiHeight = xiiMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  xiiUInt32                  uiDepth  = xiiMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  xiiGALResourceFormat::Enum format   = pDestination->GetDescription().m_Format;

  if (Diligent::ITexture* pTempTexture = m_GALDeviceDiligent.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    Diligent::Box SubRegion;
    SubRegion.MinX = DestinationBox.m_vMin.x;
    SubRegion.MinY = DestinationBox.m_vMin.y;
    SubRegion.MinZ = DestinationBox.m_vMin.z;
    SubRegion.MaxX = DestinationBox.m_vMax.x;
    SubRegion.MaxY = DestinationBox.m_vMax.y;
    SubRegion.MaxZ = DestinationBox.m_vMax.z;

    Diligent::MappedTextureSubresource MapResult;
    m_pContext->MapTextureSubresource(pTempTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, &SubRegion, MapResult);

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
      // Copy row by row
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

    m_pContext->UnmapTextureSubresource(pTempTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice);
  }
  else
  {
    XII_REPORT_FAILURE("Could not find a temp texture for update.");
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
    /// \todo Other mip levels etc?

    Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
    // Internally Direct3D swap chain images are not SRGB, and ResolveSubresource
    // requires source and destination formats to match exactly or be typeless.
    // So we will have to create a typeless texture and use SRGB render target view with it.
    ResolveTexAttribs.Format                   = Diligent::TEX_FORMAT_RGBA8_TYPELESS;
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

    Diligent::MappedTextureSubresource Mapped;
    m_pContext->MapTextureSubresource(pTextureDiligent->GetStagingTexture(), subRes.m_uiMipLevel, subRes.m_uiArraySlice, Diligent::MAP_READ, Diligent::MAP_FLAG_NONE, nullptr, Mapped); // TODO: Verify copy entire region
    {
      // TODO: Depth pitch
      if (Mapped.Stride == memDesc.m_uiRowPitch)
      {
        const xiiUInt32 uiMemorySize = xiiGALResourceFormat::GetBitsPerElement(pTextureDiligent->GetDescription().m_Format) *
          GetMipSize(pTextureDiligent->GetDescription().m_uiWidth, subRes.m_uiMipLevel) *
          GetMipSize(pTextureDiligent->GetDescription().m_uiHeight, subRes.m_uiMipLevel) / 8;
        memcpy(memDesc.m_pData, Mapped.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const xiiUInt32 uiHeight = GetMipSize(pTextureDiligent->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = xiiMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.Stride);
          void*       pDest   = xiiMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(
            pDest, pSource, xiiGALResourceFormat::GetBitsPerElement(pTextureDiligent->GetDescription().m_Format) * GetMipSize(pTextureDiligent->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
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

  m_pContext->GenerateMips(static_cast<Diligent::ITextureView*>(pResourceViewDiligent->GetResourceView().RawPtr()));
}

void xiiGALCommandEncoderImplDiligent::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void xiiGALCommandEncoderImplDiligent::PushMarkerPlatform(const char* szMarker)
{
  m_pContext->BeginDebugGroup(szMarker);
}

void xiiGALCommandEncoderImplDiligent::PopMarkerPlatform()
{
  m_pContext->EndDebugGroup();
}

void xiiGALCommandEncoderImplDiligent::InsertEventMarkerPlatform(const char* szMarker)
{
  m_pContext->InsertDebugLabel(szMarker);
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::BeginRendering(const xiiGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    xiiGALRenderTargetView* pRenderTargetViews[XII_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    xiiGALRenderTargetView* pDepthStencilView                                  = nullptr;

    const xiiUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    bool bFlushNeeded = false;

    for (xiiUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      xiiGALRenderTargetView* pRenderTargetView = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex)));
      if (pRenderTargetView != nullptr)
      {
        const xiiGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
        bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    pDepthStencilView = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget()));
    if (pDepthStencilView != nullptr)
    {
      const xiiGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
      bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushPlatform();
    }

    for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (xiiUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetViews[i])->GetRenderTargetView().RawPtr();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<xiiGALRenderTargetViewDiligent*>(pDepthStencilView)->GetDepthStencilView().RawPtr();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pContext->SetRenderTargets(xiiMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void xiiGALCommandEncoderImplDiligent::BeginCompute()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = xiiGALRenderTargetSetup();
  m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  m_bPipelineStateDirty = true;
}

// Draw functions

void xiiGALCommandEncoderImplDiligent::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pContext->ClearRenderTarget(m_pBoundRenderTargets[i], ClearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
  }

  if (bClearDepth && m_pBoundDepthStencilTarget)
  {
    m_pContext->ClearDepthStencil(m_pBoundDepthStencilTarget, bClearDepth ? Diligent::CLEAR_DEPTH_FLAG : Diligent::CLEAR_DEPTH_FLAG_NONE, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }

  if (bClearStencil && m_pBoundDepthStencilTarget)
  {
    m_pContext->ClearDepthStencil(m_pBoundDepthStencilTarget, bClearDepth ? Diligent::CLEAR_STENCIL_FLAG : Diligent::CLEAR_DEPTH_FLAG_NONE, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
}

void xiiGALCommandEncoderImplDiligent::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCount;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_NONE;
  drawAttribs.NumInstances          = 1;
  drawAttribs.FirstInstanceLocation = 0;
  drawAttribs.StartVertexLocation   = uiStartVertex;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCount;
  drawAttribs.IndexType             = Diligent::VT_UINT32;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_NONE;
  drawAttribs.NumInstances          = 1;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0;
  drawAttribs.FirstInstanceLocation = 0;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCountPerInstance;
  drawAttribs.IndexType             = Diligent::VT_UINT32;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_NONE;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.FirstIndexLocation    = uiStartIndex;
  drawAttribs.BaseVertex            = 0;
  drawAttribs.FirstInstanceLocation = 0;

  m_pContext->DrawIndexed(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndexedIndirectAttribs drawAttribs;
  drawAttribs.IndexType                        = Diligent::VT_UINT32;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_NONE;
  drawAttribs.DrawCount                        = 1;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 5;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE;

  m_pContext->DrawIndexedIndirect(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  Diligent::DrawAttribs drawAttribs;
  drawAttribs.NumVertices           = uiVertexCountPerInstance;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_NONE;
  drawAttribs.NumInstances          = uiInstanceCount;
  drawAttribs.FirstInstanceLocation = 0;
  drawAttribs.StartVertexLocation   = uiStartVertex;

  m_pContext->Draw(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DrawIndirectAttribs drawAttribs;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_NONE;
  drawAttribs.DrawCount                        = 1;
  drawAttribs.DrawArgsStride                   = sizeof(xiiUInt32) * 4;
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE;

  m_pContext->DrawIndirect(drawAttribs);
}

void xiiGALCommandEncoderImplDiligent::DrawAutoPlatform()
{
  // FlushDeferredStateChanges();

  XII_ASSERT_NOT_IMPLEMENTED
}

void xiiGALCommandEncoderImplDiligent::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::EndStreamOutPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
  xiiGALBuffer* pIBuffer = const_cast<xiiGALBuffer*>(pIndexBuffer);
  if (pIndexBuffer != nullptr)
  {
    xiiGALBufferDiligent* pDiligentBuffer = static_cast<xiiGALBufferDiligent*>(pIBuffer);
    m_pContext->SetIndexBuffer(pDiligentBuffer->GetBuffer(), 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
  else
  {
    m_pContext->SetIndexBuffer(nullptr, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
}

void xiiGALCommandEncoderImplDiligent::SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)
{
  XII_ASSERT_DEV(uiSlot < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  xiiGALBuffer* pVBuffer = const_cast<xiiGALBuffer*>(pVertexBuffer);

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pVBuffer)->GetBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDiligent::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  const Diligent::InputLayoutDesc* desc            = static_cast<const xiiGALVertexDeclarationDiligent*>(pVertexDeclaration)->GetInputLayoutDesc();
  m_PipelineStateDesc.GraphicsPipeline.InputLayout = *desc;
  m_bPipelineStateDirty                            = true;
}

static const Diligent::PRIMITIVE_TOPOLOGY GALTopologyToDiligent[xiiGALPrimitiveTopology::ENUM_COUNT] = {
  Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

void xiiGALCommandEncoderImplDiligent::SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology)
{
  if (m_PipelineStateDesc.GraphicsPipeline.PrimitiveTopology != GALTopologyToDiligent[Topology])
  {
    m_PipelineStateDesc.GraphicsPipeline.PrimitiveTopology = GALTopologyToDiligent[Topology];
    m_bPipelineStateDirty                                  = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  const float BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pContext->SetBlendFactors(BlendFactors);

  const Diligent::BlendStateDesc* desc = static_cast<const xiiGALBlendStateDiligent*>(pBlendState)->GetBlendStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.BlendDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.BlendDesc = *desc;
    m_bPipelineStateDirty                          = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  const Diligent::DepthStencilStateDesc* desc = static_cast<const xiiGALDepthStencilStateDiligent*>(pDepthStencilState)->GetDepthStencilStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc = *desc;
    m_bPipelineStateDirty                                 = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  const Diligent::RasterizerStateDesc* desc = static_cast<const xiiGALRasterizerStateDiligent*>(pRasterizerState)->GetRasterizerStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc = *desc;
    m_bPipelineStateDirty                               = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  Diligent::Viewport Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width    = rect.width;
  Viewport.Height   = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pContext->SetViewports(1, &Viewport, static_cast<Diligent::Uint32>(rect.width), static_cast<Diligent::Uint32>(rect.height));
}

void xiiGALCommandEncoderImplDiligent::SetScissorRectPlatform(const xiiRectU32& rect)
{
  Diligent::Rect ScissorRect;
  ScissorRect.left   = rect.x;
  ScissorRect.top    = rect.y;
  ScissorRect.right  = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pContext->SetScissorRects(1, &ScissorRect, rect.width, rect.height);
}

void xiiGALCommandEncoderImplDiligent::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

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
  FlushDeferredStateChanges();

  xiiGALBuffer* pIABuffer = const_cast<xiiGALBuffer*>(pIndirectArgumentBuffer);

  Diligent::DispatchComputeIndirectAttribs DispatchAttribs;
  DispatchAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE;
  DispatchAttribs.DispatchArgsByteOffset           = uiArgumentOffsetInBytes;

  // These are only needed in a Metal backend.
  DispatchAttribs.MtlThreadGroupSizeX = 0;
  DispatchAttribs.MtlThreadGroupSizeY = 0;
  DispatchAttribs.MtlThreadGroupSizeZ = 0;

  m_pContext->DispatchComputeIndirect(DispatchAttribs);
}

//////////////////////////////////////////////////////////////////////////

// Some state changes are deferred so they can be updated faster
void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChanges()
{
  if (m_bPipelineStateDirty)
  {
    m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateDesc, &m_pPipelineState);
    m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

    if (m_BoundVertexBuffersRange.IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
      const xiiUInt32 uiNumSlots  = m_BoundVertexBuffersRange.GetCount();

      m_pContext->SetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferOffsets + uiStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_NONE);

      xiiUInt32 uiCurrentStartSlot = uiStartSlot;

      // Finding valid ranges.
      for (xiiUInt32 i = uiStartSlot; i < (uiStartSlot + uiNumSlots); i++)
      {
        if (!m_pBoundVertexBuffers[i])
        {
          if (i - uiCurrentStartSlot > 0)
          {
            // There are some null elements in the array. We can't submit these to Diligent and need to skip them so flush everything before it.
            m_pContext->SetVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_NONE);
          }
          uiCurrentStartSlot = i + 1;
        }
      }

      // The last element in the buffer range must always be valid so we can simply flush the rest.
      if (m_pBoundVertexBuffers[uiCurrentStartSlot])
      {
        m_pContext->SetVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_NONE);
      }

      m_BoundVertexBuffersRange.Reset();
    }

    m_pContext->CommitShaderResources(m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_pContext->SetPipelineState(m_pPipelineState);
  }
}
