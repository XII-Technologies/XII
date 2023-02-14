#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>

#include <d3d11.h>

xiiGALQueryDX11::xiiGALQueryDX11(const xiiGALQueryCreationDescription& Description) :
  xiiGALQuery(Description), m_pDXQuery(nullptr)
{
}

xiiGALQueryDX11::~xiiGALQueryDX11() {}

xiiResult xiiGALQueryDX11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDX11* pDXDevice = static_cast<xiiGALDeviceDX11*>(pDevice);

  D3D11_QUERY_DESC desc;
  if (m_Description.m_Type == xiiGALQueryType::BinaryOcclusion)
    desc.MiscFlags = m_Description.m_bDrawIfUnknown ? D3D11_QUERY_MISC_PREDICATEHINT : 0;
  else
    desc.MiscFlags = 0;

  switch (m_Description.m_Type)
  {
    case xiiGALQueryType::Occlusion:
      desc.Query = D3D11_QUERY_OCCLUSION;
      break;
    case xiiGALQueryType::BinaryOcclusion:
      desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&desc, &m_pDXQuery)))
  {
    xiiUInt32 uiLength = xiiStringUtils::GetStringElementCount(m_Description.m_szName);

    if (m_pDXQuery != nullptr)
    {
      m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, m_Description.m_szName);
    }

    return XII_SUCCESS;
  }
  else
  {
    xiiLog::Error("Creation of native DirectX query failed!");
    return XII_FAILURE;
  }
}

xiiResult xiiGALQueryDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXQuery);
  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_QueryDX11);
