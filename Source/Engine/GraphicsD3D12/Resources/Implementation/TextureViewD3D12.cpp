#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTextureViewD3D12::xiiGALTextureViewD3D12(xiiGALDeviceD3D12* pDeviceD3D12, xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pDeviceD3D12, pTexture, creationDescription)
{
}

xiiGALTextureViewD3D12::~xiiGALTextureViewD3D12() = default;

xiiResult xiiGALTextureViewD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D12::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureViewD3D12);
