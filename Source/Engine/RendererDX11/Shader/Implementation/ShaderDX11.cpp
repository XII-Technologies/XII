#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>

#include <d3d11.h>

xiiGALShaderDX11::xiiGALShaderDX11(const xiiGALShaderCreationDescription& Description) :
  xiiGALShader(Description), m_pVertexShader(nullptr), m_pHullShader(nullptr), m_pDomainShader(nullptr), m_pGeometryShader(nullptr), m_pPixelShader(nullptr), m_pComputeShader(nullptr)
{
}

xiiGALShaderDX11::~xiiGALShaderDX11() {}

xiiResult xiiGALShaderDX11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDX11* pDXDevice    = static_cast<xiiGALDeviceDX11*>(pDevice);
  ID3D11Device*     pD3D11Device = pDXDevice->GetDXDevice();

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::VertexShader))
  {
    if (FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[xiiGALShaderStage::VertexShader]->GetByteCode(),
                                                m_Description.m_ByteCodes[xiiGALShaderStage::VertexShader]->GetSize(), nullptr, &m_pVertexShader)))
    {
      xiiLog::Error("Couldn't create native vertex shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::HullShader))
  {
    if (FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[xiiGALShaderStage::HullShader]->GetByteCode(),
                                              m_Description.m_ByteCodes[xiiGALShaderStage::HullShader]->GetSize(), nullptr, &m_pHullShader)))
    {
      xiiLog::Error("Couldn't create native hull shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::DomainShader))
  {
    if (FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[xiiGALShaderStage::DomainShader]->GetByteCode(),
                                                m_Description.m_ByteCodes[xiiGALShaderStage::DomainShader]->GetSize(), nullptr, &m_pDomainShader)))
    {
      xiiLog::Error("Couldn't create native domain shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::GeometryShader))
  {
    if (FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[xiiGALShaderStage::GeometryShader]->GetByteCode(),
                                                  m_Description.m_ByteCodes[xiiGALShaderStage::GeometryShader]->GetSize(), nullptr, &m_pGeometryShader)))
    {
      xiiLog::Error("Couldn't create native geometry shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::PixelShader))
  {
    if (FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[xiiGALShaderStage::PixelShader]->GetByteCode(),
                                               m_Description.m_ByteCodes[xiiGALShaderStage::PixelShader]->GetSize(), nullptr, &m_pPixelShader)))
    {
      xiiLog::Error("Couldn't create native pixel shader from bytecode!");
      return XII_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(xiiGALShaderStage::ComputeShader))
  {
    if (FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[xiiGALShaderStage::ComputeShader]->GetByteCode(),
                                                 m_Description.m_ByteCodes[xiiGALShaderStage::ComputeShader]->GetSize(), nullptr, &m_pComputeShader)))
    {
      xiiLog::Error("Couldn't create native compute shader from bytecode!");
      return XII_FAILURE;
    }
  }

  {
    xiiUInt32 uiLength = xiiStringUtils::GetStringElementCount(m_Description.m_szName);

    if (m_pVertexShader != nullptr)
    {
      m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    if (m_pHullShader != nullptr)
    {
      m_pHullShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    if (m_pDomainShader != nullptr)
    {
      m_pDomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    if (m_pGeometryShader != nullptr)
    {
      m_pGeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    if (m_pPixelShader != nullptr)
    {
      m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    if (m_pComputeShader != nullptr)
    {
      m_pComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pVertexShader);
  XII_GAL_DX11_RELEASE(m_pHullShader);
  XII_GAL_DX11_RELEASE(m_pDomainShader);
  XII_GAL_DX11_RELEASE(m_pGeometryShader);
  XII_GAL_DX11_RELEASE(m_pPixelShader);
  XII_GAL_DX11_RELEASE(m_pComputeShader);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_ShaderDX11);
