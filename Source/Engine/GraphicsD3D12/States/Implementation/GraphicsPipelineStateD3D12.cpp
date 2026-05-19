/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Shader/InputLayoutD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/BlendStateD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>
#include <GraphicsD3D12/States/GraphicsPipelineStateD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

namespace
{
  static constexpr xiiUInt32 s_uiGraphicsPipelineMaxRootConstantsDWORDs = 64U;

  void SetDefaultRasterizerState(D3D12_RASTERIZER_DESC& rasterizerState)
  {
    rasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
    rasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
    rasterizerState.FrontCounterClockwise = FALSE;
    rasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerState.DepthClipEnable       = TRUE;
    rasterizerState.MultisampleEnable     = FALSE;
    rasterizerState.AntialiasedLineEnable = FALSE;
    rasterizerState.ForcedSampleCount     = 0U;
    rasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  }

  void SetDefaultBlendState(D3D12_BLEND_DESC& blendState)
  {
    blendState.AlphaToCoverageEnable  = FALSE;
    blendState.IndependentBlendEnable = FALSE;

    for (D3D12_RENDER_TARGET_BLEND_DESC& renderTargetBlendState : blendState.RenderTarget)
    {
      renderTargetBlendState.BlendEnable           = FALSE;
      renderTargetBlendState.LogicOpEnable         = FALSE;
      renderTargetBlendState.SrcBlend              = D3D12_BLEND_ONE;
      renderTargetBlendState.DestBlend             = D3D12_BLEND_ZERO;
      renderTargetBlendState.BlendOp               = D3D12_BLEND_OP_ADD;
      renderTargetBlendState.SrcBlendAlpha         = D3D12_BLEND_ONE;
      renderTargetBlendState.DestBlendAlpha        = D3D12_BLEND_ZERO;
      renderTargetBlendState.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
      renderTargetBlendState.LogicOp               = D3D12_LOGIC_OP_NOOP;
      renderTargetBlendState.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
  }

  void SetDefaultDepthStencilState(D3D12_DEPTH_STENCIL_DESC& depthStencilState)
  {
    depthStencilState.DepthEnable                  = TRUE;
    depthStencilState.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilState.DepthFunc                    = D3D12_COMPARISON_FUNC_LESS;
    depthStencilState.StencilEnable                = FALSE;
    depthStencilState.StencilReadMask              = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilState.StencilWriteMask             = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStencilState.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
    depthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilState.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
    depthStencilState.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStencilState.BackFace.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
    depthStencilState.BackFace.StencilDepthFailOp  = D3D12_STENCIL_OP_KEEP;
    depthStencilState.BackFace.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
    depthStencilState.BackFace.StencilFunc         = D3D12_COMPARISON_FUNC_ALWAYS;
  }

  [[nodiscard]] bool TryFindImmutableSamplerBindingForGraphicsPipelineRootSignature(const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, const xiiGALImmutableSamplerDescription& immutableSampler, xiiUInt32& out_uiBindSlot, xiiUInt32& out_uiBindSet)
  {
    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      if (resource.m_sName == immutableSampler.m_SamplerOrTextureName &&
          (resource.m_ResourceType == xiiGALShaderResourceType::Sampler || resource.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler))
      {
        out_uiBindSlot = resource.m_uiBindSlot;
        out_uiBindSet  = resource.m_uiBindSet;
        return true;
      }
    }

    return false;
  }

