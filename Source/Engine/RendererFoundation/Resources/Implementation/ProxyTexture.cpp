#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace
{
  xiiGALTextureCreationDescription MakeProxyDesc(const xiiGALTextureCreationDescription& parentDesc, const char* szName)
  {
    xiiGALTextureCreationDescription desc = parentDesc;
    desc.m_Type                           = xiiGALTextureType::Texture2DProxy;
    desc.m_szName                         = szName;
    return desc;
  }
} // namespace

xiiGALProxyTexture::xiiGALProxyTexture(const xiiGALTexture& parentTexture, const char* szName) :
  xiiGALTexture(MakeProxyDesc(parentTexture.GetDescription(), szName)), m_pParentTexture(&parentTexture)
{
}

xiiGALProxyTexture::~xiiGALProxyTexture() {}


const xiiGALResourceBase* xiiGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

xiiResult xiiGALProxyTexture::InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALProxyTexture::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ProxyTexture);
