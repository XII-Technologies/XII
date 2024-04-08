#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

xiiGALCommandQueueNull::xiiGALCommandQueueNull(xiiGALDeviceNull* pDeviceNull, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceNull, creationDescription)
{
}

xiiGALCommandQueueNull::~xiiGALCommandQueueNull() = default;

xiiGALCommandList* xiiGALCommandQueueNull::BeginCommandList()
{
  return nullptr;
}

void xiiGALCommandQueueNull::SubmitPlatform(xiiGALCommandList* pCommandList)
{
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandQueueNull);
