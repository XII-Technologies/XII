#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

xiiGALCommandQueue::xiiGALCommandQueue(const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALResource<xiiGALCommandQueueCreationDescription>(creationDescription)
{
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);
