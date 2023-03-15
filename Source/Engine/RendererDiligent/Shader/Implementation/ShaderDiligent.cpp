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

  return XII_SUCCESS;
}

xiiResult xiiGALShaderDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pShaderStages[i]);
  }

  m_DescriptorSets->Clear();
  m_VertexInputAttributes.Clear();

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_ShaderDiligent);
