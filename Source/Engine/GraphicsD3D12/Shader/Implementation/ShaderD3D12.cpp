#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

xiiGALShaderD3D12::xiiGALShaderD3D12(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderD3D12::~xiiGALShaderD3D12() = default;

xiiResult xiiGALShaderD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

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

    pDeviceD3D12->GetDevice()->CreateShader(shaderDescription, &m_pShaderStages[i]);

    if (m_pShaderStages[i] == nullptr)
    {
      xiiLog::Error("Failed to create native shader from bytecode from type: {}.", xiiGALShaderStage::GetStageFlag(i));
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALShaderD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(m_pShaderStages[i]);
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderD3D12);
