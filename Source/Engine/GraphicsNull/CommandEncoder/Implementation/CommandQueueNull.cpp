#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

xiiGALCommandQueueNull::xiiGALCommandQueueNull(xiiGALDeviceNull* pDeviceNull) :
  xiiGALCommandQueue(pDeviceNull)
{
}

xiiGALCommandQueueNull::~xiiGALCommandQueueNull() = default;

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandQueueNull);
