#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/TextureNull.h>
#include <GraphicsNull/Resources/TextureViewNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTextureNull::xiiGALTextureNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceNull, creationDescription)
{
}

xiiGALTextureNull::~xiiGALTextureNull() = default;

xiiResult xiiGALTextureNull::InitPlatform(const xiiGALTextureData* pInitialData)
{
  XII_IGNORE_UNUSED(pInitialData);
  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALTextureView> xiiGALTextureNull::CreateViewPlatform(const xiiGALTextureViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceNull>                  pDeviceNull      = m_pDevice.Downcast<xiiGALDeviceNull>();
  xiiInternal::NewInstance<xiiGALTextureViewNull> pTextureViewNull = XII_NEW(pDeviceNull->GetAllocator(), xiiGALTextureViewNull, pDeviceNull, xiiSharedPtr<xiiGALTexture>(this, pDeviceNull->GetAllocator()), description);

  if (pTextureViewNull->InitPlatform().Succeeded())
    return pTextureViewNull;

  XII_DELETE(pTextureViewNull.m_pAllocator, pTextureViewNull.m_pInstance);

  return pTextureViewNull;
}

const xiiGALSparseTextureProperties& xiiGALTextureNull::GetSparseProperties() const
{
  static xiiGALSparseTextureProperties temporary;
  return temporary;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_TextureNull);
