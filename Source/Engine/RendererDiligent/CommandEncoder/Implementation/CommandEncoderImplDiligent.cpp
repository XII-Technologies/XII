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

#undef NULL
#define NULL 0

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
#  include <d3d11.h>

#  include <Graphics/GraphicsEngineD3D11/interface/BufferViewD3D11.h>
#  include <Graphics/GraphicsEngineD3D11/interface/DeviceContextD3D11.h>
#  include <Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h>
#  include <Graphics/GraphicsEngineD3D11/interface/TextureViewD3D11.h>
#endif

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT
#  include <d3d12.h>

#  include <Graphics/GraphicsEngineD3D12/interface/BufferViewD3D12.h>
#  include <Graphics/GraphicsEngineD3D12/interface/DeviceContextD3D12.h>
#  include <Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h>
#  include <Graphics/GraphicsEngineD3D12/interface/TextureViewD3D12.h>
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
#  include <vulkan/vulkan.hpp>

// Some of the functionality we need has moved from vulkan.hpp to vulkan_format_traits.hpp in later versions of the vulkan SDK.
#  if __has_include(<vulkan/vulkan_format_traits.hpp>)
#    include <vulkan/vulkan_format_traits.hpp>
#  endif

#  include <Graphics/GraphicsEngineVulkan/interface/BufferViewVk.h>
#  include <Graphics/GraphicsEngineVulkan/interface/BufferVk.h>
#  include <Graphics/GraphicsEngineVulkan/interface/DeviceContextVk.h>
#  include <Graphics/GraphicsEngineVulkan/interface/RenderDeviceVk.h>
#  include <Graphics/GraphicsEngineVulkan/interface/TextureViewVk.h>
#  include <Graphics/GraphicsEngineVulkan/interface/TextureVk.h>
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALRenderTargetViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const xiiUInt32&>(Value);
    return Stream;
  }
} // namespace

xiiUInt32 xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Hash(const xiiGALRenderingSetup& renderingSetup)
{
  xiiHashStreamWriter32 writer;

  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }

  writer << renderingSetup.m_ClearColor;
  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_fDepthClear;
  writer << renderingSetup.m_uiStencilClear;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;

  return writer.GetHashValue();
}

bool xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b)
{
  return a == b;
}

xiiUInt32 xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Hash(const Diligent::GraphicsPipelineStateCreateInfo& desc)
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

bool xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Equal(const Diligent::GraphicsPipelineStateCreateInfo& a, const Diligent::GraphicsPipelineStateCreateInfo& b)
{
  return a == b;
}

xiiUInt32 xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Hash(const Diligent::ComputePipelineStateCreateInfo& desc)
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

bool xiiGALCommandEncoderImplDiligent::ResourceCacheHash::Equal(const Diligent::ComputePipelineStateCreateInfo& a, const Diligent::ComputePipelineStateCreateInfo& b)
{
  return a == b;
}

xiiGALCommandEncoderImplDiligent::xiiGALCommandEncoderImplDiligent(xiiGALDeviceDiligent& deviceDiligent) :
  m_GALDeviceDiligent(deviceDiligent), m_pContext(m_GALDeviceDiligent.GetImmediateContext())
{
}

xiiGALCommandEncoderImplDiligent::~xiiGALCommandEncoderImplDiligent()
{
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

  for (auto iter : m_CachedGraphicsPipelineStates)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pPipelineState);
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pShaderResourceBinding);
  }
  m_CachedGraphicsPipelineStates.Clear();
  m_CachedGraphicsPipelineStates.Compact();

  for (auto iter : m_CachedComputePipelineStates)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pPipelineState);
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pShaderResourceBinding);
  }
  m_CachedComputePipelineStates.Clear();
  m_CachedComputePipelineStates.Compact();
}

// State setting functions

