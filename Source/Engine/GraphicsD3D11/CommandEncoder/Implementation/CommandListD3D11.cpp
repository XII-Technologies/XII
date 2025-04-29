#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <Foundation/Memory/MemoryUtils.h>

#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Device/SwapChainD3D11.h>

#include <GraphicsD3D11/Resources/BottomLevelASD3D11.h>
#include <GraphicsD3D11/Resources/BufferD3D11.h>
#include <GraphicsD3D11/Resources/BufferViewD3D11.h>
#include <GraphicsD3D11/Resources/FenceD3D11.h>
#include <GraphicsD3D11/Resources/FramebufferD3D11.h>
#include <GraphicsD3D11/Resources/QueryD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Resources/SamplerD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>
#include <GraphicsD3D11/Resources/TopLevelASD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>
#include <GraphicsD3D11/States/BlendStateD3D11.h>
#include <GraphicsD3D11/States/DepthStencilStateD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>
#include <GraphicsD3D11/States/RasterizerStateD3D11.h>

#include <d3d11_1.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandListD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandListD3D11::xiiGALCommandListD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, xiiGALCommandQueueD3D11* pCommandQueueD3D11, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(pDeviceD3D11, pCommandQueueD3D11, creationDescription), m_pCommandQueueD3D11(pCommandQueueD3D11), m_pImmediateContext(pDeviceD3D11->GetImmediateContext()), m_GALSwapChainD3D11EventSubscriptionID(xiiGALSwapChainD3D11::s_Events.AddEventHandler(xiiMakeDelegate(&xiiGALCommandListD3D11::GALSwapChainD3D11EventHandler, this)))
{
}

xiiGALCommandListD3D11::~xiiGALCommandListD3D11()
{
  xiiGALSwapChainD3D11::s_Events.RemoveEventHandler(m_GALSwapChainD3D11EventSubscriptionID);
}

void xiiGALCommandListD3D11::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_pImmediateContext != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pImmediateContext->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D11 immediate context debug name.");
    }
  }
}

void xiiGALCommandListD3D11::GALSwapChainD3D11EventHandler(const xiiGALSwapChainD3D11Event& e)
{
  // Reset command list swapchain references or ResizeBuffers will fail as the backbuffer is still referenced.
  if (e.m_pSwapChainD3D11 != nullptr && e.m_Type == xiiGALSwapChainD3D11EventType::BeforeBufferRelease)
  {
    // Nothing to do at the moment. We had this callback originally to release active command list objects from deferred contexts.
  }
}

void xiiGALCommandListD3D11::BeginPlatform()
{
  m_RecordingState = RecordingState::Recording;
}

void xiiGALCommandListD3D11::EndPlatform()
{
  m_RecordingState = RecordingState::Ended;
}

void xiiGALCommandListD3D11::ResetPlatform()
{
  XII_ASSERT_DEV(m_RecordingState == RecordingState::Ended, "Command list has not been ended by the GAL!");

  InvalidateState();

  m_RecordingState = RecordingState::Reset;
}

xiiUInt64 xiiGALCommandListD3D11::SubmitPlatform()
{
  if (xiiGALCommandQueueD3D11* pCommandQueueD3D11 = static_cast<xiiGALCommandQueueD3D11*>(GetCommandQueue()))
  {
    return pCommandQueueD3D11->SubmitCommandList(this);
  }
  return xiiMath::MaxValue<xiiUInt64>();
}

void xiiGALCommandListD3D11::SetPipelineStatePlatform(xiiSharedPtr<xiiGALPipelineState> pPipelineState)
{
  auto pPipelineStateD3D11 = pPipelineState.Downcast<xiiGALPipelineStateD3D11>();

  if (pPipelineStateD3D11 == m_pCommittedPipelineState)
    return;

  InvalidateCommittedResources();

  if (pPipelineStateD3D11 != nullptr)
  {
    const auto& description = pPipelineStateD3D11->GetDescription();

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Vertex] != pPipelineStateD3D11->GetD3D11VertexShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Vertex]                  = pPipelineStateD3D11->GetD3D11VertexShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Vertex] = true;
    }

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Pixel] != pPipelineStateD3D11->GetD3D11PixelShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Pixel]                  = pPipelineStateD3D11->GetD3D11PixelShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Pixel] = true;
    }

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Compute] != pPipelineStateD3D11->GetD3D11ComputeShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Compute]                  = pPipelineStateD3D11->GetD3D11ComputeShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Compute] = true;
    }

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Domain] != pPipelineStateD3D11->GetD3D11DomainShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Domain]                  = pPipelineStateD3D11->GetD3D11DomainShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Domain] = true;
    }

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Hull] != pPipelineStateD3D11->GetD3D11HullShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Hull]                  = pPipelineStateD3D11->GetD3D11HullShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Hull] = true;
    }

    if (m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Geometry] != pPipelineStateD3D11->GetD3D11GeometryShader())
    {
      m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Geometry]                  = pPipelineStateD3D11->GetD3D11GeometryShader();
      m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Geometry] = true;
    }

    D3D_PRIMITIVE_TOPOLOGY d3d11PrimitiveTopology = xiiD3D11TypeConversions::GetPrimitiveTopology(description.m_GraphicsPipeline.m_PrimitiveTopology);
    if (m_CommittedPrimitiveTopology != d3d11PrimitiveTopology)
    {
      m_CommittedPrimitiveTopology = d3d11PrimitiveTopology;

      m_bPrimitiveTopologyModified = true;
    }

    if (description.IsAnyGraphicsPipeline())
    {
      if (pPipelineStateD3D11->GetD3D11InputLayout() != nullptr)
      {
        m_pCommittedInputLayout = pPipelineStateD3D11->GetD3D11InputLayout();
      }
      else
      {
        m_pCommittedInputLayout = nullptr;
      }
      m_bInputLayoutStateModified = true;

      if (pPipelineStateD3D11->GetD3D11RasterizerState() != nullptr)
      {
        m_pCommittedRasterizerState = pPipelineStateD3D11->GetD3D11RasterizerState();
      }
      else
      {
        m_pCommittedRasterizerState = nullptr;
      }
      m_bRasterizerStateModified = true;

      if (pPipelineStateD3D11->GetD3D11RasterizerState() != nullptr)
      {
        m_pCommittedRasterizerState = pPipelineStateD3D11->GetD3D11RasterizerState();
      }
      else
      {
        m_pCommittedRasterizerState = nullptr;
      }
      m_bRasterizerStateModified = true;

      if (pPipelineStateD3D11->GetD3D11BlendState() != nullptr || pPipelineStateD3D11->GetDescription().m_GraphicsPipeline.m_uiSampleMask != m_uiCommittedBlendSampleMask)
      {
        m_uiCommittedBlendSampleMask = pPipelineStateD3D11->GetDescription().m_GraphicsPipeline.m_uiSampleMask;
        m_pCommittedBlendState       = pPipelineStateD3D11->GetD3D11BlendState() != nullptr ? pPipelineStateD3D11->GetD3D11BlendState() : nullptr;
      }
      else
      {
        m_pCommittedBlendState       = nullptr;
        m_uiCommittedBlendSampleMask = 0xFFFFFFFFU;
      }
      m_bBlendStateModified = true;

      if (pPipelineStateD3D11->GetD3D11DepthStencilState() != nullptr)
      {
        m_pCommittedDepthStencilState = pPipelineStateD3D11->GetD3D11DepthStencilState();
      }
      else
      {
        m_pCommittedDepthStencilState = nullptr;
      }
      m_bDepthStencilStateModified = true;
    }
  }
  else
  {
    for (xiiUInt32 uiStage = 0U; uiStage < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStage)
    {
      if (m_CommittedShaders[uiStage] != nullptr)
      {
        m_CommittedShaders[uiStage]                  = nullptr;
        m_CommittedShaderModificationStates[uiStage] = true;
      }
    }

    if (m_pCommittedInputLayout != nullptr)
    {
      m_pCommittedInputLayout     = nullptr;
      m_bInputLayoutStateModified = true;
    }

    if (m_pCommittedRasterizerState != nullptr)
    {
      m_pCommittedRasterizerState = nullptr;
      m_bRasterizerStateModified  = true;
    }

    if (m_pCommittedBlendState != nullptr)
    {
      m_pCommittedBlendState       = nullptr;
      m_uiCommittedBlendSampleMask = 0xFFFFFFFFU;
      m_bBlendStateModified        = true;
    }

    if (m_pCommittedDepthStencilState != nullptr)
    {
      m_pCommittedDepthStencilState = nullptr;
      m_bDepthStencilStateModified  = true;
    }

    if (m_CommittedPrimitiveTopology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    {
      m_CommittedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

      m_bPrimitiveTopologyModified = true;
    }
  }

  m_pCommittedPipelineState = pPipelineStateD3D11;
}