  [[nodiscard]] xiiResult CreateGraphicsPipelineRootSignatureFromDescription(ID3D12Device* pD3D12Device, const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags, ID3D12RootSignature*& out_pRootSignature)
  {
    out_pRootSignature = nullptr;

    if (pD3D12Device == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 root signature: D3D12 device is unavailable.");
      return XII_FAILURE;
    }

    xiiUInt32 uiDescriptorTableCount = 0U;
    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      if (xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
      {
        ++uiDescriptorTableCount;
      }
    }

    xiiDynamicArray<D3D12_DESCRIPTOR_RANGE> descriptorRanges;
    xiiDynamicArray<D3D12_ROOT_PARAMETER>   rootParameters;
    descriptorRanges.Reserve(uiDescriptorTableCount);
    rootParameters.Reserve(uiDescriptorTableCount + signatureDescription.m_PushConstantRanges.GetCount());

    for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
    {
      D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      if (!xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
        continue;

      D3D12_DESCRIPTOR_RANGE& descriptorRange           = descriptorRanges.ExpandAndGetRef();
      descriptorRange.RangeType                         = rangeType;
      descriptorRange.NumDescriptors                    = xiiMath::Max(1U, resource.m_uiArraySize);
      descriptorRange.BaseShaderRegister                = resource.m_uiBindSlot;
      descriptorRange.RegisterSpace                     = resource.m_uiBindSet;
      descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_PARAMETER& rootParameter               = rootParameters.ExpandAndGetRef();
      rootParameter.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rootParameter.ShaderVisibility                    = xiiD3D12TypeConversions::GetShaderVisibility(resource.m_ShaderStages);
      rootParameter.DescriptorTable.NumDescriptorRanges = 1U;
      rootParameter.DescriptorTable.pDescriptorRanges   = &descriptorRange;
    }

    for (xiiUInt32 i = 0U; i < signatureDescription.m_PushConstantRanges.GetCount(); ++i)
    {
      const xiiGALPushConstantRange& pushConstantRange = signatureDescription.m_PushConstantRanges[i];
      if (pushConstantRange.m_uiSize == 0U)
        continue;

      const xiiUInt32 uiValueCount = (pushConstantRange.m_uiSize + 3U) / 4U;
      if (uiValueCount > s_uiGraphicsPipelineMaxRootConstantsDWORDs)
      {
        xiiLog::Error("Failed to create D3D12 root signature: push constant range {} has {} DWORDs, exceeding D3D12 limit of {} DWORDs.", i, uiValueCount, s_uiGraphicsPipelineMaxRootConstantsDWORDs);
        return XII_FAILURE;
      }

      D3D12_ROOT_PARAMETER& rootParameter    = rootParameters.ExpandAndGetRef();
      rootParameter.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      rootParameter.ShaderVisibility         = xiiD3D12TypeConversions::GetShaderVisibility(pushConstantRange.m_ShaderStages);
      rootParameter.Constants.ShaderRegister = i;
      rootParameter.Constants.RegisterSpace  = 0U;
      rootParameter.Constants.Num32BitValues = uiValueCount;
    }

    xiiDynamicArray<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    staticSamplers.Reserve(signatureDescription.m_ImmutableSamplers.GetCount());

    for (const xiiGALImmutableSamplerDescription& immutableSampler : signatureDescription.m_ImmutableSamplers)
    {
      xiiUInt32 uiBindSlot = 0U;
      xiiUInt32 uiBindSet  = 0U;
      if (!TryFindImmutableSamplerBindingForGraphicsPipelineRootSignature(signatureDescription, immutableSampler, uiBindSlot, uiBindSet))
      {
        xiiLog::Error("Failed to create D3D12 root signature: immutable sampler '{}' does not match any sampler resource binding.", immutableSampler.m_SamplerOrTextureName);
        return XII_FAILURE;
      }

      const xiiGALSamplerCreationDescription& samplerDescription = immutableSampler.m_SamplerDescription;

      D3D12_STATIC_SAMPLER_DESC& staticSampler = staticSamplers.ExpandAndGetRef();
      staticSampler.Filter                     = xiiD3D12TypeConversions::GetFilter(samplerDescription.m_MinFilter, samplerDescription.m_MagFilter, samplerDescription.m_MipFilter);
      staticSampler.AddressU                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressU);
      staticSampler.AddressV                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressV);
      staticSampler.AddressW                   = xiiD3D12TypeConversions::GetTextureAddressMode(samplerDescription.m_AddressW);
      staticSampler.MipLODBias                 = samplerDescription.m_fMipLODBias;
      staticSampler.MaxAnisotropy              = samplerDescription.m_uiMaxAnisotropy;
      staticSampler.ComparisonFunc             = xiiD3D12TypeConversions::GetComparisonFunc(samplerDescription.m_ComparisonFunction);
      staticSampler.BorderColor                = xiiD3D12TypeConversions::GetStaticBorderColor(samplerDescription.m_BorderColor);
      staticSampler.MinLOD                     = samplerDescription.m_fMinLOD;
      staticSampler.MaxLOD                     = samplerDescription.m_fMaxLOD;
      staticSampler.ShaderRegister             = uiBindSlot;
      staticSampler.RegisterSpace              = uiBindSet;
      staticSampler.ShaderVisibility           = xiiD3D12TypeConversions::GetShaderVisibility(immutableSampler.m_ShaderStages);
    }

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
    rootSignatureDescription.NumParameters             = rootParameters.GetCount();
    rootSignatureDescription.pParameters               = rootParameters.GetData();
    rootSignatureDescription.NumStaticSamplers         = staticSamplers.GetCount();
    rootSignatureDescription.pStaticSamplers           = staticSamplers.GetData();
    rootSignatureDescription.Flags                     = rootSignatureFlags;

