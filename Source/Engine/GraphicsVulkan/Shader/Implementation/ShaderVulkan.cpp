#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

#include <ShaderCompiler/ShaderMetadata.h>

xiiGALShaderVulkan::xiiGALShaderVulkan(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() = default;

xiiResult xiiGALShaderVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  // Extract meta data and shader byte code.
  xiiArrayPtr<const xiiUInt8> pByteCodes[xiiGALShaderStage::ENUM_COUNT];
  xiiUInt32                   uiBindingCount = 0U;

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    if (!m_Description.HasByteCodeForStage(xiiGALShaderStage::GetStageFlag(i)))
      continue;

    xiiArrayPtr<const xiiUInt8> metaData(reinterpret_cast<const xiiUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());

    // Only the vertex shader stores inputlayouts, so passing in the array into other shaders is just a no op.
    xiiShaderMetaData::Read(metaData, pByteCodes[i], m_ShaderResourceBindings[i], m_VertexInputLayouts);
    uiBindingCount += m_ShaderResourceBindings[i].GetCount();

    Diligent::ShaderCreateInfo shaderDescription;
    shaderDescription.Desc.Name                    = m_Description.m_sName.GetStartPointer();
    shaderDescription.Desc.ShaderType              = xiiDiligentTypeConversions::GetShaderTypeFlags(xiiGALShaderStage::GetStageFlag(i));
    shaderDescription.ByteCode                     = reinterpret_cast<const void*>(pByteCodes[i].GetPtr());
    shaderDescription.ByteCodeSize                 = pByteCodes[i].GetCount();
    shaderDescription.SourceLanguage               = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    shaderDescription.LoadConstantBufferReflection = false;

    pDeviceVulkan->GetDevice()->CreateShader(shaderDescription, &m_pShaderStages[i]);

    if (m_pShaderStages[i] == nullptr)
    {
      xiiLog::Error("Failed to create native shader from bytecode from type: {}.", xiiGALShaderStage::GetStageFlag(i));
      return XII_FAILURE;
    }
  }

  Diligent::PipelineResourceSignatureDesc pipelineResourceSignatureDescription;
  pipelineResourceSignatureDescription.Name                       = m_Description.m_sName.GetStartPointer();
  pipelineResourceSignatureDescription.BindingIndex               = 0U;
  pipelineResourceSignatureDescription.ImmutableSamplers          = nullptr;
  pipelineResourceSignatureDescription.NumImmutableSamplers       = 0U;
  pipelineResourceSignatureDescription.UseCombinedTextureSamplers = false; // Handled by XII.
  pipelineResourceSignatureDescription.SRBAllocationGranularity   = 1U;    // Default.

  xiiHybridArray<Diligent::PipelineResourceDesc, 2U> resources;
  resources.SetCount(uiBindingCount);

  xiiUInt32 uiCurrentResourceIndex             = 0;
  xiiUInt32 uiCurrentPipelineResourceSignature = 0;
  for (xiiUInt32 uiShaderStage = 0; uiShaderStage < xiiGALShaderStage::ENUM_COUNT; ++uiShaderStage)
  {
    for (xiiUInt32 uiBindingIndex = 0; uiBindingIndex < m_ShaderResourceBindings[uiShaderStage].GetCount(); ++uiBindingIndex)
    {
      const xiiGALShaderResourceBinding& resourceBinding     = m_ShaderResourceBindings[uiShaderStage][uiBindingIndex];
      Diligent::PipelineResourceDesc&    resourceDescription = resources[uiCurrentResourceIndex];

      resourceDescription.Name         = resourceBinding.m_sName.GetView().GetStartPointer();
      resourceDescription.ShaderStages = xiiDiligentTypeConversions::GetShaderTypeFlags(xiiGALShaderStage::GetStageFlag(uiShaderStage));
      resourceDescription.ArraySize    = resourceBinding.m_uiArraySize;

      switch (resourceBinding.m_Type)
      {
        case xiiGALShaderResourceType::ConstantBuffer:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER;
          break;
        case xiiGALShaderResourceType::TextureSRV:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV;
          break;
        case xiiGALShaderResourceType::BufferSRV:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV;
          break;
        case xiiGALShaderResourceType::TextureUAV:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV;
          break;
        case xiiGALShaderResourceType::BufferUAV:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV;
          break;
        case xiiGALShaderResourceType::Sampler:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_SAMPLER;
          break;
        case xiiGALShaderResourceType::InputAttachment:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT;
          break;
        case xiiGALShaderResourceType::AccelerationStructure:
          resourceDescription.ResourceType = Diligent::SHADER_RESOURCE_TYPE_ACCEL_STRUCT;
          break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      resourceDescription.VarType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE; // Variables are always mutable for now.
      resourceDescription.Flags   = Diligent::PIPELINE_RESOURCE_FLAG_NONE;           // Not yet assessed.

      ++uiCurrentResourceIndex;
    }
  }

  pipelineResourceSignatureDescription.Resources    = resources.GetData();
  pipelineResourceSignatureDescription.NumResources = resources.GetCount();

  Diligent::IPipelineResourceSignature* pResourceSignature = nullptr;
  pDeviceVulkan->GetDevice()->CreatePipelineResourceSignature(pipelineResourceSignatureDescription, &pResourceSignature);
  m_PipelineResourceSignatures.EnsureCount(uiCurrentPipelineResourceSignature + 1);
  m_PipelineResourceSignatures[uiCurrentPipelineResourceSignature] = pResourceSignature;
  ++uiCurrentPipelineResourceSignature;

  for (xiiUInt32 i = 0; i < m_PipelineResourceSignatures.GetCount(); ++i)
  {
    if (m_PipelineResourceSignatures[i] == nullptr)
    {
      xiiLog::Error("Failed to create pipeline resource signature ({0}) for shader '{1}'.", i, m_Description.m_sName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  ShaderEvent e;
  e.m_Type    = ShaderEvent::BeforeDeletion;
  e.m_pShader = this;

  m_Events.Broadcast(e);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(m_pShaderStages[i]);
  }

  for (xiiUInt32 i = 0; i < m_PipelineResourceSignatures.GetCount(); ++i)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(m_PipelineResourceSignatures[i]);
  }

  m_VertexInputLayouts.Clear();

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ShaderResourceBindings[i].Clear();
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