void xiiGALCommandListD3D11::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  if (uiStencilRef != m_uiCommittedStencilReference)
  {
    m_uiCommittedStencilReference = uiStencilRef;

    m_bDepthStencilStateModified = true;
  }
}

void xiiGALCommandListD3D11::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  if (m_CommittedBlendFactors != blendFactor)
  {
    m_CommittedBlendFactors = blendFactor;

    m_bBlendStateModified = true;
  }
}

void xiiGALCommandListD3D11::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
  static_assert(XII_GAL_MAX_VIEWPORT_COUNT >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "The XII_GAL_MAX_VIEWPORT_COUNT must be greater than (or equal to) D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.");

  XII_ASSERT_DEV(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");

  D3D11_VIEWPORT d3d11Viewports[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiViewPortIndex = 0; uiViewPortIndex < pViewports.GetCount(); ++uiViewPortIndex)
  {
    d3d11Viewports[uiViewPortIndex].TopLeftX = pViewports[uiViewPortIndex].m_fTopLeftX;
    d3d11Viewports[uiViewPortIndex].TopLeftY = pViewports[uiViewPortIndex].m_fTopLeftY;
    d3d11Viewports[uiViewPortIndex].Width    = pViewports[uiViewPortIndex].m_fWidth;
    d3d11Viewports[uiViewPortIndex].Height   = pViewports[uiViewPortIndex].m_fHeight;
    d3d11Viewports[uiViewPortIndex].MinDepth = pViewports[uiViewPortIndex].m_fMinDepth;
    d3d11Viewports[uiViewPortIndex].MaxDepth = pViewports[uiViewPortIndex].m_fMaxDepth;
  }

  // All viewports must be set atomically as one operation.
  // Any viewports not defined by the call are disabled.
  m_pImmediateContext->RSSetViewports(pViewports.GetCount(), d3d11Viewports);
}

void xiiGALCommandListD3D11::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
  static_assert(XII_GAL_MAX_VIEWPORT_COUNT >= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "The XII_GAL_MAX_VIEWPORT_COUNT must be greater than (or equal to) D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.");

  XII_ASSERT_DEV(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");

  D3D11_RECT d3d11ScissorRects[XII_GAL_MAX_VIEWPORT_COUNT];

  for (xiiUInt32 uiScissorRectIndex = 0; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    d3d11ScissorRects[uiScissorRectIndex].left   = pRects[uiScissorRectIndex].Left();
    d3d11ScissorRects[uiScissorRectIndex].top    = pRects[uiScissorRectIndex].Top();
    d3d11ScissorRects[uiScissorRectIndex].right  = pRects[uiScissorRectIndex].Right();
    d3d11ScissorRects[uiScissorRectIndex].bottom = pRects[uiScissorRectIndex].Bottom();
  }

  // All scissor rects must be set atomically as one operation.
  // Any scissor rects not defined by the call are disabled.
  m_pImmediateContext->RSSetScissorRects(pRects.GetCount(), d3d11ScissorRects);
}

void xiiGALCommandListD3D11::SetIndexBufferPlatform(xiiSharedPtr<xiiGALBuffer> pIndexBuffer, xiiUInt64 uiByteOffset)
{
  auto          pIndexBufferD3D11 = pIndexBuffer.Downcast<xiiGALBufferD3D11>();
  ID3D11Buffer* pD3D11IndexBuffer = pIndexBufferD3D11 ? pIndexBufferD3D11->GetBuffer() : nullptr;

  if (pD3D11IndexBuffer != m_pCommittedIndexBuffer || m_uiCommittedIndexDataStartOffset != uiByteOffset)
  {
    m_pCommittedIndexBuffer           = pD3D11IndexBuffer;
    m_CommittedIndexBufferFormat      = DXGI_FORMAT_R16_UINT;
    m_uiCommittedIndexDataStartOffset = static_cast<xiiUInt32>(uiByteOffset);

    if (pIndexBufferD3D11 != nullptr)
    {
      const auto indexFormat = pIndexBufferD3D11->GetIndexFormat();

      DXGI_FORMAT d3d11IndexFormat = DXGI_FORMAT_R16_UINT;
      if (indexFormat == xiiGALValueType::UInt32)
      {
        d3d11IndexFormat = DXGI_FORMAT_R32_UINT;
      }
      else if (indexFormat == xiiGALValueType::UInt16)
      {
        d3d11IndexFormat = DXGI_FORMAT_R16_UINT;
      }
      else
      {
        xiiLog::Error("Unsupported index format, only xiiGALValueType::UInt16 or xiiGALValueType::UInt32 are supported.");
        return;
      }
      m_CommittedIndexBufferFormat = d3d11IndexFormat;
    }
    else
    {
      m_pCommittedIndexBuffer           = nullptr;
      m_uiCommittedIndexDataStartOffset = 0U;
    }

    m_bIndexBufferModified = true;
  }
}

