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
  m_pPipelineBarrier = m_GALDeviceDiligent.m_pPipelineBarrier.Borrow();
}

xiiGALCommandEncoderImplDiligent::~xiiGALCommandEncoderImplDiligent()
{
  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;

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

  for (auto& iter : m_CachedGraphicsPipelineStates)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pPipelineState);
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(iter.Value().m_pShaderResourceBinding);
  }
  m_CachedGraphicsPipelineStates.Clear();
  m_CachedGraphicsPipelineStates.Compact();

  for (auto& iter : m_CachedComputePipelineStates)
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
    m_bPipelineStateModified      = true;
  }
}

void xiiGALCommandEncoderImplDiligent::SetConstantBufferPlatform(xiiUInt32 uiSlot, const xiiGALBuffer* pBuffer)
{
  xiiGALBuffer* pGALBuffer = const_cast<xiiGALBuffer*>(pBuffer);

  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<xiiGALBufferDiligent*>(pGALBuffer) : nullptr;
  m_bDescriptorsModified          = true;
}

void xiiGALCommandEncoderImplDiligent::SetSamplerStatePlatform(xiiGALShaderStage::Enum Stage, xiiUInt32 uiSlot, const xiiGALSamplerState* pSamplerState)
{
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
  m_bDescriptorsModified           = true;
}

void xiiGALCommandEncoderImplDiligent::SetUnorderedAccessViewPlatform(xiiUInt32 uiSlot, const xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALUnorderedAccessView* pUAView = const_cast<xiiGALUnorderedAccessView*>(pUnorderedAccessView);

  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<xiiGALUnorderedAccessViewDiligent*>(pUAView) : nullptr;
  m_bDescriptorsModified              = true;
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

        const VkClearColorValue       clearColourValues{clearValues.x, clearValues.y, clearValues.z};
        const VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(pContextVk->GetVkCommandBuffer(), pTextureVk->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValues, 1, &subresourceRange);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTextureVk);
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
      Diligent::IDeviceContextVk* pContextVk = static_cast<Diligent::IDeviceContextVk*>(m_pContext);
      m_pContext->QueryInterface(Diligent::IID_DeviceContextVk, reinterpret_cast<Diligent::IObject**>(&pContextVk));
      XII_ASSERT_DEV(pContextVk != nullptr, "Failed to retrieve the Vulkan context.");

      if (Diligent::IBufferView* pBufferView = pUnorderedAccessViewDiligent->GetBufferView())
      {
        Diligent::IBufferVk* pBufferVk = static_cast<Diligent::IBufferVk*>(pBufferView->GetBuffer());
        pBufferView->GetBuffer()->QueryInterface(Diligent::IID_BufferVk, reinterpret_cast<Diligent::IObject**>(&pBufferVk));
        XII_ASSERT_DEV(pBufferVk != nullptr, "Failed to retrieve the Vulkan buffer view.");

        vkCmdFillBuffer(pContextVk->GetVkCommandBuffer(), pBufferVk->GetVkBuffer(), 0, VK_WHOLE_SIZE, clearValues.x);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pBufferVk);
      }

      if (Diligent::ITextureView* pTextureView = pUnorderedAccessViewDiligent->GetTextureView())
      {
        Diligent::ITextureVk* pTextureVk = static_cast<Diligent::ITextureVk*>(pTextureView->GetTexture());
        pTextureView->GetTexture()->QueryInterface(Diligent::IID_TextureVk, reinterpret_cast<Diligent::IObject**>(&pTextureVk));
        XII_ASSERT_DEV(pTextureVk != nullptr, "Failed to retrieve the Vulkan texture view.");

        const VkClearColorValue       clearColourValues{clearValues.x, clearValues.y, clearValues.z};
        const VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(pContextVk->GetVkCommandBuffer(), pTextureVk->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValues, 1, &subresourceRange);

        XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pTextureVk);
      }

      XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pContextVk);
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

  m_pPipelineBarrier->EnsureResourceState(m_pContext, pSourceBuffer, Diligent::RESOURCE_STATE_COPY_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pSourceBuffer));
  m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationBuffer, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationBuffer));

  m_pContext->CopyBuffer(pSourceBuffer, 0u, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, pDestinationBuffer, 0u, pDestination->GetSize(), Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
}

