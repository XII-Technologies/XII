#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>

xiiGALShaderDiligent::xiiGALShaderDiligent(const xiiGALShaderCreationDescription& Description) :
  xiiGALShader(Description)
{
}

xiiGALShaderDiligent::~xiiGALShaderDiligent() {}

xiiResult xiiGALShaderDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  xiiArrayPtr<const xiiUInt8> byteCode[xiiGALShaderStage::ENUM_COUNT];

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    if (!m_Description.HasByteCodeForStage((xiiGALShaderStage::Enum)i))
      continue;

    xiiArrayPtr<const xiiUInt8> metaData(reinterpret_cast<const xiiUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());

    // Only the vertex shader stores vertexInputAttributes, so passing in the array into other shaders is just a no op.
    xiiShaderMetaData::Read(metaData, byteCode[i], m_DescriptorSets[i], m_VertexInputAttributes);

    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name                    = m_Description.m_szName;
    ShaderCI.Desc.ShaderType              = xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)i);
    ShaderCI.ByteCode                     = reinterpret_cast<const void*>(byteCode[i].GetPtr());
    ShaderCI.ByteCodeSize                 = byteCode[i].GetCount();
    ShaderCI.SourceLanguage               = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.LoadConstantBufferReflection = false;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pShaderStages[i]);

    if (m_pShaderStages[i] == nullptr)
    {
      xiiLog::Error("Couldn't create native shader from bytecode from type: {}.", (xiiGALShaderStage::Enum)i);
      return XII_FAILURE;
    }
  }

  xiiHybridArray<Diligent::PipelineResourceDesc, 2u> resources;

  // Determine resource count to prevent reallocation.
  xiiUInt32 uiResourceCount = 0;
  for (xiiUInt32 uiStage = 0; uiStage < xiiGALShaderStage::ENUM_COUNT; ++uiStage)
  {
    auto& set = m_DescriptorSets[(xiiGALShaderStage::Enum)uiStage];

    for (xiiUInt32 i = 0; i < set.GetCount(); ++i)
    {
      uiResourceCount += set[i].Bindings.GetCount();
    }
  }
  xiiUInt32 uiResourceIndex = 0u;
  resources.SetCount(uiResourceCount);

  xiiUInt32 uiResourceNameIndex = 0u;
  m_StringStorage.SetCount(uiResourceCount + 1u); // Pipeline Signature + Resource names

  Diligent::PipelineResourceSignatureDesc pipelineResourceSignatureDesc;
  {
    xiiStringBuilder sResourceName    = m_Description.m_szName;
    auto&            sResourceNameRef = m_StringStorage[uiResourceNameIndex];
    sResourceNameRef                  = sResourceName;
  }
  pipelineResourceSignatureDesc.Name = m_StringStorage[uiResourceNameIndex].GetData();

  for (xiiUInt32 uiStage = 0; uiStage < xiiGALShaderStage::ENUM_COUNT; ++uiStage)
  {
    auto& set = m_DescriptorSets[(xiiGALShaderStage::Enum)uiStage];

    for (xiiUInt32 i = 0; i < set.GetCount(); ++i)
    {
      auto& bindings = set[i].Bindings;

      for (xiiUInt32 j = 0; j < bindings.GetCount(); ++j)
      {
        auto& currentBinding = bindings[j];
        {
          xiiStringBuilder sResourceName    = currentBinding.m_sName;
          auto&            sResourceNameRef = m_StringStorage[++uiResourceNameIndex];
          sResourceNameRef                  = sResourceName;
        }

        // Pipeline signature description
        {
          auto& resourceDesc = resources[uiResourceIndex];

          resourceDesc.Name         = m_StringStorage[uiResourceNameIndex].GetData();
          resourceDesc.ShaderStages = xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)uiStage);
          resourceDesc.ArraySize    = currentBinding.m_uiArraySize;

          switch (currentBinding.m_Type)
          {
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::ConstantBuffer:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewTexture:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceViewBuffer:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewTexture:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessViewBuffer:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::Sampler:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
              break;
            case xiiShaderDescriptorSetLayoutBinding::ResourceType::AccelerationStructure:
              resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_ACCEL_STRUCT;
              break;
          }

          resourceDesc.VarType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE; // Variables are always mutable for now.
          resourceDesc.Flags   = Diligent::PIPELINE_RESOURCE_FLAG_NONE;           // Not yet assessed
        }

        ++uiResourceIndex;
      }
    }
  }

  pipelineResourceSignatureDesc.Resources                  = resources.GetData();
  pipelineResourceSignatureDesc.NumResources               = resources.GetCount();
  pipelineResourceSignatureDesc.ImmutableSamplers          = nullptr;
  pipelineResourceSignatureDesc.NumImmutableSamplers       = 0;
  pipelineResourceSignatureDesc.BindingIndex               = 0;
  pipelineResourceSignatureDesc.UseCombinedTextureSamplers = false; // Handled by XII
  pipelineResourceSignatureDesc.SRBAllocationGranularity   = 1;     // Default

  pDeviceDiligent->GetDevice()->CreatePipelineResourceSignature(pipelineResourceSignatureDesc, &m_pPipelineResourceSignature);

  if (m_pPipelineResourceSignature == nullptr)
  {
    xiiLog::Error("Failed to create pipeline resource for shader {}", m_Description.m_szName);
    return XII_FAILURE;
  }

  m_uiPipelineResourceSignatureCount = 1u;

  return XII_SUCCESS;
}

xiiResult xiiGALShaderDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    pDeviceDiligent->DeleteLater({xiiResourceObjectType::Shader, m_pShaderStages[i]});
  }

  pDeviceDiligent->DeleteLater({xiiResourceObjectType::Shader, m_pPipelineResourceSignature});
  m_uiPipelineResourceSignatureCount = 0u;

  m_StringStorage.Clear();
  m_DescriptorSets->Clear();
  m_VertexInputAttributes.Clear();

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_ShaderDiligent);
