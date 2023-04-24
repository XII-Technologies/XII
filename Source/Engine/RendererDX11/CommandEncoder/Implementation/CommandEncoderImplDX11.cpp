#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

xiiGALCommandEncoderImplDX11::xiiGALCommandEncoderImplDX11(xiiGALDeviceDX11& deviceDX11) :
  m_GALDeviceDX11(deviceDX11)
{
  m_pDXContext = m_GALDeviceDX11.GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    xiiLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

xiiGALCommandEncoderImplDX11::~xiiGALCommandEncoderImplDX11()
{
  XII_GAL_DX11_RELEASE(m_pDXAnnotation);
}

// State setting functions

void xiiGALCommandEncoderImplDX11::SetShaderPlatform(const xiiGALShader* pShader)
{
  ID3D11VertexShader*   pVS = nullptr;
  ID3D11HullShader*     pHS = nullptr;
  ID3D11DomainShader*   pDS = nullptr;
  ID3D11GeometryShader* pGS = nullptr;
  ID3D11PixelShader*    pPS = nullptr;
  ID3D11ComputeShader*  pCS = nullptr;

  if (pShader != nullptr)
  {
    const xiiGALShaderDX11* pDXShader = static_cast<const xiiGALShaderDX11*>(pShader);

    pVS = pDXShader->GetDXVertexShader();
    pHS = pDXShader->GetDXHullShader();
    pDS = pDXShader->GetDXDomainShader();
    pGS = pDXShader->GetDXGeometryShader();
    pPS = pDXShader->GetDXPixelShader();
    pCS = pDXShader->GetDXComputeShader();
  }

  if (pVS != m_pBoundShaders[xiiGALShaderStage::VertexShader])
  {
    m_pDXContext->VSSetShader(pVS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::VertexShader] = pVS;
  }

  if (pHS != m_pBoundShaders[xiiGALShaderStage::HullShader])
  {
    m_pDXContext->HSSetShader(pHS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::HullShader] = pHS;
  }

  if (pDS != m_pBoundShaders[xiiGALShaderStage::DomainShader])
  {
    m_pDXContext->DSSetShader(pDS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::DomainShader] = pDS;
  }

  if (pGS != m_pBoundShaders[xiiGALShaderStage::GeometryShader])
  {
    m_pDXContext->GSSetShader(pGS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::GeometryShader] = pGS;
  }

  if (pPS != m_pBoundShaders[xiiGALShaderStage::PixelShader])
  {
    m_pDXContext->PSSetShader(pPS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::PixelShader] = pPS;
  }

  if (pCS != m_pBoundShaders[xiiGALShaderStage::ComputeShader])
  {
    m_pDXContext->CSSetShader(pCS, nullptr, 0);
    m_pBoundShaders[xiiGALShaderStage::ComputeShader] = pCS;
  }
}

void xiiGALCommandEncoderImplDX11::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const xiiGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDX11::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const xiiGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDX11::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const xiiGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  m_BoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_BoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const xiiGALUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  m_BoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

// Query functions

void xiiGALCommandEncoderImplDX11::BeginQueryPlatform(const xiiGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const xiiGALQueryDX11*>(pQuery)->GetDXQuery());
}

void xiiGALCommandEncoderImplDX11::EndQueryPlatform(const xiiGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const xiiGALQueryDX11*>(pQuery)->GetDXQuery());
}

xiiResult xiiGALCommandEncoderImplDX11::GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& uiQueryResult)
{
  return m_pDXContext->GetData(
           static_cast<const xiiGALQueryDX11*>(pQuery)->GetDXQuery(), &uiQueryResult, sizeof(xiiUInt64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE ?
    XII_FAILURE :
    XII_SUCCESS;
}

// Timestamp functions

void xiiGALCommandEncoderImplDX11::InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = m_GALDeviceDX11.GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

void xiiGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues)
{
  const xiiGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const xiiGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

void xiiGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  const xiiGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const xiiGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &clearValues.x);
}

