#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDeviceObject, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE

xiiGALDeviceObject::xiiGALDeviceObject(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALObject(), m_pDevice(std::move(pDevice))
{
  XII_ASSERT_DEBUG(m_pDevice != nullptr, "Invalid Device provided for device object.");
}

xiiGALDeviceObject::~xiiGALDeviceObject() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Declarations_Implementation_DeviceObject);
