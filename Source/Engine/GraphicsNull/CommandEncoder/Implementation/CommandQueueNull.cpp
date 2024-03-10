#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

xiiGALCommandQueueNull::xiiGALCommandQueueNull() :
  xiiGALCommandQueue()
{
}

xiiGALCommandQueueNull::~xiiGALCommandQueueNull() = default;

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandQueueNull);
