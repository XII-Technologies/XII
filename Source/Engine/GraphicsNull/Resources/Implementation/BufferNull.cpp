#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BufferNull.h>
#include <GraphicsNull/Resources/BufferViewNull.h>

xiiGALBufferNull::xiiGALBufferNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(pDeviceNull, creationDescription)
{
}

xiiGALBufferNull::~xiiGALBufferNull() = default;

xiiResult xiiGALBufferNull::InitPlatform(const xiiGALBufferData* pInitialData)
{
  XII_IGNORE_UNUSED(pInitialData);
  return XII_SUCCESS;
}

xiiResult xiiGALBufferNull::DeInitPlatform()
{
  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALBufferView> xiiGALBufferNull::CreateViewPlatform(const xiiGALBufferViewCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceNull>                 pDeviceNull     = m_pDevice.Downcast<xiiGALDeviceNull>();
  xiiInternal::NewInstance<xiiGALBufferViewNull> pBufferViewNull = XII_NEW(pDeviceNull->GetAllocator(), xiiGALBufferViewNull, pDeviceNull, xiiSharedPtr<xiiGALBuffer>(this, pDeviceNull->GetAllocator()), description);

  if (pBufferViewNull->InitPlatform().Succeeded())
    return pBufferViewNull;

  XII_DELETE(pBufferViewNull.m_pAllocator, pBufferViewNull.m_pInstance);

  return pBufferViewNull;
}

void xiiGALBufferNull::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
}

void xiiGALBufferNull::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  XII_IGNORE_UNUSED(uiStartOffset);
  XII_IGNORE_UNUSED(uiSize);
}

xiiGALSparseBufferProperties xiiGALBufferNull::GetSparseProperties() const
{
  return xiiGALSparseBufferProperties();
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_BufferNull);