    ID3DBlob* pSerializedBlob = nullptr;
    ID3DBlob* pErrorBlob      = nullptr;

    const HRESULT hSerializeResult = D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedBlob, &pErrorBlob);
    if (FAILED(hSerializeResult))
    {
      if (pErrorBlob != nullptr)
      {
        xiiLog::Error("Failed to serialize D3D12 root signature: {}.", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
      }
      else
      {
        xiiLog::Error("Failed to serialize D3D12 root signature: {}.", xiiHRESULTtoString(hSerializeResult));
      }

      XII_GAL_D3D12_RELEASE(pSerializedBlob);
      XII_GAL_D3D12_RELEASE(pErrorBlob);
      return XII_FAILURE;
    }

    const HRESULT hCreateResult = pD3D12Device->CreateRootSignature(0U, pSerializedBlob->GetBufferPointer(), pSerializedBlob->GetBufferSize(), IID_PPV_ARGS(&out_pRootSignature));

    XII_GAL_D3D12_RELEASE(pSerializedBlob);
    XII_GAL_D3D12_RELEASE(pErrorBlob);

    if (FAILED(hCreateResult) || out_pRootSignature == nullptr)
    {
      xiiLog::Error("Failed to create D3D12 root signature: {}.", xiiHRESULTtoString(hCreateResult));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }
} // namespace

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALGraphicsPipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALGraphicsPipelineStateD3D12::xiiGALGraphicsPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription) :
  xiiGALGraphicsPipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALGraphicsPipelineStateD3D12::~xiiGALGraphicsPipelineStateD3D12()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr)
  {
    XII_GAL_D3D12_RELEASE(m_pD3D12PipelineState);
    XII_GAL_D3D12_RELEASE(m_pD3D12RootSignature);
    return;
  }

  if (m_pD3D12PipelineState != nullptr)
  {
    IUnknown* pObject = m_pD3D12PipelineState;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12PipelineState = nullptr;
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    IUnknown* pObject = m_pD3D12RootSignature;
    pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    m_pD3D12RootSignature = nullptr;
  }
}

