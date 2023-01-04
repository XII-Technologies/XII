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

// State setting functions

void xiiGALCommandEncoderImplDiligent::SetShaderPlatform(const xiiGALShader* pShader)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  XII_ASSERT_NOT_IMPLEMENTED;
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
  XII_ASSERT_NOT_IMPLEMENTED;
}

// Resource update functions

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  XII_ASSERT_NOT_IMPLEMENTED;
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
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::BeginCompute()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

// Draw functions

void xiiGALCommandEncoderImplDiligent::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  XII_ASSERT_NOT_IMPLEMENTED;
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
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  XII_ASSERT_NOT_IMPLEMENTED;
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

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChanges()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}