void xiiGALCommandEncoderImplDiligent::CopyBufferRegionPlatform(const xiiGALBuffer* pDestination, xiiUInt32 uiDestOffset, const xiiGALBuffer* pSource, xiiUInt32 uiSourceOffset, xiiUInt32 uiByteCount)
{
  xiiGALBuffer*      pSrc               = const_cast<xiiGALBuffer*>(pSource);
  xiiGALBuffer*      pDst               = const_cast<xiiGALBuffer*>(pDestination);
  Diligent::IBuffer* pDestinationBuffer = static_cast<xiiGALBufferDiligent*>(pDst)->GetBuffer();
  Diligent::IBuffer* pSourceBuffer      = static_cast<xiiGALBufferDiligent*>(pSrc)->GetBuffer();

  m_pPipelineBarrier->EnsureResourceState(m_pContext, pSourceBuffer, Diligent::RESOURCE_STATE_COPY_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pSourceBuffer));
  m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationBuffer, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationBuffer));

  m_pContext->CopyBuffer(pSourceBuffer, uiSourceOffset, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, pDestinationBuffer, uiDestOffset, uiByteCount, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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

  if (bufferDescription.BindFlags & Diligent::BIND_UNIFORM_BUFFER)
  {
    XII_ASSERT_DEV(uiDestOffset == 0 && pSourceData.GetCount() == pDestination->GetSize(), "Constant buffers cannot be mapped partially, there are no checks for partial constant buffer updates.");
  }

  switch (bufferDescription.Usage)
  {
    case Diligent::USAGE_DEFAULT:
    {
      m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationBuffer, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationBuffer));

      m_pContext->UpdateBuffer(pDestinationBuffer, uiDestOffset, pSourceData.GetCount(), reinterpret_cast<const void*>(pSourceData.GetPtr()), Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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

      XII_ASSERT_DEV(pDestinationBuffer->GetState() != Diligent::RESOURCE_STATE_COPY_DEST, "");
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderImplDiligent::CopyTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTexture* pSource)
{
  if (m_bRenderpassActive)
  {
    m_pContext->EndRenderPass();

    m_bRenderpassActive = false;
  }

  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  m_pPipelineBarrier->EnsureResourceState(m_pContext, pSourceTexture, Diligent::RESOURCE_STATE_COPY_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pSourceTexture));
  m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationTexture, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationTexture));

  Diligent::CopyTextureAttribs CopyTexAttribs = {};
  CopyTexAttribs.pSrcTexture                  = pSourceTexture;
  CopyTexAttribs.pDstTexture                  = pDestinationTexture;
  CopyTexAttribs.SrcTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
  CopyTexAttribs.DstTextureTransitionMode     = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->CopyTexture(CopyTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::CopyTextureRegionPlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiVec3U32& DestinationPoint, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource, const xiiBoundingBoxu32& Box)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  m_pPipelineBarrier->EnsureResourceState(m_pContext, pSourceTexture, Diligent::RESOURCE_STATE_COPY_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pSourceTexture));
  m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationTexture, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationTexture));

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
  CopyTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  CopyTexAttribs.DstMipLevel              = DestinationSubResource.m_uiMipLevel;
  CopyTexAttribs.DstSlice                 = DestinationSubResource.m_uiArraySlice;
  CopyTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
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

      m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationTexture, Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationTexture));

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

      m_pContext->UpdateTexture(pDestinationTexture, DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice, SubRegion, SubResData, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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

      XII_ASSERT_DEV(pDestinationTexture->GetState() != Diligent::RESOURCE_STATE_COPY_DEST, "");
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiGALCommandEncoderImplDiligent::ResolveTexturePlatform(const xiiGALTexture* pDestination, const xiiGALTextureSubresource& DestinationSubResource, const xiiGALTexture* pSource, const xiiGALTextureSubresource& SourceSubResource)
{
  xiiGALTexture*      pSrc                = const_cast<xiiGALTexture*>(pSource);
  xiiGALTexture*      pDst                = const_cast<xiiGALTexture*>(pDestination);
  Diligent::ITexture* pSourceTexture      = static_cast<xiiGALTextureDiligent*>(pSrc)->GetTexture();
  Diligent::ITexture* pDestinationTexture = static_cast<xiiGALTextureDiligent*>(pDst)->GetTexture();

  m_pPipelineBarrier->EnsureResourceState(m_pContext, pSourceTexture, Diligent::RESOURCE_STATE_RESOLVE_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pSourceTexture));
  m_pPipelineBarrier->EnsureResourceState(m_pContext, pDestinationTexture, Diligent::RESOURCE_STATE_RESOLVE_DEST, xiiDiligentUtils::GetDefaultResourceState(pDestinationTexture));

  Diligent::TEXTURE_FORMAT Format = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
  ResolveTexAttribs.Format = Format;

  ResolveTexAttribs.SrcMipLevel              = SourceSubResource.m_uiMipLevel;
  ResolveTexAttribs.SrcSlice                 = SourceSubResource.m_uiArraySlice;
  ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  ResolveTexAttribs.DstMipLevel              = DestinationSubResource.m_uiMipLevel;
  ResolveTexAttribs.DstSlice                 = DestinationSubResource.m_uiArraySlice;
  ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

  m_pContext->ResolveTextureSubresource(pSourceTexture, pDestinationTexture, ResolveTexAttribs);
}

