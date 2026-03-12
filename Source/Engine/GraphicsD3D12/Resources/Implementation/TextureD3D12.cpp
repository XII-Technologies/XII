#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTextureD3D12::xiiGALTextureD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALTextureD3D12::~xiiGALTextureD3D12() = default;

xiiResult xiiGALTextureD3D12::InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  XII_IGNORE_UNUSED(pInitialData);
  XII_IGNORE_UNUSED(externalMemoryKind);
  return XII_FAILURE;
}

xiiInternal::NewInstance<xiiGALTextureView> xiiGALTextureD3D12::CreateViewPlatform(const xiiGALTextureViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D12>                  pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiInternal::NewInstance<xiiGALTextureViewD3D12> pTextureViewD3D12 = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALTextureViewD3D12, pDeviceD3D12, xiiSharedPtr<xiiGALTexture>(this, pDeviceD3D12->GetAllocator()), description);

  if (pTextureViewD3D12->InitPlatform().Succeeded())
    return pTextureViewD3D12;

  XII_DELETE(pTextureViewD3D12.m_pAllocator, pTextureViewD3D12.m_pInstance);

  return pTextureViewD3D12;
}

void xiiGALTextureD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

const xiiGALSparseTextureProperties& xiiGALTextureD3D12::GetSparseProperties() const
{
  return m_SparseTextureProperties;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureD3D12);