void xiiGALCommandListD3D11::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBuffer*> pVertexBuffers, xiiArrayPtr<xiiUInt64> pByteOffsets, xiiBitflags<xiiGALSetVertexBufferFlags> flags)
{
  XII_ASSERT_DEV((uiStartSlot + pVertexBuffers.GetCount()) <= XII_GAL_MAX_VERTEX_BUFFER_COUNT, "The number of vertex buffers to set, exceeds the maximum amount.");

  if (flags.IsSet(xiiGALSetVertexBufferFlags::Reset))
  {
    m_CommittedVertexBuffersRange.Reset();

    // Reset only the buffer slots that are not being set.
    for (xiiUInt32 i = 0; i < uiStartSlot; ++i)
    {
      m_pCommittedVertexBuffers[i]      = nullptr;
      m_CommittedVertexBufferOffsets[i] = 0U;
      m_CommittedVertexBufferStrides[i] = 0U;
    }
    for (xiiUInt32 i = uiStartSlot + pVertexBuffers.GetCount(); i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
    {
      m_pCommittedVertexBuffers[i]      = nullptr;
      m_CommittedVertexBufferOffsets[i] = 0U;
      m_CommittedVertexBufferStrides[i] = 0U;
    }
  }

  for (xiiUInt32 i = 0; i < pVertexBuffers.GetCount(); ++i)
  {
    auto pVertexBufferD3D11 = static_cast<xiiGALBufferD3D11*>(pVertexBuffers[i]);

    ID3D11Buffer* pD3D11VertexBuffer   = pVertexBufferD3D11 ? pVertexBufferD3D11->GetBuffer() : nullptr;
    xiiUInt32     uiVertexBufferOffset = pByteOffsets.IsEmpty() ? 0U : static_cast<xiiUInt32>(pByteOffsets[i]);
    xiiUInt32     uiVertexBufferStride = pVertexBufferD3D11 ? pVertexBufferD3D11->GetDescription().m_uiElementByteStride : 0U;

    xiiUInt32 uiVertexBufferSlot = i + uiStartSlot;
    if (m_pCommittedVertexBuffers[uiVertexBufferSlot] != pD3D11VertexBuffer || m_CommittedVertexBufferOffsets[uiVertexBufferSlot] != uiVertexBufferOffset || m_CommittedVertexBufferStrides[uiVertexBufferSlot] != uiVertexBufferStride)
    {
      m_pCommittedVertexBuffers[uiVertexBufferSlot]      = pD3D11VertexBuffer;
      m_CommittedVertexBufferOffsets[uiVertexBufferSlot] = uiVertexBufferOffset;
      m_CommittedVertexBufferStrides[uiVertexBufferSlot] = uiVertexBufferStride;
    }

    m_CommittedVertexBuffersRange.SetToIncludeValue(uiVertexBufferSlot);
  }
}

void xiiGALCommandListD3D11::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
  XII_ASSERT_RELEASE(bindingInformation.m_uiBindSet == 0, "In D3D11, Shader resources use a single descriptor set.");
  XII_ASSERT_RELEASE(bindingInformation.m_uiBindSlot < XII_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer bind slot ({0}) must be in the range [0, {1})!", bindingInformation.m_uiBindSlot, XII_GAL_MAX_CONSTANT_BUFFER_COUNT);

  ID3D11Buffer* pD3D11Buffer = pConstantBuffer != nullptr ? pConstantBuffer.Downcast<xiiGALBufferD3D11>()->GetBuffer() : nullptr;

  if (m_pBoundConstantBuffers[bindingInformation.m_uiBindSlot] == pD3D11Buffer)
    return;

  m_pBoundConstantBuffers[bindingInformation.m_uiBindSlot] = pD3D11Buffer;

  // Though the GAL knows about the stages, we ignore that and handle this internally.

  for (xiiUInt32 uiStage = 0; uiStage < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStage)
  {
    m_BoundConstantBuffersRange[uiStage].SetToIncludeValue(bindingInformation.m_uiBindSlot);
  }
}

void xiiGALCommandListD3D11::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  if (pBufferView != nullptr && UnsetUnorderedAccessViews(pBufferView->GetBuffer()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11ShaderResourceView* pD3D11ShaderResourceView = pBufferView != nullptr ? static_cast<ID3D11ShaderResourceView*>(pBufferView.Downcast<xiiGALBufferViewD3D11>()->GetBufferView()) : nullptr;

  for (xiiGALShaderType::Enum shaderType : xiiIterateBitIndices<xiiGALShaderType::StorageType, xiiGALShaderType::Enum>(bindingInformation.m_ShaderStages.GetValue()))
  {
    xiiUInt32 uiStageIndex = xiiGALPipelineStateD3D11::ShaderType::GetIndex(xiiGALShaderType::GetStageFlag(shaderType));

    auto& boundShaderResourceViews = m_pBoundShaderResourceViews[uiStageIndex];
    boundShaderResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

    auto& resourcesForResourceViews = m_ResourcesForResourceViews[uiStageIndex];
    resourcesForResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

    if (boundShaderResourceViews[bindingInformation.m_uiBindSlot] != pD3D11ShaderResourceView)
    {
      boundShaderResourceViews[bindingInformation.m_uiBindSlot]  = pD3D11ShaderResourceView;
      resourcesForResourceViews[bindingInformation.m_uiBindSlot] = pBufferView != nullptr ? pBufferView->GetBuffer() : nullptr;

      m_BoundShaderResourceViewsRange[uiStageIndex].SetToIncludeValue(bindingInformation.m_uiBindSlot);
    }
  }
}

void xiiGALCommandListD3D11::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  XII_ASSERT_DEV(bindingInformation.m_ResourceType == xiiGALShaderResourceType::TextureSRV, "D3D11 supports only texture shader resource views and not combined image samplers.");

  if (pTextureView != nullptr && UnsetUnorderedAccessViews(pTextureView->GetTexture()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11ShaderResourceView* pD3D11ShaderResourceView = pTextureView != nullptr ? static_cast<ID3D11ShaderResourceView*>(pTextureView.Downcast<xiiGALTextureViewD3D11>()->GetTextureView()) : nullptr;

  for (xiiGALShaderType::Enum shaderType : xiiIterateBitIndices<xiiGALShaderType::StorageType, xiiGALShaderType::Enum>(bindingInformation.m_ShaderStages.GetValue()))
  {
    xiiUInt32 uiStageIndex = xiiGALPipelineStateD3D11::ShaderType::GetIndex(xiiGALShaderType::GetStageFlag(shaderType));

    auto& boundShaderResourceViews = m_pBoundShaderResourceViews[uiStageIndex];
    boundShaderResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

    auto& resourcesForResourceViews = m_ResourcesForResourceViews[uiStageIndex];
    resourcesForResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

    if (boundShaderResourceViews[bindingInformation.m_uiBindSlot] != pD3D11ShaderResourceView)
    {
      boundShaderResourceViews[bindingInformation.m_uiBindSlot]  = pD3D11ShaderResourceView;
      resourcesForResourceViews[bindingInformation.m_uiBindSlot] = pTextureView != nullptr ? pTextureView->GetTexture() : nullptr;

      m_BoundShaderResourceViewsRange[uiStageIndex].SetToIncludeValue(bindingInformation.m_uiBindSlot);
    }
  }
}

void xiiGALCommandListD3D11::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  if (pBufferView && UnsetResourceViews(pBufferView->GetBuffer()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11UnorderedAccessView* pD3D11UnorderedAccessView = pBufferView != nullptr ? static_cast<ID3D11UnorderedAccessView*>(pBufferView.Downcast<xiiGALBufferViewD3D11>()->GetBufferView()) : nullptr;

  m_BoundUnorderedAccessViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  m_ResourcesForUnorderedAccessViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

  if (m_BoundUnorderedAccessViews[bindingInformation.m_uiBindSlot] != pD3D11UnorderedAccessView)
  {
    m_BoundUnorderedAccessViews[bindingInformation.m_uiBindSlot]        = pD3D11UnorderedAccessView;
    m_ResourcesForUnorderedAccessViews[bindingInformation.m_uiBindSlot] = pBufferView != nullptr ? pBufferView->GetBuffer() : nullptr;

    m_BoundUnorderedAccessViewsRange.SetToIncludeValue(bindingInformation.m_uiBindSlot);
  }
}

void xiiGALCommandListD3D11::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  if (pTextureView && UnsetResourceViews(pTextureView->GetTexture()))
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  ID3D11UnorderedAccessView* pD3D11UnorderedAccessView = pTextureView != nullptr ? static_cast<ID3D11UnorderedAccessView*>(pTextureView.Downcast<xiiGALTextureViewD3D11>()->GetTextureView()) : nullptr;

  m_BoundUnorderedAccessViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);
  m_ResourcesForUnorderedAccessViews.EnsureCount(bindingInformation.m_uiBindSlot + 1);

  if (m_BoundUnorderedAccessViews[bindingInformation.m_uiBindSlot] != pD3D11UnorderedAccessView)
  {
    m_BoundUnorderedAccessViews[bindingInformation.m_uiBindSlot]        = pD3D11UnorderedAccessView;
    m_ResourcesForUnorderedAccessViews[bindingInformation.m_uiBindSlot] = pTextureView != nullptr ? pTextureView->GetTexture() : nullptr;
    m_BoundUnorderedAccessViewsRange.SetToIncludeValue(bindingInformation.m_uiBindSlot);
  }
}

void xiiGALCommandListD3D11::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiSharedPtr<xiiGALSampler> pSampler)
{
  XII_ASSERT_RELEASE(bindingInformation.m_uiBindSet == 0, "In D3D11, Shader resources use a single descriptor set.");
  XII_ASSERT_RELEASE(bindingInformation.m_uiBindSlot < XII_GAL_MAX_SAMPLER_COUNT, "Sampler bind slot ({0}) must be in the range [0, {1})!", bindingInformation.m_uiBindSlot, XII_GAL_MAX_SAMPLER_COUNT);

  ID3D11SamplerState* pD3D11SamplerState = pSampler != nullptr ? pSampler.Downcast<xiiGALSamplerD3D11>()->GetSampler() : nullptr;

  for (xiiGALShaderType::Enum shaderType : xiiIterateBitIndices<xiiGALShaderType::StorageType, xiiGALShaderType::Enum>(bindingInformation.m_ShaderStages.GetValue()))
  {
    xiiUInt32 uiStageIndex = xiiGALPipelineStateD3D11::ShaderType::GetIndex(xiiGALShaderType::GetStageFlag(shaderType));

    if (m_pBoundSamplerStates[uiStageIndex][bindingInformation.m_uiBindSlot] != pD3D11SamplerState)
    {
      m_pBoundSamplerStates[uiStageIndex][bindingInformation.m_uiBindSlot] = pD3D11SamplerState;

      m_BoundSamplerStatesRange[uiStageIndex].SetToIncludeValue(bindingInformation.m_uiBindSlot);
    }
  }
}

xiiResult xiiGALCommandListD3D11::CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)
{
  XII_IGNORE_UNUSED(mode);
  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::ClearRenderTargetViewPlatform(xiiSharedPtr<xiiGALTextureView> pRenderTargetView, const xiiColor& clearColor)
{
  auto pRenderTargetViewD3D11 = pRenderTargetView.Downcast<xiiGALTextureViewD3D11>();

  XII_ASSERT_DEV(pRenderTargetViewD3D11 != nullptr, "Invalid resource.");

  // The full extent of the resource view is always cleared. Viewport and scissor settings are not applied.
  m_pImmediateContext->ClearRenderTargetView(static_cast<ID3D11RenderTargetView*>(pRenderTargetViewD3D11->GetTextureView()), clearColor.GetData());
}

void xiiGALCommandListD3D11::ClearDepthStencilViewPlatform(xiiSharedPtr<xiiGALTextureView> pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  auto pDepthStencilViewD3D11 = pDepthStencilView.Downcast<xiiGALTextureViewD3D11>();

  XII_ASSERT_DEV(pDepthStencilViewD3D11 != nullptr, "Invalid resource.");

  xiiUInt32 uiClearFlags = 0;
  if (bClearDepth)
    uiClearFlags |= D3D11_CLEAR_DEPTH;
  if (bClearStencil)
    uiClearFlags |= D3D11_CLEAR_STENCIL;

  // The full extent of the resource view is always cleared. Viewport and scissor settings are not applied.
  m_pImmediateContext->ClearDepthStencilView(static_cast<ID3D11DepthStencilView*>(pDepthStencilViewD3D11->GetTextureView()), uiClearFlags, fDepthClear, uiStencilClear);
}

void xiiGALCommandListD3D11::BeginRenderPassPlatform(xiiSharedPtr<xiiGALRenderPass> pRenderPass, xiiSharedPtr<xiiGALFramebuffer> pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
  m_AttachmentClearValues.SetCountUninitialized(pOptimizedClearValues.GetCount());
  m_AttachmentClearValues = pOptimizedClearValues;

  // Set viewport to match frame buffer size.
  const auto&    framebufferDescription = pFramebuffer->GetDescription();
  xiiGALViewport viewport               = {.m_fTopLeftX = 0.0f, .m_fTopLeftY = 0.0f, .m_fWidth = (float)framebufferDescription.m_FramebufferSize.width, .m_fHeight = (float)framebufferDescription.m_FramebufferSize.height};
  SetViewports(xiiMakeArrayPtr(&viewport, 1U));

  m_pRenderPass  = pRenderPass.Downcast<xiiGALRenderPassD3D11>();
  m_pFramebuffer = pRenderPass.Downcast<xiiGALFramebufferD3D11>();

  // Set the active render targes.
  CommitRenderTargets();
}

void xiiGALCommandListD3D11::NextSubpassPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiGALCommandListD3D11::EndRenderPassPlatform()
{
  m_pCommittedDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount     = 0U;

  m_pImmediateContext->OMSetRenderTargets(0U, nullptr, nullptr);
}

xiiResult xiiGALCommandListD3D11::DrawPlatform(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->Draw(uiVertexCount, uiStartVertex);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedPlatform(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DrawIndexed(uiIndexCount, uiStartIndex, uiBaseVertex);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedPlatform(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex, xiiUInt32 uiBaseVertex, xiiUInt32 uiFirstInstance)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, uiBaseVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawIndexedInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = pIndirectArgumentBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DrawIndexedInstancedIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawInstancedPlatform(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex, xiiUInt32 uiFirstInstance)
{
  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, uiFirstInstance);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawInstancedIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = pIndirectArgumentBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DrawInstancedIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DrawMeshPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  XII_IGNORE_UNUSED(uiThreadGroupCountX);
  XII_IGNORE_UNUSED(uiThreadGroupCountY);
  XII_IGNORE_UNUSED(uiThreadGroupCountZ);

  XII_REPORT_FAILURE("DrawMesh is not supported in Direct3D 11.");

  return XII_FAILURE;
}

xiiResult xiiGALCommandListD3D11::DispatchPlatform(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::DispatchIndirectPlatform(xiiSharedPtr<xiiGALBuffer> pIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes)
{
  auto pIndirectArgumentBufferD3D11 = pIndirectArgumentBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pIndirectArgumentBufferD3D11 != nullptr, "Invalid resource.");

  m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

  XII_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pImmediateContext->DispatchIndirect(pIndirectArgumentBufferD3D11->GetBuffer(), uiArgumentOffsetInBytes);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::BeginQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery)
{
  auto pQueryD3D11       = pQuery.Downcast<xiiGALQueryD3D11>();
  auto pImmediateContext = m_pDevice.Downcast<xiiGALDeviceD3D11>()->GetImmediateContext();

  XII_ASSERT_DEV(pQueryD3D11 != nullptr, "Invalid resource.");

  if (pQueryD3D11->GetDescription().m_Type == xiiGALQueryType::Duration)
  {
    pQueryD3D11->SetDisjointQuery(BeginDisjointQuery());

    pImmediateContext->End(pQueryD3D11->GetQuery(0));
  }
  else
  {
    pImmediateContext->Begin(pQueryD3D11->GetQuery(0));
  }
}

void xiiGALCommandListD3D11::EndQueryPlatform(xiiSharedPtr<xiiGALQuery> pQuery)
{
  auto pQueryD3D11 = pQuery.Downcast<xiiGALQueryD3D11>();

  XII_ASSERT_DEV(pQueryD3D11 != nullptr, "Invalid resource.");

  xiiEnum<xiiGALQueryType> queryType = pQuery->GetDescription().m_Type;

  XII_ASSERT_DEV(queryType != xiiGALQueryType::Duration || m_pActiveDisjointQuery, "There is no active disjoint query. Did you forget to call BeginQuery for this duration query.");

  if (queryType == xiiGALQueryType::Timestamp)
  {
    pQueryD3D11->SetDisjointQuery(BeginDisjointQuery());
  }
  m_pImmediateContext->End(pQueryD3D11->GetQuery(queryType == xiiGALQueryType::Duration ? 1 : 0));
}

void xiiGALCommandListD3D11::UpdateBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
  XII_CHECK_ALIGNMENT(pSourceData.GetPtr(), 16);

  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();
  xiiGALBufferD3D11*              pBufferD3D11 = pBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pBufferD3D11 != nullptr, "Invalid resource.");

  if (ID3D11Resource* pD3D11TempBuffer = pDeviceD3D11->FindTemporaryBuffer(pSourceData.GetCount()))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT                  hRes = m_pImmediateContext->Map(pD3D11TempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
    XII_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error: {}", xiiHRESULTtoString(hRes));
    XII_IGNORE_UNUSED(hRes);

    memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

    m_pImmediateContext->Unmap(pD3D11TempBuffer, 0);

    // Schedule copy command using this command list.
    D3D11_BOX srcBox = {0, 0, 0, pSourceData.GetCount(), 1, 1};
    m_pImmediateContext->CopySubresourceRegion(pBufferD3D11->GetBuffer(), 0, uiDestinationOffset, 0, 0, pD3D11TempBuffer, 0, &srcBox);
  }
  else
  {
    xiiLog::Warning("Could not find a temporary buffer for update. Buffer update will be performed using UpdateSubresource().");

    D3D11_BOX destinationBox = {};
    destinationBox.left      = uiDestinationOffset;
    destinationBox.right     = uiDestinationOffset + pSourceData.GetCount();
    destinationBox.top       = 0U;
    destinationBox.bottom    = 1U;
    destinationBox.front     = 0U;
    destinationBox.back      = 1U;

    D3D11_BOX* pDestinationBox = (uiDestinationOffset == 0 && pSourceData.GetCount() == pBufferD3D11->GetDescription().m_uiSize) ? nullptr : &destinationBox;

    m_pImmediateContext->UpdateSubresource(pBufferD3D11->GetBuffer(), 0U, pDestinationBox, pSourceData.GetPtr(), 0U, 0U);
  }
}

