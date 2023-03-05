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
  m_bPipelineStateModified = true;
  // m_PipelineStateDesc        = Diligent::GraphicsPipelineStateCreateInfo();
  // m_PipelineStateComputeDesc = Diligent::ComputePipelineStateCreateInfo();

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
    m_pCurrentShader = pShaderDiligent;

    pVS = pShaderDiligent->GetVertexShader();
    pHS = pShaderDiligent->GetHullShader();
    pDS = pShaderDiligent->GetDomainShader();
    pGS = pShaderDiligent->GetGeometryShader();
    pPS = pShaderDiligent->GetPixelShader();
    pCS = pShaderDiligent->GetComputeShader();
  }

  if (pVS != m_pBoundShaders[xiiGALShaderStage::VertexShader])
  {
    m_PipelineStateDesc.pVS  = pVS;
    m_bPipelineStateModified = true;
  }
  m_pBoundShaders[xiiGALShaderStage::VertexShader] = pVS;

  if (pHS != m_pBoundShaders[xiiGALShaderStage::HullShader])
  {
    m_PipelineStateDesc.pHS  = pHS;
    m_bPipelineStateModified = true;
  }
  m_pBoundShaders[xiiGALShaderStage::HullShader] = pHS;

  if (pDS != m_pBoundShaders[xiiGALShaderStage::DomainShader])
  {
    m_PipelineStateDesc.pDS  = pDS;
    m_bPipelineStateModified = true;
  }
  m_pBoundShaders[xiiGALShaderStage::DomainShader] = pDS;

  if (pGS != m_pBoundShaders[xiiGALShaderStage::GeometryShader])
  {
    m_PipelineStateDesc.pGS  = pGS;
    m_bPipelineStateModified = true;
  }
  m_pBoundShaders[xiiGALShaderStage::GeometryShader] = pGS;

  if (pPS != m_pBoundShaders[xiiGALShaderStage::PixelShader])
  {
    m_PipelineStateDesc.pPS  = pPS;
    m_bPipelineStateModified = true;
  }
  m_pBoundShaders[xiiGALShaderStage::PixelShader] = pPS;

  if (pCS != m_pBoundShaders[xiiGALShaderStage::ComputeShader])
  {
    m_PipelineStateComputeDesc.pCS = pCS;
    m_bPipelineStateModified       = true;
  }
  m_pBoundShaders[xiiGALShaderStage::ComputeShader] = pCS;
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  xiiGALBuffer*         pBufferNonConst = const_cast<xiiGALBuffer*>(pBuffer);
  xiiGALBufferDiligent* pBufferDiligent = static_cast<xiiGALBufferDiligent*>(pBufferNonConst);
  m_pBoundConstantBuffers[uiSlot]       = pBuffer != nullptr ? pBufferDiligent : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALSamplerState* pSampler = const_cast<xiiGALSamplerState*>(pSamplerState);
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<xiiGALSamplerStateDiligent*>(pSampler)->GetSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
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
  // This looks to require custom code, either using buffer copies or clearing via a compute shader

  // XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligent::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  // This looks to require custom code, either using buffer copies or clearing via a compute shader

  // XII_ASSERT_NOT_IMPLEMENTED;
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

