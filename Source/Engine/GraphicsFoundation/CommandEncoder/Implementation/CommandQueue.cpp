#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueue::xiiGALCommandQueue() :
  xiiGALDeviceObject()
{
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