void xiiGALCommandListD3D11::CopyBufferPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer)
{
  auto pSourceBufferD3D11      = pSourceBuffer.Downcast<xiiGALBufferD3D11>();
  auto pDestinationBufferD3D11 = pDestinationBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pSourceBufferD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationBufferD3D11 != nullptr, "Invalid resource.");

  m_pImmediateContext->CopyResource(pDestinationBufferD3D11->GetBuffer(), pSourceBufferD3D11->GetBuffer());
}

void xiiGALCommandListD3D11::CopyBufferRegionPlatform(xiiSharedPtr<xiiGALBuffer> pSourceBuffer, xiiUInt64 uiSourceOffset, xiiSharedPtr<xiiGALBuffer> pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  auto pSourceBufferD3D11      = pSourceBuffer.Downcast<xiiGALBufferD3D11>();
  auto pDestinationBufferD3D11 = pDestinationBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pSourceBufferD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationBufferD3D11 != nullptr, "Invalid resource.");

  D3D11_BOX sourceBox = {};
  sourceBox.left      = static_cast<xiiUInt32>(uiSourceOffset);
  sourceBox.right     = static_cast<xiiUInt32>(uiSourceOffset + uiSize);
  sourceBox.top       = 0U;
  sourceBox.bottom    = 1U;
  sourceBox.front     = 0U;
  sourceBox.back      = 1U;

  m_pImmediateContext->CopySubresourceRegion(pDestinationBufferD3D11->GetBuffer(), 0, static_cast<xiiUInt32>(uiDestinationOffset), 0, 0, pSourceBufferD3D11->GetBuffer(), 0, &sourceBox);
}