void xiiGALCommandEncoderImplDiligent::ReadbackTexturePlatform(const xiiGALTexture* pTexture)
{
  if (m_bRenderpassActive)
  {
    m_pContext->EndRenderPass();

    m_bRenderpassActive = false;
  }

  xiiGALTexture*         pTex             = const_cast<xiiGALTexture*>(pTexture);
  xiiGALTextureDiligent* pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pTex);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pTextureDiligent->GetDescription().m_SampleCount != xiiGALMSAASampleCount::None;

  XII_ASSERT_DEV(pTextureDiligent->GetStagingTexture() != nullptr, "No staging resource available for read-back");
  XII_ASSERT_DEV(pTextureDiligent->GetTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    m_pPipelineBarrier->EnsureResourceState(m_pContext, pTextureDiligent->GetTexture(), Diligent::RESOURCE_STATE_RESOLVE_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pTextureDiligent->GetTexture()));
    m_pPipelineBarrier->EnsureResourceState(m_pContext, pTextureDiligent->GetStagingTexture(), Diligent::RESOURCE_STATE_RESOLVE_DEST, xiiDiligentUtils::GetDefaultResourceState(pTextureDiligent->GetStagingTexture()));

    Diligent::ResolveTextureSubresourceAttribs ResolveTexAttribs;
    ResolveTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
    ResolveTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

    m_pContext->ResolveTextureSubresource(pTextureDiligent->GetTexture(), pTextureDiligent->GetStagingTexture(), ResolveTexAttribs);
  }
  else
  {
    m_pPipelineBarrier->EnsureResourceState(m_pContext, pTextureDiligent->GetTexture(), Diligent::RESOURCE_STATE_COPY_SOURCE, xiiDiligentUtils::GetDefaultResourceState(pTextureDiligent->GetTexture()));
    m_pPipelineBarrier->EnsureResourceState(m_pContext, pTextureDiligent->GetStagingTexture(), Diligent::RESOURCE_STATE_COPY_DEST, xiiDiligentUtils::GetDefaultResourceState(pTextureDiligent->GetStagingTexture()));

    Diligent::CopyTextureAttribs CopyTexAttribs;
    CopyTexAttribs.pSrcTexture              = pTextureDiligent->GetTexture();
    CopyTexAttribs.pDstTexture              = pTextureDiligent->GetStagingTexture();
    CopyTexAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
    CopyTexAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

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
  if (m_bRenderpassActive)
  {
    m_pContext->EndRenderPass();

    m_bRenderpassActive = false;
  }

  xiiGALResourceView*         pResource             = const_cast<xiiGALResourceView*>(pResourceView);
  xiiGALResourceViewDiligent* pResourceViewDiligent = static_cast<xiiGALResourceViewDiligent*>(pResource);

  m_pContext->GenerateMips(pResourceViewDiligent->GetTextureView());
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
  m_RenderingSetup = renderingSetup;

  m_pRenderPass  = m_GALDeviceDiligent.m_pDefaultPass->RequestRenderPass(renderingSetup);
  m_pFramebuffer = m_GALDeviceDiligent.m_pDefaultPass->RequestFrameBuffer(m_pRenderPass, renderingSetup.m_RenderTargetSetup);

  m_ClearValues.Clear();

  const bool     bHasDepthAttachment    = !m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt8 uiColorAttachmentCount = m_RenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  m_bClearSubmitted = !(renderingSetup.m_bClearDepth || renderingSetup.m_bClearStencil || renderingSetup.m_uiRenderTargetClearMask);

  if (bHasDepthAttachment)
  {
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::OptimizedClearValue& depthClear = m_ClearValues.ExpandAndGetRef();
    depthClear.SetDepthStencil(formatInfo.m_eDepthStencilType, 1.0f, 0);

    m_pPipelineBarrier->EnsureResourceState(m_pContext, const_cast<xiiGALTextureDiligent*>(pTextureDiligent)->GetTexture(), Diligent::RESOURCE_STATE_DEPTH_WRITE, Diligent::RESOURCE_STATE_DEPTH_WRITE, true, true);
  }

  for (xiiUInt8 i = 0; i < uiColorAttachmentCount; ++i)
  {
    xiiGALRenderTargetViewHandle          hColorRenderTarget        = m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
    const xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<const xiiGALRenderTargetViewDiligent*>(m_GALDeviceDiligent.GetRenderTargetView(hColorRenderTarget));

    xiiGALTextureHandle          hTexture         = pRenderTargetViewDiligent->GetDescription().m_hTexture;
    const xiiGALTextureDiligent* pTextureDiligent = static_cast<const xiiGALTextureDiligent*>(m_GALDeviceDiligent.GetTexture(hTexture)->GetParentResource());

    const xiiGALTextureCreationDescription& textureDescription = pTextureDiligent->GetDescription();
    xiiEnum<xiiGALResourceFormat>           format             = textureDescription.m_Format;
    const auto&                             formatInfo         = m_GALDeviceDiligent.GetFormatLookupTable().GetFormatInfo(format);

    Diligent::OptimizedClearValue& colorClear = m_ClearValues.ExpandAndGetRef();
    colorClear.SetColor(formatInfo.m_eRenderTarget, m_RenderingSetup.m_ClearColor.GetData());

    m_pPipelineBarrier->EnsureResourceState(m_pContext, const_cast<xiiGALTextureDiligent*>(pTextureDiligent)->GetTexture(), Diligent::RESOURCE_STATE_RENDER_TARGET, Diligent::RESOURCE_STATE_RENDER_TARGET, true, true);
  }

  m_pPipelineBarrier->FlushBarriers();
}