void xiiGALCommandEncoderImplDX11::CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const xiiGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource      = static_cast<const xiiGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void xiiGALCommandEncoderImplDX11::CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const xiiGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource      = static_cast<const xiiGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void xiiGALCommandEncoderImplDX11::UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiGALUpdateMode::Enum updateMode)
{
  XII_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  ID3D11Buffer* pDXDestination = static_cast<const xiiGALBufferDX11*>(pDestination)->GetDXBuffer();

  if (pDestination->GetDescription().m_BufferType == xiiGALBufferType::ConstantBuffer)
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && pSourceData.GetCount() == pDestination->GetSize(),
                   "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    if (updateMode == xiiGALUpdateMode::CopyToTempStorage)
    {
      if (ID3D11Resource* pDXTempBuffer = m_GALDeviceDX11.FindTempBuffer(pSourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT                  hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        XII_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

        memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, pSourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        XII_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = (updateMode == xiiGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(xiiMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
      else
      {
        xiiLog::Error("Could not map buffer to update content.");
      }
    }
  }
}

void xiiGALCommandEncoderImplDX11::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const xiiGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource      = static_cast<const xiiGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void xiiGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  ID3D11Resource* pDXDestination = static_cast<const xiiGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource      = static_cast<const xiiGALTextureDX11*>(pSource)->GetDXTexture();

  xiiUInt32 dstSubResource = D3D11CalcSubresource(
    DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  xiiUInt32 srcSubResource =
    D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {Box.m_vMin.x, Box.m_vMin.y, Box.m_vMin.z, Box.m_vMax.x, Box.m_vMax.y, Box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, DestinationPoint.x, DestinationPoint.y, DestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void xiiGALCommandEncoderImplDX11::UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const xiiGALTextureDX11*>(pDestination)->GetDXTexture();

  xiiUInt32                  uiWidth  = xiiMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  xiiUInt32                  uiHeight = xiiMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  xiiUInt32                  uiDepth  = xiiMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  xiiGALResourceFormat::Enum format   = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = m_GALDeviceDX11.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT                  hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    XII_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

    xiiUInt32 uiRowPitch   = uiWidth * xiiGALResourceFormat::GetBitsPerElement(format) / 8;
    xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    XII_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    XII_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
                   uiSlicePitch, pSourceData.m_uiSlicePitch);

    if (MapResult.RowPitch == uiRowPitch && MapResult.DepthPitch == uiSlicePitch)
    {
      memcpy(MapResult.pData, pSourceData.m_pData, uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (xiiUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = xiiMemoryUtils::AddByteOffset(pSourceData.m_pData, z * uiSlicePitch);
        void*       pDest   = xiiMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthPitch);

        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, uiRowPitch);

          pSource = xiiMemoryUtils::AddByteOffset(pSource, uiRowPitch);
          pDest   = xiiMemoryUtils::AddByteOffset(pDest, MapResult.RowPitch);
        }
      }
    }

    m_pDXContext->Unmap(pDXTempTexture, 0);

    xiiUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    XII_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void xiiGALCommandEncoderImplDX11::ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const xiiGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource      = static_cast<const xiiGALTextureDX11*>(pSource)->GetDXTexture();

  xiiUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  xiiUInt32 srcSubResource = D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void xiiGALCommandEncoderImplDX11::ReadbackTexturePlatform(const xiiGALTexture* pTexture)
{
  const xiiGALTextureDX11* pDXTexture = static_cast<const xiiGALTextureDX11*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != xiiGALMSAASampleCount::None;

  XII_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  XII_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
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

void xiiGALCommandEncoderImplDX11::CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData)
{
  const xiiGALTextureDX11* pDXTexture = static_cast<const xiiGALTextureDX11*>(pTexture);

  XII_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  XII_ASSERT_DEV(SourceSubResource.GetCount() == TargetData.GetCount(), "Source and target arrays must be of the same size.");

  const xiiUInt32 uiSubResources = SourceSubResource.GetCount();
  for (xiiUInt32 i = 0; i < uiSubResources; i++)
  {
    const xiiGALTextureSubresource&      subRes             = SourceSubResource[i];
    const xiiGALSystemMemoryDescription& memDesc            = TargetData[i];
    const xiiUInt32                      uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    D3D11_MAPPED_SUBRESOURCE Mapped;
    if (SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex, D3D11_MAP_READ, 0, &Mapped)))
    {
      // TODO: Depth pitch
      if (Mapped.RowPitch == memDesc.m_uiRowPitch)
      {
        const xiiUInt32 uiMemorySize = xiiGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) *
          GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) *
          GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel) / 8;
        memcpy(memDesc.m_pData, Mapped.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const xiiUInt32 uiHeight = GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = xiiMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
          void*       pDest   = xiiMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(
            pDest, pSource, xiiGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }

      m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex);
    }
  }
}