xiiResult xiiGALCommandListD3D11::MapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  auto pBufferD3D11 = pBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid resource.");

  D3D11_MAP bufferMapType = static_cast<D3D11_MAP>(0U);
  xiiUInt32 uiMapFlags    = 0U;
  xiiD3D11TypeConversions::GetMapTypeAndFlags(mapType, mapFlags, bufferMapType, uiMapFlags);

  D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  if (FAILED(m_pImmediateContext->Map(pBufferD3D11->GetBuffer(), 0U, bufferMapType, uiMapFlags, &mappedSubresource)))
  {
    xiiLog::Error("Failed to map buffer '{0}'.", pBufferD3D11->GetDebugName());
    return XII_FAILURE;
  }

  pMappedData = mappedSubresource.pData;

  m_MappedBuffers.Insert(pBufferD3D11, m_pImmediateContext);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::UnmapBufferPlatform(xiiSharedPtr<xiiGALBuffer> pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  XII_IGNORE_UNUSED(mapType);

  auto pBufferD3D11 = pBuffer.Downcast<xiiGALBufferD3D11>();

  XII_ASSERT_DEV(pBuffer != nullptr, "Invalid resource.");

  auto pCommandList = *m_MappedBuffers.GetValue(pBufferD3D11);
  pCommandList->Unmap(pBufferD3D11->GetBuffer(), 0U);

  m_MappedBuffers.Remove(pBufferD3D11);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::UpdateTexturePlatform(xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11  = m_pDevice.Downcast<xiiGALDeviceD3D11>();
  auto                            pTextureD3D11 = pTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();

  xiiUInt32                     uiWidth  = xiiMath::Max(textureBox.m_vMax.x - textureBox.m_vMin.x, 1U);
  xiiUInt32                     uiHeight = xiiMath::Max(textureBox.m_vMax.y - textureBox.m_vMin.y, 1U);
  xiiUInt32                     uiDepth  = xiiMath::Max(textureBox.m_vMax.z - textureBox.m_vMin.z, 1U);
  xiiEnum<xiiGALResourceFormat> format   = textureDescription.m_Format;

  if (ID3D11Resource* pD3D11TransientStagingTexture = pDeviceD3D11->FindTemporaryTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT                  hRes = m_pImmediateContext->Map(pD3D11TransientStagingTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    XII_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error: {}", xiiHRESULTtoString(hRes));
    XII_IGNORE_UNUSED(hRes);

    xiiUInt32 uiRowPitch   = uiWidth * xiiGALTextureUtilities::GetResourceFormatProperties(format).GetElementSize();
    xiiUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    XII_ASSERT_DEV(subresourceData.m_uiStride == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, subresourceData.m_uiStride);
    XII_ASSERT_DEV(subresourceData.m_uiDepthStride == 0 || subresourceData.m_uiDepthStride == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, subresourceData.m_uiDepthStride);

    if (MapResult.RowPitch == uiRowPitch && MapResult.DepthPitch == uiSlicePitch)
    {
      xiiMemoryUtils::RawByteCopy(MapResult.pData, subresourceData.m_pData.GetPtr(), uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (xiiUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource      = xiiMemoryUtils::AddByteOffset(subresourceData.m_pData.GetPtr(), z * uiSlicePitch);
        void*       pDestination = xiiMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthPitch);

        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          xiiMemoryUtils::RawByteCopy(pDestination, pSource, uiRowPitch);

          pSource      = xiiMemoryUtils::AddByteOffset(pSource, uiRowPitch);
          pDestination = xiiMemoryUtils::AddByteOffset(pDestination, MapResult.RowPitch);
        }
      }
    }

    m_pImmediateContext->Unmap(pD3D11TransientStagingTexture, 0);

    xiiUInt32 uiDestinationSubresource = D3D11CalcSubresource(textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, pTextureD3D11->GetDescription().m_uiMipLevels);
    D3D11_BOX sourceBox                = {0, 0, 0, uiWidth, uiHeight, uiDepth};

    m_pImmediateContext->CopySubresourceRegion(pTextureD3D11->GetTexture(), uiDestinationSubresource, textureBox.m_vMin.x, textureBox.m_vMin.y, textureBox.m_vMin.z, pD3D11TransientStagingTexture, 0, &sourceBox);
  }
  else
  {
    XII_ASSERT_DEV(textureDescription.m_Usage == xiiGALResourceUsage::Default || textureDescription.m_Usage == xiiGALResourceUsage::Sparse, "Only xiiGALResourceUsage::Default or xiiGALResourceUsage::Default textures should be updated with this method.");

    D3D11_BOX destinationBox = {};
    destinationBox.left      = textureBox.m_vMin.x;
    destinationBox.top       = textureBox.m_vMin.y;
    destinationBox.front     = textureBox.m_vMin.z;
    destinationBox.right     = textureBox.m_vMax.x;
    destinationBox.bottom    = textureBox.m_vMax.y;
    destinationBox.back      = textureBox.m_vMax.z;

    const auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
    {
      // Align update region by the compressed block size.
      XII_ASSERT_DEV((destinationBox.left % formatProperties.m_uiBlockWidth) == 0, "The update region min X coordinate ({0}) must be a multiple of a compressed block width ({1}).", destinationBox.left, formatProperties.m_uiBlockWidth);
      XII_ASSERT_DEV((formatProperties.m_uiBlockWidth % (formatProperties.m_uiBlockWidth - 1)) == 0, "The compressed block width ({0}) is expected to be a power of 2.", formatProperties.m_uiBlockWidth);
      destinationBox.right = (destinationBox.right + formatProperties.m_uiBlockWidth - 1) & ~(formatProperties.m_uiBlockHeight - 1);

      XII_ASSERT_DEV((destinationBox.top % formatProperties.m_uiBlockHeight) == 0, "The update region min X coordinate ({0}) must be a multiple of a compressed block height ({1}).", destinationBox.top, formatProperties.m_uiBlockHeight);
      XII_ASSERT_DEV((formatProperties.m_uiBlockHeight % (formatProperties.m_uiBlockHeight - 1)) == 0, "The compressed block height ({0}) is expected to be a power of 2.", formatProperties.m_uiBlockHeight);
      destinationBox.bottom = (destinationBox.bottom + formatProperties.m_uiBlockHeight - 1) & ~(formatProperties.m_uiBlockHeight - 1);
    }

    xiiUInt32 uiDestinationSubresourceIndex = D3D11CalcSubresource(textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);
    xiiUInt32 uiCopyFlags                   = D3D11_COPY_DISCARD;

    m_pImmediateContext->UpdateSubresource1(pTextureD3D11->GetTexture(), uiDestinationSubresourceIndex, &destinationBox, subresourceData.m_pData.GetPtr(), static_cast<xiiUInt32>(subresourceData.m_uiStride), static_cast<xiiUInt32>(subresourceData.m_uiDepthStride), uiCopyFlags);
  }
}

void xiiGALCommandListD3D11::CopyTexturePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, xiiSharedPtr<xiiGALTexture> pDestinationTexture)
{
  auto pSourceTextureD3D11      = pSourceTexture.Downcast<xiiGALTextureD3D11>();
  auto pDestinationTextureD3D11 = pDestinationTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  m_pImmediateContext->CopyResource(pDestinationTextureD3D11->GetTexture(), pSourceTextureD3D11->GetTexture());
}

