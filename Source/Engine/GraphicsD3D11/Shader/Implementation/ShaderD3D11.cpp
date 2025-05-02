#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALShaderD3D11::xiiGALShaderD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceD3D11, creationDescription)
{
}

xiiGALShaderD3D11::~xiiGALShaderD3D11()
{
  XII_GAL_D3D11_RELEASE(m_pD3D11Shader);
}

xiiResult xiiGALShaderD3D11::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  auto&   byteCode = m_Description.m_ByteCode;
  HRESULT hResult  = E_FAIL;

  switch (m_Description.m_ShaderType.GetValue())
  {
    case xiiGALShaderType::Vertex:
    {
      ID3D11VertexShader* pD3D11VertexShader = nullptr;
      hResult                                = pDeviceD3D11->GetD3D11Device()->CreateVertexShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11VertexShader);
      m_pD3D11Shader                         = pD3D11VertexShader;
    }
    break;
    case xiiGALShaderType::Pixel:
    {
      ID3D11PixelShader* pD3D11PixelShader = nullptr;
      hResult                              = pDeviceD3D11->GetD3D11Device()->CreatePixelShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11PixelShader);
      m_pD3D11Shader                       = pD3D11PixelShader;
    }
    break;
    case xiiGALShaderType::Geometry:
    {
      ID3D11GeometryShader* pD3D11GeometryShader = nullptr;
      hResult                                    = pDeviceD3D11->GetD3D11Device()->CreateGeometryShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11GeometryShader);
      m_pD3D11Shader                             = pD3D11GeometryShader;
    }
    break;
    case xiiGALShaderType::Hull:
    {
      ID3D11HullShader* pD3D11HullShader = nullptr;
      hResult                            = pDeviceD3D11->GetD3D11Device()->CreateHullShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11HullShader);
      m_pD3D11Shader                     = pD3D11HullShader;
    }
    break;
    case xiiGALShaderType::Domain:
    {
      ID3D11DomainShader* pD3D11DomainShader = nullptr;
      hResult                                = pDeviceD3D11->GetD3D11Device()->CreateDomainShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11DomainShader);
      m_pD3D11Shader                         = pD3D11DomainShader;
    }
    break;
    case xiiGALShaderType::Compute:
    {
      ID3D11ComputeShader* pD3D11ComputeShader = nullptr;
      hResult                                  = pDeviceD3D11->GetD3D11Device()->CreateComputeShader(byteCode->GetByteCode(), byteCode->GetSize(), nullptr, &pD3D11ComputeShader);
      m_pD3D11Shader                           = pD3D11ComputeShader;
    }
    break;
    default:
    {
      xiiLog::Error("Unsupported shader type");
      return XII_FAILURE;
    }
    break;
  }

  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to create Direct3D11 shader: {}", xiiHRESULTtoString(hResult));
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALInputLayout> xiiGALShaderD3D11::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D11>                  pDeviceD3D11      = m_pDevice.Downcast<xiiGALDeviceD3D11>();
  xiiInternal::NewInstance<xiiGALInputLayoutD3D11> pInputLayoutD3D11 = XII_NEW(pDeviceD3D11->GetAllocator(), xiiGALInputLayoutD3D11, pDeviceD3D11, description);

  if (pInputLayoutD3D11->InitPlatform(this).Succeeded())
    return pInputLayoutD3D11;

  XII_DELETE(pDeviceD3D11->GetAllocator(), pInputLayoutD3D11.m_pInstance);

  return pInputLayoutD3D11;
}

void xiiGALShaderD3D11::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_pD3D11Shader != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pD3D11Shader->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D11 shader debug name.");
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Shader_Implementation_ShaderD3D11);
