#include <RendererDiligentD3D11/RendererDiligentD3D11PCH.h>

#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/QueryDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererDiligent/State/StateDiligent.h>
#include <RendererDiligentD3D11/CommandEncoder/CommandEncoderImplDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/DeviceDiligentD3D11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

xiiGALCommandEncoderImplDiligentD3D11::xiiGALCommandEncoderImplDiligentD3D11(xiiGALDeviceDiligentD3D11& deviceDiligent) :
  xiiGALCommandEncoderImplDiligent(deviceDiligent), m_GALDeviceDiligent(deviceDiligent), m_pContext(m_GALDeviceDiligent.GetImmediateContext())
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
    m_PipelineStateDesc.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
  }

  // Compute Pipeline
  {
    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    m_PipelineStateComputeDesc.PSODesc.Name = "Compute Pipeline State";

    // This is a graphics pipeline
    m_PipelineStateComputeDesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_COMPUTE;

    // Define variable type that will be used by default
    m_PipelineStateComputeDesc.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
  }
}

xiiGALCommandEncoderImplDiligentD3D11::~xiiGALCommandEncoderImplDiligentD3D11()
{
}

void xiiGALCommandEncoderImplDiligentD3D11::Reset()
{
  m_bPipelineStateModified   = true;
  m_PipelineStateDesc        = Diligent::GraphicsPipelineStateCreateInfo();
  m_PipelineStateComputeDesc = Diligent::ComputePipelineStateCreateInfo();

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0;

  m_pIndexBuffer = nullptr;
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
  }

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_CONSTANT_BUFFER_COUNT; i++)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; i++)
  {
    m_pBoundShaderResourceViews[i].Clear();
  }
  m_pBoundUnoderedAccessViews.Clear();

  xiiMemoryUtils::ZeroFill(&m_pBoundSamplerStates[0][0], xiiGALShaderStage::ENUM_COUNT * XII_GAL_MAX_SAMPLER_COUNT);
}

void xiiGALCommandEncoderImplDiligentD3D11::MarkDirty()
{
  m_bDescriptorsModified   = true;
  m_bPipelineStateModified = true;

  m_BoundVertexBuffersRange.Reset();
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }
}

// State setting functions

void xiiGALCommandEncoderImplDiligentD3D11::SetShaderPlatform(const xiiGALShader* pShader)
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
    m_bPipelineStateModified                         = true;
  }

  if (pHS != m_pBoundShaders[xiiGALShaderStage::HullShader])
  {
    m_PipelineStateDesc.pHS                        = pHS;
    m_pBoundShaders[xiiGALShaderStage::HullShader] = pHS;
    m_bPipelineStateModified                       = true;
  }

  if (pDS != m_pBoundShaders[xiiGALShaderStage::DomainShader])
  {
    m_PipelineStateDesc.pDS                          = pDS;
    m_pBoundShaders[xiiGALShaderStage::DomainShader] = pDS;
    m_bPipelineStateModified                         = true;
  }

  if (pGS != m_pBoundShaders[xiiGALShaderStage::GeometryShader])
  {
    m_PipelineStateDesc.pGS                            = pGS;
    m_pBoundShaders[xiiGALShaderStage::GeometryShader] = pGS;
    m_bPipelineStateModified                           = true;
  }

  if (pPS != m_pBoundShaders[xiiGALShaderStage::PixelShader])
  {
    m_PipelineStateDesc.pPS                         = pPS;
    m_pBoundShaders[xiiGALShaderStage::PixelShader] = pPS;
    m_bPipelineStateModified                        = true;
  }

  if (pCS != m_pBoundShaders[xiiGALShaderStage::ComputeShader])
  {
    // TODO Create Compute Graphics Pipeline
    m_pBoundShaders[xiiGALShaderStage::ComputeShader] = pCS;
    m_bPipelineStateModified                          = true;
  }


  if (pCS != m_pBoundShaders[xiiGALShaderStage::ComputeShader])
  {
    m_PipelineStateComputeDesc.pCS = pCS;
    m_bPipelineStateModified       = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  xiiGALBuffer* pBufferNonConst   = const_cast<xiiGALBuffer*>(pBuffer);
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pBufferNonConst)->GetBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

void xiiGALCommandEncoderImplDiligentD3D11::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALSamplerState* pSampler = const_cast<xiiGALSamplerState*>(pSamplerState);
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<xiiGALSamplerStateDiligent*>(pSampler)->GetSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

void xiiGALCommandEncoderImplDiligentD3D11::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  xiiGALResourceView* pResource = const_cast<xiiGALResourceView*>(pResourceView);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<xiiGALResourceViewDiligent*>(pResource)->GetResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