xiiResult xiiGALGraphicsPipelineStateD3D12::InitPlatform()
{
  if (m_Description.m_PipelineType == xiiGALPipelineType::Mesh)
  {
    xiiLog::Error("Mesh pipeline creation is not yet supported in GraphicsD3D12.");
    return XII_FAILURE;
  }

  if (m_Description.m_GraphicsPipeline.m_pRenderPass == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': render pass is missing.", GetDebugName());
    return XII_FAILURE;
  }

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12Device*                   pD3D12Device = pDeviceD3D12 != nullptr ? pDeviceD3D12->GetD3D12Device() : nullptr;
  if (pD3D12Device == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': D3D12 device is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  if (CreateGraphicsPipelineRootSignatureFromDescription(pD3D12Device, m_Description.m_pPipelineResourceSignature->GetDescription(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, m_pD3D12RootSignature).Failed())
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': root signature creation failed.", GetDebugName());
    return XII_FAILURE;
  }

  m_D3D12PrimitiveTopology     = xiiD3D12TypeConversions::GetPrimitiveTopology(m_Description.m_GraphicsPipeline.m_PrimitiveTopology);
  m_D3D12PrimitiveTopologyType = xiiD3D12TypeConversions::GetPrimitiveTopologyType(m_Description.m_GraphicsPipeline.m_PrimitiveTopology);
  if (m_D3D12PrimitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED || m_D3D12PrimitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED)
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': unsupported primitive topology '{}'.", GetDebugName(), m_Description.m_GraphicsPipeline.m_PrimitiveTopology.GetValue());
    return XII_FAILURE;
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescription = {};
  pipelineStateDescription.pRootSignature                     = m_pD3D12RootSignature;
  pipelineStateDescription.SampleMask                         = m_Description.m_GraphicsPipeline.m_uiSampleMask;
  pipelineStateDescription.PrimitiveTopologyType              = m_D3D12PrimitiveTopologyType;

  SetDefaultRasterizerState(pipelineStateDescription.RasterizerState);
  SetDefaultBlendState(pipelineStateDescription.BlendState);
  SetDefaultDepthStencilState(pipelineStateDescription.DepthStencilState);

  if (xiiSharedPtr<xiiGALShaderD3D12> pVertexShaderD3D12 = m_Description.m_pVertexShader.Downcast<xiiGALShaderD3D12>())
  {
    pipelineStateDescription.VS = *pVertexShaderD3D12->GetD3D12ShaderByteCodeDescription();
  }
  if (xiiSharedPtr<xiiGALShaderD3D12> pPixelShaderD3D12 = m_Description.m_pPixelShader.Downcast<xiiGALShaderD3D12>())
  {
    pipelineStateDescription.PS = *pPixelShaderD3D12->GetD3D12ShaderByteCodeDescription();
  }
  if (xiiSharedPtr<xiiGALShaderD3D12> pGeometryShaderD3D12 = m_Description.m_pGeometryShader.Downcast<xiiGALShaderD3D12>())
  {
    pipelineStateDescription.GS = *pGeometryShaderD3D12->GetD3D12ShaderByteCodeDescription();
  }
  if (xiiSharedPtr<xiiGALShaderD3D12> pHullShaderD3D12 = m_Description.m_pHullShader.Downcast<xiiGALShaderD3D12>())
  {
    pipelineStateDescription.HS = *pHullShaderD3D12->GetD3D12ShaderByteCodeDescription();
  }
  if (xiiSharedPtr<xiiGALShaderD3D12> pDomainShaderD3D12 = m_Description.m_pDomainShader.Downcast<xiiGALShaderD3D12>())
  {
    pipelineStateDescription.DS = *pDomainShaderD3D12->GetD3D12ShaderByteCodeDescription();
  }

  if (xiiSharedPtr<xiiGALInputLayoutD3D12> pInputLayoutD3D12 = m_Description.m_GraphicsPipeline.m_pInputLayout.Downcast<xiiGALInputLayoutD3D12>())
  {
    const xiiArrayPtr<const D3D12_INPUT_ELEMENT_DESC> inputElements = pInputLayoutD3D12->GetD3D12InputLayoutElements();
    pipelineStateDescription.InputLayout.NumElements                = inputElements.GetCount();
    pipelineStateDescription.InputLayout.pInputElementDescs         = inputElements.GetPtr();
  }

  if (xiiSharedPtr<xiiGALRasterizerStateD3D12> pRasterizerStateD3D12 = m_Description.m_GraphicsPipeline.m_pRasterizerState.Downcast<xiiGALRasterizerStateD3D12>())
  {
    pipelineStateDescription.RasterizerState = *pRasterizerStateD3D12->GetRasterizerState();
  }

  if (xiiSharedPtr<xiiGALBlendStateD3D12> pBlendStateD3D12 = m_Description.m_GraphicsPipeline.m_pBlendState.Downcast<xiiGALBlendStateD3D12>())
  {
    pipelineStateDescription.BlendState = *pBlendStateD3D12->GetBlendState();
  }

  if (xiiSharedPtr<xiiGALDepthStencilStateD3D12> pDepthStencilStateD3D12 = m_Description.m_GraphicsPipeline.m_pDepthStencilState.Downcast<xiiGALDepthStencilStateD3D12>())
  {
    pipelineStateDescription.DepthStencilState = *pDepthStencilStateD3D12->GetDepthStencilState();
  }

  if (m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStrip ||
      m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStrip ||
      m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::TriangleStripAdjacent ||
      m_Description.m_GraphicsPipeline.m_PrimitiveTopology == xiiGALPrimitiveTopology::LineStripAdjacent)
  {
    pipelineStateDescription.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
  }
  else
  {
    pipelineStateDescription.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
  }

  const xiiGALRenderPassCreationDescription& renderPassDescription = m_Description.m_GraphicsPipeline.m_pRenderPass->GetDescription();
  if (m_Description.m_GraphicsPipeline.m_uiSubpassIndex >= renderPassDescription.m_SubPasses.GetCount())
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': subpass index {} is out of bounds ({} subpasses).", GetDebugName(), m_Description.m_GraphicsPipeline.m_uiSubpassIndex, renderPassDescription.m_SubPasses.GetCount());
    return XII_FAILURE;
  }

  const xiiGALSubPassDescription& subpassDescription = renderPassDescription.m_SubPasses[m_Description.m_GraphicsPipeline.m_uiSubpassIndex];

  pipelineStateDescription.NumRenderTargets = xiiMath::Min<xiiUInt32>(subpassDescription.m_RenderTargetAttachments.GetCount(), D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
  for (xiiUInt32 i = 0U; i < pipelineStateDescription.NumRenderTargets; ++i)
  {
    const xiiGALAttachmentReferenceDescription& attachmentReference = subpassDescription.m_RenderTargetAttachments[i];
    if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
    {
      pipelineStateDescription.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
      continue;
    }

    if (attachmentReference.m_uiAttachmentIndex >= renderPassDescription.m_Attachments.GetCount())
    {
      xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': render target attachment index {} is out of bounds ({} attachments).", GetDebugName(), attachmentReference.m_uiAttachmentIndex, renderPassDescription.m_Attachments.GetCount());
      return XII_FAILURE;
    }

    const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[attachmentReference.m_uiAttachmentIndex];
    pipelineStateDescription.RTVFormats[i]                             = xiiD3D12TypeConversions::GetFormat(attachmentDescription.m_Format);
  }

  if (!subpassDescription.m_DepthStencilAttachment.IsEmpty())
  {
    const xiiGALAttachmentReferenceDescription& attachmentReference = subpassDescription.m_DepthStencilAttachment[0U];
    if (attachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
    {
      if (attachmentReference.m_uiAttachmentIndex >= renderPassDescription.m_Attachments.GetCount())
      {
        xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': depth-stencil attachment index {} is out of bounds ({} attachments).", GetDebugName(), attachmentReference.m_uiAttachmentIndex, renderPassDescription.m_Attachments.GetCount());
        return XII_FAILURE;
      }

      const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[attachmentReference.m_uiAttachmentIndex];
      pipelineStateDescription.DSVFormat                                 = xiiD3D12TypeConversions::GetFormat(attachmentDescription.m_Format);
    }
  }

  pipelineStateDescription.SampleDesc.Count   = xiiMath::Max(1U, static_cast<xiiUInt32>(m_Description.m_GraphicsPipeline.m_SampleDescription.m_uiCount));
  pipelineStateDescription.SampleDesc.Quality = m_Description.m_GraphicsPipeline.m_SampleDescription.m_uiQuality;

  const HRESULT hResult = pD3D12Device->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&m_pD3D12PipelineState));
  if (FAILED(hResult) || m_pD3D12PipelineState == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 graphics pipeline '{}': {}.", GetDebugName(), xiiHRESULTtoString(hResult));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALGraphicsPipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder sb;
  const char*      szName       = sName.GetData(sb);
  const xiiUInt32  uiNameLength = static_cast<xiiUInt32>(sName.GetElementCount());

  if (m_pD3D12PipelineState != nullptr)
  {
    if (FAILED(m_pD3D12PipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set D3D12 graphics pipeline debug name '{}'.", sName);
    }
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    xiiStringBuilder rootSignatureName;
    rootSignatureName.SetFormat("{} (Root Signature)", sName);
    if (FAILED(m_pD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<xiiUInt32>(rootSignatureName.GetElementCount()), rootSignatureName.GetData())))
    {
      xiiLog::Error("Failed to set D3D12 graphics root signature debug name '{}'.", sName);
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_GraphicsPipelineStateD3D12);
