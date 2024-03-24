#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineResourceSignature.h>

xiiGALPipelineResourceSignatureVulkan::xiiGALPipelineResourceSignatureVulkan(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(creationDescription)
{
}

xiiGALPipelineResourceSignatureVulkan::~xiiGALPipelineResourceSignatureVulkan() = default;

xiiResult xiiGALPipelineResourceSignatureVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::PipelineResourceSignatureDesc pipelineResourceSignatureDescription = {};
  pipelineResourceSignatureDescription.BindingIndex                            = m_Description.m_uiBindingIndex;
  pipelineResourceSignatureDescription.UseCombinedTextureSamplers              = m_Description.m_bUseCombinedTextureSamplers;
  pipelineResourceSignatureDescription.CombinedSamplerSuffix                   = m_Description.m_sCombinedSamplerSuffix.GetStartPointer();
  pipelineResourceSignatureDescription.SRBAllocationGranularity                = 1U; // Default

  for (xiiUInt32 i = 0; i < m_Description.m_Resources.GetCount(); ++i)
  {
    const auto& sourceDescription   = m_Description.m_Resources[i];
    auto&       resourceDescription = m_PipelineResourceDescriptions.ExpandAndGetRef();

    resourceDescription.Name         = sourceDescription.m_sName;
    resourceDescription.ShaderStages = xiiDiligentTypeConversions::GetShaderTypeFlags(sourceDescription.m_ShaderStages);
    resourceDescription.ArraySize    = sourceDescription.m_uiArraySize;
    resourceDescription.ResourceType = xiiDiligentTypeConversions::GetShaderResourceType(sourceDescription.m_ResourceType);
    resourceDescription.VarType      = xiiDiligentTypeConversions::GetShaderResourceVariableType(sourceDescription.m_ResourceVariableType);
    resourceDescription.Flags        = xiiDiligentTypeConversions::GetPipelineResourceFlags(sourceDescription.m_PipelineResourceFlags);
  }

  for (xiiUInt32 i = 0; i < m_Description.m_ImmutableSamplers.GetCount(); ++i)
  {
    const auto& sourceSampler    = m_Description.m_ImmutableSamplers[i];
    auto&       immutableSampler = m_ImmutableSamplers.ExpandAndGetRef();

    immutableSampler.ShaderStages         = xiiDiligentTypeConversions::GetShaderTypeFlags(sourceSampler.m_ShaderStages);
    immutableSampler.SamplerOrTextureName = sourceSampler.m_SamplerOrTextureName;

    immutableSampler.Desc.AddressU       = xiiDiligentTypeConversions::GetTextureAddress(sourceSampler.m_SamplerDescription.m_AddressU);
    immutableSampler.Desc.AddressV       = xiiDiligentTypeConversions::GetTextureAddress(sourceSampler.m_SamplerDescription.m_AddressV);
    immutableSampler.Desc.AddressW       = xiiDiligentTypeConversions::GetTextureAddress(sourceSampler.m_SamplerDescription.m_AddressW);
    immutableSampler.Desc.BorderColor[0] = sourceSampler.m_SamplerDescription.m_BorderColor.r;
    immutableSampler.Desc.BorderColor[1] = sourceSampler.m_SamplerDescription.m_BorderColor.g;
    immutableSampler.Desc.BorderColor[2] = sourceSampler.m_SamplerDescription.m_BorderColor.b;
    immutableSampler.Desc.BorderColor[3] = sourceSampler.m_SamplerDescription.m_BorderColor.a;
    immutableSampler.Desc.ComparisonFunc = xiiDiligentTypeConversions::GetComparisonFunc(sourceSampler.m_SamplerDescription.m_ComparisonFunction);

    if (sourceSampler.m_SamplerDescription.m_MagFilter == xiiGALFilterType::Anisotropic || sourceSampler.m_SamplerDescription.m_MinFilter == xiiGALFilterType::Anisotropic || sourceSampler.m_SamplerDescription.m_MipFilter == xiiGALFilterType::Anisotropic)
    {
      if (sourceSampler.m_SamplerDescription.m_ComparisonFunction == xiiGALComparisonFunction::Never)
      {
        immutableSampler.Desc.MinFilter = immutableSampler.Desc.MagFilter = immutableSampler.Desc.MipFilter = Diligent::FILTER_TYPE_ANISOTROPIC;
      }
      else
      {
        immutableSampler.Desc.MinFilter = immutableSampler.Desc.MagFilter = immutableSampler.Desc.MipFilter = Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC;
      }
    }
    else
    {
      immutableSampler.Desc.MinFilter = xiiDiligentTypeConversions::GetFilter(sourceSampler.m_SamplerDescription.m_MinFilter);
      immutableSampler.Desc.MagFilter = xiiDiligentTypeConversions::GetFilter(sourceSampler.m_SamplerDescription.m_MagFilter);
      immutableSampler.Desc.MipFilter = xiiDiligentTypeConversions::GetFilter(sourceSampler.m_SamplerDescription.m_MipFilter);
    }

    immutableSampler.Desc.MaxAnisotropy = sourceSampler.m_SamplerDescription.m_uiMaxAnisotropy;
    immutableSampler.Desc.MinLOD        = sourceSampler.m_SamplerDescription.m_fMinLOD;
    immutableSampler.Desc.MaxLOD        = sourceSampler.m_SamplerDescription.m_fMaxLOD;
    immutableSampler.Desc.MipLODBias    = sourceSampler.m_SamplerDescription.m_fMipLODBias;
  }

  pipelineResourceSignatureDescription.NumResources         = m_PipelineResourceDescriptions.GetCount();
  pipelineResourceSignatureDescription.Resources            = m_PipelineResourceDescriptions.GetData();
  pipelineResourceSignatureDescription.NumImmutableSamplers = m_ImmutableSamplers.GetCount();
  pipelineResourceSignatureDescription.ImmutableSamplers    = m_ImmutableSamplers.GetData();

  pDeviceVulkan->GetDevice()->CreatePipelineResourceSignature(pipelineResourceSignatureDescription, &m_pPipelineResourceSignature);

  return (m_pPipelineResourceSignature != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALPipelineResourceSignatureVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pPipelineResourceSignature);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineResourceSignatureVulkan);
