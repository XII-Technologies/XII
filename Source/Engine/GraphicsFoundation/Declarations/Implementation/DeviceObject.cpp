#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDeviceObject, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiGALDeviceObject::xiiGALDeviceObject(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALObject(), m_pDevice(pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Invalid Device provided for device object.");
}

xiiGALDeviceObject::~xiiGALDeviceObject() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Declarations_Implementation_DeviceObject);
