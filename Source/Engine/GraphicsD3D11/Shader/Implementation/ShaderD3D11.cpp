#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>

#include <GraphicsD3D11/Utilities/D3D11TypeConversions.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALShaderD3D11::xiiGALShaderD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceD3D11, creationDescription)
{
}

xiiGALShaderD3D11::~xiiGALShaderD3D11() = default;

xiiResult xiiGALShaderD3D11::InitPlatform()
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Vertex))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateVertexShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pVertexShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 vertex shader.");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Hull))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateHullShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pHullShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 hull shader.");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Domain))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateDomainShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pDomainShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 domain shader.");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Geometry))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateGeometryShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pGeometryShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 geometry shader.");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Pixel))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreatePixelShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pPixelShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 pixel shader.");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::Compute))
  {
    auto& byteCode = m_Description.m_ByteCodes[xiiGALShaderStage::GetStageIndex(m_Description.m_ShaderStage)];

    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateComputeShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &m_pComputeShader)))
    {
      xiiLog::Error("Failed to create Direct3D11 compute shader.");
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALShaderD3D11::DeInitPlatform()
{
  XII_GAL_D3D11_RELEASE(m_pVertexShader);
  XII_GAL_D3D11_RELEASE(m_pHullShader);
  XII_GAL_D3D11_RELEASE(m_pDomainShader);
  XII_GAL_D3D11_RELEASE(m_pGeometryShader);
  XII_GAL_D3D11_RELEASE(m_pPixelShader);
  XII_GAL_D3D11_RELEASE(m_pComputeShader);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Shader_Implementation_ShaderD3D11);