void xiiGALCommandEncoderImplDX11::GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView)
{
  const xiiGALResourceViewDX11* pDXResourceView = static_cast<const xiiGALResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void xiiGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void xiiGALCommandEncoderImplDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    xiiStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void xiiGALCommandEncoderImplDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void xiiGALCommandEncoderImplDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    xiiStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDX11::BeginRendering(const xiiGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const xiiGALRenderTargetView* pRenderTargetViews[XII_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const xiiGALRenderTargetView* pDepthStencilView                                  = nullptr;

    const xiiUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    bool bFlushNeeded = false;

    for (xiiUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const xiiGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        const xiiGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
        bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
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
          m_pBoundRenderTargets[i] = static_cast<const xiiGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const xiiGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(xiiMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void xiiGALCommandEncoderImplDX11::BeginCompute()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = xiiGALRenderTargetSetup();
  m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
}

// Draw functions

void xiiGALCommandEncoderImplDX11::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], ClearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    xiiUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

void xiiGALCommandEncoderImplDX11::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void xiiGALCommandEncoderImplDX11::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    if (m_GALDeviceDX11.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_GALDeviceDX11.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError      = FALSE;
        static BOOL bBreakOnWarning    = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }

    m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
}

void xiiGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void xiiGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const xiiGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void xiiGALCommandEncoderImplDX11::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void xiiGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<const xiiGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void xiiGALCommandEncoderImplDX11::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawAuto();
}

void xiiGALCommandEncoderImplDX11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDX11::EndStreamOutPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDX11::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const xiiGALBufferDX11* pDX11Buffer = static_cast<const xiiGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void xiiGALCommandEncoderImplDX11::SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)
{
  XII_ASSERT_DEV(uiSlot < XII_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const xiiGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void xiiGALCommandEncoderImplDX11::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(
    pVertexDeclaration != nullptr ? static_cast<const xiiGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[xiiGALPrimitiveTopology::ENUM_COUNT] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
};

void xiiGALCommandEncoderImplDX11::SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[Topology]);
}

void xiiGALCommandEncoderImplDX11::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const xiiGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void xiiGALCommandEncoderImplDX11::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(
    pDepthStencilState != nullptr ? static_cast<const xiiGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr,
    uiStencilRefValue);
}

void xiiGALCommandEncoderImplDX11::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<const xiiGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void xiiGALCommandEncoderImplDX11::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width    = rect.width;
  Viewport.Height   = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void xiiGALCommandEncoderImplDX11::SetScissorRectPlatform(const xiiRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left   = rect.x;
  ScissorRect.top    = rect.y;
  ScissorRect.right  = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void xiiGALCommandEncoderImplDX11::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDX11::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void xiiGALCommandEncoderImplDX11::DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<const xiiGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(xiiGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, xiiUInt32 uiStartSlot, xiiUInt32 uiNumSlots, ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case xiiGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case xiiGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case xiiGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case xiiGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case xiiGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case xiiGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(
  xiiGALShaderStage::Enum stage,
  ID3D11DeviceContext*    pContext,
  xiiUInt32               uiStartSlot,
  xiiUInt32               uiNumSlots,
  ID3D11Buffer**          pConstantBuffers)
{
  switch (stage)
  {
    case xiiGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case xiiGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case xiiGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case xiiGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case xiiGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case xiiGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(
  xiiGALShaderStage::Enum stage,
  ID3D11DeviceContext*    pContext,
  xiiUInt32               uiStartSlot,
  xiiUInt32               uiNumSlots,
  ID3D11SamplerState**    pSamplerStates)
{
  switch (stage)
  {
    case xiiGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case xiiGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case xiiGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case xiiGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case xiiGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case xiiGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
}

// Some state changes are deferred so they can be updated faster
void xiiGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_BoundVertexBuffersRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const xiiUInt32 uiNumSlots  = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const xiiUInt32 uiNumSlots  = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((xiiGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_BoundUnoderedAccessViewsRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_BoundUnoderedAccessViewsRange.m_uiMin;
    const xiiUInt32 uiNumSlots  = m_BoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_BoundUnoderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_BoundUnoderedAccessViewsRange.Reset();
  }

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const xiiUInt32 uiNumSlots  = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((xiiGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const xiiUInt32 uiNumSlots  = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((xiiGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
}