void xiiGALCommandEncoderImplDiligentD3D11::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessView* pUAView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);

  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView)->GetResourceView() : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);

  m_bDescriptorsModified = true;
}

// Query functions

void xiiGALCommandEncoderImplDiligentD3D11::BeginQueryPlatform(const xiiGALQuery* pQuery)
{
  xiiGALCommandEncoderImplDiligent::BeginQueryPlatform(pQuery);
}

void xiiGALCommandEncoderImplDiligentD3D11::EndQueryPlatform(const xiiGALQuery* pQuery)
{
  xiiGALCommandEncoderImplDiligent::EndQueryPlatform(pQuery);
}

xiiResult xiiGALCommandEncoderImplDiligentD3D11::GetQueryResultPlatform(const xiiGALQuery* pQuery, xiiUInt64& uiQueryResult)
{
  return xiiGALCommandEncoderImplDiligent::GetQueryResultPlatform(pQuery, uiQueryResult);
}

// Timestamp functions

void xiiGALCommandEncoderImplDiligentD3D11::InsertTimestampPlatform(xiiGALTimestampHandle hTimestamp)
{
  // TODO

  // XII_ASSERT_NOT_IMPLEMENTED;
}

// Resource update functions

void xiiGALCommandEncoderImplDiligentD3D11::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4 clearValues)
{
  // TODO

  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligentD3D11::ClearUnorderedAccessViewPlatform(const xiiGALUnorderedAccessView* pUnorderedAccessView, xiiVec4U32 clearValues)
{
  // TODO

  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligentD3D11::CopyBufferPlatform(const xiiGALBuffer* pDestination, const xiiGALBuffer* pSource)
{
  xiiGALCommandEncoderImplDiligent::CopyBufferPlatform(pDestination, pSource);
}

void xiiGALCommandEncoderImplDiligentD3D11::CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  xiiGALCommandEncoderImplDiligent::CopyBufferRegionPlatform(pDestination, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
}

void xiiGALCommandEncoderImplDiligentD3D11::UpdateBufferPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiGALUpdateMode::Enum updateMode)
{
  xiiGALCommandEncoderImplDiligent::UpdateBufferPlatform(pDestination, uiDestOffset, pSourceData, updateMode);
}

void xiiGALCommandEncoderImplDiligentD3D11::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
  xiiGALCommandEncoderImplDiligent::CopyTexturePlatform(pDestination, pSource);
}

void xiiGALCommandEncoderImplDiligentD3D11::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  xiiGALCommandEncoderImplDiligent::CopyTexturePlatform(pDestination, pSource);
}

void xiiGALCommandEncoderImplDiligentD3D11::UpdateTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiBoundingBoxu32& DestinationBox, const xiiGALSystemMemoryDescription& pSourceData)
{
  xiiGALCommandEncoderImplDiligent::UpdateTexturePlatform(pDestination, DestinationSubResource, DestinationBox, pSourceData);
}

void xiiGALCommandEncoderImplDiligentD3D11::ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource)
{
  xiiGALCommandEncoderImplDiligent::ResolveTexturePlatform(pDestination, DestinationSubResource, pSource, SourceSubResource);
}

void xiiGALCommandEncoderImplDiligentD3D11::ReadbackTexturePlatform(const xiiGALTexture* pTexture)
{
  xiiGALCommandEncoderImplDiligent::ReadbackTexturePlatform(pTexture);
}

void xiiGALCommandEncoderImplDiligentD3D11::CopyTextureReadbackResultPlatform(const xiiGALTexture* pTexture, xiiArrayPtr<xiiGALTextureSubresource> SourceSubResource, xiiArrayPtr<xiiGALSystemMemoryDescription> TargetData)
{
  xiiGALCommandEncoderImplDiligent::CopyTextureReadbackResultPlatform(pTexture, SourceSubResource, TargetData);
}

void xiiGALCommandEncoderImplDiligentD3D11::GenerateMipMapsPlatform(const xiiGALResourceView* pResourceView)
{
  xiiGALCommandEncoderImplDiligent::GenerateMipMapsPlatform(pResourceView);
}

void xiiGALCommandEncoderImplDiligentD3D11::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void xiiGALCommandEncoderImplDiligentD3D11::PushMarkerPlatform(const char* szMarker)
{
  xiiGALCommandEncoderImplDiligent::PushMarkerPlatform(szMarker);
}

void xiiGALCommandEncoderImplDiligentD3D11::PopMarkerPlatform()
{
  xiiGALCommandEncoderImplDiligent::PopMarkerPlatform();
}

