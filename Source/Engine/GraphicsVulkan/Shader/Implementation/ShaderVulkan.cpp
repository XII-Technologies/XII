#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALShaderVulkan::xiiGALShaderVulkan(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() = default;

xiiResult xiiGALShaderVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    if (!m_Description.HasByteCodeForStage(xiiGALShaderStage::GetStageFlag(i)))
      continue;

    Diligent::ShaderCreateInfo shaderDescription;
    shaderDescription.Desc.Name                    = m_Description.m_sName.GetStartPointer();
    shaderDescription.Desc.ShaderType              = xiiDiligentTypeConversions::GetShaderTypeFlags(xiiGALShaderStage::GetStageFlag(i));
    shaderDescription.ByteCode                     = m_Description.m_ByteCodes[i]->GetByteCode();
    shaderDescription.ByteCodeSize                 = m_Description.m_ByteCodes[i]->GetSize();
    shaderDescription.SourceLanguage               = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    shaderDescription.LoadConstantBufferReflection = false;

    pDeviceVulkan->GetDevice()->CreateShader(shaderDescription, &m_pShaderStages[i]);

    if (m_pShaderStages[i] == nullptr)
    {
      xiiLog::Error("Failed to create native shader from bytecode from type: {}.", xiiGALShaderStage::GetStageFlag(i));
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALShaderVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(m_pShaderStages[i]);
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
