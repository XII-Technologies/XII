/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>
#include <GraphicsD3D12/States/ComputePipelineStateD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

namespace
{
  static constexpr xiiUInt32 s_uiComputePipelineMaxRootConstantsDWORDs = 64U;

  [[nodiscard]] bool TryFindImmutableSamplerBindingForComputePipelineRootSignature(const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, const xiiGALImmutableSamplerDescription& immutableSampler, xiiUInt32& out_uiBindSlot, xiiUInt32& out_uiBindSet)
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

  [[nodiscard]] xiiResult CreateComputePipelineRootSignatureFromDescription(ID3D12Device* pD3D12Device, const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription, ID3D12RootSignature*& out_pRootSignature)
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
      if (uiValueCount > s_uiComputePipelineMaxRootConstantsDWORDs)
      {
        xiiLog::Error("Failed to create D3D12 root signature: push constant range {} has {} DWORDs, exceeding D3D12 limit of {} DWORDs.", i, uiValueCount, s_uiComputePipelineMaxRootConstantsDWORDs);
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
      if (!TryFindImmutableSamplerBindingForComputePipelineRootSignature(signatureDescription, immutableSampler, uiBindSlot, uiBindSet))
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
    rootSignatureDescription.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALComputePipelineStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALComputePipelineStateD3D12::xiiGALComputePipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALComputePipelineStateCreationDescription& creationDescription) :
  xiiGALComputePipelineState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALComputePipelineStateD3D12::~xiiGALComputePipelineStateD3D12()
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

xiiResult xiiGALComputePipelineStateD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12Device*                   pD3D12Device = pDeviceD3D12 != nullptr ? pDeviceD3D12->GetD3D12Device() : nullptr;
  if (pD3D12Device == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 compute pipeline '{}': D3D12 device is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  xiiSharedPtr<xiiGALShaderD3D12> pComputeShaderD3D12 = m_Description.m_pComputeShader.Downcast<xiiGALShaderD3D12>();
  if (pComputeShaderD3D12 == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 compute pipeline '{}': compute shader backend type is incompatible.", GetDebugName());
    return XII_FAILURE;
  }

  if (CreateComputePipelineRootSignatureFromDescription(pD3D12Device, m_Description.m_pPipelineResourceSignature->GetDescription(), m_pD3D12RootSignature).Failed())
  {
    xiiLog::Error("Failed to create D3D12 compute pipeline '{}': root signature creation failed.", GetDebugName());
    return XII_FAILURE;
  }

  D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDescription = {};
  pipelineStateDescription.pRootSignature                    = m_pD3D12RootSignature;
  pipelineStateDescription.CS                                = *pComputeShaderD3D12->GetD3D12ShaderByteCodeDescription();

  const HRESULT hResult = pD3D12Device->CreateComputePipelineState(&pipelineStateDescription, IID_PPV_ARGS(&m_pD3D12PipelineState));
  if (FAILED(hResult) || m_pD3D12PipelineState == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 compute pipeline '{}': {}.", GetDebugName(), xiiHRESULTtoString(hResult));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALComputePipelineStateD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder sb;
  const char*      szName       = sName.GetData(sb);
  const xiiUInt32  uiNameLength = static_cast<xiiUInt32>(sName.GetElementCount());

  if (m_pD3D12PipelineState != nullptr)
  {
    if (FAILED(m_pD3D12PipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set D3D12 compute pipeline debug name '{}'.", sName);
    }
  }

  if (m_pD3D12RootSignature != nullptr)
  {
    xiiStringBuilder rootSignatureName;
    rootSignatureName.SetFormat("{} (Root Signature)", sName);
    if (FAILED(m_pD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<xiiUInt32>(rootSignatureName.GetElementCount()), rootSignatureName.GetData())))
    {
      xiiLog::Error("Failed to set D3D12 compute root signature debug name '{}'.", sName);
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_ComputePipelineStateD3D12);
