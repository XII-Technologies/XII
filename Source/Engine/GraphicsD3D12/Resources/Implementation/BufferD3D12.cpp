/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferD3D12::xiiGALBufferD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALBufferD3D12::~xiiGALBufferD3D12() = default;

xiiResult xiiGALBufferD3D12::InitPlatform(const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind)
{
  XII_IGNORE_UNUSED(pInitialData);
  XII_IGNORE_UNUSED(externalMemoryKind);
  return XII_FAILURE;
}

xiiInternal::NewInstance<xiiGALBufferView> xiiGALBufferD3D12::CreateViewPlatform(const xiiGALBufferViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceD3D12>                 pDeviceD3D12     = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiInternal::NewInstance<xiiGALBufferViewD3D12> pBufferViewD3D12 = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALBufferViewD3D12, pDeviceD3D12, xiiSharedPtr<xiiGALBuffer>(this, pDeviceD3D12->GetAllocator()), description);

  if (pBufferViewD3D12->InitPlatform().Succeeded())
    return pBufferViewD3D12;

  XII_DELETE(pBufferViewD3D12.m_pAllocator, pBufferViewD3D12.m_pInstance);

  return pBufferViewD3D12;
}

void xiiGALBufferD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

void xiiGALBufferD3D12::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
}

void xiiGALBufferD3D12::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
}

xiiGALSparseBufferProperties xiiGALBufferD3D12::GetSparseProperties() const
{
  return xiiGALSparseBufferProperties();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_BufferD3D12);