void xiiGALCommandEncoderImplDiligent::SetShaderPlatform(const xiiGALShader* pShader)
{
  if (m_pCurrentShader != pShader)
  {
    xiiGALShader* pShaderNonConst = const_cast<xiiGALShader*>(pShader);
    m_pCurrentShader              = pShader != nullptr ? static_cast<xiiGALShaderDiligent*>(pShaderNonConst) : nullptr;
  }
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALBuffer* pGALBuffer = const_cast<xiiGALBuffer*>(pBuffer);

  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pGALBuffer) : nullptr;
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  xiiGALSamplerState* pGALSampler = const_cast<xiiGALSamplerState*>(pSamplerState);

  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<xiiGALSamplerStateDiligent*>(pGALSampler) : nullptr;
}

void xiiGALCommandEncoderImplDiligent::SetResourceViewPlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);

  xiiGALResourceView* pResource    = const_cast<xiiGALResourceView*>(pResourceView);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<xiiGALResourceViewDiligent*>(pResource) : nullptr;
}

void xiiGALCommandEncoderImplDiligent::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessView* pUAView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);

  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView) : nullptr;
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
  {
    xiiGALUnorderedAccessView* pGALUnorderedAccessView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);
    pUnorderedAccessViewDiligent                       = static_cast<xiiGALUnorderedAccessViewDiligent*>(pGALUnorderedAccessView);
  }

  switch (m_GALDeviceDiligent.GetDeviceType())
  {
#if D3D11_SUPPORTED
    case Diligent::RENDER_DEVICE_TYPE_D3D11:
    {
      Diligent::IDeviceContextD3D11* pContextD3D11 = static_cast<Diligent::IDeviceContextD3D11*>(m_pContext);
      m_pContext->QueryInterface(Diligent::IID_DeviceContextD3D11, reinterpret_cast<Diligent::IObject**>(&pContextD3D11));
      XII_ASSERT_DEV(pContextD3D11 != nullptr, "Failed to retrieve the D3D11 context.");

      if (Diligent::IBufferView* pBufferView = pUnorderedAccessViewDiligent->GetBufferView())
      {
        Diligent::IBufferViewD3D11* pBufferViewD3D11 = static_cast<Diligent::IBufferViewD3D11*>(pBufferView);
        pBufferView->QueryInterface(Diligent::IID_BufferViewD3D11, reinterpret_cast<Diligent::IObject**>(&pBufferViewD3D11));
        XII_ASSERT_DEV(pBufferViewD3D11 != nullptr, "Failed to retrieve the D3D11 buffer view.");

        pContextD3D11->GetD3D11DeviceContext()->ClearUnorderedAccessViewFloat(static_cast<ID3D11UnorderedAccessView*>(pBufferViewD3D11->GetD3D11View()), &clearValues.x);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBufferViewD3D11);
      }

      if (Diligent::ITextureView* pTextureView = pUnorderedAccessViewDiligent->GetTextureView())
      {
        Diligent::ITextureViewD3D11* pTextureViewD3D11 = static_cast<Diligent::ITextureViewD3D11*>(pTextureView);
        pTextureView->QueryInterface(Diligent::IID_TextureViewD3D11, reinterpret_cast<Diligent::IObject**>(&pTextureViewD3D11));
        XII_ASSERT_DEV(pTextureViewD3D11 != nullptr, "Failed to retrieve the D3D11 texture view.");

        pContextD3D11->GetD3D11DeviceContext()->ClearUnorderedAccessViewFloat(static_cast<ID3D11UnorderedAccessView*>(pTextureViewD3D11->GetD3D11View()), &clearValues.x);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTextureViewD3D11);
      }

      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pContextD3D11);
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

      Diligent::IDeviceContextVk* pContextVk = static_cast<Diligent::IDeviceContextVk*>(m_pContext);
      m_pContext->QueryInterface(Diligent::IID_DeviceContextVk, reinterpret_cast<Diligent::IObject**>(&pContextVk));
      XII_ASSERT_DEV(pContextVk != nullptr, "Failed to retrieve the Vulkan context.");

      if (Diligent::IBufferView* pBufferView = pUnorderedAccessViewDiligent->GetBufferView())
      {
        Diligent::IBufferVk* pBufferVk = static_cast<Diligent::IBufferVk*>(pBufferView->GetBuffer());
        pBufferView->GetBuffer()->QueryInterface(Diligent::IID_BufferVk, reinterpret_cast<Diligent::IObject**>(&pBufferVk));
        XII_ASSERT_DEV(pBufferVk != nullptr, "Failed to retrieve the Vulkan buffer view.");

        xiiUInt32* pData = (xiiUInt32*)&clearValues;
        vkCmdFillBuffer(pContextVk->GetVkCommandBuffer(), pBufferVk->GetVkBuffer(), 0, VK_WHOLE_SIZE, *pData);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBufferVk);
      }

      if (Diligent::ITextureView* pTextureView = pUnorderedAccessViewDiligent->GetTextureView())
      {
        Diligent::ITextureVk* pTextureVk = static_cast<Diligent::ITextureVk*>(pTextureView->GetTexture());
        pTextureView->GetTexture()->QueryInterface(Diligent::IID_TextureVk, reinterpret_cast<Diligent::IObject**>(&pTextureVk));
        XII_ASSERT_DEV(pTextureVk != nullptr, "Failed to retrieve the Vulkan texture view.");

        const bool bIsDepthFormat = xiiDiligentUtils::IsDepthFormat();
        ;
        vk::ImageSubresourceRange range;
        range.aspectMask     = GetAspectMask();
        range.baseArrayLayer = 0;
        range.baseMipLevel   = 0;
        range.layerCount     = m_Description.m_Type == ezGALTextureType::TextureCube ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
        range.levelCount     = m_Description.m_uiMipLevelCount;

        const VkClearColorValue clearColourValues{clearValues.x, clearValues.y, clearValues.z};
        vkCmdClearColorImage(pContextVk->GetVkCommandBuffer(), pTextureVk->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValues, 1, pTextureVk->getsi);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASvkCmdFillBufferE(pTextureViewVk);
      }

      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pContextVk);
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
      Diligent::IDeviceContextD3D11* pContextD3D11 = static_cast<Diligent::IDeviceContextD3D11*>(m_pContext);
      m_pContext->QueryInterface(Diligent::IID_DeviceContextD3D11, reinterpret_cast<Diligent::IObject**>(&pContextD3D11));
      XII_ASSERT_DEV(pContextD3D11 != nullptr, "Failed to retrieve the D3D11 context.");

      if (Diligent::IBufferView* pBufferView = pUnorderedAccessViewDiligent->GetBufferView())
      {
        Diligent::IBufferViewD3D11* pBufferViewD3D11 = static_cast<Diligent::IBufferViewD3D11*>(pBufferView);
        pBufferView->QueryInterface(Diligent::IID_BufferViewD3D11, reinterpret_cast<Diligent::IObject**>(&pBufferViewD3D11));
        XII_ASSERT_DEV(pBufferViewD3D11 != nullptr, "Failed to retrieve the D3D11 buffer view.");

        pContextD3D11->GetD3D11DeviceContext()->ClearUnorderedAccessViewUint(static_cast<ID3D11UnorderedAccessView*>(pBufferViewD3D11->GetD3D11View()), &clearValues.x);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBufferViewD3D11);
      }

      if (Diligent::ITextureView* pTextureView = pUnorderedAccessViewDiligent->GetTextureView())
      {
        Diligent::ITextureViewD3D11* pTextureViewD3D11 = static_cast<Diligent::ITextureViewD3D11*>(pTextureView);
        pTextureView->QueryInterface(Diligent::IID_TextureViewD3D11, reinterpret_cast<Diligent::IObject**>(&pTextureViewD3D11));
        XII_ASSERT_DEV(pTextureViewD3D11 != nullptr, "Failed to retrieve the D3D11 texture view.");

        pContextD3D11->GetD3D11DeviceContext()->ClearUnorderedAccessViewUint(static_cast<ID3D11UnorderedAccessView*>(pTextureViewD3D11->GetD3D11View()), &clearValues.x);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTextureViewD3D11);
      }

      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pContextD3D11);
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

  m_pContext->CopyBuffer(pSourceBuffer, 0u, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, pDestinationBuffer, 0u, pDestination->GetSize(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
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
    mapFlags |= Diligent::MAP_FLAG_DISCARD;

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
        std::memcpy(xiiMemoryUtils::AddByteOffset((xiiUInt8*)pMapResult, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

        m_pContext->UnmapBuffer(pDestinationBuffer, Diligent::MAP_WRITE);
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
        std::memcpy(MapResult.pData, pSourceData.m_pData, uiSlicePitch * uiDepth);
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
            std::memcpy(pDest, pSource, uiRowPitch);

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

        std::memcpy(memDesc.m_pData, MappedSubRes.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const xiiUInt32 uiHeight = GetMipSize(pTextureDiligent->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = xiiMemoryUtils::AddByteOffset(MappedSubRes.pData, y * MappedSubRes.Stride);
          void*       pDest   = xiiMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          std::memcpy(pDest, pSource, xiiGALResourceFormat::GetBitsPerElement(pTextureDiligent->GetDescription().m_Format) * GetMipSize(pTextureDiligent->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
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
  FlushDeferredStateChangesGraphics();
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

    const xiiUInt8 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    for (xiiUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const xiiGALRenderTargetView* pRenderTargetView = m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));

      pRenderTargetViews[uiIndex] = const_cast<xiiGALRenderTargetView*>(pRenderTargetView);
    }

    pDepthStencilView = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget()));
    if (pDepthStencilView != nullptr)
    {
      const xiiGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();
    }

    for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
      m_RTVFormats[i]          = Diligent::TEX_FORMAT_UNKNOWN;
    }

    m_pBoundDepthStencilTarget = nullptr;
    m_DSVFormat                = Diligent::TEX_FORMAT_UNKNOWN;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (xiiUInt32 i = 0; i < uiRenderTargetCount; ++i)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<xiiGALRenderTargetViewDiligent*>(pRenderTargetViews[i])->GetRenderTargetView();
          m_RTVFormats[i]          = m_pBoundRenderTargets[i]->GetDesc().Format;
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<xiiGALRenderTargetViewDiligent*>(pDepthStencilView)->GetDepthStencilView();
        m_DSVFormat                = m_pBoundDepthStencilTarget->GetDesc().Format;
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pContext->SetRenderTargets(xiiMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
      {
        m_pBoundRenderTargets[i] = nullptr;
        m_RTVFormats[i]          = Diligent::TEX_FORMAT_UNKNOWN;
      }

      m_uiBoundRenderTargetCount = 0;

      m_pBoundDepthStencilTarget = nullptr;
      m_DSVFormat                = Diligent::TEX_FORMAT_UNKNOWN;

      m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
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
  for (xiiUInt32 i = 0; i < m_uiBoundRenderTargetCount; ++i)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pContext->ClearRenderTarget(m_pBoundRenderTargets[i], ClearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
  }

  if (m_pBoundDepthStencilTarget && (bClearDepth || bClearStencil))
  {
    xiiUInt32 uiClearFlags = bClearDepth ? Diligent::CLEAR_DEPTH_FLAG : 0u;
    uiClearFlags |= bClearStencil ? Diligent::CLEAR_STENCIL_FLAG : 0u;

    m_pContext->ClearDepthStencil(m_pBoundDepthStencilTarget, (Diligent::CLEAR_DEPTH_STENCIL_FLAGS)uiClearFlags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
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
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

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
  drawAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  drawAttribs.pCounterBuffer                   = nullptr;
  drawAttribs.CounterOffset                    = 0;
  drawAttribs.CounterBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

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
    m_pIndexBuffer = pIndexBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pGALIndexBuffer) : nullptr;
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
    }
  }
}

void xiiGALCommandEncoderImplDiligent::SetVertexDeclarationPlatform(const xiiGALVertexDeclaration* pVertexDeclaration)
{
  if (m_pVertexDeclaration != pVertexDeclaration)
  {
    m_pVertexDeclaration = static_cast<const xiiGALVertexDeclarationDiligent*>(pVertexDeclaration);
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
    m_PrimitiveTopology = GALTopologyToDiligent[Topology];
  }
}

void xiiGALCommandEncoderImplDiligent::SetBlendStatePlatform(const xiiGALBlendState* pBlendState, const xiiColor& BlendFactor, xiiUInt32 uiSampleMask)
{
  const float BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pContext->SetBlendFactors(BlendFactors);

  if (m_pBlendStateState != pBlendState)
  {
    m_pBlendStateState = pBlendState != nullptr ? static_cast<const xiiGALBlendStateDiligent*>(pBlendState) : nullptr;
  }
}

void xiiGALCommandEncoderImplDiligent::SetDepthStencilStatePlatform(const xiiGALDepthStencilState* pDepthStencilState, xiiUInt8 uiStencilRefValue)
{
  /// \todo RendererDiligent: Implement uiStenciValue

  if (m_pDepthStencilState != pDepthStencilState)
  {
    m_pDepthStencilState = pDepthStencilState != nullptr ? static_cast<const xiiGALDepthStencilStateDiligent*>(pDepthStencilState) : nullptr;
  }
}

void xiiGALCommandEncoderImplDiligent::SetRasterizerStatePlatform(const xiiGALRasterizerState* pRasterizerState)
{
  if (m_pRasterizerState != pRasterizerState)
  {
    m_pRasterizerState = pRasterizerState != nullptr ? static_cast<const xiiGALRasterizerStateDiligent*>(pRasterizerState) : nullptr;
  }
}

void xiiGALCommandEncoderImplDiligent::SetViewportPlatform(const xiiRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  Diligent::Viewport viewport;
  viewport.TopLeftX = rect.x;
  viewport.TopLeftY = rect.y;
  viewport.Width    = rect.width;
  viewport.Height   = rect.height;
  viewport.MinDepth = fMinDepth;
  viewport.MaxDepth = fMaxDepth;

  m_pContext->SetViewports(1, &viewport, static_cast<xiiUInt32>(rect.width), static_cast<xiiUInt32>(rect.height));
}

void xiiGALCommandEncoderImplDiligent::SetScissorRectPlatform(const xiiRectU32& rect)
{
  Diligent::Rect scissorRect;
  scissorRect.left   = rect.x;
  scissorRect.top    = rect.y;
  scissorRect.right  = rect.x + rect.width;
  scissorRect.bottom = rect.y + rect.height;

  m_pContext->SetScissorRects(1, &scissorRect, rect.width, rect.height);
}

void xiiGALCommandEncoderImplDiligent::SetStreamOutBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer, xiiUInt32 uiOffset)
{
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::BeginCompute()
{
  m_RenderTargetSetup = xiiGALRenderTargetSetup();
  m_pContext->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
}

void xiiGALCommandEncoderImplDiligent::EndCompute()
{
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
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
  DispatchAttribs.DispatchArgsByteOffset           = uiArgumentOffsetInBytes;

  // These are only needed in a Metal backend.
  DispatchAttribs.MtlThreadGroupSizeX = 0;
  DispatchAttribs.MtlThreadGroupSizeY = 0;
  DispatchAttribs.MtlThreadGroupSizeZ = 0;

  m_pContext->DispatchComputeIndirect(DispatchAttribs);
}

//////////////////////////////////////////////////////////////////////////

void xiiGALCommandEncoderImplDiligent::FlushPipelineStateCache()
{
  for (auto iter : m_CachedGraphicsPipelineStates)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pPipelineState);
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pShaderResourceBinding);
  }
  m_CachedGraphicsPipelineStates.Clear();
  m_CachedGraphicsPipelineStates.Compact();

  for (auto iter : m_CachedComputePipelineStates)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pPipelineState);
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pShaderResourceBinding);
  }
  m_CachedComputePipelineStates.Clear();
  m_CachedComputePipelineStates.Compact();
}

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChangesCompute()
{
  if (!m_pCurrentShader)
  {
    xiiLog::Error("No shader set in pipeline");
    return;
  }

  Diligent::ComputePipelineStateCreateInfo computePipelineStateDesc;
  computePipelineStateDesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_COMPUTE;
  computePipelineStateDesc.pCS                  = m_pCurrentShader->GetComputeShader();

  computePipelineStateDesc.ppResourceSignatures    = m_pCurrentShader->GetPipelineResourceSignatures();
  computePipelineStateDesc.ResourceSignaturesCount = m_pCurrentShader->GetPipelineResourceSignatureCount();

  auto pCachedPipelineStateComputeKey = m_CachedComputePipelineStates.Find(computePipelineStateDesc);

  // Create a new pipeline state if an existing one is not found, else use an existing one and update the data in it.
  if (!pCachedPipelineStateComputeKey.IsValid())
  {
    PipelineStateInfo pipelineInfo;
    m_GALDeviceDiligent.GetDevice()->CreatePipelineState(computePipelineStateDesc, &pipelineInfo.m_pPipelineState);

    XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new compute pipeline state object.");

    (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

    FillShaderDescriptorBindings(pipelineInfo.m_pShaderResourceBinding);

    m_pContext->SetPipelineState(pipelineInfo.m_pPipelineState);
    m_pContext->CommitShaderResources(pipelineInfo.m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    PipelineStateInfo oldPipelineInfo;
    if (m_CachedComputePipelineStates.Insert(computePipelineStateDesc, pipelineInfo, &oldPipelineInfo))
    {
      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(oldPipelineInfo.m_pPipelineState);
      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(oldPipelineInfo.m_pShaderResourceBinding);

      xiiLog::Dev("Released an existing Compute pipeline state object from cache.");
    }

    xiiLog::Dev("Created new Compute pipeline state object.");
  }
  else
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pCachedPipelineStateComputeKey.Value().m_pShaderResourceBinding);
    (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pCachedPipelineStateComputeKey.Value().m_pShaderResourceBinding, true);

    FillShaderDescriptorBindings(pCachedPipelineStateComputeKey.Value().m_pShaderResourceBinding);

    m_pContext->SetPipelineState(pCachedPipelineStateComputeKey.Value().m_pPipelineState);
    m_pContext->CommitShaderResources(pCachedPipelineStateComputeKey.Value().m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }

  // Set the amount of render targets. This must be rebound every render call.
  m_pContext->SetRenderTargets(m_uiBoundRenderTargetCount, m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChangesGraphics()
{
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
          m_pContext->SetVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        }
        uiCurrentStartSlot = i + 1;
      }
    }

    // The last element in the buffer range must always be valid so we can simply flush the rest.
    if (m_pBoundVertexBuffers[uiCurrentStartSlot])
    {
      m_pContext->SetVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
    }

    m_BoundVertexBuffersRange.Reset();
  }

  Diligent::IBuffer* pIndexBuffer = (m_pIndexBuffer != nullptr) ? m_pIndexBuffer->GetBuffer() : nullptr;
  m_pContext->SetIndexBuffer(pIndexBuffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  if (!m_pCurrentShader)
  {
    xiiLog::Error("No shader set in pipeline");
    m_pContext->SetPipelineState(nullptr);
    return;
  }

  if (!m_pCurrentShader->GetVertexShader())
    return;

  Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDesc;

  graphicsPipelineStateDesc.pVS = m_pCurrentShader->GetVertexShader();
  graphicsPipelineStateDesc.pPS = m_pCurrentShader->GetPixelShader();
  graphicsPipelineStateDesc.pDS = m_pCurrentShader->GetDomainShader();
  graphicsPipelineStateDesc.pHS = m_pCurrentShader->GetHullShader();
  graphicsPipelineStateDesc.pGS = m_pCurrentShader->GetGeometryShader();
  graphicsPipelineStateDesc.pAS = m_pCurrentShader->GetAmplificationShader();
  graphicsPipelineStateDesc.pMS = m_pCurrentShader->GetMeshShader();

  graphicsPipelineStateDesc.ppResourceSignatures    = m_pCurrentShader->GetPipelineResourceSignatures();
  graphicsPipelineStateDesc.ResourceSignaturesCount = m_pCurrentShader->GetPipelineResourceSignatureCount();

  graphicsPipelineStateDesc.GraphicsPipeline.PrimitiveTopology = m_PrimitiveTopology;
  graphicsPipelineStateDesc.GraphicsPipeline.NumViewports      = 1;

  if (m_pVertexDeclaration)
    graphicsPipelineStateDesc.GraphicsPipeline.InputLayout = *m_pVertexDeclaration->GetInputLayoutDesc();

  if (m_pBlendStateState)
    graphicsPipelineStateDesc.GraphicsPipeline.BlendDesc = *m_pBlendStateState->GetBlendStateDesc();

  if (m_pDepthStencilState)
    graphicsPipelineStateDesc.GraphicsPipeline.DepthStencilDesc = *m_pDepthStencilState->GetDepthStencilStateDesc();

  if (m_pRasterizerState)
    graphicsPipelineStateDesc.GraphicsPipeline.RasterizerDesc = *m_pRasterizerState->GetRasterizerStateDesc();

  graphicsPipelineStateDesc.GraphicsPipeline.NumRenderTargets = m_uiBoundRenderTargetCount;
  for (xiiUInt8 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    graphicsPipelineStateDesc.GraphicsPipeline.RTVFormats[i] = m_RTVFormats[i];
  }

  graphicsPipelineStateDesc.GraphicsPipeline.DSVFormat = m_DSVFormat;

  auto pCachedPipelineStateGraphicsKey = m_CachedGraphicsPipelineStates.Find(graphicsPipelineStateDesc);

  // Create a new pipeline state if an existing one is not found, else use an existing one and update the data in it.
  if (!pCachedPipelineStateGraphicsKey.IsValid())
  {
    PipelineStateInfo pipelineInfo;
    m_GALDeviceDiligent.GetDevice()->CreatePipelineState(graphicsPipelineStateDesc, &pipelineInfo.m_pPipelineState);

    XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new graphics pipeline state object.");

    (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

    FillShaderDescriptorBindings(pipelineInfo.m_pShaderResourceBinding);

    m_pContext->SetPipelineState(pipelineInfo.m_pPipelineState);
    m_pContext->CommitShaderResources(pipelineInfo.m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    PipelineStateInfo oldPipelineInfo;
    if (m_CachedGraphicsPipelineStates.Insert(graphicsPipelineStateDesc, pipelineInfo, &oldPipelineInfo))
    {
      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(oldPipelineInfo.m_pPipelineState);
      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(oldPipelineInfo.m_pShaderResourceBinding);

      xiiLog::Dev("Released an existing Graphics pipeline state object from cache.");
    }

    xiiLog::Dev("Created new Graphics pipeline state object.");
  }
  else
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pCachedPipelineStateGraphicsKey.Value().m_pShaderResourceBinding);
    (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pCachedPipelineStateGraphicsKey.Value().m_pShaderResourceBinding, true);

    FillShaderDescriptorBindings(pCachedPipelineStateGraphicsKey.Value().m_pShaderResourceBinding);

    m_pContext->SetPipelineState(pCachedPipelineStateGraphicsKey.Value().m_pPipelineState);
    m_pContext->CommitShaderResources(pCachedPipelineStateGraphicsKey.Value().m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }

  // Set the amount of render targets. This must be rebound every render call.
  m_pContext->SetRenderTargets(m_uiBoundRenderTargetCount, m_pBoundRenderTargets, m_pBoundDepthStencilTarget, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void xiiGALCommandEncoderImplDiligent::FillPipelineDescriptorBindings(Diligent::IPipelineState* pPipelineState)
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
              xiiLog::Error("Constant buffer pointer for '{}' returned null.", sData);
              continue;
            }
            pConstantBuffer->Set(static_cast<Diligent::IBuffer*>(m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer()), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewTexture:
          {
            auto* pResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pResourceView == nullptr)
            {
              xiiLog::Error("Resource view pointer for '{}' returned null.", sData);
              continue;
            }
            pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewBuffer:
          {
            auto* pResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pResourceView == nullptr)
            {
              xiiLog::Error("Resource view pointer for '{}' returned null.", sData);
              continue;
            }
            pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewTexture:
          {
            auto* pUnorderedResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pUnorderedResourceView == nullptr)
            {
              xiiLog::Error("Unordered access view pointer for '{}' returned null.", sData);
              continue;
            }
            pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewBuffer:
          {
            auto* pUnorderedResourceView = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pUnorderedResourceView == nullptr)
            {
              xiiLog::Error("Unordered access view pointer for '{}' returned null.", sData);
              continue;
            }
            pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::Sampler:
          {
            auto* pSampler = pPipelineState->GetStaticVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pSampler == nullptr)
            {
              xiiLog::Error("Sampler pointer for '{}' returned null.", sData);
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

void xiiGALCommandEncoderImplDiligent::FillShaderDescriptorBindings(Diligent::IShaderResourceBinding* pResourceBinding)
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
            auto* pConstantBuffer = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pConstantBuffer == nullptr)
            {
              xiiLog::Error("Constant buffer pointer for '{}' returned null.", sData);
              continue;
            }
            if (m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding])
              pConstantBuffer->Set(static_cast<Diligent::IBuffer*>(m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer()), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pConstantBuffer->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewTexture:
          {
            auto* pResourceView = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pResourceView == nullptr)
            {
              xiiLog::Error("Resource view pointer for '{}' returned null.", sData);
              continue;
            }
            if (!m_pBoundShaderResourceViews[stage].IsEmpty() && m_pBoundShaderResourceViews[stage].GetCount() > currentBinding.m_uiVirtualBinding && m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding])
              pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewBuffer:
          {
            auto* pResourceView = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pResourceView == nullptr)
            {
              xiiLog::Error("Resource view pointer for '{}' returned null.", sData);
              continue;
            }
            if (!m_pBoundShaderResourceViews[stage].IsEmpty() && m_pBoundShaderResourceViews[stage].GetCount() > currentBinding.m_uiVirtualBinding && m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding])
              pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewTexture:
          {
            auto* pUnorderedResourceView = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pUnorderedResourceView == nullptr)
            {
              xiiLog::Error("Unordered access view pointer for '{}' returned null.", sData);
              continue;
            }
            if (!m_pBoundUnoderedAccessViews.IsEmpty() && m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding])
              pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pUnorderedResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewBuffer:
          {
            auto* pUnorderedResourceView = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pUnorderedResourceView == nullptr)
            {
              xiiLog::Error("Unordered access view pointer for '{}' returned null.", sData);
              continue;
            }
            if (!m_pBoundUnoderedAccessViews.IsEmpty() && m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding])
              pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pUnorderedResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::Sampler:
          {
            auto* pSampler = pResourceBinding->GetVariableByName(xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)stage), sData);
            if (pSampler == nullptr)
            {
              xiiLog::Error("Sampler pointer for '{}' returned null.", sData);
              continue;
            }
            if (m_pBoundSamplerStates[stage] && m_pBoundSamplerStates[stage][currentBinding.m_uiVirtualBinding])
              pSampler->Set(m_pBoundSamplerStates[stage][currentBinding.m_uiVirtualBinding]->GetSamplerState(), Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
            else
              pSampler->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_NONE);
          }
          break;
        }
      }
    }
  }
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_CommandEncoder_Implementation_CommandEncoderImplDiligent);
