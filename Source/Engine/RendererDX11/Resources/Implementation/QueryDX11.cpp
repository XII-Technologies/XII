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
  if (m_Description.m_type == xiiGALQueryType::AnySamplesPassed)
    desc.MiscFlags = m_Description.m_bDrawIfUnknown ? D3D11_QUERY_MISC_PREDICATEHINT : 0;
  else
    desc.MiscFlags = 0;

  switch (m_Description.m_type)
  {
    case xiiGALQueryType::NumSamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION;
      break;
    case xiiGALQueryType::AnySamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&desc, &m_pDXQuery)))
  {
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

void xiiGALQueryDX11::SetDebugNamePlatform(const char* szName) const
{
  xiiUInt32 uiLength = xiiStringUtils::GetStringElementCount(szName);

  if (m_pDXQuery != nullptr)
  {
    m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

XII_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_QueryDX11);
