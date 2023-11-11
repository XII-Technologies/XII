#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Shader/ShaderD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

xiiBitflags<xiiGALShaderStage> GetShaderStages(xiiBitflags<xiiGALShaderStage> e)
{
  if (e.IsNoFlagSet())
    return xiiGALShaderStage::AllGraphics | xiiGALShaderStage::AllMesh | xiiGALShaderStage::AllRayTracing;
  return e;
}

xiiGALShaderD3D12::xiiGALShaderD3D12(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderD3D12::~xiiGALShaderD3D12() = default;

xiiResult xiiGALShaderD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  xiiArrayPtr<const xiiUInt8> pByteCode[xiiGALShaderStage::ENUM_COUNT];

  xiiBitflags<xiiGALShaderStage> shaderStages = GetShaderStages(m_Description.m_ShaderStage);

  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    if (!m_Description.HasByteCodeForStage(xiiGALShaderStage::GetStageFlag(i)) || !shaderStages.IsSet(xiiGALShaderStage::GetStageFlag(i)))
      continue;

    xiiArrayPtr<const xiiUInt8> metaData(reinterpret_cast<const xiiUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());

    Diligent::ShaderCreateInfo shaderDescription;
    shaderDescription.Desc.Name                    = m_Description.m_sName.GetStartPointer();
    shaderDescription.Desc.ShaderType              = xiiDiligentTypeConversions::GetShaderTypeFlags(xiiGALShaderStage::GetStageFlag(i));
    shaderDescription.ByteCode                     = reinterpret_cast<const void*>(pByteCode[i].GetPtr());
    shaderDescription.ByteCodeSize                 = pByteCode[i].GetCount();
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
    XII_GAL_DILIGENT_REF_RELEASE(m_pShaderStages[i]);
  }

  for (xiiUInt32 i = 0; i < m_PipelineResourceSignatures.GetCount(); ++i)
  {
    XII_GAL_DILIGENT_REF_RELEASE(m_PipelineResourceSignatures[i]);
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderD3D12);