#if 0
  if (pDestination->GetDescription().m_BufferType == xiiGALBufferType::ConstantBuffer)
  {
    Diligent::PVoid MapResult;

    m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, MapResult);

    if (MapResult)
    {
      memcpy(MapResult, pSourceData.GetPtr(), pSourceData.GetCount());

      m_pContext->UnmapBuffer(pDestinationBuffer, Diligent::MAP_WRITE);
    }
  }
  else
  {
    const Diligent::BufferDesc& desc = pDestinationBuffer->GetDesc();
    if (desc.Usage == Diligent::USAGE_DEFAULT || desc.Usage == Diligent::USAGE_SPARSE)
    {
      m_pContext->UpdateBuffer(pDestinationBuffer, uiDestOffset, pSourceData.GetCount(), reinterpret_cast<const void*>(pSourceData.GetPtr()), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
#else
  switch (updateMode)
  {
    case xiiGALUpdateMode::Discard:
    {
      Diligent::PVoid pMapResult;

      m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, pMapResult);

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

    case xiiGALUpdateMode::NoOverwrite:
    {
      Diligent::PVoid pMapResult;

      m_pContext->MapBuffer(pDestinationBuffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_NO_OVERWRITE, pMapResult);

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

    case xiiGALUpdateMode::CopyToTempStorage:
    {
      XII_ASSERT_NOT_IMPLEMENTED
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }
#endif
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
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  xiiUInt32                  uiWidth  = xiiMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  xiiUInt32                  uiHeight = xiiMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  xiiUInt32                  uiDepth  = xiiMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  xiiGALResourceFormat::Enum format   = pDestination->GetDescription().m_Format;

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
      const xiiGALRenderTargetView* pRenderTargetView = m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        const xiiGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
        bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = const_cast<xiiGALRenderTargetView*>(pRenderTargetView);
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
          m_pBoundRenderTargets[i] = static_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<xiiGALRenderTargetViewDiligent*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pContext->SetRenderTargets(xiiMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;

      for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; ++i)
      {
        // Set render target format which is the format of the swap chain's color buffer
        m_PipelineStateDesc.GraphicsPipeline.RTVFormats[i] = m_pBoundRenderTargets[i]->GetDesc().Format;
      }

      // Set depth buffer format which is the format of the current bound depth stencil buffer
      if (m_pBoundDepthStencilTarget != nullptr)
        m_PipelineStateDesc.GraphicsPipeline.DSVFormat = m_pBoundDepthStencilTarget->GetDesc().Format;

      // Set the number of render targets
      m_PipelineStateDesc.GraphicsPipeline.NumRenderTargets = m_uiBoundRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
      m_uiBoundRenderTargetCount = 0;

      m_PipelineStateDesc.GraphicsPipeline.NumRenderTargets = m_uiBoundRenderTargetCount;
    }
  }
  else
  {
    // Set the amount of render targets. This must be rebound every render call.
    m_pContext->SetRenderTargets(m_uiBoundRenderTargetCount, m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void xiiGALCommandEncoderImplDiligent::EndRendering()
{
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
}

void xiiGALCommandEncoderImplDiligent::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

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
  FlushDeferredStateChanges();

  Diligent::DrawIndexedAttribs drawAttribs;
  drawAttribs.NumIndices            = uiIndexCount;
  drawAttribs.IndexType             = m_IndexBufferFormat;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
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
  drawAttribs.IndexType             = m_IndexBufferFormat;
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
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
  drawAttribs.IndexType                        = m_IndexBufferFormat;
  drawAttribs.pAttribsBuffer                   = static_cast<xiiGALBufferDiligent*>(pIABuffer)->GetBuffer();
  drawAttribs.DrawArgsOffset                   = uiArgumentOffsetInBytes;
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
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
  drawAttribs.Flags                 = Diligent::DRAW_FLAG_VERIFY_ALL;
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
  drawAttribs.Flags                            = Diligent::DRAW_FLAG_VERIFY_ALL;
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
}

void xiiGALCommandEncoderImplDiligent::EndStreamOutPlatform()
{
}

void xiiGALCommandEncoderImplDiligent::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
  xiiGALBuffer*         pIndexBufferNonConst = const_cast<xiiGALBuffer*>(pIndexBuffer);
  xiiGALBufferDiligent* pDiligentBuffer      = static_cast<xiiGALBufferDiligent*>(pIndexBufferNonConst);

  if (m_pIndexBuffer != pDiligentBuffer->GetBuffer())
  {
    m_pIndexBuffer         = pDiligentBuffer->GetBuffer();
    m_IndexBufferFormat    = pDiligentBuffer->GetIndexFormat();
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
  if (pVertexDeclaration == nullptr)
    return;

  const Diligent::InputLayoutDesc desc = *static_cast<const xiiGALVertexDeclarationDiligent*>(pVertexDeclaration)->GetInputLayoutDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.InputLayout != desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.InputLayout = desc;
    m_bPipelineStateModified                         = true;
  }
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
    m_bPipelineStateModified                               = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  const float BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pContext->SetBlendFactors(BlendFactors);

  const Diligent::BlendStateDesc desc = *static_cast<const xiiGALBlendStateDiligent*>(pBlendState)->GetBlendStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.BlendDesc != desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.BlendDesc = desc;
    m_bPipelineStateModified                       = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  const Diligent::DepthStencilStateDesc desc = *static_cast<const xiiGALDepthStencilStateDiligent*>(pDepthStencilState)->GetDepthStencilStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc != desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc = desc;
    m_bPipelineStateModified                              = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  const Diligent::RasterizerStateDesc desc = *static_cast<const xiiGALRasterizerStateDiligent*>(pRasterizerState)->GetRasterizerStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc != desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc = desc;

    if (m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc.ScissorEnable != m_bScissorEnabled)
    {
      m_bScissorEnabled   = m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc.ScissorEnable;
      m_bViewportModified = true;
    }
    m_bPipelineStateModified = true;
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
  m_bPipelineStateModified    = true;

  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = xiiGALRenderTargetSetup();
  m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::EndCompute()
{
  m_bComputePipelineRequested = false;
}

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

void xiiGALCommandEncoderImplDiligent::MarkDirty()
{
  // m_bPipelineStateModified = true;
  m_bViewportModified    = true;
  m_bIndexBufferModified = true;
  m_bDescriptorsModified = true;

  m_BoundVertexBuffersRange.Reset();
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }
}

void xiiGALCommandEncoderImplDiligent::Reset()
{
  // m_bPipelineStateModified = true;
  m_bViewportModified    = true;
  m_bIndexBufferModified = true;
  m_bDescriptorsModified = true;

  // m_PipelineStateDesc        = Diligent::GraphicsPipelineStateCreateInfo();
  m_PipelineStateComputeDesc = Diligent::ComputePipelineStateCreateInfo();
#if 0
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
#endif

  m_BoundVertexBuffersRange.Reset();

  m_Viewport    = Diligent::Viewport();
  m_ScissorRect = Diligent::Rect();

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0;

  // m_pIndexBuffer = nullptr;
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

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChanges()
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingGraphics);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderResourceBindingCompute);

  if (m_bPipelineStateModified)
  {
    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateGraphics);
    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pPipelineStateCompute);

    if (m_bComputePipelineRequested)
    {
      // Copy attributes set in the graphics pipeline state
      m_PipelineStateComputeDesc.PSODesc = m_PipelineStateDesc.PSODesc;

      // Pipeline state name is used by the engine to report issues.
      // It is always a good idea to give objects descriptive names.
      m_PipelineStateComputeDesc.PSODesc.Name = "Compute Pipeline State";

      // This is a graphics pipeline
      m_PipelineStateComputeDesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_COMPUTE;

      // Define variable type that will be used by default
      m_PipelineStateComputeDesc.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

      m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateComputeDesc, &m_pPipelineStateCompute);

      XII_ASSERT_DEV(m_pPipelineStateCompute != nullptr, "Failed to create compute pipeline state.");
    }
    else
    {
      m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateDesc, &m_pPipelineStateGraphics);

      XII_ASSERT_DEV(m_pPipelineStateGraphics != nullptr, "Failed to create graphics pipeline state.");
    }

    // Do not set m_bPipelineStateModified to false here, some updates are deferred to the end of the function.

    // Changes to the descriptor layout always require the descriptor set to be re-created.
    m_bDescriptorsModified = true;
  }

  if (!m_bComputePipelineRequested && m_bViewportModified)
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

  if (!m_bComputePipelineRequested && m_BoundVertexBuffersRange.IsValid())
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
          m_pContext->SetVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        }
        uiCurrentStartSlot = i + 1;
      }
    }

    // The last element in the buffer range must always be valid so we can simply flush the rest.
    if (m_pBoundVertexBuffers[uiCurrentStartSlot])
      m_pContext->SetVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);

    m_BoundVertexBuffersRange.Reset();
  }

  if (!m_bComputePipelineRequested && m_bIndexBufferModified)
  {
    m_pContext->SetIndexBuffer(m_pIndexBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_bIndexBufferModified = false;
  }

  if (m_bDescriptorsModified)
  {
    Diligent::IPipelineState* pPipelineState = nullptr;

    if (m_bComputePipelineRequested)
      pPipelineState = m_pPipelineStateCompute;
    else
      pPipelineState = m_pPipelineStateGraphics;

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
              pSampler->Set(m_pBoundSamplerStates[stage][currentBinding.m_uiVirtualBinding], Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            }
            break;
          }
        }
      }
    }

    m_bDescriptorsModified = false;
  }

  if (m_bComputePipelineRequested)
  {
    // Create a shader resource binding object and bind all static resources in it
    m_pPipelineStateCompute->CreateShaderResourceBinding(&m_pShaderResourceBindingCompute, true);

    if (m_bPipelineStateModified)
    {
      m_bPipelineStateModified = false;
    }

    m_pContext->SetPipelineState(m_pPipelineStateCompute);

    m_pContext->CommitShaderResources(m_pShaderResourceBindingCompute, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
  else
  {
    // Create a shader resource binding object and bind all static resources in it
    m_pPipelineStateGraphics->CreateShaderResourceBinding(&m_pShaderResourceBindingGraphics, true);

    if (m_bPipelineStateModified)
    {
      m_bPipelineStateModified = false;
    }

    m_pContext->SetPipelineState(m_pPipelineStateGraphics);

    m_pContext->CommitShaderResources(m_pShaderResourceBindingGraphics, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
}
