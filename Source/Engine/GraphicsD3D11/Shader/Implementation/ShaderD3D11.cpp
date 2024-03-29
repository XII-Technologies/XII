#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>

#include <GraphicsD3D11/Utilities/D3D11TypeConversions.h>

xiiGALShaderD3D11::xiiGALShaderD3D11(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderD3D11::~xiiGALShaderD3D11() = default;

xiiResult xiiGALShaderD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

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

    pDeviceD3D11->GetDevice()->CreateShader(shaderDescription, &m_pShaderStages[i]);

    if (m_pShaderStages[i] == nullptr)
    {
      xiiLog::Error("Failed to create native shader from bytecode from type: {}.", xiiGALShaderStage::GetStageFlag(i));
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALShaderD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_D3D11_RELEASE(m_pShaderStages[i]);
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Shader_Implementation_ShaderD3D11);