void xiiGALCommandEncoderImplDiligentD3D11::InsertEventMarkerPlatform(const char* szMarker)
{
  xiiGALCommandEncoderImplDiligent::InsertEventMarkerPlatform(szMarker);
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligentD3D11::BeginRendering(const xiiGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    xiiGALRenderTargetView* pRenderTargetViews[XII_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    xiiGALRenderTargetView* pDepthStencilView                                  = nullptr;

    const xiiUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    // Get references to render target views
    for (xiiUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      xiiGALRenderTargetView* pRenderTargetView = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex)));

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    // Get reference to the depth stencil view
    pDepthStencilView = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget()));

    for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    // Set render targets and depth stencil views to be bound by Diligent
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

      for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; ++i)
      {
        // Set render target format which is the format of the swap chain's color buffer
        m_PipelineStateDesc.GraphicsPipeline.RTVFormats[i] = m_pBoundRenderTargets[i]->GetDesc().Format;
      }

      // Set depth buffer format which is the format of the current bound depth stencil buffer
      m_PipelineStateDesc.GraphicsPipeline.DSVFormat = m_pBoundDepthStencilTarget->GetDesc().Format;

      // Set the numbe of render targets
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

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void xiiGALCommandEncoderImplDiligentD3D11::BeginCompute()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = xiiGALRenderTargetSetup();
  m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  m_bPipelineStateModified = true;
}

// Draw functions

void xiiGALCommandEncoderImplDiligentD3D11::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
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

void xiiGALCommandEncoderImplDiligentD3D11::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  xiiGALCommandEncoderImplDiligent::DrawPlatform(uiVertexCount, uiStartVertex);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex)
{
  xiiGALCommandEncoderImplDiligent::DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex)
{
  xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawIndexedInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALCommandEncoderImplDiligent::DrawIndexedInstancedIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex)
{
  xiiGALCommandEncoderImplDiligent::DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawInstancedIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALCommandEncoderImplDiligent::DrawInstancedIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

void xiiGALCommandEncoderImplDiligentD3D11::DrawAutoPlatform()
{
  // FlushDeferredStateChanges();

  XII_ASSERT_NOT_IMPLEMENTED
}

void xiiGALCommandEncoderImplDiligentD3D11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligentD3D11::EndStreamOutPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandEncoderImplDiligentD3D11::SetIndexBufferPlatform(const xiiGALBuffer* pIndexBuffer)
{
  xiiGALBuffer*         pIndexBufferNonConst = const_cast<xiiGALBuffer*>(pIndexBuffer);
  xiiGALBufferDiligent* pDiligentBuffer      = static_cast<xiiGALBufferDiligent*>(pIndexBufferNonConst);

  if (m_pIndexBuffer != pDiligentBuffer->GetBuffer())
  {
    m_pIndexBuffer         = pDiligentBuffer->GetBuffer();
    m_bIndexBufferModified = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetVertexBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pVertexBuffer)
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

void xiiGALCommandEncoderImplDiligentD3D11::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  const Diligent::InputLayoutDesc* desc = static_cast<const xiiGALVertexDeclarationDiligent*>(pVertexDeclaration)->GetInputLayoutDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.InputLayout != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.InputLayout = *desc;
    m_bPipelineStateModified                         = true;
  }
}

