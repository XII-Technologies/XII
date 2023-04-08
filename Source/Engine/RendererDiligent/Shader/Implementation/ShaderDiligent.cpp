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

  /// \todo RendererDiligent: Reflect Array Size.
  /// \todo RendererDiligent: Reflect SRV type, either buffer or texture.
  /// \todo RendererDiligent: Reflect UAV type, either buffer or texture.
  /// \todo RendererDiligent: Reflect Input Attachments and Acceleration Structures.
  /// \todo RendererFoundation: Add interface for specifying resource type (dynamic, mutable, static), for choosing a resource variable type.

  xiiHybridArray<Diligent::PipelineResourceDesc, 2u> resources;

  Diligent::PipelineResourceSignatureDesc pipelineResourceSignatureDesc;
  pipelineResourceSignatureDesc.Name = m_Description.m_szName;

  for (xiiUInt32 uiStage = 0; uiStage < xiiGALShaderStage::ENUM_COUNT; ++uiStage)
  {
    auto& set = m_DescriptorSets[(xiiGALShaderStage::Enum)uiStage];

    for (xiiUInt32 i = 0; i < set.GetCount(); ++i)
    {
      auto& bindings = set[i].Bindings;

      for (xiiUInt32 j = 0; j < bindings.GetCount(); ++j)
      {
        auto& currentBinding = bindings[j];
        auto& resourceDesc   = resources.ExpandAndGetRef();

        xiiStringBuilder sData;
        currentBinding.m_sName.GetData(sData);

        resourceDesc.Name         = sData.GetData();
        resourceDesc.ShaderStages = xiiDiligentUtils::GALToDiligentShaderStage((xiiGALShaderStage::Enum)i);
        resourceDesc.ArraySize    = 1;

        switch (currentBinding.m_Type)
        {
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ConstantBuffer:
          {
            resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::ResourceView:
          {
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::UnorderedAccessView:
          {
          }
          break;
          case xiiShaderDescriptorSetLayoutBinding::ResourceType::Sampler:
          {
            resourceDesc.ResourceType = Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
          }
          break;
        }

        resourceDesc.VarType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        resourceDesc.Flags   = Diligent::PIPELINE_RESOURCE_FLAG_NONE; // Not yet assessed
      }
    }
  }

  pDeviceDiligent->GetDevice()->CreatePipelineResourceSignature(pipelineResourceSignatureDesc, &m_pPipelineResourceSignature);

  if (m_pPipelineResourceSignature == nullptr)
  {
    xiiLog::Error("Failed to create pipeline resource for shader {}", m_Description.m_szName);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pShaderStages[i]);
  }

  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pPipelineResourceSignature);

  m_DescriptorSets->Clear();
  m_VertexInputAttributes.Clear();

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_ShaderDiligent);