void xiiGALCommandEncoderImplDiligent::EndRendering()
{
  if (m_bRenderpassActive)
  {
    m_pContext->EndRenderPass();

    m_bRenderpassActive = false;
    m_bClearSubmitted   = false;
  }

  m_pRenderPass  = nullptr;
  m_pFramebuffer = nullptr;
}

// Draw functions

void xiiGALCommandEncoderImplDiligent::ClearPlatform(const xiiColor& ClearColor, xiiUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  // Render target clears not are while the Renderpass is active are not supported in D3D12
  if (!m_bIsComputeRequested && !m_bRenderpassActive && m_GALDeviceDiligent.GetCapabilities().m_DeviceType != xiiGraphicsDeviceType::D3D12)
  {
    Diligent::BeginRenderPassAttribs renderPassBeginInfo;
    renderPassBeginInfo.pRenderPass         = m_pRenderPass;
    renderPassBeginInfo.pFramebuffer        = m_pFramebuffer;
    renderPassBeginInfo.pClearValues        = m_ClearValues.GetData();
    renderPassBeginInfo.ClearValueCount     = m_ClearValues.GetCount();
    renderPassBeginInfo.StateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

    m_pContext->BeginRenderPass(renderPassBeginInfo);

    m_bRenderpassActive = true;
    m_bClearSubmitted   = true;
  }

  const bool     bHasDepthAttachment    = !m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt8 uiColorAttachmentCount = m_RenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  if (uiRenderTargetClearMask != 0)
  {
    for (xiiUInt8 i = 0; i < uiColorAttachmentCount; ++i)
    {
      if (uiRenderTargetClearMask & XII_BIT(i))
      {
        xiiGALRenderTargetView*         pGALRenderTargetView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetRenderTarget(i)));
        xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALRenderTargetView);

        m_pContext->ClearRenderTarget(pRenderTargetViewDiligent->GetRenderTargetView(), ClearColor.GetData(), Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
      }
    }
  }

  if (bHasDepthAttachment && (bClearDepth || bClearStencil))
  {
    xiiGALRenderTargetView*         pGALDepthStencilView      = const_cast<xiiGALRenderTargetView*>(m_GALDeviceDiligent.GetRenderTargetView(m_RenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
    xiiGALRenderTargetViewDiligent* pRenderTargetViewDiligent = static_cast<xiiGALRenderTargetViewDiligent*>(pGALDepthStencilView);

    Diligent::CLEAR_DEPTH_STENCIL_FLAGS flags = {};

    if (bClearDepth)
      flags |= Diligent::CLEAR_DEPTH_FLAG;

    if (bClearStencil)
      flags |= Diligent::CLEAR_STENCIL_FLAG;

    m_pContext->ClearDepthStencil(pRenderTargetViewDiligent->GetDepthStencilView(), flags, fDepthClear, uiStencilClear, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
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
  FlushDeferredStateChanges();

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
  FlushDeferredStateChanges();

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
  /// \todo RendererDiligent: Implement uiStenciValue

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
  m_RenderingSetup = xiiGALRenderingSetup();
}

void xiiGALCommandEncoderImplDiligent::EndCompute()
{
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
  DispatchAttribs.AttribsBufferStateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;
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

void xiiGALCommandEncoderImplDiligent::FlushDeferredStateChanges()
{
  TransitionResources();

  if (m_bRenderpassActive && m_pPipelineBarrier->IsBarrierModified())
  {
    m_pContext->EndRenderPass();

    m_bRenderpassActive = false;
  }

  if (!m_bRenderpassActive)
  {
    m_pPipelineBarrier->FlushBarriers();
  }

  if (m_bPipelineStateModified)
  {
    if (!m_pCurrentShader)
    {
      xiiLog::Error("No shader set in pipeline");
      return;
    }

    // Pipeline may not be begun without a vertex shader.
    if (!m_pCurrentShader->GetVertexShader())
      return;

    // Pipeline information for either the Graphics or Compute pipeline state.
    PipelineStateInfo pipelineInfo;
    {
      if (m_bIsComputeRequested)
      {
        Diligent::ComputePipelineStateCreateInfo computePipelineStateDesc;
        computePipelineStateDesc.PSODesc.PipelineType    = Diligent::PIPELINE_TYPE_COMPUTE;
        computePipelineStateDesc.pCS                     = m_pCurrentShader->GetComputeShader();
        computePipelineStateDesc.ppResourceSignatures    = m_pCurrentShader->GetPipelineResourceSignatures();
        computePipelineStateDesc.ResourceSignaturesCount = m_pCurrentShader->GetPipelineResourceSignatureCount();

        if (!m_CachedComputePipelineStates.TryGetValue(computePipelineStateDesc, pipelineInfo))
        {
          m_GALDeviceDiligent.GetDevice()->CreatePipelineState(computePipelineStateDesc, &pipelineInfo.m_pPipelineState);

          XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new Compute pipeline state object.");

          (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

          m_CachedComputePipelineStates.Insert(computePipelineStateDesc, pipelineInfo);

          xiiLog::Dev("Created new Compute pipeline state object.");
        }
        else
        {
          XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pipelineInfo.m_pShaderResourceBinding);
          (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);
        }
      }
      else
      {
        Diligent::GraphicsPipelineStateCreateInfo graphicsPipelineStateDesc;
        graphicsPipelineStateDesc.pVS                     = m_pCurrentShader->GetVertexShader();
        graphicsPipelineStateDesc.pPS                     = m_pCurrentShader->GetPixelShader();
        graphicsPipelineStateDesc.pDS                     = m_pCurrentShader->GetDomainShader();
        graphicsPipelineStateDesc.pHS                     = m_pCurrentShader->GetHullShader();
        graphicsPipelineStateDesc.pGS                     = m_pCurrentShader->GetGeometryShader();
        graphicsPipelineStateDesc.pAS                     = m_pCurrentShader->GetAmplificationShader();
        graphicsPipelineStateDesc.pMS                     = m_pCurrentShader->GetMeshShader();
        graphicsPipelineStateDesc.ppResourceSignatures    = m_pCurrentShader->GetPipelineResourceSignatures();
        graphicsPipelineStateDesc.ResourceSignaturesCount = m_pCurrentShader->GetPipelineResourceSignatureCount();

        Diligent::GraphicsPipelineDesc& graphicsPipelineDesc = graphicsPipelineStateDesc.GraphicsPipeline;
        graphicsPipelineDesc.pRenderPass                     = m_pRenderPass;
        graphicsPipelineDesc.PrimitiveTopology               = m_PrimitiveTopology;
        graphicsPipelineDesc.NumViewports                    = 1u;

        if (m_pVertexDeclaration)
          graphicsPipelineDesc.InputLayout = *m_pVertexDeclaration->GetInputLayoutDesc();
        if (m_pBlendStateState)
          graphicsPipelineDesc.BlendDesc = *m_pBlendStateState->GetBlendStateDesc();
        if (m_pDepthStencilState)
          graphicsPipelineDesc.DepthStencilDesc = *m_pDepthStencilState->GetDepthStencilStateDesc();
        if (m_pRasterizerState)
          graphicsPipelineDesc.RasterizerDesc = *m_pRasterizerState->GetRasterizerStateDesc();

        if (!m_CachedGraphicsPipelineStates.TryGetValue(graphicsPipelineStateDesc, pipelineInfo))
        {
          m_GALDeviceDiligent.GetDevice()->CreatePipelineState(graphicsPipelineStateDesc, &pipelineInfo.m_pPipelineState);

          XII_ASSERT_DEV(pipelineInfo.m_pPipelineState != nullptr, "Failed to create new Graphics pipeline state object.");

          (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);

          m_CachedGraphicsPipelineStates.Insert(graphicsPipelineStateDesc, pipelineInfo);

          xiiLog::Dev("Created new Graphics pipeline state object.");
        }
        else
        {
          XII_GAL_DILIGENT_UNWRAPPED_RELEASE(pipelineInfo.m_pShaderResourceBinding);
          (*m_pCurrentShader->GetPipelineResourceSignatures())->CreateShaderResourceBinding(&pipelineInfo.m_pShaderResourceBinding, true);
        }
      }
    }

    FillShaderDescriptorBindings(pipelineInfo.m_pShaderResourceBinding);

    m_pCurrentPipelineState         = pipelineInfo.m_pPipelineState;
    m_pCurrentShaderResourceBinding = pipelineInfo.m_pShaderResourceBinding;

    m_pContext->SetPipelineState(pipelineInfo.m_pPipelineState);
    m_pContext->CommitShaderResources(pipelineInfo.m_pShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);
  }

  if (!m_bIsComputeRequested && m_bViewportModified)
  {
    m_pContext->SetViewports(1u, &m_Viewport, static_cast<xiiUInt32>(m_Viewport.Width), static_cast<xiiUInt32>(m_Viewport.Height));

    if (m_bScissorEnabled)
    {
      m_pContext->SetScissorRects(1u, &m_ScissorRect, m_ScissorRect.right - m_ScissorRect.left, m_ScissorRect.bottom - m_ScissorRect.top);
    }
    else
    {
      Diligent::Rect ViewRectNoScissor;
      ViewRectNoScissor.left   = (xiiUInt32)m_Viewport.TopLeftX;
      ViewRectNoScissor.top    = (xiiUInt32)m_Viewport.TopLeftY;
      ViewRectNoScissor.right  = (xiiUInt32)m_Viewport.Width;
      ViewRectNoScissor.bottom = (xiiUInt32)m_Viewport.Height;

      m_pContext->SetScissorRects(1u, &ViewRectNoScissor, ViewRectNoScissor.right - ViewRectNoScissor.left, ViewRectNoScissor.bottom - ViewRectNoScissor.top);
    }

    m_bViewportModified = false;
  }

  if (!m_bIsComputeRequested && m_BoundVertexBuffersRange.IsValid())
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

  if (!m_bIsComputeRequested && m_bIndexBufferModified && m_pIndexBuffer != nullptr)
  {
    m_pContext->SetIndexBuffer(m_pIndexBuffer->GetBuffer(), 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    m_bIndexBufferModified = false;
  }

  if (!m_bIsComputeRequested && !m_bRenderpassActive)
  {
    Diligent::BeginRenderPassAttribs renderPassBeginInfo;
    renderPassBeginInfo.pRenderPass         = m_pRenderPass;
    renderPassBeginInfo.pFramebuffer        = m_pFramebuffer;
    renderPassBeginInfo.pClearValues        = m_ClearValues.GetData();
    renderPassBeginInfo.ClearValueCount     = m_ClearValues.GetCount();
    renderPassBeginInfo.StateTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_VERIFY;

    m_pContext->BeginRenderPass(renderPassBeginInfo);

    m_bRenderpassActive = true;
    m_bClearSubmitted   = true;
  }
}

void xiiGALCommandEncoderImplDiligent::TransitionResources()
{
  // Use deferred state flushes as the renderpass might be active when this is called.

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    if (m_pBoundVertexBuffers[i] == nullptr)
      continue;

    m_pPipelineBarrier->EnsureResourceState(m_pContext, m_pBoundVertexBuffers[i], Diligent::RESOURCE_STATE_VERTEX_BUFFER, xiiDiligentUtils::GetDefaultResourceState(m_pBoundVertexBuffers[i]), m_bRenderpassActive);
  }

  if (m_pIndexBuffer != nullptr)
  {
    m_pPipelineBarrier->EnsureResourceState(m_pContext, m_pIndexBuffer->GetBuffer(), Diligent::RESOURCE_STATE_INDEX_BUFFER, xiiDiligentUtils::GetDefaultResourceState(m_pIndexBuffer->GetBuffer()), m_bRenderpassActive);
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
              auto pBufferDiligent = m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer();

              m_pPipelineBarrier->EnsureResourceState(m_pContext, pBufferDiligent, Diligent::RESOURCE_STATE_CONSTANT_BUFFER, xiiDiligentUtils::GetDefaultResourceState(pBufferDiligent), m_bRenderpassActive);
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceViewBuffer:
            {
              auto& description  = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetDescription();
              auto  pSRVDiligent = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetBufferView();

              m_pPipelineBarrier->EnsureResourceState(m_pContext, pSRVDiligent->GetBuffer(), Diligent::RESOURCE_STATE_SHADER_RESOURCE, xiiDiligentUtils::GetDefaultResourceState(pSRVDiligent->GetBuffer()), m_bRenderpassActive);
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceViewTexture:
            {
              auto& description  = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetDescription();
              auto  pSRVDiligent = m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetTextureView();

              m_pPipelineBarrier->EnsureResourceState(m_pContext, pSRVDiligent->GetTexture(), Diligent::RESOURCE_STATE_SHADER_RESOURCE, xiiDiligentUtils::GetDefaultResourceState(pSRVDiligent->GetTexture()), m_bRenderpassActive);
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::UnorderedAccessViewBuffer:
            {
              auto& description  = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetDescription();
              auto  pUAVDiligent = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetBufferView();

              m_pPipelineBarrier->EnsureResourceState(m_pContext, pUAVDiligent->GetBuffer(), Diligent::RESOURCE_STATE_SHADER_RESOURCE, xiiDiligentUtils::GetDefaultResourceState(pUAVDiligent->GetBuffer()), m_bRenderpassActive);
            }
            break;
            case xiiShaderDescriptorSetLayoutBinding::UnorderedAccessViewTexture:
            {
              auto& description  = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetDescription();
              auto  pUAVDiligent = m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetTextureView();

              m_pPipelineBarrier->EnsureResourceState(m_pContext, pUAVDiligent->GetTexture(), Diligent::RESOURCE_STATE_SHADER_RESOURCE, xiiDiligentUtils::GetDefaultResourceState(pUAVDiligent->GetTexture()), m_bRenderpassActive);
            }
            break;
          }
        }
      }
    }
  }
}

void xiiGALCommandEncoderImplDiligent::FillShaderDescriptorBindings(Diligent::IShaderResourceBinding* pResourceBinding)
{
  // Note that this function does not check if the bindings have been modified.

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
              pConstantBuffer->Set(static_cast<Diligent::IBuffer*>(m_pBoundConstantBuffers[currentBinding.m_uiVirtualBinding]->GetBuffer()), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pConstantBuffer->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
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
              pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
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
              pResourceView->Set(m_pBoundShaderResourceViews[stage][currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
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
              pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetTextureView(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pUnorderedResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
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
              pUnorderedResourceView->Set(m_pBoundUnoderedAccessViews[currentBinding.m_uiVirtualBinding]->GetBufferView(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pUnorderedResourceView->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
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
              pSampler->Set(m_pBoundSamplerStates[stage][currentBinding.m_uiVirtualBinding]->GetSamplerState(), Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            else
              pSampler->Set(nullptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
          }
          break;
        }
      }
    }
  }
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_CommandEncoder_Implementation_CommandEncoderImplDiligent);
