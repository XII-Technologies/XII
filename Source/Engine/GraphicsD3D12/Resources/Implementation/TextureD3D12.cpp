#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTextureD3D12::xiiGALTextureD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceD3D12, creationDescription)
{
}

xiiGALTextureD3D12::~xiiGALTextureD3D12() = default;

xiiResult xiiGALTextureD3D12::InitPlatform(const xiiGALTextureData* pInitialData)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

const xiiGALSparseTextureProperties& xiiGALTextureD3D12::GetSparseProperties() const
{
  return m_SparseTextureProperties;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureD3D12);
