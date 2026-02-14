#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFramebufferD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALFramebufferD3D12::xiiGALFramebufferD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(pDeviceD3D12, creationDescription)
{
}

xiiGALFramebufferD3D12::~xiiGALFramebufferD3D12() = default;

xiiResult xiiGALFramebufferD3D12::InitPlatform()
{
  return XII_SUCCESS;
}

xiiResult xiiGALFramebufferD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_FramebufferD3D12);
