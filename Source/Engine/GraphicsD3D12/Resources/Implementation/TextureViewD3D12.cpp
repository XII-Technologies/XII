/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTextureViewD3D12::xiiGALTextureViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(std::move(pDeviceD3D12), std::move(pTexture), creationDescription)
{
}

xiiGALTextureViewD3D12::~xiiGALTextureViewD3D12() = default;

xiiResult xiiGALTextureViewD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALTextureViewD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureViewD3D12);
