#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandQueue::xiiGALCommandQueue(xiiGALDevice* pDevice) :
  xiiGALDeviceObject(pDevice)
{
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
