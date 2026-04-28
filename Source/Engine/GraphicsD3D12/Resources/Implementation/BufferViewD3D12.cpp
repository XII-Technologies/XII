/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferViewD3D12::xiiGALBufferViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(std::move(pDeviceD3D12), std::move(pBuffer), creationDescription)
{
}

xiiGALBufferViewD3D12::~xiiGALBufferViewD3D12() = default;

xiiResult xiiGALBufferViewD3D12::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALBufferViewD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferViewD3D12);
