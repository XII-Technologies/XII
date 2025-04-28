#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandQueue::xiiGALCommandQueue(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALDeviceObject(pDevice), m_Description(creationDescription)
{
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