static const Diligent::PRIMITIVE_TOPOLOGY GALTopologyToDiligent[xiiGALPrimitiveTopology::ENUM_COUNT] = {
  Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST,
  Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

void xiiGALCommandEncoderImplDiligentD3D11::SetPrimitiveTopologyPlatform(xiiGALPrimitiveTopology::Enum Topology)
{
  if (m_PipelineStateDesc.GraphicsPipeline.PrimitiveTopology != GALTopologyToDiligent[Topology])
  {
    m_PipelineStateDesc.GraphicsPipeline.PrimitiveTopology = GALTopologyToDiligent[Topology];
    m_bPipelineStateModified                               = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  const float BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pContext->SetBlendFactors(BlendFactors);

  const Diligent::BlendStateDesc* desc = static_cast<const xiiGALBlendStateDiligent*>(pBlendState)->GetBlendStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.BlendDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.BlendDesc = *desc;
    m_bPipelineStateModified                       = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  const Diligent::DepthStencilStateDesc* desc = static_cast<const xiiGALDepthStencilStateDiligent*>(pDepthStencilState)->GetDepthStencilStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.DepthStencilDesc = *desc;
    m_bPipelineStateModified                              = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  const Diligent::RasterizerStateDesc* desc = static_cast<const xiiGALRasterizerStateDiligent*>(pRasterizerState)->GetRasterizerStateDesc();

  if (m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc != *desc)
  {
    m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc = *desc;

    if (m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc.ScissorEnable != m_bScissorEnabled)
    {
      m_bScissorEnabled   = m_PipelineStateDesc.GraphicsPipeline.RasterizerDesc.ScissorEnable;
      m_bViewportModified = true;
    }
    m_bPipelineStateModified = true;
  }
}

void xiiGALCommandEncoderImplDiligentD3D11::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
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

void xiiGALCommandEncoderImplDiligentD3D11::SetScissorRectPlatform(const xiiRectU32& rect)
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

void xiiGALCommandEncoderImplDiligentD3D11::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligentD3D11::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  xiiGALCommandEncoderImplDiligent::DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void xiiGALCommandEncoderImplDiligentD3D11::DispatchIndirectPlatform(const xiiGALBuffer* pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  xiiGALCommandEncoderImplDiligent::DispatchIndirectPlatform(pIndirectArgumentBuffer, uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

// Some state changes are deferred so they can be updated faster
void xiiGALCommandEncoderImplDiligentD3D11::FlushDeferredStateChanges()
{
  if (m_bPipelineStateModified)
  {
    XII_GAL_DILIGENT_RELEASE(m_pShaderResourceBinding);
    XII_GAL_DILIGENT_RELEASE(m_pPipelineState);

    if (m_bComputePipelineRequested)
    {
      m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateComputeDesc, &m_pPipelineState);

      // Create a shader resource binding object and bind all static resources in it
      m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

      m_pContext->CommitShaderResources(m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    else
    {
      m_GALDeviceDiligent.GetDevice()->CreatePipelineState(m_PipelineStateDesc, &m_pPipelineState);

      // Create a shader resource binding object and bind all static resources in it
      m_pPipelineState->CreateShaderResourceBinding(&m_pShaderResourceBinding, true);

      m_pContext->SetPipelineState(m_pPipelineState);

      m_pContext->CommitShaderResources(m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    m_bPipelineStateModified = false;
    // Changes to the descriptor layout always require the descriptor set to be re-created.
    // m_bDescriptorsModified = true;
  }

  if (!m_bComputePipelineRequested && m_bViewportModified)
  {
    m_pContext->SetViewports(1, &m_Viewport, static_cast<Diligent::Uint32>(m_Viewport.Width), static_cast<Diligent::Uint32>(m_Viewport.Height));

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
    // Set Shader Resource View (Automatically handled by Diligent

    for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
      if (m_BoundShaderResourceViewsRange[stage].IsValid())
      {
        const xiiUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
        const xiiUInt32 uiNumSlots  = m_BoundShaderResourceViewsRange[stage].GetCount();

        switch (stage)
        {
          case xiiGALShaderStage::VertexShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;
          case xiiGALShaderStage::HullShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;
          case xiiGALShaderStage::DomainShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;
          case xiiGALShaderStage::GeometryShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;
          case xiiGALShaderStage::PixelShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;
          case xiiGALShaderStage::ComputeShader:
          {
            for (xiiUInt32 uiCurrentSlot = uiStartSlot; uiCurrentSlot < uiNumSlots; ++uiCurrentSlot)
            {
              if (Diligent::ITextureView* pTexView = reinterpret_cast<Diligent::ITextureView*>(*(m_pBoundShaderResourceViews[stage].GetData() + uiCurrentSlot)))
              {
                // m_pShaderResourceBinding->GetVariableByName(Diligent::SHADER_TYPE_VERTEX, "");
              }
            }
          }
          break;

            XII_DEFAULT_CASE_NOT_IMPLEMENTED;
        }

        m_pBoundShaderResourceViews;

        //SetShaderResources((xiiGALShaderStage::Enum)stage, m_pContext, uiStartSlot, uiNumSlots, (Diligent::IShaderResourceBinding*)m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

        m_BoundShaderResourceViewsRange[stage].Reset();
      }

      // Don't need to unset sampler stages for unbound shader stages.
      if (m_pBoundShaders[stage] == nullptr)
        continue;

      if (m_BoundSamplerStatesRange[stage].IsValid())
      {
        const xiiUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
        const xiiUInt32 uiNumSlots  = m_BoundSamplerStatesRange[stage].GetCount();

        //SetSamplers((xiiGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

        m_BoundSamplerStatesRange[stage].Reset();
      }
    }

    // Set Shader Constant Buffers

    // Set Shader Unordered Access Views

    // Set Shader Sampler State

    m_bDescriptorsModified = false;
  }
}
