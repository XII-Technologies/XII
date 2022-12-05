#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>

xiiGALShaderDiligent::xiiGALShaderDiligent(const xiiGALShaderCreationDescription& Description) :
  xiiGALShader(Description), m_pVertexShader(nullptr), m_pHullShader(nullptr), m_pDomainShader(nullptr), m_pGeometryShader(nullptr), m_pPixelShader(nullptr), m_pComputeShader(nullptr)
{
}

xiiGALShaderDiligent::~xiiGALShaderDiligent() {}

xiiResult xiiGALShaderDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::VertexShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::VertexShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::VertexShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pVertexShader);

    if (m_pVertexShader == nullptr)
    {
      xiiLog::Error("Couldn't create native vertex shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::HullShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_HULL;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::HullShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::HullShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pHullShader);

    if (m_pHullShader == nullptr)
    {
      xiiLog::Error("Couldn't create native hull shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::DomainShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_DOMAIN;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::DomainShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::DomainShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pDomainShader);

    if (m_pDomainShader == nullptr)
    {
      xiiLog::Error("Couldn't create native domain shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::GeometryShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_GEOMETRY;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::GeometryShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::GeometryShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pGeometryShader);

    if (m_pGeometryShader == nullptr)
    {
      xiiLog::Error("Couldn't create native geometry shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::PixelShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::PixelShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::PixelShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pPixelShader);

    if (m_pPixelShader == nullptr)
    {
      xiiLog::Error("Couldn't create native pixel shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::ComputeShader))
  {
    Diligent::ShaderCreateInfo ShaderCI;
    ShaderCI.Desc.Name       = m_Description.m_szName;
    ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_COMPUTE;
    ShaderCI.ByteCode        = m_Description.m_ByteCodes[xiiGALShaderStage::ComputeShader]->GetByteCode();
    ShaderCI.ByteCodeSize    = m_Description.m_ByteCodes[xiiGALShaderStage::ComputeShader]->GetSize();
    ShaderCI.HLSLVersion     = {5, 0};
    ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;

    pDeviceDiligent->GetDevice()->CreateShader(ShaderCI, &m_pComputeShader);

    if (m_pComputeShader == nullptr)
    {
      xiiLog::Error("Couldn't create native compute shader from bytecode!");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_RELEASE(m_pVertexShader);
  XII_GAL_DILIGENT_RELEASE(m_pHullShader);
  XII_GAL_DILIGENT_RELEASE(m_pDomainShader);
  XII_GAL_DILIGENT_RELEASE(m_pGeometryShader);
  XII_GAL_DILIGENT_RELEASE(m_pPixelShader);
  XII_GAL_DILIGENT_RELEASE(m_pComputeShader);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Shader_Implementation_ShaderDiligent);