void xiiGALCommandListD3D11::CopyTextureRegionPlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  auto pSourceTextureD3D11      = pSourceTexture.Downcast<xiiGALTextureD3D11>();
  auto pDestinationTextureD3D11 = pDestinationTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  D3D11_BOX sourceBox = {};
  sourceBox.left      = box.m_vMin.x;
  sourceBox.top       = box.m_vMin.y;
  sourceBox.front     = box.m_vMin.z;
  sourceBox.right     = box.m_vMax.x;
  sourceBox.bottom    = box.m_vMax.y;
  sourceBox.back      = box.m_vMax.z;

  xiiUInt32 uiSourceSubresource      = D3D11CalcSubresource(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pSourceTextureD3D11->GetDescription().m_uiMipLevels);
  xiiUInt32 uiDestinationSubresource = D3D11CalcSubresource(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, pDestinationTextureD3D11->GetDescription().m_uiMipLevels);

  m_pImmediateContext->CopySubresourceRegion(pDestinationTextureD3D11->GetTexture(), uiDestinationSubresource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pSourceTextureD3D11->GetTexture(), uiSourceSubresource, &sourceBox);
}

void xiiGALCommandListD3D11::ResolveTextureSubResourcePlatform(xiiSharedPtr<xiiGALTexture> pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, xiiSharedPtr<xiiGALTexture> pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData)
{
  auto pSourceTextureD3D11      = pSourceTexture.Downcast<xiiGALTextureD3D11>();
  auto pDestinationTextureD3D11 = pDestinationTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pSourceTextureD3D11 != nullptr, "Invalid resource.");
  XII_ASSERT_DEV(pDestinationTextureD3D11 != nullptr, "Invalid resource.");

  const auto& sourceTextureDescription      = pSourceTextureD3D11->GetDescription();
  const auto& destinationTextureDescription = pSourceTextureD3D11->GetDescription();

  DXGI_FORMAT textureFormat                 = xiiD3D11TypeConversions ::GetFormat(sourceTextureDescription.m_Format);
  xiiUInt32   uiSourceSubresourceIndex      = D3D11CalcSubresource(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiMipLevel, sourceTextureDescription.m_uiMipLevels);
  xiiUInt32   uiDestinationSubresourceIndex = D3D11CalcSubresource(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiMipLevel, destinationTextureDescription.m_uiMipLevels);

  m_pImmediateContext->ResolveSubresource(pDestinationTextureD3D11->GetTexture(), uiDestinationSubresourceIndex, pSourceTextureD3D11->GetTexture(), uiSourceSubresourceIndex, textureFormat);
}

void xiiGALCommandListD3D11::GenerateMipsPlatform(xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  auto pTextureViewD3D11 = pTextureView.Downcast<xiiGALTextureViewD3D11>();

  XII_ASSERT_DEV(pTextureViewD3D11 != nullptr, "Invalid resource.");

  m_pImmediateContext->GenerateMips(static_cast<ID3D11ShaderResourceView*>(pTextureViewD3D11->GetTextureView()));
}

