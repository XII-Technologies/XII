#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/ProxyTexture.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALProxyTexture, xiiGALTexture, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  xiiGALTextureCreationDescription MakeProxyDesc(const xiiGALTextureCreationDescription& parentDesc)
  {
    xiiGALTextureCreationDescription desc = parentDesc;
    desc.m_MiscFlags.Add(xiiGALMiscTextureFlags::Proxy);
    return desc;
  }
} // namespace

xiiGALProxyTexture::xiiGALProxyTexture(const xiiGALTexture& parentTexture) :
  xiiGALTexture(MakeProxyDesc(parentTexture.GetDescription())), m_pParentTexture(&parentTexture)
{
}

xiiGALProxyTexture::~xiiGALProxyTexture() = default;

const xiiGALResourceBase* xiiGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

xiiResult xiiGALProxyTexture::InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALProxyTexture::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

const xiiGALSparseTextureProperties& xiiGALProxyTexture::GetSparseProperties() const
{
  static const xiiGALSparseTextureProperties result = {};
  return result;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_ProxyTexture);