xiiResult xiiGALCommandListD3D11::MapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  XII_IGNORE_UNUSED(pTextureBox);

  auto pTextureD3D11 = pTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();
  D3D11_MAP   textureMapType     = static_cast<D3D11_MAP>(0U);
  xiiUInt32   uiMapFlags         = 0U;
  xiiD3D11TypeConversions::GetMapTypeAndFlags(mapType, mapFlags, textureMapType, uiMapFlags);

  xiiUInt32 uiSubresource = D3D11CalcSubresource(textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  HRESULT                  hr = m_pImmediateContext->Map(pTextureD3D11->GetTexture(), uiSubresource, textureMapType, uiMapFlags, &mappedSubresource);
  if (FAILED(hr))
  {
    // XII_ASSERT_DEV(hResult == DXGI_ERROR_WAS_STILL_DRAWING, "");

    xiiLog::Error("Failed to map texture subresource: {0}", xiiHRESULTtoString(hr));

    mappedData = xiiGALMappedTextureSubresource();
    return XII_FAILURE;
  }

  mappedData.m_pData         = mappedSubresource.pData;
  mappedData.m_uiStride      = mappedSubresource.RowPitch;
  mappedData.m_uiDepthStride = mappedSubresource.DepthPitch;

  m_MappedTextureSubresources.Insert(pTextureD3D11, m_pImmediateContext);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D11::UnmapTextureSubresourcePlatform(xiiSharedPtr<xiiGALTexture> pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  auto pTextureD3D11 = pTexture.Downcast<xiiGALTextureD3D11>();

  XII_ASSERT_DEV(pTextureD3D11 != nullptr, "Invalid resource.");

  const auto& textureDescription = pTextureD3D11->GetDescription();
  xiiUInt32   uiSubresource      = D3D11CalcSubresource(textureMipLevelData.m_uiMipLevel, textureMipLevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  auto pCommandList = *m_MappedTextureSubresources.GetValue(pTextureD3D11);
  pCommandList->Unmap(pTextureD3D11->GetTexture(), uiSubresource);

  m_MappedTextureSubresources.Remove(pTextureD3D11);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D11::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
  XII_IGNORE_UNUSED(color);

  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D11_RELEASE(pAnnotationD3D11));

  if (SUCCEEDED(m_pImmediateContext->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    xiiStringBuilder sb;
    xiiStringWChar   wsMarker(sName.GetData(sb));
    pAnnotationD3D11->BeginEvent(wsMarker.GetData());
  }
}

void xiiGALCommandListD3D11::EndDebugGroupPlatform()
{
  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D11_RELEASE(pAnnotationD3D11));

  if (SUCCEEDED(m_pImmediateContext->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    pAnnotationD3D11->EndEvent();
  }
}

void xiiGALCommandListD3D11::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
  XII_IGNORE_UNUSED(color);

  ID3DUserDefinedAnnotation* pAnnotationD3D11 = nullptr;
  XII_SCOPE_EXIT(XII_GAL_D3D11_RELEASE(pAnnotationD3D11));

  if (SUCCEEDED(m_pImmediateContext->QueryInterface(_uuidof(ID3DUserDefinedAnnotation), (void**)&pAnnotationD3D11)))
  {
    xiiStringBuilder sb;
    xiiStringWChar   wsMarker(sName.GetData(sb));
    pAnnotationD3D11->SetMarker(wsMarker.GetData());
  }
}

void xiiGALCommandListD3D11::InvalidateStatePlatform()
{
  XII_ASSERT_DEV(m_MappedBuffers.IsEmpty(), "Not all mapped buffers are released.");
  XII_ASSERT_DEV(m_MappedTextureSubresources.IsEmpty(), "Not all mapped texture subresources are released.");

  m_bIndexBufferModified       = false;
  m_bBlendStateModified        = false;
  m_bInputLayoutStateModified  = false;
  m_bDepthStencilStateModified = false;
  m_bRasterizerStateModified   = false;
  m_bPrimitiveTopologyModified = false;

  m_pCommittedPipelineState     = nullptr;
  m_pCommittedInputLayout       = nullptr;
  m_pCommittedRasterizerState   = nullptr;
  m_pCommittedBlendState        = nullptr;
  m_pCommittedDepthStencilState = nullptr;

  xiiMemoryUtils::ZeroFillArray(m_pCommittedVertexBuffers);
  xiiMemoryUtils::ZeroFillArray(m_CommittedVertexBufferStrides);
  xiiMemoryUtils::ZeroFillArray(m_CommittedVertexBufferOffsets);
  m_CommittedVertexBuffersRange.Reset();

  m_pCommittedIndexBuffer           = nullptr;
  m_CommittedIndexBufferFormat      = DXGI_FORMAT_R16_UINT;
  m_uiCommittedIndexDataStartOffset = 0U;

  m_CommittedPrimitiveTopology  = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  m_CommittedBlendFactors       = xiiColor::White;
  m_uiCommittedBlendSampleMask  = 0xFFFFFFFFU;
  m_uiCommittedStencilReference = 0x0U;

  xiiMemoryUtils::ZeroFillArray(m_pCommittedRenderTargets);
  m_pCommittedDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount     = 0U;

  m_uiSubpassIndex = 0U;
  m_pRenderPass    = nullptr;
  m_pFramebuffer   = nullptr;
  m_AttachmentClearValues.Clear();

  xiiMemoryUtils::ZeroFillArray(m_pBoundConstantBuffers);

  for (xiiUInt32 uiStage = 0U; uiStage < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStage)
  {
    m_CommittedShaders[uiStage]                  = nullptr;
    m_CommittedShaderModificationStates[uiStage] = false;

    m_BoundSamplerStatesRange[uiStage].Reset();

    for (xiiUInt32 i = 0; i < m_pBoundShaderResourceViews[uiStage].GetCount(); ++i)
    {
      m_pBoundShaderResourceViews[uiStage][i] = nullptr;
      m_ResourcesForResourceViews[uiStage][i] = nullptr;
    }
    m_BoundShaderResourceViewsRange[uiStage].Reset();

    xiiMemoryUtils::ZeroFillArray(m_pBoundSamplerStates[uiStage]);
    m_BoundSamplerStatesRange[uiStage].Reset();
  }

  for (xiiUInt32 i = 0; i < m_BoundUnorderedAccessViews.GetCount(); ++i)
  {
    m_BoundUnorderedAccessViews[i]        = nullptr;
    m_ResourcesForUnorderedAccessViews[i] = nullptr;
  }
  m_BoundUnorderedAccessViewsRange.Reset();

  m_pImmediateContext->ClearState();
}

void xiiGALCommandListD3D11::InvalidateCommittedResources()
{
  xiiMemoryUtils::ZeroFillArray(m_pBoundConstantBuffers);

  for (xiiUInt32 uiStage = 0U; uiStage < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStage)
  {
    if (m_CommittedShaders[uiStage] != nullptr)
    {
      m_CommittedShaders[uiStage]                  = nullptr;
      m_CommittedShaderModificationStates[uiStage] = true;
    }

    // This causes samplers to be unbound and not set, need to figure out why.
    for (xiiUInt32 i = 0; i < XII_GAL_MAX_SAMPLER_COUNT; ++i)
    {
      if (m_pBoundSamplerStates[uiStage][i] != nullptr)
      {
        m_pBoundSamplerStates[uiStage][i] = nullptr;

        m_BoundSamplerStatesRange[uiStage].SetToIncludeValue(i);
      }
    }

    for (xiiUInt32 i = 0; i < m_pBoundShaderResourceViews[uiStage].GetCount(); ++i)
    {
      m_pBoundShaderResourceViews[uiStage][i] = nullptr;
      m_ResourcesForResourceViews[uiStage][i] = nullptr;
      m_BoundShaderResourceViewsRange[uiStage].SetToIncludeValue(i);
    }
  }

  for (xiiUInt32 i = 0; i < m_BoundUnorderedAccessViews.GetCount(); ++i)
  {
    m_BoundUnorderedAccessViews[i]        = nullptr;
    m_ResourcesForUnorderedAccessViews[i] = nullptr;
    m_BoundUnorderedAccessViewsRange.SetToIncludeValue(i);
  }

  FlushDeferredStateChanges().IgnoreResult();
}

void xiiGALCommandListD3D11::CommitRenderTargets()
{
  if (!m_pRenderPass || !m_pFramebuffer)
    return;

  const auto& renderPassDescription  = m_pRenderPass->GetDescription();
  const auto& framebufferDescription = m_pFramebuffer->GetDescription();
  const auto& currentSubpass         = renderPassDescription.m_SubPasses[m_uiSubpassIndex];

  const xiiGALTextureViewD3D11* pAttachmentViews[XII_GAL_MAX_RENDERTARGET_COUNT] = {};
  const xiiGALTextureViewD3D11* pDepthStencilView                                = nullptr;
  const xiiUInt32               uiRenderTargetCount                              = currentSubpass.m_RenderTargetAttachments.GetCount();
  bool                          bFlushRequired                                   = false;

  // Unbind these attachments that will be used for output by the subpass.
  // There is no need to unbind textures from output as the new subpass attachments.
  // will be committed as render target/depth stencil anyway, so these that can be used for input will be unbound.

  for (xiiUInt32 i = 0; i < uiRenderTargetCount; ++i)
  {
    const auto& attachmentDescription = currentSubpass.m_RenderTargetAttachments[i];

    if (attachmentDescription.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
    {
      auto pRenderTargetView = framebufferDescription.m_Attachments[attachmentDescription.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D11>();

      XII_ASSERT_DEV((pRenderTargetView->GetDescription().m_ViewType == xiiGALTextureViewType::RenderTarget), "Expected xiiGALTextureViewType::RenderTarget at the subpass color attachment render target index.");

      bFlushRequired |= UnsetResourceViews(pRenderTargetView->GetTexture());
      bFlushRequired |= UnsetUnorderedAccessViews(pRenderTargetView->GetTexture());

      pAttachmentViews[i] = pRenderTargetView;
    }
    else
    {
      pAttachmentViews[i] = nullptr;
    }
  }

  if (!currentSubpass.m_DepthStencilAttachment.IsEmpty())
  {
    const auto& attachmentDescription = currentSubpass.m_DepthStencilAttachment.PeekBack();

    if (attachmentDescription.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
    {
      pDepthStencilView = framebufferDescription.m_Attachments[attachmentDescription.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D11>();

      XII_ASSERT_DEV((pDepthStencilView->GetDescription().m_ViewType == xiiGALTextureViewType::DepthStencil || pDepthStencilView->GetDescription().m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil), "Expected xiiGALTextureViewType::DepthStencil or xiiGALTextureViewType::ReadOnlyDepthStencil at the subpass depth attachment render target index.");

      bFlushRequired |= UnsetResourceViews(pDepthStencilView->GetTexture());
      bFlushRequired |= UnsetUnorderedAccessViews(pDepthStencilView->GetTexture());
    }
  }

  if (bFlushRequired)
  {
    FlushDeferredStateChanges().IgnoreResult();
  }

  xiiMemoryUtils::ZeroFillArray(m_pCommittedRenderTargets);
  m_pCommittedDepthStencilTarget = nullptr;

  if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
  {
    for (xiiUInt32 i = 0; i < uiRenderTargetCount; ++i)
    {
      if (pAttachmentViews[i] != nullptr)
      {
        m_pCommittedRenderTargets[i] = static_cast<ID3D11RenderTargetView*>(pAttachmentViews[i]->GetTextureView());
      }
    }

    if (pDepthStencilView != nullptr)
    {
      m_pCommittedDepthStencilTarget = static_cast<ID3D11DepthStencilView*>(pDepthStencilView->GetTextureView());
    }

    // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
    m_pImmediateContext->OMSetRenderTargets(xiiMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pCommittedRenderTargets, m_pCommittedDepthStencilTarget);

    m_uiBoundRenderTargetCount = uiRenderTargetCount;
  }
  else
  {
    m_pCommittedDepthStencilTarget = nullptr;
    m_uiBoundRenderTargetCount     = 0U;

    m_pImmediateContext->OMSetRenderTargets(0U, nullptr, nullptr);
  }

  // Clear render targets.
  for (xiiUInt32 i = 0; i < renderPassDescription.m_Attachments.GetCount(); ++i)
  {
    const auto& attachmentDescription = renderPassDescription.m_Attachments[i];
    const auto& pTextureView          = framebufferDescription.m_Attachments[i];

    if (attachmentDescription.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
    {
      if (pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::DepthStencil || pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil)
      {
        bool bClearStencil = attachmentDescription.m_StencilLoadOperation == xiiGALAttachmentLoadOperation::Clear;

        ClearDepthStencilViewPlatform(pTextureView, true, bClearStencil, m_AttachmentClearValues[i].m_DepthStencil.m_fDepth, m_AttachmentClearValues[i].m_DepthStencil.m_uiStencil);
      }
      else if (pTextureView->GetDescription().m_ViewType == xiiGALTextureViewType::RenderTarget)
      {
        ClearRenderTargetViewPlatform(pTextureView, m_AttachmentClearValues[i].m_ClearColor);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(xiiGALPipelineStateD3D11::ShaderType::Enum stage, ID3D11DeviceContext* pContext, xiiUInt32 uiStartSlot, xiiUInt32 uiSlotCount, ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case xiiGALPipelineStateD3D11::ShaderType::Vertex:
      pContext->VSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Hull:
      pContext->HSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Domain:
      pContext->DSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Geometry:
      pContext->GSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Pixel:
      pContext->PSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Compute:
      pContext->CSSetShaderResources(uiStartSlot, uiSlotCount, pShaderResourceViews);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(xiiGALPipelineStateD3D11::ShaderType::Enum stage, ID3D11DeviceContext* pContext, xiiUInt32 uiStartSlot, xiiUInt32 uiSlotCount, ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case xiiGALPipelineStateD3D11::ShaderType::Vertex:
      pContext->VSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Hull:
      pContext->HSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Domain:
      pContext->DSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Geometry:
      pContext->GSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Pixel:
      pContext->PSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Compute:
      pContext->CSSetConstantBuffers(uiStartSlot, uiSlotCount, pConstantBuffers);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(xiiGALPipelineStateD3D11::ShaderType::Enum stage, ID3D11DeviceContext* pContext, xiiUInt32 uiStartSlot, xiiUInt32 uiSlotCount, ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case xiiGALPipelineStateD3D11::ShaderType::Vertex:
      pContext->VSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Hull:
      pContext->HSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Domain:
      pContext->DSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Geometry:
      pContext->GSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Pixel:
      pContext->PSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;
    case xiiGALPipelineStateD3D11::ShaderType::Compute:
      pContext->CSSetSamplers(uiStartSlot, uiSlotCount, pSamplerStates);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

//////////////////////////////////////////////////////////////////////////

bool xiiGALCommandListD3D11::UnsetResourceViews(const xiiSharedPtr<xiiGALResource> pResource)
{
  XII_ASSERT_DEV(pResource != nullptr, "");

  bool bResult = false;

  for (xiiUInt32 uiStage = 0U; uiStage < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStage)
  {
    for (xiiUInt32 uiSlot = 0U; uiSlot < m_ResourcesForResourceViews[uiStage].GetCount(); ++uiSlot)
    {
      if (m_ResourcesForResourceViews[uiStage][uiSlot] == pResource)
      {
        m_ResourcesForResourceViews[uiStage][uiSlot] = nullptr;
        m_pBoundShaderResourceViews[uiStage][uiSlot] = nullptr;
        m_BoundShaderResourceViewsRange[uiStage].SetToIncludeValue(uiSlot);

        bResult = true;
      }
    }
  }
  return bResult;
}

bool xiiGALCommandListD3D11::UnsetUnorderedAccessViews(const xiiSharedPtr<xiiGALResource> pResource)
{
  XII_ASSERT_DEV(pResource != nullptr, "");

  bool bResult = false;

  for (xiiUInt32 uiSlot = 0U; uiSlot < m_ResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_ResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_ResourcesForUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnorderedAccessViews[uiSlot]        = nullptr;
      m_BoundUnorderedAccessViewsRange.SetToIncludeValue(uiSlot);

      bResult = true;
    }
  }
  return bResult;
}

xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> xiiGALCommandListD3D11::BeginDisjointQuery()
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  if (!m_pActiveDisjointQuery)
  {
    m_pActiveDisjointQuery = m_DisjointQueryPool.GetDisjointQuery(pDeviceD3D11->GetD3D11Device());

    // Disjoint timestamp queries should be only invoked once per frame or less.
    m_pImmediateContext->Begin(m_pActiveDisjointQuery->m_pQueryD3D11);

    m_pActiveDisjointQuery->m_bIsEnded = false;
  }
  return m_pActiveDisjointQuery;
}

xiiResult xiiGALCommandListD3D11::FlushDeferredStateChanges()
{
  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Vertex])
  {
    m_pImmediateContext->VSSetShader(static_cast<ID3D11VertexShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Vertex]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Vertex] = false;
  }

  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Pixel])
  {
    m_pImmediateContext->PSSetShader(static_cast<ID3D11PixelShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Pixel]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Pixel] = false;
  }

  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Compute])
  {
    m_pImmediateContext->CSSetShader(static_cast<ID3D11ComputeShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Compute]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Compute] = false;
  }

  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Domain])
  {
    m_pImmediateContext->DSSetShader(static_cast<ID3D11DomainShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Domain]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Domain] = false;
  }

  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Hull])
  {
    m_pImmediateContext->HSSetShader(static_cast<ID3D11HullShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Hull]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Hull] = false;
  }

  if (m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Geometry])
  {
    m_pImmediateContext->GSSetShader(static_cast<ID3D11GeometryShader*>(m_CommittedShaders[xiiGALPipelineStateD3D11::ShaderType::Geometry]), nullptr, 0);

    m_CommittedShaderModificationStates[xiiGALPipelineStateD3D11::ShaderType::Geometry] = false;
  }

  if (m_CommittedVertexBuffersRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_CommittedVertexBuffersRange.m_uiMin;
    const xiiUInt32 uiSlotCount = m_CommittedVertexBuffersRange.GetCount();

    m_pImmediateContext->IASetVertexBuffers(uiStartSlot, uiSlotCount, m_pCommittedVertexBuffers + uiStartSlot, m_CommittedVertexBufferStrides + uiStartSlot, m_CommittedVertexBufferOffsets + uiStartSlot);

    m_CommittedVertexBuffersRange.Reset();
  }

  if (m_bIndexBufferModified)
  {
    m_pImmediateContext->IASetIndexBuffer(m_pCommittedIndexBuffer, m_CommittedIndexBufferFormat, m_uiCommittedIndexDataStartOffset);

    m_bIndexBufferModified = false;
  }

  if (m_bInputLayoutStateModified)
  {
    m_pImmediateContext->IASetInputLayout(m_pCommittedInputLayout);

    m_bInputLayoutStateModified = false;
  }

  if (m_bRasterizerStateModified)
  {
    m_pImmediateContext->RSSetState(m_pCommittedRasterizerState);

    m_bRasterizerStateModified = false;
  }

  if (m_bBlendStateModified)
  {
    m_pImmediateContext->OMSetBlendState(m_pCommittedBlendState, m_CommittedBlendFactors.GetData(), m_uiCommittedBlendSampleMask);

    m_bBlendStateModified = false;
  }

  if (m_bDepthStencilStateModified)
  {
    m_pImmediateContext->OMSetDepthStencilState(m_pCommittedDepthStencilState, m_uiCommittedStencilReference);

    m_bDepthStencilStateModified = false;
  }

  if (m_bPrimitiveTopologyModified)
  {
    m_pImmediateContext->IASetPrimitiveTopology(m_CommittedPrimitiveTopology);

    m_bPrimitiveTopologyModified = false;
  }

  for (xiiUInt32 uiStageIndex = 0; uiStageIndex < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStageIndex)
  {
    if (m_CommittedShaders[uiStageIndex] != nullptr && m_BoundConstantBuffersRange[uiStageIndex].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundConstantBuffersRange[uiStageIndex].m_uiMin;
      const xiiUInt32 uiSlotCount = m_BoundConstantBuffersRange[uiStageIndex].GetCount();

      SetConstantBuffers((xiiGALPipelineStateD3D11::ShaderType::Enum)uiStageIndex, m_pImmediateContext, uiStartSlot, uiSlotCount, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[uiStageIndex].Reset();
    }
  }

  // Set Unordered Access Views (UAVs) before shader resource views (SRVs), since UAVs are outputs that need to be unbound before they need to be potentially rebound as SRVs.
  if (m_BoundUnorderedAccessViewsRange.IsValid())
  {
    const xiiUInt32 uiStartSlot = m_BoundUnorderedAccessViewsRange.m_uiMin;
    const xiiUInt32 uiSlotCount = m_BoundUnorderedAccessViewsRange.GetCount();

    m_pImmediateContext->CSSetUnorderedAccessViews(uiStartSlot, uiSlotCount, m_BoundUnorderedAccessViews.GetData() + uiStartSlot, nullptr); // Maybe consider unordered access views count reset.

    m_BoundUnorderedAccessViewsRange.Reset();
  }

  for (xiiUInt32 uiStageIndex = 0; uiStageIndex < xiiGALPipelineStateD3D11::ShaderType::ENUM_COUNT; ++uiStageIndex)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[uiStageIndex].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[uiStageIndex].m_uiMin;
      const xiiUInt32 uiSlotCount = m_BoundShaderResourceViewsRange[uiStageIndex].GetCount();

      SetShaderResources((xiiGALPipelineStateD3D11::ShaderType::Enum)uiStageIndex, m_pImmediateContext, uiStartSlot, uiSlotCount, m_pBoundShaderResourceViews[uiStageIndex].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[uiStageIndex].Reset();
    }

    if (m_BoundSamplerStatesRange[uiStageIndex].IsValid())
    {
      const xiiUInt32 uiStartSlot = m_BoundSamplerStatesRange[uiStageIndex].m_uiMin;
      const xiiUInt32 uiSlotCount = m_BoundSamplerStatesRange[uiStageIndex].GetCount();

      SetSamplers((xiiGALPipelineStateD3D11::ShaderType::Enum)uiStageIndex, m_pImmediateContext, uiStartSlot, uiSlotCount, m_pBoundSamplerStates[uiStageIndex] + uiStartSlot);

      m_BoundSamplerStatesRange[uiStageIndex].Reset();
    }
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandListD3D11);
